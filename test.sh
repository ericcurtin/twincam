#!/bin/bash

set -e

base="$(basename $PWD)"
path="/home/ecurtin/git/$base/"
pre="sudo valgrind --leak-check=full --error-exitcode=1 ./twincam"
if [ -z "$1" ]; then
  ./twincam -c -l
  $pre -c -l
  ./twincam -c
  $pre -c
  ./twincam -l
  $pre -l
  ./twincam
  $pre
else
  ssh $1 "cd $path && ./twincam -c -l"
  ssh $1 "cd $path && $pre -c -l"
  ssh $1 "cd $path && ./twincam -c"
  ssh $1 "cd $path && $pre -c"
  ssh $1 "cd $path && ./twincam -l"
  ssh $1 "cd $path && $pre -l"
  ssh $1 "cd $path && ./twincam"
  ssh $1 "cd $path && $pre"
fi

