#pragma once

#include <kernel/common.h>
#include <std/string_view.h>

namespace kernel::pci {

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

}