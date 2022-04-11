/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021, Ideas on Board Oy
 *
 * kms_sink.cpp - KMS Sink
 */

#include "kms_sink.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <array>
#include <memory>

#include <libcamera/camera.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer.h>
#include <libcamera/stream.h>

#include "drm.h"
#include "twincam.h"

KMSSink::KMSSink(const std::string& connectorName) {
  if (dev_.init() < 0)
    return;

  /*
   * Find the requested connector. If no specific connector is requested,
   * pick the first connected connector or, if no connector is connected,
   * the first connector with unknown status.
   */
  for (const DRM::Connector& conn : dev_.connectors()) {
    if (!connectorName.empty()) {
      if (conn.name() != connectorName)
        continue;

      connector_ = &conn;
      break;
    }

    if (conn.status() == DRM::Connector::Connected) {
      connector_ = &conn;
      break;
    }

    if (!connector_ && conn.status() == DRM::Connector::Unknown)
      connector_ = &conn;
  }

  if (!connector_) {
    if (!connectorName.empty())
      eprintf("Connector %s not found\n", connectorName.c_str());
    else
      eprintf("No connected connector found\n");
    return;
  }

  dev_.requestComplete.connect(this, &KMSSink::requestComplete);
}

void KMSSink::mapBuffer(libcamera::FrameBuffer* buffer) {
  std::array<uint32_t, 4> strides = {};

  /* \todo Should libcamera report per-plane strides ? */
  unsigned int uvStrideMultiplier;

  switch (format_) {
    case libcamera::formats::NV24:
    case libcamera::formats::NV42:
      uvStrideMultiplier = 4;
      break;
    case libcamera::formats::YUV420:
    case libcamera::formats::YVU420:
    case libcamera::formats::YUV422:
      uvStrideMultiplier = 1;
      break;
    default:
      uvStrideMultiplier = 2;
      break;
  }

  strides[0] = stride_;
  for (unsigned int i = 1; i < buffer->planes().size(); ++i)
    strides[i] = stride_ * uvStrideMultiplier / 2;

  std::unique_ptr<DRM::FrameBuffer> drmBuffer =
      dev_.createFrameBuffer(*buffer, format_, size_, strides);
  if (!drmBuffer)
    return;

  buffers_.emplace(std::piecewise_construct, std::forward_as_tuple(buffer),
                   std::forward_as_tuple(std::move(drmBuffer)));
}

int KMSSink::configure(const libcamera::CameraConfiguration& config) {
  if (!connector_)
    return -EINVAL;

  crtc_ = nullptr;
  plane_ = nullptr;
  mode_ = nullptr;

  const libcamera::StreamConfiguration& cfg = config.at(0);

  const std::vector<DRM::Mode>& modes = connector_->modes();

if (int ret = configurePipeline(cfg.pixelFormat); ret < 0)
    return ret;

  mode_ = &modes[0];
  size_ = cfg.size;
  x_ = (mode_->hdisplay - size_.width) / 2;
  y_ = (mode_->vdisplay - size_.height) / 2;
  stride_ = cfg.stride;

  return 0;
}

int KMSSink::configurePipeline(const libcamera::PixelFormat& format) {
  /*
   * If the requested format has an alpha channel, also consider the X
   * variant.
   */
  libcamera::PixelFormat xFormat;

  switch (format) {
    case libcamera::formats::ABGR8888:
      xFormat = libcamera::formats::XBGR8888;
      break;
    case libcamera::formats::ARGB8888:
      xFormat = libcamera::formats::XRGB8888;
      break;
    case libcamera::formats::BGRA8888:
      xFormat = libcamera::formats::BGRX8888;
      break;
    case libcamera::formats::RGBA8888:
      xFormat = libcamera::formats::RGBX8888;
      break;
    default:
      break;
  }

  /*
   * Find a CRTC and plane suitable for the request format and the
   * connector at the end of the pipeline. Restrict the search to primary
   * planes for now.
   */
  for (const DRM::Encoder* encoder : connector_->encoders()) {
    for (const DRM::Crtc* crtc : encoder->possibleCrtcs()) {
      for (const DRM::Plane* plane : crtc->planes()) {
        if (plane->planeType() != DRM::Plane::TypePrimary)
          continue;

        if (plane->supportsFormat(format)) {
          crtc_ = crtc;
          plane_ = plane;
          format_ = format;
          goto break_all;
        }

        if (plane->supportsFormat(xFormat)) {
          crtc_ = crtc;
          plane_ = plane;
          format_ = xFormat;
          goto break_all;
        }
      }
    }
  }

// hack to fix
break_all:

  if (!crtc_) {
    eprintf("Unable to find display pipeline for format %s\n",
            format.toString().c_str());

    return -EPIPE;
  }

  printf("Using KMS plane %u, CRTC %u, connector %s (%u)\n", plane_->id(),
         crtc_->id(), connector_->name().c_str(), connector_->id());

  return 0;
}

