#!/bin/bash

set -ex

if command -v podman > /dev/null; then
  container="podman"
elif command -v docker > /dev/null; then
  container="docker"
fi

container_run() {
  cmd="tests/prepare_env.sh && tests/build_twincam.sh && tests/sonar.sh && tests/cpp_check.sh"
  id=$(sudo $container run --cap-add=SYS_PTRACE -d -it $1 /bin/bash)
  sudo $container exec -it $id /bin/bash -c "mkdir -p $PWD"
  sudo $container cp $PWD $id:$PWD/..
  sudo $container exec -it $id /bin/bash -c "cd $PWD && $cmd"
  sudo $container rm -f $id
}

if [ -n "$container" ]; then
  container_run "centos:stream9"
  container_run "fedora:36"
  container_run "ubuntu:22.04"
  exit 0
fi

$cmd

