# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "/YOUR_PICO_SDK_PATH/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-pwm-audio C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Pico SDK
pico_sdk_init()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

# Add executable. Default name is the project name, version 0.1
add_executable(pico-pwm-audio )

pico_generate_pio_header(pico-pwm-audio  ${CMAKE_CURRENT_LIST_DIR}/servo_bb.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated PRIVATE)
pico_generate_pio_header(pico-pwm-audio  ${CMAKE_CURRENT_LIST_DIR}/audio_bb.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated PRIVATE)

target_sources(pico-pwm-audio 
PRIVATE
pico-pwm-audio.c
pico-pwm-audio.h
config.h
control.c
control.h
cmdqueue.c
cmdqueue.h
# include generated audio snippet files here
audio-data/rogerroger.cpp
)

pico_set_program_name(pico-pwm-audio "pico-pwm-audio")
pico_set_program_version(pico-pwm-audio "0.1")

pico_enable_stdio_uart(pico-pwm-audio 0)
pico_enable_stdio_usb(pico-pwm-audio 1)

# Add the standard library to the build
target_link_libraries(pico-pwm-audio pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico-pwm-audio
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_pwm
        pico_multicore
        pico_rand
        pico_stdlib
        pico_mem_ops
        )

pico_add_extra_outputs(pico-pwm-audio)

