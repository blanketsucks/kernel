set(CPP_SOURCES math.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_compile_options(-nostdlib -Wall -fno-exceptions -fno-rtti)
add_link_options(-nostdlib)

create_library(libm m usr/include)