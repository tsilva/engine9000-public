/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "e9ui.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>

#include "alloc.h"
#include "e9ui_text_cache.h"

typedef struct e9ui_range_bar_state {
    float startPercent;
    float endPercent;
    int dragging;
    int draggingHandle;
    int hoverThumb;
    int marginTop;
    int marginBottom;
    int marginSide;
    int width;
    int hoverMargin;
    e9ui_range_bar_side_t side;
    e9ui_range_bar_change_cb_t cb;
    void *cbUser;
    e9ui_range_bar_drag_cb_t dragCb;
    void *dragUser;
    e9ui_range_bar_tooltip_cb_t tooltipCb;
    void *tooltipUser;
} e9ui_range_bar_state_t;

static SDL_Cursor *e9ui_range_bar_cursorArrow = NULL;
static SDL_Cursor *e9ui_range_bar_cursorNs = NULL;

enum
{
    E9UI_RANGE_BAR_HANDLE_NONE = 0,
    E9UI_RANGE_BAR_HANDLE_START = 1,
    E9UI_RANGE_BAR_HANDLE_END = 2
};

static void
e9ui_range_bar_ensureCursors(void)
{
    if (!e9ui_range_bar_cursorArrow) {
        e9ui_range_bar_cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    }
    if (!e9ui_range_bar_cursorNs) {
        e9ui_range_bar_cursorNs = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    }
}

static float
e9ui_range_bar_clampPercent(float percent)
{
    if (percent < 0.0f) {
        return 0.0f;
    }
    if (percent > 1.0f) {
        return 1.0f;
    }
    return percent;
}

static int
e9ui_range_bar_percentToY(const e9ui_rect_t *bounds, float percent)
{
    if (!bounds || bounds->h <= 1) {
        return bounds ? bounds->y : 0;
    }
    float p = e9ui_range_bar_clampPercent(percent);
    return bounds->y + (int)(p * (float)(bounds->h - 1) + 0.5f);
}

static float
e9ui_range_bar_yToPercent(const e9ui_rect_t *bounds, int y)
{
    if (!bounds || bounds->h <= 1) {
        return 0.0f;
    }
    float percent = (float)(y - bounds->y) / (float)(bounds->h - 1);
    return e9ui_range_bar_clampPercent(percent);
}

static void
e9ui_range_bar_emitChange(e9ui_range_bar_state_t *state)
{
    if (!state || !state->cb) {
        return;
    }
    state->cb(state->startPercent, state->endPercent, state->cbUser);
}

static void
e9ui_range_bar_emitDrag(e9ui_range_bar_state_t *state, int dragging)
{
    if (!state || !state->dragCb) {
        return;
    }
    state->dragCb(dragging ? 1 : 0, state->startPercent, state->endPercent, state->dragUser);
}

static int
e9ui_range_bar_pickHandle(const e9ui_rect_t *bounds, float startPercent, float endPercent, int y)
{
    int startY = e9ui_range_bar_percentToY(bounds, startPercent);
    int endY = e9ui_range_bar_percentToY(bounds, endPercent);
    int startDistance = y - startY;
    int endDistance = y - endY;
    if (startDistance < 0) {
        startDistance = -startDistance;
    }
    if (endDistance < 0) {
        endDistance = -endDistance;
    }
    if (startDistance <= endDistance) {
        return E9UI_RANGE_BAR_HANDLE_START;
    }
    return E9UI_RANGE_BAR_HANDLE_END;
}

