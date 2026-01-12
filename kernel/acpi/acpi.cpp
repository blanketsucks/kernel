#include <kernel/acpi/acpi.h>
#include <kernel/memory/manager.h>
#include <kernel/serial.h>

#include <std/utility.h>
#include <std/format.h>
#include <std/cstring.h>

namespace kernel {

using namespace acpi;

static ACPIParser s_instance;
static bool s_initialized = false;

ACPIParser* ACPIParser::instance() {
    return &s_instance;
}

void ACPIParser::init() {
    if (s_initialized) {
        return;
    }

    s_instance.initialize();
    s_initialized = true;
}

void ACPIParser::initialize() {
    this->find_root_table();
    this->parse_acpi_tables();
}

SDTHeader* ACPIParser::map_acpi_table(PhysicalAddress addr) {
    PhysicalAddress base = page_base_of(addr);
    size_t offset = offset_in_page(addr);

    void* ptr = reinterpret_cast<void*>(base);

    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(ptr, PAGE_SIZE));
    auto* header = reinterpret_cast<SDTHeader*>(region + offset);

    // Ensure the table is at least a page in size
    size_t length = std::align_up(header->length, PAGE_SIZE);
    if (length == PAGE_SIZE) {
        return header; // No need to remap if the size has not changed
    }

    MM->unmap_kernel_region(region);
    region = reinterpret_cast<u8*>(MM->map_physical_region(ptr, length));

    return reinterpret_cast<SDTHeader*>(region + offset);
}

bool ACPIParser::find_root_table() {
    if (g_boot_info->rsdp) {
        m_rsdp = reinterpret_cast<RSDP*>(MM->map_physical_region(g_boot_info->rsdp, PAGE_SIZE));

        if (m_rsdp->revision == 2) {
            m_xsdp = reinterpret_cast<XSDP*>(m_rsdp);
            m_xsdt = reinterpret_cast<XSDT*>(this->map_acpi_table(m_xsdp->xsdt_address));

            return true;
        }

        m_rsdt = reinterpret_cast<RSDT*>(this->map_acpi_table(m_rsdp->rsdt_address));
        return true;
    }

    void* start = reinterpret_cast<void*>(RSDP_START);
    size_t size = RSDP_END - RSDP_START;

    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(start, size));
    if (!region) {
        return false;
    }

    for (u32 i = 0; i < size; i += 16) {
        if (memcmp(region + i, "RSD PTR ", 8) == 0) {
            m_rsdp = reinterpret_cast<RSDP*>(region + i);
            break;
        }
    }

    if (!m_rsdp) {
        return false;
    }

    m_rsdt = reinterpret_cast<RSDT*>(this->map_acpi_table(m_rsdp->rsdt_address));
    return true;
}

void ACPIParser::parse_acpi_tables() {
    size_t entries = 0;
    if (m_xsdt) {
        entries = (m_xsdt->header.length - sizeof(SDTHeader)) / 8;
    } else if (m_rsdt) {
        entries = (m_rsdt->header.length - sizeof(SDTHeader)) / 4;
    } else {
        return;
    }

    m_tables.reserve(entries);
    dbgln("ACPI Tables ({} entries):", entries);

    for (u32 i = 0; i < entries; i++) {
        PhysicalAddress table = 0;
        if (m_xsdt) {
            table = m_xsdt->tables[i];
        } else {
            table = m_rsdt->tables[i];
        }

        SDTHeader* header = this->map_acpi_table(table);
        dbgln(" - {}{}{}{}:", header->signature[0], header->signature[1], header->signature[2], header->signature[3]);

        dbgln("  - Length: {}", header->length);
        dbgln("  - Revision: {}", header->revision);
        dbgln("  - Checksum: {:#x}", header->checksum);
        dbgln("  - OEM ID: {}{}{}{}{}{}", header->oem_id[0], header->oem_id[1], header->oem_id[2], header->oem_id[3], header->oem_id[4], header->oem_id[5]);

        m_tables.append(header);
    }

    dbgln();

    FADT* fadt = this->find<FADT>();
    m_dsdt = this->map_acpi_table(fadt->dsdt);
}

SDTHeader* ACPIParser::find_table(const char* signature) {
    if (memcmp(signature, "DSDT", 4) == 0) {
        return m_dsdt;
    }

    for (auto* table : m_tables) {
        if (memcmp(table->signature, signature, 4) == 0) {
            return table;
        }
    }

    return nullptr;
}

}