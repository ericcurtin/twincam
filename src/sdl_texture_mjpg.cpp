#include "sdl_texture_mjpg.h"
#include "twncm_stdio.h"
#include "jpeg_error_manager.h"

#include <jpeglib.h>

using namespace libcamera;

SDLTextureMJPG::SDLTextureMJPG(const SDL_Rect& rect)
    : SDLTexture(rect, SDL_PIXELFORMAT_RGB24, rect.w * 3),
      rgb_(std::make_unique<unsigned char[]>(pitch_ * rect.h)) {}

int SDLTextureMJPG::decompress(const Span<uint8_t>& data) {
  struct jpeg_decompress_struct cinfo;
  JpegErrorManager jpegErrorManager(cinfo);
  if (setjmp(jpegErrorManager.escape_)) {
    /* libjpeg found an error */
    jpeg_destroy_decompress(&cinfo);
    EPRINT("JPEG decompression error\n");
    return -EINVAL;
  }

  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, data.data(), data.size());

  jpeg_read_header(&cinfo, TRUE);

  jpeg_start_decompress(&cinfo);

  for (int i = 0; cinfo.output_scanline < cinfo.output_height; ++i) {
    JSAMPROW rowptr = rgb_.get() + i * pitch_;
    jpeg_read_scanlines(&cinfo, &rowptr, 1);
  }

  jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  return 0;
}

void SDLTextureMJPG::update(const Span<uint8_t>& data) {
  decompress(data);
  SDL_UpdateTexture(ptr_, nullptr, rgb_.get(), pitch_);
}
