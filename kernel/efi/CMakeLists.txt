file(GLOB SOURCES *.cpp)

set(CMAKE_CXX_LINK_EXECUTABLE "ld <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_executable(
    efi-loader
    ${SOURCES}
    ${CMAKE_SOURCE_DIR}/std/cstring.cpp
)

set_target_properties(efi-loader PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/elf_x86_64_efi.ld)
target_include_directories(efi-loader PRIVATE ${CMAKE_SOURCE_DIR}/)

target_compile_options(
    efi-loader PRIVATE
    -fpic -fPIC -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -mno-red-zone
    -D__KERNEL_LOADER__ -g
)

target_link_options(
    efi-loader PRIVATE
    -nostdlib -shared -Bsymbolic -T ${CMAKE_CURRENT_SOURCE_DIR}/elf_x86_64_efi.ld
)

add_custom_command(
    TARGET efi-loader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 $<TARGET_FILE:efi-loader> ${CMAKE_CURRENT_BINARY_DIR}/loader.efi
)