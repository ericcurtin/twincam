#pragma once

int twincam_open_write(const char* file);
int twincam_open_read(const char* file);
int pid_write(const int pidfile_fd);
int pid_read(const int pidfile_fd, char* const buf);
int twincam_close(const int fd);
int twincam_remove(const char* file);
