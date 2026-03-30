/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "amiga_custom_ui_internal.h"

static e9k_debug_option_t
amiga_custom_ui_audio_optionForIndex(int audioChannelIndex)
{
    switch (audioChannelIndex) {
        case 0: return E9K_DEBUG_OPTION_AMIGA_AUDIO0;
        case 1: return E9K_DEBUG_OPTION_AMIGA_AUDIO1;
        case 2: return E9K_DEBUG_OPTION_AMIGA_AUDIO2;
        case 3: return E9K_DEBUG_OPTION_AMIGA_AUDIO3;
        default: break;
    }
    return e9k_debug_option_none;
}


void
amiga_custom_ui_audio_applyOption(int audioChannelIndex)
{
    amiga_custom_ui_state_t *ui = &amiga_custom_ui_state;
    if (audioChannelIndex < 0 || audioChannelIndex >= AMIGA_CUSTOM_UI_AMIGA_AUDIO_CHANNEL_COUNT) {
        return;
    }
    e9k_debug_option_t option = amiga_custom_ui_audio_optionForIndex(audioChannelIndex);
    if (option == e9k_debug_option_none) {
        return;
    }
    amiga_custom_ui_common_applyOption(option, ui->audioEnabled[audioChannelIndex] ? 1u : 0u);
}


static int
amiga_custom_ui_audio_areAllEnabled(const amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return 0;
    }
    for (int audioChannelIndex = 0; audioChannelIndex < AMIGA_CUSTOM_UI_AMIGA_AUDIO_CHANNEL_COUNT; ++audioChannelIndex) {
        if (!ui->audioEnabled[audioChannelIndex]) {
            return 0;
        }
    }
    return 1;
}


void
amiga_custom_ui_audio_syncMasterCheckbox(amiga_custom_ui_state_t *ui)
{
    if (!ui) {
        return;
    }
    ui->audiosEnabled = amiga_custom_ui_audio_areAllEnabled(ui) ? 1 : 0;
    if (!ui->audiosCheckbox) {
        return;
    }
    ui->suppressAudioCallbacks = 1;
    e9ui_checkbox_setSelected(ui->audiosCheckbox, ui->audiosEnabled, &ui->ctx);
    ui->suppressAudioCallbacks = 0;
}


void
amiga_custom_ui_audio_masterChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    amiga_custom_ui_state_t *ui = (amiga_custom_ui_state_t*)user;
    if (!ui) {
        return;
    }
    if (ui->suppressAudioCallbacks) {
        return;
    }
    int nextValue = selected ? 1 : 0;
    ui->audiosEnabled = nextValue;
    for (int audioChannelIndex = 0; audioChannelIndex < AMIGA_CUSTOM_UI_AMIGA_AUDIO_CHANNEL_COUNT; ++audioChannelIndex) {
        ui->audioEnabled[audioChannelIndex] = nextValue;
        amiga_custom_ui_audio_applyOption(audioChannelIndex);
        if (ui->audioCheckboxes[audioChannelIndex]) {
            ui->suppressAudioCallbacks = 1;
            e9ui_checkbox_setSelected(ui->audioCheckboxes[audioChannelIndex], nextValue, &ui->ctx);
            ui->suppressAudioCallbacks = 0;
        }
    }
}


void
amiga_custom_ui_audio_changed(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    if (!user) {
        return;
    }
    struct amiga_custom_ui_audio_cb *cb = (struct amiga_custom_ui_audio_cb *)user;
    if (!cb->ui) {
        return;
    }
    if (cb->audioChannelIndex < 0 || cb->audioChannelIndex >= AMIGA_CUSTOM_UI_AMIGA_AUDIO_CHANNEL_COUNT) {
        return;
    }
    if (cb->ui->suppressAudioCallbacks) {
        return;
    }
    cb->ui->audioEnabled[cb->audioChannelIndex] = selected ? 1 : 0;
    amiga_custom_ui_audio_applyOption(cb->audioChannelIndex);
    amiga_custom_ui_audio_syncMasterCheckbox(cb->ui);
}
