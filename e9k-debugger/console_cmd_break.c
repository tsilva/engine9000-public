/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "base_map.h"
#include "breakpoints.h"
#include "debugger.h"
#include "file.h"
#include "hunk_fileline_cache.h"
#include "libretro_host.h"
#include "machine.h"
#include "print_eval.h"
#include "symbol_text_map.h"

typedef struct console_cmd_break_completion {
    char **items;
    int count;
    int cap;
} console_cmd_break_completion_t;

static void
console_cmd_break_completionAdd(console_cmd_break_completion_t *list, const char *s)
{
    if (!list || !s || !*s) {
        return;
    }
    if (list->count >= list->cap) {
        int next = list->cap ? list->cap * 2 : 32;
        char **tmp = (char**)alloc_realloc(list->items, (size_t)next * sizeof(char*));
        if (!tmp) {
            return;
        }
        list->items = tmp;
        list->cap = next;
    }
    size_t len = strlen(s);
    char *dup = (char*)alloc_alloc(len + 1);
    if (!dup) {
        return;
    }
    memcpy(dup, s, len + 1);
    list->items[list->count++] = dup;
}

static const char *
console_cmd_break_basename(const char *path)
{
    if (!path) {
        return NULL;
    }
    const char *slash = strrchr(path, '/');
    const char *back = strrchr(path, '\\');
    const char *best = slash > back ? slash : back;
    return best ? best + 1 : path;
}

static int
console_cmd_break_fileMatches(const char *a, const char *b)
{
    if (!a || !b) {
        return 0;
    }
    if (strcmp(a, b) == 0) {
        return 1;
    }
    const char *ba = console_cmd_break_basename(a);
    const char *bb = console_cmd_break_basename(b);
    if (!ba || !bb) {
        return 0;
    }
    return strcmp(ba, bb) == 0;
}

static int
console_cmd_break_symbolMatch(const char *name, const char *symbol)
{
    if (!name || !symbol) {
        return 0;
    }
    if (strcmp(name, symbol) == 0) {
        return 1;
    }
    size_t baseLen = strlen(symbol);
    if (baseLen == 0) {
        return 0;
    }
    if (strncmp(name, symbol, baseLen) == 0) {
        char next = name[baseLen];
        if (next == '.' || next == '$') {
            return 1;
        }
    }
    if (symbol[0] != '_' && name[0] == '_' && strcmp(name + 1, symbol) == 0) {
        return 1;
    }
    return 0;
}

static int
console_cmd_break_parseStabStringName(const char *stabStr, char *outName, size_t cap)
{
    if (!stabStr || !outName || cap == 0) {
        return 0;
    }
    outName[0] = '\0';
    const char *colon = strchr(stabStr, ':');
    if (!colon || colon == stabStr) {
        return 0;
    }
    size_t len = (size_t)(colon - stabStr);
    if (len >= cap) {
        len = cap - 1;
    }
    memcpy(outName, stabStr, len);
    outName[len] = '\0';
    return outName[0] ? 1 : 0;
}

