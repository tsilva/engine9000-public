/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

typedef struct aux_window_ops
{
    void (*setFocus)(int focused);
    void (*render)(void);
} aux_window_ops_t;

int
aux_window_register(const aux_window_ops_t *ops, void *registrationKey);

void
aux_window_unregister(const aux_window_ops_t *ops, void *registrationKey);

void
aux_window_setFocus(int focused);

void
aux_window_render(void);
