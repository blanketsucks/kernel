#include <kernel/net/e1000.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip/arp.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>

#include <std/format.h>

namespace kernel::net {

RefPtr<NetworkAdapter> E1000NetworkAdapter::create(pci::Device device) {
    if (device.vendor_id != VENDOR_ID || device.id != DEVICE_ID) {
        return nullptr;
    }

    return RefPtr<NetworkAdapter>(new E1000NetworkAdapter(device.address));
}

E1000NetworkAdapter::E1000NetworkAdapter(pci::Address address) : IRQHandler(address.interrupt_line()), m_address(address) {
    this->disable_irq_handler();

    address.set_bus_master(true);
    switch (address.bar_type(0)) {
        case pci::BARType::IO:
            m_io_port = address.bar0() & ~1;
            break;
        default: {
            PhysicalAddress bar0 = address.bar0() & ~3;
            size_t size = address.bar_size(0);

            m_memory = reinterpret_cast<u8*>(MM->map_physical_region(reinterpret_cast<void*>(bar0), size));
            break;
        }
    }

    this->detect_eeprom();
    this->read_mac_address();

    this->rx_init();
    this->tx_init();
    
    this->setup_link();
    this->enable_interrupts();

    dbgln("E1000NetworkAdapter ({}:{}:{}):", address.bus, address.device, address.function);
    dbgln(" - Has EEPROM: {}", m_has_eeprom);
    dbgln(" - MAC Address: {}", mac_address());
    dbgln();
}

void E1000NetworkAdapter::write(u16 address, u32 value) {
    if (!m_memory) {
        io::write<u32>(m_io_port + address, value);
    } else {
        *reinterpret_cast<u32*>(m_memory + address) = value;
    }
}

u32 E1000NetworkAdapter::read(u16 address) {
    if (!m_memory) {
        return io::read<u32>(m_io_port + address);
    } else {
        return *reinterpret_cast<u32*>(m_memory + address);
    }
}

void E1000NetworkAdapter::setup_link() {
    write(REG_CTRL, read(REG_CTRL) | ECTRL_SLU);
}

void E1000NetworkAdapter::enable_interrupts() {
    this->enable_irq_handler();

    write(REG_ITR, 5580);
    write(REG_IMASK, IMS_LSC | IMS_RX0 | IMS_RXT0);
    
    read(REG_ICR);
    m_address.set_interrupt_line(true);   
}

void E1000NetworkAdapter::handle_interrupt(arch::InterruptRegisters*) {
    u32 status = read(REG_ICR);
    if (!status) {
        return;
    }

    if (status & IMS_LSC) {
        dbgln("E1000: Link Status Change");
        this->setup_link();
    }

    if (status & IMS_RX0) {
        dbgln("E1000: Receiver FIFO Overrun");
    }

    if (status & IMS_RXT0) {
        this->receive();
    }
}

void E1000NetworkAdapter::detect_eeprom() {
    static constexpr int TRIES = 1000;

    write(REG_EEPROM, 0x1);
    for (int i = 0; i < TRIES; i++) {
        if (read(REG_EEPROM) & 0x10) {
            m_has_eeprom = true;
            return;
        }
    }
}

u32 E1000NetworkAdapter::read_eeprom(u8 address) {
    u32 tmp = 0;
    if (m_has_eeprom) {
        write(REG_EEPROM, 1 | (static_cast<u32>(address) << 8));
        while (!((tmp = read(REG_EEPROM)) & (1 << 4))) {
            asm volatile("pause");
        }
    } else {
        write(REG_EEPROM, 1 | (static_cast<u32>(address) << 2));
        while (!((tmp = read(REG_EEPROM)) & (1 << 1))) {
            asm volatile("pause");
        }
    }

    return (tmp >> 16) & 0xFFFF;
}

void E1000NetworkAdapter::read_mac_address() {
    if (m_has_eeprom) {
        MACAddress mac;

        u32 tmp = read_eeprom(0);
        mac[0] = tmp & 0xFF;
        mac[1] = tmp >> 8;

        tmp = read_eeprom(1);
        mac[2] = tmp & 0xFF;
        mac[3] = tmp >> 8;

        tmp = read_eeprom(2);
        mac[4] = tmp & 0xFF;
        mac[5] = tmp >> 8;

        this->set_mac_address(mac);
        return;
    }
}

void E1000NetworkAdapter::rx_init() {
    m_rx_buffer = reinterpret_cast<u8*>(MM->allocate_dma_region(NUM_RX_DESCRIPTORS * BUFFER_SIZE));
    m_rx_descriptors = reinterpret_cast<E1000RxDescriptor*>(MM->allocate_dma_region(NUM_RX_DESCRIPTORS * sizeof(E1000RxDescriptor)));

    for (size_t i = 0; i < NUM_RX_DESCRIPTORS; i++) {
        auto& descriptor = m_rx_descriptors[i];
        u8* buffer = m_rx_buffer + i * BUFFER_SIZE;

        descriptor.address = MM->get_physical_address(buffer);
        descriptor.status = 0;
    }

    PhysicalAddress address = MM->get_physical_address(m_rx_descriptors);

    write(REG_RXDESCLO, static_cast<u64>(address) & 0xFFFFFFFF);
    write(REG_RXDESCHI, static_cast<u64>(address) >> 32);
    write(REG_RXDESCLEN, NUM_RX_DESCRIPTORS * sizeof(E1000RxDescriptor));
    write(REG_RXDESCHEAD, 0);
    write(REG_RXDESCTAIL, NUM_RX_DESCRIPTORS - 1);

    write(REG_RCTRL, RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_8192);
}

void E1000NetworkAdapter::tx_init() {
    m_tx_buffer = reinterpret_cast<u8*>(MM->allocate_dma_region(NUM_TX_DESCRIPTORS * BUFFER_SIZE));
    m_tx_descriptors = reinterpret_cast<E1000TxDescriptor*>(MM->allocate_dma_region(NUM_TX_DESCRIPTORS * sizeof(E1000TxDescriptor)));

    for (size_t i = 0; i < NUM_TX_DESCRIPTORS; i++) {
        auto& descriptor = m_tx_descriptors[i];
        u8* buffer = m_tx_buffer + i * BUFFER_SIZE;

        descriptor.address = MM->get_physical_address(buffer);
        descriptor.cmd = 0;
        descriptor.status = TSTA_DD;
    }

    PhysicalAddress address = MM->get_physical_address(m_tx_descriptors);

    write(REG_TXDESCLO, static_cast<u64>(address) & 0xFFFFFFFF);
    write(REG_TXDESCHI, static_cast<u64>(address) >> 32);
    write(REG_TXDESCLEN, NUM_TX_DESCRIPTORS * sizeof(E1000TxDescriptor));
    write(REG_TXDESCHEAD, 0);
    write(REG_TXDESCTAIL, 0);

    write(REG_TCTRL, read(REG_TCTRL) | TCTL_EN | TCTL_PSP);
    write(REG_TIPG, 0x0060200A);
}

void E1000NetworkAdapter::send(u8 const* data, size_t size) {
    this->disable_irq_handler();

    size_t current = read(REG_TXDESCTAIL);
    auto& descriptor = m_tx_descriptors[current];
    
    memcpy(m_tx_buffer + current * BUFFER_SIZE, data, size);

    descriptor.length = size;
    descriptor.cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    descriptor.status = 0;

    this->enable_irq_handler();
    write(REG_TXDESCTAIL, (current + 1) % NUM_TX_DESCRIPTORS);

    while (true) {
        if (descriptor.status) {
            break;
        }
    }
}

void E1000NetworkAdapter::receive() {
    while (true) {
        u32 current = (read(REG_RXDESCTAIL) + 1) % NUM_RX_DESCRIPTORS;
        auto& descriptor = m_rx_descriptors[current];
        if (!(descriptor.status & 0x1)) {
            break;
        }

        u8* buffer = m_rx_buffer + current * BUFFER_SIZE;
        size_t length = descriptor.length;

        on_packet_receive(buffer, length);

        descriptor.status = 0;
        write(REG_RXDESCTAIL, current);
    }    
}

}