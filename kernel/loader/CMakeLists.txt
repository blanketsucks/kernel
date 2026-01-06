file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE ASM_SOURCES *.asm)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(loader.bin ${SOURCES} ${ASM_SOURCES} ${CMAKE_SOURCE_DIR}/std/cstring.cpp)
set_target_properties(loader.bin PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

target_include_directories(loader.bin PRIVATE ${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/include)

target_compile_options(
    loader.bin PRIVATE
    -no-pie -fno-pic -nostdlib -nostdinc++ -ffreestanding -fno-exceptions -fno-rtti
    -g
)

target_compile_definitions(loader.bin PRIVATE __KERNEL_LOADER__)

target_link_options(loader.bin PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib -no-pie -fno-pic
)

add_custom_command(
    TARGET loader.bin POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O elf32-i386 $<TARGET_FILE:loader.bin> $<TARGET_FILE:loader.bin>
)