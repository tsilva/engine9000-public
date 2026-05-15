/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stdio.h>

void
mega_memview_toggle(void);

int
mega_memview_isOpen(void);

void
mega_memview_render(void);

void
mega_memview_persistConfig(FILE *file);

int
mega_memview_loadConfigProperty(const char *prop, const char *value);
