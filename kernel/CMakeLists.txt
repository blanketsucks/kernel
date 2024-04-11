file(GLOB_RECURSE CPP_SOURCES *.cpp ${CMAKE_SOURCE_DIR}/std/*.cpp)
file(GLOB_RECURSE ASM_SOURCES boot/*.asm)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/boot/linker.ld)

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_FLAGS "-g -felf32 -I${CMAKE_SOURCE_DIR}/")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> ${CMAKE_ASM_NASM_FLAGS} -o <OBJECT> <SOURCE>")

enable_language(ASM_NASM)
set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_custom_target(
    symbols
    # COMMAND ${CMAKE_OBJCOPY} -O elf32-i386 -I elf32-i386 --only-keep-debug kernel.bin kernel.sym
    # COMMAND ${CMAKE_OBJCOPY} -O elf32-i386 -I elf32-i386 --strip-debug kernel.bin
    COMMAND objdump -S -D -C -M i386 -M intel kernel.bin > kernel.dump
    DEPENDS kernel.bin
)

add_executable(kernel.bin ${CPP_SOURCES} ${ASM_SOURCES})
target_include_directories(kernel.bin SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(kernel.bin PRIVATE __KERNEL__)
target_compile_options(
    kernel.bin PRIVATE -g
    -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti
)

target_link_directories(kernel.bin PRIVATE ${CMAKE_SOURCE_DIR}/include/lai)
target_link_libraries(kernel.bin PRIVATE lai)

target_link_options(
    kernel.bin PRIVATE
    -T ${LINKER_SCRIPT} -nostdlib -ffreestanding -fno-exceptions -fno-rtti -g
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
        kernel.bin PRIVATE 
        --target=i686-pc-none-elf -m32 -Wno-interrupt-service-routine
    )

    target_link_options(kernel.bin PRIVATE --target=i686-pc-none-elf -m32)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(kernel.bin PRIVATE -mgeneral-regs-only)
    target_link_libraries(kernel.bin PRIVATE gcc)
endif()