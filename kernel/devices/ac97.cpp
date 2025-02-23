#include <kernel/devices/ac97.h>
#include <kernel/arch/interrupts.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/ioctl.h>
#include <kernel/io.h>

#include <std/format.h>

namespace kernel::devices {

AC97Device* AC97Device::create() {
    pci::Address address = {};
    pci::enumerate([&address](pci::Device device) {
        if (device.is_audio_device()) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    return new AC97Device(address);
}

AC97Device::AC97Device(pci::Address address) : CharacterDevice(4, 0), IRQHandler(address.interrupt_line()) {
    m_audio_mixer_port = address.bar0() & ~1;
    m_audio_bus_port = address.bar1() & ~1;
    m_audio_output_port = m_audio_bus_port + PCMOut;

    address.set_bus_master(true);
    address.set_interrupt_line(true);

    u32 control = read_bus_reg32(GlobalControl);
    control |= 0x3;

    write_bus_reg32(GlobalControl, control); // Global interrupt enable = 1, Cold reset = 1
    write_mixer_reg(Reset, 1);               // Reset all registers

    u32 global_status = read_bus_reg32(GlobalStatus);
    m_channels = (((global_status >> 20) & 0x3) + 1) * 2;

    // Set volume to max
    write_mixer_reg(PCMVolume, 0);
    write_mixer_reg(MasterVolume, 0);
    
    write_mixer_reg(SampleRate, m_sample_rate);
    this->reset();
    
    m_descriptors = reinterpret_cast<BufferDescriptor*>(MM->allocate_dma_region(sizeof(BufferDescriptor) * DESCRIPTOR_COUNT));
    m_output_buffer = reinterpret_cast<u8*>(MM->allocate_dma_region(OUTPUT_BUFFER_SIZE));

    u16 extended_capabilities = read_mixer_reg(ExtendedCapabilities);
    if (extended_capabilities & 0x1) {
        m_variable_rate = true;
        extended_capabilities |= 0x1;
    } if (extended_capabilities & 0x2) {
        m_double_rate = true;
        extended_capabilities |= 0x2;
    }

    write_mixer_reg(ExtendedCapabilities, extended_capabilities);

    u16 capabilities = read_mixer_reg(Capabilities);
    dbgln("AC97 Device ({}:{}:{}):", address.bus, address.device, address.function);
    dbgln("├ Channels: {}", m_channels);
    dbgln("├ Sample Rate: {} Hz", m_sample_rate);
    dbgln("├ Variable Sample Rate: {}", m_variable_rate);
    dbgln("├ Double Rate: {}", m_double_rate);
    dbgln("├ Capabilities: {:#x}", capabilities);
    dbgln("└ Extended Capabilities: {:#x}\n", extended_capabilities);
}


void AC97Device::reset() {
    write_output_reg8(TransferControl, 2); // Reset = 1
    while (read_output_reg8(TransferControl) & 1) {
        io::wait(50); // Wait for reset to complete
    }

    m_dma_enabled = false;
    m_current_descriptor = 0;
}

void AC97Device::handle_interrupt(arch::InterruptRegisters*) {
    TransferStatusInfo status = read_output_reg16(TransferStatus);
    if (status.fifo_error) {
        dbgln("AC97: FIFO error");
    }

    if (!status.ioc) {
        return;
    }

    write_output_reg16(TransferStatus, 0x1C); // Clear the interrupt bits by setting them to 1

    // Check if the DMA caught up
    u8 index = read_output_reg8(ProcessedEntries);
    u8 end = read_output_reg8(EntryCount);

    if (index == end) {
        this->reset();
    }

    m_irq_blocker.set_value(true);
}

ssize_t AC97Device::read(void*, size_t, size_t) {
    return -ENOTSUP;
}

ssize_t AC97Device::write(const void* buffer, size_t count, size_t) {
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
            TransferStatusInfo status = read_output_reg16(TransferStatus);

            u8 index = read_output_reg8(ProcessedEntries);
            u8 end = read_output_reg8(EntryCount);

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

    write_output_reg32(BufferDescriptorList, static_cast<u32>(MM->get_physical_address(&descriptor)));
    write_output_reg8(EntryCount, m_current_descriptor);

    if (!m_dma_enabled) {
        TransferControlInfo control = read_output_reg8(TransferControl);
        control.controller_status = control.ioc_ie = control.fifo_error_ie = 1;

        write_output_reg8(TransferControl, control.value);
        m_dma_enabled = true;
    }

    m_current_descriptor = (m_current_descriptor + 1) % DESCRIPTOR_COUNT;
    m_current_page = (m_current_page + 1) % OUTPUT_BUFFER_PAGES;
}

bool AC97Device::set_sample_rate(u16 sample_rate) {
    if (sample_rate == m_sample_rate) {
        return true;
    }

    if (m_double_rate) {
        sample_rate >>= 1;
    }

    if (!m_variable_rate && (sample_rate != DEFAULT_SAMPLE_RATE)) {
        return false;
    } else if (MIN_SAMPLE_RATE > sample_rate || sample_rate > MAX_SAMPLE_RATE) {
        return false;
    }

    write_mixer_reg(SampleRate, sample_rate);
    m_sample_rate = read_mixer_reg(SampleRate);

    return true;
}

ErrorOr<int> AC97Device::ioctl(unsigned request, unsigned arg) {
    auto* process = Scheduler::current_process();
    switch (request) {
        // FIXME: Support the rest (SOUNDCARD_SET_VOLUME, SOUNDCARD_GET_VOLUME)
        case SOUNDCARD_GET_CHANNELS: {
            int* argp = reinterpret_cast<int*>(arg);
            process->validate_write(argp, sizeof(int));

            *argp = m_channels;
            return 0;
        }
        case SOUNDCARD_GET_SAMPLE_RATE: {
            int* argp = reinterpret_cast<int*>(arg);
            process->validate_write(argp, sizeof(int));

            *argp = m_sample_rate;
            return 0;
        }
        case SOUNDCARD_SET_SAMPLE_RATE: {
            if (!this->set_sample_rate(arg)) {
                return -EINVAL;
            }

            return 0;
        }
        default:
            return -EINVAL;
    }
}

}