#pragma once

#include <kernel/common.h>

#define EFIAPI __attribute__((ms_abi))

#define EFIERR(n) (0x8000000000000000 | n)
#define EFI_ERROR(status) ((int)status < 0)

#define EFI_SUCCESS 0

#define EFI_LOAD_ERROR EFIERR(1)

#define EFI_LOAD_FILE_PROTOCOL_GUID { 0x56EC3091, 0x954C, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID { 0x964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }
#define EFI_LOADED_IMAGE_PROTOCOL_GUID { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_FILE_INFO_ID { 0x9576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define ACPI_TABLE_GUID { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d } }
#define EFI_ACPI_TABLE_GUID { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVED       0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFIAllocateType;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiUnacceptedMemoryType,
    EfiMaxMemoryType
} EFIMemoryType;

using EFIHandle = void*;
using EFIStatus = uintptr_t;

struct EFIGUID {
    u32 a;
    u16 b;
    u16 c;
    u8 d[8];
};

struct EFITime {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 pad1;
    u32 nanosecond;
    i16 timezone;
    u8 daylight;
    u8 pad2;
};

struct EFIFileInfo {
    u64 size;
    u64 file_size;
    u64 physical_size;
    EFITime create_time;
    EFITime last_access_time;
    EFITime modification_time;
    u64 attribute;
    char16_t filename[0];
};

struct EFIConfigurationTable {
    EFIGUID vendor_guid;
    void* vendor_table;
};

struct EFISimpleTextOutputProtocol;
struct EFISimpleFileSystemProtocol;
struct EFIFileProtocol;
struct EFILoadedImageProtocol;

struct EFIMemoryDescriptor {
    u32 type;
    u64 physical_start;
    u64 virtual_start;
    u64 number_of_pages;
    u64 attribute;  
};

typedef EFIStatus (EFIAPI *EFIExit)(EFIHandle image_handle, EFIStatus exit_status, u64 exit_data_size, char16_t* exit_data);

typedef EFIStatus (EFIAPI *EFITextString)(
    EFISimpleTextOutputProtocol* self,
    const char16_t* string
);

typedef EFIStatus (EFIAPI *EFIAllocatePages)(
    EFIAllocateType type,
    EFIMemoryType memory_type,
    size_t pages,
    uintptr_t* memory
);

typedef EFIStatus (EFIAPI *EFIFreePages)(
    uintptr_t memory,
    size_t pages
);

typedef EFIStatus (EFIAPI *EFIGetMemoryMap)(
    u64* memory_map_size,
    EFIMemoryDescriptor* memory_map,
    u64* map_key,
    u64* descriptor_size,
    u32* descriptor_version
);

typedef EFIStatus (EFIAPI *EFIHandleProtocol)(
    EFIHandle handle,
    const EFIGUID* protocol,
    void** interface
);

typedef EFIStatus (EFIAPI *EFIOpenProtocol)(
    EFIHandle handle,
    const EFIGUID* protocol,
    void** interface,
    EFIHandle agent_handle,
    EFIHandle controller_handle,
    u32 attributes
);

typedef EFIStatus (EFIAPI *EFIProtocolsPerHandle)(
    EFIHandle handle,
    EFIGUID*** protocols,
    u64* protocol_count
);

typedef EFIStatus (EFIAPI *EFILocateProtocol) (
    const EFIGUID* protocol,
    void* registration,
    void** interface
);

typedef EFIStatus (EFIAPI *EFISimpleFileSystemProtocolOpenVolume)(
    EFISimpleFileSystemProtocol* self,
    EFIFileProtocol** root
);

typedef EFIStatus (EFIAPI *EFIFileOpen)(
    EFIFileProtocol* self,
    EFIFileProtocol** file,
    const char16_t* filename,
    u64 mode,
    u64 attributes
);

typedef EFIStatus (EFIAPI *EFIFileClose)(
    EFIFileProtocol* self
);

typedef EFIStatus (EFIAPI *EFIFileRead)(
    EFIFileProtocol* self,
    u64* buffer_size,
    void* buffer
);

typedef EFIStatus (EFIAPI *EFIFileGetInfo)(
    EFIFileProtocol* self,
    EFIGUID* information_type,
    u64* buffer_size,
    void* buffer
);

struct EFITableHeader {
    u64 signature;
    u32 revision;
    u32 header_size;
    u32 crc32;
    u32 reserved;
};

struct EFISimpleTextInputProtocol {};

struct EFISimpleTextOutputProtocol {
    void* Reset;
    EFITextString OutputString;
};

struct EFIRuntimeServices {};

struct EFIBootServices {
    EFITableHeader header;

    void* RaiseTPL;
    void* RestoreTPL;

    EFIAllocatePages AllocatePages;
    EFIFreePages FreePages;
    EFIGetMemoryMap GetMemoryMap;
    void* AllocatePool;
    void* FreePool;

    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;

    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    EFIHandleProtocol HandleProtocol;
    void* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;

    void* LoadImage;
    void* StartImage;
    EFIExit Exit;
    void* UnloadImage;
    void* ExitBootServices;

    void* GetNextMonotonicCount;
    void* Stall;
    void* SetWatchdogTimer;

    void* ConnectController;
    void* DisconnectController;

    EFIOpenProtocol OpenProtocol;
    void* CloseProtocol;
    void* OpenProtocolInformation;

    EFIProtocolsPerHandle ProtocolsPerHandle;
    void* LocateHandleBuffer;
    EFILocateProtocol LocateProtocol;
    void* InstallMultipleProtocolInterfaces;
    void* UninstallMultipleProtocolInterfaces;

    void* CalculateCrc32;

    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
};

struct EFISystemTable {
    EFITableHeader header;
    char16_t* firmware_vendor;
    uint32_t firmware_revision;
    EFIHandle stdin_handle;
    EFISimpleTextInputProtocol* stdin;
    EFIHandle stdout_handle;
    EFISimpleTextOutputProtocol* stdout;
    EFIHandle stderr_handle;
    EFISimpleTextOutputProtocol* stderr;
    EFIRuntimeServices* runtime_services;
    EFIBootServices* boot_services;
    uint64_t number_of_table_entries;
    EFIConfigurationTable* configuration_table;
};

struct EFISimpleFileSystemProtocol {
    u64 revision;
    EFISimpleFileSystemProtocolOpenVolume OpenVolume;
};

struct EFIFileProtocol {
    u64 revision;
    EFIFileOpen Open;
    EFIFileClose Close;
    void* Delete;
    EFIFileRead Read;
    void* Write;
    void* GetPosition;
    void* SetPosition;
    EFIFileGetInfo GetInfo;
};

struct EFILoadedImageProtocol {
    u32 revision;
    EFIHandle parent_handle;
    EFISystemTable* system_table;

    EFIHandle device_handle;
    void* filepath;
    void* reserved;

    u64 load_options_size;
    void* load_options;

    void* image_base;
    u64 image_size;
    u32 image_code_type;
    u32 image_data_type;
    void* unload;
};

