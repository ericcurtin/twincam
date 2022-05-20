#!/bin/bash

set -ex

j=16

clean() {
  rm -rf build
  if [ "$arg1" == "clean" ]; then
    git clean -fdx > /dev/null 2>&1
  fi
}

# sonarcloud
export SONAR_TOKEN="44c84548eb18cdb52d0fc102d178bb5b82dcf641"
mkdir -p $HOME/.sonar
SONAR_SCANNER_VERSION="4.7.0.2747"
SONAR_SCANNER_DOWNLOAD_URL="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux.zip"
curl -sSLo $HOME/.sonar/sonar-scanner.zip $SONAR_SCANNER_DOWNLOAD_URL
unzip -o $HOME/.sonar/sonar-scanner.zip -d $HOME/.sonar/
export PATH="$HOME/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-linux/bin:$PATH"
SONAR_SERVER_URL="https://sonarcloud.io"
BUILD_WRAPPER_DOWNLOAD_URL="$SONAR_SERVER_URL/static/cpp/build-wrapper-linux-x86.zip"
curl -sSLo $HOME/.sonar/build-wrapper-linux-x86.zip $BUILD_WRAPPER_DOWNLOAD_URL
unzip -o $HOME/.sonar/build-wrapper-linux-x86.zip -d $HOME/.sonar/
export PATH="$HOME/.sonar/build-wrapper-linux-x86:$PATH"
BUILD_WRAPPER_OUT_DIR="build_wrapper_output_directory"
clean
meson build --prefix=/usr
build-wrapper-linux-x86-64 --out-dir $BUILD_WRAPPER_OUT_DIR ninja -v -C build
sonar-scanner --define sonar.host.url="$SONAR_SERVER_URL" --define sonar.cfamily.build-wrapper-output="$BUILD_WRAPPER_OUT_DIR"
