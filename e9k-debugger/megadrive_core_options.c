/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "libretro_host.h"
#include "megadrive_core_options.h"

typedef struct megadrive_core_options_kv {
    char *key;
    char *value;
} megadrive_core_options_kv_t;

static megadrive_core_options_kv_t *megadrive_coreOptions_entries = NULL;
static size_t megadrive_coreOptions_entryCount = 0;
static size_t megadrive_coreOptions_entryCap = 0;
static int megadrive_coreOptions_dirty = 0;

static const char *
megadrive_coreOptions_basename(const char *path)
{
    if (!path || !*path) {
        return NULL;
    }
    const char *slash = strrchr(path, '/');
    const char *back = strrchr(path, '\\');
    const char *sep = slash > back ? slash : back;
    return sep ? sep + 1 : path;
}

static void
megadrive_coreOptions_trimRight(char *text)
{
    if (!text) {
        return;
    }
    size_t len = strlen(text);
    while (len > 0) {
        char c = text[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            text[len - 1] = '\0';
            len--;
            continue;
        }
        break;
    }
}

static char *
megadrive_coreOptions_trimLeft(char *text)
{
    if (!text) {
        return NULL;
    }
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

static megadrive_core_options_kv_t *
megadrive_coreOptions_findEntry(const char *key)
{
    if (!key || !*key) {
        return NULL;
    }
    for (size_t i = 0; i < megadrive_coreOptions_entryCount; ++i) {
        if (megadrive_coreOptions_entries[i].key && strcmp(megadrive_coreOptions_entries[i].key, key) == 0) {
            return &megadrive_coreOptions_entries[i];
        }
    }
    return NULL;
}

static megadrive_core_options_kv_t *
megadrive_coreOptions_getOrAddEntry(const char *key)
{
    if (!key || !*key) {
        return NULL;
    }
    megadrive_core_options_kv_t *existing = megadrive_coreOptions_findEntry(key);
    if (existing) {
        return existing;
    }
    if (megadrive_coreOptions_entryCount >= megadrive_coreOptions_entryCap) {
        size_t nextCap = megadrive_coreOptions_entryCap ? megadrive_coreOptions_entryCap * 2 : 64;
        megadrive_core_options_kv_t *next =
            (megadrive_core_options_kv_t *)alloc_realloc(megadrive_coreOptions_entries, nextCap * sizeof(*next));
        if (!next) {
            return NULL;
        }
        megadrive_coreOptions_entries = next;
        megadrive_coreOptions_entryCap = nextCap;
    }
    megadrive_core_options_kv_t *entry = &megadrive_coreOptions_entries[megadrive_coreOptions_entryCount++];
    memset(entry, 0, sizeof(*entry));
    entry->key = alloc_strdup(key);
    entry->value = alloc_strdup("");
    return entry;
}

static void
megadrive_coreOptions_removeEntry(const char *key)
{
    if (!key || !*key || !megadrive_coreOptions_entries) {
        return;
    }
    for (size_t i = 0; i < megadrive_coreOptions_entryCount; ++i) {
        if (megadrive_coreOptions_entries[i].key && strcmp(megadrive_coreOptions_entries[i].key, key) == 0) {
            alloc_free(megadrive_coreOptions_entries[i].key);
            alloc_free(megadrive_coreOptions_entries[i].value);
            for (size_t j = i + 1; j < megadrive_coreOptions_entryCount; ++j) {
                megadrive_coreOptions_entries[j - 1] = megadrive_coreOptions_entries[j];
            }
            megadrive_coreOptions_entryCount--;
            return;
        }
    }
}

static int
megadrive_coreOptions_parseLine(const char *line, char *outKey, size_t keyCap, char *outValue, size_t valueCap)
{
    if (!line || !outKey || keyCap == 0 || !outValue || valueCap == 0) {
        return 0;
    }
    outKey[0] = '\0';
    outValue[0] = '\0';

    const char *cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    if (*cursor == '\0' || *cursor == ';' || *cursor == '#') {
        return 0;
    }

    const char *eq = strchr(cursor, '=');
    if (!eq) {
        return 0;
    }

    size_t keyLen = (size_t)(eq - cursor);
    if (keyLen >= keyCap) {
        keyLen = keyCap - 1;
    }
    memcpy(outKey, cursor, keyLen);
    outKey[keyLen] = '\0';
    megadrive_coreOptions_trimRight(outKey);
    char *trimmedKey = megadrive_coreOptions_trimLeft(outKey);
    if (trimmedKey != outKey) {
        memmove(outKey, trimmedKey, strlen(trimmedKey) + 1);
    }
    if (!outKey[0]) {
        return 0;
    }

    const char *value = eq + 1;
    while (*value == ' ' || *value == '\t') {
        value++;
    }
    strncpy(outValue, value, valueCap - 1);
    outValue[valueCap - 1] = '\0';
    megadrive_coreOptions_trimRight(outValue);
    return 1;
}

int
megadrive_coreOptionsDirty(void)
{
    return megadrive_coreOptions_dirty ? 1 : 0;
}

void
megadrive_coreOptionsClear(void)
{
    if (megadrive_coreOptions_entries) {
        for (size_t i = 0; i < megadrive_coreOptions_entryCount; ++i) {
            alloc_free(megadrive_coreOptions_entries[i].key);
            alloc_free(megadrive_coreOptions_entries[i].value);
        }
        alloc_free(megadrive_coreOptions_entries);
        megadrive_coreOptions_entries = NULL;
    }
    megadrive_coreOptions_entryCount = 0;
    megadrive_coreOptions_entryCap = 0;
    megadrive_coreOptions_dirty = 0;
}

const char *
megadrive_coreOptionsGetValue(const char *key)
{
    megadrive_core_options_kv_t *entry = megadrive_coreOptions_findEntry(key);
    return entry ? entry->value : NULL;
}

void
megadrive_coreOptionsSetValue(const char *key, const char *value)
{
    if (!key || !*key) {
        return;
    }
    if (!value) {
        megadrive_coreOptions_removeEntry(key);
        megadrive_coreOptions_dirty = 1;
        return;
    }
    megadrive_core_options_kv_t *entry = megadrive_coreOptions_getOrAddEntry(key);
    if (!entry) {
        return;
    }
    alloc_free(entry->value);
    entry->value = alloc_strdup(value);
    megadrive_coreOptions_dirty = 1;
}

int
megadrive_coreOptionsBuildPath(char *out, size_t cap, const char *saveDir, const char *romPath)
{
    if (!out || cap == 0) {
        return 0;
    }
    out[0] = '\0';
    if (!saveDir || !*saveDir || !romPath || !*romPath) {
        return 0;
    }
    const char *base = megadrive_coreOptions_basename(romPath);
    if (!base || !*base) {
        return 0;
    }
    char sep = '/';
    if (strchr(saveDir, '\\')) {
        sep = '\\';
    }
    int needsSep = 1;
    size_t len = strlen(saveDir);
    if (len > 0 && (saveDir[len - 1] == '/' || saveDir[len - 1] == '\\')) {
        needsSep = 0;
    }
    int written = 0;
    if (needsSep) {
        written = snprintf(out, cap, "%s%c%s.core_options", saveDir, sep, base);
    } else {
        written = snprintf(out, cap, "%s%s.core_options", saveDir, base);
    }
    if (written < 0 || (size_t)written >= cap) {
        out[cap - 1] = '\0';
        return 0;
    }
    return 1;
}

int
megadrive_coreOptionsLoadFromFile(const char *saveDir, const char *romPath)
{
    char path[PATH_MAX];
    if (!megadrive_coreOptionsBuildPath(path, sizeof(path), saveDir, romPath)) {
        megadrive_coreOptionsClear();
        return 0;
    }
    megadrive_coreOptionsClear();
    FILE *file = fopen(path, "r");
    if (!file) {
        megadrive_coreOptions_dirty = 0;
        return 1;
    }
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        char key[1024];
        char value[3072];
        if (!megadrive_coreOptions_parseLine(line, key, sizeof(key), value, sizeof(value))) {
            continue;
        }
        megadrive_core_options_kv_t *entry = megadrive_coreOptions_getOrAddEntry(key);
        if (!entry) {
            continue;
        }
        alloc_free(entry->value);
        entry->value = alloc_strdup(value);
    }
    fclose(file);
    megadrive_coreOptions_dirty = 0;
    return 1;
}

