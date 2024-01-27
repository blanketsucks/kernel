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

#include <std/result.h>
#include <std/vector.h>
#include <std/utility.h>
#include <std/memory.h>
#include <std/hash_map.h>

#include <kernel/time/rtc.h>

#include <kernel/cpu/gdt.h>
#include <kernel/cpu/idt.h>
#include <kernel/cpu/pic.h>
#include <kernel/cpu/cpu.h>

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

#include <kernel/syscalls/syscalls.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ext2fs/filesystem.h>

using namespace kernel;
using namespace kernel::devices;

multiboot_info_t* load_multiboot_header(u32 address);
void dump_mmap_entries(multiboot_info_t* header);

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

struct PSF2Header {
    u32 magic;
    u32 version;
    u32 header_size;
    u32 flags;
    u32 glyph_count;
    u32 glyph_size;
    u32 height;
    u32 width;
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

extern "C" u64 _kernel_start;
extern "C" u64 _kernel_end;

extern "C" void main(u32 ptr) {
    asm volatile("cli");

    serial::init();
    vga::clear();

    kernel::run_global_constructors();

    multiboot_info_t header = *load_multiboot_header(ptr);
    pic::remap();

    cpu::init_gdt();
    cpu::init_idt();
    kernel::init_syscalls();

    DeviceManager::init();
    KeyboardDevice::init();
    MouseDevice::init();

    memory::MemoryManager::init(&header);
    asm volatile("sti");

    auto* device = BochsVGADevice::create(640, 480);
    pci::enumerate([](pci::Device device) {
        serial::printf("PCI Device: '%s: %s'\n", device.class_name().data(), device.subclass_name().data());
    });

    PATADevice* disk = PATADevice::create(ATAChannel::Primary, ATADrive::Master);
    if (!disk) {
        serial::printf("Failed to create ATA device\n");
        return;
    }

    MasterBootRecord mbr = {};
    disk->read_sectors(0, 1, reinterpret_cast<u8*>(&mbr));

    u32 offset = 0;
    for (auto& partition : mbr.partitions) {
        if (!partition.is_bootable()) {
            continue;
        }

        offset = partition.lba_start;
        break;
    }

    BlockDevice* partition = nullptr;
    if (offset != 0) {
        partition = PartitionDevice::create(3, 1, disk, offset);
    } else {
        partition = disk;
    }

    auto fs = ext2fs::FileSystem::create(partition);
    if (!fs) {
        kernel::panic("Could not create the ext2 filesystem.");
    }

    auto root = fs->inode(fs->root());
    for (auto& entry : root->readdir()) {
        serial::printf("Entry (%d): %*s\n", entry.type, entry.name.size(), entry.name.data());
    }

    fs::VFS vfs;
    vfs.mount_root(fs);

    // ino_t id = fs->resolve("/boot/test");
    // serial::printf("Inode ID: %u\n", id);

    // auto inode = fs->inode(id);
    // if (!inode) {
    //     serial::printf("Failed to get inode\n");
    //     return;
    // }


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
}

void find_hardware_rng() {
    cpu::CPUID cpuid(7);
    if (cpuid.ebx() & bit_RDSEED) {
        serial::printf("rdseed instruction supported.\n");
    } else {
        cpuid = cpu::CPUID(1);
        if (cpuid.ecx() & bit_RDRND) {
            serial::printf("rdrand instruction supported.\n");
        } else {
            serial::printf("No hardware random number generator available.\n");
        }
    }
}

multiboot_info_t* load_multiboot_header(u32 address) {
    return reinterpret_cast<multiboot_info_t*>(address + KERNEL_VIRTUAL_BASE);
}

