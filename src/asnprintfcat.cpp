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

  if (*size + additional_size > *capacity) {
    for (*capacity = 16; *size + additional_size - 1 > *capacity;
         *capacity *= 2) {
    }

    *str = (char*)realloc(*str, *capacity);

    --*capacity;
  }

  if (!*str) {
    return -1;
  }

  *size += vsnprintf(*str + *size, *capacity, fmt, args);

  return *size;
}
