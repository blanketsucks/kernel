
#include <kernel/acpi/acpi.h>
#include <kernel/acpi/lai.h>
#include <kernel/memory/manager.h>
#include <kernel/serial.h>

#include <std/utility.h>
#include <std/format.h>
#include <std/cstring.h>

#include <lai/core.h>
#include <lai/helpers/resource.h>

namespace kernel::acpi {

static Parser s_parser;

Parser* Parser::instance() {
    return &s_parser;
}

void Parser::init() {
    this->find_rsdt();
    this->parse_acpi_tables();
}

SDTHeader* Parser::map_acpi_table(u32 addr) {
    u32 base = page_base_of(addr);
    u32 offset = offset_in_page(addr);

    void* ptr = reinterpret_cast<void*>(base);

    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(ptr, PAGE_SIZE));
    auto* header = reinterpret_cast<SDTHeader*>(region + offset);

    // Ensure the table is at least a page in size
    size_t length = std::align_up(header->length, PAGE_SIZE);
    if (length == PAGE_SIZE) {
        return header; // No need to remap if the size has not changed
    }

    MM->unmap_physical_region(region);
    region = reinterpret_cast<u8*>(MM->map_physical_region(ptr, length));

    return reinterpret_cast<SDTHeader*>(region + offset);
}

bool Parser::find_rsdt() {
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

    // TODO: Support for XSDT and make sure to validate the checksum
    m_rsdt = reinterpret_cast<RSDT*>(this->map_acpi_table(m_rsdp->rsdt_address));
    return true;
}

void Parser::parse_acpi_tables() {
    u32 entries = (m_rsdt->header.length - sizeof(SDTHeader)) / 4;

    m_tables.reserve(entries);
    serial::printf("ACPI Tables (%u entries): \n", entries);

    for (u32 i = 0; i < entries; i++) {
        u32 table = m_rsdt->tables[i];

        SDTHeader* header = this->map_acpi_table(table);
        serial::printf("  %.*s:\n", 4, header->signature);

        serial::printf("    Length: %u\n", header->length);
        serial::printf("    Revision: %u\n", header->revision);
        serial::printf("    Checksum: %u\n", header->checksum);
        serial::printf("    OEM ID: %.*s\n", 6, header->oem_id);

        m_tables.append(header);
    }

    FADT* fadt = this->find_table<FADT>("FACP");
    m_dsdt = this->map_acpi_table(fadt->dsdt);

    // lai_set_acpi_revision(m_rsdp->revision);
    // lai_create_namespace();
}

SDTHeader* Parser::find_table(const char* signature) {
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