#pragma once

#include <string.h>
#include <string>

// function that will sprintf to a C++ string starting from std::string::size()
// so if you want to completely overwrite a string or start at a specific point
// use std::string::clear() or std::string::resize(). str is a std::string.
// Optimistically guesses string to write is less that 128 bytes on first parse
#define STRING_PRINTF(str, ...)                                     \
  do {                                                              \
    const size_t write_point = str.size();                          \
    str.resize(write_point + 127);                                  \
    const int size = snprintf(&str[write_point], 128, __VA_ARGS__); \
    str.resize(write_point + size);                                 \
    if (size < 128) {                                               \
      break;                                                        \
    }                                                               \
                                                                    \
    snprintf(&str[write_point], size + 1, __VA_ARGS__);             \
  } while (0)
