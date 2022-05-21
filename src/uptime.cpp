#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include <string>

#include "twincam.h"
#include "uptime.h"

struct c_str {
  ~c_str() { free(buf); }

  char* buf = 0;
};

float init_time = 0;
int uptime(float* up, float* elapsed) {
  int fd;
  const char* filename = "/proc/uptime";
  char buf[32];
  ssize_t n;
  if ((fd = open(filename, O_RDONLY)) == -1) {
    fputs("Error: /proc must be mounted\n", stderr);
    fflush(NULL);
    return 102;
  }

  lseek(fd, 0L, SEEK_SET);
  if ((n = read(fd, buf, sizeof buf - 1)) < 0) {
    perror(filename);
    fflush(NULL);
    return 103;
  }

  buf[n] = '\0';

  c_str savelocale;
  savelocale.buf = strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  if (sscanf(buf, "%f", up) < 1) {
    setlocale(LC_NUMERIC, savelocale.buf);
    fputs("bad data in /proc/uptime\n", stderr);
    return 104;
  }

  if (init_time) {
    *elapsed = *up - init_time;
  } else {
    init_time = *up;
    *elapsed = 0;
  }

  setlocale(LC_NUMERIC, savelocale.buf);
  return 0;
}

