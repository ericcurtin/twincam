#pragma once

#include <syslog.h>

struct options {
  int camera = 0;
  bool print_available_cameras = false;
  bool print_uptime = false;
  bool to_syslog = false;
  bool verbose = false;
};

extern options opts;

