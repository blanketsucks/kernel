import subprocess
import pathlib

addrs = [
    0xffffffff8002bcaf,
    0xffffffff80004ba8,
    0xffffffff8003b8a0,
    0xffffffff8003b8e1,
    0xffffffff8000cc89,
    0xffffffff8000c903,
    0xffffffff80011b68,
    0xffffffff80012336,
    0xffffffff8002e5c4,
    0xffffffff8002e349,
    0xffffffff8002e3e0,
    0xffffffff8002e424,
    0xffffffff8002e48d,
    0xffffffff80011ca7,
    0xffffffff80011ad6,
    0xffffffff800230a3
]

ROOT = pathlib.Path(__file__).parent.resolve()
KERNEL = ROOT.parent / 'build' / 'kernel' / 'kernel.bin'

result ,= subprocess.run(['addr2line', '-e', str(KERNEL), *[hex(addr) for addr in addrs]], capture_output=True, text=True),
for line, address in zip(result.stdout.strip().split('\n'), addrs):
    print(f'0x{address:x} -> {line}')