/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sdl_sink.h - SDL Sink
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include <libcamera/camera.h>
#include <libcamera/stream.h>

#include <SDL2/SDL.h>

class Image;

class SDLSink {
 public:
  SDLSink();
  ~SDLSink();

  int configure(const libcamera::CameraConfiguration& config);
  int start();
  int stop();
  void mapBuffer(libcamera::FrameBuffer* buffer);

  bool processRequest(libcamera::Request* request);

  libcamera::Signal<libcamera::Request*> requestProcessed;

 private:
  void processSDLEvents();
  void updateTexture(libcamera::FrameBuffer* buffer);
  void renderBuffer(libcamera::FrameBuffer* buffer);

  std::map<libcamera::FrameBuffer*, std::unique_ptr<Image>> mappedBuffers_;

  SDL_Window* sdlWindow_;
  SDL_Renderer* sdlRenderer_;
  SDL_Texture* sdlTexture_;
  SDL_Rect sdlRect_;
  SDL_PixelFormatEnum pixelFormat_;
  bool sdlInit_;
  int pitch_;
};
