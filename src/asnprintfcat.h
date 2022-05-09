#include <stdarg.h>
#include <stdlib.h>

int asnprintfcat(char** str,
                 size_t* size,
                 size_t* capacity,
                 const char* fmt,
                 ...);
int vasnprintfcat(char** str,
                  size_t* size,
                  size_t* capacity,
                  const char* fmt,
                  va_list args);
