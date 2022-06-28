#pragma once

#include <syslog.h>
#include <string>

struct options {
  unsigned long camera = 0;
  bool print_available_cameras = false;
  bool print_func = false;
  bool to_syslog = false;
  bool verbose = false;
  bool sdl = false;
  std::string pf = "YUYV";
  std::string filename;
};

extern options opts;

