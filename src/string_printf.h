#pragma once

#include <string.h>
#include <string>

// function that will sprintf to a C++ string starting from std::string::size()
// so if you want to completely overwrite a string or start at a specific point
// use std::string::clear() or std::string::resize(). str is a std::string.
#define STRING_PRINTF(str, ...)                                   \
  do {                                                            \
    const int size = snprintf(NULL, 0, __VA_ARGS__);              \
    const size_t start_of_string = str.size();                    \
    str.resize(start_of_string + size);                           \
    snprintf(&str[start_of_string], str.size() + 1, __VA_ARGS__); \
  } while (0)
