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

int
shader_ui_init(void);

void
shader_ui_shutdown(void);

int
shader_ui_isOpen(void);

void
shader_ui_render(void);

void
shader_ui_persistConfig(FILE *file);

int
shader_ui_loadConfigProperty(const char *prop, const char *value);
