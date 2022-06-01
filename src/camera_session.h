/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * camera_session.h - Camera capture session
 */

#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/signal.h>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

#if 0  // not priority right now, for MJPG mainly
#include "image.h"
#endif

class FrameSink;

class CameraSession {
 public:
  CameraSession(const libcamera::CameraManager* const cm);
  ~CameraSession();

  int init();
  bool isValid() const { return config_ != nullptr; }

  libcamera::Camera* camera() { return camera_.get(); }
  libcamera::CameraConfiguration* config() { return config_.get(); }

  void listControls() const;
  void listProperties() const;
  void infoConfiguration() const;

  int start();
  void stop();

  libcamera::Signal<> captureDone;

 private:
  int startCapture();

  int queueRequest(libcamera::Request* request);
  void requestComplete(libcamera::Request* request);
  void processRequest(libcamera::Request* request);
  void sinkRelease(libcamera::Request* request);

  std::shared_ptr<libcamera::Camera> camera_;
  std::unique_ptr<libcamera::CameraConfiguration> config_;

  std::map<const libcamera::Stream*, std::string> streamNames_;
  std::unique_ptr<FrameSink> sink_;
  unsigned int cameraIndex_ = 0;

  uint64_t last_ = 0;

  unsigned int queueCount_ = 0;
  unsigned int captureCount_ = 0;

#if 0  // not priority right now, for MJPG mainly
  FormatConverter converter_;
#endif

  std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;
  std::vector<std::unique_ptr<libcamera::Request>> requests_;
#if 0  // not priority right now, for MJPG mainly
  std::map<libcamera::FrameBuffer*, std::unique_ptr<Image>> mappedBuffers_;
#endif
};
