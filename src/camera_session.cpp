/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * camera_session.cpp - Camera capture session
 */

#include <limits.h>
#include <iomanip>

#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"

#include "kms_sink.h"
#include "twincam.h"

using namespace libcamera;

CameraSession::CameraSession(const CameraManager* const cm)
    : camera_(cm->cameras()[0]) {}

CameraSession::~CameraSession() {
  if (camera_)
    camera_->release();
}

int CameraSession::init() {
  if (!camera_) {
    eprintf("Camera not found\n");
    return 1;
  }

  if (camera_->acquire()) {
    eprintf("Failed to acquire camera\n");
    return 0;
  }

  std::unique_ptr<CameraConfiguration> config =
      camera_->generateConfiguration({libcamera::Viewfinder});
  if (!config || config->size() != 1) {
    eprintf("Failed to get default stream configuration\n");
    return 0;
  }

  config->at(0).pixelFormat = PixelFormat::fromString("YUYV");

  config_ = std::move(config);

  return 0;
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
    const StreamConfiguration& cfg = config_->at(index);
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
  for (const StreamConfiguration& cfg : *config_) {
    ret = allocator_->allocate(cfg.stream());
    if (ret < 0) {
      eprintf("Can't allocate buffers\n");
      return -ENOMEM;
    }

#if 0  // not priority right now, for MJPG mainly
    for (const std::unique_ptr<FrameBuffer>& buffer :
         allocator_->buffers(cfg.stream())) {
      /* Map memory buffers and cache the mappings. */
      std::unique_ptr<Image> image =
          Image::fromFrameBuffer(buffer.get(), Image::MapMode::ReadOnly);
      if (!image) {
        eprintf("Can't allocate image buffers\n");
      }

      mappedBuffers_[buffer.get()] = std::move(image);
    }
#endif

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

    for (const StreamConfiguration& cfg : *config_) {
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

  ret = camera_->start();
  if (ret) {
    printf("Failed to start capture\n");
    if (sink_)
      sink_->stop();
    return ret;
  }

  for (const std::unique_ptr<Request>& request : requests_) {
    ret = queueRequest(request.get());
    if (ret < 0) {
      eprintf("Can't queue request\n");
      camera_->stop();
      if (sink_)
        sink_->stop();
      return ret;
    }
  }

  printf("cam%u: Capture until user interrupts by SIGINT\n", cameraIndex_);

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
  EventLoop::instance()->callLater([request, this]() { processRequest(request); });
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

  printf("%.6f (%.2f fps)", ts / 1000000000.0, fps);
  for (const auto& [stream, buffer] : buffers) {
    const FrameMetadata& metadata = buffer->metadata();

    printf(" %s seq: %d bytesused: ", streamNames_[stream].c_str(),
           metadata.sequence);
    unsigned int nplane = 0;
    for (const FrameMetadata::Plane& plane : metadata.planes()) {
      printf("%d", plane.bytesused);
      if (++nplane < metadata.planes().size())
        printf("/");
    }

#if 0  // not priority right now, for MJPG mainly
    Image* image = mappedBuffers_[buffer].get();
    converter_.convert(image, size, &image_);
#endif
  }

  if (sink_ && !sink_->processRequest(request)) {
    requeue = false;
  }

  puts("");

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
