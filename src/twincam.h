#pragma once

#include <syslog.h>

struct options {
  bool print_available_cameras = false;
  bool print_uptime = false;
  bool to_syslog = false;
};

extern options opts;

void print(const char* const fmt, ...);
void eprint(const char* const fmt, ...);
