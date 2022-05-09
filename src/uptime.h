#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "asnprintfcat.h"

int uptime(float* up, float* elapsed);
void write_uptime_to_file();

extern std::string uptime_filename;
extern char* uptime_buf;
extern size_t uptime_buf_size;
extern size_t uptime_buf_capacity;
extern bool print_uptime;

#define PRINT_UPTIME()                                                    \
  do {                                                                    \
    if (print_uptime) {                                                   \
      float up;                                                           \
      float elapsed;                                                      \
      uptime(&up, &elapsed);                                              \
      if (!uptime_filename.empty()) {                                     \
        asnprintfcat(&uptime_buf, &uptime_buf_size, &uptime_buf_capacity, \
                     "uptime: %.2f (%.2f), %s at %s:%d\n", up, elapsed,   \
                     __PRETTY_FUNCTION__, __FILE__, __LINE__);            \
      } else {                                                            \
        printf("uptime: %.2f (%.2f), %s at %s:%d\n", up, elapsed,         \
               __PRETTY_FUNCTION__, __FILE__, __LINE__);                  \
      }                                                                   \
    }                                                                     \
  } while (0)
