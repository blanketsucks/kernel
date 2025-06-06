#!/bin/bash

BINUTILS_VERSION="2.44"
BRANCH_NAME="binutils-${BINUTILS_VERSION//./_}"

BINUTILS_GIT_URL="https://sourceware.org/git/binutils-gdb.git"

function build_binutils() {
    echo "Building Binutils version $BINUTILS_VERSION..."

    if [ -d "binutils-gdb" ]; then
        echo "Removing existing binutils-gdb directory..."
        rm -rf binutils-gdb
    fi

    git clone --depth 1 --branch $BRANCH_NAME $BINUTILS_GIT_URL
    cd binutils-gdb

    git checkout $BRANCH_NAME
    git apply ../binutils-$BINUTILS_VERSION.patch

    mkdir build
    cd build

    ../configure --prefix="$PREFIX" --target="$TARGET-corn" --with-sysroot="$SYSROOT" --disable-werror --disable-nls

    make -j8
    make install

    echo "Binutils build completed successfully."
    cd ../..
}
