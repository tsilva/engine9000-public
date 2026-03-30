/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdio.h>

#include "amiga_custom_ui_internal.h"
#include "e9ui_textbox.h"

static e9k_debug_option_t
amiga_custom_ui_bplptr_optionForIndex(int bplptrIndex)
{
    switch (bplptrIndex) {
        case 0: return E9K_DEBUG_OPTION_AMIGA_BPLPTR1_BLOCK;
        case 1: return E9K_DEBUG_OPTION_AMIGA_BPLPTR2_BLOCK;
        case 2: return E9K_DEBUG_OPTION_AMIGA_BPLPTR3_BLOCK;
        case 3: return E9K_DEBUG_OPTION_AMIGA_BPLPTR4_BLOCK;
        case 4: return E9K_DEBUG_OPTION_AMIGA_BPLPTR5_BLOCK;
        case 5: return E9K_DEBUG_OPTION_AMIGA_BPLPTR6_BLOCK;
        default: break;
    }
    return e9k_debug_option_none;
}


void
amiga_custom_ui_bplptr_applyBlockAllOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_BPLPTR_BLOCK_ALL, ui->bplptrBlockAllEnabled ? 1u : 0u);
}


void
amiga_custom_ui_bplptr_applyLineLimitStartOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    ui->bplptrLineLimitStart = amiga_custom_ui_common_clampCopperLine(ui->bplptrLineLimitStart);
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_BPLPTR_LINE_LIMIT_START, (uint32_t)ui->bplptrLineLimitStart);
}


void
amiga_custom_ui_bplptr_applyLineLimitEndOption(void)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    ui->bplptrLineLimitEnd = amiga_custom_ui_common_clampCopperLine(ui->bplptrLineLimitEnd);
    amiga_custom_ui_common_applyOption(E9K_DEBUG_OPTION_AMIGA_BPLPTR_LINE_LIMIT_END, (uint32_t)ui->bplptrLineLimitEnd);
}


void
amiga_custom_ui_bplptr_applyBlockOption(int bplptrIndex)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    if (bplptrIndex < 0 || bplptrIndex >= AMIGA_CUSTOM_UI_AMIGA_BPLPTR_COUNT) {
        return;
    }
    e9k_debug_option_t option = amiga_custom_ui_bplptr_optionForIndex(bplptrIndex);
    if (option == e9k_debug_option_none) {
        return;
    }
    amiga_custom_ui_common_applyOption(option, ui->bplptrBlockEnabled[bplptrIndex] ? 1u : 0u);
}


static int
amiga_custom_ui_bplptr_areAllBlocked(const amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return 0;
    }
    for (int bplptrIndex = 0; bplptrIndex < AMIGA_CUSTOM_UI_AMIGA_BPLPTR_COUNT; ++bplptrIndex) {
        if (!ui->bplptrBlockEnabled[bplptrIndex]) {
            return 0;
        }
    }
    return 1;
}


void
amiga_custom_ui_bplptr_syncBlockMasterCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    ui->bplptrBlockAllEnabled = amiga_custom_ui_bplptr_areAllBlocked(ui) ? 1 : 0;
    if (!ui->bplptrBlockAllCheckbox) {
        return;
    }
    ui->suppressBplptrBlockCallbacks = 1;
    e9ui_checkbox_setSelected(ui->bplptrBlockAllCheckbox, ui->bplptrBlockAllEnabled, &ui->ctx);
    ui->suppressBplptrBlockCallbacks = 0;
}


void
amiga_custom_ui_bplptr_blockAllChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    if (ui->suppressBplptrBlockCallbacks) {
        return;
    }
    int nextValue = selected ? 1 : 0;
    ui->bplptrBlockAllEnabled = nextValue;
    amiga_custom_ui_bplptr_applyBlockAllOption();
    for (int bplptrIndex = 0; bplptrIndex < AMIGA_CUSTOM_UI_AMIGA_BPLPTR_COUNT; ++bplptrIndex) {
        ui->bplptrBlockEnabled[bplptrIndex] = nextValue;
        amiga_custom_ui_bplptr_applyBlockOption(bplptrIndex);
        if (ui->bplptrBlockCheckboxes[bplptrIndex]) {
            ui->suppressBplptrBlockCallbacks = 1;
            e9ui_checkbox_setSelected(ui->bplptrBlockCheckboxes[bplptrIndex], nextValue, &ui->ctx);
            ui->suppressBplptrBlockCallbacks = 0;
        }
    }
}


