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

#include "file_sink.h"
#ifdef HAVE_DRM
#include "kms_sink.h"
#endif
#include "twincam.h"
#include "uptime.h"

#ifdef HAVE_SDL
#include "sdl_sink.h"
#endif

using namespace libcamera;

CameraSession::CameraSession(const CameraManager* const cm) : cm_(cm) {
  PRINT_FUNC();
}

CameraSession::~CameraSession() {
  if (camera_)
    camera_->release();
}

int CameraSession::init() {
  PRINT_FUNC();
  if (opts.camera < 0) {
    for (size_t i = 0; i < cm_->cameras().size(); ++i) {
      camera_ = cm_->cameras()[i];
      if (!validateConfig())
        return 0;
    }
  } else if (opts.camera < static_cast<int>(cm_->cameras().size())) {
    camera_ = cm_->cameras()[opts.camera];
    if (!validateConfig()) {
      return 0;
    }
  }

  return 1;
}

int CameraSession::validateConfig() {
  PRINT_FUNC();
  if (!camera_) {
    EPRINT("Camera not found\n");
    return 1;
  }

  if (camera_->acquire()) {
    EPRINT("Failed to acquire camera\n");
    return 2;
  }

  std::unique_ptr<CameraConfiguration> cfg =
      camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});
  if (!cfg || cfg->size() != 1) {
    EPRINT("Failed to get default stream configuration\n");
    camera_->release();
    camera_ = nullptr;
    return 3;
  }

  cfg->at(0).pixelFormat = PixelFormat::fromString(opts.pf);

  switch (cfg->validate()) {
    case CameraConfiguration::Valid:
      break;

    case CameraConfiguration::Adjusted:
      break;

    case CameraConfiguration::Invalid:
      EPRINT("Camera configuration invalid\n");
      camera_->release();
      camera_ = nullptr;
      return 4;

    default:
      break;
  }

  config_ = std::move(cfg);

  return 0;
}

int CameraSession::parse_args() {
#ifdef HAVE_SDL
  if (opts.sdl) {
    sink_ = std::make_unique<SDLSink>();
    return 1;
  }
#endif

#ifdef HAVE_DRM
  if (opts.drm) {
    sink_ = std::make_unique<KMSSink>("");
    return 2;
  }
#endif

  if (!opts.filename.empty()) {
    sink_ = std::make_unique<FileSink>(streamNames_, opts.filename);
    return 3;
  }

  return 0;
}

int CameraSession::start() {
  PRINT_FUNC();
  int ret;

  queueCount_ = 0;
  captureCount_ = 0;

  ret = camera_->configure(config_.get());
  if (ret < 0) {
    PRINT("Failed: %d = camera_->configure(config_.get())\n", ret);
    return ret;
  }

  streamNames_.clear();
  for (unsigned int index = 0; index < config_->size(); ++index) {
    const StreamConfiguration& cfg = config_->at(index);
    streamNames_[cfg.stream()] = "cam" + std::to_string(cameraIndex_) +
                                 "-stream" + std::to_string(index);
  }

  camera_->requestCompleted.connect(this, &CameraSession::requestComplete);

  // When the user executes 'twincam' we want the most aesthetically pleasing
  // sink to be used, the advanced users can use command line parameters
  if (!parse_args()) {
#if HAVE_SDL
    sink_ = std::make_unique<SDLSink>();
#elif HAVE_DRM
    sink_ = std::make_unique<KMSSink>("");
#else
    sink_ = std::make_unique<FileSink>(streamNames_, opts.filename);
#endif
  }

  ret = sink_->configure(*config_);
  if (ret < 0) {
    PRINT("Failed to configure frame sink\n");
    return ret;
  }

  sink_->requestProcessed.connect(this, &CameraSession::sinkRelease);

  allocator_ = std::make_unique<FrameBufferAllocator>(camera_);

  return startCapture();
}

void CameraSession::stop() {
  int ret = camera_->stop();
  if (ret)
    PRINT("Failed to stop capture\n");

  if (sink_) {
    ret = sink_->stop();
    if (ret)
      PRINT("Failed to stop frame sink\n");
  }

  sink_.reset();

  requests_.clear();

  allocator_.reset();
}

