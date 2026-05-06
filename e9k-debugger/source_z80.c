/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "alloc.h"
#include "debugger.h"
#include "libretro_host.h"
#include "source_z80.h"
#include "strutil.h"
#include "target.h"

enum
{
    SOURCE_Z80_MAX_INSN_BYTES = 4,
    SOURCE_Z80_MAX_BANK_NOI_FILES = 16
};

typedef struct source_z80_symbol {
    uint16_t addr;
    int order;
    char *name;
} source_z80_symbol_t;

typedef struct source_z80_symbol_cache {
    source_z80_symbol_t *symbols;
    int symbolCount;
    int symbolCap;
    int nextOrder;
    int ready;
    char symbolBaseDir[PATH_MAX];
} source_z80_symbol_cache_t;

static source_z80_symbol_cache_t source_z80_symbolCache;

static int
source_z80_findProcessorId(uint32_t *outProcessorId);

static int
source_z80_resolveSymbol(uint64_t addr, const char **outName, uint16_t *outSymbolAddr);

static int
source_z80_resolveExactSymbol(uint16_t addr, const char **outName);

static int
source_z80_isHexDigit(char ch);

int
source_z80_isModeAvailable(void)
{
    uint32_t processorId = 0;

    if (target != target_neogeo()) {
        return 0;
    }
    return source_z80_findProcessorId(&processorId);
}

static int
source_z80_findProcessorId(uint32_t *outProcessorId)
{
    e9k_debug_processor_info_t processors[8];
    size_t count = 0;

    if (outProcessorId) {
        *outProcessorId = 0;
    }
    if (!libretro_host_debugReadProcessors(processors, countof(processors), &count)) {
        return 0;
    }
    if (count > countof(processors)) {
        count = countof(processors);
    }
    for (size_t i = 0; i < count; ++i) {
        if (strcasecmp(processors[i].name, "Z80") == 0) {
            if (outProcessorId) {
                *outProcessorId = processors[i].id;
            }
            return 1;
        }
    }
    return 0;
}

void
source_z80_copySymbolBaseDir(char *out, size_t cap)
{
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    const char *elf = debugger.libretro.exePath;
    if (!elf || !elf[0]) {
        return;
    }

    const char *slash = strrchr(elf, '/');
    const char *back = strrchr(elf, '\\');
    const char *sep = slash;
    if (back && (!sep || back > sep)) {
        sep = back;
    }
    if (!sep) {
        strutil_strlcpy(out, cap, ".");
        return;
    }

    size_t len = (size_t)(sep - elf);
    if (len == 0) {
        len = 1;
    }
    if (len >= cap) {
        len = cap - 1;
    }
    if (len > 0) {
        memcpy(out, elf, len);
    }
    out[len] = '\0';
}

static void
source_z80_clearSymbols(void)
{
    for (int i = 0; i < source_z80_symbolCache.symbolCount; ++i) {
        alloc_free(source_z80_symbolCache.symbols[i].name);
        source_z80_symbolCache.symbols[i].name = NULL;
    }
    alloc_free(source_z80_symbolCache.symbols);
    source_z80_symbolCache.symbols = NULL;
    source_z80_symbolCache.symbolCount = 0;
    source_z80_symbolCache.symbolCap = 0;
    source_z80_symbolCache.nextOrder = 0;
    source_z80_symbolCache.ready = 0;
    source_z80_symbolCache.symbolBaseDir[0] = '\0';
}

static int
source_z80_symbolCompare(const void *a, const void *b)
{
    const source_z80_symbol_t *sa = (const source_z80_symbol_t *)a;
    const source_z80_symbol_t *sb = (const source_z80_symbol_t *)b;

    if (sa->addr < sb->addr) {
        return -1;
    }
    if (sa->addr > sb->addr) {
        return 1;
    }
    if (sa->order < sb->order) {
        return -1;
    }
    if (sa->order > sb->order) {
        return 1;
    }
    return 0;
}

