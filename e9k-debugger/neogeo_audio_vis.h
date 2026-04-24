/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <SDL.h>
#include <stdio.h>

#include "e9k-geo.h"

void
neogeo_audio_vis_toggle(void);

int
neogeo_audio_vis_isOpen(void);

void
neogeo_audio_vis_render(const e9k_debug_audio_frame_t *frame);

int
neogeo_audio_vis_handleKeydown(const SDL_KeyboardEvent *kev);

void
neogeo_audio_vis_setMainWindowFocused(int focused);

void
neogeo_audio_vis_persistConfig(FILE *file);

int
neogeo_audio_vis_loadConfigProperty(const char *prop, const char *value);
