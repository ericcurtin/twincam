#!/bin/bash
  
set -e

if [ -n "$CI" ] ; then
  export DEBIAN_FRONTEND=noninteractive
  sudo apt update || true
  sudo apt install -y clang || true
  sudo apt install -y gnutls-dev || true
  sudo apt install -y libboost-dev || true
  sudo apt install -y meson || true
  sudo apt install -y python3-jinja2 || true
  sudo apt install -y python3-ply || true
  sudo apt install -y python3-yaml || true
  sudo apt install -y cmake || true
  sudo apt install -y pkg-config || true
  sudo apt install -y libevent-dev || true
  sudo apt install -y libdrm-dev || true
  sudo apt install -y gcc || true
  sudo apt install -y make || true
  sudo apt install -y autoconf || true
  sudo apt install -y automake || true
  sudo apt install -y cppcheck || true
  sudo apt install -y libsdl2-dev || true
  sudo apt install -y meson || true
  sudo apt install -y git || true
  sudo apt install -y vim || true
  sudo apt install -y python3-jinja2 || true
  sudo apt install -y python3-ply || true
  sudo apt install -y libgtest-dev || true
else
  echo "skipping building packages"
fi
