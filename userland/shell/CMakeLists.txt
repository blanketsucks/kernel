file(GLOB_RECURSE CPP_SOURCES *.cpp)

add_executable(shell ${CPP_SOURCES} ${STD_CPP_SOURCES})

target_include_directories(shell PRIVATE ${CMAKE_SOURCE_DIR}/userland)
target_compile_options(shell PRIVATE -Wall -Wextra -g -fno-exceptions -fno-rtti -static)

target_link_options(shell PRIVATE -nostdlib++ -g -static)

add_dependencies(shell libc libgfx)
target_link_libraries(shell gfx)

install(TARGETS shell DESTINATION bin)