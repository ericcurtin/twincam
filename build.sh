#!/bin/bash

set -e

base="$(basename $PWD)"
path="/home/ecurtin/git/$base/"
cmd1="clang-format -i --style=file twincam.cpp"
cmd2="unbuffer g++ --std=c++17 -ggdb -O0 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -Werror -o twincam twincam.cpp -I /usr/include/libcamera -lkms++util -lcamera -lkms++ -lfmt | head"
if [ -z "$1" ]; then
  $cmd1
  /bin/bash -c "$cmd2"
else
  ssh $1 "cd $path && $cmd1 && $cmd2"
fi

