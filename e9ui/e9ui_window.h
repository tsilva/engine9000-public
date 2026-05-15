/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <limits.h>
#include <stdio.h>
#include <SDL.h>
#include <stdint.h>

#include "e9ui_types.h"

#define E9UI_WINDOW_COORD_UNSET INT_MIN

typedef enum e9ui_window_backend
{
    e9ui_window_backend_overlay = 0
} e9ui_window_backend_t;

typedef struct e9ui_window e9ui_window_t;

typedef struct e9ui_window_state
{
    int open;
    e9ui_window_t *windowHost;
    int winHasSaved;
    int winX;
    int winY;
    int winW;
    int winH;
    int openMinWidthPx;
    int openMinHeightPx;
    int openMinWidthNoSavedSizePx;
    int openMinHeightNoSavedSizePx;
    int openCenterWhenNoSaved;
} e9ui_window_state_t;

typedef void (*e9ui_window_close_cb_t)(e9ui_window_t *window, void *user);

e9ui_window_t *
e9ui_windowCreate(e9ui_window_backend_t backend);

void
e9ui_windowDestroy(e9ui_window_t *window);

int
e9ui_windowOpen(e9ui_window_t *window,
                const char *title,
                e9ui_rect_t rect,
                e9ui_component_t *body,
                e9ui_window_close_cb_t onClose,
                void *onCloseUser,
                e9ui_context_t *ctx);

void
e9ui_windowSetMinSize(e9ui_window_t *window, int minWidthPx, int minHeightPx);

void
e9ui_windowSetCloseOnEscape(e9ui_window_t *window, int closeOnEscape);

void
e9ui_windowClose(e9ui_window_t *window);

void
e9ui_windowCloseAllOverlay(void);

int
e9ui_windowCloseTopOverlay(void);

void
e9ui_window_resetOverlayResources(void);

int
e9ui_windowIsOpen(const e9ui_window_t *window);

e9ui_rect_t
e9ui_windowGetRect(const e9ui_window_t *window);

int
e9ui_windowCaptureRectToInts(const e9ui_window_t *window,
                             const e9ui_context_t *ctx,
                             int *outX,
                             int *outY,
                             int *outW,
                             int *outH);

e9ui_rect_t
e9ui_windowRestoreRect(const e9ui_context_t *ctx,
                             e9ui_rect_t defaultRect,
                             int hasPos,
                             int hasSize,
                             int x,
                             int y,
                             int w,
                             int h);

void
e9ui_windowClampRectSize(e9ui_rect_t *rect,
                         const e9ui_context_t *ctx,
                         int minWidthPx,
                         int minHeightPx);

int
e9ui_windowHasSavedPosition(int x, int y);

int
e9ui_windowHasSavedSize(int w, int h);

int
e9ui_windowCaptureRectChanged(e9ui_window_t *window,
                              const e9ui_context_t *ctx,
                              int *hasSaved,
                              int *x,
                              int *y,
                              int *w,
                              int *h);

int
e9ui_windowCaptureRectSnapshot(const e9ui_window_t *window,
                               const e9ui_context_t *ctx,
                               int *hasSaved,
                               int *x,
                               int *y,
                               int *w,
                               int *h);

int
e9ui_windowCaptureStateRectChanged(e9ui_window_state_t *state,
                                   const e9ui_context_t *ctx);

int
e9ui_windowCaptureStateRectSnapshot(e9ui_window_state_t *state,
                                    const e9ui_context_t *ctx);

void
e9ui_windowPersistRect(FILE *file,
                       const char *prefix,
                       const e9ui_window_t *window,
                       const e9ui_context_t *ctx,
                       int *hasSaved,
                       int *x,
                       int *y,
                       int *w,
                       int *h);

void
e9ui_windowPersistStateRect(FILE *file,
                            const char *prefix,
                            e9ui_window_state_t *state,
                            const e9ui_context_t *ctx);

e9ui_rect_t
e9ui_windowResolveOpenRect(const e9ui_context_t *ctx,
                           e9ui_rect_t defaultRect,
                           int minWidthPx,
                           int minHeightPx,
                           int centerWhenNoSaved,
                           int x,
                           int y,
                           int w,
                           int h);

e9ui_rect_t
e9ui_windowResolveStateOpenRect(const e9ui_context_t *ctx,
                                e9ui_rect_t defaultRect,
                                const e9ui_window_state_t *state);

int
e9ui_windowDispatchKeydown(e9ui_component_t *root, e9ui_context_t *ctx, const e9ui_event_t *ev);
