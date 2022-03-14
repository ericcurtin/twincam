#!/bin/bash
  
set -e

if [ -n "$CI" ] ; then
  sudo apt update
  sudo apt install -y clang gnutls-dev libboost-dev meson python3-jinja2 \
    python3-ply python3-yaml cmake pkg-config libevent-dev libdrm-dev gcc make \
    autoconf automake cppcheck
else
  echo "skipping building packages"
fi
