#!/bin/bash

clang-format -i --style=file twincam.cpp
clang++ -ggdb -O0 -Wall -Wextra -Werror twincam.cpp -I /usr/include/libcamera -lkms++util -lcamera -lkms++

