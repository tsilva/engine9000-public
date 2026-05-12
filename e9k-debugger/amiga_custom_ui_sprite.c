/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "amiga_custom_ui_internal.h"
#include "libretro_host.h"

static e9k_debug_option_t
amiga_custom_ui_sprite_optionForIndex(int spriteIndex)
{
    switch (spriteIndex) {
        case 0: return E9K_DEBUG_OPTION_AMIGA_SPRITE0;
        case 1: return E9K_DEBUG_OPTION_AMIGA_SPRITE1;
        case 2: return E9K_DEBUG_OPTION_AMIGA_SPRITE2;
        case 3: return E9K_DEBUG_OPTION_AMIGA_SPRITE3;
        case 4: return E9K_DEBUG_OPTION_AMIGA_SPRITE4;
        case 5: return E9K_DEBUG_OPTION_AMIGA_SPRITE5;
        case 6: return E9K_DEBUG_OPTION_AMIGA_SPRITE6;
        case 7: return E9K_DEBUG_OPTION_AMIGA_SPRITE7;
        default: break;
    }
    return e9k_debug_option_none;
}


void
amiga_custom_ui_sprite_applyVisOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    (void)libretro_host_amiga_setSpriteVis(ui->spriteVisEnabled ? 1 : 0);
}


void
amiga_custom_ui_sprite_applyOption(int spriteIndex)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    if (spriteIndex < 0 || spriteIndex >= AMIGA_CUSTOM_UI_AMIGA_SPRITE_COUNT) {
        return;
    }
    e9k_debug_option_t option = amiga_custom_ui_sprite_optionForIndex(spriteIndex);
    if (option == e9k_debug_option_none) {
        return;
    }
    amiga_custom_ui_common_applyOption(option, ui->spriteEnabled[spriteIndex] ? 1u : 0u);
}


void
amiga_custom_ui_sprite_syncVisCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    int enabled = 0;
    if (!libretro_host_amiga_getSpriteVis(&enabled)) {
        return;
    }
    ui->spriteVisEnabled = enabled ? 1 : 0;
    if (ui->spriteVisCheckbox) {
        ui->suppressSpriteVisCallbacks = 1;
        e9ui_checkbox_setSelected(ui->spriteVisCheckbox, ui->spriteVisEnabled, &ui->ctx);
        ui->suppressSpriteVisCallbacks = 0;
    }
}


static int
amiga_custom_ui_sprite_areAllEnabled(const amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return 0;
    }
    for (int spriteIndex = 0; spriteIndex < AMIGA_CUSTOM_UI_AMIGA_SPRITE_COUNT; ++spriteIndex) {
        if (!ui->spriteEnabled[spriteIndex]) {
            return 0;
        }
    }
    return 1;
}


void
amiga_custom_ui_sprite_syncMasterCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    ui->spritesEnabled = amiga_custom_ui_sprite_areAllEnabled(ui) ? 1 : 0;
    if (!ui->spritesCheckbox) {
        return;
    }
    ui->suppressSpriteCallbacks = 1;
    e9ui_checkbox_setSelected(ui->spritesCheckbox, ui->spritesEnabled, &ui->ctx);
    ui->suppressSpriteCallbacks = 0;
}


void
amiga_custom_ui_sprite_visChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t *)user;
    if (!ui) {
        return;
    }
    if (ui->suppressSpriteVisCallbacks) {
        return;
    }
    ui->spriteVisEnabled = selected ? 1 : 0;
    amiga_custom_ui_sprite_applyVisOption();
}


void
amiga_custom_ui_sprite_masterChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    if (ui->suppressSpriteCallbacks) {
        return;
    }
    int nextValue = selected ? 1 : 0;
    ui->spritesEnabled = nextValue;
    for (int spriteIndex = 0; spriteIndex < AMIGA_CUSTOM_UI_AMIGA_SPRITE_COUNT; ++spriteIndex) {
        ui->spriteEnabled[spriteIndex] = nextValue;
        amiga_custom_ui_sprite_applyOption(spriteIndex);
        if (ui->spriteCheckboxes[spriteIndex]) {
            ui->suppressSpriteCallbacks = 1;
            e9ui_checkbox_setSelected(ui->spriteCheckboxes[spriteIndex], nextValue, &ui->ctx);
            ui->suppressSpriteCallbacks = 0;
        }
    }
}


void
amiga_custom_ui_sprite_changed(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    if (!user) {
        return;
    }
    struct amiga_custom_ui_sprite_cb *cb = (struct amiga_custom_ui_sprite_cb *)user;
    if (!cb->ui) {
        return;
    }
    if (cb->spriteIndex < 0 || cb->spriteIndex >= AMIGA_CUSTOM_UI_AMIGA_SPRITE_COUNT) {
        return;
    }
    if (cb->ui->suppressSpriteCallbacks) {
        return;
    }
    cb->ui->spriteEnabled[cb->spriteIndex] = selected ? 1 : 0;
    amiga_custom_ui_sprite_applyOption(cb->spriteIndex);
    amiga_custom_ui_sprite_syncMasterCheckbox(cb->ui);
}
