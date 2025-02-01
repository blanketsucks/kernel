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
#include <std/format.h>

#include <kernel/acpi/smbios.h>
#include <kernel/acpi/acpi.h>

#include <kernel/time/rtc.h>
#include <kernel/time/pit.h>

#include <kernel/memory/physical.h>
#include <kernel/memory/manager.h>
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
#include <kernel/process/syscalls.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/ptsfs.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ext2fs/filesystem.h>

#include <kernel/arch/arch.h>
#include <kernel/arch/boot_info.h>

#include <kernel/tty/virtual.h>

using namespace kernel;
using namespace kernel::devices;

static PATADevice* disk = nullptr;

void stage2();

String format_guid(Array<u8, 16> guid) {
    String result;

    for (size_t i = 4; i > 0; i--) {
        result.append(std::format("{:02X}", guid[i - 1]));
    }

    result.append('-');
    for (size_t i = 6; i > 4; i--) {
        result.append(std::format("{:02X}", guid[i - 1]));
    }

    result.append('-');
    for (size_t i = 8; i > 6; i--) {
        result.append(std::format("{:02X}", guid[i - 1]));
    }

    result.append('-');
    for (size_t i = 8; i < guid.size(); i++) {
        if (i == 10) {
            result.append('-');
        }

        result.append(std::format("{:02X}", guid[i]));
    }

    return result;
}

arch::BootInfo const* g_boot_info = nullptr;
extern "C" void main(arch::BootInfo const& boot_info) {
    g_boot_info = &boot_info;

    serial::init();
    kernel::run_global_constructors();
    arch::init();

    pic::init();

    memory::MemoryManager::init();

    asm volatile("sti");

    dbgln("PCI Bus:");
    pci::enumerate([](pci::Device device) {
        dbgln("  {}: {}", device.class_name(), device.subclass_name());
    });

    dbgln();

    acpi::Parser parser;
    parser.init();

    KeyboardDevice::init();
    // while (1) {
    //     asm volatile("hlt");
    // }

    // pit::init();

    // MouseDevice::init();

    // asm volatile("sti");

    // BochsVGADevice::create(800, 600);

    disk = PATADevice::create(ATAChannel::Primary, ATADrive::Master);
    if (!disk) {
        serial::printf("Failed to create ATA device\n");
        return;
    }

    // auto* process = Process::create_kernel_process("Kernel Stage 2", stage2);
    // Scheduler::add_process(process);

    // Scheduler::init();

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

        if (memcmp(gpt.signature, "EFI PART", 8) != 0) {
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

            dbgln("GPT Entry:");
            dbgln("  Name: {}", entry.name());
            dbgln("  First LBA: {}", entry.first_lba);
            dbgln("  Last LBA: {}", entry.last_lba);
            dbgln("  Attributes: {}", entry.attributes);
            dbgln("  Type GUID: {}", format_guid(entry.partition_type_guid));
            dbgln("  Is EFI system Partition: {}", entry.is_efi_system_partition());

            offset = entry.first_lba;
        }
    }

    // BlockDevice* partition = nullptr;
    // if (offset != 0) {
    //     partition = PartitionDevice::create(3, 1, disk, offset);
    // } else {
    //     partition = disk;
    // }

    // auto* fs = ext2fs::FileSystem::create(partition);
    // if (!fs) {
    //     serial::printf("Could not create main ext2 filesystem.\n");
    //     return;
    // }

    // auto* vfs = fs::vfs();
    // vfs->mount_root(fs);

    // Vector2 a = { 100, 25 };
    // Vector2 b = { 200, 50 };
    // Vector2 c = { 150, 200 };

    // draw_triangle(device, a, b, c, 0x34EBB7);

    // Vector2 center = barycentric(a, b, c);
    // device->set_pixel(center.x, center.y, 0xFF0000);

    // parse_symbols_from_fs();

    // auto* parser = acpi::Parser::instance();
    // parser->init();

    // auto* ptsfs = new fs::PTSFS();
    // ptsfs->init();

    // vfs->mount(ptsfs, vfs->resolve("/dev/pts").unwrap());

    // auto* tty0 = new VirtualTTY(0);

    // auto fd = vfs->open("/bin/shell", O_RDONLY, 0).unwrap();
    // ELF elf(fd);

    // auto cwd = vfs->resolve("/").unwrap();

    // ProcessArguments arguments;
    // arguments.argv = { "/bin/shell" };

    // auto* process = Process::create_user_process("/bin/shell", elf, cwd, move(arguments), tty0);

    // auto* kprocess = Process::create_kernel_process("kernel");

    // Scheduler::add_process(process);
    // Scheduler::init();

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

void stage2() {
    auto* process = Scheduler::current_process();

    auto* fs = ext2fs::FileSystem::create(disk);
    if (!fs) {
        serial::printf("Could not create main ext2 filesystem.\n");
    }

    auto* vfs = fs::vfs();
    vfs->mount_root(fs);
    
    parse_symbols_from_fs();
    auto* ptsfs = new fs::PTSFS();
    ptsfs->init();

    vfs->mount(ptsfs, vfs->resolve("/dev/pts").unwrap());

    auto* tty0 = new VirtualTTY(0);
    
    auto fd = vfs->open("/bin/test", O_RDONLY, 0).unwrap();
    ELF elf(fd);

    auto cwd = vfs->resolve("/").unwrap();

    ProcessArguments arguments;
    arguments.argv = { "/bin/test" };

    auto* shell = Process::create_user_process("/bin/test", elf, cwd, move(arguments), tty0);
    dbgln("Kernel Stage 2 finished");
    Scheduler::add_process(shell);


    process->sys$exit(0);
}