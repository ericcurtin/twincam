#!/bin/bash

set -e

echo "check if libcamera exist" 
if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

cd ..
echo "$(pwd)"
echo "clone libcamera" 
git clone https://git.libcamera.org/libcamera/libcamera.git
cd libcamera
echo "build libcamera" 
meson build --buildtype=release --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
cd ../twincam
echo "$(pwd)"

echo "build twincam"
export PKG_CONFIG_PATH="/usr/lib64/pkgconfig/"
meson build --buildtype=release --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
