/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "alloc.h"
#include "base_map.h"
#include "config.h"
#include "debugger.h"
#include "print_eval.h"
#include "symbol_text_map.h"
#include "target.h"

typedef struct console_cmd_symbols_export_symbol {
    char *name;
    uint32_t addr;
    symbol_text_map_symbol_kind_t kind;
} console_cmd_symbols_export_symbol_t;

static int
console_cmd_symbols_exportSymbolKindPriority(symbol_text_map_symbol_kind_t kind)
{
    switch (kind) {
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION:
        return 0;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE:
        return 1;
    case SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN:
    default:
        return 2;
    }
}

static const char *
console_cmd_symbols_exportSymbolKindName(symbol_text_map_symbol_kind_t kind)
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

static void
console_cmd_symbols_printUsage(void)
{
    debug_printf("Usage: symbols export <path>\n");
    debug_printf("       symbols load <path>\n");
    debug_printf("       symbols save\n");
    debug_printf("       symbols list\n");
    debug_printf("       symbols show <symbol>\n");
    debug_printf("       symbols lookup <addr>\n");
    debug_printf("       symbols add <symbol> <addr> <function|variable|unknown>\n");
    debug_printf("       symbols delete <symbol>\n");
    debug_printf("       symbols reset\n");
}

static int
console_cmd_symbols_parseKind(const char *text, symbol_text_map_symbol_kind_t *outKind)
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

static void
console_cmd_symbols_invalidateCaches(void)
{
    print_eval_invalidateCache();
    debugger_refreshElfValid();
}

static int
console_cmd_symbols_setActivePath(const char *path)
{
    if (!path || !path[0]) {
        return 0;
    }

    if (target == target_amiga()) {
        debugger_copyPath(debugger.config.amiga.libretro.exePath,
                          sizeof(debugger.config.amiga.libretro.exePath),
                          path);
    } else if (target == target_neogeo()) {
        debugger_copyPath(debugger.config.neogeo.libretro.exePath,
                          sizeof(debugger.config.neogeo.libretro.exePath),
                          path);
    } else if (target == target_megadrive()) {
        debugger_copyPath(debugger.config.megadrive.libretro.exePath,
                          sizeof(debugger.config.megadrive.libretro.exePath),
                          path);
    } else {
        return 0;
    }

    debugger_copyPath(debugger.libretro.exePath, sizeof(debugger.libretro.exePath), path);
    config_saveConfig();
    return 1;
}

static int
console_cmd_symbols_getEditablePath(const char **outPath)
{
    const char *path = debugger.libretro.exePath;
    if (outPath) {
        *outPath = NULL;
    }
    if (!path || !path[0]) {
        return 0;
    }
    if (!symbol_text_map_hasActive() && !symbol_text_map_canLoad(path)) {
        return 0;
    }
    if (outPath) {
        *outPath = path;
    }
    return 1;
}

static int
console_cmd_symbols_printEntry(const symbol_text_map_entry_t *entry)
{
    if (!entry || !entry->name || !entry->name[0]) {
        return 0;
    }
    debug_printf("0x%X %s %s\n",
                 (unsigned)(entry->addr & 0x00ffffffu),
                 entry->name,
                 console_cmd_symbols_exportSymbolKindName(entry->kind));
    return 1;
}

static symbol_text_map_symbol_kind_t
console_cmd_symbols_exportSymbolKindFromTokens(char **tokens, int count)
{
    if (!tokens || count <= 0) {
        return SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
    }
    for (int i = 1; i < count - 1; ++i) {
        const char *token = tokens[i];
        if (!token || !token[0]) {
            continue;
        }
        if (strcmp(token, "F") == 0 || strcmp(token, "FUNC") == 0) {
            return SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION;
        }
        if (strcmp(token, "O") == 0 || strcmp(token, "OBJECT") == 0) {
            return SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE;
        }
    }
    return SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
}

static void
console_cmd_symbols_exportSymbolsFree(console_cmd_symbols_export_symbol_t *symbols, int count)
{
    if (!symbols) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        alloc_free(symbols[i].name);
    }
    alloc_free(symbols);
}

