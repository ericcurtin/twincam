#pragma once

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

struct JpegErrorManager {
  JpegErrorManager(struct jpeg_decompress_struct& cinfo);

  /* Order very important for reinterpret_cast */
  struct jpeg_error_mgr errmgr_;
  jmp_buf escape_;
};

