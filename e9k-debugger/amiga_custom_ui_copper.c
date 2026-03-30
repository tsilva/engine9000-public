/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdio.h>

#include "amiga_custom_ui_internal.h"
#include "emu_ami.h"
#include "e9ui_textbox.h"
#include "libretro_host.h"

void
amiga_custom_ui_copper_statsChartSetValues(e9ui_component_t *comp,
                                 int hasCopperStats,
                                 uint32_t copperSlotsFrame,
                                 uint32_t copperSlotsMaxFrame)
{
    if (!comp || !comp->state) {
        return;
    }
    amiga_custom_ui_copper_stats_chart_state_t *st = (amiga_custom_ui_copper_stats_chart_state_t *)comp->state;
    st->hasCopperStats = hasCopperStats ? 1 : 0;
    st->copperSlotsFrame = copperSlotsFrame;
    st->copperSlotsMaxFrame = copperSlotsMaxFrame;
}


static int
amiga_custom_ui_copper_statsChartPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)availW;
    if (!self || !self->state || !ctx) {
        return 0;
    }
    amiga_custom_ui_copper_stats_chart_state_t *st = (amiga_custom_ui_copper_stats_chart_state_t *)self->state;
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
amiga_custom_ui_copper_statsChartLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}


static void
amiga_custom_ui_copper_statsChartRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !self->state || !ctx || !ctx->renderer) {
        return;
    }

    amiga_custom_ui_copper_stats_chart_state_t *st = (amiga_custom_ui_copper_stats_chart_state_t *)self->state;
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
                                     "Copper",
                                     st->hasCopperStats,
                                     st->copperSlotsFrame,
                                     st->copperSlotsMaxFrame,
                                     "slots/frame",
                                     0,
                                     amiga_custom_ui_common_dmaColorCopper,
                                     0,
                                     0);
    (void)bottomPad;
}


static void
amiga_custom_ui_copper_statsChartDtor(e9ui_component_t *self, e9ui_context_t *ctx)
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
amiga_custom_ui_copper_statsChartMake(void)
{
    e9ui_component_t *comp = (e9ui_component_t *)alloc_calloc(1, sizeof(*comp));
    amiga_custom_ui_copper_stats_chart_state_t *st =
        (amiga_custom_ui_copper_stats_chart_state_t *)alloc_calloc(1, sizeof(*st));
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

    comp->name = "amiga_custom_ui_copper_stats_chart";
    comp->state = st;
    comp->preferredHeight = amiga_custom_ui_copper_statsChartPreferredHeight;
    comp->layout = amiga_custom_ui_copper_statsChartLayout;
    comp->render = amiga_custom_ui_copper_statsChartRender;
    comp->dtor = amiga_custom_ui_copper_statsChartDtor;
    return comp;
}


void
amiga_custom_ui_copper_applyLimitEnabledOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_COPPER_LINE_LIMIT_ENABLED, ui->copperLimitEnabled ? 1u : 0u);
}


void
amiga_custom_ui_copper_applyLimitStartOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    ui->copperLimitStart = amiga_custom_ui_common_clampCopperLine(ui->copperLimitStart);
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_COPPER_LINE_LIMIT_START, (uint32_t)ui->copperLimitStart);
}


void
amiga_custom_ui_copper_applyLimitEndOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    ui->copperLimitEnd = amiga_custom_ui_common_clampCopperLine(ui->copperLimitEnd);
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_COPPER_LINE_LIMIT_END, (uint32_t)ui->copperLimitEnd);
}


void
amiga_custom_ui_copper_syncLimitSuboptions(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    int disabled = ui->copperLimitEnabled ? 0 : 1;
    amiga_custom_ui_common_setComponentDisabled(ui->copperLimitStartRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->copperLimitStartTextbox, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->copperLimitEndRow, disabled);
    amiga_custom_ui_common_setComponentDisabled(ui->copperLimitEndTextbox, disabled);
}


void
amiga_custom_ui_copper_syncVisualiserCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    ui->copperVisualiserEnabled = emu_ami_getCopperDebugEnabled() ? 1 : 0;
    if (ui->copperVisualiserCheckbox) {
        e9ui_checkbox_setSelected(ui->copperVisualiserCheckbox, ui->copperVisualiserEnabled, &ui->ctx);
    }
}


