/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <limits.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "aux_window.h"
#include "config.h"
#include "e9ui.h"
#include "e9ui_scroll.h"
#include "e9ui_text_cache.h"
#include "libretro_host.h"
#include "mega_vdp.h"
#include "mega_palette_debug.h"

#define MEGA_VDP_PALETTE_COUNT 4
#define MEGA_VDP_REG_TEXT_CAP 4096

typedef struct mega_vdp_state
{
    e9ui_window_state_t windowState;
    e9ui_component_t *root;
    e9ui_context_t ctx;
    e9ui_component_t *paletteCheckboxes[MEGA_VDP_PALETTE_COUNT];
    int spritesEnabled;
    int planeAEnabled;
    int planeBEnabled;
    uint32_t paletteGreyscaleMask;
} mega_vdp_state_t;

typedef struct mega_vdp_overlay_body_state
{
    mega_vdp_state_t *ui;
} mega_vdp_overlay_body_state_t;

typedef struct mega_vdp_palette_checkbox
{
    mega_vdp_state_t *ui;
    unsigned paletteIndex;
} mega_vdp_palette_checkbox_t;

typedef struct mega_vdp_register_view_state
{
    char text[MEGA_VDP_REG_TEXT_CAP];
} mega_vdp_register_view_state_t;

typedef struct mega_vdp_bandwidth_view_state
{
    uint32_t maxObservedSlots;
    uint32_t maxObservedDmaBytes;
} mega_vdp_bandwidth_view_state_t;

static void
mega_vdp_applyAllOptions(void);

static void
mega_vdp_syncPaletteCheckboxes(mega_vdp_state_t *ui, e9ui_context_t *ctx);

static e9ui_component_t *
mega_vdp_makeRegisterView(void);

static e9ui_component_t *
mega_vdp_makeBandwidthView(void);

static mega_vdp_state_t mega_vdp_state = {
    .windowState.winX = E9UI_WINDOW_COORD_UNSET,
    .windowState.winY = E9UI_WINDOW_COORD_UNSET,
    .windowState.openMinWidthPx = 220,
    .windowState.openMinHeightPx = 260,
    .windowState.openCenterWhenNoSaved = 1,
    .spritesEnabled = 1,
    .planeAEnabled = 1,
    .planeBEnabled = 1,
    .paletteGreyscaleMask = 0u
};

static mega_vdp_palette_checkbox_t mega_vdp_paletteCheckboxes[MEGA_VDP_PALETTE_COUNT];

static const SDL_Color mega_vdp_statsChartLabelColor = { 220, 220, 220, 255 };
static const SDL_Color mega_vdp_statsChartTextColor = { 232, 236, 240, 255 };
static const SDL_Color mega_vdp_statsChartTextShadowColor = { 12, 14, 18, 220 };
static const SDL_Color mega_vdp_dmaColorCpu = { 0xa2, 0x53, 0x42, 255 };
static const SDL_Color mega_vdp_dmaColorCopper = { 0xee, 0xee, 0x00, 255 };
static const SDL_Color mega_vdp_dmaColorAudio = { 0xff, 0x00, 0x00, 255 };
static const SDL_Color mega_vdp_dmaColorBlitter = { 0x00, 0x88, 0x88, 255 };
static const SDL_Color mega_vdp_dmaColorBitplane = { 0x00, 0x00, 0xff, 255 };
static const SDL_Color mega_vdp_dmaColorSprite = { 0xff, 0x00, 0xff, 255 };
static const SDL_Color mega_vdp_dmaColorOther = { 0xff, 0xb8, 0x40, 255 };
static const SDL_Color mega_vdp_dmaColorIdle = { 0x5a, 0x5a, 0x5a, 255 };

static const aux_window_ops_t mega_vdp_auxWindowOps = {
    .render = mega_vdp_render,
};

static int
mega_vdp_parseInt(const char *value, int *out)
{
    if (!value || !out) {
        return 0;
    }
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (!end || end == value) {
        return 0;
    }
    if (parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    *out = (int)parsed;
    return 1;
}

static e9ui_window_backend_t
mega_vdp_windowBackend(void)
{
    return e9ui_window_backend_overlay;
}

static e9ui_rect_t
mega_vdp_windowDefaultRect(const e9ui_context_t *ctx)
{
    e9ui_rect_t rect = {
        e9ui_scale_px(ctx, 96),
        e9ui_scale_px(ctx, 96),
        e9ui_scale_px(ctx, 260),
        e9ui_scale_px(ctx, 300)
    };
    return rect;
}

static void
mega_vdp_applyOption(e9k_debug_option_t option, int enabled)
{
    if (!libretro_host_debugSetDebugOption(option, enabled ? 1u : 0u, NULL)) {
        printf("mega vdp: core does not expose debug option API\n");
    }
}

static void
mega_vdp_applyAllOptions(void)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_SPRITES, ui->spritesEnabled);
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_PLANE_A, ui->planeAEnabled);
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_PLANE_B, ui->planeBEnabled);
    mega_palette_debug_setGreyscaleMask(ui->paletteGreyscaleMask);
}

static void
mega_vdp_statsChartDrawText(e9ui_context_t *ctx,
                            TTF_Font *font,
                            const char *text,
                            SDL_Color color,
                            int x,
                            int y)
{
    TTF_Font *useFont = font ? font : (ctx ? ctx->font : NULL);
    if (!ctx || !ctx->renderer || !useFont || !text || !*text) {
        return;
    }
    int tw = 0;
    int th = 0;
    SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, useFont, text, color, &tw, &th);
    if (!tex) {
        return;
    }
    SDL_Rect dst = { x, y, tw, th };
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
}

static int
mega_vdp_statsChartU32ToText(uint32_t value, char *buf, int cap)
{
    if (!buf || cap <= 1) {
        return 0;
    }
    char rev[16];
    int n = 0;
    uint32_t v = value;
    do {
        rev[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v > 0u && n < (int)sizeof(rev));
    if (n + 1 > cap) {
        n = cap - 1;
    }
    for (int i = 0; i < n; ++i) {
        buf[i] = rev[n - 1 - i];
    }
    buf[n] = '\0';
    return n;
}

static void
mega_vdp_statsChartMeasureUint(e9ui_context_t *ctx, TTF_Font *font, uint32_t value, SDL_Color color, int *outW, int *outH)
{
    if (outW) {
        *outW = 0;
    }
    if (outH) {
        *outH = 0;
    }
    if (!ctx || !ctx->renderer || !font) {
        return;
    }
    char digits[16];
    int n = mega_vdp_statsChartU32ToText(value, digits, (int)sizeof(digits));
    int w = 0;
    int h = 0;
    for (int i = 0; i < n; ++i) {
        char ch[2] = { digits[i], '\0' };
        int cw = 0;
        int chh = 0;
        SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, font, ch, color, &cw, &chh);
        if (!tex) {
            continue;
        }
        w += cw;
        if (chh > h) {
            h = chh;
        }
    }
    if (outW) {
        *outW = w;
    }
    if (outH) {
        *outH = h;
    }
}