void
amiga_custom_ui_bplptr_blockChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    if (!user) {
        return;
    }
    struct amiga_custom_ui_bplptr_cb *cb = (struct amiga_custom_ui_bplptr_cb *)user;
    if (!cb->ui) {
        return;
    }
    if (cb->bplptrIndex < 0 || cb->bplptrIndex >= AMIGA_CUSTOM_UI_AMIGA_BPLPTR_COUNT) {
        return;
    }
    if (cb->ui->suppressBplptrBlockCallbacks) {
        return;
    }
    cb->ui->bplptrBlockEnabled[cb->bplptrIndex] = selected ? 1 : 0;
    amiga_custom_ui_bplptr_applyBlockOption(cb->bplptrIndex);
    amiga_custom_ui_bplptr_syncBlockMasterCheckbox(cb->ui);
    amiga_custom_ui_bplptr_applyBlockAllOption();
}


void
amiga_custom_ui_bplptr_lineLimitStartChanged(e9ui_context_t *ctx, e9ui_component_t *comp, const char *value, void *user)
{
    (void)ctx;
    (void)comp;
    (void)value;
    (void)user;
}


void
amiga_custom_ui_bplptr_lineLimitEndChanged(e9ui_context_t *ctx, e9ui_component_t *comp, const char *value, void *user)
{
    (void)ctx;
    (void)comp;
    (void)value;
    (void)user;
}


static void
amiga_custom_ui_bplptr_commitLineLimitStartTextbox(amiga_custom_ui_state_t *ui)
{
    if (!ui || !ui->bplptrLineLimitStartTextbox) {
        return;
    }

    const char *value = e9ui_textbox_getText(ui->bplptrLineLimitStartTextbox);
    int nextStart = 0;
    if (!value || sscanf(value, "%d", &nextStart) != 1) {
        return;
    }
    nextStart = amiga_custom_ui_common_clampCopperLine(nextStart);
    if (nextStart == ui->bplptrLineLimitStart) {
        return;
    }
    ui->bplptrLineLimitStart = nextStart;
    amiga_custom_ui_bplptr_applyLineLimitStartOption();
}


static void
amiga_custom_ui_bplptr_commitLineLimitEndTextbox(amiga_custom_ui_state_t *ui)
{
    if (!ui || !ui->bplptrLineLimitEndTextbox) {
        return;
    }

    const char *value = e9ui_textbox_getText(ui->bplptrLineLimitEndTextbox);
    int nextEnd = 0;
    if (!value || sscanf(value, "%d", &nextEnd) != 1) {
        return;
    }
    nextEnd = amiga_custom_ui_common_clampCopperLine(nextEnd);
    if (nextEnd == ui->bplptrLineLimitEnd) {
        return;
    }
    ui->bplptrLineLimitEnd = nextEnd;
    amiga_custom_ui_bplptr_applyLineLimitEndOption();
}


int
amiga_custom_ui_bplptr_lineLimitStartTextboxKey(e9ui_context_t *ctx, SDL_Keycode key, SDL_Keymod mods, void *user)
{
    (void)ctx;
    (void)mods;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui || !ui->bplptrLineLimitStartTextbox) {
        return 0;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        amiga_custom_ui_bplptr_commitLineLimitStartTextbox(ui);
    }
    return 0;
}


int
amiga_custom_ui_bplptr_lineLimitEndTextboxKey(e9ui_context_t *ctx, SDL_Keycode key, SDL_Keymod mods, void *user)
{
    (void)ctx;
    (void)mods;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui || !ui->bplptrLineLimitEndTextbox) {
        return 0;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        amiga_custom_ui_bplptr_commitLineLimitEndTextbox(ui);
    }
    return 0;
}


void
amiga_custom_ui_bplptr_tickLineLimitTextboxes(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    int startHasFocus = e9ui_getFocus(&ui->ctx) == ui->bplptrLineLimitStartTextbox ? 1 : 0;
    if (ui->bplptrLineLimitStartTextboxHadFocus && !startHasFocus) {
        amiga_custom_ui_bplptr_commitLineLimitStartTextbox(ui);
    }
    ui->bplptrLineLimitStartTextboxHadFocus = startHasFocus;

    int endHasFocus = e9ui_getFocus(&ui->ctx) == ui->bplptrLineLimitEndTextbox ? 1 : 0;
    if (ui->bplptrLineLimitEndTextboxHadFocus && !endHasFocus) {
        amiga_custom_ui_bplptr_commitLineLimitEndTextbox(ui);
    }
    ui->bplptrLineLimitEndTextboxHadFocus = endHasFocus;
}
