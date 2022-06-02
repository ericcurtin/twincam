/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022, Ideas on Board Oy
 *
 * sdl_texture_yuyv.h - SDL Texture YUYV
 */

#pragma once

#include "sdl_texture.h"

class SDLTextureYUYV : public SDLTexture
{
public:
	SDLTextureYUYV(const SDL_Rect &rect);
	void update(const libcamera::Span<uint8_t> &data) override;
};