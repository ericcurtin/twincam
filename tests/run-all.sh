#!/bin/bash

set -e


if command -v podman > /dev/null; then
  container="podman"
elif command -v docker > /dev/null; then
  container="docker"
fi

cmd="tests/prepare_env.sh && tests/build_twincam.sh && tests/sonar.sh && tests/cpp_check.sh"
if [ -n "$container" ]; then
  id=$(sudo $container run -d -it ubuntu:22.04 /bin/bash)
  sudo $container exec -it $id /bin/bash -c "mkdir -p $PWD"
  sudo $container cp $PWD $id:$PWD/..
  sudo $container exec -it $id /bin/bash -c "cd $PWD && $cmd"
  sudo $container rm -f $id
  exit 0
fi

$cmd

