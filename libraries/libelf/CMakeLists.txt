file(GLOB_RECURSE CPP_SOURCES *.cpp)

add_link_options(-nostdlib++)
add_compile_options(-fno-exceptions)

create_library(libelf elf usr/include/libelf)