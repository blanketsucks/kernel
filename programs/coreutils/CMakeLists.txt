set(PROGRAMS true false test)

# FIXME: Remove `-static` when we have proper dynamic linking support
add_link_options(-nostdlib++ -static)

foreach(PROG ${PROGRAMS})
    add_executable(${PROG} ${PROG}.cpp)
    add_dependencies(${PROG} libc)

    install(TARGETS ${PROG} DESTINATION bin)
endforeach()