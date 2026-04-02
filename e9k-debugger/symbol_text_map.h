/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum symbol_text_map_symbol_kind {
    SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN = 0,
    SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION = 1,
    SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE = 2
} symbol_text_map_symbol_kind_t;

typedef struct symbol_text_map_entry {
    const char *name;
    uint32_t addr;
    symbol_text_map_symbol_kind_t kind;
} symbol_text_map_entry_t;

#define SYMBOL_TEXT_MAP_SYMBOL_MASK_UNKNOWN  (1u << 0)
#define SYMBOL_TEXT_MAP_SYMBOL_MASK_FUNCTION (1u << 1)
#define SYMBOL_TEXT_MAP_SYMBOL_MASK_VARIABLE (1u << 2)
#define SYMBOL_TEXT_MAP_SYMBOL_MASK_ALL      (SYMBOL_TEXT_MAP_SYMBOL_MASK_UNKNOWN | \
                                              SYMBOL_TEXT_MAP_SYMBOL_MASK_FUNCTION | \
                                              SYMBOL_TEXT_MAP_SYMBOL_MASK_VARIABLE)

int
symbol_text_map_canLoad(const char *path);

int
symbol_text_map_hasActive(void);

int
symbol_text_map_importFile(const char *path);

int
symbol_text_map_saveToPath(const char *path);

int
symbol_text_map_isValid(const char *path);

int
symbol_text_map_getEntries(const char *path, const symbol_text_map_entry_t **outEntries, int *outCount);

int
symbol_text_map_findExact(const char *path, const char *name, unsigned allowedKinds,
                          const symbol_text_map_entry_t **outEntry);

int
symbol_text_map_findNearest(const char *path, uint32_t addr, unsigned allowedKinds,
                            const symbol_text_map_entry_t **outEntry);

int
symbol_text_map_upsert(const char *name, uint32_t addr, symbol_text_map_symbol_kind_t kind);

int
symbol_text_map_delete(const char *name, int *outRemovedCount);

int
symbol_text_map_reset(void);

uint64_t
symbol_text_map_revision(void);

void
symbol_text_map_clear(void);