static void
mega_vdp_statsChartMeasureText(e9ui_context_t *ctx, TTF_Font *font, const char *text, SDL_Color color, int *outW, int *outH)
{
    if (outW) {
        *outW = 0;
    }
    if (outH) {
        *outH = 0;
    }
    if (!ctx || !ctx->renderer || !font || !text || !*text) {
        return;
    }
    int tw = 0;
    int th = 0;
    SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, font, text, color, &tw, &th);
    if (!tex) {
        return;
    }
    if (outW) {
        *outW = tw;
    }
    if (outH) {
        *outH = th;
    }
}

static void
mega_vdp_statsChartDrawUint(e9ui_context_t *ctx, TTF_Font *font, uint32_t value, SDL_Color color, int x, int y)
{
    if (!ctx || !ctx->renderer || !font) {
        return;
    }
    char digits[16];
    int n = mega_vdp_statsChartU32ToText(value, digits, (int)sizeof(digits));
    int penX = x;
    for (int i = 0; i < n; ++i) {
        char ch[2] = { digits[i], '\0' };
        int cw = 0;
        int chh = 0;
        SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, font, ch, color, &cw, &chh);
        if (!tex) {
            continue;
        }
        SDL_Rect dst = { penX, y, cw, chh };
        SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
        penX += cw;
    }
}

static void
mega_vdp_statsChartDrawValueUsedSuffix(e9ui_context_t *ctx,
                                       TTF_Font *font,
                                       uint32_t valueUsed,
                                       const char *valueSuffix,
                                       SDL_Color color,
                                       int x,
                                       int y)
{
    int usedW = 0;
    int usedH = 0;
    int spaceW = 0;
    int spaceH = 0;
    int suffixW = 0;
    int suffixH = 0;
    mega_vdp_statsChartMeasureUint(ctx, font, valueUsed, color, &usedW, &usedH);
    mega_vdp_statsChartMeasureText(ctx, font, " ", color, &spaceW, &spaceH);
    mega_vdp_statsChartMeasureText(ctx, font, valueSuffix, color, &suffixW, &suffixH);
    int lineH = usedH;
    if (spaceH > lineH) {
        lineH = spaceH;
    }
    if (suffixH > lineH) {
        lineH = suffixH;
    }
    int penX = x;
    int usedY = y + (lineH - usedH) / 2;
    int spaceY = y + (lineH - spaceH) / 2;
    int suffixY = y + (lineH - suffixH) / 2;
    mega_vdp_statsChartDrawUint(ctx, font, valueUsed, color, penX, usedY);
    penX += usedW;
    mega_vdp_statsChartDrawText(ctx, font, " ", color, penX, spaceY);
    penX += spaceW;
    mega_vdp_statsChartDrawText(ctx, font, valueSuffix, color, penX, suffixY);
}

static int
mega_vdp_textboxLikeHeight(e9ui_context_t *ctx)
{
    TTF_Font *font = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : (ctx ? ctx->font : NULL);
    int lineHeight = font ? TTF_FontHeight(font) : 16;
    if (lineHeight <= 0) {
        lineHeight = 16;
    }
    return lineHeight + 12;
}

