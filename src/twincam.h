#pragma once

#include <syslog.h>
#include <string>

struct options {
  unsigned long camera = 0;
  unsigned long capture_limit = 0;
  bool capture_limit_reached = false;
  bool print_available_cameras = false;
  bool print_uptime = false;
  bool to_syslog = false;
  bool verbose = false;
  bool sdl = false;
  std::string pf = "YUYV";
  std::string filename;
};

extern options opts;

