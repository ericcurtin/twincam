#!/bin/bash

set -e

valgrind --leak-check=full --error-exitcode=1 ./twincam -c -l
valgrind --leak-check=full --error-exitcode=1 ./twincam -c
valgrind --leak-check=full --error-exitcode=1 ./twincam -l
valgrind --leak-check=full --error-exitcode=1 ./twincam

