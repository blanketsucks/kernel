#pragma once

#include <kernel/acpi/tables.h>
#include <std/vector.h>

namespace kernel {

template<typename T>
concept HasACPISignature = requires { T::SIGNATURE; };

class ACPIParser {
public:
    static ACPIParser* instance();
    static void init();

    template<typename T> requires HasACPISignature<T>
    static T* find() {
        auto* parser = instance();
        return reinterpret_cast<T*>(parser->find_table(T::SIGNATURE));
    }

    acpi::SDTHeader* find_table(const char* signature);

private:
    void initialize();

    bool find_root_table();
    void parse_acpi_tables();

    acpi::SDTHeader* map_acpi_table(PhysicalAddress address);

    acpi::RSDP* m_rsdp = nullptr;
    acpi::XSDP* m_xsdp = nullptr;

    acpi::RSDT* m_rsdt = nullptr;
    acpi::XSDT* m_xsdt = nullptr;

    acpi::SDTHeader* m_dsdt = nullptr;

    Vector<acpi::SDTHeader*> m_tables;
};

}