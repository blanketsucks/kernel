file(GLOB_RECURSE CPP_SOURCES *.cpp)

# For `std`
add_link_options(-nostdlib++)

create_library(libelf elf usr/include/libelf)