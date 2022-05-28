#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "twncm_stdio.h"

int uptime(float* up, float* elapsed);

extern float init_time;

#define PRINT_UPTIME()                                                      \
  do {                                                                      \
    if (opts.print_uptime) {                                                \
      float up;                                                             \
      float elapsed;                                                        \
      uptime(&up, &elapsed);                                                \
      PRINT("%.6f (%.2f), %s at %s:%d\n", up, elapsed, __PRETTY_FUNCTION__, \
            __FILE__, __LINE__);                                            \
    }                                                                       \
  } while (0)
