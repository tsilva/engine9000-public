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

#include "alloc.h"
#include "e9ui_seek_bar.h"

typedef struct e9ui_slider_state
{
    char *label;
    e9ui_component_t *bar;
    int labelWidthPx;
    int gapPx;
    int barHeightPx;
    int rowPaddingPx;
    int rightMarginPx;
} e9ui_slider_state_t;

static int
e9ui_sliderPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    e9ui_slider_state_t *st = (e9ui_slider_state_t *)self->state;
    int barH = e9ui_scale_px(ctx, st->barHeightPx);
    if (barH <= 0) {
        barH = 1;
    }
    TTF_Font *font = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
    int textH = font ? TTF_FontHeight(font) : barH;
    if (textH < barH) {
        textH = barH;
    }
    int pad = e9ui_scale_px(ctx, st->rowPaddingPx);
    return textH + pad * 2;
}

static void
e9ui_sliderLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    if (!self || !self->state || !ctx) {
        return;
    }
    e9ui_slider_state_t *st = (e9ui_slider_state_t *)self->state;
    self->bounds = bounds;
    if (!st->bar) {
        return;
    }

    int labelW = e9ui_scale_px(ctx, st->labelWidthPx);
    int gap = e9ui_scale_px(ctx, st->gapPx);
    int barH = e9ui_scale_px(ctx, st->barHeightPx);
    if (barH <= 0) {
        barH = 1;
    }
    int knobR = barH / 2;
    if (knobR < 6) {
        knobR = 6;
    }
    int inset = knobR;
    int rightMargin = e9ui_scale_px(ctx, st->rightMarginPx);
    int barW = bounds.w - labelW - gap - inset * 2 - rightMargin;
    if (barW < 0) {
        barW = 0;
    }
    int barX = bounds.x + labelW + gap + inset;
    int barY = bounds.y + (bounds.h - barH) / 2;
    st->bar->bounds = (e9ui_rect_t){ barX, barY, barW, barH };
}

static void
e9ui_sliderRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }
    e9ui_slider_state_t *st = (e9ui_slider_state_t *)self->state;
    if (st->label && *st->label) {
        TTF_Font *font = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
        if (font) {
            SDL_Color color = (SDL_Color){220, 220, 220, 255};
            int tw = 0;
            int th = 0;
            SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, font, st->label, color, &tw, &th);
            if (tex) {
                int pad = e9ui_scale_px(ctx, st->rightMarginPx);
                int labelW = e9ui_scale_px(ctx, st->labelWidthPx) - pad;
                if (labelW < 0) {
                    labelW = 0;
                }
                int textX = self->bounds.x + pad;
                if (labelW > tw) {
                    textX = self->bounds.x + pad + labelW - tw;
                }
                int textY = self->bounds.y + (self->bounds.h - th) / 2;
                SDL_Rect dst = { textX, textY, tw, th };
                SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
            }
        }
    }
    if (st->bar && st->bar->render) {
        st->bar->render(st->bar, ctx);
    }
}

static void
e9ui_sliderDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self || !self->state) {
        return;
    }
    e9ui_slider_state_t *st = (e9ui_slider_state_t *)self->state;
    if (st->label) {
        alloc_free(st->label);
        st->label = NULL;
    }
}

e9ui_component_t *
e9ui_slider_make(const char *label,
                 int labelWidthPx,
                 int gapPx,
                 int rowPaddingPx,
                 int barHeightPx,
                 int rightMarginPx,
                 e9ui_component_t **outBar)
{
    e9ui_component_t *row = (e9ui_component_t *)alloc_calloc(1, sizeof(*row));
    e9ui_slider_state_t *st = (e9ui_slider_state_t *)alloc_calloc(1, sizeof(*st));
    if (!row || !st) {
        if (row) {
            alloc_free(row);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    if (label && *label) {
        st->label = alloc_strdup(label);
    }
    st->labelWidthPx = labelWidthPx;
    st->gapPx = gapPx;
    st->barHeightPx = barHeightPx;
    st->rowPaddingPx = rowPaddingPx;
    st->rightMarginPx = rightMarginPx;
    st->bar = e9ui_seek_bar_make();
    if (st->bar) {
        e9ui_seek_bar_setMargins(st->bar, 0, 0, 0);
    }

    row->name = "e9ui_slider";
    row->state = st;
    row->preferredHeight = e9ui_sliderPreferredHeight;
    row->layout = e9ui_sliderLayout;
    row->render = e9ui_sliderRender;
    row->dtor = e9ui_sliderDtor;

    if (st->bar) {
        e9ui_child_add(row, st->bar, NULL);
    }
    if (outBar) {
        *outBar = st->bar;
    }
    return row;
}
