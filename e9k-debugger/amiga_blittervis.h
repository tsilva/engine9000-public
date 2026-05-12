/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stdint.h>

#include "e9ui.h"
#include "libretro_host.h"

void
amiga_blittervis_destroy(void);

void
amiga_blittervis_renderOverlay(e9ui_context_t *ctx, SDL_Rect *dst);

int
amiga_blittervis_handleOverlayEvent(e9ui_context_t *ctx, const SDL_Rect *dst, const e9ui_event_t *ev);

int
amiga_blittervis_getLatestStats(e9k_debug_ami_blitter_vis_stats_t *outStats);

uint32_t
amiga_blittervis_getHoveredBlitId(void);

uint32_t
amiga_blittervis_getColor(uint32_t blitId);
