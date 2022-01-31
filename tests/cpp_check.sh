#!/bin/bash

set -e

u="$u -U __REDIRECT_NTH -U _BSD_RUNE_T_ -U _TYPE_size_t -U __LDBL_REDIR1_DECL"
supp="--suppress=missingInclude --suppress=unusedFunction"
suppf="redblack.c"
arg="-q --force $u --enable=all $inc $supp --error-exitcode=1"
cppcheck="xargs cppcheck $arg"

find . -name "*.[c|h]" | grep -v "$suppf" | $cppcheck  --language=c++ 2>&1
