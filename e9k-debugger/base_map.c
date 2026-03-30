/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "base_map.h"

#define BASE_MAP_ADDR_MASK 0x00ffffffu
#define BASE_MAP_ADDR_SPACE 0x01000000u
#define BASE_MAP_STACK_CAP 256u

typedef struct base_map_entry {
    base_map_section_t section;
    uint32_t base;
    uint32_t size;
} base_map_entry_t;

static base_map_mode_t base_map_mode = BASE_MAP_MODE_BASIC;
static uint32_t base_map_basicBase[BASE_MAP_SECTION_COUNT];
static base_map_entry_t base_map_stackEntries[BASE_MAP_STACK_CAP];
static size_t base_map_stackCount = 0;

static int
base_map_sectionValid(base_map_section_t section)
{
    return section >= BASE_MAP_SECTION_TEXT && section < BASE_MAP_SECTION_COUNT;
}

static uint32_t
base_map_mask24(uint32_t addr)
{
    return addr & BASE_MAP_ADDR_MASK;
}

static int
base_map_sizeValid(uint32_t size)
{
    return size != 0u && size != BASE_MAP_INVALID_SIZE;
}

static int
base_map_rangeContains(uint32_t base, uint32_t size, uint32_t addr, uint32_t *outOffset)
{
    if (!base_map_sizeValid(size)) {
        return 0;
    }
    uint32_t base24 = base_map_mask24(base);
    uint32_t addr24 = base_map_mask24(addr);
    uint32_t offset = 0;
    uint64_t end = (uint64_t)base24 + (uint64_t)size;
    if (end <= (uint64_t)BASE_MAP_ADDR_SPACE) {
        uint32_t end24 = (uint32_t)end;
        if (addr24 < base24 || addr24 >= end24) {
            return 0;
        }
        offset = addr24 - base24;
    } else {
        uint32_t wrapEnd = (uint32_t)(end & BASE_MAP_ADDR_MASK);
        if (!(addr24 >= base24 || addr24 < wrapEnd)) {
            return 0;
        }
        if (addr24 >= base24) {
            offset = addr24 - base24;
        } else {
            offset = (uint32_t)((BASE_MAP_ADDR_SPACE - base24) + addr24);
        }
        if ((uint64_t)offset >= (uint64_t)size) {
            return 0;
        }
    }
    if (outOffset) {
        *outOffset = offset;
    }
    return 1;
}

static int
base_map_runtimeToDebugStackInternal(base_map_section_t section, uint32_t runtimeAddr,
                                     uint32_t *outDebugAddr, size_t *outIndex)
{
    uint32_t runtime24 = base_map_mask24(runtimeAddr);
    if (outDebugAddr) {
        *outDebugAddr = runtime24;
    }
    if (outIndex) {
        *outIndex = 0;
    }
    for (size_t i = base_map_stackCount; i > 0; --i) {
        const base_map_entry_t *entry = &base_map_stackEntries[i - 1u];
        if (entry->section != section || !base_map_sizeValid(entry->size)) {
            continue;
        }
        uint32_t offset = 0;
        if (!base_map_rangeContains(entry->base, entry->size, runtime24, &offset)) {
            continue;
        }
        if (outDebugAddr) {
            *outDebugAddr = base_map_mask24(offset);
        }
        if (outIndex) {
            *outIndex = i - 1u;
        }
        return 1;
    }
    for (size_t i = base_map_stackCount; i > 0; --i) {
        const base_map_entry_t *entry = &base_map_stackEntries[i - 1u];
        if (entry->section != section || base_map_sizeValid(entry->size)) {
            continue;
        }
        uint32_t base24 = base_map_mask24(entry->base);
        if (runtime24 < base24) {
            continue;
        }
        if (outDebugAddr) {
            *outDebugAddr = base_map_mask24(runtime24 - base24);
        }
        if (outIndex) {
            *outIndex = i - 1u;
        }
        return 1;
    }
    return 0;
}

