/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021, Ideas on Board Oy
 *
 * kms_sink.cpp - KMS Sink
 */

#include "kms_sink.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <array>
#include <memory>

#include <SDL2/SDL_syswm.h>
#include <libcamera/camera.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer.h>
#include <libcamera/stream.h>

#include <xf86drm.h>

//#include "drm.h"
#include "event_loop.h"
#include "twincam.h"

Device::Device() : fd_(-1) {}

Device::~Device() {
  if (fd_ != -1)
    drmClose(fd_);
}

int Device::init() {
  printf("Device::init()\n");
  constexpr size_t NODE_NAME_MAX = sizeof("/dev/dri/card255");
  char name[NODE_NAME_MAX];
  int ret;

  /*
   * Open the first DRM/KMS device. The libdrm drmOpen*() functions
   * require either a module name or a bus ID, which we don't have, so
   * bypass them. The automatic module loading and device node creation
   * from drmOpen() is of no practical use as any modern system will
   * handle that through udev or an equivalent component.
   */
  snprintf(name, sizeof(name), "/dev/dri/card%u", 0);
  fd_ = open(name, O_RDWR | O_CLOEXEC);
  printf("fd_ = %d\n", fd_);
  if (fd_ < 0) {
    ret = -errno;
    fprintf(stderr, "Failed to open DRM/KMS device %s: %s\n", name,
            strerror(-ret));
    return ret;
  }

  /*
   * Enable the atomic APIs. This also automatically enables the
   * universal planes API.
   */
  ret = drmSetClientCap(fd_, DRM_CLIENT_CAP_ATOMIC, 1);
  if (ret < 0) {
    ret = -errno;
    fprintf(stderr, "Failed to enable atomic capability: %s\n", strerror(-ret));
    return ret;
  }
  EventLoop::instance()->addEvent(fd_, EventLoop::Read,
                                  std::bind(&Device::drmEvent, this));

  return 0;
}

KMSSink::KMSSink(const libcamera::CameraConfiguration& config) {
  printf("KMSSink::KMSSink(const libcamera::CameraConfiguration& config)\n");

  // SDL2 begins
  memset(&sdl_rect, 0, sizeof(sdl_rect));
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
    printf("Could not initialize SDL - %s\n", SDL_GetError());
    return;
  }

  const libcamera::StreamConfiguration& cfg = config.at(0);
  sdl_screen = SDL_CreateWindow(
      "twincam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      cfg.size.width, cfg.size.height,
      SDL_WINDOW_SHOWN /*| SDL_WINDOW_ALLOW_HIGHDPI*/ |
          SDL_WINDOW_RESIZABLE);  // | SDL_WINDOW_FULLSCREEN_DESKTOP);

  printf("2\n");
  if (!sdl_screen) {
    fprintf(stderr, "SDL: could not create window - exiting:%s\n",
            SDL_GetError());
    return;
  }

  /*
    SDL_SysWMinfo info;
    if (!SDL_GetWindowWMInfo(sdl_screen, &info)) {
      fprintf(stderr, "SDL: could not retrieve SDL_SysWMinfo - exiting:%s\n",
    SDL_GetError()); return;
    }

    device_.fd_ = info.info.kmsdrm.drm_fd;
    printf("fd_ = %d\n", device_.fd_);
  */

  int ret = device_.init();
  if (ret < 0)
    return;

  sdl_renderer = SDL_CreateRenderer(
      sdl_screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (sdl_renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer Error\n");
    return;
  }
  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) {
    printf("Warning: Nearest pixel texture filtering not enabled!\n");
  }
  SDL_RenderSetLogicalSize(sdl_renderer, cfg.size.width, cfg.size.height);
  sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_YUY2,
                                  SDL_TEXTUREACCESS_STREAMING, cfg.size.width,
                                  cfg.size.height);
  printf("configured %dx%d\n", cfg.size.width, cfg.size.height);
  device_.requestComplete.connect(this, &KMSSink::requestComplete);
}

