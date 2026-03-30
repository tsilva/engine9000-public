/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdint.h>

#include "amiga_custom_ui_internal.h"
#include "libretro_host.h"

void
amiga_custom_ui_dma_blitterStatsChartSetValues(e9ui_component_t *comp,
                                        int hasBlitterDmaStats,
                                        uint32_t blitterSlotsFrame,
                                        uint32_t blitterSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_blitter_dma_stats_chart_state_t *st = (amiga_custom_ui_blitter_dma_stats_chart_state_t *)comp->state;
    st->hasBlitterDmaStats = hasBlitterDmaStats ? 1 : 0;
    st->blitterSlotsFrame = blitterSlotsFrame;
    st->blitterSlotsMaxFrame = blitterSlotsMaxFrame;
}


void
amiga_custom_ui_dma_cpuStatsChartSetValues(e9ui_component_t *comp,
                                    int hasCpuDmaStats,
                                    uint32_t cpuSlotsFrame,
                                    uint32_t cpuSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_cpu_dma_stats_chart_state_t *st = (amiga_custom_ui_cpu_dma_stats_chart_state_t *)comp->state;
    st->hasCpuDmaStats = hasCpuDmaStats ? 1 : 0;
    st->cpuSlotsFrame = cpuSlotsFrame;
    st->cpuSlotsMaxFrame = cpuSlotsMaxFrame;
}


void
amiga_custom_ui_dma_bitplaneStatsChartSetValues(e9ui_component_t *comp,
                                         int hasBitplaneDmaStats,
                                         uint32_t bitplaneSlotsFrame,
                                         uint32_t bitplaneSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_bitplane_dma_stats_chart_state_t *st = (amiga_custom_ui_bitplane_dma_stats_chart_state_t *)comp->state;
    st->hasBitplaneDmaStats = hasBitplaneDmaStats ? 1 : 0;
    st->bitplaneSlotsFrame = bitplaneSlotsFrame;
    st->bitplaneSlotsMaxFrame = bitplaneSlotsMaxFrame;
}


void
amiga_custom_ui_dma_spriteStatsChartSetValues(e9ui_component_t *comp,
                                       int hasSpriteDmaStats,
                                       uint32_t spriteSlotsFrame,
                                       uint32_t spriteSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_sprite_dma_stats_chart_state_t *st = (amiga_custom_ui_sprite_dma_stats_chart_state_t *)comp->state;
    st->hasSpriteDmaStats = hasSpriteDmaStats ? 1 : 0;
    st->spriteSlotsFrame = spriteSlotsFrame;
    st->spriteSlotsMaxFrame = spriteSlotsMaxFrame;
}


void
amiga_custom_ui_dma_diskStatsChartSetValues(e9ui_component_t *comp,
                                     int hasDiskDmaStats,
                                     uint32_t diskSlotsFrame,
                                     uint32_t diskSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_disk_dma_stats_chart_state_t *st = (amiga_custom_ui_disk_dma_stats_chart_state_t *)comp->state;
    st->hasDiskDmaStats = hasDiskDmaStats ? 1 : 0;
    st->diskSlotsFrame = diskSlotsFrame;
    st->diskSlotsMaxFrame = diskSlotsMaxFrame;
}


void
amiga_custom_ui_dma_audioStatsChartSetValues(e9ui_component_t *comp,
                                      int hasAudioDmaStats,
                                      uint32_t audioSlotsFrame,
                                      uint32_t audioSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_audio_dma_stats_chart_state_t *st = (amiga_custom_ui_audio_dma_stats_chart_state_t *)comp->state;
    st->hasAudioDmaStats = hasAudioDmaStats ? 1 : 0;
    st->audioSlotsFrame = audioSlotsFrame;
    st->audioSlotsMaxFrame = audioSlotsMaxFrame;
}


void
amiga_custom_ui_dma_otherStatsChartSetValues(e9ui_component_t *comp,
                                      int hasOtherDmaStats,
                                      uint32_t otherSlotsFrame,
                                      uint32_t otherSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_other_dma_stats_chart_state_t *st = (amiga_custom_ui_other_dma_stats_chart_state_t *)comp->state;
    st->hasOtherDmaStats = hasOtherDmaStats ? 1 : 0;
    st->otherSlotsFrame = otherSlotsFrame;
    st->otherSlotsMaxFrame = otherSlotsMaxFrame;
}


void
amiga_custom_ui_dma_idleStatsChartSetValues(e9ui_component_t *comp,
                                     int hasIdleDmaStats,
                                     uint32_t idleSlotsFrame,
                                     uint32_t idleSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_idle_dma_stats_chart_state_t *st = (amiga_custom_ui_idle_dma_stats_chart_state_t *)comp->state;
    st->hasIdleDmaStats = hasIdleDmaStats ? 1 : 0;
    st->idleSlotsFrame = idleSlotsFrame;
    st->idleSlotsMaxFrame = idleSlotsMaxFrame;
}


