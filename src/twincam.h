#pragma once

#include <syslog.h>

extern bool to_syslog;

#define print(...)                   \
  do {                               \
    if (to_syslog) {                 \
      syslog(LOG_INFO, __VA_ARGS__); \
    }                                \
                                     \
    printf(__VA_ARGS__);             \
  } while (0)

#define eprint(...)                  \
  do {                               \
    if (to_syslog) {                 \
      syslog(LOG_INFO, __VA_ARGS__); \
    }                                \
                                     \
    fprintf(stderr, __VA_ARGS__);    \
  } while (0)
