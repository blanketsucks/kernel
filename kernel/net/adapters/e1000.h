#pragma once

#include <kernel/common.h>
#include <kernel/arch/irq.h>
#include <kernel/pci/pci.h>

#include <kernel/net/mac.h>
#include <kernel/net/adapter.h>

#include <kernel/process/blocker.h>

#include <std/memory.h>

namespace kernel::net {

// https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf
class E1000NetworkAdapter : public NetworkAdapter, public IRQHandler {
public:
    // Table 13-2
    enum Register : u16 {
        Control = 0x0000,
        Status = 0x0008,
        EEPROM = 0x0014,
        ControlExt = 0x0018,
        InterruptCause = 0x00C0,
        InterruptThrottle = 0x00C4,
        InterruptMask = 0x00D0,

        ReceiveCtrl = 0x0100,
        RxDescriptorLow = 0x2800,
        RxDescriptorHigh = 0x2804,
        RxDescriptorLength = 0x2808,
        RxDescriptorHead = 0x2810,
        RxDescriptorTail = 0x2818,

        TransmitCtrl = 0x0400,
        TxDescriptorLow = 0x3800,
        TxDescriptorHigh = 0x3804,
        TxDescriptorLength = 0x3808,
        TxDescriptorHead = 0x3810,
        TxDescriptorTail = 0x3818,
        TransmitIPG = 0x0410,
    };

    enum CtrlRegister : u32 {
        SLU = 1 << 6, // Set Link Up
    };

    enum InterruptMaskSet : u32 {
        LCS = 1 << 2,  // Link Status Change
        RXO = 1 << 6,  // Receiver FIFO Overrun
        RXT0 = 1 << 7, // RX Timer Interrupt
    };

    enum TransmitCommand : u8 {
        EOP = 1 << 0,  // End of Packet
        IFCS = 1 << 1, // Insert FCS
        IC = 1 << 2,   // Insert Checksum
        RS = 1 << 3,   // Report Status
        RPS = 1 << 4,  // Report Packet Sent
        VLE = 1 << 6,  // VLAN Packet Enable
        IDE = 1 << 7,  // Interrupt Delay Enable
    };

    enum BufferSize {
        BufferSize2048 = 0b00,
        BufferSize1024 = 0b01,
        BufferSize512  = 0b10,
        BufferSize256  = 0b11,

        // With buffer size extension
        BufferSize16384 = 0b01,
        BufferSize8192  = 0b10,
        BufferSize4096  = 0b11,
    };

    enum ReceiveThresholdSize {
        Half = 0b00,
        Quarter = 0b01,
        Eighth = 0b10,
    };

    // Section 13.4.22
    union ReceiveControl {
        struct {
            u8 : 1;
            u8 enable : 1;
            u8 store_bad_packets : 1;
            u8 unicast_promiscuous : 1;
            u8 multicast_promiscuous : 1;
            u8 long_packet_enable : 1;
            u8 loopback_mode : 2;
            u8 receive_descrptor_threshold_size : 2;
            u8 : 2;
            u8 multicast_offset : 2;
            u8 : 1;
            u8 broadcast_accept_mode : 1;
            u8 buffer_size : 2;
            u8 vlan_filter_enable : 1;
            u8 canonical_form_indicator_enable : 1;
            u8 canonical_form_indicator : 1;
            u8 : 1;
            u8 discard_pause_frames : 1;
            u8 pass_mac_control_frames : 1;
            u8 : 1;
            u8 buffer_size_extension : 1;
            u8 strip_ethernet_crc : 1;
            u8 : 5;
        } PACKED;
        
        u32 value;

        ReceiveControl() : value(0) {}
    };

    enum CollisionDistance {
        HalfDuplex = 0x200,
        FullDuplex = 0x40
    };

    // Section 13.4.33
    union TransmitControl {
        struct {
            u8 : 1;
            u8 enable : 1;
            u8 : 1;
            u8 pad_short_packets : 1;
            u16 collision_threshold : 8; // Number of attempts to transmit before giving up.
            u16 collision_distance : 10; // Specifies the minimum number of byte times that must elapse for proper CSMA/CD operation.
            u8 software_xoff_transmission : 1;
            u8 : 1;
            u8 re_transmit_on_late_collision : 1;
            u8 no_re_transmit_on_underrun : 1;
            u8 : 6;
        } PACKED;

        u32 value;

        TransmitControl() : value(0) {}
    };

    // Section 13.4.34.
    // Measured in increments of the MAC clock.
    // • 8 ns MAC clock when operating @ 1 Gbps.
    // • 80 ns MAC clock when operating @ 100 Mbps.
    // • 800 ns MAC clock when operating @ 10 Mbps
    union TransmitInterPacketGap {
        struct {
            u16 ipg_transmit_time : 10;
            u16 ipg_receive_time_1 : 10;
            u16 ipg_receive_time_2 : 10;
            u8 : 2;
        } PACKED;

        u32 value;

        TransmitInterPacketGap() : value(0) {}
    };

    struct RxDescriptor {
        u64 address;
        u16 length;
        u16 checksum;
        u8 status;
        u8 errors;
        u16 special;
    } PACKED;
    
    struct TxDescriptor {
        u64 address;
        u16 length;
        u8 cso;
        u8 cmd;
        u8 status;
        u8 css;
        u16 special;
    } PACKED;

    static constexpr u16 VENDOR_ID = 0x8086;
    static constexpr u16 DEVICE_ID = 0x100E;

    static constexpr size_t BUFFER_SIZE = 8192;

    static constexpr size_t NUM_RX_DESCRIPTORS = 32;
    static constexpr size_t NUM_TX_DESCRIPTORS = 8;

    static RefPtr<NetworkAdapter> create(pci::Device);

    Type type() const override { return Type::Ethernet; }

    void detect_eeprom();
    u32 read_eeprom(u8 address);

    void read_mac_address();

    void rx_init();
    void tx_init();

    void setup_link();
    void enable_interrupts();

    void send(u8 const* data, size_t size) override;

private:
    E1000NetworkAdapter(pci::Address address);

    void write(u16 address, u32 value);
    u32 read(u16 address);

    void handle_irq() override;

    void receive();

    pci::Address m_address;

    u32 m_io_port;
    u8* m_memory = nullptr;

    bool m_has_eeprom = false;

    u8* m_rx_buffer;
    u8* m_tx_buffer;

    RxDescriptor* m_rx_descriptors;
    TxDescriptor* m_tx_descriptors;
};

}