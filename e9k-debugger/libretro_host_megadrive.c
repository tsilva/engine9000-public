/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdbool.h>

#include "libretro_host_internal.h"

bool
libretro_host_megadrive_getSpriteState(e9k_debug_mega_sprite_state_t *out)
{
    if (!out || !libretro_host.debugMegadriveGetSpriteState) {
        return false;
    }
    size_t n = libretro_host.debugMegadriveGetSpriteState(out, sizeof(*out));
    return n == sizeof(*out);
}

size_t
libretro_host_megadrive_getRoms(e9k_debug_rom_entry_t *out, size_t cap)
{
    size_t n = 0u;

    if (!libretro_host.debugMegadriveGetRoms) {
        return 0u;
    }
    n = libretro_host.debugMegadriveGetRoms(out, cap * sizeof(*out));
    if (n > cap * sizeof(*out)) {
        n = cap * sizeof(*out);
    }
    return n / sizeof(*out);
}

void
libretro_host_megadrive_bindApis(void)
{
    libretro_host.debugMegadriveGetSpriteState =
        (e9k_debug_megadrive_get_sprite_state_fn_t)libretro_host_loadSymbol("e9k_debug_megadrive_get_sprite_state");
    libretro_host.debugMegadriveGetRoms =
        (e9k_debug_megadrive_get_roms_fn_t)libretro_host_loadSymbol("e9k_debug_megadrive_get_roms");
}

void
libretro_host_megadrive_unbindApis(void)
{
    libretro_host.debugMegadriveGetSpriteState = NULL;
    libretro_host.debugMegadriveGetRoms = NULL;
}
