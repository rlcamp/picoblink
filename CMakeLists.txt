# invoke using:
# mkdir -p build && cd build && PICO_SDK_PATH=~/Downloads/pico-sdk/ PICO_TOOLCHAIN_PATH=~/Downloads/arm-gnu-toolchain-12.2.mpacbti-rel1-darwin-arm64-arm-none-eabi/bin cmake .. -DPICO_BOARD=adafruit_feather_rp2040 && cd .. && make -C build -j4 VERBOSE=1

cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{HOME}/Downloads/pico-sdk/pico_sdk_init.cmake)

# use directory name as project name
get_filename_component(project ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${project})

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# assume there is a single .c file with the same name as the directory
add_executable(${project}
    ${project}.c
)

target_link_libraries(${project} pico_stdlib cmsis_core)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_uf2_output(${project})
