mkdir -p ./mnt/dev
mkdir -p ./mnt/dev/input

mknod ./mnt/dev/input/mouse c 13 0
mknod ./mnt/dev/input/keyboard c 13 1

mknod ./mnt/dev/zero c 1 5
mknod ./mnt/dev/null c 1 3

mkdir -p ./mnt/boot
cp ./kernel.map ./mnt/boot

mkdir ./mnt/home
touch ./mnt/home/test.txt