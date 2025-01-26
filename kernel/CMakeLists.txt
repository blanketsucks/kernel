file(GLOB_RECURSE CPP_SOURCES *.cpp)

if (x86_64)
    list(FILTER CPP_SOURCES EXCLUDE REGEX ".*arch/x86/.*")
    file(GLOB_RECURSE ASM_SOURCES arch/x86_64/asm/*.asm)

    set(CMAKE_CXX_COMPILER g++)

    add_compile_definitions(__x86_64__)
    add_compile_options(
        -fno-stack-protector -fno-stack-check
        -fno-lto -fno-pic
        -mno-red-zone
        -mcmodel=kernel
        -m64
        -march=x86-64
        -mno-sse
        -mno-sse2
        -mno-mmx
    )

    add_link_options(
        -zmax-page-size=0x1000
        -znoexecstack
        -Wl,-m,elf_x86_64
        -Wl,--build-id=none
        -static
    )

    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/linker.ld)

    set(TARGET_TRIPLE x86_64-pc-none-elf)
    set(BITS 64)
else()
    list(FILTER CPP_SOURCES EXCLUDE REGEX ".*arch/x86_64/.*")
    file(GLOB_RECURSE ASM_SOURCES arch/x86/asm/*.asm)

    add_compile_definitions(__x86__)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/arch/x86/linker.ld)

    set(TARGET_TRIPLE i686-pc-none-elf)
    set(BITS 32)
endif()

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(kernel.bin ${CPP_SOURCES} ${ASM_SOURCES} ${STD_CPP_SOURCES})
target_include_directories(kernel.bin SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(kernel.bin PRIVATE __KERNEL__)
target_compile_options(
    kernel.bin
    PRIVATE
    -g -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti
)

target_link_options(
    kernel.bin
    PRIVATE
    -T ${LINKER_SCRIPT} -nostdlib -ffreestanding -fno-exceptions -fno-rtti -g
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kernel.bin PRIVATE --target=${TARGET_TRIPLE} -m${BITS} -Wno-interrupt-service-routine)
    target_link_options(kernel.bin PRIVATE --target=${TARGET_TRIPLE} -m${BITS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # target_compile_options(kernel.bin PRIVATE -mgeneral-regs-only)
    target_link_libraries(kernel.bin PRIVATE gcc)
endif()

# Install kernel headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/kernel DESTINATION usr/include FILES_MATCHING PATTERN "*.h")