static int
console_cmd_break_resolveSymbolStabsFun(const char *elf, const char *symbol, uint32_t *outAddr)
{
    if (!elf || !*elf || !symbol || !*symbol || !outAddr) {
        return 0;
    }
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        return 0;
    }
    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "-G", elf, 0)) {
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[12];
        int count = 0;
        char *cursor = line;
        while (*cursor && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (!*cursor) {
            continue;
        }
        while (count < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
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
        if (count < 7) {
            continue;
        }
        const char *stabType = tokens[1];
        if (!stabType || strcmp(stabType, "FUN") != 0) {
            continue;
        }
        const char *nValueStr = tokens[4];
        const char *stabStr = tokens[count - 1];
        if (!nValueStr || !*nValueStr || !stabStr || !*stabStr) {
            continue;
        }
        char name[256];
        if (!console_cmd_break_parseStabStringName(stabStr, name, sizeof(name))) {
            continue;
        }
        if (!console_cmd_break_symbolMatch(name, symbol)) {
            continue;
        }
        errno = 0;
        uint32_t nValue = (uint32_t)strtoul(nValueStr, NULL, 16);
        if (errno != 0) {
            continue;
        }
        *outAddr = nValue & 0x00ffffffu;
        pclose(fp);
        return 1;
    }
    pclose(fp);
    return 0;
}

static int
console_cmd_break_resolveSymbol(const char *elf, const char *symbol, uint32_t *outAddr)
{
    if (!elf || !*elf || !symbol || !*symbol || !outAddr) {
        return 0;
    }
    if (debugger.symbolFileKind == DEBUGGER_SYMBOL_FILE_KIND_TEXT_MAP) {
        const symbol_text_map_entry_t *entry = NULL;
        if (!symbol_text_map_findExact(elf,
                                       symbol,
                                       SYMBOL_TEXT_MAP_SYMBOL_MASK_ALL,
                                       &entry) ||
            !entry) {
            return 0;
        }
        *outAddr = entry->addr & 0x00ffffffu;
        return 1;
    }
    char cmd[PATH_MAX * 2];
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        debug_error("break: failed to resolve objdump");
        return 0;
    }
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "--syms", elf, 0)) {
        debug_error("break: failed to build objdump command");
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        debug_error("break: failed to run objdump");
        return 0;
    }
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[8];
        int count = 0;
        char *cursor = line;
        while (count < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
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
        if (count < 2) {
            continue;
        }
        const char *name = tokens[count - 1];
        if (!console_cmd_break_symbolMatch(name, symbol)) {
            continue;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(tokens[0], &addr)) {
            continue;
        }
        if (debugger_toolchainUsesHunkAddr2line() &&
            base_map_getMode() == BASE_MAP_MODE_STACK &&
            count >= 3) {
            uint32_t runtimeAddr = addr;
            if (base_map_symbolToRuntimeHunk(tokens[2], addr, &runtimeAddr)) {
                addr = runtimeAddr;
            }
        }
        *outAddr = addr;
        pclose(fp);
        return 1;
    }
    pclose(fp);
    return console_cmd_break_resolveSymbolStabsFun(elf, symbol, outAddr);
}

