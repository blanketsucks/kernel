from typing import NamedTuple
import subprocess
import sys
import pathlib
import struct

CWD = pathlib.Path(__file__).parent
ROOT = CWD.parent

DEFAULT_KERNEL_LOCATION = ROOT  / 'build' / 'kernel' / 'kernel.bin'
MAP_FILE = CWD / 'kernel.map'

class Entry(NamedTuple):
    address: int
    name: str

try:
    binary = sys.argv[1]
except IndexError:
    binary = str(DEFAULT_KERNEL_LOCATION)

process = subprocess.Popen(['nm', '-n', '-C', binary], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout, _ = process.communicate()
stdout = stdout.decode('utf-8')

map = open(MAP_FILE, 'wb')

entries: list[Entry] = []
for line in stdout.split('\n'):
    if not line:
        continue
    
    address, type, name = line.split(' ', 2)
    if type not in ('T', 'W'):
        continue

    entries.append(Entry(int(address, 16), name))

map.write(struct.pack('I', len(entries)))
for (address, name) in entries:
    map.write(struct.pack('QI', address, len(name)))
    map.write(name.encode('ascii'))

map.close()