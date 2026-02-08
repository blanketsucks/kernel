#!/bin/bash

GCC_VERSION="14.2.0"
GCC_NAME="gcc-${GCC_VERSION}"
GCC_PACKAGE="${GCC_NAME}.tar.xz"
GCC_URL="https://ftpmirror.gnu.org/gnu/gcc"

function build_gcc() {
    echo "Building GCC version $GCC_VERSION..."

    if [ -d "gcc" ]; then
        echo "Removing existing gcc directory..."

        rm -rf gcc
        rm $GCC_PACKAGE
    fi

    mkdir gcc

    curl -LO $GCC_URL/$GCC_NAME/$GCC_PACKAGE
    tar -xf $GCC_PACKAGE --strip-components=1 -C gcc

    cd gcc

    patch -p1 < ../gcc-$GCC_VERSION.patch

    mkdir build
    cd build

    ../configure --prefix="$PREFIX" --target="$TARGET-corn" --with-sysroot="$SYSROOT" --enable-languages=c,c++ --enable-shared --disable-nls --disable-initfini-array
    make -j16 all-gcc all-target-libgcc
    make install-gcc install-target-libgcc

    # TODO: Compile libstdc++
    echo "GCC build completed successfully."

    cd ../..
}
