#!/bin/bash

set -ex

if command -v cppcheck > /dev/null; then
  u="$u -U __REDIRECT_NTH -U _BSD_RUNE_T_ -U _TYPE_size_t -U __LDBL_REDIR1_DECL"
  supp="--suppress=missingInclude --suppress=unusedFunction"
  supp="$supp --suppress=noExplicitConstructor --suppress=uninitMemberVar"
  supp="$supp --suppress=unknownMacro --suppress=unusedStructMember"
  if cppcheck --errorlist | grep -q id=\"constParameter\"; then
    supp="$supp --suppress=constParameter"
  fi

  arg="-q --force $u --enable=all $inc $supp --error-exitcode=1"
  cppcheck="xargs cppcheck $arg"
  find . -name  "*.h" -o -name "*.cpp" | $cppcheck --language=c++ 2>&1
fi