int
megadrive_coreOptionsWriteToFile(const char *saveDir, const char *romPath)
{
    char path[PATH_MAX];
    if (!megadrive_coreOptionsBuildPath(path, sizeof(path), saveDir, romPath)) {
        return 0;
    }
    if (megadrive_coreOptions_entryCount == 0) {
        remove(path);
        megadrive_coreOptions_dirty = 0;
        return 1;
    }
    char tmpPath[PATH_MAX];
    int tmpWritten = snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
    if (tmpWritten < 0 || tmpWritten >= (int)sizeof(tmpPath)) {
        return 0;
    }
    FILE *out = fopen(tmpPath, "w");
    if (!out) {
        return 0;
    }
    for (size_t i = 0; i < megadrive_coreOptions_entryCount; ++i) {
        const char *key = megadrive_coreOptions_entries[i].key;
        const char *value = megadrive_coreOptions_entries[i].value;
        if (!key || !*key) {
            continue;
        }
        fprintf(out, "%s=%s\n", key, value ? value : "");
    }
    fclose(out);
    remove(path);
    if (rename(tmpPath, path) != 0) {
        remove(tmpPath);
        return 0;
    }
    megadrive_coreOptions_dirty = 0;
    return 1;
}

int
megadrive_coreOptionsApplyFileToHost(const char *saveDir, const char *romPath)
{
    char path[PATH_MAX];
    if (!megadrive_coreOptionsBuildPath(path, sizeof(path), saveDir, romPath)) {
        return 0;
    }
    FILE *file = fopen(path, "r");
    if (!file) {
        return 1;
    }
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        char key[1024];
        char value[3072];
        if (!megadrive_coreOptions_parseLine(line, key, sizeof(key), value, sizeof(value))) {
            continue;
        }
        libretro_host_setCoreOption(key, value);
    }
    fclose(file);
    return 1;
}
