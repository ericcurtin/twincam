#!/bin/bash

set -e

./build.sh
./build.sh $1
./test.sh
./test.sh $1

