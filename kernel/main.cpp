#include <kernel/common.h>
#include <kernel/vga.h>
#include <kernel/arch/io.h>
#include <kernel/serial.h>
#include <kernel/ctors.h>
#include <kernel/pci/pci.h>
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
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/manager.h>

#include <kernel/memory/manager.h>
#include <kernel/memory/liballoc.h>

#include <kernel/devices/device.h>
#include <kernel/devices/null.h>
#include <kernel/devices/zero.h>
#include <kernel/devices/devctl.h>
#include <kernel/devices/audio/ac97.h>
#include <kernel/devices/input/mouse.h>
#include <kernel/devices/gpu/manager.h>
#include <kernel/devices/input/keyboard.h>
#include <kernel/devices/audio/pcspeaker.h>
#include <kernel/devices/storage/ide/controller.h>
#include <kernel/devices/storage/ahci/controller.h>

#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/syscalls.h>
#include <kernel/process/elf.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/ptsfs.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ext2fs/filesystem.h>

#include <kernel/arch/cpu.h>
#include <kernel/arch/processor.h>
#include <kernel/arch/pic.h>
#include <kernel/boot/command_line.h>
#include <kernel/boot/boot_info.h>
#include <kernel/arch/apic.h>

#include <kernel/tty/virtual.h>
#include <kernel/tty/multiplexer.h>

#include <kernel/net/manager.h>

#include <kernel/usb/uhci/controller.h>
#include <kernel/usb/pipe.h>
#include <kernel/usb/device.h>
#include <kernel/usb/usb.h>

#include <kernel/virtio/virtio.h>
#include <kernel/virtio/device.h>

using namespace kernel;

void stage2();

BootInfo const* g_boot_info = nullptr;
extern "C" void main(BootInfo const& boot_info) {
    g_boot_info = &boot_info;
    kernel::run_global_constructors();

    serial::init();
    Processor::init();

    pic::init();
    
    memory::MemoryManager::init();
    TimeManager::init();

    asm volatile("sti");

    auto* process = Process::create_kernel_process("Kernel Stage 2", stage2);
    Scheduler::add_process(process);

    Scheduler::init();

    for (;;) {
        asm volatile("hlt");
    }
}

void stage2() {
    auto* process = Process::current();

    dbgln("PCI Bus:");
    pci::enumerate([](pci::Device device) {
        dbgln(" - {}: {}", device.class_name(), device.subclass_name());
    });
    
    dbgln();

    CommandLine::init();

    NullDevice::create();
    ZeroDevice::create();
    DeviceControl::create();
    
    AC97Device::create();

    usb::UHCIController::create();

    GPUManager::initialize();
    NetworkManager::initialize();
    StorageManager::initialize();

    auto* disk = StorageManager::determine_boot_device();

    if (!disk) {
        auto* cmdline = CommandLine::instance();
        dbgln("Could not find boot device: '{}'\n", cmdline->root());

        process->sys$exit(1);
    }

    auto* fs = ext2fs::FileSystem::create(disk);
    if (!fs) {
        dbgln("Could not create main ext2 filesystem.\n");
        process->sys$exit(1);
    }

    auto* vfs = fs::vfs();
    vfs->mount_root(fs);

    parse_symbols_from_fs();

    PTYMultiplexer::create();

    auto* ptsfs = new fs::PTSFS();
    ptsfs->init();

    vfs->mount(ptsfs, vfs->resolve("/dev/pts").unwrap());

    auto* tty0 = VirtualTTY::create(0);
    
    auto fd = vfs->open("/bin/shell", O_RDONLY, 0).unwrap();
    ELF elf(fd);

    ProcessArguments arguments;
    arguments.argv = { "/bin/shell" };

    Process* shell = Process::create_user_process("/bin/shell", elf, nullptr, move(arguments), tty0);
    Scheduler::add_process(shell);

    process->sys$exit(0);
}