static void
e9ui_range_bar_updateFromY(e9ui_range_bar_state_t *state, const e9ui_rect_t *bounds, int handle, int y)
{
    if (!state || !bounds) {
        return;
    }
    float nextPercent = e9ui_range_bar_yToPercent(bounds, y);
    if (handle == E9UI_RANGE_BAR_HANDLE_START) {
        if (nextPercent > state->endPercent) {
            nextPercent = state->endPercent;
        }
        if (nextPercent != state->startPercent) {
            state->startPercent = nextPercent;
            e9ui_range_bar_emitChange(state);
        }
        return;
    }
    if (handle == E9UI_RANGE_BAR_HANDLE_END) {
        if (nextPercent < state->startPercent) {
            nextPercent = state->startPercent;
        }
        if (nextPercent != state->endPercent) {
            state->endPercent = nextPercent;
            e9ui_range_bar_emitChange(state);
        }
    }
}

static int
e9ui_range_bar_handleEvent(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev)
{
    if (!self || !ctx || !self->state || !ev) {
        return 0;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)self->state;
    int grab = e9ui_scale_px(ctx, 6);
    if (grab < 0) {
        grab = 0;
    }
    if (ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_LEFT) {
        int mouseX = ev->button.x;
        int mouseY = ev->button.y;
        int inside = (mouseX >= self->bounds.x - grab && mouseX < self->bounds.x + self->bounds.w + grab &&
                      mouseY >= self->bounds.y - grab && mouseY < self->bounds.y + self->bounds.h + grab);
        if (!inside) {
            return 0;
        }
        state->dragging = 1;
        state->draggingHandle = e9ui_range_bar_pickHandle(&self->bounds, state->startPercent, state->endPercent, mouseY);
        state->hoverThumb = 1;
        e9ui_range_bar_ensureCursors();
        if (e9ui_range_bar_cursorNs) {
            e9ui_cursorCapture(ctx, self, e9ui_range_bar_cursorNs);
        }
        e9ui_range_bar_emitDrag(state, 1);
        e9ui_range_bar_updateFromY(state, &self->bounds, state->draggingHandle, mouseY);
        return 1;
    }
    if (ev->type == SDL_MOUSEMOTION) {
        int mouseX = ev->motion.x;
        int mouseY = ev->motion.y;
        int inside = (mouseX >= self->bounds.x - grab && mouseX < self->bounds.x + self->bounds.w + grab &&
                      mouseY >= self->bounds.y - grab && mouseY < self->bounds.y + self->bounds.h + grab);
        e9ui_range_bar_ensureCursors();
        if (state->dragging) {
            if (e9ui_range_bar_cursorNs) {
                e9ui_cursorCapture(ctx, self, e9ui_range_bar_cursorNs);
            }
        } else if (inside) {
            state->hoverThumb = 1;
            if (e9ui_range_bar_cursorNs) {
                e9ui_cursorRequest(ctx, self, e9ui_range_bar_cursorNs);
            }
        } else if (state->hoverThumb) {
            state->hoverThumb = 0;
            if (!ctx->cursorOverride && e9ui_range_bar_cursorArrow) {
                SDL_SetCursor(e9ui_range_bar_cursorArrow);
            }
        }
        if (!state->dragging) {
            return 0;
        }
        e9ui_range_bar_updateFromY(state, &self->bounds, state->draggingHandle, ev->motion.y);
        return 1;
    }
    if (ev->type == SDL_MOUSEBUTTONUP && ev->button.button == SDL_BUTTON_LEFT) {
        if (!state->dragging) {
            return 0;
        }
        state->dragging = 0;
        state->draggingHandle = E9UI_RANGE_BAR_HANDLE_NONE;
        e9ui_cursorRelease(ctx, self);
        e9ui_range_bar_emitDrag(state, 0);
        return 1;
    }
    return 0;
}

