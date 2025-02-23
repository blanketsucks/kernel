#pragma once

#define FB_GET_RESOLUTION 1

#define SOUNDCARD_GET_SAMPLE_RATE 1
#define SOUNDCARD_SET_SAMPLE_RATE 2
#define SOUNDCARD_GET_CHANNELS 3
#define SOUNDCARD_GET_VOLUME 4
#define SOUNDCARD_SET_VOLUME 5

struct FrameBufferResolution {
    int width;
    int height;
    int pitch;
};