static int
console_cmd_symbols_exportSymbolsAdd(console_cmd_symbols_export_symbol_t **symbols,
                             int *count,
                             int *cap,
                             const char *name,
                             uint32_t addr,
                             symbol_text_map_symbol_kind_t kind)
{
    if (!symbols || !count || !cap || !name || !name[0]) {
        return 0;
    }
    for (int i = 0; i < *count; ++i) {
        console_cmd_symbols_export_symbol_t *existing = &(*symbols)[i];
        if (existing->addr == (addr & 0x00ffffffu) &&
            strcmp(existing->name, name) == 0) {
            if (console_cmd_symbols_exportSymbolKindPriority(kind) <
                console_cmd_symbols_exportSymbolKindPriority(existing->kind)) {
                existing->kind = kind;
            }
            return 1;
        }
    }
    if (*count >= *cap) {
        int nextCap = *cap ? (*cap * 2) : 256;
        console_cmd_symbols_export_symbol_t *nextSymbols =
            (console_cmd_symbols_export_symbol_t *)alloc_realloc(*symbols, sizeof(*nextSymbols) * (size_t)nextCap);
        if (!nextSymbols) {
            return 0;
        }
        *symbols = nextSymbols;
        *cap = nextCap;
    }
    char *nameDup = alloc_strdup(name);
    if (!nameDup) {
        return 0;
    }
    console_cmd_symbols_export_symbol_t *entry = &(*symbols)[(*count)++];
    entry->name = nameDup;
    entry->addr = addr & 0x00ffffffu;
    entry->kind = kind;
    return 1;
}

static int
console_cmd_symbols_exportSymbolsCompare(const void *a, const void *b)
{
    const console_cmd_symbols_export_symbol_t *sa = (const console_cmd_symbols_export_symbol_t *)a;
    const console_cmd_symbols_export_symbol_t *sb = (const console_cmd_symbols_export_symbol_t *)b;
    if (sa->addr < sb->addr) {
        return -1;
    }
    if (sa->addr > sb->addr) {
        return 1;
    }
    int nameCmp = strcmp(sa->name ? sa->name : "", sb->name ? sb->name : "");
    if (nameCmp != 0) {
        return nameCmp;
    }
    return console_cmd_symbols_exportSymbolKindPriority(sa->kind) -
           console_cmd_symbols_exportSymbolKindPriority(sb->kind);
}

static int
console_cmd_symbols_exportSymbolsFromTextMap(const char *path,
                                     console_cmd_symbols_export_symbol_t **outSymbols,
                                     int *outCount)
{
    if (outSymbols) {
        *outSymbols = NULL;
    }
    if (outCount) {
        *outCount = 0;
    }
    if (!path || !*path || !outSymbols || !outCount) {
        return 0;
    }
    const symbol_text_map_entry_t *entries = NULL;
    int entryCount = 0;
    if (!symbol_text_map_getEntries(path, &entries, &entryCount) || !entries || entryCount <= 0) {
        return 0;
    }
    console_cmd_symbols_export_symbol_t *symbols = NULL;
    int count = 0;
    int cap = 0;
    for (int i = 0; i < entryCount; ++i) {
        const symbol_text_map_entry_t *entry = &entries[i];
        if (!console_cmd_symbols_exportSymbolsAdd(&symbols, &count, &cap,
                                          entry->name, entry->addr, entry->kind)) {
            console_cmd_symbols_exportSymbolsFree(symbols, count);
            return 0;
        }
    }
    *outSymbols = symbols;
    *outCount = count;
    return count > 0 ? 1 : 0;
}

