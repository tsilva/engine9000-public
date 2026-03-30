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
neogeo_sprite_debug_toggle(void);

int
neogeo_sprite_debug_is_open(void);

void
neogeo_sprite_debug_render(const e9k_debug_sprite_state_t *st);

int
neogeo_sprite_debug_handleKeydown(const SDL_KeyboardEvent *kev);

void
neogeo_sprite_debug_setMainWindowFocused(int focused);

void
neogeo_sprite_debug_persistConfig(FILE *file);

int
neogeo_sprite_debug_loadConfigProperty(const char *prop, const char *value);
