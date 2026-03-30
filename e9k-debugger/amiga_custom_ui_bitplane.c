/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "amiga_custom_ui_internal.h"

static e9k_debug_option_t
amiga_custom_ui_bitplane_optionForIndex(int bitplaneIndex)
{
    switch (bitplaneIndex) {
        case 0: return E9K_DEBUG_OPTION_AMIGA_BITPLANE0;
        case 1: return E9K_DEBUG_OPTION_AMIGA_BITPLANE1;
        case 2: return E9K_DEBUG_OPTION_AMIGA_BITPLANE2;
        case 3: return E9K_DEBUG_OPTION_AMIGA_BITPLANE3;
        case 4: return E9K_DEBUG_OPTION_AMIGA_BITPLANE4;
        case 5: return E9K_DEBUG_OPTION_AMIGA_BITPLANE5;
        case 6: return E9K_DEBUG_OPTION_AMIGA_BITPLANE6;
        case 7: return E9K_DEBUG_OPTION_AMIGA_BITPLANE7;
        default: break;
    }
    return e9k_debug_option_none;
}


void
amiga_custom_ui_bitplane_applyPaletteVisOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_PALETTE_VIS, ui->paletteVisualiserEnabled ? 1u : 0u);
}


void
amiga_custom_ui_bitplane_applyBplcon1DelayScrollOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_BPLCON1_DELAY_SCROLL, ui->bplcon1DelayScrollEnabled ? 1u : 0u);
}


void
amiga_custom_ui_bitplane_applyOption(int bitplaneIndex)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    if (bitplaneIndex < 0 || bitplaneIndex >= AMIGA_CUSTOM_UI_AMIGA_BITPLANE_COUNT) {
        return;
    }
    e9k_debug_option_t option = amiga_custom_ui_bitplane_optionForIndex(bitplaneIndex);
    if (option == e9k_debug_option_none) {
        return;
    }
    amiga_custom_ui_common_applyOption(option, ui->bitplaneEnabled[bitplaneIndex] ? 1u : 0u);
}


static int
amiga_custom_ui_bitplane_areAllEnabled(const amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return 0;
    }
    for (int bitplaneIndex = 0; bitplaneIndex < AMIGA_CUSTOM_UI_AMIGA_BITPLANE_COUNT; ++bitplaneIndex) {
        if (!ui->bitplaneEnabled[bitplaneIndex]) {
            return 0;
        }
    }
    return 1;
}


void
amiga_custom_ui_bitplane_syncMasterCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    ui->bitplanesEnabled = amiga_custom_ui_bitplane_areAllEnabled(ui) ? 1 : 0;
    if (!ui->bitplanesCheckbox) {
        return;
    }
    ui->suppressBitplaneCallbacks = 1;
    e9ui_checkbox_setSelected(ui->bitplanesCheckbox, ui->bitplanesEnabled, &ui->ctx);
    ui->suppressBitplaneCallbacks = 0;
}


void
amiga_custom_ui_bitplane_paletteVisualiserChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    ui->paletteVisualiserEnabled = selected ? 1 : 0;
    amiga_custom_ui_bitplane_applyPaletteVisOption();
}


void
amiga_custom_ui_bitplane_bplcon1DelayScrollChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    ui->bplcon1DelayScrollEnabled = selected ? 1 : 0;
    amiga_custom_ui_bitplane_applyBplcon1DelayScrollOption();
}


void
amiga_custom_ui_bitplane_masterChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    if (ui->suppressBitplaneCallbacks) {
        return;
    }
    int nextValue = selected ? 1 : 0;
    ui->bitplanesEnabled = nextValue;
    for (int bitplaneIndex = 0; bitplaneIndex < AMIGA_CUSTOM_UI_AMIGA_BITPLANE_COUNT; ++bitplaneIndex) {
        ui->bitplaneEnabled[bitplaneIndex] = nextValue;
        amiga_custom_ui_bitplane_applyOption(bitplaneIndex);
        if (ui->bitplaneCheckboxes[bitplaneIndex]) {
            ui->suppressBitplaneCallbacks = 1;
            e9ui_checkbox_setSelected(ui->bitplaneCheckboxes[bitplaneIndex], nextValue, &ui->ctx);
            ui->suppressBitplaneCallbacks = 0;
        }
    }
}


void
amiga_custom_ui_bitplane_changed(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    if (!user) {
        return;
    }
    struct amiga_custom_ui_bitplane_cb *cb = (struct amiga_custom_ui_bitplane_cb *)user;
    if (!cb->ui) {
        return;
    }
    if (cb->bitplaneIndex < 0 || cb->bitplaneIndex >= AMIGA_CUSTOM_UI_AMIGA_BITPLANE_COUNT) {
        return;
    }
    if (cb->ui->suppressBitplaneCallbacks) {
        return;
    }
    cb->ui->bitplaneEnabled[cb->bitplaneIndex] = selected ? 1 : 0;
    amiga_custom_ui_bitplane_applyOption(cb->bitplaneIndex);
    amiga_custom_ui_bitplane_syncMasterCheckbox(cb->ui);
}
