file(GLOB_RECURSE CPP_SOURCES *.cpp)
list(FILTER CPP_SOURCES EXCLUDE REGEX "crt0\.cpp")

add_compile_options(-nostdlib -Wall -fno-exceptions -fno-rtti)
add_link_options(-nostdlib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

add_library(crt0 STATIC crt0.cpp)
set_target_properties(crt0 PROPERTIES PREFIX "" SUFFIX ".o")

create_library(libc c usr/include)
target_link_directories(libc-shared PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_directories(libc-static PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(libc-static crt0)

install(TARGETS crt0 DESTINATION lib)

# As soon as `libc` is built, we need to install it to be able to build the rest of the libraries
add_custom_command(TARGET libc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libc-shared> ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libc-static> ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:crt0> ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND rsync -au --include '*/' --include='*.h' --exclude='*' ${CMAKE_SOURCE_DIR}/libraries/libc/* ${CMAKE_INSTALL_PREFIX}/usr/include
)