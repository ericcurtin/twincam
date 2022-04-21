#!/bin/bash

set -e

tests/prepare_env.sh
tests/build_twincam.sh
tests/sonar.sh
tests/cpp_check.sh

