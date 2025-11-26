file(GLOB_RECURSE CPP_SOURCES *.cpp)

set(LOADER_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

add_compile_options(-static -g -fno-exceptions)
add_link_options(-static -nostdlib++ -g -T ${LOADER_LINKER_SCRIPT})

add_executable(dyn-loader.so ${CPP_SOURCES})

target_include_directories(dyn-loader.so PRIVATE ${CMAKE_SOURCE_DIR}/userland)

add_dependencies(dyn-loader.so libc-static crt0 libelf-static)
target_link_libraries(dyn-loader.so libelf-static)

install(TARGETS dyn-loader.so DESTINATION lib)