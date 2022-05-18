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

  char* savelocale = strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  if (sscanf(buf, "%f", up) < 1) {
    setlocale(LC_NUMERIC, savelocale);
    free(savelocale);
    fputs("bad data in /proc/uptime\n", stderr);
    return 104;
  }

  if (init_time) {
    *elapsed = *up - init_time;
  } else {
    init_time = *up;
    *elapsed = 0;
  }

  setlocale(LC_NUMERIC, savelocale);
  free(savelocale);
  return 0;
}

void write_uptime_to_file() {
  int fd = open(uptime_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 00666);
  if (fd < 0) {
    eprint(
        "errno: %d, %d = open(%s, "
        "O_WRONLY|O_CREAT|O_TRUNC)\n",
        errno, fd, uptime_filename.c_str());
  }

  ssize_t ret = write(fd, uptime_buf.c_str(), uptime_buf.size());
  if (ret < 0) {
    eprint("errno: %d, %ld = write(%d, uptime_buf, %zu)\n", errno, ret, fd,
           uptime_buf.size());
  }

  close(fd);
}
