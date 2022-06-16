/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * file_sink.h - File Sink
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include <libcamera/stream.h>

#include "frame_sink.h"

class Image;

class FileSink : public FrameSink {
 public:
  FileSink(const std::map<const libcamera::Stream*, std::string>& streamNames,
           const std::string& filename = "/dev/null");
  ~FileSink();

  int configure(const libcamera::CameraConfiguration& config) override;

  void mapBuffer(libcamera::FrameBuffer* buffer) override;

  bool processRequest(libcamera::Request* request) override;

 private:
  void writeBuffer(libcamera::FrameBuffer* buffer);

  std::map<const libcamera::Stream*, std::string> streamNames_;
  std::string filename_;
  std::map<libcamera::FrameBuffer*, std::unique_ptr<Image>> mappedBuffers_;
};
