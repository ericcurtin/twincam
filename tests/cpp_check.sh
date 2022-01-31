#!/bin/bash

set -e

if [ -n "$CI" ] ; then
  sudo apt install -y gcc
  sudo apt install -y make
  sudo apt install -y autoconf
  sudo apt install -y automake
  sudo apt install -y cppcheck
fi
 
if command -v cppcheck > /dev/null; then
  u="$u -U __REDIRECT_NTH -U _BSD_RUNE_T_ -U _TYPE_size_t -U __LDBL_REDIR1_DECL"
  supp="--suppress=missingInclude --suppress=unusedFunction --suppress=noExplicitConstructor"
  supp="$supp --suppress=useInitializationList --suppress=uninitMemberVar"
  arg="-q --force $u --enable=all $inc $supp --error-exitcode=1"
  cppcheck="xargs cppcheck $arg"
  find . -name  "*.h" -o -name "*.cpp"| grep -v "$suppf" | $cppcheck  --language=c++ 2>&1
fi
