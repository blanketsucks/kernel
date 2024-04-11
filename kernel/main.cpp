#include <kernel/common.h>
#include <kernel/multiboot.h>
#include <kernel/vga.h>
#include <kernel/io.h>
#include <kernel/serial.h>
#include <kernel/ctors.h>
#include <kernel/font.h>
#include <kernel/pci.h>
#include <kernel/mbr.h>
#include <kernel/elf.h>
#include <kernel/symbols.h>

#include <std/result.h>
#include <std/vector.h>
#include <std/utility.h>
#include <std/memory.h>
#include <std/hash_map.h>

#include <kernel/acpi/smbios.h>
#include <kernel/acpi/acpi.h>

#include <kernel/time/rtc.h>
#include <kernel/time/pit.h>

#include <kernel/cpu/gdt.h>
#include <kernel/cpu/idt.h>
#include <kernel/cpu/pic.h>
#include <kernel/cpu/cpu.h>
#include <kernel/cpu/apic.h>

#include <kernel/memory/physical.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/mm.h>
#include <kernel/memory/liballoc.h>

#include <kernel/devices/keyboard.h>
#include <kernel/devices/mouse.h>
#include <kernel/devices/device.h>
#include <kernel/devices/pcspeaker.h>
#include <kernel/devices/bochs_vga.h>
#include <kernel/devices/pata.h>
#include <kernel/devices/partition.h>

#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/threads.h>

#include <kernel/syscalls/syscalls.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ext2fs/filesystem.h>

using namespace kernel;
using namespace kernel::devices;

multiboot_info_t* load_multiboot_header(u32 address);

struct Command {
    String name;
    Vector<String> args;

    Command(String name, Vector<String> args) : name(move(name)), args(move(args)) {}

    StringView get(size_t index) const {
        if (index >= args.size()) { return StringView(); }
        return args[index];
    }

    bool has(size_t index) const { return index < args.size(); }
};

void process_commands();
void run_command(Command command);

Command parse_shell_command(StringView line) {
    Vector<String> args;
    String name;

    size_t index = line.find(' ');

    if (index == StringView::npos) {
        return { line, move(args) };
    } else {
        name = line.substr(0, index);
        line = line.substr(index + 1);
    }

    while (!line.empty()) {
        bool in_quotes = false;
        if (line.startswith('"')) {
            in_quotes = true;
            line = line.substr(1);
        }

        if (in_quotes) {
            index = line.find('"');
        } else {
            index = line.find(' ');
        }

        if (index == StringView::npos) {
            args.append(line);
            break;
        }

        args.append(line.substr(0, index));
        line = line.substr(index + 1);
    }

    return { move(name), move(args) };
}

struct Vector2 {
    i32 x;
    i32 y;
};

void draw_rect(BochsVGADevice* framebuffer, Vector2 position, Vector2 size, u32 color) {
    for (i32 y = position.y; y < position.y + size.y; y++) {
        for (i32 x = position.x; x < position.x + size.x; x++) {
            framebuffer->set_pixel(x, y, color);
        }
    }
}

void draw_line(BochsVGADevice* framebuffer, Vector2 start, Vector2 end, u32 color) {
    i32 dx = end.x - start.x;
    i32 dy = end.y - start.y;

    if (!dx && !dy) {
        framebuffer->set_pixel(start.x, start.y, color);
        return;
    }

    if (std::abs(dx) > std::abs(dy)) {
        if (start.x > end.x) {
            std::swap(start, end);
        }

        for (i32 x = start.x; x <= end.x; x++) {
            i32 y = start.y + dy * (x - start.x) / dx;
            framebuffer->set_pixel(x, y, color);
        }
    } else {
        if (start.y > end.y) {
            std::swap(start, end);
        }

        for (i32 y = start.y; y <= end.y; y++) {
            i32 x = start.x + dx * (y - start.y) / dy;
            framebuffer->set_pixel(x, y, color);
        }
    }
}

void draw_triangle(BochsVGADevice* framebuffer, Vector2 a, Vector2 b, Vector2 c, u32 color) {
    draw_line(framebuffer, a, b, color);
    draw_line(framebuffer, b, c, color);
    draw_line(framebuffer, c, a, color);
}

Vector2 barycentric(Vector2 a, Vector2 b, Vector2 c) {
    return {
        (a.x + b.x + c.x) / 3,
        (a.y + b.y + c.y) / 3
    };
}

