/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <SDL.h>

typedef struct aux_window_ops
{
    void (*setFocus)(int focused);
    int (*handleKeydown)(const SDL_KeyboardEvent *kev);
    void (*render)(void);
} aux_window_ops_t;

int
aux_window_register(const aux_window_ops_t *ops, void *registrationKey);

void
aux_window_unregister(const aux_window_ops_t *ops, void *registrationKey);

void
aux_window_setFocus(int focused);

int
aux_window_handleKeydown(const SDL_KeyboardEvent *kev);

void
aux_window_render(void);
