cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE "toolchain.cmake")
project("tb-display")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#function(ihx_to_hex bin)
#    add_custom_command(TARGET ${bin} POST_BUILD COMMAND  packihx ${bin}.ihx > ${bin}.hex)
#endfunction(ihx_to_hex)

include_directories("/opt/sdcc/STM8_headers/include/  ")

add_executable(tb-display)
target_sources(tb-display PRIVATE main.c) # stm8_interrupt_vector.c)
#add_subdirectory("ledBlink")
#add_subdirectory("ledBlinkFreeRTOS") 
#add_subdirectory("spi")