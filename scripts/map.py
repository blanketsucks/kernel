import subprocess
import sys
import pathlib
import struct

CWD = pathlib.Path(__file__).parent
ROOT = CWD.parent

DEFAULT_KERNEL_LOCATION = ROOT  / 'build' / 'kernel' / 'kernel.bin'

try:
    binary = sys.argv[1]
except IndexError:
    binary = str(DEFAULT_KERNEL_LOCATION)

process = subprocess.Popen(['nm', '-n', '-C', binary], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout, _ = process.communicate()
stdout = stdout.decode('utf-8')

map = open('kernel.map', 'wb')

for line in stdout.split('\n'):
    if not line:
        continue
    
    address, type, name = line.split(' ', 2)
    if type not in ('T', 'W'):
        continue

    entry = struct.pack('QI', int(address, 16), len(name));

    map.write(entry)
    map.write(name.encode('ascii'))

null_entry = struct.pack('QI', 0, 0)
map.write(null_entry)

map.close()