static int
console_cmd_break_resolveFileLine(const char *elf, const char *file, int lineNo, uint32_t *outAddr)
{
    if (!elf || !*elf || !file || !*file || lineNo <= 0 || !outAddr) {
        return 0;
    }
    if (debugger_toolchainUsesHunkAddr2line()) {
        return hunk_fileline_cache_resolveFileLine(elf, file, lineNo, outAddr);
    }
    char cmd[PATH_MAX * 2];
    char objdump[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdump, sizeof(objdump), "objdump")) {
        debug_error("break: failed to resolve objdump");
        return 0;
    }
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdump, "-l -d", elf, 0)) {
        debug_error("break: failed to build objdump command");
        return 0;
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        debug_error("break: failed to run objdump");
        return 0;
    }
    char line[1024];
    int wantAddr = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n');
        if (nl) {
            *nl = '\0';
        }
        char *colon = strrchr(line, ':');
        if (colon && isdigit((unsigned char)colon[1])) {
            int ln = atoi(colon + 1);
            if (ln == lineNo) {
                *colon = '\0';
                if (console_cmd_break_fileMatches(line, file)) {
                    wantAddr = 1;
                    continue;
                }
            }
        }
        if (wantAddr) {
            char *p = line;
            while (*p && isspace((unsigned char)*p)) {
                ++p;
            }
            if (isxdigit((unsigned char)*p)) {
                char *end = p;
                while (*end && isxdigit((unsigned char)*end)) {
                    ++end;
                }
                if (*end == ':') {
                    char addrBuf[32];
                    size_t len = (size_t)(end - p);
                    if (len < sizeof(addrBuf)) {
                        memcpy(addrBuf, p, len);
                        addrBuf[len] = '\0';
                        uint32_t addr = 0;
                        if (console_cmd_parseHex(addrBuf, &addr)) {
                            *outAddr = addr;
                            pclose(fp);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    pclose(fp);
    return 0;
}

static int
console_cmd_break_addBreakpoint(uint32_t addr)
{
    machine_breakpoint_t *bp = machine_addBreakpoint(&debugger.machine, addr, 1);
    if (!bp) {
        debug_error("break: failed to add breakpoint");
        return 0;
    }
    breakpoints_resolveLocation(bp);
    libretro_host_debugAddBreakpoint(addr);
    breakpoints_markDirty();
    debug_printf("break: added at 0x%06X\n", addr);
    return 1;
}

int
console_cmd_break_command(int argc, char **argv)
{
    if (argc < 2) {
        debug_printf("Usage: break <addr|symbol|file:line>\n");
        return 0;
    }
    const char *arg = argv[1];
    const char *elf = debugger.libretro.exePath;
    uint32_t addr = 0;
    int ok = 0;
    const char *colon = strrchr(arg, ':');
    if (colon && colon[1]) {
        int lineNo = atoi(colon + 1);
        if (lineNo > 0) {
            size_t len = (size_t)(colon - arg);
            char fileBuf[PATH_MAX];
            if (len < sizeof(fileBuf)) {
                memcpy(fileBuf, arg, len);
                fileBuf[len] = '\0';
                if (!elf || !*elf) {
                    debug_error("break: no debug symbol file configured (set --elf/--hunk/--symbols or Settings)");
                    return 0;
                }
                if (debugger.symbolFileKind == DEBUGGER_SYMBOL_FILE_KIND_TEXT_MAP) {
                    debug_error("break: file:line requires binary debug symbols");
                    return 0;
                }
                ok = console_cmd_break_resolveFileLine(elf, fileBuf, lineNo, &addr);
                if (ok) {
                    if (!(debugger_toolchainUsesHunkAddr2line() &&
                          base_map_getMode() == BASE_MAP_MODE_STACK)) {
                        (void)base_map_debugToRuntime(BASE_MAP_SECTION_TEXT, addr, &addr);
                    }
                }
            }
        }
    }
    if (!ok) {
        ok = console_cmd_parseHex(arg, &addr);
    }
    if (ok) {
        return console_cmd_break_addBreakpoint(addr);
    }
    if (!elf || !*elf) {
        debug_error("break: no debug symbol file configured (set --elf/--hunk/--symbols or Settings)");
        return 0;
    }
    if (!ok) {
        ok = console_cmd_break_resolveSymbol(elf, arg, &addr);
    }
    if (!ok) {
        debug_error("break: failed to resolve '%s'", arg);
        return 0;
    }
    if (debugger.symbolFileKind != DEBUGGER_SYMBOL_FILE_KIND_TEXT_MAP) {
        if (!(debugger_toolchainUsesHunkAddr2line() &&
              base_map_getMode() == BASE_MAP_MODE_STACK)) {
            (void)base_map_debugToRuntime(BASE_MAP_SECTION_TEXT, addr, &addr);
        }
    }
    return console_cmd_break_addBreakpoint(addr);
}

static int
console_cmd_break_completeFromObjdump(const char *prefix,
                                      const char *elf,
                                      const char *objdumpExe,
                                      const char *objdumpArgs,
                                      char ***outList,
                                      int *outCount)
{
    if (!elf || !*elf || !objdumpExe || !*objdumpExe || !objdumpArgs || !*objdumpArgs || !outList || !outCount) {
        return 0;
    }
    const char *matchPrefix = prefix ? prefix : "";
    size_t matchPrefixLen = strlen(matchPrefix);

    char cmd[PATH_MAX * 2];
    if (!debugger_platform_formatToolCommand(cmd, sizeof(cmd), objdumpExe, objdumpArgs, elf, 0)) {
        return 0;
    }

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    console_cmd_break_completion_t list = {0};
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *tokens[8];
        int count = 0;
        char *cursor = line;
        while (count < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
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
        if (count < 2) {
            continue;
        }
        const char *name = tokens[count - 1];
        if (matchPrefixLen > 0 && strncmp(name, matchPrefix, matchPrefixLen) != 0) {
            continue;
        }
        console_cmd_break_completionAdd(&list, name);
    }
    pclose(fp);

    if (list.count == 0) {
        alloc_free(list.items);
        return 0;
    }

    *outList = list.items;
    *outCount = list.count;
    return 1;
}

int
console_cmd_break_complete(const char *prefix, char ***outList, int *outCount)
{
    if (outList) {
        *outList = NULL;
    }
    if (outCount) {
        *outCount = 0;
    }
    if (!outList || !outCount) {
        return 0;
    }
    const char *completionPrefix = prefix ? prefix : "";

    if (print_eval_complete(completionPrefix, outList, outCount)) {
        if (*outCount > 0) {
            return 1;
        }
        print_eval_freeCompletions(*outList, *outCount);
        *outList = NULL;
        *outCount = 0;
    }

    const char *elf = debugger.libretro.exePath;
    if (!elf || !*elf) {
        return 0;
    }

    char objdumpBin[PATH_MAX];
    if (!debugger_toolchainBuildBinary(objdumpBin, sizeof(objdumpBin), "objdump")) {
        return 0;
    }

    char objdumpExe[PATH_MAX];
    if (!file_findInPath(objdumpBin, objdumpExe, sizeof(objdumpExe))) {
        return 0;
    }

    if (console_cmd_break_completeFromObjdump(completionPrefix, elf, objdumpExe, "--syms", outList, outCount)) {
        return 1;
    }

    return console_cmd_break_completeFromObjdump(completionPrefix, elf, objdumpExe, "-t", outList, outCount);
}
