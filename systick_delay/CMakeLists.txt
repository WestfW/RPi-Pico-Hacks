cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test)
pico_sdk_init()
add_executable(test
    systick_delay.c
)

pico_enable_stdio_usb(test 1)
pico_enable_stdio_uart(test 0)

pico_add_extra_outputs(test)
#pico_set_binary_type(test no_flash)
#set (pico_bare_metal 1)
target_link_libraries(test pico_stdlib)
#target_link_libraries(test)
