set($LIBNAME lcd_tft)
add_library(usermod_${LIBNAME} INTERFACE)

target_sources(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lcd_tft.c
    ${CMAKE_CURRENT_LIST_DIR}/lcd_tft.cpp
)

target_include_directories(usermod_${LIBNAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_${LIBNAME})

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/lcd_tft.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-discarded-qualifiers"
)
