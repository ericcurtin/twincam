#pragma once

#include "sdl_texture.h"

class SDLTextureYUYV : public SDLTexture {
 public:
  SDLTextureYUYV(const SDL_Rect& rect, unsigned int stride);
  void update(const std::vector<libcamera::Span<const uint8_t>>& data) override;
};
