#!/bin/bash

GCC_VERSION="14.2.0"
GCC_GIT_URL="https://gcc.gnu.org/git/gcc.git"

function build_gcc() {
    echo "Building GCC version $GCC_VERSION..."

    if [ -d "gcc" ]; then
        echo "Removing existing gcc directory..."
        rm -rf gcc
    fi 

    git clone --depth 1 --branch releases/gcc-$GCC_VERSION $GCC_GIT_URL
    cd gcc

    git checkout releases/gcc-$GCC_VERSION
    git apply ../gcc-$GCC_VERSION.patch

    mkdir build
    cd build

    ../configure --prefix="$PREFIX" --target="$TARGET-corn" --with-sysroot="$SYSROOT" --enable-languages=c,c++ --enable-shared --disable-nls

    make -j8 all-gcc all-target-libgcc
    make install-gcc install-target-libgcc

    # TODO: Compile libstdc++
    echo "GCC build completed successfully."

    cd ../..
}
