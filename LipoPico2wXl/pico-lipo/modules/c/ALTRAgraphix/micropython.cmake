set(LIBNAME altra_graphix)
add_library(usermod_${LIBNAME} INTERFACE)

target_sources(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ALTRAgraphix.c
    ${CMAKE_CURRENT_LIST_DIR}/canvas.c
    ${CMAKE_CURRENT_LIST_DIR}/fxbuf.c
    ${CMAKE_CURRENT_LIST_DIR}/fxbufset.c
    ${CMAKE_CURRENT_LIST_DIR}/sprite.c
    ${CMAKE_CURRENT_LIST_DIR}/types.c
    ${CMAKE_CURRENT_LIST_DIR}/cpp/fxbuf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cpp/fxbufset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cpp/canvas.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cpp/sprite.cpp
)

target_include_directories(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(usermod_${LIBNAME} INTERFACE
    MICROPY_MODULE_BUILTIN_SUBPACKAGES=1
)

target_link_libraries(usermod INTERFACE usermod_${LIBNAME})

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/ALTRAgraphix.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-discarded-qualifiers"
)
