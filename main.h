/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * main.h - Cam application
 */

#pragma once

enum {
  OptCamera = 'c',
  OptCapture = 'C',
  OptDisplay = 'D',
  OptHelp = 'h',
  OptInfo = 'I',
  OptList = 'l',
  OptListProperties = 'p',
  OptMonitor = 'm',
  OptStream = 's',
  OptListControls = 256,
  OptStrictFormats = 257,
  OptMetadata = 258,
};

#define eprintf(...) do fprintf(stderr, __VA_ARGS__); while (0)