static int
base_map_runtimeToDebugStack(base_map_section_t section, uint32_t runtimeAddr, uint32_t *outDebugAddr)
{
    return base_map_runtimeToDebugStackInternal(section, runtimeAddr, outDebugAddr, NULL);
}

static int
base_map_debugToRuntimeStack(base_map_section_t section, uint32_t debugAddr, uint32_t *outRuntimeAddr)
{
    uint32_t debug24 = base_map_mask24(debugAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = debug24;
    }
    for (size_t i = base_map_stackCount; i > 0; --i) {
        const base_map_entry_t *entry = &base_map_stackEntries[i - 1u];
        if (entry->section != section || !base_map_sizeValid(entry->size)) {
            continue;
        }
        if ((uint64_t)debug24 >= (uint64_t)entry->size) {
            continue;
        }
        if (outRuntimeAddr) {
            *outRuntimeAddr = base_map_mask24(entry->base + debug24);
        }
        return 1;
    }
    for (size_t i = base_map_stackCount; i > 0; --i) {
        const base_map_entry_t *entry = &base_map_stackEntries[i - 1u];
        if (entry->section != section || base_map_sizeValid(entry->size)) {
            continue;
        }
        if (outRuntimeAddr) {
            *outRuntimeAddr = base_map_mask24(entry->base + debug24);
        }
        return 1;
    }
    return 0;
}

static int
base_map_debugToRuntimeStackByIndex(size_t index, uint32_t debugAddr, uint32_t *outRuntimeAddr)
{
    uint32_t debug24 = base_map_mask24(debugAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = debug24;
    }
    if (index >= base_map_stackCount) {
        return 0;
    }
    const base_map_entry_t *entry = &base_map_stackEntries[index];
    if (base_map_sizeValid(entry->size) && (uint64_t)debug24 >= (uint64_t)entry->size) {
        return 0;
    }
    if (outRuntimeAddr) {
        *outRuntimeAddr = base_map_mask24(entry->base + debug24);
    }
    return 1;
}

