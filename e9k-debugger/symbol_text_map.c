/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "symbol_text_map.h"
#include "alloc.h"
#include "debugger.h"

typedef struct symbol_text_map_state {
    symbol_text_map_entry_t *entries;
    int entryCount;
    int entryCap;
    char loadedPath[PATH_MAX];
    int loadAttempted;
    int parsed;
    int valid;
    uint64_t revision;
} symbol_text_map_state_t;

static symbol_text_map_state_t symbol_text_map_state;

static void
symbol_text_map_freeEntries(symbol_text_map_state_t *state)
{
    if (!state) {
        return;
    }
    for (int i = 0; i < state->entryCount; ++i) {
        alloc_free((void *)state->entries[i].name);
    }
    alloc_free(state->entries);
    state->entries = NULL;
    state->entryCount = 0;
    state->entryCap = 0;
}

static void
symbol_text_map_resetState(void)
{
    symbol_text_map_freeEntries(&symbol_text_map_state);
    symbol_text_map_state.loadedPath[0] = '\0';
    symbol_text_map_state.loadAttempted = 0;
    symbol_text_map_state.parsed = 0;
    symbol_text_map_state.valid = 0;
    symbol_text_map_state.revision++;
}

void
symbol_text_map_clear(void)
{
    symbol_text_map_resetState();
}

static char *
symbol_text_map_trim(char *text)
{
    if (!text) {
        return NULL;
    }
    while (*text && isspace((unsigned char)*text)) {
        ++text;
    }
    size_t len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }
    return text;
}

static unsigned
symbol_text_map_kindToMask(symbol_text_map_symbol_kind_t kind)
{
    switch (kind) {
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION:
        return SYMBOL_TEXT_MAP_SYMBOL_MASK_FUNCTION;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE:
        return SYMBOL_TEXT_MAP_SYMBOL_MASK_VARIABLE;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN:
    default:
        return SYMBOL_TEXT_MAP_SYMBOL_MASK_UNKNOWN;
    }
}

static int
symbol_text_map_kindPriority(symbol_text_map_symbol_kind_t kind)
{
    switch (kind) {
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION:
        return 0;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN:
        return 1;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE:
    default:
        return 2;
    }
}

static int
symbol_text_map_parseKind(const char *text, symbol_text_map_symbol_kind_t *outKind)
{
    if (outKind) {
        *outKind = SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
    }
    if (!text || !*text || !outKind) {
        return 0;
    }
    if (strcasecmp(text, "f") == 0 ||
        strcasecmp(text, "func") == 0 ||
        strcasecmp(text, "function") == 0) {
        *outKind = SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION;
        return 1;
    }
    if (strcasecmp(text, "v") == 0 ||
        strcasecmp(text, "var") == 0 ||
        strcasecmp(text, "variable") == 0) {
        *outKind = SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE;
        return 1;
    }
    if (strcasecmp(text, "u") == 0 ||
        strcasecmp(text, "unk") == 0 ||
        strcasecmp(text, "unknown") == 0) {
        *outKind = SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
        return 1;
    }
    return 0;
}

static int
symbol_text_map_parseAddress(const char *text, uint32_t *outAddr)
{
    if (outAddr) {
        *outAddr = 0;
    }
    if (!text || !*text || !outAddr) {
        return 0;
    }
    const char *cursor = text;
    if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        cursor += 2;
    }
    if (!*cursor) {
        return 0;
    }
    for (const char *p = cursor; *p; ++p) {
        if (!isxdigit((unsigned char)*p)) {
            return 0;
        }
    }
    char *end = NULL;
    unsigned long value = strtoul(cursor, &end, 16);
    if (!end || *end != '\0') {
        return 0;
    }
    *outAddr = (uint32_t)value & 0x00ffffffu;
    return 1;
}

static int
symbol_text_map_ensureCapacity(symbol_text_map_state_t *state, int minCap)
{
    if (!state) {
        return 0;
    }
    if (state->entryCap >= minCap) {
        return 1;
    }
    int nextCap = state->entryCap > 0 ? state->entryCap * 2 : 256;
    while (nextCap < minCap) {
        nextCap *= 2;
    }
    symbol_text_map_entry_t *nextEntries =
        (symbol_text_map_entry_t *)alloc_realloc(state->entries,
                                                 sizeof(*nextEntries) * (size_t)nextCap);
    if (!nextEntries) {
        return 0;
    }
    state->entries = nextEntries;
    state->entryCap = nextCap;
    return 1;
}

