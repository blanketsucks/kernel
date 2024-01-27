#!/bin/bash

KERNEL=$1
DISK_IMAGE=$2

QEMU_ARGS=(
    -kernel "$KERNEL"
    -drive format=raw,file="$DISK_IMAGE",id=disk,if=ide
    -serial stdio
    -device ac97
    -cpu max
)

if [ -n "$QEMU_DEBUG" ]; then
    QEMU_ARGS+=(
        -d cpu_reset,int
        -D qemu.log
        -s
    )
fi

if [ -n "$QEMU_PCSPEAKER" ]; then
    QEMU_ARGS+=(
        -audiodev pa,id=speaker
        -machine pcspk-audiodev=speaker
    )
fi

qemu-system-i386 "${QEMU_ARGS[@]}"


# qemu-system-i386 -d cpu_reset -D qemu.log -kernel "$KERNEL" -drive format=raw,file="$DISK_IMAGE",id=disk,if=ide -serial stdio