static int
base_map_parseIndexedSectionName(const char *sectionName, base_map_section_t *outSection, size_t *outIndex)
{
    if (outSection) {
        *outSection = BASE_MAP_SECTION_TEXT;
    }
    if (outIndex) {
        *outIndex = 0u;
    }
    if (!sectionName || !*sectionName || !outSection || !outIndex) {
        return 0;
    }

    const char *suffix = NULL;
    if (strncmp(sectionName, ".text.", 6) == 0) {
        *outSection = BASE_MAP_SECTION_TEXT;
        suffix = sectionName + 6;
    } else if (strncmp(sectionName, ".data.", 6) == 0) {
        *outSection = BASE_MAP_SECTION_DATA;
        suffix = sectionName + 6;
    } else if (strncmp(sectionName, ".bss.", 5) == 0) {
        *outSection = BASE_MAP_SECTION_BSS;
        suffix = sectionName + 5;
    } else if (strncmp(sectionName, ".rodata.", 8) == 0) {
        *outSection = BASE_MAP_SECTION_DATA;
        suffix = sectionName + 8;
    } else {
        return 0;
    }

    if (!suffix || !*suffix) {
        return 0;
    }
    for (const char *p = suffix; *p; ++p) {
        if (!isdigit((unsigned char)*p)) {
            return 0;
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(suffix, &end, 10);
    if (errno != 0 || !end || end == suffix || *end != '\0') {
        return 0;
    }
    *outIndex = (size_t)parsed;
    return 1;
}

void
base_map_reset(void)
{
    base_map_mode = BASE_MAP_MODE_BASIC;
    memset(base_map_basicBase, 0, sizeof(base_map_basicBase));
    memset(base_map_stackEntries, 0, sizeof(base_map_stackEntries));
    base_map_stackCount = 0;
}

base_map_mode_t
base_map_getMode(void)
{
    return base_map_mode;
}

int
base_map_sectionFromIndex(uint32_t index, base_map_section_t *outSection)
{
    if (!outSection || index >= (uint32_t)BASE_MAP_SECTION_COUNT) {
        return 0;
    }
    *outSection = (base_map_section_t)index;
    return 1;
}

void
base_map_setBasicBase(base_map_section_t section, uint32_t base)
{
    if (!base_map_sectionValid(section)) {
        return;
    }
    base_map_basicBase[section] = base_map_mask24(base);
}

uint32_t
base_map_getBasicBase(base_map_section_t section)
{
    if (!base_map_sectionValid(section)) {
        return 0;
    }
    return base_map_basicBase[section];
}

void
base_map_setBasicBases(uint32_t textBase, uint32_t dataBase, uint32_t bssBase)
{
    base_map_setBasicBase(BASE_MAP_SECTION_TEXT, textBase);
    base_map_setBasicBase(BASE_MAP_SECTION_DATA, dataBase);
    base_map_setBasicBase(BASE_MAP_SECTION_BSS, bssBase);
}

void
base_map_getBasicBases(uint32_t *outTextBase, uint32_t *outDataBase, uint32_t *outBssBase)
{
    if (outTextBase) {
        *outTextBase = base_map_basicBase[BASE_MAP_SECTION_TEXT];
    }
    if (outDataBase) {
        *outDataBase = base_map_basicBase[BASE_MAP_SECTION_DATA];
    }
    if (outBssBase) {
        *outBssBase = base_map_basicBase[BASE_MAP_SECTION_BSS];
    }
}

int
base_map_push(base_map_section_t section, uint32_t base, uint32_t size)
{
    if (!base_map_sectionValid(section) || base_map_stackCount >= BASE_MAP_STACK_CAP) {
        return 0;
    }
    base_map_entry_t *entry = &base_map_stackEntries[base_map_stackCount++];
    entry->section = section;
    entry->base = base_map_mask24(base);
    entry->size = size;
    base_map_mode = BASE_MAP_MODE_STACK;
    return 1;
}

size_t
base_map_getStackCount(void)
{
    return base_map_stackCount;
}

int
base_map_getStackEntry(size_t index, base_map_section_t *outSection, uint32_t *outBase, uint32_t *outSize)
{
    if (index >= base_map_stackCount) {
        return 0;
    }
    const base_map_entry_t *entry = &base_map_stackEntries[index];
    if (outSection) {
        *outSection = entry->section;
    }
    if (outBase) {
        *outBase = entry->base;
    }
    if (outSize) {
        *outSize = entry->size;
    }
    return 1;
}

int
base_map_runtimeToDebug(base_map_section_t section, uint32_t runtimeAddr, uint32_t *outDebugAddr)
{
    uint32_t runtime24 = base_map_mask24(runtimeAddr);
    if (outDebugAddr) {
        *outDebugAddr = runtime24;
    }
    if (!base_map_sectionValid(section)) {
        return 0;
    }
    if (base_map_mode == BASE_MAP_MODE_STACK) {
        return base_map_runtimeToDebugStack(section, runtime24, outDebugAddr);
    }
    uint32_t base24 = base_map_basicBase[section];
    if (base24 != 0 && runtime24 >= base24) {
        if (outDebugAddr) {
            *outDebugAddr = base_map_mask24(runtime24 - base24);
        }
        return 1;
    }
    return 0;
}

int
base_map_runtimeToDebugWithIndex(base_map_section_t section, uint32_t runtimeAddr,
                                 uint32_t *outDebugAddr, size_t *outIndex)
{
    uint32_t runtime24 = base_map_mask24(runtimeAddr);
    if (outDebugAddr) {
        *outDebugAddr = runtime24;
    }
    if (outIndex) {
        *outIndex = (size_t)section;
    }
    if (!base_map_sectionValid(section)) {
        return 0;
    }
    if (base_map_mode == BASE_MAP_MODE_STACK) {
        return base_map_runtimeToDebugStackInternal(section, runtime24, outDebugAddr, outIndex);
    }
    uint32_t base24 = base_map_basicBase[section];
    if (base24 != 0 && runtime24 >= base24) {
        if (outDebugAddr) {
            *outDebugAddr = base_map_mask24(runtime24 - base24);
        }
        return 1;
    }
    return 0;
}

int
base_map_debugToRuntime(base_map_section_t section, uint32_t debugAddr, uint32_t *outRuntimeAddr)
{
    uint32_t debug24 = base_map_mask24(debugAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = debug24;
    }
    if (!base_map_sectionValid(section)) {
        return 0;
    }
    if (base_map_mode == BASE_MAP_MODE_STACK) {
        return base_map_debugToRuntimeStack(section, debug24, outRuntimeAddr);
    }
    uint32_t base24 = base_map_basicBase[section];
    if (base24 != 0) {
        if (outRuntimeAddr) {
            *outRuntimeAddr = base_map_mask24(base24 + debug24);
        }
        return 1;
    }
    return 0;
}

int
base_map_debugToRuntimeWithIndex(size_t index, uint32_t debugAddr, uint32_t *outRuntimeAddr)
{
    uint32_t debug24 = base_map_mask24(debugAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = debug24;
    }
    if (base_map_mode != BASE_MAP_MODE_STACK) {
        return 0;
    }
    return base_map_debugToRuntimeStackByIndex(index, debug24, outRuntimeAddr);
}

int
base_map_symbolToRuntime(const char *sectionName, uint32_t symAddr, uint32_t *outRuntimeAddr)
{
    uint32_t sym24 = base_map_mask24(symAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = sym24;
    }
    if (!sectionName || !*sectionName) {
        return 0;
    }
    if (strcmp(sectionName, ".text") == 0 || strncmp(sectionName, ".text.", 6) == 0) {
        return base_map_debugToRuntime(BASE_MAP_SECTION_TEXT, sym24, outRuntimeAddr);
    }
    if (strcmp(sectionName, ".data") == 0 || strncmp(sectionName, ".data.", 6) == 0) {
        return base_map_debugToRuntime(BASE_MAP_SECTION_DATA, sym24, outRuntimeAddr);
    }
    if (strcmp(sectionName, ".bss") == 0 || strncmp(sectionName, ".bss.", 5) == 0) {
        return base_map_debugToRuntime(BASE_MAP_SECTION_BSS, sym24, outRuntimeAddr);
    }
    if (strcmp(sectionName, ".rodata") == 0 || strncmp(sectionName, ".rodata.", 8) == 0) {
        if (base_map_debugToRuntime(BASE_MAP_SECTION_DATA, sym24, outRuntimeAddr)) {
            return 1;
        }
        return base_map_debugToRuntime(BASE_MAP_SECTION_TEXT, sym24, outRuntimeAddr);
    }
    return 0;
}

int
base_map_symbolToRuntimeHunk(const char *sectionName, uint32_t symAddr, uint32_t *outRuntimeAddr)
{
    uint32_t sym24 = base_map_mask24(symAddr);
    if (outRuntimeAddr) {
        *outRuntimeAddr = sym24;
    }
    base_map_section_t section = BASE_MAP_SECTION_TEXT;
    size_t index = 0u;
    if (!base_map_parseIndexedSectionName(sectionName, &section, &index)) {
        return base_map_symbolToRuntime(sectionName, sym24, outRuntimeAddr);
    }

    if (base_map_mode == BASE_MAP_MODE_STACK && base_map_debugToRuntimeStackByIndex(index, sym24, outRuntimeAddr)) {
        base_map_section_t indexedSection = BASE_MAP_SECTION_TEXT;
        if (base_map_getStackEntry(index, &indexedSection, NULL, NULL) &&
            indexedSection == section) {
            return 1;
        }
        if (section == BASE_MAP_SECTION_DATA && indexedSection == BASE_MAP_SECTION_TEXT &&
            strncmp(sectionName, ".rodata.", 8) == 0) {
            return 1;
        }
    }

    return base_map_symbolToRuntime(sectionName, sym24, outRuntimeAddr);
}
