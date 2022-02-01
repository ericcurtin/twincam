#!/bin/bash

set -e


git clone https://github.com/kbingham/libcamera.git
cd libcamera
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
cd -
git clone https://github.com/ericcurtin/twincam
cd twincam
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
