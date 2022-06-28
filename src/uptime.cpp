#include "uptime.h"

float init_time = 0;
int uptime(float* up, float* elapsed) {
  struct timespec timespec = {0, 0};
  clock_gettime(CLOCK_MONOTONIC, &timespec);
  *up = ((timespec.tv_sec * 1000000000) + timespec.tv_nsec) / 1000000000.0;
  if (init_time) {
    *elapsed = *up - init_time;
  } else {
    init_time = *up;
    *elapsed = 0;
  }

  return 0;
}

void uptime_str(std::string& str) {
  float up;
  float elapsed;
  uptime(&up, &elapsed);
  STRING_PRINTF(str, "%.6f (%.2f)", up, elapsed);
}
