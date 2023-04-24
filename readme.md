## Instructions

These assume a vanilla ARM64 Mac running Ventura, with Xcode or at least its command line tools, without Homebrew.

### Obtain prerequisites

- Build and install xz from source (macOS doesn't come with it, but macOS tar WILL use it to extract .tar.xz if present)
- Build and install cmake from source
- Download a prebuilt gcc-arm-none-eabi from arm (also satisfies newlib dep) and extract it in ~/Downloads/
- git clone pico-sdk, also in ~/Downloads/
- run `git submodule update --init` in the resulting pico-sdk directory

### Build this code

- `mkdir build` in this directory
- `cd build && PICO_TOOLCHAIN_PATH=~/Downloads/arm-gnu-toolchain-12.2.mpacbti-rel1-darwin-arm64-arm-none-eabi/bin cmake .. && cd ..`
- `make -C build -j4`

### Upload and run this code

- Hold down BOOTSEL and press RESET
- `cp blink.uf2 /Volumes/RPI-RP2`
