#!/bin/bash

set -ex

if [ -d "../libcamera" ]; then
  rm -rf ../libcamera
fi

if [ "$EUID" -ne 0 ]; then
  prefix="sudo"
fi

tests() {
  twincam
  twincam -h
  twincam -c
  twincam -u
  twincam -s
}

build() {
  meson build $1 --buildtype=release --prefix=/usr
  ninja -v -C build
  $prefix ninja -v -C build install
}

valgrind_tests() {
  valgrind twincam
  valgrind twincam -h
  valgrind twincam -c
  valgrind twincam -u
  valgrind twincam -s
}

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
valgrind_tests

export CC=gcc
export CXX=g++
git clean -fdx > /dev/null 2>&1
build
tests
valgrind_tests

git clean -fdx > /dev/null 2>&1
build "-Db_sanitize=address"
tests

