file(GLOB_RECURSE CPP_SOURCES *.cpp)

add_link_options(-nostdlib++ -static)
add_compile_options(-fno-exceptions -fno-rtti -g)

include_directories(${CMAKE_SOURCE_DIR}/programs)

add_executable(shell ${CPP_SOURCES})
add_dependencies(shell libc libgfx)

target_link_libraries(shell gfx)
install(TARGETS shell DESTINATION bin)