static void
mega_vdp_statsChartRenderBarRow(e9ui_context_t *ctx,
                                int contentX,
                                int barX,
                                int barW,
                                int barHeight,
                                int labelGap,
                                int rowY,
                                int rowH,
                                int fontH,
                                const char *labelText,
                                int hasData,
                                uint32_t valueUsed,
                                uint32_t valueMax,
                                const char *valueSuffix,
                                int useGradientFill,
                                SDL_Color solidFillColor,
                                int useTextShadow,
                                int useTextClip)
{
    (void)useGradientFill;

    SDL_Color labelColor = mega_vdp_statsChartLabelColor;
    SDL_Color textColor = mega_vdp_statsChartTextColor;
    SDL_Color textShadow = mega_vdp_statsChartTextShadowColor;
    int rowLabelY = rowY + (rowH - fontH) / 2;
    int rowBarY = rowY + (rowH - barHeight) / 2;

    {
        TTF_Font *labelFont = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
        int labelTextW = 0;
        if (labelFont) {
            TTF_SizeText(labelFont, labelText, &labelTextW, NULL);
        }
        int labelX = barX - labelGap - labelTextW;
        if (labelX < contentX) {
            labelX = contentX;
        }
        mega_vdp_statsChartDrawText(ctx, labelFont, labelText, labelColor, labelX, rowLabelY);
    }

    SDL_Rect trackRect = { barX, rowBarY, barW, barHeight };
    SDL_Rect innerRect = trackRect;
    if (innerRect.w > 2) {
        innerRect.x += 1;
        innerRect.w -= 2;
    }
    if (innerRect.h > 2) {
        innerRect.y += 1;
        innerRect.h -= 2;
    }

    SDL_SetRenderDrawColor(ctx->renderer, 34, 40, 46, 255);
    SDL_RenderFillRect(ctx->renderer, &trackRect);
    if (innerRect.w > 0 && innerRect.h > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 22, 26, 31, 255);
        SDL_RenderFillRect(ctx->renderer, &innerRect);
    }

    int fillW = 0;
    if (hasData && valueMax > 0u && innerRect.w > 0) {
        uint64_t scaled = (uint64_t)valueUsed * (uint64_t)(uint32_t)innerRect.w;
        fillW = (int)((scaled + (uint64_t)(valueMax / 2u)) / (uint64_t)valueMax);
        if (fillW < 0) {
            fillW = 0;
        }
        if (fillW > innerRect.w) {
            fillW = innerRect.w;
        }
    }
    if (fillW > 0 && innerRect.h > 0) {
        SDL_Rect fillRect = { innerRect.x, innerRect.y, fillW, innerRect.h };
        SDL_SetRenderDrawColor(ctx->renderer, solidFillColor.r, solidFillColor.g, solidFillColor.b, 255);
        SDL_RenderFillRect(ctx->renderer, &fillRect);
    }

    SDL_SetRenderDrawColor(ctx->renderer, 64, 72, 82, 255);
    SDL_RenderDrawRect(ctx->renderer, &trackRect);

    if (ctx->font) {
        int tw = 0;
        int th = 0;
        if (!hasData) {
            mega_vdp_statsChartMeasureText(ctx, ctx->font, "n/a", textColor, &tw, &th);
        } else {
            int usedW = 0;
            int usedH = 0;
            int spaceW = 0;
            int spaceH = 0;
            int suffixW = 0;
            int suffixH = 0;
            mega_vdp_statsChartMeasureUint(ctx, ctx->font, valueUsed, textColor, &usedW, &usedH);
            mega_vdp_statsChartMeasureText(ctx, ctx->font, " ", textColor, &spaceW, &spaceH);
            mega_vdp_statsChartMeasureText(ctx, ctx->font, valueSuffix, textColor, &suffixW, &suffixH);
            tw = usedW + spaceW + suffixW;
            th = usedH;
            if (spaceH > th) {
                th = spaceH;
            }
            if (suffixH > th) {
                th = suffixH;
            }
        }
        if (tw > 0 && th > 0) {
            int tx = barX + (barW - tw) / 2;
            int ty = rowBarY + (barHeight - th) / 2;
            if (tx < barX + 2) {
                tx = barX + 2;
            }
            if (ty < rowBarY) {
                ty = rowBarY;
            }
            SDL_bool hadPrevClip = SDL_FALSE;
            SDL_Rect prevClip;
            if (useTextClip) {
                hadPrevClip = SDL_RenderIsClipEnabled(ctx->renderer);
                if (hadPrevClip) {
                    SDL_RenderGetClipRect(ctx->renderer, &prevClip);
                }
                int clipPad = e9ui_scale_px(ctx, 4);
                if (clipPad < 1) {
                    clipPad = 1;
                }
                SDL_Rect textClip = { barX + clipPad, rowBarY, barW - clipPad * 2, barHeight };
                if (textClip.w < 1) {
                    textClip.x = barX;
                    textClip.w = barW;
                }
                if (textClip.h < 1) {
                    textClip.y = rowBarY;
                    textClip.h = barHeight;
                }
                SDL_RenderSetClipRect(ctx->renderer, &textClip);
            }
            if (!hasData) {
                if (useTextShadow) {
                    mega_vdp_statsChartDrawText(ctx, NULL, "n/a", textShadow, tx + 1, ty + 1);
                }
                mega_vdp_statsChartDrawText(ctx, NULL, "n/a", textColor, tx, ty);
            } else {
                if (useTextShadow) {
                    mega_vdp_statsChartDrawValueUsedSuffix(ctx,
                                                           ctx->font,
                                                           valueUsed,
                                                           valueSuffix,
                                                           textShadow,
                                                           tx + 1,
                                                           ty + 1);
                }
                mega_vdp_statsChartDrawValueUsedSuffix(ctx,
                                                       ctx->font,
                                                       valueUsed,
                                                       valueSuffix,
                                                       textColor,
                                                       tx,
                                                       ty);
            }
            if (useTextClip) {
                if (hadPrevClip) {
                    SDL_RenderSetClipRect(ctx->renderer, &prevClip);
                } else {
                    SDL_RenderSetClipRect(ctx->renderer, NULL);
                }
            }
        }
    }
}

static int
mega_vdp_appendf(char *buf, size_t cap, size_t *offset, const char *fmt, ...)
{
    va_list ap;
    int written = 0;

    if (!buf || !offset || !fmt || *offset >= cap) {
        return 0;
    }

    va_start(ap, fmt);
    written = vsnprintf(buf + *offset, cap - *offset, fmt, ap);
    va_end(ap);
    if (written < 0) {
        return 0;
    }
    if ((size_t)written >= cap - *offset) {
        *offset = cap - 1u;
        buf[*offset] = '\0';
        return 0;
    }
    *offset += (size_t)written;
    return 1;
}

static uint32_t
mega_vdp_planeATableBase(const uint8_t *regs)
{
    return ((uint32_t)(regs[2] & 0x38u) << 9u) * 2u;
}

static uint32_t
mega_vdp_planeBTableBase(const uint8_t *regs)
{
    return ((uint32_t)(regs[4] & 0x07u) << 12u) * 2u;
}

static uint32_t
mega_vdp_windowTableBase(const uint8_t *regs)
{
    uint32_t mask = (regs[12] & 1u) ? 0x3cu : 0x3eu;
    return (uint32_t)(regs[3] & mask) << 10u;
}

static uint32_t
mega_vdp_spriteTableBase(const uint8_t *regs)
{
    uint32_t table = (uint32_t)(regs[5] & 0x7fu);

    if (regs[12] & 1u) {
        table &= 0x7eu;
    }
    return table << 9u;
}

static uint32_t
mega_vdp_hscrollTableBase(const uint8_t *regs)
{
    return (uint32_t)(regs[13] & 0x3fu) << 10u;
}

static void
mega_vdp_scrollSize(const uint8_t *regs, unsigned *outCols, unsigned *outRows)
{
    static const uint8_t shift[4] = { 5u, 6u, 5u, 7u };
    unsigned widthCode = (unsigned)(regs[16] & 3u);
    unsigned heightCode = (unsigned)((regs[16] >> 4) & 3u);
    unsigned rows = (heightCode << 5u) | 0x1fu;

    if (widthCode == 1u) {
        rows &= 0x3fu;
    } else if (widthCode > 1u) {
        rows = 0x1fu;
    }

    if (outCols) {
        *outCols = 1u << shift[widthCode];
    }
    if (outRows) {
        *outRows = rows + 1u;
    }
}

static const char *
mega_vdp_hscrollMode(uint8_t reg11)
{
    switch (reg11 & 3u) {
        case 0:
            return "full";
        case 2:
            return "cell";
        case 3:
            return "line";
        default:
            return "invalid";
    }
}

static const char *
mega_vdp_interlaceMode(uint8_t reg12)
{
    switch ((reg12 >> 1) & 3u) {
        case 0:
            return "off";
        case 1:
            return "mode 1";
        case 3:
            return "mode 2";
        default:
            return "invalid";
    }
}

