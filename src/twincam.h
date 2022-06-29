#pragma once

#include <syslog.h>
#include <string>

struct options {
  unsigned long camera = 0;
#ifdef HAVE_DRM
  bool drm = false;
#endif
  bool print_available_cameras = false;
  bool print_func = false;
  bool to_syslog = false;
  bool uptime = false;
  bool verbose = false;
#ifdef HAVE_SDL
  bool sdl = false;
#endif
  std::string pf = "YUYV";
  std::string filename;
};

extern options opts;

