#pragma once

int uptime(float* up, float* elapsed);

#define PRINT_UPTIME()                                                     \
  do {                                                                     \
    float up;                                                              \
    float elapsed;                                                         \
    uptime(&up, &elapsed);                                                 \
    printf("%.2f (%.2f), %s at %s:%d\n", up, elapsed, __PRETTY_FUNCTION__, \
           __FILE__, __LINE__);                                            \
  } while (0)
