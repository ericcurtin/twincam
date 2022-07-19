/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022, Ideas on Board Oy
 *
 * jpeg_error_manager.h - JPEG Error Manager
 */

#pragma once

#include <setjmp.h>
#include "twncm_stdio.h"

#include <jpeglib.h>

struct JpegErrorManager {
  JpegErrorManager(struct jpeg_decompress_struct& cinfo);

  /* Order very important for reinterpret_cast */
  struct jpeg_error_mgr errmgr_;
  jmp_buf escape_;
};