void Device::pageFlipComplete([[maybe_unused]] int fd,
                              [[maybe_unused]] unsigned int sequence,
                              [[maybe_unused]] unsigned int tv_sec,
                              [[maybe_unused]] unsigned int tv_usec,
                              void* user_data) {
  printf("Device::pageFlipComplete\n");
  Device* device = static_cast<Device*>(user_data);
  Image img;
  device->requestComplete.emit(&img);
}

void Device::drmEvent() {
  printf("Device::drmEvent()\n");
  drmEventContext ctx{};
  ctx.version = DRM_EVENT_CONTEXT_VERSION;
  ctx.page_flip_handler = &Device::pageFlipComplete;

  drmHandleEvent(fd_, &ctx);
}

void KMSSink::mapBuffer(libcamera::FrameBuffer* buffer) {
  printf("KMSSink::mapBuffer\n");
  std::unique_ptr<Image> image =
      Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
  assert(image != nullptr);

  buffers_[buffer] = std::move(image);
}

int KMSSink::commit(Image* img) {
  printf("commit\n");
  for (unsigned int i = 0; i < img->numPlanes(); ++i) {
    printf("commit buffer %u\n", i);
    SDL_UpdateTexture(sdl_texture, &sdl_rect, img->data(i).data(),
                      sdl_rect.w * 2);
    //  SDL_UpdateYUVTexture
    SDL_RenderClear(sdl_renderer);
    int ret = SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
    if (ret) {
      return ret;
    }

    SDL_RenderPresent(sdl_renderer);
  }
  /*printf("try to sigRequestComplete\n");
  sigRequestComplete.emit(img);
  printf("finish sigRequestComplete\n");*/
  // requestComplete(img);
  // active_ = std::move(queued_);
  // printf("active_ = std::move(queued_) %p\n", (void*)active_.get());
  return 0;
}

int KMSSink::stop() {
  SDL_Quit();

  /* Free all buffers. */
  pending_.reset();
  queued_.reset();
  active_.reset();
  buffers_.clear();

  return 0;
}

bool KMSSink::processRequest(libcamera::Request* camRequest) {
  printf("processRequest\n");
  /*
   * Perform a very crude rate adaptation by simply dropping the request
   * if the display queue is full.
   */
  if (pending_) {
    printf("pending_ %p\n", (void*)pending_.get());
    return true;
  }

  libcamera::FrameBuffer* buffer = camRequest->buffers().begin()->second;
  auto iter = buffers_.find(buffer);
  if (iter == buffers_.end())
    return true;

  Image* imgBuffer = iter->second.get();
  pending_ = std::make_unique<Request>(imgBuffer, camRequest);
  printf("pending_ = std::make_unique<Request>(imgBuffer, camRequest) %p\n",
         (void*)pending_.get());

  std::lock_guard<std::mutex> lock(lock_);
  printf("lock processRequest\n");
  if (!queued_) {
    printf("process commit\n");
    int ret = commit(imgBuffer);
    if (ret < 0) {
      eprintf("Failed to commit image (processRequest): %s\n", strerror(-ret));
    }

    queued_ = std::move(pending_);
    printf("queued_ = std::move(pending_) %p\n", (void*)queued_.get());
  }

  printf("unlock processRequest\n");
  return false;
}

void KMSSink::requestComplete(Image* request) {
  printf("lock requestComplete\n");
  std::lock_guard<std::mutex> lock(lock_);

  assert(queued_ && queued_->imgRequest_.get() == request);

  /* Complete the active request, if any. */
  if (active_)
    requestProcessed.emit(active_->camRequest_);

  /* The queued request becomes active. */
  active_ = std::move(queued_);
  printf("active_ = std::move(queued_) %p\n", (void*)active_.get());

  /* Queue the pending request, if any. */
  if (pending_) {
    printf("pending commit\n");
    int ret = commit(pending_->imgRequest_.get());
    if (ret < 0) {
      eprintf("Failed to commit image (requestComplete): %s\n", strerror(-ret));
    }

    queued_ = std::move(pending_);
    printf("queued_ = std::move(pending_) %p\n", (void*)queued_.get());
  }

  printf("unlock requestComplete\n");
}
