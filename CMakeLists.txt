project(Kernel)

cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++)

set(DISK_IMAGE ${CMAKE_SOURCE_DIR}/scripts/fs.img)
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