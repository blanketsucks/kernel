#!/bin/bash

# Assume we are running in the kernel/toolchain directory
CWD=$(pwd)

mkdir -p $CWD/../root/usr/include
mkdir -p $CWD/../root/lib

SYSROOT="$CWD/../root"
LIBC="$CWD/../libraries/libc"
KERNEL="$CWD/../kernel"
PREFIX="$HOME/opt/cross"
PATH="$PREFIX/bin:$PATH"

TARGET="x86_64"

rsync -avm --include='*.h' --include='*/' --exclude='*' $LIBC/ $SYSROOT/usr/include
rsync -avm --include='*.h' --include='*/' --exclude='*' $KERNEL $SYSROOT/usr/include

source "build-gcc.sh"
source "build-binutils.sh"

build_binutils
build_gcc

echo "Toolchain build completed successfully."
echo "You can now compile the kernel."