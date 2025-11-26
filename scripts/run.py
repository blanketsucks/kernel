from typing import List, NamedTuple

import argparse
import pathlib
import subprocess
import sys
from enum import Enum

CWD = pathlib.Path(__file__).parent
ROOT = CWD.parent

DEFAULT_DISK_IMAGE = CWD / 'disk.img'

DEFAULT_KERNEL_LOCATION = ROOT  / 'build' / 'kernel' / 'kernel.bin'
DEFAULT_LOADER_LOCATION = ROOT / 'build' / 'kernel' / 'loader' / 'loader.bin'

DEFAULT_QEMU_ARGS: List[str] = ['-D', 'qemu.log', '-d', 'cpu_reset,int,guest_errors,unimp', '-no-reboot', '-no-shutdown']

class USBController(str, Enum):
    OHCI = 'ohci'
    UHCI = 'uhci'
    EHCI = 'ehci'
    xHCI = 'xhci'

class DiskImage(NamedTuple):
    file: str
    format: str = 'raw'
    interface: str = 'ide'
    id: str = 'disk'

    def build(self) -> str:
        return f'file={self.file},format={self.format},if={self.interface},id={self.id}'

class QemuArgs(NamedTuple):
    kernel: str
    disk_image: DiskImage
    
    memory: int

    debug: bool
    serial: bool
    monitor: bool
    x86_64: bool
    uefi: bool
    usb: bool
    usb_controller: USBController
    use_loader: bool
    virtio_gpu: bool
    kvm: bool

    ovmf: str

    cmdline: List[str] = []

    netdev: str = 'user,id=net0,hostfwd=tcp:127.0.0.1:8888-10.0.2.15:8888'
    devices: List[str] = ['ac97', 'e1000,netdev=net0']

    def build_disk_image_argument(self) -> List[str]:
        return ['-drive', self.disk_image.build()]
    
    def build_memory_argument(self) -> List[str]:
        return ['-m', str(self.memory)]
    
    def build_serial_argument(self) -> List[str]:
        if self.monitor:
            return ['-monitor', 'stdio']

        return ['-serial', 'stdio'] if self.serial else []
    
    def build_debug_argument(self) -> List[str]:
        return ['-s', '-S'] if self.debug else []
    
    def build_network_argument(self) -> List[str]:
        return ['-netdev', self.netdev]
    
    def build(self) -> List[str]:
        args = [
            *DEFAULT_QEMU_ARGS,
            *self.build_disk_image_argument(),
            *self.build_memory_argument(),
            *self.build_serial_argument(),
            *self.build_debug_argument(),
            *self.build_network_argument()
        ]

        if self.usb:
            if self.usb_controller is USBController.UHCI:
                args.append('-usb')
            elif self.usb_controller is USBController.OHCI:
                args.extend(['-device', 'pci-ohci,id=usb-bus'])
            else:
                # TODO: Support
                ...

            args.extend(['-device', 'usb-audio,bus=usb-bus.0,id=foobar'])

        if self.virtio_gpu:
            args.extend(['-device', 'virtio-gpu-pci'])

        for device in self.devices:
            args.extend(['-device', device])

        if not self.x86_64:
            args.extend(['-kernel', self.kernel])
        elif self.use_loader and not self.uefi:
            args.extend(['-kernel', str(DEFAULT_LOADER_LOCATION), '-initrd', self.kernel])
            self.cmdline.append(f'root=/dev/hda2')

        if self.uefi:
            args.extend(['-bios', self.ovmf])

        if self.kvm:
            args.extend(['-accel', 'kvm'])

        args.extend(['-append', ' '.join(self.cmdline)])
        return args

def main():
    parser = argparse.ArgumentParser(description='Run kernel in QEMU.')

    parser.add_argument('--kernel', type=str, default=DEFAULT_KERNEL_LOCATION, help='Path to kernel image.')
    parser.add_argument('--x86', action='store_true', help='Run kernel in x86 mode.')

    parser.add_argument('--qemu', type=str, default=None, help='Path to QEMU executable.')
    parser.add_argument('--disk-image', type=str, default=DEFAULT_DISK_IMAGE, help='Path to disk image.')
    parser.add_argument('--memory', type=int, default=256, help='Amount of memory to allocate to QEMU (in MB).')
    parser.add_argument('--debug', action='store_true', help='Run QEMU in debug mode and listen to GDB connections.')
    parser.add_argument('--with-monitor', action='store_true', help='Run QEMU with a monitor (useful for debugging).')
    parser.add_argument('--disable-loader', action='store_true', help='Run the kernel without the 64-bit loader.')
    parser.add_argument('--usb', action='store_true', help='Enable USB support.')
    parser.add_argument('--usb-controller', type=USBController, default='uhci', help='USB controller type (default: uhci).')
    parser.add_argument('--enable-virtio-gpu', action='store_true', help='Enable VirtIO GPU support.')
    parser.add_argument('--init', type=str, default='/bin/shell', help='Initial process to run.')
    parser.add_argument('--uefi', action='store_true', help='Run kernel in UEFI mode.')
    parser.add_argument('--kvm', action='store_true', help='Enable KVM.')

    action = parser.add_argument('--ovmf', type=str, default=None, help='Path to OVMF firmware')

    if sys.platform == 'linux':
        action.default = '/usr/share/ovmf/OVMF.fd'
    elif sys.platform == 'darwin':
        action.default = '/usr/local/share/qemu/OVMF.fd'
    elif sys.platform == 'win32':
        action.default = 'C:/Program Files/qemu/OVMF.fd'
    
    args = parser.parse_args()
    if not args.disable_loader and args.x86:
        print('Loader is only supported in x86_64 mode.')
        return

    qemu_args = QemuArgs(
        disk_image=DiskImage(args.disk_image),
        kernel=str(args.kernel),
        memory=args.memory,
        debug=args.debug,
        monitor=args.with_monitor,
        x86_64=not args.x86,
        uefi=args.uefi,
        ovmf=args.ovmf,
        kvm=args.kvm,
        serial=True,
        usb=args.usb,
        usb_controller=args.usb_controller,
        use_loader=not args.disable_loader,
        virtio_gpu=args.enable_virtio_gpu,
    )

    qemu_args.cmdline.append(f'init={args.init}')

    if not args.qemu:
        args.qemu = 'qemu-system-i386' if args.x86 else 'qemu-system-x86_64'

    command = [args.qemu, *qemu_args.build()]
    print(' '.join(command), end='\n\n')

    try:
        subprocess.run(command)
    except KeyboardInterrupt:
        pass

if __name__ == '__main__':
    main()
    

    