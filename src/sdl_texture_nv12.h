#pragma once

#include "sdl_texture.h"

#if SDL_VERSION_ATLEAST(2, 0, 16)
class SDLTextureNV12 : public SDLTexture {
 public:
  SDLTextureNV12(const SDL_Rect& rect, unsigned int stride);
  void update(const std::vector<libcamera::Span<const uint8_t>>& data) override;
};
#endif
