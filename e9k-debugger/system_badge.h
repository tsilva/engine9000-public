/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <SDL.h>

#include "target.h"

SDL_Texture *
system_badge_getTexture(SDL_Renderer *renderer, target_iface_t *system, int *outW, int *outH);

SDL_Texture *
system_badge_loadTexture(SDL_Renderer *renderer, const char *asset, int *outW, int *outH);
