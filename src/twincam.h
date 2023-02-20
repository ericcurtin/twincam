#pragma once

#include <syslog.h>
#include <string>

struct options {
  long camera = -1;
#ifdef HAVE_DRM
  bool drm = false;
#endif
  bool print_available_cameras = false;
  bool print_func = false;
  bool to_syslog = false;
  bool uptime = false;
  bool verbose = false;
  long cl = -1;
#ifdef HAVE_SDL
  bool sdl = false;
#endif
#ifdef HAVE_LIBJPEG
  std::string pf = "MJPEG";
#else
  std::string pf = "YUYV";
#endif
  std::string filename;
};

extern options opts;