static void
mega_vdp_formatRegisters(char *buf, size_t cap)
{
    e9k_debug_mega_sprite_state_t vdp;
    size_t offset = 0u;
    const uint8_t *r = NULL;
    unsigned scrollCols = 0u;
    unsigned scrollRows = 0u;
    unsigned bgPalette = 0u;
    unsigned bgColor = 0u;

    if (!buf || cap == 0u) {
        return;
    }
    buf[0] = '\0';

    memset(&vdp, 0, sizeof(vdp));
    if (!libretro_host_megadrive_getSpriteState(&vdp)) {
        (void)mega_vdp_appendf(buf, cap, &offset, "VDP registers unavailable\n");
        return;
    }

    r = vdp.vdpRegs;
    mega_vdp_scrollSize(r, &scrollCols, &scrollRows);
    bgPalette = (unsigned)((r[7] >> 4) & 3u);
    bgColor = (unsigned)(r[7] & 0x0fu);

    (void)mega_vdp_appendf(buf, cap, &offset, "REGISTERS\n");
    for (unsigned base = 0u; base < 32u; base += 8u) {
        (void)mega_vdp_appendf(buf,
                               cap,
                               &offset,
                               "%02X-%02X  %02X %02X %02X %02X %02X %02X %02X %02X\n",
                               base,
                               base + 7u,
                               r[base + 0u],
                               r[base + 1u],
                               r[base + 2u],
                               r[base + 3u],
                               r[base + 4u],
                               r[base + 5u],
                               r[base + 6u],
                               r[base + 7u]);
    }

    (void)mega_vdp_appendf(buf, cap, &offset, "\nDISPLAY\n");
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "display:%s  width:%u cells  screen:%dx%d  lines:%d\n",
                           (r[1] & 0x40u) ? "on" : "off",
                           (r[12] & 1u) ? 40u : 32u,
                           vdp.screenW,
                           vdp.screenH,
                           vdp.lineCount);
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "interlace:%s  shadow/highlight:%s  bg:P%u C%02u\n",
                           mega_vdp_interlaceMode(r[12]),
                           (r[12] & 0x08u) ? "on" : "off",
                           bgPalette,
                           bgColor);
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "hscroll:%s  vscroll:%s  autoinc:%u\n",
                           mega_vdp_hscrollMode(r[11]),
                           (r[11] & 0x04u) ? "2-cell" : "full",
                           (unsigned)r[15]);

    (void)mega_vdp_appendf(buf, cap, &offset, "\nTABLES\n");
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "plane A:$%04X  plane B:$%04X  window:$%04X\n",
                           mega_vdp_planeATableBase(r),
                           mega_vdp_planeBTableBase(r),
                           mega_vdp_windowTableBase(r));
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "sprites:$%04X  hscroll:$%04X  plane:%ux%u\n",
                           mega_vdp_spriteTableBase(r),
                           mega_vdp_hscrollTableBase(r),
                           scrollCols,
                           scrollRows);

    (void)mega_vdp_appendf(buf, cap, &offset, "\nINTERRUPTS / DMA\n");
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "hint:%s counter:%u  vint:%s  dma:%s\n",
                           (r[0] & 0x10u) ? "on" : "off",
                           (unsigned)r[10],
                           (r[1] & 0x20u) ? "on" : "off",
                           (r[1] & 0x10u) ? "on" : "off");
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "dma len:$%04X  src:$%06X  mode:%u\n",
                           (unsigned)r[19] | ((unsigned)r[20] << 8),
                           ((unsigned)r[21] | ((unsigned)r[22] << 8) | ((unsigned)(r[23] & 0x7fu) << 16)) << 1,
                           (unsigned)(r[23] >> 6));

    (void)mega_vdp_appendf(buf, cap, &offset, "\nWINDOW\n");
    (void)mega_vdp_appendf(buf,
                           cap,
                           &offset,
                           "h:%s %u  v:%s %u\n",
                           (r[17] & 0x80u) ? "right" : "left",
                           (unsigned)(r[17] & 0x1fu),
                           (r[18] & 0x80u) ? "down" : "up",
                           (unsigned)(r[18] & 0x1fu));
}

static int
mega_vdp_registerViewLineCount(const char *text)
{
    int lines = 1;

    if (!text || !*text) {
        return 1;
    }
    for (const char *p = text; *p; ++p) {
        if (*p == '\n' && p[1] != '\0') {
            ++lines;
        }
    }
    return lines;
}

static int
mega_vdp_registerViewPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    mega_vdp_register_view_state_t *st = NULL;
    TTF_Font *font = NULL;
    int lineHeight = 14;

    (void)availW;

    if (!self || !self->state) {
        return e9ui_scale_px(ctx, 120);
    }
    st = (mega_vdp_register_view_state_t *)self->state;
    mega_vdp_formatRegisters(st->text, sizeof(st->text));
    font = e9ui->theme.text.source ? e9ui->theme.text.source : (ctx ? ctx->font : NULL);
    if (font) {
        lineHeight = TTF_FontHeight(font);
    }
    if (lineHeight <= 0) {
        lineHeight = 14;
    }
    return e9ui_scale_px(ctx, 16) +
           mega_vdp_registerViewLineCount(st->text) * (lineHeight + e9ui_scale_px(ctx, 2));
}

static void
mega_vdp_registerViewLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}

static void
mega_vdp_registerViewRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    mega_vdp_register_view_state_t *st = NULL;
    TTF_Font *font = NULL;
    SDL_Color titleColor = {150, 190, 230, 255};
    SDL_Color textColor = {220, 220, 220, 255};
    int lineHeight = 14;
    int x = 0;
    int y = 0;
    char line[160];
    size_t lineLen = 0u;

    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }
    st = (mega_vdp_register_view_state_t *)self->state;
    mega_vdp_formatRegisters(st->text, sizeof(st->text));

    font = e9ui->theme.text.source ? e9ui->theme.text.source : ctx->font;
    if (!font) {
        return;
    }
    lineHeight = TTF_FontHeight(font);
    if (lineHeight <= 0) {
        lineHeight = 14;
    }

    x = self->bounds.x + e9ui_scale_px(ctx, 12);
    y = self->bounds.y + e9ui_scale_px(ctx, 8);
    for (const char *p = st->text; ; ++p) {
        if (*p != '\n' && *p != '\0' && lineLen + 1u < sizeof(line)) {
            line[lineLen++] = *p;
            continue;
        }

        line[lineLen] = '\0';
        if (lineLen > 0u) {
            int tw = 0;
            int th = 0;
            SDL_Color color = strchr(line, ':') || strstr(line, "$") ? textColor : titleColor;
            SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer, font, line, color, &tw, &th);
            if (tex) {
                SDL_Rect dst = { x, y, tw, th };
                SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
            }
        }
        y += lineHeight + e9ui_scale_px(ctx, 2);
        lineLen = 0u;
        if (*p == '\0') {
            break;
        }
    }
}