static void
e9ui_range_bar_renderTooltip(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_range_bar_state_t *state, int anchorY)
{
    if (!self || !ctx || !ctx->renderer || !ctx->font || !state || !state->dragging) {
        return;
    }
    char text[128];
    text[0] = '\0';
    if (state->tooltipCb) {
        state->tooltipCb(state->startPercent, state->endPercent, text, sizeof(text), state->tooltipUser);
    }
    if (!text[0]) {
        snprintf(text, sizeof(text), "%.1f%%..%.1f%%", state->startPercent * 100.0f, state->endPercent * 100.0f);
    }
    SDL_Color color = { 255, 255, 255, 255 };
    int textW = 0;
    int textH = 0;
    SDL_Texture *texture = e9ui_text_cache_getText(ctx->renderer, ctx->font, text, color, &textW, &textH);
    if (!texture) {
        return;
    }
    int padX = e9ui_scale_px(ctx, 6);
    int padY = e9ui_scale_px(ctx, 4);
    int offsetX = e9ui_scale_px(ctx, 8);
    int tipW = textW + padX * 2;
    int tipH = textH + padY * 2;
    int tipX = 0;
    if (state->side == e9ui_range_bar_sideLeft) {
        tipX = self->bounds.x + self->bounds.w + offsetX;
    } else {
        tipX = self->bounds.x - tipW - offsetX;
    }
    int tipY = anchorY - tipH / 2;
    if (tipX < 0) {
        tipX = 0;
    }
    if (tipY < 0) {
        tipY = 0;
    }
    if (ctx->winW > 0 && tipX + tipW > ctx->winW) {
        tipX = ctx->winW - tipW;
    }
    if (ctx->winH > 0 && tipY + tipH > ctx->winH) {
        tipY = ctx->winH - tipH;
    }
    SDL_Rect bg = { tipX, tipY, tipW, tipH };
    SDL_SetRenderDrawColor(ctx->renderer, 30, 30, 30, 230);
    SDL_RenderFillRect(ctx->renderer, &bg);
    SDL_Rect dst = { tipX + padX, tipY + padY, textW, textH };
    SDL_RenderCopy(ctx->renderer, texture, NULL, &dst);
}

static void
e9ui_range_bar_render(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !ctx || !ctx->renderer || !self->state) {
        return;
    }
    if (e9ui_getHidden(self)) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)self->state;
    int x = self->bounds.x;
    int y = self->bounds.y;
    int w = self->bounds.w;
    int h = self->bounds.h;
    if (w <= 0 || h <= 0) {
        return;
    }

    int trackW = w / 3;
    if (trackW < 3) {
        trackW = 3;
    }
    int trackX = x + (w - trackW) / 2;
    int startY = e9ui_range_bar_percentToY(&self->bounds, state->startPercent);
    int endY = e9ui_range_bar_percentToY(&self->bounds, state->endPercent);
    if (endY < startY) {
        int temp = startY;
        startY = endY;
        endY = temp;
    }

    SDL_Rect track = { trackX, y, trackW, h };
    SDL_SetRenderDrawColor(ctx->renderer, 80, 80, 80, 255);
    SDL_RenderFillRect(ctx->renderer, &track);

    int rangeH = endY - startY + 1;
    if (rangeH < 1) {
        rangeH = 1;
    }
    SDL_Rect range = { trackX, startY, trackW, rangeH };
    SDL_SetRenderDrawColor(ctx->renderer, 230, 33, 23, 220);
    SDL_RenderFillRect(ctx->renderer, &range);

    int handleH = e9ui_scale_px(ctx, 6);
    if (handleH < 4) {
        handleH = 4;
    }
    SDL_Rect startHandle = { x, startY - handleH / 2, w, handleH };
    SDL_Rect endHandle = { x, endY - handleH / 2, w, handleH };
    SDL_SetRenderDrawColor(ctx->renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(ctx->renderer, &startHandle);
    SDL_RenderFillRect(ctx->renderer, &endHandle);

    if (state->dragging) {
        int anchorY = (state->draggingHandle == E9UI_RANGE_BAR_HANDLE_START) ? startY : endY;
        e9ui_range_bar_renderTooltip(self, ctx, state, anchorY);
    }
}

