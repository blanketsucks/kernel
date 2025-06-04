#pragma once

enum {
    GPU_GET_CONNECTOR_COUNT,
    GPU_GET_CONNECTORS,

    GPU_CONNECTOR_MAP_FB,
    GPU_CONNECTOR_FLUSH,

    SOUNDCARD_GET_SAMPLE_RATE,
    SOUNDCARD_SET_SAMPLE_RATE,
    SOUNDCARD_GET_CHANNELS,
    SOUNDCARD_GET_VOLUME,
    SOUNDCARD_SET_VOLUME,

    STORAGE_GET_SIZE,

    TIOCGPTN
};

struct gpu_connector_map_fb {
    int id;
    void* framebuffer;
};

struct gpu_connector_flush {
    int id;
};

struct gpu_connector {
    int id;

    int width;
    int height;
    int pitch;
    int bpp;
};
