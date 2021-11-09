#!/bin/bash

set -e

clang-format -i --style=file twincam.cpp
g++ --std=c++17 -ggdb -O0 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -Werror -o twincam twincam.cpp -I /usr/include/libcamera -lkms++util -lcamera -lkms++

