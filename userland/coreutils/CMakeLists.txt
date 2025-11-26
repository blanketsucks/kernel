set(PROGRAMS ls true false test audio_test execve)

# FIXME: Remove `-static` when we have proper dynamic linking support
add_link_options(-nostdlib++ -g)
add_compile_options(-fno-exceptions -fno-rtti -g)

foreach(PROG ${PROGRAMS})
    add_executable(${PROG} ${PROG}.cpp ${STD_CPP_SOURCES})
    add_dependencies(${PROG} libc)

    install(TARGETS ${PROG} DESTINATION bin)
endforeach()