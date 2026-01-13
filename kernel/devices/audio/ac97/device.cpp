#include <kernel/devices/audio/ac97/device.h>
#include <kernel/arch/interrupts.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/ioctl.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>
#include <kernel/fs/devfs/filesystem.h>

#include <std/format.h>

namespace kernel {

RefPtr<AC97Device> AC97Device::create(pci::Device device) {
    if (!device.is_audio_device()) {
        return nullptr;
    }

    return Device::create<AC97Device>(device.address());
}

AC97Device::AC97Device(pci::Address address) : IRQHandler(address.interrupt_line()) {
    m_audio_mixer = io::Port(address.bar(0) & ~1);
    m_audio_bus = io::Port(address.bar(1) & ~1);

    m_audio_output = m_audio_bus.offset(PCMOut);

    address.set_bus_master(true);
    address.set_interrupt_line(true);

    u32 control = m_audio_bus.read<u32>(GlobalControl);
    control |= 0x3;

    m_audio_bus.write<u32>(GlobalControl, control); // Global interrupt enable = 1, Cold reset = 1
    m_audio_mixer.write<u16>(Reset, 1);             // Reset all registers

    u32 global_status = m_audio_bus.read<u32>(GlobalStatus);
    m_channels = (((global_status >> 20) & 0x3) + 1) * 2;

    // Set volume to max
    m_audio_mixer.write<u16>(PCMVolume, 0);
    m_audio_mixer.write<u16>(MasterVolume, 0);
    
    m_audio_mixer.write<u16>(SampleRate, m_sample_rate);
    this->reset();

    m_descriptors = reinterpret_cast<BufferDescriptor*>(MUST(MM->allocate_dma_region(sizeof(BufferDescriptor) * DESCRIPTOR_COUNT)));
    m_output_buffer = reinterpret_cast<u8*>(MUST(MM->allocate_dma_region(OUTPUT_BUFFER_SIZE)));

    u16 extended_capabilities = m_audio_mixer.read<u16>(ExtendedCapabilities);
    if (extended_capabilities & 0x1) {
        m_variable_rate = true;
        extended_capabilities |= 0x1;
    } if (extended_capabilities & 0x2) {
        m_double_rate = true;
        extended_capabilities |= 0x2;
    }

    m_audio_mixer.write<u16>(ExtendedCapabilities, extended_capabilities);
    this->enable_irq();
    
    dbgln("AC97 Device ({}:{}:{}):", address.bus(), address.device(), address.function());
    dbgln(" - Channels: {}", m_channels);
    dbgln(" - Sample Rate: {} Hz", m_sample_rate);
    dbgln(" - Variable Sample Rate: {}", m_variable_rate);
    dbgln(" - Double Rate: {}", m_double_rate);
    dbgln();
}


void AC97Device::reset() {
    m_audio_output.write<u8>(TransferControl, 2); // Reset = 1
    while (m_audio_output.read<u8>(TransferControl) & 1) {
        io::wait(50); // Wait for reset to complete
    }

    m_dma_enabled = false;
    m_current_descriptor = 0;
}

void AC97Device::handle_irq() {
    TransferStatusInfo status = m_audio_output.read<u16>(TransferStatus);
    if (!status.ioc) {
        return;
    }

    m_audio_output.write<u16>(TransferStatus, 0x1C); // Clear the interrupt bits by setting them to 1

    // Check if the DMA caught up
    u8 index = m_audio_output.read<u8>(ProcessedEntries);
    u8 end = m_audio_output.read<u8>(EntryCount);

    if (index == end) {
        this->reset();
    }

    m_irq_blocker.set_value(true);
}

ErrorOr<size_t> AC97Device::read(void*, size_t, size_t) {
    return Error(ENOTSUP);
}

ErrorOr<size_t> AC97Device::write(const void* buffer, size_t count, size_t) {
    size_t offset = 0;
    ssize_t remaining = count;

    while (remaining > 0) {
        this->write_single(buffer, std::min(static_cast<size_t>(remaining), PAGE_SIZE), offset);

        offset += PAGE_SIZE;
        remaining -= PAGE_SIZE;
    }

    return count;
}

void AC97Device::write_single(const void* buffer, size_t count, size_t offset) {
    {
        arch::InterruptDisabler disabler;
        do {
            TransferStatusInfo status = m_audio_output.read<u16>(TransferStatus);

            u8 index = m_audio_output.read<u8>(ProcessedEntries);
            u8 end = m_audio_output.read<u8>(EntryCount);

            u8 buffers = end - index;
            if (index > end)
                buffers = DESCRIPTOR_COUNT - index + end;
            if (!status.controller_status)
                buffers++;

            if (buffers > OUTPUT_BUFFER_PAGES) {
                m_current_descriptor = index + 1;
                break;
            }

            if (buffers < OUTPUT_BUFFER_PAGES) {
                break;
            }
        
            m_irq_blocker.set_value(false);
            m_irq_blocker.wait();
        } while (m_dma_enabled);
    }

    u8* output = m_output_buffer + m_current_page * PAGE_SIZE;
    memcpy(output, reinterpret_cast<const u8*>(buffer) + offset, count);

    u16 samples = count / sizeof(u16);
    BufferDescriptor& descriptor = m_descriptors[m_current_descriptor];

    descriptor.address = static_cast<u32>(MM->get_physical_address(output));
    descriptor.samples = samples;
    descriptor.last_entry = 0;
    descriptor.interrupt_on_completion = 1;

    m_audio_output.write<u32>(BufferDescriptorList, static_cast<u32>(MM->get_physical_address(&descriptor)));
    m_audio_output.write<u8>(EntryCount, m_current_descriptor);

    if (!m_dma_enabled) {
        TransferControlInfo control = m_audio_output.read<u8>(TransferControl);
        control.controller_status = control.ioc_ie = control.fifo_error_ie = 1;

        m_audio_output.write<u8>(TransferControl, control.value);
        m_dma_enabled = true;
    }

    m_current_descriptor = (m_current_descriptor + 1) % DESCRIPTOR_COUNT;
    m_current_page = (m_current_page + 1) % OUTPUT_BUFFER_PAGES;
}

ErrorOr<void> AC97Device::set_sample_rate(u16 sample_rate) {
    if (sample_rate == m_sample_rate) {
        return {};
    }

    if (m_double_rate) {
        sample_rate >>= 1;
    }

    if (!m_variable_rate && (sample_rate != DEFAULT_SAMPLE_RATE)) {
        return Error(EINVAL);
    } else if (MIN_SAMPLE_RATE > sample_rate || sample_rate > MAX_SAMPLE_RATE) {
        return Error(EINVAL);
    }

    m_audio_mixer.write<u16>(SampleRate, sample_rate);
    m_sample_rate = m_audio_mixer.read<u16>(SampleRate);

    return {};
}

}