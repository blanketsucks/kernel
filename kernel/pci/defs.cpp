#include <kernel/pci/defs.h>

namespace kernel::pci {

StringView get_class_name(DeviceClass device_class) {
    switch (device_class) {
        case DeviceClass::Unclassified: return "Unclassified";
        case DeviceClass::MassStorageController: return "Mass Storage Controller";
        case DeviceClass::NetworkController: return "Network Controller";
        case DeviceClass::DisplayController: return "Display Controller";
        case DeviceClass::MultimediaController: return "Multimedia Controller";
        case DeviceClass::MemoryController: return "Memory Controller";
        case DeviceClass::Bridge: return "Bridge";
        case DeviceClass::SimpleCommunicationController: return "Simple Communication Controller";
        case DeviceClass::BaseSystemPeripheral: return "Base System Peripheral";
        case DeviceClass::InputDeviceController: return "Input Device Controller";
        case DeviceClass::DockingStation: return "Docking Station";
        case DeviceClass::Processor: return "Processor";
        case DeviceClass::SerialBusController: return "Serial Bus Controller";
        case DeviceClass::WirelessController: return "Wireless Controller";
        case DeviceClass::IntelligentController: return "Intelligent Controller";
        case DeviceClass::SatelliteCommunicationController: return "Satellite Communication Controller";
        case DeviceClass::EncryptionController: return "Encryption Controller";
        case DeviceClass::SignalProcessingController: return "Signal Processing Controller";
        case DeviceClass::ProcessingAccelerator: return "Processing Accelerator";
        case DeviceClass::NonEssentialInstrumentation: return "Non-Essential Instrumentation";
        case DeviceClass::Coprocessor: return "Coprocessor";
    }

    return {};
}

StringView get_subclass_name(DeviceClass device_class, DeviceSubclass device_subclass) {
    switch (device_class) {
        case DeviceClass::Unclassified: {
            switch (device_subclass) {
                case DeviceSubclass::NonVgaCompatible: return "Non-VGA-Compatible";
                case DeviceSubclass::VgaCompatible: return "VGA-Compatible";
                default: return "Other";
            }
        }
        case DeviceClass::MassStorageController: {
            switch (device_subclass) {
                case DeviceSubclass::SCSIBusController: return "SCSI Bus Controller";
                case DeviceSubclass::IDEController: return "IDE Controller";
                case DeviceSubclass::FloppyDiskController: return "Floppy Disk Controller";
                case DeviceSubclass::IPIBusController: return "IPI Bus Controller";
                case DeviceSubclass::RAIDController: return "RAID Controller";
                case DeviceSubclass::ATAController: return "ATA Controller";
                case DeviceSubclass::SATAController: return "Serial ATA Controller";
                case DeviceSubclass::SASController: return "Serial Attached SCSI Controller";
                case DeviceSubclass::NVMController: return "Non-Volatile Memory Controller";
                default: return "Other";
            }
        }
        case DeviceClass::NetworkController: {
            switch (device_subclass) {
                case DeviceSubclass::EthernetController: return "Ethernet Controller";
                case DeviceSubclass::TokenRingController: return "Token Ring Controller";
                case DeviceSubclass::FDDIController: return "FDDI Controller";
                case DeviceSubclass::ATMController: return "ATM Controller";
                case DeviceSubclass::ISDNController: return "ISDN Controller";
                case DeviceSubclass::WorldFipController: return "WorldFip Controller";
                case DeviceSubclass::PICMGController: return "PICMG Controller";
                case DeviceSubclass::InfinibandController: return "Infiniband Controller";
                case DeviceSubclass::FabricController: return "Fabric Controller";
                default: return "Other";
            }
        }
        case DeviceClass::DisplayController: {
            switch (device_subclass) {
                case DeviceSubclass::VGACompatibleController: return "VGA-Compatible Controller";
                case DeviceSubclass::XGAController: return "XGA-Compatible Controller";
                case DeviceSubclass::ThreeDController: return "3D Controller";
                default: return "Other";
            }
        }
        case DeviceClass::MultimediaController: {
            switch (device_subclass) {
                case DeviceSubclass::MultimediaVideoController: return "Video Device";
                case DeviceSubclass::MultimediaAudioController: return "Audio Device";
                case DeviceSubclass::ComputerTelephonyDevice: return "Computer Telephony Device";
                case DeviceSubclass::AudioDevice: return "Audio Device";
                default: return "Other";
            }
        }
        case DeviceClass::MemoryController: {
            switch (device_subclass) {
                case DeviceSubclass::RAMController: return "RAM Controller";
                case DeviceSubclass::FlashController: return "Flash Controller";
                default: return "Other";
            }
        }
        case DeviceClass::Bridge: {
            switch (device_subclass) {
                case DeviceSubclass::HostBridge: return "Host Bridge";
                case DeviceSubclass::ISABridge: return "ISA Bridge";
                case DeviceSubclass::EISABridge: return "EISA Bridge";
                case DeviceSubclass::MCABridge: return "MCA Bridge";
                case DeviceSubclass::PCItoPCIBridge: return "PCI-to-PCI Bridge";
                case DeviceSubclass::PCMCIABridge: return "PCMCIA-Compatible Bridge";
                case DeviceSubclass::NuBusBridge: return "NuBus Bridge";
                case DeviceSubclass::CardBusBridge: return "CardBus Bridge";
                case DeviceSubclass::RACEwayBridge: return "RACEway Bridge";
                case DeviceSubclass::PCItoPCI: return "PCI-to-PCI Bridge";
                default: return "Other";
            }
        }
        case DeviceClass::SimpleCommunicationController: {
            switch (device_subclass) {
                case DeviceSubclass::SerialController: return "Serial Controller";
                case DeviceSubclass::ParallelController: return "Parallel Controller";
                case DeviceSubclass::MultiportSerialController: return "Multiport Serial Controller";
                case DeviceSubclass::Modem: return "Modem";
                case DeviceSubclass::GPIBController: return "GPIB Controller";
                case DeviceSubclass::SmartCardController: return "Smart Card Controller";
                default: return "Other";
            }
        }
        case DeviceClass::BaseSystemPeripheral: {
            switch (device_subclass) {
                case DeviceSubclass::PIC: return "PIC";
                case DeviceSubclass::DMAController: return "DMA Controller";
                case DeviceSubclass::Timer: return "Timer";
                case DeviceSubclass::RTCController: return "RTC Controller";
                case DeviceSubclass::PCIHotPlugController: return "PCI Hot-Plug Controller";
                case DeviceSubclass::SDHostController: return "SD Host Controller";
                case DeviceSubclass::IOMMU: return "IOMMU";
                default: return "Other";
            }
        }
        case DeviceClass::InputDeviceController: {
            switch (device_subclass) {
                case DeviceSubclass::KeyboardController: return "Keyboard Controller";
                case DeviceSubclass::DigitizerPen: return "Digitizer Pen";
                case DeviceSubclass::MouseController: return "Mouse Controller";
                case DeviceSubclass::ScannerController: return "Scanner Controller";
                case DeviceSubclass::GameportController: return "Gameport Controller";
                default: return "Other";
            }
        }
        case DeviceClass::DockingStation: {
            switch (device_subclass) {
                case DeviceSubclass::GenericDockingStation: return "Generic Docking Station";
                default: return "Other";
            }
        }
        case DeviceClass::Processor: {
            switch (device_subclass) {
                case DeviceSubclass::Intel386: return "i386";
                case DeviceSubclass::Intel486: return "i486";
                case DeviceSubclass::Pentium: return "Pentium";
                case DeviceSubclass::Alpha: return "Alpha";
                case DeviceSubclass::PowerPC: return "PowerPC";
                case DeviceSubclass::MIPS: return "MIPS";
                case DeviceSubclass::CoProcessor: return "Co-Processor";
                default: return "Other";
            }
        }
        case DeviceClass::SerialBusController: {
            switch (device_subclass) {
                case DeviceSubclass::FirewireController: return "Firewire Controller";
                case DeviceSubclass::ACCESSBusController: return "ACCESS Bus Controller";
                case DeviceSubclass::SSAController: return "SSA Controller";
                case DeviceSubclass::USBController: return "USB Controller";
                case DeviceSubclass::FibreChannelController: return "Fibre Channel Controller";
                case DeviceSubclass::SMBusController: return "SMBus Controller";
                case DeviceSubclass::InfiniBandController: return "InfiniBand Controller";
                case DeviceSubclass::IPMIController: return "IPMI Controller";
                case DeviceSubclass::SERCOSController: return "SERCOS Controller";
                case DeviceSubclass::CANBusController: return "CAN Bus Controller";
                default: return "Other";
            }
        }
        case DeviceClass::WirelessController: {
            switch (device_subclass) {
                case DeviceSubclass::iRDACompatibleController: return "iRDA Compatible Controller";
                case DeviceSubclass::ConsumerIRController: return "Consumer IR Controller";
                case DeviceSubclass::RFController: return "RF Controller";
                case DeviceSubclass::BluetoothController: return "Bluetooth Controller";
                case DeviceSubclass::BroadbandController: return "Broadband Controller";
                case DeviceSubclass::EthernetController802_1a: return "Ethernet Controller (802.1a)";
                case DeviceSubclass::EthernetController802_1b: return "Ethernet Controller (802.1b)";
                default: return "Other";
            }
        }
        case DeviceClass::IntelligentController: {
            switch (device_subclass) {
                case DeviceSubclass::I20: return "I2O";
                default: return "Other";
            }
        }
        case DeviceClass::SatelliteCommunicationController: {
            switch (device_subclass) {
                case DeviceSubclass::SatelliteTVController: return "Satellite TV Controller";
                case DeviceSubclass::SatelliteAudioController: return "Satellite Audio Controller";
                case DeviceSubclass::SatelliteVoiceController: return "Satellite Voice Controller";
                case DeviceSubclass::SatelliteDataController: return "Satellite Data Controller";
                default: return "Other";
            }
        }
        case DeviceClass::EncryptionController: {
            switch (device_subclass) {
                case DeviceSubclass::NetworkEncryptionController: return "Network and Computing Encryption Device";
                case DeviceSubclass::EntertainmentEncryptionController: return "Entertainment Encryption Device";
                default: return "Other";
            }
        }
        case DeviceClass::SignalProcessingController: {
            switch (device_subclass) {
                case DeviceSubclass::DPIOModules: return "DPIO Modules";
                case DeviceSubclass::PerformanceCounters: return "Performance Counters";
                case DeviceSubclass::CommunicationSynchronizer: return "Communication Synchronizer";
                case DeviceSubclass::SignalProcessingManagement: return "Signal Processing Management";
                default: return "Other";
            }
        }
        default: return "Other";
    }
}


}