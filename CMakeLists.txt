# invoke using: mkdir -p build && cd build && PICO_SDK_PATH=~/Downloads/pico-sdk/ PICO_TOOLCHAIN_PATH=~/Downloads/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin cmake .. -DPICO_BOARD=pico2 && cd ..

cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(my_project)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(blink
    blink.c
)

# pull in common dependencies
target_link_libraries(blink pico_stdlib cmsis_core)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(blink)