extern "C" void main(multiboot_info_t* ptr) {
    auto header = *ptr;

    serial::init();
    vga::clear();

    kernel::run_global_constructors();

    cpu::init_gdt();
    cpu::init_idt();
    
    pic::init();
    pit::init();

    DeviceManager::init();
    KeyboardDevice::init();
    MouseDevice::init();

    memory::MemoryManager::init(&header);
    asm volatile("sti");

    auto* device = BochsVGADevice::create(800, 600);

    serial::printf("PCI Bus:\n");
    pci::enumerate([](pci::Device device) {
        serial::printf("  Device: '%s: %s'\n", device.class_name().data(), device.subclass_name().data());
    });

    PATADevice* disk = PATADevice::create(ATAChannel::Primary, ATADrive::Master);
    if (!disk) {
        serial::printf("Failed to create ATA device\n");
        return;
    }

    MasterBootRecord mbr = {};
    disk->read_sectors(0, 1, reinterpret_cast<u8*>(&mbr));

    u32 offset = 0;
    // FIXME: This assumes that the OS partition is always the first
    if (!mbr.is_protective()) {
        for (auto& partition : mbr.partitions) {
            if (!partition.is_bootable()) {
                continue;
            }

            offset = partition.offset;
            break;
        }
    } else {
        GPTHeader gpt = {};
        disk->read_sectors(1, 1, reinterpret_cast<u8*>(&gpt));

        if (std::memcmp(gpt.signature, "EFI PART", 8) != 0) {
            serial::printf("Invalid GPT header signature\n");
            return;
        }

        Vector<GPTEntry> entries;
        entries.resize(gpt.partition_count);

        disk->read_sectors(gpt.partition_table_lba, gpt.partition_count, reinterpret_cast<u8*>(entries.data()));

        for (auto& entry : entries) {
            if (!entry.is_valid()) {
                continue;
            }

            offset = entry.first_lba;
            break;
        }
    }

    BlockDevice* partition = nullptr;
    if (offset != 0) {
        partition = PartitionDevice::create(3, 1, disk, offset);
    } else {
        partition = disk;
    }

    auto fs = ext2fs::FileSystem::create(partition);
    if (!fs) {
        serial::printf("Could not create main ext2 filesystem.\n");
        return;
    }

    auto vfs = fs::vfs();
    vfs->mount_root(fs);

    Vector2 a = { 100, 25 };
    Vector2 b = { 200, 50 };
    Vector2 c = { 150, 200 };

    draw_triangle(device, a, b, c, 0x34EBB7);

    Vector2 center = barycentric(a, b, c);
    device->set_pixel(center.x, center.y, 0xFF0000);

    auto* parser = acpi::Parser::instance();
    parser->init();

#if 0
    ELF elf(inode->file());
    elf.read_program_headers();

    auto& file = elf.file();
    auto& interpreter = elf.interpreter();

    serial::printf("Interpreter: %*s\n", interpreter.size(), interpreter.data());
    serial::printf("Entry point: %u\n", elf.header()->e_entry);

    auto mm = memory::MemoryManager::instance();
    
    // Calculate the total number of pages needed to load the ELF file
    size_t total_pages = 0;
    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD) continue;
        total_pages += (ph.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
    }

    serial::printf("Total pages: %u\n", total_pages);
    u8* region = static_cast<u8*>(mm->allocate_heap_region(total_pages).value());

    for (auto& ph : elf.program_headers()) {
        if (ph.p_type != PT_LOAD) continue;

        file.seek(ph.p_offset);
        file.read(region + ph.p_vaddr, ph.p_filesz);
    }

    auto entry = reinterpret_cast<int(*)()>(region + elf.header()->e_entry);
    serial::printf("Result: %d\n", entry());
#endif

    for (;;) {
        asm volatile("hlt");
    }
}

void find_hardware_rng() {
    cpu::CPUID cpuid(7);
    if (cpuid.ebx() & bit_RDSEED) {
        serial::printf("rdseed instruction supported.\n");
        return;
    }

    cpuid = cpu::CPUID(1);
    if (!(cpuid.ecx() & bit_RDRND)) {
        serial::printf("No hardware random number generator available.\n");
        return;
    }
    
    serial::printf("rdrand instruction supported.\n");
}

multiboot_info_t* load_multiboot_header(u32 address) {
    return reinterpret_cast<multiboot_info_t*>(address);
}
