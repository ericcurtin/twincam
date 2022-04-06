/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sdl_sink.cpp - SDL Sink
 */

#include "sdl_sink.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <libcamera/camera.h>
#include <libcamera/formats.h>

#include "event_loop.h"
#include "image.h"

using namespace libcamera;

SDLSink::SDLSink()
    : sdlWindow_(nullptr),
      sdlRenderer_(nullptr),
      sdlTexture_(nullptr),
      sdlRect_({}),
      sdlInit_(false) {}

SDLSink::~SDLSink() {
  stop();
}

int SDLSink::configure(const libcamera::CameraConfiguration& cfg) {
  if (cfg.size() > 1) {
    std::cerr << "SDL sink only supports one camera stream at present, "
                 "streaming first camera stream"
              << std::endl;
  } else if (cfg.empty()) {
    std::cerr << "Require at least one camera stream to process" << std::endl;
    return -EINVAL;
  }

  const libcamera::StreamConfiguration& sCfg = cfg.at(0);
  sdlRect_.w = sCfg.size.width;
  sdlRect_.h = sCfg.size.height;
  switch (sCfg.pixelFormat) {
    case libcamera::formats::YUYV:
      pixelFormat_ = SDL_PIXELFORMAT_YUY2;
      pitch_ = 4 * ((sdlRect_.w + 1) / 2);
      break;

    /* From here down the fourcc values are identical between SDL, drm,
     * libcamera */
    case libcamera::formats::NV21:
      pixelFormat_ = SDL_PIXELFORMAT_NV21;
      pitch_ = sdlRect_.w;
      break;
    case libcamera::formats::NV12:
      pixelFormat_ = SDL_PIXELFORMAT_NV12;
      pitch_ = sdlRect_.w;
      break;
    case libcamera::formats::YVU420:
      pixelFormat_ = SDL_PIXELFORMAT_YV12;
      pitch_ = sdlRect_.w;
      break;
    case libcamera::formats::YVYU:
      pixelFormat_ = SDL_PIXELFORMAT_YVYU;
      pitch_ = 4 * ((sdlRect_.w + 1) / 2);
      break;
    case libcamera::formats::UYVY:
      pixelFormat_ = SDL_PIXELFORMAT_UYVY;
      pitch_ = 4 * ((sdlRect_.w + 1) / 2);
      break;
    default:
      std::cerr << sCfg.pixelFormat.toString()
                << " not present in libcamera <-> SDL map" << std::endl;
      return -EINVAL;
  };

  return 0;
}

int SDLSink::start() {
  int ret = SDL_Init(SDL_INIT_VIDEO);
  if (ret) {
    std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return ret;
  }

  sdlInit_ = true;
  sdlWindow_ = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED, sdlRect_.w, sdlRect_.h,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!sdlWindow_) {
    std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
    return -EINVAL;
  }

  sdlRenderer_ = SDL_CreateRenderer(sdlWindow_, -1, 0);
  if (!sdlRenderer_) {
    std::cerr << "Failed to create SDL renderer: " << SDL_GetError()
              << std::endl;
    return -EINVAL;
  }

  ret = SDL_RenderSetLogicalSize(sdlRenderer_, sdlRect_.w, sdlRect_.h);
  if (ret) { /* Not critical to set, don't return in this case, set for scaling
                purposes */
    std::cerr << "Failed to set SDL render logical size: " << SDL_GetError()
              << std::endl;
  }

  sdlTexture_ =
      SDL_CreateTexture(sdlRenderer_, pixelFormat_, SDL_TEXTUREACCESS_STREAMING,
                        sdlRect_.w, sdlRect_.h);
  if (!sdlTexture_) {
    std::cerr << "Failed to create SDL texture: " << SDL_GetError()
              << std::endl;
    return -EINVAL;
  }

  EventLoop::instance()->addEvent(-1, EventLoop::Default,
                                  std::bind(&SDLSink::processSDLEvents, this));

  return 0;
}

int SDLSink::stop() {
  if (sdlTexture_) {
    SDL_DestroyTexture(sdlTexture_);
    sdlTexture_ = nullptr;
  }

  if (sdlRenderer_) {
    SDL_DestroyRenderer(sdlRenderer_);
    sdlRenderer_ = nullptr;
  }

  if (sdlWindow_) {
    SDL_DestroyWindow(sdlWindow_);
    sdlWindow_ = nullptr;
  }

  if (sdlInit_) {
    SDL_Quit();
    sdlInit_ = false;
  }

  return 0;
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
    if (e.type == SDL_QUIT) { /* click close icon then quit */
      EventLoop::instance()->exit(0);
    }
  }
}

void SDLSink::renderBuffer(FrameBuffer* buffer) {
  Image* image = mappedBuffers_[buffer].get();

  for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
    const FrameMetadata::Plane& meta = buffer->metadata().planes()[i];

    Span<uint8_t> data = image->data(i);
    if (meta.bytesused > data.size())
      std::cerr << "payload size " << meta.bytesused
                << " larger than plane size " << data.size() << std::endl;

    SDL_UpdateTexture(sdlTexture_, &sdlRect_, data.data(), pitch_);
    SDL_RenderClear(sdlRenderer_);
    SDL_RenderCopy(sdlRenderer_, sdlTexture_, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer_);
  }
}