int KMSSink::stop() {
  /* Display pipeline. */
  DRM::AtomicRequest request(&dev_);

  request.addProperty(connector_, "CRTC_ID", 0);
  request.addProperty(crtc_, "ACTIVE", 0);
  request.addProperty(crtc_, "MODE_ID", 0);
  request.addProperty(plane_, "CRTC_ID", 0);
  request.addProperty(plane_, "FB_ID", 0);
  
  if (int ret = request.commit(DRM::AtomicRequest::FlagAllowModeset); ret < 0) {
     eprintf("Failed to stop display pipeline: %s\n", strerror(-ret));
     return ret;
  }

  /* Free all buffers. */
  pending_.reset();
  queued_.reset();
  active_.reset();
  buffers_.clear();

  return 0;
}

bool KMSSink::processRequest(libcamera::Request* camRequest) {
  /*
   * Perform a very crude rate adaptation by simply dropping the request
   * if the display queue is full.
   */
  if (pending_)
    return true;

  libcamera::FrameBuffer* buffer = camRequest->buffers().begin()->second;
  auto iter = buffers_.find(buffer);
  if (iter == buffers_.end())
    return true;

  const DRM::FrameBuffer* drmBuffer = iter->second.get();

  unsigned int flags = DRM::AtomicRequest::FlagAsync;
  auto* drmRequest = new DRM::AtomicRequest(&dev_);
  drmRequest->addProperty(plane_, "FB_ID", drmBuffer->id());

  if (!active_ && !queued_) {
    /* Enable the display pipeline on the first frame. */
    drmRequest->addProperty(connector_, "CRTC_ID", crtc_->id());


    drmRequest->addProperty(plane_, "SRC_X", 0 << 16);
    drmRequest->addProperty(plane_, "SRC_Y", 0 << 16);
    drmRequest->addProperty(plane_, "SRC_W", size_.width << 16);
    drmRequest->addProperty(plane_, "SRC_H", size_.height << 16);
    drmRequest->addProperty(plane_, "CRTC_X", x_);
    drmRequest->addProperty(plane_, "CRTC_Y", y_);
    drmRequest->addProperty(plane_, "CRTC_W", size_.width);
    drmRequest->addProperty(plane_, "CRTC_H", size_.height);

  }

  pending_ = std::make_unique<Request>(drmRequest, camRequest);

  std::scoped_lock<std::mutex> lock(lock_);

  if (!queued_) {
    if (int ret = drmRequest->commit(flags); ret < 0) {
      eprintf("Failed to commit atomic request: %s\n", strerror(-ret));
      if (-ret == EACCES) {
        eprintf(
            "You need to run 'sudo fgconsole -n | xargs sudo chvt' to switch\n"
            "out of Desktop Environment if you still have Gnome, XFCE, etc. \n"
            "running\n");
      }
    }

    queued_ = std::move(pending_);
  }

  return false;
}

void KMSSink::requestComplete(DRM::AtomicRequest* const request) {
  const std::lock_guard lock(lock_);

  assert(queued_ && queued_->drmRequest_.get() == request);

  /* Complete the active request, if any. */
  if (active_)
    requestProcessed.emit(active_->camRequest_);

  /* The queued request becomes active. */
  active_ = std::move(queued_);

  /* Queue the pending request, if any. */
  if (pending_) {
    pending_->drmRequest_->commit(DRM::AtomicRequest::FlagAsync);
    queued_ = std::move(pending_);
  }
}
