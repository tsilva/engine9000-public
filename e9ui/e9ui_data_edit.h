/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include "e9ui_component.h"
#include "e9ui_context.h"

typedef enum e9ui_data_edit_mode {
    e9ui_data_edit_mode_hex_bytes = 0,
    e9ui_data_edit_mode_hex_words16,
    e9ui_data_edit_mode_ascii_fixed
} e9ui_data_edit_mode_t;

typedef void (*e9ui_data_edit_submit_cb_t)(e9ui_context_t *ctx, void *user);
typedef int (*e9ui_data_edit_key_cb_t)(e9ui_context_t *ctx, SDL_Keycode key, SDL_Keymod mods, void *user);

e9ui_component_t *
e9ui_data_edit_make(int cellCount, e9ui_data_edit_submit_cb_t onSubmit, void *user);

void
e9ui_data_edit_setCellCount(e9ui_component_t *comp, int cellCount);

int
e9ui_data_edit_getCellCount(const e9ui_component_t *comp);

void
e9ui_data_edit_setMode(e9ui_component_t *comp, e9ui_data_edit_mode_t mode);

e9ui_data_edit_mode_t
e9ui_data_edit_getMode(const e9ui_component_t *comp);

void
e9ui_data_edit_setText(e9ui_component_t *comp, const char *text);

const char *
e9ui_data_edit_getText(const e9ui_component_t *comp);

void
e9ui_data_edit_setCursor(e9ui_component_t *comp, int cursor);

int
e9ui_data_edit_getCursor(const e9ui_component_t *comp);

void
e9ui_data_edit_selectAllExternal(e9ui_component_t *comp);

void
e9ui_data_edit_setKeyHandler(e9ui_component_t *comp, e9ui_data_edit_key_cb_t cb, void *user);
