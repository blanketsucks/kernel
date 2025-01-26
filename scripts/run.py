from typing import List, NamedTuple

import argparse
import pathlib
import subprocess

CWD = pathlib.Path(__file__).parent
ROOT = CWD.parent

DEFAULT_DISK_IMAGE = CWD / 'disk.img'
DEFAULT_KERNEL_LOCATION = ROOT  / 'build' / 'kernel' / 'kernel.bin'

DEFAULT_QEMU_ARGS: List[str] = ['-D', 'qemu.log', '-d', 'cpu_reset,int', '-no-reboot', '-no-shutdown']

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

    debug: bool = False
    serial: bool = True
    monitor: bool = False

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
    
    def build(self) -> List[str]:
        return [
            *DEFAULT_QEMU_ARGS,
            *self.build_disk_image_argument(),
            *self.build_memory_argument(),
            *self.build_serial_argument(),
            *self.build_debug_argument(),
            '-kernel', self.kernel
        ]

def main():
    parser = argparse.ArgumentParser(description='Run kernel in QEMU.')

    parser.add_argument('--kernel', type=str, default=DEFAULT_KERNEL_LOCATION, help='Path to kernel image.')

    parser.add_argument('--qemu', type=str, default='qemu-system-i386', help='Path to QEMU executable.')
    parser.add_argument('--disk-image', type=str, default=DEFAULT_DISK_IMAGE, help='Path to disk image.')
    parser.add_argument('--memory', type=int, default=512, help='Amount of memory to allocate to QEMU (in MB).')
    parser.add_argument('--debug', action='store_true', help='Run QEMU in debug mode and listen to GDB connections.')
    parser.add_argument('--with-monitor', action='store_true', help='Run QEMU with a monitor (useful for debugging).')

    args = parser.parse_args()
    qemu_args = QemuArgs(
        disk_image=DiskImage(args.disk_image),
        kernel=args.kernel,
        memory=args.memory,
        debug=args.debug,
        monitor=args.with_monitor
    )

    command = [args.qemu, *qemu_args.build()]
    try:
        subprocess.run(command)
    except KeyboardInterrupt:
        pass

if __name__ == '__main__':
    main()
    

    