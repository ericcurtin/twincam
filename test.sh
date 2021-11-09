#!/bin/bash

set -e

valgrind --leak-check=full --error-exitcode=1 ./twincam -c -l

