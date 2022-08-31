#include "sdl_sink.h"
#include "twncm_stdio.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>

#include <libcamera/camera.h>
#include <libcamera/formats.h>

#include "event_loop.h"
#include "image.h"
#include "twincam.h"
#include "twncm_stdio.h"

#ifdef HAVE_LIBJPEG
#include "sdl_texture_mjpg.h"
#endif
#include "sdl_texture_yuyv.h"

using namespace libcamera;

using namespace std::chrono_literals;

SDLSink::SDLSink()
    : window_(nullptr), renderer_(nullptr), rect_({}), init_(false) {}

SDLSink::~SDLSink() {
  stop();
}

int SDLSink::configure(const libcamera::CameraConfiguration& config) {
  int ret = FrameSink::configure(config);
  if (ret < 0)
    return ret;

  if (config.size() > 1) {
    EPRINT(
        "SDL sink only supports one camera stream at present, streaming first "
        "camera stream\n");
  } else if (config.empty()) {
    EPRINT("Require at least one camera stream to process");
    return -EINVAL;
  }

  const libcamera::StreamConfiguration& cfg = config.at(0);
  rect_.w = cfg.size.width;
  rect_.h = cfg.size.height;

  switch (cfg.pixelFormat) {
#ifdef HAVE_LIBJPEG
    case libcamera::formats::MJPEG:
      texture_ = std::make_unique<SDLTextureMJPG>(rect_);
      break;
#endif
    case libcamera::formats::YUYV:
      texture_ = std::make_unique<SDLTextureYUYV>(rect_, cfg.stride);
      break;
    default:
      EPRINT("Unsupported pixel format %s\n",
             cfg.pixelFormat.toString().c_str());
      return -EINVAL;
  };

  return 0;
}

int SDLSink::start() {
  int ret = SDL_Init(SDL_INIT_VIDEO);
  if (ret) {
    EPRINT("Failed to initialize SDL: %s\n", SDL_GetError());
    return ret;
  }

  init_ = true;
  window_ = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, rect_.w, rect_.h,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window_) {
    EPRINT("Failed to create SDL window: %s\n", SDL_GetError());
    return -EINVAL;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, 0);
  if (!renderer_) {
    EPRINT("Failed to create SDL renderer: %s\n", SDL_GetError());
    return -EINVAL;
  }

  /*
   * Set for scaling purposes, not critical, don't return in case of
   * error.
   */
  ret = SDL_RenderSetLogicalSize(renderer_, rect_.w, rect_.h);
  if (ret)
    EPRINT("Failed to set SDL render logical size: %s\n", SDL_GetError());

  ret = texture_->create(renderer_);
  if (ret) {
    return ret;
  }

  SDL_ShowCursor(SDL_DISABLE);

  /* \todo Make the event cancellable to support stop/start cycles. */
  EventLoop::instance()->addTimerEvent(
      10ms, std::bind(&SDLSink::processSDLEvents, this));

  return 0;
}

int SDLSink::stop() {
  texture_.reset();

  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }

  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  if (init_) {
    SDL_Quit();
    init_ = false;
  }

  return FrameSink::stop();
}

void SDLSink::mapBuffer(FrameBuffer* buffer) {
  std::unique_ptr<Image> image =
      Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
  assert(image != nullptr);

  mappedBuffers_[buffer] = std::move(image);
}

bool SDLSink::processRequest(Request* request) {
  for (auto [stream, buffer] : request->buffers()) {
    renderBuffer(buffer);
    break; /* to be expanded to launch SDL window per buffer */
  }

  return true;
}

/*
 * Process SDL events, required for things like window resize and quit button
 */
void SDLSink::processSDLEvents() {
  for (SDL_Event e; SDL_PollEvent(&e);) {
    if (e.type == SDL_QUIT) {
      /* Click close icon then quit */
      EventLoop::instance()->exit(0);
    }
  }
}

void SDLSink::renderBuffer(FrameBuffer* buffer) {
  Image* image = mappedBuffers_[buffer].get();

  /* \todo Implement support for multi-planar formats. */
  const FrameMetadata::Plane& meta = buffer->metadata().planes()[0];

  Span<uint8_t> data = image->data(0);
  if (meta.bytesused > data.size())
    EPRINT("payload size %d larger than plane size %zu\n", meta.bytesused,
           data.size());

  texture_->update(data);

  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_->get(), nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}
