file(GLOB_RECURSE CPP_SOURCES *.cpp ${CMAKE_SOURCE_DIR}/std/*.cpp)
file(GLOB_RECURSE ASM_SOURCES boot/*.asm)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/boot/linker.ld)

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_FLAGS "-felf32")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> ${CMAKE_ASM_NASM_FLAGS} -o <OBJECT> <SOURCE>")

enable_language(ASM_NASM)
set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(kernel.bin ${CPP_SOURCES} ${ASM_SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/ SYSTEM)

target_compile_definitions(kernel.bin PRIVATE __KERNEL__)

target_compile_options(
    kernel.bin PRIVATE
    -Wall -Wextra -Wno-interrupt-service-routine
    -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti
    --target=i386-pc-none-elf -m32
)

target_link_options(
    kernel.bin PRIVATE
    -T ${LINKER_SCRIPT} --target=i386-pc-none-elf -m32
    -nostdlib -ffreestanding -fno-exceptions -fno-rtti
)