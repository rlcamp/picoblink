#!/bin/bash
# this script works on arm64 macos ventura, as of 20230223, provided xz has been compiled from source

cd ~/Downloads/

if [ ! -d ~/Downloads/arm-gnu-toolchain-*-arm-none-eabi ]; then
    if uname -sm | grep 'Linux x86_64' ; then
        curl -LO https://developer.arm.com/-/media/Files/downloads/gnu/12.2.mpacbti-rel1/binrel/arm-gnu-toolchain-12.2.mpacbti-rel1-x86_64-arm-none-eabi.tar.xz
        tar Jxf arm-gnu-toolchain-12.2.mpacbti-rel1-x86_64-arm-none-eabi.tar.xz
    else
        curl -LO https://developer.arm.com/-/media/Files/downloads/gnu/12.2.mpacbti-rel1/binrel/arm-gnu-toolchain-12.2.mpacbti-rel1-darwin-arm64-arm-none-eabi.tar.xz
        tar Jxf arm-gnu-toolchain-12.2.mpacbti-rel1-darwin-arm64-arm-none-eabi.tar.xz
        cd arm-gnu-toolchain-12.2.mpacbti-rel1-darwin-arm64-arm-none-eabi
        find . -perm +111 -type f | while read k; do cat $k > .tmp && chmod +x .tmp && mv .tmp $k; done
        cd ..
    fi
fi

git clone --depth 1 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
cd ..
