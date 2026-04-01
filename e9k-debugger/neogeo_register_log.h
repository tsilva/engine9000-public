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
#include <stdint.h>

#include "e9k-lib.h"

int
neogeo_register_log_init(void);

void
neogeo_register_log_shutdown(void);

void
neogeo_register_log_toggle(void);

int
neogeo_register_log_isOpen(void);

void
neogeo_register_log_setMainWindowFocused(int focused);

void
neogeo_register_log_render(void);

void
neogeo_register_log_persistConfig(FILE *file);

int
neogeo_register_log_loadConfigProperty(const char *prop, const char *value);

void
neogeo_register_log_captureFrame(const e9k_debug_geo_register_log_entry_t *entries,
                        size_t count,
                        uint32_t dropped,
                        uint64_t frameNo);
