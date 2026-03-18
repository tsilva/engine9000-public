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
custom_ui_init(void);

void
custom_ui_shutdown(void);

void
custom_ui_toggle(void);

int
custom_ui_isOpen(void);

int
custom_ui_getBlitterVisDecay(void);

int
custom_ui_getEstimateFpsEnabled(void);

int
custom_ui_getCopperLimitEnabled(void);

int
custom_ui_getCopperLimitRange(int *outStart, int *outEnd);

void
custom_ui_setCopperLimitRange(int start, int end);

int
custom_ui_getBplptrBlockEnabled(void);

int
custom_ui_getBplptrLineLimitRange(int *outStart, int *outEnd);

void
custom_ui_setBplptrLineLimitRange(int start, int end);

void
custom_ui_setMainWindowFocused(int focused);

void
custom_ui_render(void);

void
custom_ui_persistConfig(FILE *file);

int
custom_ui_loadConfigProperty(const char *prop, const char *value);
