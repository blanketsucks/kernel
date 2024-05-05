file(GLOB_RECURSE CPP_SOURCES *.cpp ${CMAKE_SOURCE_DIR}/std/*.cpp)

link_directories(${CMAKE_SOURCE_DIR}/include/lai)

if (X86_64)
    list(FILTER CPP_SOURCES EXCLUDE REGEX ".*arch/x86/.*")

    add_compile_definitions(__x86_64__)
    add_compile_options(-fno-stack-protector -fno-stack-check -fno-lto -fPIE -mno-80387 -mno-sse -mno-sse2 -mno-red-zone -mno-mmx)

    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/linker.ld)

    set(TARGET_TRIPLE x86_64-pc-none-elf)
    set(BITS 64)

    file(GLOB_RECURSE ASM_SOURCES arch/x86_64/asm/*.asm)
    set(CMAKE_ASM_NASM_FLAGS "-g -felf64 -I${CMAKE_SOURCE_DIR}/")
else()
    list(FILTER CPP_SOURCES EXCLUDE REGEX ".*arch/x86_64/.*")

    add_compile_definitions(__x86__)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/arch/x86/linker.ld)

    set(TARGET_TRIPLE i686-pc-none-elf)
    set(BITS 32)

    file(GLOB_RECURSE ASM_SOURCES arch/x86/asm/*.asm)
    set(CMAKE_ASM_NASM_FLAGS "-g -felf32 -I${CMAKE_SOURCE_DIR}/")

    link_libraries(lai32)
endif()

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> ${CMAKE_ASM_NASM_FLAGS} -o <OBJECT> <SOURCE>")

enable_language(ASM_NASM)
set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(kernel.bin ${CPP_SOURCES} ${ASM_SOURCES})
target_include_directories(kernel.bin SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(kernel.bin PRIVATE __KERNEL__)
target_compile_options(
    kernel.bin PRIVATE -g
    -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti
)

target_link_options(
    kernel.bin PRIVATE
    -T ${LINKER_SCRIPT} -nostdlib -ffreestanding -fno-exceptions -fno-rtti -g
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kernel.bin PRIVATE --target=${TARGET_TRIPLE} -m${BITS} -Wno-interrupt-service-routine)
    target_link_options(kernel.bin PRIVATE --target=${TARGET_TRIPLE} -m${BITS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(kernel.bin PRIVATE -mgeneral-regs-only)
    target_link_libraries(kernel.bin PRIVATE gcc)
endif()