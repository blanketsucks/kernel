import subprocess
import sys

try:
    binary = sys.argv[1]
except IndexError:
    print(f'Usage: {sys.executable} map.py <binary>')
    sys.exit(1)

process = subprocess.Popen(['nm', '-n', '-C', binary], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout, _ = process.communicate()
stdout = stdout.decode('utf-8')

map = open('kernel.map', 'w')

for line in stdout.split('\n'):
    if not line:
        continue
    
    address, type, name = line.split(' ', 2)
    if type not in ('T', 'W'):
        continue

    map.write(f'{address} {name}\n')

map.close()