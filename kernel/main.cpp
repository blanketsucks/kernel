#include <kernel/common.h>
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
#include <kernel/fs/devfs/filesystem.h>

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
#include <kernel/usb/ohci/controller.h>
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
    
    MemoryManager::init();
    TimeManager::init();

    devfs::init();

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

    PCI::initialize();
    PCI::enumerate([](pci::Device device) {
        dbgln(" - {}: {}", device.class_name(), device.subclass_name());
    });
    
    dbgln();

    CommandLine::initialize();

    NullDevice::create();
    ZeroDevice::create();
    DeviceControl::create();
    
    AC97Device::create();

    usb::UHCIController::create();
    usb::OHCIController::create();

    InputManager::initialize();
    GPUManager::initialize();
    NetworkManager::initialize();
    StorageManager::initialize();

    auto* disk = StorageManager::determine_boot_device();
    auto* cmdline = CommandLine::instance();

    if (!disk) {
        dbgln("Could not find boot device: '{}'\n", cmdline->root());
        process->sys$exit(1);
    }

    auto* fs = ext2fs::FileSystem::create(disk);
    if (!fs) {
        dbgln("Could not create main ext2 filesystem for '{}'.\n", cmdline->root());
        process->sys$exit(1);
    }

    auto* vfs = fs::vfs();
    vfs->mount_root(fs);

    parse_symbols_from_fs();

    devfs::mount();

    PTYMultiplexer::create();

    auto* ptsfs = new fs::PTSFS();
    ptsfs->init();

    MUST(vfs->mount(ptsfs, MUST(vfs->resolve("/dev/pts"))));

    // Before we hand off control to userspace, we need to ensure that all the previous device events have been processed.
    // If not we might end up with a situation where a userspace process tries to open a device that has been initialized but not yet registered
    // in the devfs.
    while (devfs::has_events()) {}

    auto* tty0 = VirtualTTY::create(0);
    auto result = Process::create_user_process(cmdline->init(), nullptr, tty0);

    if (result.is_err()) {
        dbgln("Failed to create user process: errno={}", result.error().code());
        process->sys$exit(1);
    }

    Process* shell = result.release_value();
    Scheduler::add_process(shell);

    process->sys$exit(0);
}