static int
source_z80_ensureSymbolCapacity(int minCap)
{
    if (source_z80_symbolCache.symbolCap >= minCap) {
        return 1;
    }

    int nextCap = source_z80_symbolCache.symbolCap > 0 ? source_z80_symbolCache.symbolCap : 256;
    while (nextCap < minCap) {
        nextCap *= 2;
    }

    source_z80_symbol_t *nextSymbols =
        (source_z80_symbol_t *)alloc_realloc(source_z80_symbolCache.symbols,
                                             (size_t)nextCap * sizeof(*nextSymbols));
    if (!nextSymbols) {
        return 0;
    }
    for (int i = source_z80_symbolCache.symbolCap; i < nextCap; ++i) {
        nextSymbols[i].addr = 0;
        nextSymbols[i].order = 0;
        nextSymbols[i].name = NULL;
    }
    source_z80_symbolCache.symbols = nextSymbols;
    source_z80_symbolCache.symbolCap = nextCap;
    return 1;
}

static int
source_z80_addSymbol(uint16_t addr, const char *name)
{
    if (!name || !name[0]) {
        return 0;
    }
    if (!source_z80_ensureSymbolCapacity(source_z80_symbolCache.symbolCount + 1)) {
        return 0;
    }

    char *nameDup = alloc_strdup(name);
    if (!nameDup) {
        return 0;
    }

    source_z80_symbol_t *symbol = &source_z80_symbolCache.symbols[source_z80_symbolCache.symbolCount++];
    symbol->addr = addr;
    symbol->order = source_z80_symbolCache.nextOrder++;
    symbol->name = nameDup;
    return 1;
}

static int
source_z80_parseNoiAddress(const char *text, uint16_t *outAddr)
{
    const char *digits = text;
    char normalized[64];
    size_t pos = 0;

    if (!text || !outAddr) {
        return 0;
    }
    if (digits[0] == '$') {
        digits++;
    } else if (digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X')) {
        digits += 2;
    }
    if (!source_z80_isHexDigit(*digits)) {
        return 0;
    }
    while (source_z80_isHexDigit(*digits)) {
        if (pos + 1 >= sizeof(normalized)) {
            return 0;
        }
        normalized[pos++] = *digits++;
    }
    if (*digits != '\0' || pos == 0) {
        return 0;
    }
    normalized[pos] = '\0';

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(normalized, &end, 16);
    if (errno != 0 || !end || *end != '\0' || value > 0xfffful) {
        return 0;
    }

    *outAddr = (uint16_t)value;
    return 1;
}

static int
source_z80_parseNoiDefLine(const char *line, uint16_t *outAddr, char *outName, size_t nameCap)
{
    char tag[8];
    char name[256];
    char addrText[64];

    if (!line || !outAddr || !outName || nameCap == 0) {
        return 0;
    }
    tag[0] = '\0';
    name[0] = '\0';
    addrText[0] = '\0';
    if (sscanf(line, "%7s %255s %63s", tag, name, addrText) != 3) {
        return 0;
    }
    if (strcmp(tag, "DEF") != 0 || !name[0]) {
        return 0;
    }

    uint16_t addr = 0;
    if (!source_z80_parseNoiAddress(addrText, &addr)) {
        return 0;
    }

    *outAddr = addr;
    strutil_strlcpy(outName, nameCap, name);
    return 1;
}

static int
source_z80_loadNoiFile(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }

    int added = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        uint16_t addr = 0;
        char name[256];

        if (source_z80_parseNoiDefLine(line, &addr, name, sizeof(name))) {
            if (source_z80_addSymbol(addr, name)) {
                added++;
            }
        }
    }

    fclose(fp);
    return added > 0 ? 1 : 0;
}

static void
source_z80_buildNoiPath(char *out, size_t cap, const char *leaf)
{
    char symbolBaseDir[PATH_MAX];

    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    source_z80_copySymbolBaseDir(symbolBaseDir, sizeof(symbolBaseDir));
    strutil_pathJoinTrunc(out, cap, symbolBaseDir, leaf);
}

