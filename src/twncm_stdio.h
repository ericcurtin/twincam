#pragma once

#include <string.h>
#include <string>

#include "twincam.h"
#include "uptime.h"

int uptime(float* up, float* elapsed);
void uptime_str(std::string& str);

// function that will sprintf to a C++ string starting from std::string::size()
// so if you want to completely overwrite a string or start at a specific point
// use std::string::clear() or std::string::resize(). str is a std::string.
// Tries to write to capacity of string and extends capacity if needs be.
#define STRING_PRINTF(str, ...)                                  \
  do {                                                           \
    const size_t write_point = str.size();                       \
    str.resize(str.capacity());                                  \
    const int capacity_left = str.capacity() - str.size();       \
    const int sizesn =                                           \
        snprintf(&str[write_point], capacity_left, __VA_ARGS__); \
    str.resize(write_point + sizesn);                            \
    if (sizesn < capacity_left) {                                \
      break;                                                     \
    }                                                            \
                                                                 \
    snprintf(&str[write_point], sizesn + 1, __VA_ARGS__);        \
  } while (0)

#define PRINT(...)                         \
  do {                                     \
    std::string str;                       \
    uptime_str(str);                       \
    str += ", ";                           \
    STRING_PRINTF(str, __VA_ARGS__);       \
    if (opts.to_syslog) {                  \
      syslog(LOG_INFO, "%s", str.c_str()); \
    }                                      \
                                           \
    printf("%s", str.c_str());             \
  } while (0)

#define EPRINT(...)                        \
  do {                                     \
    std::string str;                       \
    uptime_str(str);                       \
    str += ", ";                           \
    STRING_PRINTF(str, __VA_ARGS__);       \
    if (opts.to_syslog) {                  \
      syslog(LOG_INFO, "%s", str.c_str()); \
    }                                      \
                                           \
    fprintf(stderr, "%s", str.c_str());    \
  } while (0)

#define VERBOSE_PRINT(...)                   \
  do {                                       \
    if (opts.verbose) {                      \
      std::string str;                       \
      uptime_str(str);                       \
      str += ", ";                           \
      STRING_PRINTF(str, __VA_ARGS__);       \
      if (opts.to_syslog) {                  \
        syslog(LOG_INFO, "%s", str.c_str()); \
      }                                      \
                                             \
      printf("%s", str.c_str());             \
    }                                        \
  } while (0)