e9ui_component_t *
e9ui_range_bar_make(void)
{
    e9ui_component_t *comp = (e9ui_component_t*)alloc_calloc(1, sizeof(*comp));
    if (!comp) {
        return NULL;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)alloc_calloc(1, sizeof(*state));
    if (!state) {
        alloc_free(comp);
        return NULL;
    }
    state->startPercent = 0.2f;
    state->endPercent = 0.8f;
    state->dragging = 0;
    state->draggingHandle = E9UI_RANGE_BAR_HANDLE_NONE;
    state->marginTop = 10;
    state->marginBottom = 10;
    state->marginSide = 10;
    state->width = 12;
    state->hoverMargin = 18;
    state->side = e9ui_range_bar_sideLeft;

    comp->name = "e9ui_range_bar";
    comp->state = state;
    comp->render = e9ui_range_bar_render;
    comp->handleEvent = e9ui_range_bar_handleEvent;
    return comp;
}

void
e9ui_range_bar_setSide(e9ui_component_t *comp, e9ui_range_bar_side_t side)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->side = side;
}

void
e9ui_range_bar_setMargins(e9ui_component_t *comp, int top, int bottom, int side)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->marginTop = top;
    state->marginBottom = bottom;
    state->marginSide = side;
}

void
e9ui_range_bar_setWidth(e9ui_component_t *comp, int width)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->width = width;
}

void
e9ui_range_bar_setHoverMargin(e9ui_component_t *comp, int margin)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->hoverMargin = margin;
}

int
e9ui_range_bar_getHoverMargin(e9ui_component_t *comp)
{
    if (!comp || !comp->state) {
        return 0;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    return state->hoverMargin;
}

void
e9ui_range_bar_setCallback(e9ui_component_t *comp, e9ui_range_bar_change_cb_t cb, void *user)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->cb = cb;
    state->cbUser = user;
}

void
e9ui_range_bar_setDragCallback(e9ui_component_t *comp, e9ui_range_bar_drag_cb_t cb, void *user)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->dragCb = cb;
    state->dragUser = user;
}

void
e9ui_range_bar_setTooltipCallback(e9ui_component_t *comp, e9ui_range_bar_tooltip_cb_t cb, void *user)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    state->tooltipCb = cb;
    state->tooltipUser = user;
}

void
e9ui_range_bar_setRangePercent(e9ui_component_t *comp, float startPercent, float endPercent)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    float nextStart = e9ui_range_bar_clampPercent(startPercent);
    float nextEnd = e9ui_range_bar_clampPercent(endPercent);
    if (nextEnd < nextStart) {
        float temp = nextStart;
        nextStart = nextEnd;
        nextEnd = temp;
    }
    state->startPercent = nextStart;
    state->endPercent = nextEnd;
}

int
e9ui_range_bar_isDragging(e9ui_component_t *comp)
{
    if (!comp || !comp->state) {
        return 0;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    return state->dragging ? 1 : 0;
}

void
e9ui_range_bar_layoutInParent(e9ui_component_t *comp, e9ui_context_t *ctx, e9ui_rect_t parent)
{
    if (!comp || !comp->state) {
        return;
    }
    e9ui_range_bar_state_t *state = (e9ui_range_bar_state_t*)comp->state;
    int top = state->marginTop;
    int bottom = state->marginBottom;
    int side = state->marginSide;
    int width = state->width;
    if (ctx) {
        top = e9ui_scale_px(ctx, top);
        bottom = e9ui_scale_px(ctx, bottom);
        side = e9ui_scale_px(ctx, side);
        width = e9ui_scale_px(ctx, width);
    }
    int h = parent.h - top - bottom;
    if (h < 1) {
        h = 1;
    }
    if (width < 1) {
        width = 1;
    }
    comp->bounds.y = parent.y + top;
    comp->bounds.h = h;
    comp->bounds.w = width;
    if (state->side == e9ui_range_bar_sideRight) {
        comp->bounds.x = parent.x + parent.w - side - width;
    } else {
        comp->bounds.x = parent.x + side;
    }
}