static void
source_z80_loadSymbols(void)
{
    char symbolBaseDir[PATH_MAX];

    source_z80_copySymbolBaseDir(symbolBaseDir, sizeof(symbolBaseDir));
    if (!symbolBaseDir[0]) {
        source_z80_symbolCache.ready = 1;
        return;
    }

    char path[PATH_MAX];
    source_z80_buildNoiPath(path, sizeof(path), "demo_driver.noi");
    (void)source_z80_loadNoiFile(path);

    for (int bank = 1; bank < SOURCE_Z80_MAX_BANK_NOI_FILES; ++bank) {
        char leaf[64];

        snprintf(leaf, sizeof(leaf), "demo_driver_bank%d.noi", bank);
        source_z80_buildNoiPath(path, sizeof(path), leaf);
        (void)source_z80_loadNoiFile(path);
    }

    if (source_z80_symbolCache.symbolCount > 1) {
        qsort(source_z80_symbolCache.symbols,
              (size_t)source_z80_symbolCache.symbolCount,
              sizeof(*source_z80_symbolCache.symbols),
              source_z80_symbolCompare);
    }
    strutil_strlcpy(source_z80_symbolCache.symbolBaseDir,
                    sizeof(source_z80_symbolCache.symbolBaseDir),
                    symbolBaseDir);
    source_z80_symbolCache.ready = 1;
}

static void
source_z80_ensureSymbols(void)
{
    char symbolBaseDir[PATH_MAX];

    source_z80_copySymbolBaseDir(symbolBaseDir, sizeof(symbolBaseDir));
    if (source_z80_symbolCache.ready &&
        strcmp(source_z80_symbolCache.symbolBaseDir, symbolBaseDir) == 0) {
        return;
    }

    source_z80_clearSymbols();
    source_z80_loadSymbols();
}

