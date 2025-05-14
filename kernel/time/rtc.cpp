#include <kernel/time/rtc.h>
#include <kernel/arch/io.h>

namespace kernel::rtc {

static time_t s_boot_time = 0;

static const u32 DAYS_IN_MONTH[] = {
    31, // January
    28, // February
    31, // March
    30, // April
    31, // May
    30, // June
    31, // July
    31, // August
    30, // September
    31, // October
    30, // November
    31, // December
};

constexpr u16 CMOS_ADDRESS = 0x70;
constexpr u16 CMOS_DATA = 0x71;

static constexpr u8 bcd_to_binary(u8 bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

static bool is_updating() {
    io::write<u8>(CMOS_ADDRESS, 0x0A);
    return io::read<u8>(CMOS_DATA) & 0x80;
}

static u8 read_register(u8 reg) {
    io::write<u8>(CMOS_ADDRESS, reg);
    return io::read<u8>(CMOS_DATA);
}

static bool read_time_registers(
    u32& year, u32& month, u32& day, u32& hour, u32& minute, u32& second
) {
    while (is_updating()) {
        asm volatile("pause");
    }

    u8 status_b = read_register(0x0B);

    second = read_register(0x00);
    minute = read_register(0x02);
    hour = read_register(0x04);
    day = read_register(0x07);
    month = read_register(0x08);
    year = read_register(0x09);

    bool is_pm = hour & 0x80;
    if (!(status_b & 0x04)) {
        second = bcd_to_binary(second);
        minute = bcd_to_binary(minute);
        hour = bcd_to_binary(hour & 0x7F);
        day = bcd_to_binary(day);
        month = bcd_to_binary(month);
        year = bcd_to_binary(year);
    }

    if (!(status_b & 0x02)) {
        hour = (hour % 12) + (is_pm ? 12 : 0);
    }

    year += 2000;
    return true;
}

static constexpr bool is_leap_year(u32 year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

time_t now() {
    u32 year, month, day, hour, minute, second;
    read_time_registers(year, month, day, hour, minute, second);

    time_t result = second + (minute * 60) + (hour * 3600) + (day * 86400);
    for (u32 i = 0; i < month - 1; i++) {
        result += DAYS_IN_MONTH[i] * 86400;
    }

    for (u32 i = 1970; i < year - 1; i++) {
        if (is_leap_year(i)) {
            result += 86400;
        }
    }

    result += (year - 1970) * 31536000;
    return result;
}

void init() {
    s_boot_time = now();
}

time_t boot_time() {
    return s_boot_time;
}

}