#include <kernel/acpi/smbios.h>
#include <kernel/memory/manager.h>

#include <std/cstring.h>

namespace kernel::smbios {

size_t get_table_size(TableHeader* table) {
    const char* strtab = reinterpret_cast<const char*>(table) + table->length;
    size_t size = 1;

    while (strtab[size - 1] != '\0' || strtab[size] != '\0') {
        size++;
    }

    return table->length + size + 1;
}

static DMIParser s_instance;

void DMIParser::init() {
    if (m_initialized) {
        return;
    }

    this->find_entry_points();
    if (m_64_bit_entry_point) {
        m_table_address = m_64_bit_entry_point->table_address;
        m_table_length = m_64_bit_entry_point->table_maximum_size;
        m_table_count = m_64_bit_entry_point->table_maximum_size;
    } else if (m_32_bit_entry_point) {
        m_table_address = m_32_bit_entry_point->table_address;
        m_table_length = m_32_bit_entry_point->table_length;
        m_table_count = m_32_bit_entry_point->tables_count;
    }

    m_table_headers.reserve(m_table_count);
    this->read_table_headers();

    m_initialized = true;
}

void DMIParser::find_entry_points() {
    void* ptr = reinterpret_cast<void*>(BASE_ADDRESS);
    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(ptr, MAX_SIZE));
    if (!region) {
        return;
    }

    for (u32 i = 0; i < MAX_SIZE; i += 16) {
        if (!memcmp(region + i, "_SM3_", 5)) {
            m_64_bit_entry_point = reinterpret_cast<EntryPoint64Bit*>(region + i);
            return;
        } else if (!memcmp(region + i, "_SM_", 4)) {
            m_32_bit_entry_point = reinterpret_cast<EntryPoint32Bit*>(region + i);
            return;
        }
    }
}

void DMIParser::read_table_headers() {
    u8* region = reinterpret_cast<u8*>(MM->map_physical_region(reinterpret_cast<void*>(m_table_address), m_table_length));
    if (!region) {
        return;
    }

    u8* end = region + m_table_length;
    while (region < end) {
        TableHeader* header = reinterpret_cast<TableHeader*>(region);
        if (header->type == 127) {
            break;
        }

        m_table_headers.append(header);

        size_t size = get_table_size(header);
        region += size;
    }
}

}