/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * main.cpp - cam - The libcamera swiss army knife
 */

#include <signal.h>
#include <string.h>
#include <atomic>
#include <iomanip>
#include <unistd.h>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"
#include "twincam.h"

using namespace libcamera;

class CamApp {
 public:
  CamApp();

  static CamApp* instance();

  int init();
  void cleanup();

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

  int ret = cm_->start();
  if (ret) {
    printf("Failed to start camera manager: %s\n", strerror(-ret));
    return ret;
  }

  return 0;
}

void CamApp::cleanup() {
  cm_->stop();
}

int CamApp::exec(int argc, char** argv) {
  int ret;

  ret = run(argc, argv);
  cleanup();

  return ret;
}

void CamApp::quit() {
  loop_.exit();
}

int CamApp::run(int argc, char** argv) {
  for (int opt; (opt = getopt(argc, argv, "lch")) != -1;) {
    switch (opt) {
      case 'c':
        printf("Available cameras:\n");
        for (size_t i = 0; i < cm_->cameras().size(); ++i) {
          printf("%zu: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
        }

        break;
      case 'h':
      default:
        printf(
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -c, --list-cameras          List cameras\n"
            "  -h, --help                  Print this help\n");
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
        name = "Camera location unknown"
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
}

int main(int argc, char** argv) {
  CamApp app;
  int ret = app.init();
  if (ret)
    return ret == -EINTR ? 0 : EXIT_FAILURE;

  struct sigaction sa = {};
  sa.sa_handler = &signalHandler;
  sigaction(SIGINT, &sa, nullptr);

  if (app.exec(argc, argv))
    return EXIT_FAILURE;

  return 0;
}
