#pragma once

#include <syslog.h>
#include <string>

struct options {
  unsigned long camera = 0;
  bool print_available_cameras = false;
  bool print_uptime = false;
  bool to_syslog = false;
  bool verbose = false;
  bool opt_sdl = false;
  std::string opt_pf = "YUYV";
};

extern options opts;

