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
    PRINT("Failed to start camera manager: %s\n", strerror(-ret));
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
  if (opts.print_available_cameras) {
    PRINT("Available cameras:\n");
    for (size_t i = 0; i < cm_->cameras().size(); ++i) {
      PRINT("%zu: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
    }

    return 0;
  }

  if (cm_->cameras().empty()) {
    PRINT("No cameras available\n");
    return 0;
  }

  CameraSession session(cm_.get());
  int ret = session.init();
  if (ret) {
    PRINT("Failed to init camera session\n");
    return ret;
  }

  ret = session.start();
  if (ret) {
    PRINT("Failed to start camera session\n");
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
  PRINT("Exiting\n");
  CamApp::instance()->quit();
  if (opts.to_syslog) {
    closelog();
  }
}

static int processArgs(int argc, char** argv) {
  const struct option options[] = {
      {"help", no_argument, 0, 'h'},    {"list-cameras", no_argument, 0, 'l'},
      {"syslog", no_argument, 0, 's'},  {"uptime", no_argument, 0, 'u'},
      {"verbose", no_argument, 0, 'v'}, {NULL, 0, 0, '\0'}};
  for (int opt;
       (opt = getopt_long(argc, argv, "hlusv", options, NULL)) != -1;) {
    switch (opt) {
      case 'l':
        opts.print_available_cameras = true;
        break;
      case 'u':
        opts.print_uptime = true;
        break;
      case 's':
        opts.to_syslog = true;
        setenv("LIBCAMERA_LOG_FILE", "syslog", 1);
        openlog("twincam", 0, LOG_LOCAL1);
        break;
      case 'v':
        opts.verbose = true;
        setenv("LIBCAMERA_LOG_LEVELS", "DEBUG", 1);
        break;
      default:
        PRINT(
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -h, --help          Print this help\n"
            "  -l, --list-cameras  List cameras\n"
            "  -u, --uptime        Trace the uptime\n"
            "  -s, --syslog        Also trace output in syslog\n"
            "  -v, --verbose       Enable verbose logging\n");
        return 1;
    }
  }

  return 0;
}

options opts;
int main(int argc, char** argv) {
  PRINT_UPTIME();  // Although this is never printed, it's important because it
                   // sets the initial timer

  CamApp app;
  struct sigaction sa = {};
  int ret = processArgs(argc, argv);
  if (ret) {
    ret = 0;
    goto end;
  }

  ret = app.init();
  if (ret) {
    ret = ret == -EINTR ? 0 : EXIT_FAILURE;
    goto end;
  }

  sa.sa_handler = &signalHandler;
  sigaction(SIGINT, &sa, nullptr);

  if (app.exec()) {
    ret = EXIT_FAILURE;
    goto end;
  }

end:
  VERBOSE_PRINT("Exit with status: %d", ret);
  return ret;
}
