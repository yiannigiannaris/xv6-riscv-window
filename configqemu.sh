#!/bin/bash

git clone https://github.com/qemu/qemu
cd qemu/
git fetch
git checkout stable-4.1
git apply ../sdl.patch
