/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * camera_session.cpp - Camera capture session
 */

#include <limits.h>
#include <iomanip>
#include <sstream>

#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"
#include "kms_sink.h"
#include "main.h"

using namespace libcamera;

CameraSession::CameraSession(CameraManager* cm,
                             const std::string& cameraId,
                             unsigned int cameraIndex)
    : cameraIndex_(cameraIndex), last_(0), queueCount_(0), captureCount_(0) {
  char* endptr;
  unsigned long index = strtoul(cameraId.c_str(), &endptr, 10);
  if (*endptr == '\0' && index > 0 && index <= cm->cameras().size())
    camera_ = cm->cameras()[index - 1];
  else
    camera_ = cm->get(cameraId);

  if (!camera_) {
    eprintf("Camera %s not found\n", cameraId.c_str());
    return;
  }

  if (camera_->acquire()) {
    eprintf("Failed to acquire camera %s\n", cameraId.c_str());
    return;
  }

  std::unique_ptr<CameraConfiguration> config =
      camera_->generateConfiguration();
  if (!config || config->size() != 1) {
    eprintf("Failed to get default stream configuration\n");
    return;
  }

  config_ = std::move(config);
}

CameraSession::~CameraSession() {
  if (camera_)
    camera_->release();
}

void CameraSession::listControls() const {
  for (const auto& ctrl : camera_->controls()) {
    const ControlId* id = ctrl.first;
    const ControlInfo& info = ctrl.second;

    printf("Control: %s: %s\n", id->name().c_str(), info.toString().c_str());
  }
}

void CameraSession::listProperties() const {
  for (const auto& prop : camera_->properties()) {
    const ControlId* id = properties::properties.at(prop.first);
    const ControlValue& value = prop.second;

    printf("Property: %s = %s\n", id->name().c_str(), value.toString().c_str());
  }
}

void CameraSession::infoConfiguration() const {
  unsigned int index = 0;
  for (const StreamConfiguration& cfg : *config_) {
    printf("%d: %s\n", index, cfg.toString().c_str());

    const StreamFormats& formats = cfg.formats();
    for (PixelFormat pixelformat : formats.pixelformats()) {
      printf(" * Pixelformat: %s %s\n", pixelformat.toString().c_str(),
             formats.range(pixelformat).toString().c_str());

      for (const Size& size : formats.sizes(pixelformat))
        printf("  - %s\n", size.toString().c_str());
    }

    index++;
  }
}

int CameraSession::start() {
  int ret;

  queueCount_ = 0;
  captureCount_ = 0;

  ret = camera_->configure(config_.get());
  if (ret < 0) {
    printf("Failed to configure camera\n");
    return ret;
  }

  streamNames_.clear();
  for (unsigned int index = 0; index < config_->size(); ++index) {
    StreamConfiguration& cfg = config_->at(index);
    streamNames_[cfg.stream()] = "cam" + std::to_string(cameraIndex_) +
                                 "-stream" + std::to_string(index);
  }

  camera_->requestCompleted.connect(this, &CameraSession::requestComplete);
  sink_ = std::make_unique<KMSSink>("");

  if (sink_) {
    ret = sink_->configure(*config_);
    if (ret < 0) {
      printf("Failed to configure frame sink\n");
      return ret;
    }

    sink_->requestProcessed.connect(this, &CameraSession::sinkRelease);
  }

  allocator_ = std::make_unique<FrameBufferAllocator>(camera_);

  return startCapture();
}

void CameraSession::stop() {
  int ret = camera_->stop();
  if (ret)
    printf("Failed to stop capture\n");

  if (sink_) {
    ret = sink_->stop();
    if (ret)
      printf("Failed to stop frame sink\n");
  }

  sink_.reset();

  requests_.clear();

  allocator_.reset();
}

