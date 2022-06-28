#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "twncm_stdio.h"

extern float init_time;

#define PRINT_FUNC()                                                   \
  do {                                                                 \
    if (opts.print_func) {                                             \
      PRINT("%s at %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }                                                                  \
  } while (0)
