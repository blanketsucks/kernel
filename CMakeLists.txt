cmake_minimum_required(VERSION 3.22)

project(Kernel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/root)
set(CMAKE_INSTALL_MESSAGE LAZY)

option(USE_CLANG "Use clang as the compiler" OFF)
option(DEBUG "Enable debug mode" OFF)
option(x86_64 "Build for x86_64" OFF)

if (USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)

    set(CMAKE_CXX_COMPILER_ID "Clang")
else()
    set(CMAKE_CXX_COMPILER i686-chair-g++)
    set(CMAKE_C_COMPILER i686-chair-gcc)

    add_link_options(-z noexecstack)
    set(CMAKE_CXX_COMPILER_ID "GNU")
endif()

if (x86_64)
    set(CMAKE_ASM_NASM_FLAGS "-g -felf64 -I${CMAKE_SOURCE_DIR}/")
else()
    set(CMAKE_ASM_NASM_FLAGS "-g -felf32 -I${CMAKE_SOURCE_DIR}/")
endif()

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> ${CMAKE_ASM_NASM_FLAGS} -o <OBJECT> <SOURCE>")

enable_language(ASM_NASM)

file(GLOB_RECURSE STD_CPP_SOURCES ${CMAKE_SOURCE_DIR}/std/*.cpp)

set(DISK_IMAGE ${CMAKE_SOURCE_DIR}/scripts/disk.img)
set(SCRIPTS ${CMAKE_SOURCE_DIR}/scripts)

function(create_library libname output dst)
    add_library(${libname}-static STATIC ${CPP_SOURCES})
    set_target_properties(${libname}-static PROPERTIES OUTPUT_NAME ${output})

    add_library(${libname}-shared SHARED ${CPP_SOURCES})
    set_target_properties(${libname}-shared PROPERTIES OUTPUT_NAME ${output})

    add_custom_target(${libname})
    add_dependencies(${libname} ${libname}-static ${libname}-shared)

    install(TARGETS ${libname}-static DESTINATION lib)
    install(TARGETS ${libname}-shared DESTINATION lib)

    # To ensure that all the programs are built with the latest build of the library
    add_custom_command(TARGET ${libname} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${libname}-static> ${CMAKE_INSTALL_PREFIX}/lib
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${libname}-shared> ${CMAKE_INSTALL_PREFIX}/lib
    )

    if (dst)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${dst} FILES_MATCHING PATTERN "*.h")
    endif()
endfunction()

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libraries ${CMAKE_SOURCE_DIR}/libraries/libc)

add_subdirectory(kernel)
add_subdirectory(libraries)
add_subdirectory(userland)

add_custom_target(
    grub-image
    COMMAND sudo ${SCRIPTS}/mkfs.grub.sh
    USES_TERMINAL
)

add_custom_target(
    image
    COMMAND sudo ${SCRIPTS}/mkfs.qemu.sh
    USES_TERMINAL
)

add_custom_target(
    run
    COMMAND python3 ${SCRIPTS}/run.py
    DEPENDS kernel.bin
    USES_TERMINAL
)