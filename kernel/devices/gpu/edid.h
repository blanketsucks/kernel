#pragma once

#include <kernel/common.h>

namespace kernel {

struct StandardTiming {
    u8 x_resolution;
    u8 vertical_frequency : 4;
    u8 aspect_ratio : 4;
} PACKED;

struct DetailedTimingDescriptor {
    u16 pixel_clock;
    u8 horizontal_active_pixels_low;
    u8 horizontal_blanking_pixels_low;
    u8 horizontal_blanking_pixels_high : 4;
    u8 horizontal_active_pixels_high : 4;
    u8 vertical_active_lines_low;
    u8 vertical_blanking_lines_low;
    u8 vertical_blanking_lines_high : 4;
    u8 vertical_active_lines_high : 4;
    u8 horizontal_front_porch_low;
    u8 horizontal_sync_pulse_width_low;
    u8 veritical_sync_pulse_width_low : 4;
    u8 vertical_front_porch_low : 4;
    u8 vertical_sync_pulse_width_high : 2;
    u8 vertical_front_porch_high : 2;
    u8 horizontal_sync_pulse_width_high : 2;
    u8 horizontal_front_porch_high : 2;
    u8 horizontal_image_size_mm_low;
    u8 vertical_image_size_mm_low;
    u8 vertical_image_size_mm_high : 4;
    u8 horizontal_image_size_mm_high : 4;
    u8 horizontal_border_pixels_low;
    u8 vertical_border_pixels_low;
    u8 features;
} PACKED;

struct MonitorDescriptor {
    u16 descriptor;
    u8 reserved;
    u8 type;
    u8 reserved1;
    u8 data[13];
} PACKED;

struct EDID {
    u64 header;
    u16 manufacturer_id;
    u16 product_code;
    u32 serial_number;
    u8 week_of_manufacture;
    u8 year_of_manufacture;
    u8 version;
    u8 revision;
    u8 video_input_parameters;
    u8 horizontal_size_cm;
    u8 vertical_size_cm;
    u8 display_gamma;
    u8 features;
    u8 red_green_low_bits;
    u8 blue_white_low_bits;
    u8 red_x_high_bits;
    u8 red_y_high_bits;
    u8 green_x_high_bits;
    u8 green_y_high_bits;
    u8 blue_x_high_bits;
    u8 blue_y_high_bits;
    u8 default_white_x_high_bits;
    u8 default_white_y_high_bits;
    u32 established_timings : 24;
    StandardTiming standard_timings[8];
    union {
        DetailedTimingDescriptor detailed_timing;
        MonitorDescriptor monitor_descriptor;
    } timings[4];
    u8 extension_flag;
    u8 checksum;
} PACKED;


}