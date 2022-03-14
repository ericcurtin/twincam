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

#include <SDL2/SDL.h>
#include <libcamera/camera.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>

#include "image.h"

class Device {
 public:
  Device();
  ~Device();

  int init();

  int fd_;

  libcamera::Signal<Image*> requestComplete;

 private:
  void drmEvent();
  static void pageFlipComplete(int fd,
                               unsigned int sequence,
                               unsigned int tv_sec,
                               unsigned int tv_usec,
                               void* user_data);
};

class KMSSink {
 public:
  KMSSink(const libcamera::CameraConfiguration& config);
  void mapBuffer(libcamera::FrameBuffer* buffer);
  int commit(Image* img);
  int stop();

  bool processRequest(libcamera::Request* request);

  libcamera::Signal<libcamera::Request*> requestProcessed;

 private:
  class Request {
   public:
    Request(Image* imgRequest, libcamera::Request* camRequest)
        : imgRequest_(imgRequest), camRequest_(camRequest) {}

    std::unique_ptr<Image> imgRequest_;
    libcamera::Request* camRequest_;
  };

  int configurePipeline(const libcamera::PixelFormat& format);
  void requestComplete(Image* request);

  Device device_;

  libcamera::Signal<Image*> sigRequestComplete;
  libcamera::PixelFormat format_;
  libcamera::Size size_;
  unsigned int stride_;

  SDL_Window* sdl_screen;
  SDL_Renderer* sdl_renderer;
  SDL_Texture* sdl_texture;
  SDL_Rect sdl_rect;

  std::mutex lock_;
  std::unique_ptr<Request> pending_;
  std::unique_ptr<Request> queued_;
  std::unique_ptr<Request> active_;
  std::map<libcamera::FrameBuffer*, std::unique_ptr<Image>> buffers_;
};
