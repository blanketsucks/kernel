#pragma once

#include <kernel/devices/character_device.h>
#include <kernel/arch/irq.h>
#include <kernel/process/blocker.h>
#include <kernel/pci/pci.h>
#include <kernel/arch/io.h>

namespace kernel {

class AC97Device : public CharacterDevice, public IRQHandler {
public:
    static constexpr u16 DEFAULT_SAMPLE_RATE = 48000;

    static constexpr u16 MIN_SAMPLE_RATE = 8000;
    static constexpr u16 MAX_SAMPLE_RATE = 48000;

    static constexpr size_t DESCRIPTOR_COUNT = 32;

    static constexpr size_t OUTPUT_BUFFER_PAGES = 8;
    static constexpr size_t OUTPUT_BUFFER_SIZE = PAGE_SIZE * OUTPUT_BUFFER_PAGES;

    enum AudioMixerRegisters : u16 {
        Reset = 0x00,
        Capabilities = 0x00,
        MasterVolume = 0x02,
        AuxVolume = 0x04,
        MicrophoneVolume = 0x0E,
        PCMVolume = 0x18,
        InputDevice = 0x1A,
        InputGain = 0x1C,
        MicrophoneGain = 0x1E,
        ExtendedCapabilities = 0x2A,
        SampleRate = 0x2C,

        VendorID1 = 0x7C,
        VendorID2 = 0x7E,
    };

    enum AudioBusRegisters : u16 {
        PCMIn = 0x00,
        PCMOut = 0x10,
        Microphone = 0x20,
        GlobalControl = 0x2C,
        GlobalStatus = 0x30,
    };

    // For PCMIn, PCMOut, and Microphone
    enum AudioRegisterBoxRegisters {
        BufferDescriptorList = 0x00,
        ProcessedEntries = 0x04,
        EntryCount = 0x05,
        TransferStatus = 0x06,
        TransferedSamples = 0x08,
        NextEntry = 0x0A,
        TransferControl = 0x0B,
    };

    union TransferStatusInfo {
        struct {
            u8 controller_status : 1; // 0 = Transferring data, 1 = Halted
            u8 end_of_transfer : 1;   // 0 = Current entry is not equal to last valid entry, 1 = CE = LVE
            u8 last_buffer_entry : 1; // 1 = Interrupt is firing
            u8 ioc : 1;               // 1 = Interrupt is firing
            u8 fifo_error : 1;        // 1 = Interrupt is firing
            u16 : 11;
        };

        u16 value;

        TransferStatusInfo(u16 value) : value(value) {}

        operator u16() const { return value; }
    };

    union TransferControlInfo {
        struct {
            u8 controller_status : 1;    // 0 = Pause transferring, 1 = Transfer data
            u8 reset : 1;                // 0 = Remove reset condition, 1 = Reset this NABM register box
            u8 last_buffer_entry_ie : 1; // 0 = Disable interrupt, 1 = Enable interrupt
            u8 ioc_ie : 1;               // 0 = Disable interrupt, 1 = Enable interrupt
            u8 fifo_error_ie : 1;        // 0 = Disable interrupt, 1 = Enable interrupt
            u8 : 3;
        };

        u8 value;

        TransferControlInfo(u8 value) : value(value) {}

        operator u8() const { return value; }
    };

    struct BufferDescriptor {
        u32 address;
        u16 samples;
    
        u16 : 14;
    
        u8 last_entry : 1;
        u8 interrupt_on_completion : 1;
    } PACKED;

    static AC97Device* create();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;

    bool can_read(fs::FileDescriptor const&) const override { return false; }
    bool can_write(fs::FileDescriptor const&) const override { return true; }

    void reset();

    void write_single(const void* data, size_t count, size_t offset);

    bool set_sample_rate(u16 sample_rate);

private:
    friend class Device;

    AC97Device(pci::Address address);

    void handle_irq() override;

    io::Port m_audio_mixer;
    io::Port m_audio_bus;
    io::Port m_audio_output;

    u16 m_channels;
    u16 m_sample_rate = DEFAULT_SAMPLE_RATE;

    BufferDescriptor* m_descriptors;
    u8* m_output_buffer;
    
    u8 m_current_descriptor = 0;
    u8 m_current_page = 0;

    bool m_dma_enabled = false;    
    bool m_variable_rate = false;
    bool m_double_rate = false;

    BooleanBlocker m_irq_blocker;
};

}