#!/bin/bash

set -ex

if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

if [ "$EUID" -ne 0 ]; then
  prefix="sudo"
fi

build() {
  meson build --buildtype=release --prefix=/usr
  ninja -v -C build
  $prefix ninja -v -C build install
}

mkdir ../libcamera
git clone https://git.libcamera.org/libcamera/libcamera.git ../libcamera
cd ../libcamera
build
cd -

export PKG_CONFIG_PATH="/usr/lib64/pkgconfig/"
export CC=clang
export CXX=clang++
git clean -fdx > /dev/null 2>&1
build

export CC=gcc
export CXX=g++
git clean -fdx > /dev/null 2>&1
build

