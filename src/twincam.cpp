/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * main.cpp - cam - The libcamera swiss army knife
 */

#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <atomic>
#include <iomanip>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"
#include "twincam.h"
#include "uptime.h"

static void vprint(const char* const fmt, va_list args) {
  if (to_syslog) {
    va_list args2;
    va_copy(args2, args);
    vsyslog(LOG_INFO, fmt, args2);
  }

  vprintf(fmt, args);
}

static void veprint(const char* const fmt, va_list args) {
  if (to_syslog) {
    va_list args2;
    va_copy(args2, args);
    vsyslog(LOG_INFO, fmt, args2);
  }

  vfprintf(stderr, fmt, args);
}

void print(const char* const fmt, ...) {
  va_list args;

  va_start(args, fmt);
  vprint(fmt, args);
  va_end(args);
}

void eprint(const char* const fmt, ...) {
  va_list args;

  va_start(args, fmt);
  veprint(fmt, args);
  va_end(args);
}

using namespace libcamera;

class CamApp {
 public:
  CamApp();

  static CamApp* instance();

  int init();
  void cleanup() const;

  int exec();
  void quit();

 private:
  void cameraAdded(std::shared_ptr<Camera> cam);
  void cameraRemoved(std::shared_ptr<Camera> cam);
  void captureDone();
  int run();

  static std::string cameraName(const Camera* camera);

  static CamApp* app_;

  std::unique_ptr<CameraManager> cm_;

  EventLoop loop_;
};

CamApp* CamApp::app_ = nullptr;

CamApp::CamApp() {
  CamApp::app_ = this;
}

CamApp* CamApp::instance() {
  return CamApp::app_;
}

int CamApp::init() {
  cm_ = std::make_unique<CameraManager>();

  if (int ret = cm_->start(); ret) {
    print("Failed to start camera manager: %s\n", strerror(-ret));
    return ret;
  }

  return 0;
}

void CamApp::cleanup() const {
  cm_->stop();
}

int CamApp::exec() {
  PRINT_UPTIME();
  int ret;

  ret = run();
  cleanup();

  return ret;
}

void CamApp::quit() {
  loop_.exit();
}

int CamApp::run() {
  PRINT_UPTIME();
  if (print_available_cameras) {
    print("Available cameras:\n");
    for (size_t i = 0; i < cm_->cameras().size(); ++i) {
      print("%zu: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
    }
  }

  if (cm_->cameras().empty()) {
    print("No cameras available\n");
    return 0;
  }

  CameraSession session(cm_.get());
  int ret = session.init();
  if (ret) {
    print("Failed to init camera session\n");
    return ret;
  }

  ret = session.start();
  if (ret) {
    print("Failed to start camera session\n");
    return ret;
  }

  loop_.exec();

  session.stop();

  return 0;
}

std::string CamApp::cameraName(const Camera* camera) {
  const ControlList& props = camera->properties();
  bool addModel = true;
  std::string name;

  /*
   * Construct the name from the camera location, model and ID. The model
   * is only used if the location isn't present or is set to External.
   */
  if (props.contains(properties::Location)) {
    switch (props.get(properties::Location)) {
      case properties::CameraLocationFront:
        addModel = false;
        name = "Internal front camera ";
        break;
      case properties::CameraLocationBack:
        addModel = false;
        name = "Internal back camera ";
        break;
      case properties::CameraLocationExternal:
        name = "External camera ";
        break;
      default:
        break;
    }
  }

  if (addModel && props.contains(properties::Model)) {
    /*
     * If the camera location is not availble use the camera model
     * to build the camera name.
     */
    name = "'" + props.get(properties::Model) + "' ";
  }

  name += "(" + camera->id() + ")";

  return name;
}

void signalHandler([[maybe_unused]] int signal) {
  print("Exiting\n");
  CamApp::instance()->quit();
  if (to_syslog) {
    closelog();
  }
}

static int processArgs(int argc, char** argv) {
  const struct option options[] = {{"list-cameras", no_argument, 0, 'c'},
                                   {"help", no_argument, 0, 'h'},
                                   {"uptime", optional_argument, 0, 'u'},
                                   {NULL, 0, 0, '\0'}};
  for (int opt;
       (opt = getopt_long(argc, argv, "chu::s", options, NULL)) != -1;) {
    switch (opt) {
      case 'c':
        print_available_cameras = true;
        break;
      case 'u':
        print_uptime = true;
        break;
      case 's':
        to_syslog = true;
        openlog("twincam", 0, LOG_LOCAL1);
        break;
      default:
        print(
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -c, --list-cameras  List cameras\n"
            "  -h, --help          Print this help\n"
            "  -u, --uptime        Trace the uptime\n"
            "  -s, --syslog        Also trace output in syslog\n");
        return 1;
    }
  }

  return 0;
}

bool print_uptime = false;
bool to_syslog = false;
bool print_available_cameras = false;
int main(int argc, char** argv) {
  PRINT_UPTIME();  // Although this is never printed, it's important because it
                   // sets the initial timer
  if (int ret = processArgs(argc, argv); ret) {
    return 0;
  }

  CamApp app;
  if (int ret = app.init(); ret)
    return ret == -EINTR ? 0 : EXIT_FAILURE;

  struct sigaction sa = {};
  sa.sa_handler = &signalHandler;
  sigaction(SIGINT, &sa, nullptr);

  if (app.exec())
    return EXIT_FAILURE;

  return 0;
}
