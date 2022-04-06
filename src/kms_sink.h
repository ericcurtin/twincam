/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021, Ideas on Board Oy
 *
 * kms_sink.h - KMS Sink
 */

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <libcamera/base/signal.h>

#include <libcamera/camera.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>

#include "drm.h"

class KMSSink {
 public:
  KMSSink(const std::string& connectorName);

  void mapBuffer(libcamera::FrameBuffer* buffer);

  int configure(const libcamera::CameraConfiguration& config);
  int stop();

  bool processRequest(libcamera::Request* request);

  libcamera::Signal<libcamera::Request*> requestProcessed;

 private:
  class Request {
   public:
    Request(DRM::AtomicRequest* drmRequest, libcamera::Request* camRequest)
        : drmRequest_(drmRequest), camRequest_(camRequest) {}

    std::unique_ptr<DRM::AtomicRequest> drmRequest_;
    libcamera::Request* camRequest_;
  };

  int configurePipeline(const libcamera::PixelFormat& format);
  void requestComplete(DRM::AtomicRequest* request);

  DRM::Device dev_;

  const DRM::Connector* connector_ = nullptr;
  const DRM::Crtc* crtc_ = nullptr;
  const DRM::Plane* plane_ = nullptr;
  const DRM::Mode* mode_ = nullptr;

  libcamera::PixelFormat format_;
  libcamera::Size size_;
  unsigned int stride_;
  unsigned int x_;  // Where to start drawing camera output
  unsigned int y_;  // Where to start drawing camera output

  std::map<libcamera::FrameBuffer*, std::unique_ptr<DRM::FrameBuffer>> buffers_;

  std::mutex lock_;
  std::unique_ptr<Request> pending_;
  std::unique_ptr<Request> queued_;
  std::unique_ptr<Request> active_;
};
