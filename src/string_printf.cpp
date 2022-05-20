#include "string_printf.h"

#include <stdarg.h>  // For va_start, etc.
#include <string.h>
#include <memory>    // For std::unique_ptr

static int vasprintf(std::string& str, const char* fmt, va_list args) {
  int size = 0;
  va_list tmpa;

  // copy
  va_copy(tmpa, args);

  // apply variadic arguments to
  // sprintf with format to get size
  size = vsnprintf(NULL, 0, fmt, tmpa);

  // toss args
  va_end(tmpa);

  // return -1 to be compliant if
  // size is less than 0
  if (size < 0) {
    return -1;
  }

  // alloc with size plus 1 for `\0'
  str.resize(size);

  // format string with original
  // variadic arguments and set new size
  size = vsnprintf(&str[0], str.size() + 1, fmt, args);
  return size;
}

std::string string_printf(const char *const fmt, ...) {
  va_list args;

  va_start(args, fmt);
  std::string ret;
  vasprintf(ret, fmt, args);
  va_end(args);

  return ret;
}

