mkdir -p ./mnt/dev
mkdir -p ./mnt/dev/input
mkdir -p ./mnt/dev/pts

mknod ./mnt/dev/input/mouse c 13 0
mknod ./mnt/dev/input/keyboard c 13 1

mknod ./mnt/dev/zero c 1 5
mknod ./mnt/dev/null c 1 3

mknod ./mnt/dev/ptmx c 5 2

mknod ./mnt/dev/fb0 b 29 0

mkdir -p ./mnt/boot
cp ./kernel.map ./mnt/boot

mkdir ./mnt/home
cp ./test.txt ./mnt/home/test.txt

cp ./test ./mnt/home

cp -r ../root/* ./mnt
cp -r ../base/* ./mnt