int CameraSession::startCapture() {
  PRINT_FUNC();
  int ret;

  /* Identify the stream with the least number of buffers. */
  size_t nbuffers = UINT_MAX;
  for (const StreamConfiguration& cfg : *config_) {
    ret = allocator_->allocate(cfg.stream());
    if (ret < 0) {
      EPRINT("Can't allocate buffers\n");
      return -ENOMEM;
    }

#if 0  // not priority right now, for MJPG mainly
    for (const std::unique_ptr<FrameBuffer>& buffer :
         allocator_->buffers(cfg.stream())) {
      /* Map memory buffers and cache the mappings. */
      std::unique_ptr<Image> image =
          Image::fromFrameBuffer(buffer.get(), Image::MapMode::ReadOnly);
      if (!image) {
        EPRINT("Can't allocate image buffers\n");
      }

      mappedBuffers_[buffer.get()] = std::move(image);
    }
#endif

    size_t allocated = allocator_->buffers(cfg.stream()).size();
    nbuffers = std::min(nbuffers, allocated);
  }

  /*
   * TODO: make cam tool smarter to support still capture by for
   * example pushing a button. For now run all streams all the time.
   */

  for (unsigned int i = 0; i < nbuffers; i++) {
    std::unique_ptr<Request> request = camera_->createRequest();
    if (!request) {
      EPRINT("Can't create request\n");
      return -ENOMEM;
    }

    for (const StreamConfiguration& cfg : *config_) {
      Stream* stream = cfg.stream();
      const std::vector<std::unique_ptr<FrameBuffer>>& buffers =
          allocator_->buffers(stream);
      const std::unique_ptr<FrameBuffer>& buffer = buffers[i];

      ret = request->addBuffer(stream, buffer.get());
      if (ret < 0) {
        EPRINT("Can't set buffer for request\n");
        return ret;
      }

      if (sink_)
        sink_->mapBuffer(buffer.get());
    }

    VERBOSE_PRINT("Pushing request onto vector\n");
    requests_.push_back(std::move(request));
  }

  if (sink_) {
    ret = sink_->start();
    if (ret) {
      PRINT("Failed to start frame sink\n");
      return ret;
    }
  }

  VERBOSE_PRINT("Buffers mapped... Starting camera\n");
  ret = camera_->start();
  if (ret) {
    PRINT("Failed to start capture\n");
    if (sink_)
      sink_->stop();
    return ret;
  }

  VERBOSE_PRINT("Capture started\n");
  for (const std::unique_ptr<Request>& request : requests_) {
    ret = queueRequest(request.get());
    if (ret < 0) {
      EPRINT("Can't queue request\n");
      camera_->stop();
      if (sink_)
        sink_->stop();
      return ret;
    }
  }

  PRINT("cam%u: Capture until user interrupts by SIGINT\n", cameraIndex_);

  return 0;
}

int CameraSession::queueRequest(Request* request) {
  ++queueCount_;

  return camera_->queueRequest(request);
}

void CameraSession::requestComplete(Request* request) {
  if (request->status() == Request::RequestCancelled)
    return;

  /*
   * Defer processing of the completed request to the event loop, to avoid
   * blocking the camera manager thread.
   */
  EventLoop::instance()->callLater(
      [request, this]() { processRequest(request); });
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

  std::string frame_str;
  STRING_PRINTF(frame_str, "(%.2f fps)", fps);
  for (const std::pair<const libcamera::Stream* const, libcamera::FrameBuffer*>&
           buf : buffers) {
    const FrameMetadata& metadata = buf.second->metadata();

    frame_str += " " + streamNames_[buf.first] +
                 " seq: " + std::to_string(metadata.sequence) + " bytesused: ";

    unsigned int nplane = 0;
    for (const FrameMetadata::Plane& plane : metadata.planes()) {
      frame_str += std::to_string(plane.bytesused);
      if (++nplane < metadata.planes().size())
        frame_str += "/";
    }

#if 0  // not priority right now, for MJPG mainly
    Image* image = mappedBuffers_[buffer].get();
    converter_.convert(image, size, &image_);
#endif
  }

  if (sink_ && !sink_->processRequest(request)) {
    requeue = false;
  }

  PRINT("%s\n", frame_str.c_str());

  /*
   * Notify the user that capture is complete if the limit has just been
   * reached.
   */
  ++captureCount_;

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
