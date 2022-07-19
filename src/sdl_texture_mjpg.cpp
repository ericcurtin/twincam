#include "sdl_texture_mjpg.h"
#include "jpeg_error_manager.h"

#include <jpeglib.h>

using namespace libcamera;

SDLTextureMJPG::SDLTextureMJPG(const SDL_Rect& rect)
    : SDLTexture(rect, SDL_PIXELFORMAT_RGB24, 0) {
  pitch_ = rect_.w * 3;
  rgbdata_ = (unsigned char*)malloc(pitch_ * rect_.h);
}

SDLTextureMJPG::~SDLTextureMJPG() {
  free(rgbdata_);
}

int SDLTextureMJPG::decompress(const Span<uint8_t>& data) {
  struct jpeg_error_mgr jerr;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr);

  JpegErrorManager jpegErrorManager(cinfo);
  if (setjmp(jpegErrorManager.escape_)) {
    /* libjpeg found an error */
    jpeg_destroy_decompress(&cinfo);

    EPRINT("JPEG decompression error");
    return -EINVAL;
  }

  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, data.data(), data.size());

  jpeg_read_header(&cinfo, TRUE);

  jpeg_start_decompress(&cinfo);

  int row_stride = cinfo.output_width * cinfo.output_components;
  unsigned char* buffer = (unsigned char*)malloc(row_stride);
  for (int i = 0; cinfo.output_scanline < cinfo.output_height; ++i) {
    jpeg_read_scanlines(&cinfo, &buffer, 1);
    memcpy(rgbdata_ + i * pitch_, buffer, row_stride);
  }

  free(buffer);
  jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  return 0;
}

void SDLTextureMJPG::update(const Span<uint8_t>& data) {
  decompress(data);
  SDL_UpdateTexture(ptr_, nullptr, rgbdata_, pitch_);
}
