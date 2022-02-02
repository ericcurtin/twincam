#!/bin/bash

set -e

if [ -n "$CI" ] ; then
  sudo apt install -y clang
  sudo apt install -y gnutls-dev
  sudo apt install -y libboost-dev
  sudo apt install -y meson
  sudo apt install -y python3-pip
  python3 -m pip install jinja2
  python3 -m pip install ply
  sudo apt install -y cmake
  sudo apt install -y pkg-config
  sudo apt install -y libevent-dev
  sudo apt install -y libdrm-dev
fi

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
