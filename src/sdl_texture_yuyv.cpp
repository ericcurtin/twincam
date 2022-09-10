#include "sdl_texture_yuyv.h"

using namespace libcamera;

SDLTextureYUYV::SDLTextureYUYV(const SDL_Rect& rect, unsigned int stride)
    : SDLTexture(rect, SDL_PIXELFORMAT_YUY2, stride) {}

void SDLTextureYUYV::update(const std::vector<libcamera::Span<const uint8_t>>& data) {
    SDL_UpdateTexture(ptr_, &rect_, data[0].data(), pitch_);
}
