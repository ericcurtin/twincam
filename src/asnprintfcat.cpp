#include "asnprintfcat.h"

#include <stdio.h>

int asnprintfcat(char** str,
                 size_t* size,
                 size_t* capacity,
                 const char* fmt,
                 ...) {
  va_list args;

  va_start(args, fmt);
  *size = vasnprintfcat(str, size, capacity, fmt, args);
  va_end(args);

  return *size;
}

int vasnprintfcat(char** str,
                  size_t* size,
                  size_t* capacity,
                  const char* fmt,
                  va_list args) {
  va_list tmpa;

  va_copy(tmpa, args);
  const int additional_size = vsnprintf(NULL, 0, fmt, tmpa);
  va_end(tmpa);
  if (additional_size < 0) {
    return -1;
  }

  const size_t new_size = *size + additional_size;
  if (new_size > *capacity) {
    *capacity = new_size;

    // Get closes power of 2
    --*capacity;
    *capacity |= *capacity >> 1;
    *capacity |= *capacity >> 2;
    *capacity |= *capacity >> 4;
    *capacity |= *capacity >> 8;
    *capacity |= *capacity >> 16;
    ++*capacity;

    *str = (char*)realloc(*str, *capacity);

    --*capacity;
  }

  if (!*str) {
    return -1;
  }

  *size += vsnprintf(*str + *size, *capacity, fmt, args);

  return *size;
}
