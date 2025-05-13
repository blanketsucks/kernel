#!/bin/bash

if [ -z "$SUDO_UID" ]; then
    echo "Please run this script with sudo."
    exit 1
fi

cd "$(dirname "$0")"

DISK_IMAGE="disk.img"

rm -f "$IMAGE_NAME"
rm -rf ./mnt

qemu-img create "$DISK_IMAGE" 512M
chown "$SUDO_UID:$SUDO_GID" "$DISK_IMAGE"

echo "Disk image created."

mke2fs -q -I 128 "${DISK_IMAGE}"

echo "ext2 filesystem created."

mkdir -p ./mnt
mount "${DISK_IMAGE}" ./mnt

./mkfs.root.sh

sync
umount ./mnt

echo "Done."