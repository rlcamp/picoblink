# invoke using: mkdir -p build && cd build && cmake .. -DPICO_BOARD=pico2 -DPICO_SDK_PATH=${HOME}/Downloads/pico-sdk/ -DPICO_TOOLCHAIN_PATH=/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/ && cd ..

cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# note CXX is only necessary for new_delete.cpp in pico_runtime
project(my_project C ASM CXX)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

set(SOURCES
    blink.c
)

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wshadow;-Wdouble-promotion")

# rest of your project
add_executable(blink ${SOURCES})

# pull in common dependencies
target_link_libraries(blink pico_stdlib hardware_xosc cmsis_core hardware_powman)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(blink)
