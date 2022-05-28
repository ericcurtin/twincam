#pragma once

int twncm_open_write(const char* file);
int twncm_open_read(const char* file);
int pid_write(const int pidfile_fd);
int pid_read(const int pidfile_fd, char* const buf);
int twncm_close(const int fd);
int twncm_remove(const char* file);
