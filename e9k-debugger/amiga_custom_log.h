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
amiga_custom_log_init(void);

void
amiga_custom_log_shutdown(void);

void
amiga_custom_log_toggle(void);

int
amiga_custom_log_isOpen(void);

void
amiga_custom_log_setMainWindowFocused(int focused);

void
amiga_custom_log_render(void);

void
amiga_custom_log_persistConfig(FILE *file);

int
amiga_custom_log_loadConfigProperty(const char *prop, const char *value);

void
amiga_custom_log_captureFrame(const e9k_debug_ami_custom_log_entry_t *entries,
                        size_t count,
                        uint32_t dropped,
                        uint64_t frameNo);
