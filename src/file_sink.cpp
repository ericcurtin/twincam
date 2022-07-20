/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * file_sink.cpp - File Sink
 */

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>

#include <libcamera/camera.h>

#include "file_sink.h"
#include "image.h"
#include "twncm_stdio.h"

using namespace libcamera;

FileSink::FileSink(
    const std::map<const libcamera::Stream*, std::string>& streamNames,
    const std::string& filename)
    : streamNames_(streamNames), filename_(filename) {}

FileSink::~FileSink() {}

int FileSink::configure(const libcamera::CameraConfiguration& config) {
  int ret = FrameSink::configure(config);
  if (ret < 0)
    return ret;

  return 0;
}

void FileSink::mapBuffer(FrameBuffer* buffer) {
  std::unique_ptr<Image> image =
      Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
  assert(image != nullptr);

  mappedBuffers_[buffer] = std::move(image);
}

bool FileSink::processRequest(Request* request) {
  for (auto [stream, buffer] : request->buffers())
    writeBuffer(buffer);

  return true;
}

void FileSink::writeBuffer(FrameBuffer* buffer) {
  int ret = 0;

  int fd = open(filename_.c_str(), O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (fd == -1) {
    ret = -errno;
    EPRINT("failed to open file %s: %s\n", filename_.c_str(), strerror(-ret));
    return;
  }

  Image* image = mappedBuffers_[buffer].get();

  for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
    const FrameMetadata::Plane& meta = buffer->metadata().planes()[i];

    Span<uint8_t> data = image->data(i);
    unsigned int length = std::min<unsigned int>(meta.bytesused, data.size());

    if (meta.bytesused > data.size())
      EPRINT("payload size %d larger than plane size %lu\n", meta.bytesused,
             data.size());

    ret = ::write(fd, data.data(), length);
    if (ret < 0) {
      ret = -errno;
      EPRINT("write error: %s\n", strerror(-ret));
      break;
    } else if (ret != static_cast<int>(length)) {
      EPRINT("write error: only %d bytes written instead of %d\n", ret, length);
      break;
    }
  }

  close(fd);
}