static int
symbol_text_map_addEntry(symbol_text_map_state_t *state, const char *name, uint32_t addr, symbol_text_map_symbol_kind_t kind)
{
    if (!state || !name || !*name) {
        return 0;
    }
    if (!symbol_text_map_ensureCapacity(state, state->entryCount + 1)) {
        return 0;
    }
    char *nameDup = alloc_strdup(name);
    if (!nameDup) {
        return 0;
    }
    symbol_text_map_entry_t *entry = &state->entries[state->entryCount++];
    entry->name = nameDup;
    entry->addr = addr & 0x00ffffffu;
    entry->kind = kind;
    return 1;
}

static int
symbol_text_map_parseLine(symbol_text_map_state_t *state, char *line)
{
    char *trimmed = symbol_text_map_trim(line);
    if (!trimmed || !*trimmed) {
        return 1;
    }
    if (trimmed[0] == '#' || trimmed[0] == ';') {
        return 1;
    }
    size_t len = strlen(trimmed);
    if (len >= 2 && trimmed[0] == '(' && trimmed[len - 1] == ')') {
        trimmed[len - 1] = '\0';
        trimmed = symbol_text_map_trim(trimmed + 1);
        if (!trimmed || !*trimmed) {
            return 0;
        }
    }

    char *tokens[4];
    int count = 0;
    char *cursor = trimmed;
    while (*cursor && count < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
        while (*cursor && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (!*cursor) {
            break;
        }
        tokens[count++] = cursor;
        while (*cursor && !isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (*cursor) {
            *cursor++ = '\0';
        }
    }
    if (count < 2 || count > 3) {
        return 0;
    }

    uint32_t addr = 0;
    if (!symbol_text_map_parseAddress(tokens[0], &addr)) {
        return 0;
    }

    symbol_text_map_symbol_kind_t kind = SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
    if (count >= 3 && !symbol_text_map_parseKind(tokens[2], &kind)) {
        return 0;
    }
    return symbol_text_map_addEntry(state, tokens[1], addr, kind);
}

static int
symbol_text_map_loadFile(symbol_text_map_state_t *state, const char *path)
{
    if (!state || !path || !*path) {
        return 0;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return 0;
    }

    char *line = NULL;
    size_t cap = 0;
    int ok = 1;
    while (debugger_platform_getline(&line, &cap, file) != -1) {
        if (!symbol_text_map_parseLine(state, line)) {
            ok = 0;
            break;
        }
    }
    free(line);
    fclose(file);
    return ok;
}

static int
symbol_text_map_parseFile(const char *path, symbol_text_map_state_t *outState)
{
    if (!outState) {
        return 0;
    }
    memset(outState, 0, sizeof(*outState));
    if (!symbol_text_map_loadFile(outState, path)) {
        symbol_text_map_freeEntries(outState);
        return 0;
    }
    outState->parsed = 1;
    outState->valid = outState->entryCount > 0 ? 1 : 0;
    return 1;
}

static const char *
symbol_text_map_kindName(symbol_text_map_symbol_kind_t kind)
{
    switch (kind) {
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION:
        return "function";
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE:
        return "variable";
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN:
    default:
        return "unknown";
    }
}

static int
symbol_text_map_loadInternal(const char *path, int allowEmpty)
{
    if (symbol_text_map_state.parsed) {
        if (allowEmpty) {
            return 1;
        }
        return symbol_text_map_state.valid;
    }
    if (!path || !*path) {
        return 0;
    }

    symbol_text_map_resetState();
    strncpy(symbol_text_map_state.loadedPath, path, sizeof(symbol_text_map_state.loadedPath) - 1);
    symbol_text_map_state.loadedPath[sizeof(symbol_text_map_state.loadedPath) - 1] = '\0';
    symbol_text_map_state.loadAttempted = 1;
    int ok = symbol_text_map_loadFile(&symbol_text_map_state, path);

    if (!ok) {
        symbol_text_map_freeEntries(&symbol_text_map_state);
        symbol_text_map_state.parsed = 0;
        symbol_text_map_state.valid = 0;
        return 0;
    }

    symbol_text_map_state.parsed = 1;
    symbol_text_map_state.valid = symbol_text_map_state.entryCount > 0 ? 1 : 0;
    if (allowEmpty) {
        return 1;
    }
    return symbol_text_map_state.valid;
}

static int
symbol_text_map_findEntryIndexByName(const char *name)
{
    if (!name || !*name) {
        return -1;
    }
    for (int i = 0; i < symbol_text_map_state.entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &symbol_text_map_state.entries[i];
        if (entry->name && strcmp(entry->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void
symbol_text_map_removeEntryAt(int index)
{
    if (index < 0 || index >= symbol_text_map_state.entryCount) {
        return;
    }
    alloc_free((void *)symbol_text_map_state.entries[index].name);
    for (int i = index; i + 1 < symbol_text_map_state.entryCount; ++i) {
        symbol_text_map_state.entries[i] = symbol_text_map_state.entries[i + 1];
    }
    symbol_text_map_state.entryCount--;
    symbol_text_map_state.valid = symbol_text_map_state.entryCount > 0 ? 1 : 0;
}

uint64_t
symbol_text_map_revision(void)
{
    return symbol_text_map_state.revision;
}

int
symbol_text_map_canLoad(const char *path)
{
    symbol_text_map_state_t tempState;
    int ok = symbol_text_map_parseFile(path, &tempState);
    symbol_text_map_freeEntries(&tempState);
    return ok;
}

int
symbol_text_map_hasActive(void)
{
    return symbol_text_map_state.parsed ? 1 : 0;
}

int
symbol_text_map_importFile(const char *path)
{
    if (!path || !*path) {
        return 0;
    }

    symbol_text_map_state_t tempState;
    if (!symbol_text_map_parseFile(path, &tempState)) {
        return 0;
    }

    for (int i = 0; i < tempState.entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &tempState.entries[i];
        if (!symbol_text_map_upsert(entry->name, entry->addr, entry->kind)) {
            symbol_text_map_freeEntries(&tempState);
            return 0;
        }
    }
    symbol_text_map_freeEntries(&tempState);
    return 1;
}

int
symbol_text_map_saveToPath(const char *path)
{
    if (!path || !path[0]) {
        return 0;
    }
    if (!symbol_text_map_loadInternal(path, 1)) {
        return 0;
    }

    FILE *file = fopen(path, "w");
    if (!file) {
        return 0;
    }

    for (int i = 0; i < symbol_text_map_state.entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &symbol_text_map_state.entries[i];
        if (!entry->name || !entry->name[0]) {
            continue;
        }
        fprintf(file, "0x%06X %s %s\n",
                (unsigned)(entry->addr & 0x00ffffffu),
                entry->name,
                symbol_text_map_kindName(entry->kind));
    }

    fclose(file);
    return 1;
}

int
symbol_text_map_isValid(const char *path)
{
    return symbol_text_map_loadInternal(path, 0);
}

int
symbol_text_map_getEntries(const char *path, const symbol_text_map_entry_t **outEntries, int *outCount)
{
    if (outEntries) {
        *outEntries = NULL;
    }
    if (outCount) {
        *outCount = 0;
    }
    if (!outEntries || !outCount) {
        return 0;
    }
    if (!symbol_text_map_loadInternal(path, 1)) {
        return 0;
    }
    *outEntries = symbol_text_map_state.entries;
    *outCount = symbol_text_map_state.entryCount;
    return 1;
}

int
symbol_text_map_findExact(const char *path, const char *name, unsigned allowedKinds,
                          const symbol_text_map_entry_t **outEntry)
{
    if (outEntry) {
        *outEntry = NULL;
    }
    if (!name || !*name || !outEntry) {
        return 0;
    }
    if (allowedKinds == 0u) {
        allowedKinds = SYMBOL_TEXT_MAP_SYMBOL_MASK_ALL;
    }
    if (!symbol_text_map_loadInternal(path, 0)) {
        return 0;
    }

    const symbol_text_map_entry_t *best = NULL;
    int bestPriority = 99;
    for (int i = 0; i < symbol_text_map_state.entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &symbol_text_map_state.entries[i];
        if (strcmp(entry->name, name) != 0) {
            continue;
        }
        if ((allowedKinds & symbol_text_map_kindToMask(entry->kind)) == 0u) {
            continue;
        }
        int priority = symbol_text_map_kindPriority(entry->kind);
        if (!best || priority < bestPriority) {
            best = entry;
            bestPriority = priority;
        }
    }
    if (!best) {
        return 0;
    }
    *outEntry = best;
    return 1;
}

int
symbol_text_map_findNearest(const char *path, uint32_t addr, unsigned allowedKinds,
                            const symbol_text_map_entry_t **outEntry)
{
    if (outEntry) {
        *outEntry = NULL;
    }
    if (!outEntry) {
        return 0;
    }
    if (allowedKinds == 0u) {
        allowedKinds = SYMBOL_TEXT_MAP_SYMBOL_MASK_ALL;
    }
    if (!symbol_text_map_loadInternal(path, 0)) {
        return 0;
    }

    const symbol_text_map_entry_t *best = NULL;
    int bestPriority = 99;
    uint32_t bestAddr = 0;
    uint32_t queryAddr = addr & 0x00ffffffu;
    for (int i = 0; i < symbol_text_map_state.entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &symbol_text_map_state.entries[i];
        if ((allowedKinds & symbol_text_map_kindToMask(entry->kind)) == 0u) {
            continue;
        }
        if (entry->addr > queryAddr) {
            continue;
        }
        int priority = symbol_text_map_kindPriority(entry->kind);
        if (!best || entry->addr > bestAddr ||
            (entry->addr == bestAddr && priority < bestPriority)) {
            best = entry;
            bestAddr = entry->addr;
            bestPriority = priority;
        }
    }
    if (!best) {
        return 0;
    }
    *outEntry = best;
    return 1;
}

int
symbol_text_map_upsert(const char *name, uint32_t addr, symbol_text_map_symbol_kind_t kind)
{
    if (!name || !*name) {
        return 0;
    }
    if (!symbol_text_map_state.parsed) {
        return 0;
    }

    int keptOne = 0;
    for (int i = 0; i < symbol_text_map_state.entryCount; ) {
        symbol_text_map_entry_t *entry = &symbol_text_map_state.entries[i];
        if (!entry->name || strcmp(entry->name, name) != 0) {
            ++i;
            continue;
        }
        if (!keptOne) {
            entry->addr = addr & 0x00ffffffu;
            entry->kind = kind;
            keptOne = 1;
            ++i;
            continue;
        }
        symbol_text_map_removeEntryAt(i);
    }

    if (!keptOne && !symbol_text_map_addEntry(&symbol_text_map_state, name, addr, kind)) {
        return 0;
    }
    symbol_text_map_state.parsed = 1;
    symbol_text_map_state.valid = symbol_text_map_state.entryCount > 0 ? 1 : 0;
    symbol_text_map_state.revision++;
    return 1;
}

int
symbol_text_map_delete(const char *name, int *outRemovedCount)
{
    if (outRemovedCount) {
        *outRemovedCount = 0;
    }
    if (!name || !*name) {
        return 0;
    }
    if (!symbol_text_map_state.parsed) {
        return 0;
    }

    int removed = 0;
    while (symbol_text_map_findEntryIndexByName(name) >= 0) {
        int index = symbol_text_map_findEntryIndexByName(name);
        symbol_text_map_removeEntryAt(index);
        removed++;
    }
    if (outRemovedCount) {
        *outRemovedCount = removed;
    }
    if (removed <= 0) {
        return 0;
    }
    symbol_text_map_state.parsed = 1;
    symbol_text_map_state.valid = symbol_text_map_state.entryCount > 0 ? 1 : 0;
    symbol_text_map_state.revision++;
    return 1;
}

int
symbol_text_map_reset(void)
{
    if (!symbol_text_map_state.parsed) {
        return 0;
    }

    while (symbol_text_map_state.entryCount > 0) {
        symbol_text_map_removeEntryAt(symbol_text_map_state.entryCount - 1);
    }
    symbol_text_map_state.parsed = 1;
    symbol_text_map_state.valid = 0;
    symbol_text_map_state.revision++;
    return 1;
}
