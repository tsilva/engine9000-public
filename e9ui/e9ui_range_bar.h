/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stddef.h>

#include "e9ui_component.h"
#include "e9ui_context.h"

typedef enum e9ui_range_bar_side
{
    e9ui_range_bar_sideLeft = 0,
    e9ui_range_bar_sideRight = 1
} e9ui_range_bar_side_t;

typedef void (*e9ui_range_bar_change_cb_t)(float startPercent, float endPercent, void *user);
typedef void (*e9ui_range_bar_drag_cb_t)(int dragging, float startPercent, float endPercent, void *user);
typedef void (*e9ui_range_bar_tooltip_cb_t)(float startPercent, float endPercent, char *out, size_t cap, void *user);

e9ui_component_t *
e9ui_range_bar_make(void);

void
e9ui_range_bar_setSide(e9ui_component_t *comp, e9ui_range_bar_side_t side);

void
e9ui_range_bar_setMargins(e9ui_component_t *comp, int top, int bottom, int side);

void
e9ui_range_bar_setWidth(e9ui_component_t *comp, int width);

void
e9ui_range_bar_setHoverMargin(e9ui_component_t *comp, int margin);

int
e9ui_range_bar_getHoverMargin(e9ui_component_t *comp);

void
e9ui_range_bar_setCallback(e9ui_component_t *comp, e9ui_range_bar_change_cb_t cb, void *user);

void
e9ui_range_bar_setDragCallback(e9ui_component_t *comp, e9ui_range_bar_drag_cb_t cb, void *user);

void
e9ui_range_bar_setTooltipCallback(e9ui_component_t *comp, e9ui_range_bar_tooltip_cb_t cb, void *user);

void
e9ui_range_bar_setRangePercent(e9ui_component_t *comp, float startPercent, float endPercent);

int
e9ui_range_bar_isDragging(e9ui_component_t *comp);

void
e9ui_range_bar_layoutInParent(e9ui_component_t *comp, e9ui_context_t *ctx, e9ui_rect_t parent);