void
amiga_custom_ui_copper_limitChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    ui->copperLimitEnabled = selected ? 1 : 0;
    amiga_custom_ui_copper_applyLimitEnabledOption();
    amiga_custom_ui_copper_syncLimitSuboptions(ui);
}


void
amiga_custom_ui_copper_visualiserChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    ui->copperVisualiserEnabled = selected ? 1 : 0;
    emu_ami_setCopperDebugEnabled(ui->copperVisualiserEnabled);
}


void
amiga_custom_ui_copper_limitStartChanged(e9ui_context_t *ctx, e9ui_component_t *comp, const char *value, void *user)
{
    (void)ctx;
    (void)comp;
    (void)value;
    (void)user;
}


void
amiga_custom_ui_copper_limitEndChanged(e9ui_context_t *ctx, e9ui_component_t *comp, const char *value, void *user)
{
    (void)ctx;
    (void)comp;
    (void)value;
    (void)user;
}


static void
amiga_custom_ui_copper_commitLimitStartTextbox(amiga_custom_ui_state_t *ui)
{
    if (!ui || !ui->copperLimitStartTextbox) {
        return;
    }
    if (!ui->copperLimitEnabled) {
        return;
    }

    const char *value = e9ui_textbox_getText(ui->copperLimitStartTextbox);
    int nextStart = 0;
    if (!value || sscanf(value, "%d", &nextStart) != 1) {
        return;
    }
    nextStart = amiga_custom_ui_common_clampCopperLine(nextStart);
    if (nextStart == ui->copperLimitStart) {
        return;
    }
    ui->copperLimitStart = nextStart;
    amiga_custom_ui_copper_applyLimitStartOption();
}


static void
amiga_custom_ui_copper_commitLimitEndTextbox(amiga_custom_ui_state_t *ui)
{
    if (!ui || !ui->copperLimitEndTextbox) {
        return;
    }
    if (!ui->copperLimitEnabled) {
        return;
    }

    const char *value = e9ui_textbox_getText(ui->copperLimitEndTextbox);
    int nextEnd = 0;
    if (!value || sscanf(value, "%d", &nextEnd) != 1) {
        return;
    }
    nextEnd = amiga_custom_ui_common_clampCopperLine(nextEnd);
    if (nextEnd == ui->copperLimitEnd) {
        return;
    }
    ui->copperLimitEnd = nextEnd;
    amiga_custom_ui_copper_applyLimitEndOption();
}


int
amiga_custom_ui_copper_limitStartTextboxKey(e9ui_context_t *ctx, SDL_Keycode key, SDL_Keymod mods, void *user)
{
    (void)ctx;
    (void)mods;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui || !ui->copperLimitStartTextbox) {
        return 0;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        amiga_custom_ui_copper_commitLimitStartTextbox(ui);
    }
    return 0;
}


int
amiga_custom_ui_copper_limitEndTextboxKey(e9ui_context_t *ctx, SDL_Keycode key, SDL_Keymod mods, void *user)
{
    (void)ctx;
    (void)mods;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui || !ui->copperLimitEndTextbox) {
        return 0;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        amiga_custom_ui_copper_commitLimitEndTextbox(ui);
    }
    return 0;
}


void
amiga_custom_ui_copper_tickLimitTextboxes(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    int startHasFocus = e9ui_getFocus(&ui->ctx) == ui->copperLimitStartTextbox ? 1 : 0;
    if (ui->copperLimitStartTextboxHadFocus && !startHasFocus) {
        amiga_custom_ui_copper_commitLimitStartTextbox(ui);
    }
    ui->copperLimitStartTextboxHadFocus = startHasFocus;

    int endHasFocus = e9ui_getFocus(&ui->ctx) == ui->copperLimitEndTextbox ? 1 : 0;
    if (ui->copperLimitEndTextboxHadFocus && !endHasFocus) {
        amiga_custom_ui_copper_commitLimitEndTextbox(ui);
    }
    ui->copperLimitEndTextboxHadFocus = endHasFocus;
}
