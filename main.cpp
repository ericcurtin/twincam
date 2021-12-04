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
#include <iostream>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"
#include "main.h"

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

  std::atomic_uint loopUsers_;
  EventLoop loop_;
};

CamApp* CamApp::app_ = nullptr;

CamApp::CamApp() : loopUsers_(0) {
  CamApp::app_ = this;
}

CamApp* CamApp::instance() {
  return CamApp::app_;
}

int CamApp::init() {
  cm_ = std::make_unique<CameraManager>();

  int ret = cm_->start();
  if (ret) {
    std::cout << "Failed to start camera manager: " << strerror(-ret)
              << std::endl;
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

void CamApp::cameraAdded(std::shared_ptr<Camera> cam) {
  std::cout << "Camera Added: " << cam->id() << std::endl;
}

void CamApp::cameraRemoved(std::shared_ptr<Camera> cam) {
  std::cout << "Camera Removed: " << cam->id() << std::endl;
}

void CamApp::captureDone() {
  if (--loopUsers_ == 0)
    EventLoop::instance()->exit(0);
}

int CamApp::run(int argc, char** argv) {
    for (int opt; (opt = getopt(argc, argv, "lch")) != -1; ) {
     switch (opt) {
     case 'l': break;
     case 'c':     printf("Available cameras:\n");
         for (size_t i = 0; i < cm_->cameras().size(); ++i) {
           printf("%ld: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
         }

         break;
     case 'h':
     default:
         printf(    "Usage: twincam [OPTIONS]\n\n"
                  "Options:\n"
                  "  -l, --list-displays         List displays\n"
                  "  -c, --list-cameras          List cameras\n"
                  "  -h, --help                  Print this help\n");
     }
}
#if 0
  /* 2. Create the camera sessions. */
  std::vector<std::unique_ptr<CameraSession>> sessions;

  if (options_.isSet(OptCamera)) {
    unsigned int index = 0;

    for (const OptionValue& camera : options_[OptCamera].toArray()) {
      std::unique_ptr<CameraSession> session = std::make_unique<CameraSession>(
          cm_.get(), camera.toString(), index, camera.children());
      if (!session->isValid()) {
        std::cout << "Failed to create camera session" << std::endl;
        return -EINVAL;
      }

      std::cout << "Using camera " << session->camera()->id() << " as cam"
                << index << std::endl;

      session->captureDone.connect(this, &CamApp::captureDone);

      sessions.push_back(std::move(session));
      index++;
    }
  }

  /* 3. Print camera information. */
  if (options_.isSet(OptListControls) || options_.isSet(OptListProperties) ||
      options_.isSet(OptInfo)) {
    for (const auto& session : sessions) {
      if (options_.isSet(OptListControls))
        session->listControls();
      if (options_.isSet(OptListProperties))
        session->listProperties();
      if (options_.isSet(OptInfo))
        session->infoConfiguration();
    }
  }

  /* 4. Start capture. */
  for (const auto& session : sessions) {
    if (!session->options().isSet(OptCapture))
      continue;

    ret = session->start();
    if (ret) {
      std::cout << "Failed to start camera session" << std::endl;
      return ret;
    }

    loopUsers_++;
  }

  /* 5. Enable hotplug monitoring. */
  if (options_.isSet(OptMonitor)) {
    std::cout << "Monitoring new hotplug and unplug events" << std::endl;
    std::cout << "Press Ctrl-C to interrupt" << std::endl;

    cm_->cameraAdded.connect(this, &CamApp::cameraAdded);
    cm_->cameraRemoved.connect(this, &CamApp::cameraRemoved);

    loopUsers_++;
  }

  if (loopUsers_)
    loop_.exec();

  /* 6. Stop capture. */
  for (const auto& session : sessions) {
    if (!session->options().isSet(OptCapture))
      continue;

    session->stop();
  }
#endif

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
  std::cout << "Exiting" << std::endl;
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
