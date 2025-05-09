#pragma once

#include <kernel/common.h>

namespace kernel::virtio {

#define VIRTIO_F_VERSION_1 ((u64)1 << 32)

enum DeviceType : u8 {
    Reserved = 0,
    NetworkCard = 1,
    BlockDevice = 2,
    Console = 3,
    EntropySource = 4,
    MemoryBallooning = 5,
    IOMemory = 6,
    RPMSG = 7,
    SCSIHost = 8,
    P9Transport = 9, 
    MAC80211WLAN = 10,
    RPROCSerial = 11,
    CAIF = 12,
    MemoryBallon = 13,
    GPUDevice = 16,
    TimerDevice = 17,
    InputDevice = 18,
    SocketDevice = 19,
    CryptoDevice = 20,
    SignalDistributionModule = 21,
    PStoreDevice = 22,
    IOMMUDevice = 23,
    MemoryDevice = 24
};

enum PCICapabilities {
    Type = 0x03,
    Bar = 0x04,
    Offset = 0x08,
    Length = 0x0C
};

constexpr u16 pci_device_type(DeviceType type) {
    return type + 0x1040;
}

constexpr DeviceType to_device_type(u16 device_id) {
    return static_cast<DeviceType>(device_id - 0x1040);
}

struct QueueDescriptor {
    enum {
        Next = 0x1,
        Write = 0x2,
        Indirect = 0x4,
    };

    u64 address;
    u32 length;
    u16 flags;
    u16 next;
} PACKED;

struct QueueDriver {
    u16 flags;
    u16 index;
    u16 ring[];
} PACKED;

struct QueueDeviceElement {
    u32 id;
    u32 len;
} PACKED;

struct QueueDevice {
    u16 flags;
    u16 index;
    QueueDeviceElement ring[];
} PACKED;

enum CommonConfig {
    DeviceFeatureSelect = 0x00,
    DeviceFeature = 0x04,
    DriverFeatureSelect = 0x08,
    DriverFeature = 0x0C,
    MSIXConfig = 0x10,
    NumQueues = 0x12,
    DeviceStatus = 0x14,
    ConfigGeneration = 0x15,
    QueueSelect = 0x16,
    QueueSize = 0x18,
    QueueMSIXVector = 0x1A,
    QueueEnable = 0x1C,
    QueueNotifyOffset = 0x1E,
    QueueDescriptorAddress = 0x20,
    QueueDriverAddress = 0x28,
    QueueDeviceAddress = 0x30
};

enum DeviceStatus {
    Acknowledge = 1,
    Driver = 2,
    Failed = 128,
    FeaturesOK = 8,
    DriverOK = 4,
    DriverNeedsReset = 64
};

enum ISRStatus : u8 {
    UsedBufferNotification = 0x1,
    ConfigurationChange = 0x2,
};

}