int CameraSession::startCapture() {
  int ret;

  /* Identify the stream with the least number of buffers. */
  unsigned int nbuffers = UINT_MAX;
  for (StreamConfiguration& cfg : *config_) {
    ret = allocator_->allocate(cfg.stream());
    if (ret < 0) {
      eprintf("Can't allocate buffers\n");
      return -ENOMEM;
    }

    unsigned int allocated = allocator_->buffers(cfg.stream()).size();
    nbuffers = std::min(nbuffers, allocated);
  }

  /*
   * TODO: make cam tool smarter to support still capture by for
   * example pushing a button. For now run all streams all the time.
   */

  for (unsigned int i = 0; i < nbuffers; i++) {
    std::unique_ptr<Request> request = camera_->createRequest();
    if (!request) {
      eprintf("Can't create request\n");
      return -ENOMEM;
    }

    for (StreamConfiguration& cfg : *config_) {
      Stream* stream = cfg.stream();
      const std::vector<std::unique_ptr<FrameBuffer>>& buffers =
          allocator_->buffers(stream);
      const std::unique_ptr<FrameBuffer>& buffer = buffers[i];

      ret = request->addBuffer(stream, buffer.get());
      if (ret < 0) {
        eprintf("Can't set buffer for request\n");
        return ret;
      }

      if (sink_)
        sink_->mapBuffer(buffer.get());
    }

    requests_.push_back(std::move(request));
  }

  if (sink_) {
    ret = sink_->start();
    if (ret) {
      printf("Failed to start frame sink\n");
      return ret;
    }
  }

  ret = camera_->start();
  if (ret) {
    printf("Failed to start capture\n");
    if (sink_)
      sink_->stop();
    return ret;
  }

  for (std::unique_ptr<Request>& request : requests_) {
    ret = queueRequest(request.get());
    if (ret < 0) {
      eprintf("Can't queue request\n");
      camera_->stop();
      if (sink_)
        sink_->stop();
      return ret;
    }
  }

  printf("cam%d: Capture until user interrupts by SIGINT\n", cameraIndex_);

  return 0;
}

int CameraSession::queueRequest(Request* request) {
  queueCount_++;

  return camera_->queueRequest(request);
}

void CameraSession::requestComplete(Request* request) {
  if (request->status() == Request::RequestCancelled)
    return;

  /*
   * Defer processing of the completed request to the event loop, to avoid
   * blocking the camera manager thread.
   */
  EventLoop::instance()->callLater([=, this]() { processRequest(request); });
}

void CameraSession::processRequest(Request* request) {
  const Request::BufferMap& buffers = request->buffers();

  /*
   * Compute the frame rate. The timestamp is arbitrarily retrieved from
   * the first buffer, as all buffers should have matching timestamps.
   */
  uint64_t ts = buffers.begin()->second->metadata().timestamp;
  double fps = ts - last_;
  fps = last_ != 0 && fps ? 1000000000.0 / fps : 0.0;
  last_ = ts;

  bool requeue = true;

  std::stringstream info;
  info << ts / 1000000000 << "." << std::setw(6) << std::setfill('0')
       << ts / 1000 % 1000000 << " (" << std::fixed << std::setprecision(2)
       << fps << " fps)";

  for (auto it = buffers.begin(); it != buffers.end(); ++it) {
    const Stream* stream = it->first;
    FrameBuffer* buffer = it->second;

    const FrameMetadata& metadata = buffer->metadata();

    info << " " << streamNames_[stream] << " seq: " << std::setw(6)
         << std::setfill('0') << metadata.sequence << " bytesused: ";

    unsigned int nplane = 0;
    for (const FrameMetadata::Plane& plane : metadata.planes()) {
      info << plane.bytesused;
      if (++nplane < metadata.planes().size())
        info << "/";
    }
  }

  if (sink_) {
    if (!sink_->processRequest(request))
      requeue = false;
  }

  puts(info.str().c_str());

#if 0
  if (printMetadata_) {
    const ControlList& requestMetadata = request->metadata();
    for (const auto& ctrl : requestMetadata) {
      const ControlId* id = controls::controls.at(ctrl.first);
      std::cout << "\t" << id->name() << " = " << ctrl.second.toString()
                << std::endl;
    }
  }
#endif

  /*
   * Notify the user that capture is complete if the limit has just been
   * reached.
   */
  captureCount_++;

  /*
   * If the frame sink holds on the request, we'll requeue it later in the
   * complete handler.
   */
  if (!requeue)
    return;

  request->reuse(Request::ReuseBuffers);
  camera_->queueRequest(request);
}

void CameraSession::sinkRelease(Request* request) {
  request->reuse(Request::ReuseBuffers);
  queueRequest(request);
}
