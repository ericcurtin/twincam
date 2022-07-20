#pragma once

#include "sdl_texture.h"

class SDLTextureMJPG : public SDLTexture {
 public:
  SDLTextureMJPG(const SDL_Rect& rect);

  void update(const libcamera::Span<uint8_t>& data) override;

 private:
  int decompress(const libcamera::Span<uint8_t>& data);

  std::unique_ptr<unsigned char[]> rgb_;
};
