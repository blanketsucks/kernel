#pragma once

#include <kernel/virtio/virtio.h>

namespace kernel::virtio {

#define VIRTIO_GPU_MAX_SCANOUTS 16

#define VIRTIO_GPU_F_VIRGL (1 << 0)
#define VIRTIO_GPU_F_EDID  (1 << 1)

enum GPUControlType : u32 {
    /* 2d commands */
    GetDisplayInfo = 0x0100,
    ResourceCreate2D,
    ResourceUnref,
    SetScanout,
    ResourceFlush,
    TransferToHost2D,
    ResourceAttachBacking,
    ResourceDetachBacking,
    GetCapsetInfo,
    GetCapset,
    GetEDID,

    /* cursor commands */
    UpdateCursor = 0x0300,
    MoveCursor,

    /* success responses */
    RespOKNoData = 0x1100,
    RespOKDisplayInfo,
    RespOKCapsetInfo,
    RespOKCapset,
    RespOKEDID,

    /* error responses */
    RespErrUnspec = 0x1200,
    RespErrOutOfMemory,
    RespErrInvalidScanoutId,
    RespErrInvalidResourceId,
    RespErrInvalidContextId,
    RespErrInvalidParameter,
};

enum GPUFormat : u32 {
    B8G8R8A8 = 1,
    B8G8R8X8 = 2,
    A8R8G8B8 = 3,
    X8R8G8B8 = 4,

    R8G8B8A8 = 67,
    X8B8G8R8 = 68,

    A8B8G8R8 = 121,
    R8G8B8X8 = 134
};

enum GPUDeviceConfig {
    EventsRead = 0x00,
    EventsClear = 0x04,
    Scanouts = 0x08,
};

struct GPUControlHeader {
    u32 type;
    u32 flags;
    u64 fence_id;
    u32 ctx_id;
    u32 padding;
};

struct GPURect {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
};

struct GPUDisplayInfo {
    GPUControlHeader header;
    struct {
        GPURect rect;
        u32 enabled;
        u32 flags;
    } modes[VIRTIO_GPU_MAX_SCANOUTS];
};

struct GPUGetEDID {
    GPUControlHeader header;
    u32 scanout;
    u32 padding;
};

struct GPUGetEDIDResponse {
    GPUControlHeader header;
    u32 size;
    u32 padding;
    u8 edid[1024];
};

struct GPUResourceCreate2D {
    GPUControlHeader header;
    u32 resource_id;
    u32 format;
    u32 width;
    u32 height;
};

struct GPUMemEntry {
    u64 address;
    u32 length;
    u32 padding;
};

struct GPUResourceAttachBacking {
    GPUControlHeader header;
    u32 resource_id;
    u32 nr_entries;
    GPUMemEntry entries[];
};

struct GPUSetScanout {
    GPUControlHeader header;
    GPURect rect;
    u32 scanout_id;
    u32 resource_id;
};

struct GPUTransferToHost2D {
    GPUControlHeader header;
    GPURect rect;
    u64 offset;
    u32 resource_id;
    u32 padding;
};

struct GPUResourceFlush {
    GPUControlHeader header;
    GPURect rect;
    u32 resource_id;
    u32 padding;
};

}