static void
mega_vdp_registerViewDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    alloc_free(self->state);
    self->state = NULL;
}

static e9ui_component_t *
mega_vdp_makeRegisterView(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    if (!comp) {
        return NULL;
    }
    mega_vdp_register_view_state_t *st = (mega_vdp_register_view_state_t *)alloc_calloc(1, sizeof(*st));
    if (!st) {
        alloc_free(comp);
        return NULL;
    }
    comp->name = "mega_vdp_register_view";
    comp->state = st;
    comp->preferredHeight = mega_vdp_registerViewPreferredHeight;
    comp->layout = mega_vdp_registerViewLayout;
    comp->render = mega_vdp_registerViewRender;
    comp->dtor = mega_vdp_registerViewDtor;
    return comp;
}

static int
mega_vdp_bandwidthViewPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)self;
    (void)availW;
    if (!ctx) {
        return 0;
    }
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = mega_vdp_textboxLikeHeight(ctx);
    int rowGap = e9ui_scale_px(ctx, 4);
    int titleGap = e9ui_scale_px(ctx, 8);
    int sectionGap = e9ui_scale_px(ctx, 14);
    int topPad = e9ui_scale_px(ctx, 8);
    int bottomPad = e9ui_scale_px(ctx, 8);

    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + fontH + titleGap + rowH * 6 + rowGap * 5 +
           sectionGap + fontH + titleGap + rowH * 7 + rowGap * 6 +
           bottomPad;
}

static void
mega_vdp_bandwidthViewLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}

static void
mega_vdp_bandwidthDrawTotalRow(e9ui_context_t *ctx,
                               int contentX,
                               int barX,
                               int barW,
                               int barHeight,
                               int labelGap,
                               int rowY,
                               int rowH,
                               int fontH,
                               uint32_t maxSlots,
                               const e9k_debug_mega_vdp_bandwidth_frame_t *frame)
{
    TTF_Font *labelFont = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
    int labelTextW = 0;
    int rowLabelY = rowY + (rowH - fontH) / 2;
    int rowBarY = rowY + (rowH - barHeight) / 2;
    SDL_Rect trackRect = { barX, rowBarY, barW, barHeight };
    SDL_Rect innerRect = trackRect;
    uint32_t values[5];
    SDL_Color colors[5];
    uint64_t accum = 0u;
    int drawX = 0;

    if (!ctx || !ctx->renderer || !frame) {
        return;
    }

    if (labelFont) {
        TTF_SizeText(labelFont, "Total", &labelTextW, NULL);
    }
    int labelX = barX - labelGap - labelTextW;
    if (labelX < contentX) {
        labelX = contentX;
    }
    mega_vdp_statsChartDrawText(ctx,
                                                     labelFont,
                                                     "Total",
                                                     mega_vdp_statsChartLabelColor,
                                                     labelX,
                                                     rowLabelY);

    if (innerRect.w > 2) {
        innerRect.x += 1;
        innerRect.w -= 2;
    }
    if (innerRect.h > 2) {
        innerRect.y += 1;
        innerRect.h -= 2;
    }

    SDL_SetRenderDrawColor(ctx->renderer, 34, 40, 46, 255);
    SDL_RenderFillRect(ctx->renderer, &trackRect);
    if (innerRect.w > 0 && innerRect.h > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 22, 26, 31, 255);
        SDL_RenderFillRect(ctx->renderer, &innerRect);
    }

    values[0] = frame->cpuWriteSlots;
    values[1] = frame->dma68kSlots;
    values[2] = frame->dmaCopySlots;
    values[3] = frame->dmaFillSlots;
    values[4] = frame->readSlots;
    colors[0] = mega_vdp_dmaColorCpu;
    colors[1] = mega_vdp_dmaColorBlitter;
    colors[2] = mega_vdp_dmaColorBitplane;
    colors[3] = mega_vdp_dmaColorSprite;
    colors[4] = mega_vdp_dmaColorCopper;

    if (maxSlots > 0u && innerRect.w > 0 && innerRect.h > 0) {
        drawX = innerRect.x;
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
            uint64_t prev = accum;
            accum += (uint64_t)values[i];
            if (accum > (uint64_t)maxSlots) {
                accum = (uint64_t)maxSlots;
            }
            int x0 = innerRect.x + (int)((prev * (uint64_t)(uint32_t)innerRect.w) / (uint64_t)maxSlots);
            int x1 = innerRect.x + (int)((accum * (uint64_t)(uint32_t)innerRect.w) / (uint64_t)maxSlots);
            if (x1 <= x0) {
                continue;
            }
            SDL_Rect seg = { x0, innerRect.y, x1 - x0, innerRect.h };
            SDL_SetRenderDrawColor(ctx->renderer, colors[i].r, colors[i].g, colors[i].b, 255);
            SDL_RenderFillRect(ctx->renderer, &seg);
            drawX = x1;
        }
        if (drawX < innerRect.x + innerRect.w) {
            SDL_Rect seg = { drawX, innerRect.y, innerRect.x + innerRect.w - drawX, innerRect.h };
            SDL_SetRenderDrawColor(ctx->renderer,
                                   mega_vdp_dmaColorIdle.r,
                                   mega_vdp_dmaColorIdle.g,
                                   mega_vdp_dmaColorIdle.b,
                                   255);
            SDL_RenderFillRect(ctx->renderer, &seg);
        }
    }

    SDL_SetRenderDrawColor(ctx->renderer, 64, 72, 82, 255);
    SDL_RenderDrawRect(ctx->renderer, &trackRect);
}

