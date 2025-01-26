#!/bin/bash

if [ -z "$SUDO_UID" ]; then
    echo "Please run this script with sudo."
    exit 1
fi

if [ ! -d "limine" ]; then
    git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
    make -C limine
fi

cd "$(dirname "$0")"

DISK_IMAGE="disk.img"
rm -f "$DISK_IMAGE"

rm -rf ./mnt
rm -rf ./esp

qemu-img create "$DISK_IMAGE" 512M
chown "$SUDO_UID:$SUDO_GID" "$DISK_IMAGE"

echo "Disk image created."

dev=$(losetup --find --partscan --show "$DISK_IMAGE")

echo "Loopback device set to '$dev'"

parted -s "${dev}" mklabel gpt mkpart ESP fat16 1MiB 32MiB mkpart OS ext2 32MiB 100% set 1 esp on
echo "Partition table created."

sudo mkfs.fat -F 16 -n "EFI System" ${dev}p1
sudo mkfs.ext2 -q -I 128 -L OS ${dev}p2

mkdir -p esp
mkdir -p mnt

sudo mount "${dev}p1" esp
sudo mount "${dev}p2" mnt

mkdir -p esp/EFI/BOOT

cp ./limine/BOOTX64.EFI esp/EFI/BOOT/BOOTX64.EFI
cp ./limine/BOOTIA32.EFI esp/EFI/BOOT/BOOTIA32.EFI

cp ./limine/limine-bios.sys esp/limine-bios.sys
cp ./limine/limine-bios-cd.bin esp/limine-bios-cd.bin
cp ./limine.cfg esp/limine.cfg
cp ../build/kernel/kernel.bin esp/kernel

./limine/limine bios-install "$dev"

./mkfs.root.sh

sync

umount ./esp
umount ./mnt

losetup -d "$dev"

echo "Done."