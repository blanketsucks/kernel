set(PROGRAMS true false test audio_test execve)

# FIXME: Remove `-static` when we have proper dynamic linking support
add_link_options(-nostdlib++ -static)
add_compile_options(-fno-exceptions -fno-rtti)

foreach(PROG ${PROGRAMS})
    add_executable(${PROG} ${PROG}.cpp ${STD_CPP_SOURCES})
    add_dependencies(${PROG} libc)

    install(TARGETS ${PROG} DESTINATION bin)
endforeach()