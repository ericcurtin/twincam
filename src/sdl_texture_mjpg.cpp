#include "sdl_texture_mjpg.h"

#include <jpeglib.h>

using namespace libcamera;

SDLTextureMJPG::SDLTextureMJPG(const SDL_Rect& rect)
    : SDLTexture(rect, SDL_PIXELFORMAT_RGB24, 0) {
  pitch_ = rect_.w * 3;
  rgbdata_ = (unsigned char*)malloc(pitch_ * rect_.h);
}

SDLTextureMJPG::~SDLTextureMJPG() {
  if (rgbdata_) {
    free(rgbdata_);
  }
}

void SDLTextureMJPG::read_JPEG_file(const unsigned char* jpegData,
                                    unsigned long jpegsize) {
  struct jpeg_error_mgr jerr;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, jpegData, jpegsize);

  (void)jpeg_read_header(&cinfo, TRUE);

  (void)jpeg_start_decompress(&cinfo);

  int row_stride = cinfo.output_width * cinfo.output_components;
  unsigned char* buffer = (unsigned char*)malloc(row_stride);
  for (int i = 0; cinfo.output_scanline < cinfo.output_height; ++i) {
    (void)jpeg_read_scanlines(&cinfo, &buffer, 1);
    memcpy(rgbdata_ + i * pitch_, buffer, row_stride);
  }

  free(buffer);
  (void)jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);
}

void SDLTextureMJPG::update(const Span<uint8_t>& data) {
  read_JPEG_file(data.data(), data.size());
  SDL_UpdateTexture(ptr_, nullptr, rgbdata_, pitch_);
}
