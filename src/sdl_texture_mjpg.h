#pragma once

#include "sdl_texture.h"

class SDLTextureMJPG : public SDLTexture {
 public:
  SDLTextureMJPG(const SDL_Rect& rect);
  virtual ~SDLTextureMJPG();
  void update(const libcamera::Span<uint8_t>& data) override;

 private:
  unsigned char* rgbdata_ = 0;
  void read_JPEG_file(const unsigned char* jpegData, unsigned long jpegsize);
};
