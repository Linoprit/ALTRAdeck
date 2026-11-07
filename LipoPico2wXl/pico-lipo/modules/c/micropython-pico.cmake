# This top-level micropython.cmake is responsible for listing
# the individual modules we want to include.
# Paths are absolute, and ${CMAKE_CURRENT_LIST_DIR} can be
# used to prefix subdirectories.


include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/powman/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/wakeup/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cppmem/micropython.cmake)

# include(${CMAKE_CURRENT_LIST_DIR}/lcd_tft/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ili934x_tft/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ALTRAgraphix/micropython.cmake)
