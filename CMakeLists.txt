cmake_minimum_required(VERSION 3.22)

project(Kernel)

set(CMAKE_CXX_STANDARD 20)

option(USE_CLANG "Use clang as the compiler" OFF)
option(DEBUG "Enable debug mode" OFF)
option(X86_64 "Build for x86_64" OFF)

if (USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)

    set(CMAKE_CXX_COMPILER_ID "Clang")
else()
    set(CMAKE_CXX_COMPILER i686-elf-g++)
    set(CMAKE_C_COMPILER i686-elf-gcc)

    set(CMAKE_CXX_COMPILER_ID "GNU")
endif()

set(DISK_IMAGE ${CMAKE_SOURCE_DIR}/scripts/disk.img)
set(SCRIPTS ${CMAKE_SOURCE_DIR}/scripts)

add_subdirectory(kernel)

add_custom_target(
    grub-image
    COMMAND sudo ${SCRIPTS}/mkfs.grub.sh
    USES_TERMINAL
)

add_custom_target(
    qemu-image
    COMMAND sudo ${SCRIPTS}/mkfs.qemu.sh
    USES_TERMINAL
)

add_custom_target(
    run 
    COMMAND ${SCRIPTS}/run.sh ${CMAKE_BINARY_DIR}/kernel/kernel.bin ${DISK_IMAGE}
    DEPENDS kernel.bin
    USES_TERMINAL
)