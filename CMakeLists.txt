# this entire file is cargo culted from the pico-sdk readme
cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{HOME}/Downloads/pico-sdk/pico_sdk_init.cmake)

project(blink)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(blink
    blink.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(blink pico_stdlib)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(blink)
