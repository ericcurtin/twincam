#!/bin/bash

set -e

echo "check if libcamera exist" 
if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

cd ..
echo "$(pwd)"
echo "clone libcamera" 
git clone https://github.com/kbingham/libcamera.git
cd libcamera
echo "build libcamera" 
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
cd ../twincam
echo "$(pwd)"

echo "build twincam" 
meson build --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