static void
mega_vdp_bandwidthViewRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    mega_vdp_bandwidth_view_state_t *st = NULL;
    e9k_debug_mega_vdp_bandwidth_frame_t frame;
    TTF_Font *titleFont = NULL;
    int leftInset = 0;
    int rightInset = 0;
    int topPad = 0;
    int rowGap = 0;
    int labelWidth = 0;
    int labelGap = 0;
    int barHeight = 0;
    int fontH = 0;
    int contentX = 0;
    int contentY = 0;
    int contentW = 0;
    int rowH = 0;
    int barX = 0;
    int barW = 0;
    uint32_t maxSlots = 0u;
    uint32_t dma68kBytes = 0u;
    uint32_t dmaFillBytes = 0u;
    uint32_t dmaVramBytes = 0u;
    uint32_t dmaCramBytes = 0u;
    uint32_t dmaVsramBytes = 0u;
    uint32_t maxDmaBytes = 0u;

    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }
    st = (mega_vdp_bandwidth_view_state_t *)self->state;
    memset(&frame, 0, sizeof(frame));

    if (!ctx->font) {
        return;
    }

    if (!libretro_host_megadrive_getVdpBandwidthFrame(&frame)) {
        mega_vdp_statsChartDrawText(ctx,
                                                         ctx->font,
                                                         "VDP transfer load unavailable",
                                                         mega_vdp_statsChartTextColor,
                                                         self->bounds.x + e9ui_scale_px(ctx, 12),
                                                         self->bounds.y + e9ui_scale_px(ctx, 8));
        return;
    }

    if (frame.totalSlots > st->maxObservedSlots) {
        st->maxObservedSlots = frame.totalSlots;
    }
    maxSlots = frame.estimatedMaxSlots;
    if (maxSlots < st->maxObservedSlots) {
        maxSlots = st->maxObservedSlots;
    }
    if (maxSlots == 0u) {
        maxSlots = 1u;
    }
    dma68kBytes = frame.dma68kVramBytes + frame.dma68kCramBytes + frame.dma68kVsramBytes;
    dmaFillBytes = frame.dmaFillVramBytes + frame.dmaFillCramBytes + frame.dmaFillVsramBytes;
    dmaVramBytes = frame.dma68kVramBytes + frame.dmaCopyVramBytes + frame.dmaFillVramBytes;
    dmaCramBytes = frame.dma68kCramBytes + frame.dmaFillCramBytes;
    dmaVsramBytes = frame.dma68kVsramBytes + frame.dmaFillVsramBytes;
    if (frame.dmaTotalBytes > st->maxObservedDmaBytes) {
        st->maxObservedDmaBytes = frame.dmaTotalBytes;
    }
    maxDmaBytes = st->maxObservedDmaBytes;
    if (maxDmaBytes == 0u) {
        maxDmaBytes = 1u;
    }

    leftInset = e9ui_scale_px(ctx, 12);
    rightInset = e9ui_scale_px(ctx, 14);
    topPad = e9ui_scale_px(ctx, 8);
    rowGap = e9ui_scale_px(ctx, 4);
    labelWidth = e9ui_scale_px(ctx, 78);
    labelGap = e9ui_scale_px(ctx, 8);
    barHeight = mega_vdp_textboxLikeHeight(ctx);
    fontH = TTF_FontHeight(ctx->font);
    if (fontH <= 0) {
        fontH = 12;
    }
    contentX = self->bounds.x + leftInset;
    contentY = self->bounds.y + topPad;
    contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }
    rowH = fontH > barHeight ? fontH : barHeight;
    barX = contentX + labelWidth + labelGap;
    barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }

    titleFont = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
    mega_vdp_statsChartDrawText(ctx,
                                                     titleFont,
                                                     "VDP transfer load",
                                                     mega_vdp_statsChartLabelColor,
                                                     contentX,
                                                     contentY);
    contentY += fontH + e9ui_scale_px(ctx, 8);

    mega_vdp_bandwidthDrawTotalRow(ctx,
                                   contentX,
                                   barX,
                                   barW,
                                   barHeight,
                                   labelGap,
                                   contentY,
                                   rowH,
                                   fontH,
                                   maxSlots,
                                   &frame);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "CPU",
                                                   1,
                                                   frame.cpuWriteSlots,
                                                   maxSlots,
                                                   "slots/frame",
                                                   0,
                                                   mega_vdp_dmaColorCpu,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "DMA68K",
                                                   1,
                                                   frame.dma68kSlots,
                                                   maxSlots,
                                                   "slots/frame",
                                                   0,
                                                   mega_vdp_dmaColorBlitter,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Copy",
                                                   1,
                                                   frame.dmaCopySlots,
                                                   maxSlots,
                                                   "slots/frame",
                                                   0,
                                                   mega_vdp_dmaColorBitplane,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Fill",
                                                   1,
                                                   frame.dmaFillSlots,
                                                   maxSlots,
                                                   "slots/frame",
                                                   0,
                                                   mega_vdp_dmaColorSprite,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Read",
                                                   1,
                                                   frame.readSlots,
                                                   maxSlots,
                                                   "slots/frame",
                                                   0,
                                                   mega_vdp_dmaColorCopper,
                                                   0,
                                                   1);
    contentY += rowH + e9ui_scale_px(ctx, 14);

    mega_vdp_statsChartDrawText(ctx,
                                                     titleFont,
                                                     "VDP DMA payload",
                                                     mega_vdp_statsChartLabelColor,
                                                     contentX,
                                                     contentY);
    contentY += fontH + e9ui_scale_px(ctx, 8);

    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Total",
                                                   1,
                                                   frame.dmaTotalBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorOther,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "68K->VDP",
                                                   1,
                                                   dma68kBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorBlitter,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Copy",
                                                   1,
                                                   frame.dmaCopyVramBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorBitplane,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "Fill",
                                                   1,
                                                   dmaFillBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorSprite,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "VRAM",
                                                   1,
                                                   dmaVramBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorCpu,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "CRAM",
                                                   1,
                                                   dmaCramBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorCopper,
                                                   0,
                                                   1);
    contentY += rowH + rowGap;
    mega_vdp_statsChartRenderBarRow(ctx,
                                                   contentX,
                                                   barX,
                                                   barW,
                                                   barHeight,
                                                   labelGap,
                                                   contentY,
                                                   rowH,
                                                   fontH,
                                                   "VSRAM",
                                                   1,
                                                   dmaVsramBytes,
                                                   maxDmaBytes,
                                                   "bytes/frame",
                                                   0,
                                                   mega_vdp_dmaColorAudio,
                                                   0,
                                                   1);
}

static void
mega_vdp_bandwidthViewDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    alloc_free(self->state);
    self->state = NULL;
}

static e9ui_component_t *
mega_vdp_makeBandwidthView(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    if (!comp) {
        return NULL;
    }
    mega_vdp_bandwidth_view_state_t *st = (mega_vdp_bandwidth_view_state_t *)alloc_calloc(1, sizeof(*st));
    if (!st) {
        alloc_free(comp);
        return NULL;
    }
    comp->name = "mega_vdp_bandwidth_view";
    comp->state = st;
    comp->preferredHeight = mega_vdp_bandwidthViewPreferredHeight;
    comp->layout = mega_vdp_bandwidthViewLayout;
    comp->render = mega_vdp_bandwidthViewRender;
    comp->dtor = mega_vdp_bandwidthViewDtor;
    return comp;
}

