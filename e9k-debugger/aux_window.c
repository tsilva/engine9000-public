/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdio.h>
#include <string.h>

#include "aux_window.h"

#define AUX_WINDOW_MAX 32

typedef struct aux_window_entry
{
    const aux_window_ops_t *ops;
    void *registrationKey;
} aux_window_entry_t;

static aux_window_entry_t aux_window_entries[AUX_WINDOW_MAX];
static size_t aux_window_entryCount = 0;

static size_t
aux_window_snapshot(aux_window_entry_t *snapshot, size_t snapshotCap)
{
    size_t count = aux_window_entryCount;
    if (count > snapshotCap) {
        count = snapshotCap;
    }
    if (count > 0) {
        memcpy(snapshot, aux_window_entries, count * sizeof(snapshot[0]));
    }
    return count;
}

int
aux_window_register(const aux_window_ops_t *ops, void *registrationKey)
{
    if (!ops) {
        return 0;
    }
    for (size_t i = 0; i < aux_window_entryCount; ++i) {
        if (aux_window_entries[i].ops == ops &&
            aux_window_entries[i].registrationKey == registrationKey) {
            return 1;
        }
    }
    if (aux_window_entryCount >= AUX_WINDOW_MAX) {
        printf("aux_window: registration overflow\n");
        return 0;
    }
    aux_window_entries[aux_window_entryCount].ops = ops;
    aux_window_entries[aux_window_entryCount].registrationKey = registrationKey;
    aux_window_entryCount++;
    return 1;
}

void
aux_window_unregister(const aux_window_ops_t *ops, void *registrationKey)
{
    for (size_t i = 0; i < aux_window_entryCount; ++i) {
        if (aux_window_entries[i].ops == ops &&
            aux_window_entries[i].registrationKey == registrationKey) {
            aux_window_entryCount--;
            if (i < aux_window_entryCount) {
                memmove(&aux_window_entries[i],
                        &aux_window_entries[i + 1],
                        (aux_window_entryCount - i) * sizeof(aux_window_entries[0]));
            }
            return;
        }
    }
}

void
aux_window_setFocus(int focused)
{
    aux_window_entry_t snapshot[AUX_WINDOW_MAX];
    size_t count = aux_window_snapshot(snapshot, AUX_WINDOW_MAX);
    for (size_t i = 0; i < count; ++i) {
        const aux_window_entry_t *entry = &snapshot[i];
        if (entry->ops->setFocus) {
            entry->ops->setFocus(focused);
        }
    }
}

int
aux_window_handleKeydown(const SDL_KeyboardEvent *kev)
{
    if (!kev) {
        return 0;
    }
    aux_window_entry_t snapshot[AUX_WINDOW_MAX];
    size_t count = aux_window_snapshot(snapshot, AUX_WINDOW_MAX);
    for (size_t i = count; i > 0; --i) {
        const aux_window_entry_t *entry = &snapshot[i - 1];
        if (entry->ops->handleKeydown &&
            entry->ops->handleKeydown(kev)) {
            return 1;
        }
    }
    return 0;
}

void
aux_window_render(void)
{
    aux_window_entry_t snapshot[AUX_WINDOW_MAX];
    size_t count = aux_window_snapshot(snapshot, AUX_WINDOW_MAX);
    for (size_t i = 0; i < count; ++i) {
        const aux_window_entry_t *entry = &snapshot[i];
        if (entry->ops->render) {
            entry->ops->render();
        }
    }
}