void
amiga_custom_ui_dma_totalMixChartSetValues(e9ui_component_t *comp,
                                    int hasStats,
                                    uint32_t totalSlotsFrame,
                                    uint32_t totalSlotsMaxFrame,
                                    uint32_t cpuSlotsFrame,
                                    uint32_t copperSlotsFrame,
                                    uint32_t audioSlotsFrame,
                                    uint32_t blitterSlotsFrame,
                                    uint32_t bitplaneSlotsFrame,
                                    uint32_t spriteSlotsFrame,
                                    uint32_t diskSlotsFrame,
                                    uint32_t otherSlotsFrame,
                                    uint32_t idleSlotsFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_dma_total_mix_chart_state_t *st = (amiga_custom_ui_dma_total_mix_chart_state_t *)comp->state;
    st->hasStats = hasStats ? 1 : 0;
    st->totalSlotsFrame = totalSlotsFrame;
    st->totalSlotsMaxFrame = totalSlotsMaxFrame;
    st->cpuSlotsFrame = cpuSlotsFrame;
    st->copperSlotsFrame = copperSlotsFrame;
    st->audioSlotsFrame = audioSlotsFrame;
    st->blitterSlotsFrame = blitterSlotsFrame;
    st->bitplaneSlotsFrame = bitplaneSlotsFrame;
    st->spriteSlotsFrame = spriteSlotsFrame;
    st->diskSlotsFrame = diskSlotsFrame;
    st->otherSlotsFrame = otherSlotsFrame;
    st->idleSlotsFrame = idleSlotsFrame;
}


