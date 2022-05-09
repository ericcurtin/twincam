#!/bin/bash

set -e

echo "check if libcamera exist" 
if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

build() {
  meson build --buildtype=release --prefix=/usr && ninja -v -C build && sudo ninja -v -C build install
}

cd ..
echo "$(pwd)"
echo "clone libcamera" 
git clone https://git.libcamera.org/libcamera/libcamera.git
cd libcamera
echo "build libcamera"
build
cd ../twincam
echo "$(pwd)"

echo "build twincam"
export PKG_CONFIG_PATH="/usr/lib64/pkgconfig/"
export CC=clang
export CXX=clang++
build

git clean -fdx
export CC=gcc
export CXX=g++
build

