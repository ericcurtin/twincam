#include "twncm_fnctl.h"
#include <fcntl.h>
#include <unistd.h>
#include "twincam.h"
#include "twncm_stdio.h"

int twncm_open_write(const char* file) {
  int fd_ = open(file, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
  if (fd_ < 0) {
    EPRINT("Failed to open file %s: %s\n", file, strerror(errno));
  }

  return fd_;
}

int twncm_open_read(const char* file) {
  int fd_ = open(file, O_RDONLY | O_CLOEXEC);
  if (fd_ < 0) {
    EPRINT("Failed to open file %s: %s\n", file, strerror(errno));
  }

  return fd_;
}

int pid_write(const int pidfile_fd) {
  char buf[16];
  int bytes_to_write = snprintf(buf, 16, "%d", getpid());
  ssize_t bytes_written = write(pidfile_fd, buf, bytes_to_write);
  if (bytes_written < 0) {
    EPRINT("Failed to write %ld bytes to fd %d: %s\n", bytes_written,
           pidfile_fd, strerror(errno));
  }

  return bytes_written;
}

int pid_read(const int pidfile_fd, char* const buf) {
  ssize_t bytes_read = read(pidfile_fd, buf, 15);
  if (bytes_read < 0) {
    EPRINT("Failed to read %ld bytes from fd %d: %s\n", bytes_read, pidfile_fd,
           strerror(errno));
  }

  return bytes_read;
}

int twncm_close(const int fd) {
  int ret = close(fd);
  if (ret < 0) {
    EPRINT("Failed %d = close(%d): %s\n", ret, fd, strerror(errno));
  }

  return ret;
}

int twncm_remove(const char* file) {
  int ret = unlink(file);
  if (ret < 0) {
    EPRINT("Failed %d = unlink(%s): %s\n", ret, file, strerror(errno));
  }

  return ret;
}
