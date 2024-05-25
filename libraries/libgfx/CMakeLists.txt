file(GLOB_RECURSE CPP_SOURCES *.cpp)
list(APPEND CPP_SOURCES ${STD_CPP_SOURCES})

add_link_options(-nostdlib++ -fno-exceptions -fno-rtti)
add_compile_options(-fno-exceptions -fno-rtti)

create_library(libgfx gfx usr/include/libgfx)