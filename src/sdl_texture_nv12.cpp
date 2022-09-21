#include "sdl_texture_nv12.h"

using namespace libcamera;

#if SDL_VERSION_ATLEAST(2, 0, 16)
SDLTextureNV12::SDLTextureNV12(const SDL_Rect& rect, unsigned int stride)
    : SDLTexture(rect, SDL_PIXELFORMAT_NV12, stride) {}

void SDLTextureNV12::update(
    const std::vector<libcamera::Span<const uint8_t>>& data) {
  SDL_UpdateNVTexture(ptr_, &rect_, data[0].data(), stride_, data[1].data(),
                      stride_);
}
#endif
