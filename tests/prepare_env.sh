#!/bin/bash
  
set -e

if [ "$EUID" -ne 0 ]; then
  prefix="sudo"
fi

export DEBIAN_FRONTEND=noninteractive
$prefix apt update || true
$prefix apt install -y git || true
git fetch --unshallow &
$prefix apt install -y clang || true
$prefix apt install -y gnutls-dev || true
$prefix apt install -y libboost-dev || true
$prefix apt install -y meson || true
$prefix apt install -y python3-jinja2 || true
$prefix apt install -y python3-ply || true
$prefix apt install -y python3-yaml || true
$prefix apt install -y cmake || true
$prefix apt install -y pkg-config || true
$prefix apt install -y libevent-dev || true
$prefix apt install -y libdrm-dev || true
$prefix apt install -y gcc || true
$prefix apt install -y make || true
$prefix apt install -y autoconf || true
$prefix apt install -y automake || true
$prefix apt install -y cppcheck || true
$prefix apt install -y libsdl2-dev || true
$prefix apt install -y meson || true
$prefix apt install -y vim || true
$prefix apt install -y python3-jinja2 || true
$prefix apt install -y python3-ply || true
$prefix apt install -y libgtest-dev || true
$prefix apt install -y libyaml-dev || true