static int
amiga_custom_ui_dma_blitterStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_blitter_dma_stats_chart_state_t *st = (amiga_custom_ui_blitter_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_blitterStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_blitterStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_blitter_dma_stats_chart_state_t *st = (amiga_custom_ui_blitter_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Blitter",
                                     st->hasBlitterDmaStats,
                                     st->blitterSlotsFrame,
                                     st->blitterSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorBlitter,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_blitterStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_blitterStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_blitter_dma_stats_chart_state_t *st =
        (amiga_custom_ui_blitter_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_blitter_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_blitterStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_blitterStatsChartLayout;
    comp->render = amiga_custom_ui_dma_blitterStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_blitterStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_cpuStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_cpu_dma_stats_chart_state_t *st = (amiga_custom_ui_cpu_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_cpuStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_cpuStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_cpu_dma_stats_chart_state_t *st = (amiga_custom_ui_cpu_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "CPU",
                                     st->hasCpuDmaStats,
                                     st->cpuSlotsFrame,
                                     st->cpuSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorCpu,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_cpuStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_cpuStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_cpu_dma_stats_chart_state_t *st =
        (amiga_custom_ui_cpu_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_cpu_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_cpuStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_cpuStatsChartLayout;
    comp->render = amiga_custom_ui_dma_cpuStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_cpuStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_bitplaneStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_bitplane_dma_stats_chart_state_t *st = (amiga_custom_ui_bitplane_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_bitplaneStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_bitplaneStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_bitplane_dma_stats_chart_state_t *st = (amiga_custom_ui_bitplane_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Bitplane",
                                     st->hasBitplaneDmaStats,
                                     st->bitplaneSlotsFrame,
                                     st->bitplaneSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorBitplane,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_bitplaneStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_bitplaneStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_bitplane_dma_stats_chart_state_t *st =
        (amiga_custom_ui_bitplane_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_bitplane_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_bitplaneStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_bitplaneStatsChartLayout;
    comp->render = amiga_custom_ui_dma_bitplaneStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_bitplaneStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_spriteStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_sprite_dma_stats_chart_state_t *st = (amiga_custom_ui_sprite_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_spriteStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_spriteStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_sprite_dma_stats_chart_state_t *st = (amiga_custom_ui_sprite_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Sprite",
                                     st->hasSpriteDmaStats,
                                     st->spriteSlotsFrame,
                                     st->spriteSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorSprite,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_spriteStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_spriteStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_sprite_dma_stats_chart_state_t *st =
        (amiga_custom_ui_sprite_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_sprite_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_spriteStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_spriteStatsChartLayout;
    comp->render = amiga_custom_ui_dma_spriteStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_spriteStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_diskStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_disk_dma_stats_chart_state_t *st = (amiga_custom_ui_disk_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_diskStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_diskStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_disk_dma_stats_chart_state_t *st = (amiga_custom_ui_disk_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Disk",
                                     st->hasDiskDmaStats,
                                     st->diskSlotsFrame,
                                     st->diskSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorDisk,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_diskStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_diskStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_disk_dma_stats_chart_state_t *st =
        (amiga_custom_ui_disk_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_disk_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_diskStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_diskStatsChartLayout;
    comp->render = amiga_custom_ui_dma_diskStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_diskStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_audioStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_audio_dma_stats_chart_state_t *st = (amiga_custom_ui_audio_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_audioStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_audioStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_audio_dma_stats_chart_state_t *st = (amiga_custom_ui_audio_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Audio",
                                     st->hasAudioDmaStats,
                                     st->audioSlotsFrame,
                                     st->audioSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorAudio,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_audioStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_audioStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_audio_dma_stats_chart_state_t *st =
        (amiga_custom_ui_audio_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_audio_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_audioStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_audioStatsChartLayout;
    comp->render = amiga_custom_ui_dma_audioStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_audioStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_otherStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_other_dma_stats_chart_state_t *st = (amiga_custom_ui_other_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_otherStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_otherStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_other_dma_stats_chart_state_t *st = (amiga_custom_ui_other_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Other",
                                     st->hasOtherDmaStats,
                                     st->otherSlotsFrame,
                                     st->otherSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorOther,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_otherStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_otherStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_other_dma_stats_chart_state_t *st =
        (amiga_custom_ui_other_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_other_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_otherStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_otherStatsChartLayout;
    comp->render = amiga_custom_ui_dma_otherStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_otherStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_idleStatsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_idle_dma_stats_chart_state_t *st = (amiga_custom_ui_idle_dma_stats_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_idleStatsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_idleStatsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_idle_dma_stats_chart_state_t *st = (amiga_custom_ui_idle_dma_stats_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);

    if (fontH <= 0) {
        fontH = 12;
    }
    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    amiga_custom_ui_blitter_statsChartRenderBarRow(ctx,
                                     contentX,
                                     barX,
                                     barW,
                                     barHeight,
                                     labelGap,
                                     contentY,
                                     rowH,
                                     fontH,
                                     "Idle",
                                     st->hasIdleDmaStats,
                                     st->idleSlotsFrame,
                                     st->idleSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorIdle,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_idleStatsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_idleStatsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_idle_dma_stats_chart_state_t *st =
        (amiga_custom_ui_idle_dma_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_idle_dma_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_idleStatsChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_idleStatsChartLayout;
    comp->render = amiga_custom_ui_dma_idleStatsChartRender;
    comp->dtor = amiga_custom_ui_dma_idleStatsChartDtor;
    return comp;
}


static int
amiga_custom_ui_dma_totalMixChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_dma_total_mix_chart_state_t *st = (amiga_custom_ui_dma_total_mix_chart_state_t *)self->state;
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    int barH = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    if (fontH <= 0) {
        fontH = 12;
    }
    int rowH = fontH > barH ? fontH : barH;
    return topPad + rowH + bottomPad;
}


static void
amiga_custom_ui_dma_totalMixChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_dma_totalMixChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_dma_total_mix_chart_state_t *st = (amiga_custom_ui_dma_total_mix_chart_state_t *)self->state;
    int leftInset = e9ui_scale_px(ctx, st->leftInset);
    int rightInset = e9ui_scale_px(ctx, st->rightInset);
    int topPad = e9ui_scale_px(ctx, st->topPadding);
    int bottomPad = e9ui_scale_px(ctx, st->bottomPadding);
    int labelWidth = e9ui_scale_px(ctx, st->labelWidth);
    int labelGap = e9ui_scale_px(ctx, st->labelGap);
    int barHeight = amiga_custom_ui_common_textboxLikeHeight(ctx);
    int fontH = ctx->font ? TTF_FontHeight(ctx->font) : e9ui_scale_px(ctx, 12);
    if (fontH <= 0) {
        fontH = 12;
    }

    int contentX = self->bounds.x + leftInset;
    int contentY = self->bounds.y + topPad;
    int contentW = self->bounds.w - leftInset - rightInset;
    if (contentW < 1) {
        contentW = 1;
    }

    int rowH = fontH > barHeight ? fontH : barHeight;
    int barX = contentX + labelWidth + labelGap;
    int barW = contentW - labelWidth - labelGap;
    if (barW < 1) {
        barW = 1;
    }
    {
        TTF_Font *labelFont = e9ui->theme.text.prompt ? e9ui->theme.text.prompt : ctx->font;
        int labelTextW = 0;
        if (labelFont) {
            TTF_SizeText(labelFont, "Total", &labelTextW, NULL);
        }
        int labelX = barX - labelGap - labelTextW;
        if (labelX < contentX) {
            labelX = contentX;
        }
        amiga_custom_ui_common_blitterStatsChartDrawText(ctx,
                                            labelFont,
                                            "Total",
                                            amiga_custom_ui_common_blitterStatsChartLabelColor,
                                            labelX,
                                            contentY + (rowH - fontH) / 2);
    }

    SDL_Rect trackRect = { barX, contentY + (rowH - barHeight) / 2, barW, barHeight };
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

    if (st->hasStats && st->totalSlotsMaxFrame > 0u && innerRect.w > 0 && innerRect.h > 0) {
        const uint32_t values[] = {
            st->cpuSlotsFrame,
            st->copperSlotsFrame,
            st->audioSlotsFrame,
            st->blitterSlotsFrame,
            st->bitplaneSlotsFrame,
            st->spriteSlotsFrame,
            st->diskSlotsFrame,
            st->otherSlotsFrame,
            st->idleSlotsFrame
        };
        const SDL_Color colors[] = {
            amiga_custom_ui_common_dmaColorCpu,
            amiga_custom_ui_common_dmaColorCopper,
            amiga_custom_ui_common_dmaColorAudio,
            amiga_custom_ui_common_dmaColorBlitter,
            amiga_custom_ui_common_dmaColorBitplane,
            amiga_custom_ui_common_dmaColorSprite,
            amiga_custom_ui_common_dmaColorDisk,
            amiga_custom_ui_common_dmaColorOther,
            amiga_custom_ui_common_dmaColorIdle
        };
        uint64_t accum = 0u;
        int drawX = innerRect.x;
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
            uint64_t prev = accum;
            accum += (uint64_t)values[i];
            if (accum > (uint64_t)st->totalSlotsMaxFrame) {
                accum = (uint64_t)st->totalSlotsMaxFrame;
            }
            int x0 = innerRect.x + (int)((prev * (uint64_t)(uint32_t)innerRect.w) / (uint64_t)st->totalSlotsMaxFrame);
            int x1 = innerRect.x + (int)((accum * (uint64_t)(uint32_t)innerRect.w) / (uint64_t)st->totalSlotsMaxFrame);
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
            SDL_SetRenderDrawColor(ctx->renderer, amiga_custom_ui_common_dmaColorIdle.r, amiga_custom_ui_common_dmaColorIdle.g, amiga_custom_ui_common_dmaColorIdle.b, 255);
            SDL_RenderFillRect(ctx->renderer, &seg);
        }
    }

    SDL_SetRenderDrawColor(ctx->renderer, 64, 72, 82, 255);
    SDL_RenderDrawRect(ctx->renderer, &trackRect);
    (void)bottomPad;
}


static void
amiga_custom_ui_dma_totalMixChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self) {
        return;
    }
    if (self->state) {
        alloc_free(self->state);
        self->state = NULL;
    }
}


e9ui_component_t *
amiga_custom_ui_dma_totalMixChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_dma_total_mix_chart_state_t *st =
        (amiga_custom_ui_dma_total_mix_chart_state_t *)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        if (comp) {
            alloc_free(comp);
        }
        if (st) {
            alloc_free(st);
        }
        return NULL;
    }

    st->leftInset = 0;
    st->rightInset = 14;
    st->topPadding = 2;
    st->bottomPadding = 2;
    st->rowGap = 4;
    st->labelWidth = 78;
    st->labelGap = 8;
    st->barHeight = 0;

    comp->name = "amiga_custom_ui_dma_total_mix_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_dma_totalMixChartPreferredHeight;
    comp->layout = amiga_custom_ui_dma_totalMixChartLayout;
    comp->render = amiga_custom_ui_dma_totalMixChartRender;
    comp->dtor = amiga_custom_ui_dma_totalMixChartDtor;
    return comp;
}


void
amiga_custom_ui_dma_enableDebugForCopperStats(amiga_custom_ui_state_t *ui)
{
    if (!ui || ui->dmaDebugAutoEnabled) {
        return;
    }
    int *debugDma = NULL;
    if (!libretro_host_debugGetAmigaDebugDmaAddr(&debugDma) || !debugDma) {
        return;
    }
    if (*debugDma == 0) {
        ui->dmaDebugAutoPrevValue = 0;
        *debugDma = (int)E9K_DEBUG_AMI_DMA_DEBUG_MODE_COLLECT_ONLY;
        ui->dmaDebugAutoEnabled = 1;
    }
}


void
amiga_custom_ui_dma_restoreDebugForCopperStats(amiga_custom_ui_state_t *ui)
{
    if (!ui || !ui->dmaDebugAutoEnabled) {
        return;
    }
    int *debugDma = NULL;
    if (libretro_host_debugGetAmigaDebugDmaAddr(&debugDma) && debugDma) {
        if (*debugDma == (int)E9K_DEBUG_AMI_DMA_DEBUG_MODE_COLLECT_ONLY) {
            *debugDma = ui->dmaDebugAutoPrevValue;
        }
    }
    ui->dmaDebugAutoEnabled = 0;
}


void
amiga_custom_ui_dma_updateCopperStatsChart(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }

    const e9k_debug_ami_dma_debug_frame_view_t *probedFrame = NULL;
    int haveProbe = 0;
    if (ui->dmaStatsEnabled) {
        probedFrame = libretro_host_debugAmiGetDmaDebugFrameView(
            E9K_DEBUG_AMI_DMA_DEBUG_FRAME_LATEST_COMPLETE);
        if (probedFrame &&
            probedFrame->records &&
            probedFrame->info.recordCount > 0u &&
            probedFrame->info.frameNumber >= 0) {
            haveProbe = 1;
            if (ui->dmaStatsCacheValid &&
                ui->dmaStatsCacheFrameSelect == (int)E9K_DEBUG_AMI_DMA_DEBUG_FRAME_LATEST_COMPLETE &&
                ui->dmaStatsCacheFrameNumber == probedFrame->info.frameNumber) {
                return;
            }
        }
    }

    int hasCopperStats = 0;
    uint32_t copperSlotsFrame = 0u;
    uint32_t copperSlotsMaxFrame = 0u;
    int hasBlitterDmaStats = 0;
    uint32_t blitterSlotsFrame = 0u;
    uint32_t blitterSlotsMaxFrame = 0u;
    int hasCpuDmaStats = 0;
    uint32_t cpuSlotsFrame = 0u;
    uint32_t cpuSlotsMaxFrame = 0u;
    int hasBitplaneDmaStats = 0;
    uint32_t bitplaneSlotsFrame = 0u;
    uint32_t bitplaneSlotsMaxFrame = 0u;
    int hasSpriteDmaStats = 0;
    uint32_t spriteSlotsFrame = 0u;
    uint32_t spriteSlotsMaxFrame = 0u;
    int hasDiskDmaStats = 0;
    uint32_t diskSlotsFrame = 0u;
    uint32_t diskSlotsMaxFrame = 0u;
    int hasAudioDmaStats = 0;
    uint32_t audioSlotsFrame = 0u;
    uint32_t audioSlotsMaxFrame = 0u;
    int hasOtherDmaStats = 0;
    uint32_t otherSlotsFrame = 0u;
    uint32_t otherSlotsMaxFrame = 0u;
    int hasIdleDmaStats = 0;
    uint32_t idleSlotsFrame = 0u;
    uint32_t idleSlotsMaxFrame = 0u;
    int hasDmaTotalStats = 0;
    uint32_t dmaTotalSlotsFrame = 0u;
    uint32_t dmaTotalSlotsMaxFrame = 0u;
    uint32_t dmaSlotsAvailableFrame = 0u;
    if (ui->dmaStatsEnabled) {
        int *debugDma = NULL;
        libretro_host_debugGetAmigaDebugDmaAddr(&debugDma);
        uint32_t dmaFrameSelect = E9K_DEBUG_AMI_DMA_DEBUG_FRAME_LATEST_COMPLETE;
        const e9k_debug_ami_dma_debug_frame_view_t *dmaFrame = probedFrame;
        if (!haveProbe) {
            dmaFrame = libretro_host_debugAmiGetDmaDebugFrameView(dmaFrameSelect);
        }
        if (dmaFrame && dmaFrame->records && dmaFrame->info.recordCount > 0u) {
            size_t readTotal = dmaFrame->info.recordCount;
            e9k_debug_ami_dma_debug_frame_info_t readInfo = dmaFrame->info;
            const e9k_debug_ami_dma_debug_raw_record_t *records = dmaFrame->records;
            int stride = readInfo.hposCount;
            if (stride <= 0) {
                stride = 288;
            }
            size_t rowCount = readTotal / (size_t)stride;
            int xLimit = stride;
            int yLimit = (int)rowCount - 1;
            int videoLineCount = 0;
            if (libretro_host_debugAmiGetVideoLineCount(&videoLineCount) && videoLineCount > 0) {
                int debugDmaMode = (debugDma && *debugDma > 0) ? *debugDma : 0;
                int visibleMax = videoLineCount - 1;
                if (debugDmaMode == (int)E9K_DEBUG_AMI_DMA_DEBUG_MODE_COLLECT_ONLY ||
                    debugDmaMode == (int)E9K_DEBUG_AMI_DMA_DEBUG_MODE_VIDEO_SYNC) {
                    // These modes do not use the legacy core-side overlay scaling.
                } else if (debugDmaMode >= 4) {
                    visibleMax /= 2;
                } else {
                    visibleMax -= 8;
                }
                if (visibleMax < 0) {
                    visibleMax = 0;
                }
                if (yLimit > visibleMax) {
                    yLimit = visibleMax;
                }
            }
            if (yLimit >= (int)rowCount) {
                yLimit = (int)rowCount - 1;
            }
            if (yLimit < -1) {
                yLimit = -1;
            }
            for (int row = 0; row <= yLimit; ++row) {
                size_t rowBase = (size_t)row * (size_t)stride;
                for (int x = 0; x < xLimit; ++x) {
                    const e9k_debug_ami_dma_debug_raw_record_t *rec = &records[rowBase + (size_t)x];
                    if (rec->end) {
                        break;
                    }
                    if (rec->reg != 0xffffu) {
                        switch (rec->type) {
                        case AMIGA_CUSTOM_UI_DMA_RECORD_COPPER:
                            if (copperSlotsFrame < UINT32_MAX) {
                                copperSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_BLITTER:
                            if (blitterSlotsFrame < UINT32_MAX) {
                                blitterSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_CPU:
                            if (cpuSlotsFrame < UINT32_MAX) {
                                cpuSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_BITPLANE:
                            if (bitplaneSlotsFrame < UINT32_MAX) {
                                bitplaneSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_SPRITE:
                            if (spriteSlotsFrame < UINT32_MAX) {
                                spriteSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_DISK:
                            if (diskSlotsFrame < UINT32_MAX) {
                                diskSlotsFrame++;
                            }
                            continue;
                        case AMIGA_CUSTOM_UI_DMA_RECORD_AUDIO:
                            if (audioSlotsFrame < UINT32_MAX) {
                                audioSlotsFrame++;
                            }
                            continue;
                        default:
                            break;
                        }
                    }
                    if (rec->type == (int16_t)AMIGA_CUSTOM_UI_DMA_RECORD_REFRESH ||
                        rec->type == (int16_t)AMIGA_CUSTOM_UI_DMA_RECORD_CONFLICT ||
                        rec->type != 0) {
                        if (otherSlotsFrame < UINT32_MAX) {
                            otherSlotsFrame++;
                        }
                    } else {
                        if (idleSlotsFrame < UINT32_MAX) {
                            idleSlotsFrame++;
                        }
                    }
                }
            }
            if (xLimit > 0 && yLimit >= 0) {
                        uint64_t max64 = (uint64_t)(uint32_t)(yLimit + 1) *
                                         (uint64_t)((uint32_t)xLimit / 2u);
                        uint64_t totalAvailable64 = (uint64_t)(uint32_t)(yLimit + 1) *
                                                    (uint64_t)(uint32_t)xLimit;
                        if (max64 > 0xffffffffu) {
                            max64 = 0xffffffffu;
                        }
                        if (totalAvailable64 > 0xffffffffu) {
                            totalAvailable64 = 0xffffffffu;
                        }
                        copperSlotsMaxFrame = (uint32_t)max64;
                        hasCopperStats = 1;
                        blitterSlotsMaxFrame = (uint32_t)max64;
                        hasBlitterDmaStats = 1;
                        cpuSlotsMaxFrame = (uint32_t)max64;
                        hasCpuDmaStats = 1;
                        bitplaneSlotsMaxFrame = (uint32_t)max64;
                        hasBitplaneDmaStats = 1;
                        spriteSlotsMaxFrame = (uint32_t)max64;
                        hasSpriteDmaStats = 1;
                        diskSlotsMaxFrame = (uint32_t)max64;
                        hasDiskDmaStats = 1;
                        audioSlotsMaxFrame = (uint32_t)max64;
                        hasAudioDmaStats = 1;
                        otherSlotsMaxFrame = (uint32_t)totalAvailable64;
                        hasOtherDmaStats = 1;
                        idleSlotsMaxFrame = (uint32_t)totalAvailable64;
                        hasIdleDmaStats = 1;
                        dmaSlotsAvailableFrame = (uint32_t)totalAvailable64;
            }
            if (!hasCopperStats &&
                readInfo.hposCount > 0 &&
                libretro_host_debugAmiGetVideoLineCount(&videoLineCount) &&
                videoLineCount > 0) {
                        uint64_t max64 = (uint64_t)(uint32_t)videoLineCount *
                                         (uint64_t)((uint32_t)readInfo.hposCount / 2u);
                        uint64_t totalAvailable64 = (uint64_t)(uint32_t)videoLineCount *
                                                    (uint64_t)(uint32_t)readInfo.hposCount;
                        if (max64 > 0xffffffffu) {
                            max64 = 0xffffffffu;
                        }
                        if (totalAvailable64 > 0xffffffffu) {
                            totalAvailable64 = 0xffffffffu;
                        }
                        copperSlotsMaxFrame = (uint32_t)max64;
                        hasCopperStats = 1;
                        blitterSlotsMaxFrame = (uint32_t)max64;
                        hasBlitterDmaStats = 1;
                        cpuSlotsMaxFrame = (uint32_t)max64;
                        hasCpuDmaStats = 1;
                        bitplaneSlotsMaxFrame = (uint32_t)max64;
                        hasBitplaneDmaStats = 1;
                        spriteSlotsMaxFrame = (uint32_t)max64;
                        hasSpriteDmaStats = 1;
                        diskSlotsMaxFrame = (uint32_t)max64;
                        hasDiskDmaStats = 1;
                        audioSlotsMaxFrame = (uint32_t)max64;
                        hasAudioDmaStats = 1;
                        otherSlotsMaxFrame = (uint32_t)totalAvailable64;
                        hasOtherDmaStats = 1;
                        idleSlotsMaxFrame = (uint32_t)totalAvailable64;
                        hasIdleDmaStats = 1;
                        dmaSlotsAvailableFrame = (uint32_t)totalAvailable64;
            }
        }
        if (dmaFrame &&
            dmaFrameSelect == E9K_DEBUG_AMI_DMA_DEBUG_FRAME_LATEST_COMPLETE &&
            dmaFrame->info.frameNumber >= 0) {
            ui->dmaStatsCacheValid = 1;
            ui->dmaStatsCacheFrameSelect = (int)dmaFrameSelect;
            ui->dmaStatsCacheFrameNumber = dmaFrame->info.frameNumber;
        } else {
            ui->dmaStatsCacheValid = 0;
            ui->dmaStatsCacheFrameSelect = 0;
            ui->dmaStatsCacheFrameNumber = -1;
        }
    } else {
        ui->dmaStatsCacheValid = 0;
        ui->dmaStatsCacheFrameSelect = 0;
        ui->dmaStatsCacheFrameNumber = -1;
    }
    if (hasCopperStats && copperSlotsFrame > copperSlotsMaxFrame) {
        copperSlotsMaxFrame = copperSlotsFrame;
    }
    if (hasBlitterDmaStats && blitterSlotsFrame > blitterSlotsMaxFrame) {
        blitterSlotsMaxFrame = blitterSlotsFrame;
    }
    if (hasCpuDmaStats && cpuSlotsFrame > cpuSlotsMaxFrame) {
        cpuSlotsMaxFrame = cpuSlotsFrame;
    }
    if (hasBitplaneDmaStats && bitplaneSlotsFrame > bitplaneSlotsMaxFrame) {
        bitplaneSlotsMaxFrame = bitplaneSlotsFrame;
    }
    if (hasSpriteDmaStats && spriteSlotsFrame > spriteSlotsMaxFrame) {
        spriteSlotsMaxFrame = spriteSlotsFrame;
    }
    if (hasDiskDmaStats && diskSlotsFrame > diskSlotsMaxFrame) {
        diskSlotsMaxFrame = diskSlotsFrame;
    }
    if (hasAudioDmaStats && audioSlotsFrame > audioSlotsMaxFrame) {
        audioSlotsMaxFrame = audioSlotsFrame;
    }
    if (hasOtherDmaStats && otherSlotsFrame > otherSlotsMaxFrame) {
        otherSlotsMaxFrame = otherSlotsFrame;
    }
    if (hasIdleDmaStats && idleSlotsFrame > idleSlotsMaxFrame) {
        idleSlotsMaxFrame = idleSlotsFrame;
    }
    if (hasCopperStats) {
        hasDmaTotalStats = 1;
        dmaTotalSlotsFrame = copperSlotsFrame +
                             blitterSlotsFrame +
                             cpuSlotsFrame +
                             bitplaneSlotsFrame +
                             spriteSlotsFrame +
                             diskSlotsFrame +
                             audioSlotsFrame +
                             otherSlotsFrame +
                             idleSlotsFrame;
        dmaTotalSlotsMaxFrame = dmaSlotsAvailableFrame;
        if (dmaTotalSlotsFrame > dmaTotalSlotsMaxFrame) {
            dmaTotalSlotsMaxFrame = dmaTotalSlotsFrame;
        }
    }

    if (ui->copperStatsChart) {
        amiga_custom_ui_copper_statsChartSetValues(ui->copperStatsChart,
                                            hasCopperStats,
                                            copperSlotsFrame,
                                            copperSlotsMaxFrame);
    }
    if (ui->blitterDmaStatsChart) {
        amiga_custom_ui_dma_blitterStatsChartSetValues(ui->blitterDmaStatsChart,
                                                hasBlitterDmaStats,
                                                blitterSlotsFrame,
                                                blitterSlotsMaxFrame);
    }
    if (ui->cpuDmaStatsChart) {
        amiga_custom_ui_dma_cpuStatsChartSetValues(ui->cpuDmaStatsChart,
                                            hasCpuDmaStats,
                                            cpuSlotsFrame,
                                            cpuSlotsMaxFrame);
    }
    if (ui->bitplaneDmaStatsChart) {
        amiga_custom_ui_dma_bitplaneStatsChartSetValues(ui->bitplaneDmaStatsChart,
                                                 hasBitplaneDmaStats,
                                                 bitplaneSlotsFrame,
                                                 bitplaneSlotsMaxFrame);
    }
    if (ui->spriteDmaStatsChart) {
        amiga_custom_ui_dma_spriteStatsChartSetValues(ui->spriteDmaStatsChart,
                                               hasSpriteDmaStats,
                                               spriteSlotsFrame,
                                               spriteSlotsMaxFrame);
    }
    if (ui->diskDmaStatsChart) {
        amiga_custom_ui_dma_diskStatsChartSetValues(ui->diskDmaStatsChart,
                                             hasDiskDmaStats,
                                             diskSlotsFrame,
                                             diskSlotsMaxFrame);
    }
    if (ui->audioDmaStatsChart) {
        amiga_custom_ui_dma_audioStatsChartSetValues(ui->audioDmaStatsChart,
                                              hasAudioDmaStats,
                                              audioSlotsFrame,
                                              audioSlotsMaxFrame);
    }
    if (ui->otherDmaStatsChart) {
        amiga_custom_ui_dma_otherStatsChartSetValues(ui->otherDmaStatsChart,
                                              hasOtherDmaStats,
                                              otherSlotsFrame,
                                              otherSlotsMaxFrame);
    }
    if (ui->idleDmaStatsChart) {
        amiga_custom_ui_dma_idleStatsChartSetValues(ui->idleDmaStatsChart,
                                             hasIdleDmaStats,
                                             idleSlotsFrame,
                                             idleSlotsMaxFrame);
    }
    if (ui->dmaTotalMixChart) {
        amiga_custom_ui_dma_totalMixChartSetValues(ui->dmaTotalMixChart,
                                            hasDmaTotalStats,
                                            dmaTotalSlotsFrame,
                                            dmaTotalSlotsMaxFrame,
                                            cpuSlotsFrame,
                                            copperSlotsFrame,
                                            audioSlotsFrame,
                                            blitterSlotsFrame,
                                            bitplaneSlotsFrame,
                                            spriteSlotsFrame,
                                            diskSlotsFrame,
                                            otherSlotsFrame,
                                            idleSlotsFrame);
    }
}


void
amiga_custom_ui_dma_updateStatsCharts(amiga_custom_ui_state_t *ui)
{
    amiga_custom_ui_blitter_updateStatsChart(ui);
    amiga_custom_ui_dma_updateCopperStatsChart(ui);
}


void
amiga_custom_ui_dma_syncStatsSuboptions(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    int disabled = ui->dmaStatsEnabled ? 0 : 1;
    amiga_custom_ui_common_setComponentDisabled(ui->copperStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->copperStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->blitterDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->blitterDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->cpuDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->cpuDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->bitplaneDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->bitplaneDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->spriteDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->spriteDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->diskDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->diskDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->audioDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->audioDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->otherDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->otherDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->idleDmaStatsChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->idleDmaStatsChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->dmaTotalMixChartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->dmaTotalMixChart, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->dmaStatsHintTextRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->dmaStatsHintText, disabled);
    amiga_custom_ui_common_syncDmaStatsCycleExactHint(ui);
}


void
amiga_custom_ui_dma_statsChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    ui->dmaStatsEnabled = selected ? 1 : 0;
    if (ui->dmaStatsEnabled) {
        amiga_custom_ui_dma_enableDebugForCopperStats(ui);
    } else {
        amiga_custom_ui_dma_restoreDebugForCopperStats(ui);
    }
    amiga_custom_ui_dma_syncStatsSuboptions(ui);
    amiga_custom_ui_dma_updateStatsCharts(ui);
}
