/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include "e9ui.h"
#include "e9k-geo.h"
#include "emu.h"

void
emu_geo_setSpriteState(const e9k_debug_sprite_state_t *state, int ready);

void
emu_geo_setAudioFrame(const e9k_debug_audio_frame_t *frame, int ready);

void
emu_geo_shutdown(void);

extern const emu_system_iface_t emu_geo_iface;
