#!/bin/bash
  
set -e

if [ -n "$CI" ] ; then
# required for build 
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
# required for cppcheck
  sudo apt install -y gcc
  sudo apt install -y make
  sudo apt install -y autoconf
  sudo apt install -y automake
  sudo apt install -y cppcheck
else
  echo "skipping building packages"
fi