static int
console_cmd_symbols_exportSymbolsFromBinary(const char *path,
                                    console_cmd_symbols_export_symbol_t **outSymbols,
                                    int *outCount)
{
    if (outSymbols) {
        *outSymbols = NULL;
    }
    if (outCount) {
        *outCount = 0;
    }
    if (!path || !*path || !outSymbols || !outCount) {
        return 0;
    }
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        return 0;
    }
    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "--syms", path, 0)) {
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    console_cmd_symbols_export_symbol_t *symbols = NULL;
    int count = 0;
    int cap = 0;
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[16];
        int tokenCount = 0;
        char *cursor = line;
        while (tokenCount < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            while (*cursor && isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (!*cursor) {
                break;
            }
            tokens[tokenCount++] = cursor;
            while (*cursor && !isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (*cursor) {
                *cursor++ = '\0';
            }
        }
        if (tokenCount < 2) {
            continue;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(tokens[0], &addr)) {
            continue;
        }
        const char *section = NULL;
        for (int i = 1; i < tokenCount - 1; ++i) {
            const char *token = tokens[i];
            if (!token || !token[0]) {
                continue;
            }
            if (token[0] == '.' || token[0] == '*'
                || strcmp(token, "UND") == 0) {
                section = token;
                break;
            }
        }
        const char *name = tokens[tokenCount - 1];
        if (!name || !name[0]) {
            continue;
        }
        symbol_text_map_symbol_kind_t kind = console_cmd_symbols_exportSymbolKindFromTokens(tokens, tokenCount);
        uint32_t runtimeAddr = addr;
        if (debugger_toolchainUsesHunkAddr2line()) {
            (void)base_map_symbolToRuntimeHunk(section, addr, &runtimeAddr);
        } else {
            (void)base_map_symbolToRuntime(section, addr, &runtimeAddr);
        }
        if (!console_cmd_symbols_exportSymbolsAdd(&symbols, &count, &cap, name, runtimeAddr, kind)) {
            pclose(fp);
            console_cmd_symbols_exportSymbolsFree(symbols, count);
            return 0;
        }
    }
    pclose(fp);
    *outSymbols = symbols;
    *outCount = count;
    return count > 0 ? 1 : 0;
}

static int
console_cmd_symbols_exportSymbolsSupplementReadelfFunctions(const char *path,
                                                    console_cmd_symbols_export_symbol_t **ioSymbols,
                                                    int *ioCount,
                                                    int *ioCap)
{
    if (!path || !*path || !ioSymbols || !ioCount || !ioCap) {
        return 0;
    }
    char readelf[PATH_MAX];
    if (!debugger_toolchainBuildBinary(readelf, sizeof(readelf), "readelf")) {
        return 0;
    }
    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), readelf, "-Ws", path, 1)) {
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    int added = 0;
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[12];
        int tokenCount = 0;
        char *cursor = line;
        while (*cursor && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (!*cursor || !isdigit((unsigned char)*cursor)) {
            continue;
        }
        while (tokenCount < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            while (*cursor && isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (!*cursor) {
                break;
            }
            tokens[tokenCount++] = cursor;
            while (*cursor && !isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (*cursor) {
                *cursor++ = '\0';
            }
        }
        if (tokenCount < 8) {
            continue;
        }
        if (strcmp(tokens[3], "FUNC") != 0 || strcmp(tokens[6], "UND") == 0) {
            continue;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(tokens[1], &addr)) {
            continue;
        }
        const char *name = tokens[7];
        if (!name || !name[0]) {
            continue;
        }
        uint32_t runtimeAddr = addr;
        if (debugger_toolchainUsesHunkAddr2line()) {
            (void)base_map_symbolToRuntimeHunk(tokens[6], addr, &runtimeAddr);
        } else {
            (void)base_map_symbolToRuntime(tokens[6], addr, &runtimeAddr);
        }
        if (console_cmd_symbols_exportSymbolsAdd(ioSymbols,
                                         ioCount,
                                         ioCap,
                                         name,
                                         runtimeAddr,
                                         SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION)) {
            added++;
        }
    }
    pclose(fp);
    return added;
}

static int
console_cmd_symbols_exportSymbolsSupplementObjdumpTextFunctions(const char *path,
                                                        console_cmd_symbols_export_symbol_t **ioSymbols,
                                                        int *ioCount,
                                                        int *ioCap)
{
    if (!path || !*path || !ioSymbols || !ioCount || !ioCap) {
        return 0;
    }
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        return 0;
    }
    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "-t", path, 1)) {
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    int added = 0;
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[16];
        int tokenCount = 0;
        char *cursor = line;
        while (*cursor && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (!*cursor || !isxdigit((unsigned char)*cursor)) {
            continue;
        }
        while (tokenCount < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            while (*cursor && isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (!*cursor) {
                break;
            }
            tokens[tokenCount++] = cursor;
            while (*cursor && !isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (*cursor) {
                *cursor++ = '\0';
            }
        }
        if (tokenCount < 4) {
            continue;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(tokens[0], &addr)) {
            continue;
        }
        int textSymbol = 0;
        for (int i = 1; i < tokenCount; ++i) {
            if (strcmp(tokens[i], ".text") == 0 || strncmp(tokens[i], ".text.", 6) == 0) {
                textSymbol = 1;
                break;
            }
        }
        if (!textSymbol) {
            continue;
        }
        const char *name = tokens[tokenCount - 1];
        if (!name || !name[0]) {
            continue;
        }
        uint32_t runtimeAddr = addr;
        if (debugger_toolchainUsesHunkAddr2line()) {
            (void)base_map_symbolToRuntimeHunk(".text", addr, &runtimeAddr);
        } else {
            (void)base_map_symbolToRuntime(".text", addr, &runtimeAddr);
        }
        if (console_cmd_symbols_exportSymbolsAdd(ioSymbols,
                                         ioCount,
                                         ioCap,
                                         name,
                                         runtimeAddr,
                                         SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION)) {
            added++;
        }
    }
    pclose(fp);
    return added;
}

static int
console_cmd_symbols_exportSymbolsSupplementStabsFunctions(const char *path,
                                                  console_cmd_symbols_export_symbol_t **ioSymbols,
                                                  int *ioCount,
                                                  int *ioCap)
{
    if (!path || !*path || !ioSymbols || !ioCount || !ioCap) {
        return 0;
    }
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        return 0;
    }
    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "-G", path, 1)) {
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    int added = 0;
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[12];
        int tokenCount = 0;
        char *cursor = line;
        while (*cursor && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (!*cursor) {
            continue;
        }
        while (tokenCount < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            while (*cursor && isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (!*cursor) {
                break;
            }
            tokens[tokenCount++] = cursor;
            while (*cursor && !isspace((unsigned char)*cursor)) {
                ++cursor;
            }
            if (*cursor) {
                *cursor++ = '\0';
            }
        }
        if (tokenCount < 7) {
            continue;
        }
        if (strcmp(tokens[1], "FUN") != 0) {
            continue;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(tokens[4], &addr) || addr == 0) {
            continue;
        }
        const char *stab = tokens[tokenCount - 1];
        if (!stab || !stab[0]) {
            continue;
        }
        const char *end = strchr(stab, ':');
        size_t len = end ? (size_t)(end - stab) : strlen(stab);
        if (len == 0 || len >= 1024) {
            continue;
        }
        char name[1024];
        memcpy(name, stab, len);
        name[len] = '\0';
        if (!name[0]) {
            continue;
        }
        uint32_t runtimeAddr = addr;
        if (debugger_toolchainUsesHunkAddr2line()) {
            (void)base_map_symbolToRuntimeHunk(".text", addr, &runtimeAddr);
        } else {
            (void)base_map_symbolToRuntime(".text", addr, &runtimeAddr);
        }
        if (console_cmd_symbols_exportSymbolsAdd(ioSymbols,
                                         ioCount,
                                         ioCap,
                                         name,
                                         runtimeAddr,
                                         SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION)) {
            added++;
        }
    }
    pclose(fp);
    return added;
}

static int
console_cmd_symbols_exportSymbolsSupplementFromPrintEval(console_cmd_symbols_export_symbol_t **ioSymbols,
                                                 int *ioCount,
                                                 int *ioCap)
{
    if (!ioSymbols || !ioCount || !ioCap) {
        return 0;
    }
    char **names = NULL;
    int nameCount = 0;
    if (!print_eval_complete("", &names, &nameCount) || !names || nameCount <= 0) {
        return 0;
    }
    int added = 0;
    for (int i = 0; i < nameCount; ++i) {
        const char *name = names[i];
        uint32_t addr = 0;
        size_t size = 0;
        int isVariable = 0;
        if (!name || !name[0]) {
            continue;
        }
        if (!print_eval_resolveSymbol(name, &addr, &size)) {
            continue;
        }
        if (addr == 0) {
            continue;
        }
        symbol_text_map_symbol_kind_t kind = SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
        if (print_eval_resolveNamedKind(name, &isVariable)) {
            kind = isVariable ? SYMBOL_TEXT_MAP_SYMBOL_KIND_VARIABLE :
                                SYMBOL_TEXT_MAP_SYMBOL_KIND_FUNCTION;
        }
        if (console_cmd_symbols_exportSymbolsAdd(ioSymbols,
                                         ioCount,
                                         ioCap,
                                         name,
                                         addr,
                                         kind)) {
            added++;
        }
        (void)size;
    }
    print_eval_freeCompletions(names, nameCount);
    return added;
}

static int
console_cmd_symbols_exportSymbolsWriteFile(const char *outPath, console_cmd_symbols_export_symbol_t *symbols, int count)
{
    if (!outPath || !*outPath || !symbols || count <= 0) {
        return 0;
    }
    qsort(symbols, (size_t)count, sizeof(*symbols), console_cmd_symbols_exportSymbolsCompare);
    FILE *fp = fopen(outPath, "w");
    if (!fp) {
        return 0;
    }
    for (int i = 0; i < count; ++i) {
        const console_cmd_symbols_export_symbol_t *entry = &symbols[i];
        if (!entry->name || !entry->name[0]) {
            continue;
        }
        fprintf(fp, "0x%06X %s %s\n",
                (unsigned)(entry->addr & 0x00ffffffu),
                entry->name,
                console_cmd_symbols_exportSymbolKindName(entry->kind));
    }
    fclose(fp);
    return 1;
}

int
console_cmd_symbols_command(int argc, char **argv)
{
    if (argc < 2) {
        console_cmd_symbols_printUsage();
        return 0;
    }
    if (strcasecmp(argv[1], "export") == 0) {
        if (argc < 3 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }

        const char *symbolPath = debugger.libretro.exePath;
        if (!symbolPath || !symbolPath[0] || !debugger.symbolValid) {
            debug_error("symbols: no debug symbol file configured");
            return 0;
        }

        console_cmd_symbols_export_symbol_t *symbols = NULL;
        int symbolCount = 0;
        int ok = 0;
        if (debugger.symbolFileKind == DEBUGGER_SYMBOL_FILE_KIND_TEXT_MAP) {
            ok = console_cmd_symbols_exportSymbolsFromTextMap(symbolPath, &symbols, &symbolCount);
        } else {
            ok = console_cmd_symbols_exportSymbolsFromBinary(symbolPath, &symbols, &symbolCount);
            if (!symbols) {
                symbols = NULL;
                symbolCount = 0;
            }
            int symbolCap = symbolCount;
            if (debugger.elfValid) {
                (void)console_cmd_symbols_exportSymbolsSupplementReadelfFunctions(symbolPath,
                                                                          &symbols,
                                                                          &symbolCount,
                                                                          &symbolCap);
                (void)console_cmd_symbols_exportSymbolsSupplementStabsFunctions(symbolPath,
                                                                        &symbols,
                                                                        &symbolCount,
                                                                        &symbolCap);
                (void)console_cmd_symbols_exportSymbolsSupplementObjdumpTextFunctions(symbolPath,
                                                                              &symbols,
                                                                              &symbolCount,
                                                                              &symbolCap);
            }
            if (console_cmd_symbols_exportSymbolsSupplementFromPrintEval(&symbols, &symbolCount, &symbolCap) > 0) {
                ok = 1;
            }
        }
        if (!ok || !symbols || symbolCount <= 0) {
            console_cmd_symbols_exportSymbolsFree(symbols, symbolCount);
            debug_error("symbols: failed to collect symbols from '%s'", symbolPath);
            return 0;
        }

        if (!console_cmd_symbols_exportSymbolsWriteFile(argv[2], symbols, symbolCount)) {
            console_cmd_symbols_exportSymbolsFree(symbols, symbolCount);
            debug_error("symbols: failed to write '%s'", argv[2]);
            return 0;
        }

        debug_printf("symbols: exported %d symbols to %s\n", symbolCount, argv[2]);
        console_cmd_symbols_exportSymbolsFree(symbols, symbolCount);
        return 1;
    }

    if (strcasecmp(argv[1], "load") == 0) {
        const char *activePath = NULL;
        if (argc < 3 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (console_cmd_symbols_getEditablePath(&activePath)) {
            if (!symbol_text_map_importFile(argv[2])) {
                debug_error("symbols: failed to merge '%s'", argv[2]);
                return 0;
            }
            console_cmd_symbols_invalidateCaches();
            debug_printf("symbols: loaded %s (merged)\n", argv[2]);
            return 1;
        }

        if (!symbol_text_map_canLoad(argv[2])) {
            debug_error("symbols: failed to load text symbol file '%s'", argv[2]);
            return 0;
        }
        symbol_text_map_clear();
        if (!console_cmd_symbols_setActivePath(argv[2])) {
            debug_error("symbols: failed to activate symbol file '%s'", argv[2]);
            return 0;
        }
        console_cmd_symbols_invalidateCaches();
        debug_printf("symbols: loaded %s\n", argv[2]);
        return 1;
    }

    if (strcasecmp(argv[1], "save") == 0) {
        const char *path = NULL;
        if (argc != 2) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(&path)) {
            debug_error("symbols: no text symbol file configured");
            return 0;
        }
        if (!symbol_text_map_saveToPath(path)) {
            debug_error("symbols: failed to write '%s'", path);
            return 0;
        }
        debug_printf("symbols: saved active symbol table to %s\n", path);
        return 1;
    }

    if (strcasecmp(argv[1], "list") == 0) {
        const char *path = NULL;
        const symbol_text_map_entry_t *entries = NULL;
        int count = 0;
        if (argc != 2) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(&path)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!symbol_text_map_getEntries(path, &entries, &count)) {
            debug_error("symbols: failed to read '%s'", path);
            return 0;
        }
        if (count <= 0) {
            debug_printf("symbols: table is empty\n");
            return 1;
        }
        for (int i = 0; i < count; ++i) {
            (void)console_cmd_symbols_printEntry(&entries[i]);
        }
        debug_printf("symbols: listed %d symbol%s\n", count, count == 1 ? "" : "s");
        return 1;
    }

    if (strcasecmp(argv[1], "show") == 0) {
        const char *path = NULL;
        const symbol_text_map_entry_t *entries = NULL;
        int count = 0;
        int matched = 0;
        if (argc < 3 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(&path)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!symbol_text_map_getEntries(path, &entries, &count)) {
            debug_error("symbols: failed to read '%s'", path);
            return 0;
        }
        for (int i = 0; i < count; ++i) {
            if (strcmp(entries[i].name, argv[2]) != 0) {
                continue;
            }
            matched += console_cmd_symbols_printEntry(&entries[i]);
        }
        if (matched <= 0) {
            debug_error("symbols: symbol '%s' not found", argv[2]);
            return 0;
        }
        return 1;
    }

    if (strcasecmp(argv[1], "lookup") == 0) {
        const char *path = NULL;
        const symbol_text_map_entry_t *entries = NULL;
        uint32_t addr = 0;
        int count = 0;
        int matched = 0;
        if (argc != 3 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(&path)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!console_cmd_parseHex(argv[2], &addr)) {
            debug_error("symbols: invalid address '%s'", argv[2]);
            return 0;
        }
        if (!symbol_text_map_getEntries(path, &entries, &count)) {
            debug_error("symbols: failed to read '%s'", path);
            return 0;
        }
        for (int i = 0; i < count; ++i) {
            if ((entries[i].addr & 0x00ffffffu) != (addr & 0x00ffffffu)) {
                continue;
            }
            matched += console_cmd_symbols_printEntry(&entries[i]);
        }
        if (matched <= 0) {
            debug_error("symbols: no exact match at 0x%X", (unsigned)(addr & 0x00ffffffu));
            return 0;
        }
        return 1;
    }

    if (strcasecmp(argv[1], "add") == 0) {
        uint32_t addr = 0;
        symbol_text_map_symbol_kind_t kind = SYMBOL_TEXT_MAP_SYMBOL_KIND_UNKNOWN;
        if (argc < 5 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(NULL)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!console_cmd_parseHex(argv[3], &addr)) {
            debug_error("symbols: invalid address '%s'", argv[3]);
            return 0;
        }
        if (!console_cmd_symbols_parseKind(argv[4], &kind)) {
            debug_error("symbols: invalid kind '%s' (expected function, variable, or unknown)", argv[4]);
            return 0;
        }
        if (!symbol_text_map_upsert(argv[2], addr, kind)) {
            debug_error("symbols: failed to update '%s'", argv[2]);
            return 0;
        }
        console_cmd_symbols_invalidateCaches();
        debug_printf("symbols: set %s = 0x%X (%s)\n",
                     argv[2],
                     (unsigned)(addr & 0x00ffffffu),
                     console_cmd_symbols_exportSymbolKindName(kind));
        return 1;
    }

    if (strcasecmp(argv[1], "delete") == 0) {
        int removedCount = 0;
        if (argc < 3 || !argv[2] || !argv[2][0]) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(NULL)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!symbol_text_map_delete(argv[2], &removedCount)) {
            debug_error("symbols: symbol '%s' not found", argv[2]);
            return 0;
        }
        console_cmd_symbols_invalidateCaches();
        debug_printf("symbols: removed %d entr%s named %s\n",
                     removedCount,
                     removedCount == 1 ? "y" : "ies",
                     argv[2]);
        return 1;
    }

    if (strcasecmp(argv[1], "reset") == 0) {
        if (argc != 2) {
            console_cmd_symbols_printUsage();
            return 0;
        }
        if (!console_cmd_symbols_getEditablePath(NULL)) {
            debug_error("symbols: no text symbol file loaded");
            return 0;
        }
        if (!symbol_text_map_reset()) {
            debug_error("symbols: failed to reset active symbol table");
            return 0;
        }
        console_cmd_symbols_invalidateCaches();
        debug_printf("symbols: reset active symbol table\n");
        return 1;
    }

    debug_error("symbols: unknown subcommand '%s'", argv[1]);
    return 0;
}
