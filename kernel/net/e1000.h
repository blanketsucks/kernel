#pragma once

#include <kernel/common.h>
#include <kernel/arch/pic.h>
#include <kernel/pci.h>

#include <kernel/net/mac.h>
#include <kernel/net/network_adapter.h>

#include <kernel/process/blocker.h>

#include <std/memory.h>

#define REG_CTRL       0x0000
#define REG_STATUS     0x0008
#define REG_EEPROM     0x0014
#define REG_CTRL_EXT   0x0018
#define REG_ICR        0x00C0
#define REG_ITR        0x00C4
#define REG_IMASK      0x00D0
#define REG_RCTRL      0x0100
#define REG_RXDESCLO   0x2800
#define REG_RXDESCHI   0x2804
#define REG_RXDESCLEN  0x2808
#define REG_RXDESCHEAD 0x2810
#define REG_RXDESCTAIL 0x2818

#define REG_TCTRL      0x0400
#define REG_TXDESCLO   0x3800
#define REG_TXDESCHI   0x3804
#define REG_TXDESCLEN  0x3808
#define REG_TXDESCHEAD 0x3810
#define REG_TXDESCTAIL 0x3818

#define REG_RDTR   0x2820 // RX Delay Timer Register
#define REG_RXDCTL 0x2828 // RX Descriptor Control
#define REG_RADV   0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD  0x2C00 // RX Small Packet Detect Interrupt

#define REG_TIPG  0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU 0x40        // Set link up

#define RCTL_EN            (1 << 1)  // Receiver Enable
#define RCTL_SBP           (1 << 2)  // Store Bad Packets
#define RCTL_UPE           (1 << 3)  // Unicast Promiscuous Enabled
#define RCTL_MPE           (1 << 4)  // Multicast Promiscuous Enabled
#define RCTL_LPE           (1 << 5)  // Long Packet Reception Enable
#define RCTL_LBM_NONE      (0 << 6)  // No Loopback
#define RCTL_LBM_PHY       (3 << 6)  // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF    (0 << 8)  // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER (1 << 8)  // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH  (2 << 8)  // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36         (0 << 12) // Multicast Offset - bits 47:36
#define RCTL_MO_35         (1 << 12) // Multicast Offset - bits 46:35
#define RCTL_MO_34         (2 << 12) // Multicast Offset - bits 45:34
#define RCTL_MO_32         (3 << 12) // Multicast Offset - bits 43:32
#define RCTL_BAM           (1 << 15) // Broadcast Accept Mode
#define RCTL_VFE           (1 << 18) // VLAN Filter Enable
#define RCTL_CFIEN         (1 << 19) // Canonical Form Indicator Enable
#define RCTL_CFI           (1 << 20) // Canonical Form Indicator Bit Value
#define RCTL_DPF           (1 << 22) // Discard Pause Frames
#define RCTL_PMCF          (1 << 23) // Pass MAC Control Frames
#define RCTL_SECRC         (1 << 26) // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256   (3 << 16)
#define RCTL_BSIZE_512   (2 << 16)
#define RCTL_BSIZE_1024  (1 << 16)
#define RCTL_BSIZE_2048  (0 << 16)
#define RCTL_BSIZE_4096  ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192  ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384 ((1 << 16) | (1 << 25))

// Transmit Command
#define CMD_EOP  (1 << 0)    // End of Packet
#define CMD_IFCS (1 << 1)    // Insert FCS
#define CMD_IC   (1 << 2)    // Insert Checksum
#define CMD_RS   (1 << 3)    // Report Status
#define CMD_RPS  (1 << 4)    // Report Packet Sent
#define CMD_VLE  (1 << 6)    // VLAN Packet Enable
#define CMD_IDE  (1 << 7)    // Interrupt Delay Enable

// TCTL Register
#define TCTL_EN         (1 << 1)  // Transmit Enable
#define TCTL_PSP        (1 << 3)  // Pad Short Packets
#define TCTL_CT_SHIFT   4         // Collision Threshold
#define TCTL_COLD_SHIFT 12        // Collision Distance
#define TCTL_SWXOFF     (1 << 22) // Software XOFF Transmission
#define TCTL_RTLC       (1 << 24) // Re-transmit on Late Collision

// Status Register
#define STATUS_FD (1 << 0) // Full Duplex
#define STATUS_LU (1 << 1) // Link Up

#define TSTA_DD (1 << 0) // Descriptor Done
#define TSTA_EC (1 << 1) // Excess Collisions
#define TSTA_LC (1 << 2) // Late Collision
#define LSTA_TU (1 << 3) // Transmit Underrun

#define IMS_LSC   (1 << 2) // Link Status Change
#define IMS_RX0   (1 << 6) // Receiver FIFO Overrun
#define IMS_RXT0  (1 << 7) // RX Timer Interrupt

namespace kernel::net {

struct E1000RxDescriptor {
    u64 address;
    u16 length;
    u16 checksum;
    u8 status;
    u8 errors;
    u16 special;
} PACKED;

struct E1000TxDescriptor {
    u64 address;
    u16 length;
    u8 cso;
    u8 cmd;
    u8 status;
    u8 css;
    u16 special;
} PACKED;

class E1000NetworkAdapter : public NetworkAdapter, public IRQHandler {
public:
    static constexpr u16 VENDOR_ID = 0x8086;
    static constexpr u16 DEVICE_ID = 0x100E;

    static constexpr size_t BUFFER_SIZE = 8192;
    static constexpr size_t PAGE_BUFFER_SIZE = BUFFER_SIZE / PAGE_SIZE;

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

    void handle_interrupt(arch::InterruptRegisters*) override;

    void receive();

    pci::Address m_address;

    u32 m_io_port;
    u8* m_memory = nullptr;

    bool m_has_eeprom = false;

    u8* m_rx_buffer;
    u8* m_tx_buffer;

    E1000RxDescriptor* m_rx_descriptors;
    E1000TxDescriptor* m_tx_descriptors;
};

}