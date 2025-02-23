#include <kernel/common.h>
#include <kernel/vga.h>
#include <kernel/io.h>
#include <kernel/serial.h>
#include <kernel/ctors.h>
#include <kernel/font.h>
#include <kernel/pci.h>
#include <kernel/mbr.h>
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
#include <kernel/devices/ac97.h>
#include <kernel/devices/partition.h>

#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/syscalls.h>
#include <kernel/process/elf.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/ptsfs.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ext2fs/filesystem.h>

#include <kernel/arch/arch.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/boot_info.h>
#include <kernel/arch/processor.h>

#include <kernel/tty/virtual.h>

using namespace kernel;
using namespace kernel::devices;

static BlockDevice* s_disk = nullptr;

void stage2();

void test_usb(pci::Device device) {
    u8 interface = device.address.prog_if();
    if (interface != 0x00) {
        return;
    }

    device.address.bar4();
}

struct Foo {};

arch::BootInfo const* g_boot_info = nullptr;
extern "C" void main(arch::BootInfo const& boot_info) {
    g_boot_info = &boot_info;
    kernel::run_global_constructors();

    serial::init();
    Processor::init();

    pic::init();
    pit::init();

    KeyboardDevice::init();
    MouseDevice::init();
    
    memory::MemoryManager::init();
    
    asm volatile("sti");
    
    dbgln("PCI Bus:");
    pci::enumerate([](pci::Device device) {
        dbgln("  {}: {}", device.class_name(), device.subclass_name());
        
        if (device.is_usb_controller()) {
            test_usb(device);
        }
    });
    
    dbgln();
    
    AC97Device::create();

    PATADevice* disk = PATADevice::create(ATAChannel::Primary, ATADrive::Master);
    if (!disk) {
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

        if (memcmp(gpt.signature, "EFI PART", 8) != 0) {
            dbgln("Invalid GPT header signature\n");
            return;
        }

        Vector<GPTEntry> entries;
        entries.resize(gpt.partition_count);

        disk->read_sectors(gpt.partition_table_lba, gpt.partition_count, reinterpret_cast<u8*>(entries.data()));

        offset = entries[1].first_lba; // FIXME: This assumes that the OS partition is always the second
        for (auto& entry : entries) {
            if (!entry.is_valid()) {
                continue;
            }

            dbgln("GPT Entry:");
            dbgln("  Name: {}", entry.name());
            dbgln("  First LBA: {}", entry.first_lba);
            dbgln("  Last LBA: {}", entry.last_lba);
            dbgln("  Attributes: {}", entry.attributes);
            dbgln("  Is EFI system Partition: {}", entry.is_efi_system_partition());

            // offset = entry.first_lba;
        }
    }

    if (offset != 0) {
        s_disk = PartitionDevice::create(3, 1, disk, offset);
    } else {
        s_disk = disk;
    }

    auto* process = Process::create_kernel_process("Kernel Stage 2", stage2);
    Scheduler::add_process(process);

    Scheduler::init();

    for (;;) {
        asm volatile("hlt");
    }
}

void stage2() {
    auto* process = Scheduler::current_process();

    auto* fs = ext2fs::FileSystem::create(s_disk);
    if (!fs) {
        dbgln("Could not create main ext2 filesystem.\n");
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
    Scheduler::add_process(shell);

    process->sys$exit(0);
}