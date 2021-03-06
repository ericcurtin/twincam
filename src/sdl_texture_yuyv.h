#pragma once

#include "sdl_texture.h"

class SDLTextureYUYV : public SDLTexture {
 public:
  SDLTextureYUYV(const SDL_Rect& rect);
  void update(const libcamera::Span<uint8_t>& data) override;
};
