#!/bin/bash

set -e

base="$(basename $PWD)"
path="/home/ecurtin/git/$base/"
pre="sudo valgrind --leak-check=full --error-exitcode=1 ./twincam"
if [ -z "$1" ]; then
  $pre -c -l
  $pre -c
  $pre -l
  $pre
else
  ssh $1 "cd $path && $pre -c -l"
  ssh $1 "cd $path && $pre -c -l"
  ssh $1 "cd $path && $pre -c -l"
  ssh $1 "cd $path && $pre -c -l"
fi

