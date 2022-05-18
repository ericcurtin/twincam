#include "string_printf.h"

#include <stdarg.h>  // For va_start, etc.
#include <string.h>
#include <memory>    // For std::unique_ptr

std::string string_printf(const char *const fmt, ...) {
  va_list args;

  va_start(args, fmt);
  char* buf;
  const int size = vasprintf(&buf, fmt, args);
  va_end(args);

  const std::string ret(buf, size);
  free(buf);

  return ret;
}

