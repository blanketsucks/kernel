#!/bin/bash

if [ -z "$SUDO_UID" ]; then
    echo "Please run this script with sudo."
    exit 1
fi

cd "$(dirname "$0")"

DISK_IMAGE="disk.img"

rm -f "$DISK_IMAGE"
rm -rf ./mnt

qemu-img create "$DISK_IMAGE" 512M
chown "$SUDO_UID:$SUDO_GID" "$DISK_IMAGE"

echo "Disk image created."

dev=$(losetup --find --partscan --show "$DISK_IMAGE")
part="p1"

echo "Loopback device set to '$dev'"

parted -s "${dev}" mklabel msdos mkpart primary ext2 32k 100% -a minimal set 1 boot on
echo "Partition created."

mke2fs -q -I 128 "${dev}${part}" 

echo "ext2 filesystem created."

mkdir -p ./mnt
mount "${dev}${part}" ./mnt

mkdir -p ./mnt/usr
mkdir -p ./mnt/boot

grub-install --boot-directory=mnt/boot --target=i386-pc --modules="ext2 part_msdos" "${dev}"
cp grub.cfg ./mnt/boot/grub

cp ./kernel.map ./mnt/boot
cp ../build/kernel/kernel.bin ./mnt/boot/kernel

./mkfs.root.sh

sync
umount ./mnt

losetup -d "$dev"
echo "Done."