#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "string_printf.h"

int uptime(float* up, float* elapsed);
void uptime_output(const std::string& s);
void write_uptime_to_file();

extern std::string uptime_filename;
extern std::string uptime_buf;
extern bool print_uptime;
extern float init_time;

#define PRINT_UPTIME()                                            \
  do {                                                            \
    if (print_uptime) {                                           \
      float up;                                                   \
      float elapsed;                                              \
      uptime(&up, &elapsed);                                      \
      const std::string this_line =                               \
          string_printf("%.6f (%.2f), %s at %s:%d", up, elapsed,  \
                        __PRETTY_FUNCTION__, __FILE__, __LINE__); \
      uptime_output(this_line);                                   \
    }                                                             \
  } while (0)
