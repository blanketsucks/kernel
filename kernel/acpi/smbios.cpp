#include <kernel/acpi/smbios.h>
#include <kernel/memory/mm.h>

#include <std/cstring.h>

namespace kernel::smbios {

size_t get_table_size(TableHeader* table) {
    const char* strtab = reinterpret_cast<const char*>(table) + table->length;
    size_t size = 0;

    while (strtab[size] != '\0' || strtab[size + 1] != '\0') {
        size++;
    }

    return size + 2;
}

static DMIParser s_instance;

void DMIParser::init() {
    if (m_initialized) {
        return;
    }

    m_entry_point = this->find_entry_point();
    if (!m_entry_point) {
        return;
    }

    m_table_headers.resize(m_entry_point->tables_count);

    m_initialized = true;
}

EntryPoint* DMIParser::find_entry_point() {
    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(BASE_ADDRESS, MAX_SIZE).value());
    if (!region) {
        return nullptr;
    }

    for (u32 i = 0; i < MAX_SIZE; i += 16) {
        if (std::memcmp(region + i, "_SM_", 4) != 0) {
            continue;
        }

        u8 checksum = 0;
        for (u32 j = 0; j < 16; j++) {
            checksum += region[i + j];
        }

        if (checksum == 0) {
            return reinterpret_cast<EntryPoint*>(region + i);
        }
    }

    return nullptr;
}



}