#pragma once

#define eprintf(...)              \
  do                              \
    fprintf(stderr, __VA_ARGS__); \
  while (0)