static void
mega_vdp_spritesChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    mega_vdp_state_t *ui = (mega_vdp_state_t *)user;
    if (!ui) {
        return;
    }
    ui->spritesEnabled = selected ? 1 : 0;
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_SPRITES, ui->spritesEnabled);
}

static void
mega_vdp_planeAChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    mega_vdp_state_t *ui = (mega_vdp_state_t *)user;
    if (!ui) {
        return;
    }
    ui->planeAEnabled = selected ? 1 : 0;
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_PLANE_A, ui->planeAEnabled);
}

static void
mega_vdp_planeBChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    mega_vdp_state_t *ui = (mega_vdp_state_t *)user;
    if (!ui) {
        return;
    }
    ui->planeBEnabled = selected ? 1 : 0;
    mega_vdp_applyOption(E9K_DEBUG_OPTION_MEGA_VDP_PLANE_B, ui->planeBEnabled);
}

static void
mega_vdp_paletteChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    mega_vdp_palette_checkbox_t *cb = (mega_vdp_palette_checkbox_t *)user;
    if (!cb || !cb->ui || cb->paletteIndex >= MEGA_VDP_PALETTE_COUNT) {
        return;
    }

    uint32_t bit = 1u << cb->paletteIndex;
    if (selected) {
        cb->ui->paletteGreyscaleMask &= ~bit;
    } else {
        cb->ui->paletteGreyscaleMask |= bit;
    }
    cb->ui->paletteGreyscaleMask &= 0x0fu;
    mega_palette_debug_setGreyscaleMask(cb->ui->paletteGreyscaleMask);
}

static void
mega_vdp_syncPaletteCheckboxes(mega_vdp_state_t *ui, e9ui_context_t *ctx)
{
    if (!ui) {
        return;
    }

    uint32_t mask = mega_palette_debug_getGreyscaleMask() & 0x0fu;
    if (ui->paletteGreyscaleMask == mask) {
        return;
    }
    ui->paletteGreyscaleMask = mask;
    for (unsigned paletteIndex = 0; paletteIndex < MEGA_VDP_PALETTE_COUNT; ++paletteIndex) {
        e9ui_component_t *checkbox = ui->paletteCheckboxes[paletteIndex];
        int selected = (mask & (1u << paletteIndex)) ? 0 : 1;
        if (checkbox && e9ui_checkbox_isSelected(checkbox) != selected) {
            e9ui_checkbox_setSelected(checkbox, selected, ctx);
        }
    }
}

static e9ui_component_t *
mega_vdp_buildRoot(mega_vdp_state_t *ui)
{
    e9ui_component_t *root = e9ui_stack_makeVertical();
    e9ui_stack_addFixed(root, e9ui_vspacer_make(12));

    e9ui_component_t *cbSprites = e9ui_checkbox_make("Sprites",
                                                     ui->spritesEnabled,
                                                     mega_vdp_spritesChanged,
                                                     ui);
    e9ui_checkbox_setLeftMargin(cbSprites, 12);
    e9ui_stack_addFixed(root, cbSprites);
    e9ui_stack_addFixed(root, e9ui_vspacer_make(8));

    e9ui_component_t *cbPlaneA = e9ui_checkbox_make("Plane A",
                                                    ui->planeAEnabled,
                                                    mega_vdp_planeAChanged,
                                                    ui);
    e9ui_checkbox_setLeftMargin(cbPlaneA, 12);
    e9ui_stack_addFixed(root, cbPlaneA);
    e9ui_stack_addFixed(root, e9ui_vspacer_make(8));

    e9ui_component_t *cbPlaneB = e9ui_checkbox_make("Plane B",
                                                    ui->planeBEnabled,
                                                    mega_vdp_planeBChanged,
                                                    ui);
    e9ui_checkbox_setLeftMargin(cbPlaneB, 12);
    e9ui_stack_addFixed(root, cbPlaneB);
    e9ui_stack_addFixed(root, e9ui_vspacer_make(14));

    static const char *paletteLabels[MEGA_VDP_PALETTE_COUNT] = {
        "Palette 0",
        "Palette 1",
        "Palette 2",
        "Palette 3"
    };
    for (unsigned paletteIndex = 0; paletteIndex < MEGA_VDP_PALETTE_COUNT; ++paletteIndex) {
        mega_vdp_paletteCheckboxes[paletteIndex].ui = ui;
        mega_vdp_paletteCheckboxes[paletteIndex].paletteIndex = paletteIndex;
        e9ui_component_t *cbPalette =
            e9ui_checkbox_make(paletteLabels[paletteIndex],
                               (ui->paletteGreyscaleMask & (1u << paletteIndex)) ? 0 : 1,
                               mega_vdp_paletteChanged,
                               &mega_vdp_paletteCheckboxes[paletteIndex]);
        e9ui_checkbox_setLeftMargin(cbPalette, 12);
        e9ui_stack_addFixed(root, cbPalette);
        ui->paletteCheckboxes[paletteIndex] = cbPalette;
        if (paletteIndex + 1u < MEGA_VDP_PALETTE_COUNT) {
            e9ui_stack_addFixed(root, e9ui_vspacer_make(8));
        }
    }

    e9ui_stack_addFixed(root, e9ui_vspacer_make(14));
    e9ui_component_t *bandwidthView = mega_vdp_makeBandwidthView();
    if (bandwidthView) {
        e9ui_stack_addFixed(root, bandwidthView);
        e9ui_stack_addFixed(root, e9ui_vspacer_make(14));
    }

    e9ui_component_t *registerView = mega_vdp_makeRegisterView();
    if (registerView) {
        e9ui_stack_addFixed(root, registerView);
    }
    e9ui_stack_addFlex(root, e9ui_vspacer_make(1));

    return root;
}

static void
mega_vdp_overlayBodyLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    if (!self) {
        return;
    }
    self->bounds = bounds;
    mega_vdp_overlay_body_state_t *st = (mega_vdp_overlay_body_state_t *)self->state;
    if (!st || !st->ui || !st->ui->root || !st->ui->root->layout) {
        return;
    }
    st->ui->root->layout(st->ui->root, ctx, bounds);
}

