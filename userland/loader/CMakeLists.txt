file(GLOB_RECURSE CPP_SOURCES *.cpp)

add_compile_options(-static -g)
add_link_options(-static -nostdlib++ -g)

add_executable(dyn-loader.so ${CPP_SOURCES})
add_dependencies(dyn-loader.so libc-static crt0 libelf-static)

# target_link_options(dyn-loader.so PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

install(TARGETS dyn-loader.so DESTINATION lib)