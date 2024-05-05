#!/bin/bash

KERNEL=$1
DISK_IMAGE=$2

QEMU_ARGS=(
    -kernel "$KERNEL"
    -drive format=raw,file="$DISK_IMAGE",id=disk,if=ide
    -serial stdio
    -audiodev sdl,id=speaker
    -device ac97,audiodev=speaker
    -D qemu.log
    -d cpu_reset,int
    -no-reboot
)

if [ -n "$QEMU_DEBUG" ]; then
    QEMU_ARGS+=( -s -S )
fi

if [ -n "$QEMU_AUDIO" ]; then
    QEMU_ARGS+=(
        -audiodev sdl,id=speaker
        -machine pcspk-audiodev=speaker
        -device ac97,audiodev=speaker
    )
fi

qemu-system-i386 "${QEMU_ARGS[@]}"