static int
mega_vdp_overlayBodyPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    if (!self) {
        return 0;
    }
    mega_vdp_overlay_body_state_t *st = (mega_vdp_overlay_body_state_t *)self->state;
    if (!st || !st->ui || !st->ui->root || !st->ui->root->preferredHeight) {
        return 0;
    }
    return st->ui->root->preferredHeight(st->ui->root, ctx, availW);
}

static void
mega_vdp_overlayBodyRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !ctx) {
        return;
    }
    mega_vdp_overlay_body_state_t *st = (mega_vdp_overlay_body_state_t *)self->state;
    if (!st || !st->ui || !st->ui->root) {
        return;
    }
    st->ui->ctx = *ctx;
    if (st->ui->root->render) {
        mega_vdp_syncPaletteCheckboxes(st->ui, ctx);
        st->ui->root->render(st->ui->root, ctx);
    }
}

static e9ui_component_t *
mega_vdp_makeOverlayBodyHost(mega_vdp_state_t *ui)
{
    if (!ui || !ui->root) {
        return NULL;
    }
    e9ui_component_t *host = (e9ui_component_t *)alloc_calloc(1, sizeof(*host));
    if (!host) {
        return NULL;
    }
    mega_vdp_overlay_body_state_t *st = (mega_vdp_overlay_body_state_t *)alloc_calloc(1, sizeof(*st));
    if (!st) {
        alloc_free(host);
        return NULL;
    }
    st->ui = ui;
    host->name = "mega_vdp_overlay_body";
    host->state = st;
    host->preferredHeight = mega_vdp_overlayBodyPreferredHeight;
    host->layout = mega_vdp_overlayBodyLayout;
    host->render = mega_vdp_overlayBodyRender;
    e9ui_child_add(host, ui->root, alloc_strdup("mega_vdp_root"));
    return host;
}

static void
mega_vdp_overlayWindowCloseRequested(e9ui_window_t *window, void *user)
{
    (void)window;
    (void)user;
    mega_vdp_toggle();
}

static int
mega_vdp_init(void)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    if (ui->windowState.open) {
        return 1;
    }
    ui->windowState.windowHost = e9ui_windowCreate(mega_vdp_windowBackend());
    if (!ui->windowState.windowHost) {
        return 0;
    }
    ui->paletteGreyscaleMask = mega_palette_debug_getGreyscaleMask() & 0x0fu;
    ui->root = mega_vdp_buildRoot(ui);
    if (!ui->root) {
        e9ui_windowDestroy(ui->windowState.windowHost);
        ui->windowState.windowHost = NULL;
        return 0;
    }

    e9ui_rect_t rect = e9ui_windowResolveStateOpenRect(&e9ui->ctx,
                                                       mega_vdp_windowDefaultRect(&e9ui->ctx),
                                                       &ui->windowState);
    e9ui_component_t *overlayBodyHost = mega_vdp_makeOverlayBodyHost(ui);
    e9ui_component_t *scroll = NULL;
    if (!overlayBodyHost) {
        e9ui_windowDestroy(ui->windowState.windowHost);
        ui->windowState.windowHost = NULL;
        ui->root = NULL;
        return 0;
    }
    scroll = e9ui_scroll_make(overlayBodyHost);
    e9ui_windowOpen(ui->windowState.windowHost,
                    "MEGADRIVE VDP",
                    rect,
                    scroll ? scroll : overlayBodyHost,
                    mega_vdp_overlayWindowCloseRequested,
                    ui,
                    &e9ui->ctx);
    ui->ctx = e9ui->ctx;
    ui->windowState.open = 1;
    mega_vdp_applyAllOptions();
    aux_window_register(&mega_vdp_auxWindowOps, ui);
    return 1;
}

static void
mega_vdp_shutdown(void)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    if (!ui->windowState.open) {
        return;
    }
    aux_window_unregister(&mega_vdp_auxWindowOps, ui);
    if (ui->windowState.windowHost) {
        e9ui_windowDestroy(ui->windowState.windowHost);
        ui->windowState.windowHost = NULL;
    }
    ui->root = NULL;
    memset(ui->paletteCheckboxes, 0, sizeof(ui->paletteCheckboxes));
    ui->windowState.open = 0;
    memset(&ui->ctx, 0, sizeof(ui->ctx));
}

void
mega_vdp_toggle(void)
{
    if (mega_vdp_isOpen()) {
        mega_vdp_shutdown();
        return;
    }
    (void)mega_vdp_init();
}

int
mega_vdp_isOpen(void)
{
    return mega_vdp_state.windowState.open ? 1 : 0;
}

void
mega_vdp_render(void)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    if (!ui->windowState.open || !ui->root) {
        return;
    }
    if (e9ui_windowCaptureStateRectChanged(&ui->windowState, &e9ui->ctx)) {
        config_saveConfig();
    }
}

void
mega_vdp_persistConfig(FILE *file)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    if (!file) {
        return;
    }
    e9ui_windowPersistStateRect(file, "comp.mega_vdp", &ui->windowState, &e9ui->ctx);
    fprintf(file, "comp.mega_vdp.sprites=%d\n", ui->spritesEnabled ? 1 : 0);
    fprintf(file, "comp.mega_vdp.plane_a=%d\n", ui->planeAEnabled ? 1 : 0);
    fprintf(file, "comp.mega_vdp.plane_b=%d\n", ui->planeBEnabled ? 1 : 0);
}

int
mega_vdp_loadConfigProperty(const char *prop, const char *value)
{
    mega_vdp_state_t *ui = &mega_vdp_state;
    if (!prop || !value) {
        return 0;
    }
    int intValue = 0;
    if (strcmp(prop, "win_x") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winX = intValue;
    } else if (strcmp(prop, "win_y") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winY = intValue;
    } else if (strcmp(prop, "win_w") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winW = intValue;
    } else if (strcmp(prop, "win_h") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winH = intValue;
    } else if (strcmp(prop, "sprites") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->spritesEnabled = intValue ? 1 : 0;
        return 1;
    } else if (strcmp(prop, "plane_a") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->planeAEnabled = intValue ? 1 : 0;
        return 1;
    } else if (strcmp(prop, "plane_b") == 0) {
        if (!mega_vdp_parseInt(value, &intValue)) {
            return 0;
        }
        ui->planeBEnabled = intValue ? 1 : 0;
        return 1;
    } else {
        return 0;
    }
    ui->windowState.winHasSaved = e9ui_windowHasSavedPosition(ui->windowState.winX, ui->windowState.winY);
    return 1;
}
