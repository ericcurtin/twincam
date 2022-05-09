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

  int exec(int argc, char** argv);
  void quit();

 private:
  void cameraAdded(std::shared_ptr<Camera> cam);
  void cameraRemoved(std::shared_ptr<Camera> cam);
  void captureDone();
  int run(int argc, char** argv);

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
    printf("Failed to start camera manager: %s\n", strerror(-ret));
    return ret;
  }

  return 0;
}

void CamApp::cleanup() const {
  cm_->stop();
}

int CamApp::exec(int argc, char** argv) {
  PRINT_UPTIME();
  int ret;

  ret = run(argc, argv);
  cleanup();

  return ret;
}

void CamApp::quit() {
  loop_.exit();
}

int CamApp::run(int argc, char** argv) {
  PRINT_UPTIME();
  const struct option options[] = {{"list-cameras", no_argument, 0, 'c'},
                                   {"help", no_argument, 0, 'h'},
                                   {"uptime", optional_argument, 0, 'u'},
                                   {NULL, 0, 0, '\0'}};
  for (int opt;
       (opt = getopt_long(argc, argv, "chu::s", options, NULL)) != -1;) {
    switch (opt) {
      case 'c':
        printf("Available cameras:\n");
        for (size_t i = 0; i < cm_->cameras().size(); ++i) {
          printf("%zu: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
        }

        break;
      case 'u':
        print_uptime = true;
        if (((optarg == NULL && optind < argc && argv[optind][0] != '-')
                 ? (bool)(optarg = argv[optind++])
                 : (optarg != NULL))) {
          uptime_filename = optarg;
        }

        printf("uptime_filename %s\n", uptime_filename.c_str());
        break;
      case 's':
        print_uptime = true;
        uptime_syslog = true;
        openlog("twincam", 0, LOG_LOCAL1);
        break;
      default:
        printf(
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -c, --list-cameras  List cameras\n"
            "  -h, --help          Print this help\n"
            "  -u, --uptime        Trace the uptime (output "
            "to file if specified, otherwise "
            "stdout\n"
            "  -s, --syslog        Trace the uptime in syslog\n");
        return 0;
    }
  }

  if (cm_->cameras().empty()) {
    printf("No cameras available\n");
    return 0;
  }

  CameraSession session(cm_.get());
  int ret = session.init();
  if (ret) {
    printf("Failed to init camera session\n");
    return ret;
  }

  ret = session.start();
  if (ret) {
    printf("Failed to start camera session\n");
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
  printf("Exiting\n");
  CamApp::instance()->quit();
  if (!uptime_filename.empty()) {
    write_uptime_to_file();
  }

  if (uptime_syslog) {
    closelog();
  }
}

bool print_uptime = false;
bool uptime_syslog = false;
std::string uptime_filename;
char* uptime_buf = 0;
size_t uptime_buf_size = 0;
size_t uptime_buf_capacity = 0;
int main(int argc, char** argv) {
  PRINT_UPTIME();  // Although this is never printed, it's important because it
                   // sets the initial timer
  CamApp app;
  if (int ret = app.init(); ret)
    return ret == -EINTR ? 0 : EXIT_FAILURE;

  struct sigaction sa = {};
  sa.sa_handler = &signalHandler;
  sigaction(SIGINT, &sa, nullptr);

  if (app.exec(argc, argv))
    return EXIT_FAILURE;

  return 0;
}
