#!/bin/bash

git clone https://github.com/qemu/qemu
cd qemu/
git fetch
git checkout stable-4.1
git apply ../sdl.patch
./configure --target-list=riscv64-softmmu --disable-bsd-user --disable-guest-agent --enable-libssh --enable-vde --disable-cocoa --enable-sdl --disable-gtk && make
