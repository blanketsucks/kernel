#pragma once

enum {
    FB_GET_RESOLUTION,

    SOUNDCARD_GET_SAMPLE_RATE,
    SOUNDCARD_SET_SAMPLE_RATE,
    SOUNDCARD_GET_CHANNELS,
    SOUNDCARD_GET_VOLUME,
    SOUNDCARD_SET_VOLUME,

    STORAGE_GET_SIZE,

    TIOCGPTN
};

struct FrameBufferResolution {
    int width;
    int height;
    int pitch;
};