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

#include "e9k-mega.h"

void
mega_sprite_debug_toggle(void);

int
mega_sprite_debug_is_open(void);

void
mega_sprite_debug_render(const e9k_debug_mega_sprite_state_t *st);

void
mega_sprite_debug_persistConfig(FILE *file);

int
mega_sprite_debug_loadConfigProperty(const char *prop, const char *value);
