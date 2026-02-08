#!/bin/bash

BINUTILS_VERSION="2.44"
BINUTILS_NAME="binutils-${BINUTILS_VERSION}"
BINUTILS_PACKAGE="${BINUTILS_NAME}.tar.xz"
BINUTILS_URL="https://ftpmirror.gnu.org/gnu/binutils"

function build_binutils() {
    echo "Building Binutils version $BINUTILS_VERSION..."

    if [ -d "binutils-gdb" ]; then
        echo "Removing existing binutils-gdb directory..."

        rm -rf binutils-gdb
        rm $BINUTILS_PACKAGE
    fi

    mkdir binutils-gdb

    curl -LO $BINUTILS_URL/$BINUTILS_PACKAGE
    tar -xf $BINUTILS_PACKAGE --strip-components=1 -C binutils-gdb

    cd binutils-gdb

    patch -p1 < ../binutils-$BINUTILS_VERSION.patch

    mkdir build
    cd build

    ../configure --prefix="$PREFIX" --target="$TARGET-corn" --with-sysroot="$SYSROOT" --disable-werror --disable-nls --disable-initfini-array

    make -j16
    make install

    echo "Binutils build completed successfully."
    cd ../..
}
