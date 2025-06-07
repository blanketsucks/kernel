file(GLOB_RECURSE CPP_SOURCES *.cpp)

add_compile_options(-static -g)
add_link_options(-static -nostdlib++ -g)

add_executable(dyn-loader.so ${CPP_SOURCES})
add_dependencies(dyn-loader.so libc-static crt0 libelf-static)

install(TARGETS dyn-loader.so DESTINATION lib)