#pragma once

#include <kernel/common.h>

#include <std/function.h>
#include <std/string.h>

namespace kernel::pci {

constexpr u16 CONFIG_ADDRESS = 0xCF8;
constexpr u16 CONFIG_DATA = 0xCFC;
constexpr u16 SECONDARY_BUS = 0x19;

constexpr u16 INTERRUPT_LINE = 0x3C;
constexpr u16 INTERRUPT_PIN = 0x3D;

constexpr u16 VENDOR_ID = 0x00;
constexpr u16 DEVICE_ID = 0x02;
constexpr u16 HEADER_TYPE = 0x0E;

constexpr u16 CLASS = 0x0B;
constexpr u16 SUBCLASS = 0x0A;

constexpr u16 MULTIFUNCTION = 0x80;

constexpr u16 UNUSED = 0xFFFF;

constexpr u16 BAR0 = 0x10;
constexpr u16 BAR1 = 0x14;
constexpr u16 BAR2 = 0x18;
constexpr u16 BAR3 = 0x1C;
constexpr u16 BAR4 = 0x20;
constexpr u16 BAR5 = 0x24;

constexpr u16 PROG_IF = 0x09;

constexpr u16 COMMAND = 0x04;

union Command {
    struct {
        u16 io_space : 1;
        u16 memory_space : 1;
        u16 bus_master : 1;
        u16 special_cycles : 1;
        u16 memory_write_and_invalidate : 1;
        u16 vga_palette_snoop : 1;
        u16 parity_error_response : 1;
        u16 reserved : 1;
        u16 serr_enable : 1;
        u16 fast_back_to_back : 1;
        u16 interrupt_disable : 1;
    };

    u16 value;

    Command() = default;
    Command(u16 value) : value(value) {}
};

enum class BARType : u8 {
    Memory16,
    Memory32,
    Memory64,
    IO,
};

union Address {
    struct {
        u32 register_offset : 8;
        u32 function : 3;
        u32 device : 5;
        u32 bus : 8;
        u32 reserved : 7;
        u32 enable : 1;
    };

    u32 value;

    Address() = default;
    Address(u32 value) : value(value) {}
    Address(u8 bus, u8 device, u8 function) : function(function), device(device), bus(bus) {}

    bool is_null() const { return value == 0; }

    u32 bar0() const;
    u32 bar1() const;
    u32 bar2() const;
    u32 bar3() const;
    u32 bar4() const;
    u32 bar5() const;

    u32 bar(u8 index) const;
    BARType bar_type(u8 index) const;
    size_t bar_size(u8 index) const;

    u8 prog_if() const;
    u8 interrupt_line() const;

    void set_interrupt_line(bool value) const;
    void set_bus_master(bool value) const;
    void set_io_space(bool value) const;
};

enum class DeviceClass : u8 {
    Unclassified = 0x00,
    MassStorageController = 0x01,
    NetworkController = 0x02,
    DisplayController = 0x03,
    MultimediaController = 0x04,
    MemoryController = 0x05,
    Bridge = 0x06,
    SimpleCommunicationController = 0x07,
    BaseSystemPeripheral = 0x08,
    InputDeviceController = 0x09,
    DockingStation = 0x0A,
    Processor = 0x0B,
    SerialBusController = 0x0C,
    WirelessController = 0x0D,
    IntelligentController = 0x0E,
    SatelliteCommunicationController = 0x0F,
    EncryptionController = 0x10,
    SignalProcessingController = 0x11,
    ProcessingAccelerator = 0x12,
    NonEssentialInstrumentation = 0x13,
    Coprocessor = 0x40,
};

enum class DeviceSubclass : u8 {
    Other = 0x80,

    // Unclassified
    NonVgaCompatible = 0x00,
    VgaCompatible = 0x01,

    // Mass Storage Controller
    SCSIBusController = 0x00,
    IDEController = 0x01,
    FloppyDiskController = 0x02,
    IPIBusController = 0x03,
    RAIDController = 0x04,
    ATAController = 0x05,
    SATAController = 0x06,
    SASController = 0x07,
    NVMController = 0x08,

    // Network Controller
    EthernetController = 0x00,
    TokenRingController = 0x01,
    FDDIController = 0x02,
    ATMController = 0x03,
    ISDNController = 0x04,
    WorldFipController = 0x05,
    PICMGController = 0x06,
    InfinibandController = 0x07,
    FabricController = 0x08,

    // Display Controller
    VGACompatibleController = 0x00,
    XGAController = 0x01,
    ThreeDController = 0x02,

    // Multimedia Controller
    MultimediaVideoController = 0x00,
    MultimediaAudioController = 0x01,
    ComputerTelephonyDevice = 0x02,
    AudioDevice = 0x03,

