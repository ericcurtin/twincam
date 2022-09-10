#include "sdl_texture_nv12.h"

using namespace libcamera;

SDLTextureNV12::SDLTextureNV12(const SDL_Rect& rect, unsigned int stride)
    : SDLTexture(rect, SDL_PIXELFORMAT_NV12, stride) {}

void
SDLTextureNV12::update(const std::vector<libcamera::Span<const uint8_t>>& data)
{
  SDL_UpdateNVTexture(ptr_, &rect_, data[0].data(), pitch_,
    data[1].data(), pitch_);
}
