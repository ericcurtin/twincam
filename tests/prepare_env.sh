#!/bin/bash
  
set -ex

if [ "$EUID" -ne 0 ]; then
  prefix="sudo"
fi

if command -v apt > /dev/null; then
  export DEBIAN_FRONTEND=noninteractive
  $prefix apt update || true
  $prefix apt install -y git || true
  git fetch --unshallow &
  $prefix apt install -y clang || true
  $prefix apt install -y gnutls-dev || true
  $prefix apt install -y libboost-dev || true
  $prefix apt install -y meson || true
  $prefix apt install -y python3-pip || true
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
  $prefix apt install -y libgtest-dev || true
  $prefix apt install -y libyaml-dev || true
  $prefix apt install -y curl || true
  $prefix apt install -y zip || true
  $prefix apt install -y valgrind || true
  $prefix apt install -y libasan5 || true
  $prefix apt install -y libsdl2-image-dev || true
elif command -v dnf > /dev/null; then
  $prefix dnf install -y 'dnf-command(config-manager)'
  $prefix dnf config-manager --set-enabled crb || true
  $prefix dnf install -y epel-release || true
  $prefix dnf install -y SDL2_image-devel || true
  $prefix dnf install -y git gcc g++ libevent libevent-devel openssl \
    openssl-devel gnutls gnutls-devel meson boost boost-devel python3-pip \
    libdrm libdrm-devel systemd-udev doxygen cmake graphviz libatomic \
    texlive-latex cppcheck libyaml-devel clang zip valgrind libasan findutils \
    systemd-devel
fi

$prefix pip install jinja2 ply pyyaml