    // Memory Controller
    RAMController = 0x00,
    FlashController = 0x01,

    // Bridge
    HostBridge = 0x00,
    ISABridge = 0x01,
    EISABridge = 0x02,
    MCABridge = 0x03,
    PCItoPCIBridge = 0x04,
    PCMCIABridge = 0x05,
    NuBusBridge = 0x06,
    CardBusBridge = 0x07,
    RACEwayBridge = 0x08,
    PCItoPCI = 0x09,
    InfiniBandtoPCI = 0x0A,

    // Simple Communication Controller
    SerialController = 0x00,
    ParallelController = 0x01,
    MultiportSerialController = 0x02,
    Modem = 0x03,
    GPIBController = 0x04,
    SmartCardController = 0x05,

    // Base System Peripheral
    PIC = 0x00,
    DMAController = 0x01,
    Timer = 0x02,
    RTCController = 0x03,
    PCIHotPlugController = 0x04,
    SDHostController = 0x05,
    IOMMU = 0x06,

    // Input Device Controller
    KeyboardController = 0x00,
    DigitizerPen = 0x01,
    MouseController = 0x02,
    ScannerController = 0x03,
    GameportController = 0x04,

    // Docking Station
    GenericDockingStation = 0x00,

    // Processor
    Intel386 = 0x00,
    Intel486 = 0x01,
    Pentium = 0x02,
    Alpha = 0x10,
    PowerPC = 0x20,
    MIPS = 0x30,
    CoProcessor = 0x40,

    // Serial Bus Controller
    FirewireController = 0x00,
    ACCESSBusController = 0x01,
    SSAController = 0x02,
    USBController = 0x03,
    FibreChannelController = 0x04,
    SMBusController = 0x05,
    InfiniBandController = 0x06,
    IPMIController = 0x07,
    SERCOSController = 0x08,
    CANBusController = 0x09,

    // Wireless Controller
    iRDACompatibleController = 0x00,
    ConsumerIRController = 0x01,
    RFController = 0x10,
    BluetoothController = 0x11,
    BroadbandController = 0x12,
    EthernetController802_1a = 0x20,
    EthernetController802_1b = 0x21,

    // Intelligent Controller
    I20 = 0x00,

    // Satellite Communication Controller
    SatelliteTVController = 0x01,
    SatelliteAudioController = 0x02,
    SatelliteVoiceController = 0x03,
    SatelliteDataController = 0x04,

    // Encryption Controller
    NetworkEncryptionController = 0x00,
    EntertainmentEncryptionController = 0x10,

    // Signal Processing Controller
    DPIOModules = 0x00,
    PerformanceCounters = 0x01,
    CommunicationSynchronizer = 0x10,
    SignalProcessingManagement = 0x20,
};

StringView get_class_name(DeviceClass device_class);
StringView get_subclass_name(DeviceClass device_class, DeviceSubclass device_subclass);

struct Device {
    Address address;

    u16 vendor_id;
    u16 id;

    DeviceClass device_class;
    DeviceSubclass subclass;

    bool operator==(const Device& other) const { return vendor_id == other.vendor_id && id == other.id; }

    StringView class_name() const { return get_class_name(device_class); }
    StringView subclass_name() const { return get_subclass_name(device_class, subclass); }

    bool is_ide_controller() const {
        return device_class == DeviceClass::MassStorageController && subclass == DeviceSubclass::IDEController;
    }

    bool is_sata_controller() const {
        return device_class == DeviceClass::MassStorageController && subclass == DeviceSubclass::SATAController;
    }

    bool is_usb_controller() const {
        return device_class == DeviceClass::SerialBusController && subclass == DeviceSubclass::USBController;
    }

    bool is_audio_device() const {
        return device_class == DeviceClass::MultimediaController && subclass == DeviceSubclass::MultimediaAudioController;
    }

    bool is_bochs_vga() const {
        return vendor_id == 0x1234 && id == 0x1111;
    }
};

using EnumerationCallback = Function<void(Device)>;

template<typename T> T read(Address address, u8 offset) = delete;

template<> u8 read<u8>(Address address, u8 offset);
template<> u16 read<u16>(Address address, u8 offset);
template<> u32 read<u32>(Address address, u8 offset);

template<typename T> void write(Address address, u8 offset, T value) = delete;

template<> void write<u8>(Address address, u8 offset, u8 value);
template<> void write<u16>(Address address, u8 offset, u16 value);
template<> void write<u32>(Address address, u8 offset, u32 value);

u16 get_vendor_id(Address address);
u16 get_device_id(Address address);

u8 get_class(Address address);
u8 get_subclass(Address address);

u16 get_type(Address address);

void enumerate(EnumerationCallback callback);

}