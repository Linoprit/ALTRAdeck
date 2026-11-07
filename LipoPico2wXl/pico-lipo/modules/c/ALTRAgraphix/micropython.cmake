set($LIBNAME altra_graphix)
add_library(usermod_${LIBNAME} INTERFACE)

target_sources(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ALTRAgraphix.c
    ${CMAKE_CURRENT_LIST_DIR}/fxbuf.c
)

target_include_directories(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_${LIBNAME})

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/ALTRAgraphix.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-discarded-qualifiers"
)
