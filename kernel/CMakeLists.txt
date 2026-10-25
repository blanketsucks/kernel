file(GLOB_RECURSE CPP_SOURCES *.cpp)

list(FILTER CPP_SOURCES EXCLUDE REGEX ".*arch/x86/.*")
file(GLOB_RECURSE ASM_SOURCES arch/x86_64/asm/*.asm)

add_subdirectory(efi)

add_compile_definitions(__x86_64__)
add_compile_options(
    -fno-stack-protector -fno-stack-check
    -fno-lto -fno-pic
    -mno-red-zone
    -mcmodel=kernel
    -m64
    -march=x86-64
    -mno-80387
    -mno-mmx
    -mno-sse
    -mno-sse2
)

add_link_options(
    -zmax-page-size=0x1000
    -z noexecstack
    -Wl,-m,elf_x86_64
    -Wl,--build-id=none
    -static
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/linker.ld)

add_subdirectory(loader)

list(FILTER CPP_SOURCES EXCLUDE REGEX ".*loader/.*")
list(FILTER CPP_SOURCES EXCLUDE REGEX ".*efi/.*")

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(kernel.bin ${CPP_SOURCES} ${ASM_SOURCES} ${STD_CPP_SOURCES})
target_include_directories(kernel.bin SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(kernel.bin PRIVATE __KERNEL__)
target_compile_options(
    kernel.bin
    PRIVATE
    -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti -g
    -Wno-unknown-warning-option -Wnull-dereference -Wunused-result
)

target_link_options(
    kernel.bin
    PRIVATE
    -T ${LINKER_SCRIPT} -nostdlib -ffreestanding -fno-exceptions -fno-rtti -g
)

set_target_properties(kernel.bin PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries(kernel.bin PRIVATE gcc)

# Install kernel headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/kernel DESTINATION usr/include FILES_MATCHING PATTERN "*.h")