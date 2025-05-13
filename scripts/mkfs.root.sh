mkdir -p ./mnt/dev
mkdir -p ./mnt/dev/input
mkdir -p ./mnt/dev/pts

# TODO: Automatically create these devices when the kernel boots
mknod ./mnt/dev/input/mouse c 5 0
mknod ./mnt/dev/input/keyboard c 5 1

mknod ./mnt/dev/null c 1 1
mknod ./mnt/dev/zero c 1 2

mknod ./mnt/dev/ptmx c 99 0

mknod ./mnt/dev/snd0 c 6 0
mknod ./mnt/dev/fb0 b 7 0

mkdir -p ./mnt/boot
cp ./kernel.map ./mnt/boot

mkdir ./mnt/home
cp ./test.txt ./mnt/home/test.txt

cp ./test ./mnt/home

cp -r ../root/* ./mnt
cp -r ../base/* ./mnt