#!/bin/bash

set -ex

if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

if [ "$EUID" -ne 0 ]; then
  prefix="sudo"
fi

tests() {
  set +e

  $1 twincam
  ret=$?
  if [ "$ret" -ne "0" ] && [ "$ret" -ne "1" ]; then
    exit $ret
  fi

  $1 twincam -c
  ret=$?
  if [ "$ret" -ne "0" ] && [ "$ret" -ne "1" ]; then
    exit $ret
  fi

  $1 twincam -u
  ret=$?
  if [ "$ret" -ne "0" ] && [ "$ret" -ne "1" ]; then
    exit $ret
  fi

  $1 twincam -s
  ret=$?
  if [ "$ret" -ne "0" ] && [ "$ret" -ne "1" ]; then
    exit $ret
  fi

  set -e

  $1 twincam -h
}

build() {
  meson build $1 --buildtype=release --prefix=/usr
  ninja -v -C build
  $prefix ninja -v -C build install
}

clang --version
gcc --version

mkdir ../libcamera
git clone https://git.libcamera.org/libcamera/libcamera.git ../libcamera
cd ../libcamera
build
cd -

export PATH="build:$PATH"
export PKG_CONFIG_PATH="/usr/lib64/pkgconfig/"
export CC=clang
export CXX=clang++
git clean -fdx > /dev/null 2>&1
build
tests
tests "valgrind --leak-check=full --error-exitcode=2"

export CC=gcc
export CXX=g++
git clean -fdx > /dev/null 2>&1
build
tests
tests "valgrind --leak-check=full --error-exitcode=2"

git clean -fdx > /dev/null 2>&1
build "-Db_sanitize=address"
tests

