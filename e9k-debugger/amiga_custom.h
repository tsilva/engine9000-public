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

int
amiga_custom_init(void);

void
amiga_custom_shutdown(void);

void
amiga_custom_toggle(void);

int
amiga_custom_isOpen(void);

void
amiga_custom_render(void);

void
amiga_custom_persistConfig(FILE *file);

int
amiga_custom_loadConfigProperty(const char *prop, const char *value);
