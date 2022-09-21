#include "sdl_texture.h"
#include "twincam.h"
#include "twncm_stdio.h"

SDLTexture::SDLTexture(const SDL_Rect& rect,
                       SDL_PixelFormatEnum pixelFormat,
                       const int stride)
    : ptr_(nullptr), rect_(rect), pixelFormat_(pixelFormat), stride_(stride) {}

SDLTexture::~SDLTexture() {
  if (ptr_)
    SDL_DestroyTexture(ptr_);
}

int SDLTexture::create(SDL_Renderer* renderer) {
  ptr_ = SDL_CreateTexture(renderer, pixelFormat_, SDL_TEXTUREACCESS_STREAMING,
                           rect_.w, rect_.h);
  if (!ptr_) {
    EPRINT("Failed to create SDL texture: %s\n", SDL_GetError());
    return -ENOMEM;
  }

  return 0;
}