static int
source_z80_resolveSymbol(uint64_t addr, const char **outName, uint16_t *outSymbolAddr)
{
    if (outName) {
        *outName = NULL;
    }
    if (outSymbolAddr) {
        *outSymbolAddr = 0;
    }

    source_z80_ensureSymbols();
    if (source_z80_symbolCache.symbolCount <= 0) {
        return 0;
    }

    uint16_t queryAddr = (uint16_t)source_z80_resolveAnchorAddr(addr);
    int lo = 0;
    int hi = source_z80_symbolCache.symbolCount - 1;
    int best = -1;

    while (lo <= hi) {
        int mid = lo + ((hi - lo) / 2);
        uint16_t symbolAddr = source_z80_symbolCache.symbols[mid].addr;

        if (symbolAddr <= queryAddr) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (best < 0) {
        return 0;
    }

    while (best > 0 &&
           source_z80_symbolCache.symbols[best - 1].addr == source_z80_symbolCache.symbols[best].addr) {
        best--;
    }

    if (outName) {
        *outName = source_z80_symbolCache.symbols[best].name;
    }
    if (outSymbolAddr) {
        *outSymbolAddr = source_z80_symbolCache.symbols[best].addr;
    }
    return source_z80_symbolCache.symbols[best].name ? 1 : 0;
}

static int
source_z80_resolveExactSymbol(uint16_t addr, const char **outName)
{
    if (outName) {
        *outName = NULL;
    }

    source_z80_ensureSymbols();
    if (source_z80_symbolCache.symbolCount <= 0) {
        return 0;
    }

    int lo = 0;
    int hi = source_z80_symbolCache.symbolCount - 1;
    int best = -1;

    while (lo <= hi) {
        int mid = lo + ((hi - lo) / 2);
        uint16_t symbolAddr = source_z80_symbolCache.symbols[mid].addr;

        if (symbolAddr == addr) {
            best = mid;
            break;
        }
        if (symbolAddr < addr) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (best < 0) {
        return 0;
    }
    while (best > 0 &&
           source_z80_symbolCache.symbols[best - 1].addr == source_z80_symbolCache.symbols[best].addr) {
        best--;
    }

    if (!source_z80_symbolCache.symbols[best].name) {
        return 0;
    }
    if (outName) {
        *outName = source_z80_symbolCache.symbols[best].name;
    }
    return 1;
}

int
source_z80_getSymbolCount(void)
{
    source_z80_ensureSymbols();
    return source_z80_symbolCache.symbolCount;
}

int
source_z80_getSymbol(int index, const char **outName, uint16_t *outAddr)
{
    if (outName) {
        *outName = NULL;
    }
    if (outAddr) {
        *outAddr = 0;
    }

    source_z80_ensureSymbols();
    if (index < 0 || index >= source_z80_symbolCache.symbolCount) {
        return 0;
    }

    source_z80_symbol_t *symbol = &source_z80_symbolCache.symbols[index];
    if (!symbol->name) {
        return 0;
    }
    if (outName) {
        *outName = symbol->name;
    }
    if (outAddr) {
        *outAddr = symbol->addr;
    }
    return 1;
}

uint32_t
source_z80_processorId(void)
{
    uint32_t processorId = 0;

    (void)source_z80_findProcessorId(&processorId);
    return processorId;
}

uint64_t
source_z80_resolveAnchorAddr(uint64_t addr)
{
    return addr & 0xffffull;
}

static int
source_z80_readPc(uint64_t *outAddr)
{
    e9k_debug_processor_reg_t regs[32];
    size_t count = 0;

    if (!outAddr) {
        return 0;
    }
    if (!libretro_host_debugReadProcessorRegs(source_z80_processorId(), regs, countof(regs), &count)) {
        return 0;
    }
    for (size_t i = 0; i < count; ++i) {
        if (strcasecmp(regs[i].name, "PC") == 0) {
            *outAddr = source_z80_resolveAnchorAddr(regs[i].value);
            return 1;
        }
    }
    return 0;
}

uint64_t
source_z80_getCurrentAddr(source_pane_state_t *st)
{
    uint64_t addr = 0;

    if (st && st->overrideActive) {
        return source_z80_resolveAnchorAddr(st->overrideAddr);
    }
    if (source_z80_readPc(&addr)) {
        return addr;
    }
    if (st && st->scrollAnchorValid) {
        return source_z80_resolveAnchorAddr(st->scrollAnchorAddr);
    }
    return 0;
}

static size_t
source_z80_appendChar(char *out, size_t cap, size_t *pos, char ch)
{
    if (!out || cap == 0 || !pos || *pos + 1 >= cap) {
        return 0;
    }
    out[*pos] = ch;
    (*pos)++;
    out[*pos] = '\0';
    return 1;
}

static void
source_z80_appendText(char *out, size_t cap, size_t *pos, const char *text)
{
    if (!text) {
        return;
    }
    for (size_t i = 0; text[i]; ++i) {
        if (!source_z80_appendChar(out, cap, pos, text[i])) {
            return;
        }
    }
}

static int
source_z80_isHexDigit(char ch)
{
    return isxdigit((unsigned char)ch) ? 1 : 0;
}

static int
source_z80_isHexSuffixLiteral(const char *text, size_t start, size_t end)
{
    if (!text || end <= start + 1 || text[end - 1] != 'h') {
        return 0;
    }
    if (!isdigit((unsigned char)text[start])) {
        return 0;
    }
    for (size_t i = start; i + 1 < end; ++i) {
        if (!source_z80_isHexDigit(text[i])) {
            return 0;
        }
    }
    return 1;
}

static void
source_z80_formatDisassembly(char *out, size_t cap, const char *text)
{
    size_t pos = 0;
    size_t i = 0;

    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!text) {
        return;
    }

    while (text[i] && pos + 1 < cap) {
        if (isdigit((unsigned char)text[i])) {
            size_t start = i;
            while (source_z80_isHexDigit(text[i])) {
                i++;
            }
            if (text[i] == 'h') {
                i++;
                if (source_z80_isHexSuffixLiteral(text, start, i)) {
                    size_t digitStart = start;
                    if (!source_z80_appendChar(out, cap, &pos, '$')) {
                        return;
                    }
                    if (digitStart + 2 < i &&
                        text[digitStart] == '0' &&
                        isalpha((unsigned char)text[digitStart + 1])) {
                        digitStart++;
                    }
                    for (size_t j = digitStart; j + 1 < i; ++j) {
                        if (!source_z80_appendChar(out, cap, &pos, (char)toupper((unsigned char)text[j]))) {
                            return;
                        }
                    }
                    continue;
                }
            }
            for (size_t j = start; j < i && pos + 1 < cap; ++j) {
                if (!source_z80_appendChar(out, cap, &pos, text[j])) {
                    return;
                }
            }
            continue;
        }
        if (!source_z80_appendChar(out, cap, &pos, text[i])) {
            return;
        }
        i++;
    }
}

static int
source_z80_isTargetMnemonic(const char *line)
{
    if (!line) {
        return 0;
    }
    return strncasecmp(line, "call", 4) == 0 ||
           strncasecmp(line, "jp", 2) == 0 ||
           strncasecmp(line, "jr", 2) == 0 ||
           strncasecmp(line, "djnz", 4) == 0;
}

static int
source_z80_parseHexOperand(const char *text, uint16_t *outAddr)
{
    const char *p = text;

    if (!text || !outAddr) {
        return 0;
    }
    while (*p && *p != '$') {
        p++;
    }
    if (*p != '$') {
        return 0;
    }
    p++;
    if (!source_z80_isHexDigit(*p)) {
        return 0;
    }

    unsigned long value = 0;
    int digits = 0;
    while (source_z80_isHexDigit(*p)) {
        value <<= 4;
        if (*p >= '0' && *p <= '9') {
            value += (unsigned long)(*p - '0');
        } else {
            value += (unsigned long)(toupper((unsigned char)*p) - 'A' + 10);
        }
        digits++;
        p++;
    }
    if (digits <= 0 || value > 0xfffful) {
        return 0;
    }

    *outAddr = (uint16_t)value;
    return 1;
}

static void
source_z80_annotateTargetSymbol(char *out, size_t cap, const char *line)
{
    uint16_t targetAddr = 0;
    const char *symbolName = NULL;
    char addrText[16];
    size_t pos = 0;

    if (!out || cap == 0) {
        return;
    }
    strutil_strlcpy(out, cap, line ? line : "");
    if (!line || !source_z80_isTargetMnemonic(line)) {
        return;
    }
    if (!source_z80_parseHexOperand(line, &targetAddr)) {
        return;
    }
    if (!source_z80_resolveExactSymbol(targetAddr, &symbolName) || !symbolName) {
        return;
    }

    snprintf(addrText, sizeof(addrText), "$%04X", (unsigned int)targetAddr);
    out[0] = '\0';
    source_z80_appendText(out, cap, &pos, line);
    source_z80_appendText(out, cap, &pos, " ; ");
    source_z80_appendText(out, cap, &pos, symbolName);
    source_z80_appendText(out, cap, &pos, " (");
    source_z80_appendText(out, cap, &pos, addrText);
    source_z80_appendText(out, cap, &pos, ")");
}

static size_t
source_z80_disassembleLine(uint64_t addr, char *out, size_t cap)
{
    char disasm[256];
    char formatted[256];
    char annotated[320];
    size_t insnLen = 0;

    if (!out || cap == 0) {
        return 1;
    }
    out[0] = '\0';
    disasm[0] = '\0';
    formatted[0] = '\0';
    annotated[0] = '\0';

    if (!libretro_host_debugDisassembleProcessorQuick(source_z80_processorId(),
                                                      (uint32_t)source_z80_resolveAnchorAddr(addr),
                                                      disasm,
                                                      sizeof(disasm),
                                                      &insnLen) ||
        insnLen == 0) {
        strutil_strlcpy(out, cap, "??");
        return 1;
    }
    if (insnLen > SOURCE_Z80_MAX_INSN_BYTES) {
        insnLen = SOURCE_Z80_MAX_INSN_BYTES;
    }
    source_z80_formatDisassembly(formatted, sizeof(formatted), disasm);
    source_z80_annotateTargetSymbol(annotated, sizeof(annotated), formatted);

    const char *symbolName = NULL;
    uint16_t symbolAddr = 0;
    if (source_z80_resolveSymbol(addr, &symbolName, &symbolAddr) && symbolName) {
        uint16_t resolvedAddr = (uint16_t)source_z80_resolveAnchorAddr(addr);

        if (resolvedAddr == symbolAddr) {
            size_t pos = 0;

            out[0] = '\0';
            source_z80_appendText(out, cap, &pos, symbolName);
            source_z80_appendText(out, cap, &pos, ": ");
            source_z80_appendText(out, cap, &pos, annotated);
        } else {
            strutil_strlcpy(out, cap, annotated);
        }
    } else {
        strutil_strlcpy(out, cap, annotated);
    }

    return insnLen == 0 ? 1 : insnLen;
}

int
source_z80_getWindow(source_pane_state_t *st, int maxLines, uint64_t *outCurAddr,
                     const char ***outLines, const uint64_t **outAddrs, int *outCount)
{
    if (outCurAddr) {
        *outCurAddr = 0;
    }
    if (outLines) {
        *outLines = NULL;
    }
    if (outAddrs) {
        *outAddrs = NULL;
    }
    if (outCount) {
        *outCount = 0;
    }
    if (!st || maxLines <= 0 || !outCurAddr || !outLines || !outAddrs || !outCount) {
        return 0;
    }

    int freezeWhileRunning = source_pane_shouldFreezeAsmWhileRunning(st);
    if (st->frozenActive && !freezeWhileRunning) {
        st->frozenActive = 0;
        source_pane_freeFrozenAsm(st);
    }
    if (freezeWhileRunning && !st->frozenActive) {
        st->frozenPcAddr = source_z80_getCurrentAddr(st);
        st->frozenActive = 1;
        st->frozenAsmStartIndex = -1;
        st->frozenAsmMaxLines = 0;
        source_pane_freeFrozenAsm(st);
    }

    uint64_t curAddr = freezeWhileRunning ? st->frozenPcAddr : source_z80_getCurrentAddr(st);
    curAddr = source_z80_resolveAnchorAddr(curAddr);
    *outCurAddr = curAddr;

    uint64_t startAddr = curAddr;
    if (st->scrollLocked) {
        if (st->scrollAnchorValid) {
            startAddr = st->scrollAnchorAddr;
        } else if (st->scrollIndex >= 0) {
            startAddr = (uint64_t)(uint32_t)st->scrollIndex;
        }
    } else if (maxLines > 1) {
        uint64_t back = (uint64_t)(maxLines / 2);
        startAddr = curAddr > back ? curAddr - back : 0;
    }
    startAddr = source_z80_resolveAnchorAddr(startAddr);
    int startIndex = (int)startAddr;

    if (freezeWhileRunning &&
        st->frozenActive &&
        st->frozenAsmLines &&
        st->frozenAsmAddrs &&
        st->frozenAsmStartIndex == startIndex &&
        st->frozenAsmMaxLines == maxLines) {
        *outLines = (const char **)st->frozenAsmLines;
        *outAddrs = (const uint64_t *)st->frozenAsmAddrs;
        *outCount = st->frozenAsmCount;
        return st->frozenAsmCount > 0 ? 1 : 0;
    }

    source_pane_freeFrozenAsm(st);
    st->frozenAsmLines = (char **)alloc_calloc((size_t)maxLines, sizeof(*st->frozenAsmLines));
    st->frozenAsmAddrs = (uint64_t *)alloc_calloc((size_t)maxLines, sizeof(*st->frozenAsmAddrs));
    if (!st->frozenAsmLines || !st->frozenAsmAddrs) {
        source_pane_freeFrozenAsm(st);
        return 0;
    }

    uint64_t addr = startAddr;
    for (int i = 0; i < maxLines; ++i) {
        char line[320];
        size_t insnLen = source_z80_disassembleLine(addr, line, sizeof(line));
        st->frozenAsmLines[i] = alloc_strdup(line);
        if (!st->frozenAsmLines[i]) {
            break;
        }
        st->frozenAsmAddrs[i] = source_z80_resolveAnchorAddr(addr);
        st->frozenAsmCount++;
        if (insnLen == 0) {
            insnLen = 1;
        }
        addr = source_z80_resolveAnchorAddr(addr + insnLen);
    }

    if (st->frozenAsmCount <= 0) {
        source_pane_freeFrozenAsm(st);
        return 0;
    }

    st->frozenAsmStartIndex = startIndex;
    st->frozenAsmMaxLines = maxLines;
    if (!st->scrollLocked) {
        st->scrollIndex = startIndex;
    } else if (!st->scrollAnchorValid) {
        st->scrollAnchorAddr = startAddr;
        st->scrollAnchorValid = 1;
    }

    *outLines = (const char **)st->frozenAsmLines;
    *outAddrs = (const uint64_t *)st->frozenAsmAddrs;
    *outCount = st->frozenAsmCount;
    return 1;
}
