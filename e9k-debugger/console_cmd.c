/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <errno.h>

#include "console_cmd.h"
#include "console_cmd_internal.h"
#include "debugger.h"
#include "alloc.h"
#include "libretro_host.h"
#include "machine.h"

typedef struct console_cmd_entry {
    const char *name;
    const char *shortcut;
    const char *usage;
    const char *help;
    console_cmd_handler_t handler;
    console_cmd_complete_t complete;
} console_cmd_entry_t;

static int console_cmd_help(int argc, char **argv);

static const console_cmd_entry_t console_cmd_entries[] = {
    { "help",  "h", "help [command]", "Show available commands or detailed help.", console_cmd_help, NULL },
    { "base",  NULL, "base [text|data|bss] [addr|clear]", "Set or show section base addresses (subtracted from addresses passed to toolchain tools).", console_cmd_base_command, NULL },
    { "break", "b", "break <addr|symbol|file:line>", "Set a breakpoint at an address, symbol, or file:line.", console_cmd_break_command, console_cmd_break_complete },    
    { "cls",  NULL, "cls", "Clear the console output.", console_cmd_cls_command, NULL },    
    { "continue", "c", "continue", "Continue execution and defocus the prompt.", console_cmd_continue_command, NULL },
    { "diff", NULL, "diff <fromFrame> <toFrame> [size=8|16|32]", "Show RAM addresses that differ between two recorded frames.", console_cmd_diff_command, NULL },    
    { "loop", NULL, "loop <from> <to>\nloop\nloop clear", "Loop between two recorded frame numbers (decimal).", console_cmd_loop_command, NULL },
    { "print", "p", "print <expr> [size=8|16|32]\nprint addr <expr>", "Print an expression using DWARF + symbol info. Use size=8|16|32 to force a memory read size, or 'print addr <expr>' to show the resolved runtime address.", console_cmd_print_command, console_cmd_print_complete },   
    { "protect", NULL, "protect\nprotect clear\nprotect del <addr> [size=8|16|32]\nprotect <addr> block [size=8|16|32]\nprotect <addr> set=0x...|$... [size=8|16|32]", "Protect addresses by blocking writes or forcing a value (core-side).", console_cmd_protect_command, NULL },    
    { "next", "n", "next", "Step over the next line.", console_cmd_next_command, NULL },    
    { "finish", "f", "finish", "Step out of the current function.", console_cmd_finish_command, NULL },
    { "step", "s", "step", "Step to next source line.", console_cmd_step_command, NULL },
    { "stepi", "i", "stepi", "Step one instruction.", console_cmd_stepi_command, NULL },
    { "symbols", "y", "symbols export <path>\nsymbols load <path>\nsymbols save\nsymbols list\nsymbols show <symbol>\nsymbols lookup <addr>\nsymbols add <symbol> <addr> <function|variable|unknown>\nsymbols delete <symbol>\nsymbols reset", "Export, load, and edit text-map symbols.", console_cmd_symbols_command, NULL },
    { "train", NULL, "train <from> <to> [size=8|16|32]\ntrain ignore\ntrain clear", "Train by breaking on a value transition (from/to accept decimal or 0x... or $...).", console_cmd_train_command, NULL },    
    { "transition", NULL, "transition <slide|explode|doom|flip|rbar|random|cycle|none>", "Set the transition mode for startup and fullscreen.", console_cmd_transition_command, console_cmd_transition_complete },
    { "watch", "wa", NULL, "Set or list watchpoints.", console_cmd_watch_command, NULL },    
    { "write", NULL, "write <dest> <value>", "Write a hex value to an address or symbol.", console_cmd_write_command, console_cmd_write_complete },

};

static const size_t console_cmd_entryCount = sizeof(console_cmd_entries) / sizeof(console_cmd_entries[0]);

typedef struct console_completion {
    char **items;
    int count;
    int cap;
} console_completion_t;

static void
console_cmd_completionAdd(console_completion_t *list, const char *s)
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

static int
console_cmd_tokenize(char *buf, char **argv, int cap)
{
    int argc = 0;
    char *p = buf;
    while (p && *p) {
        while (*p && isspace((unsigned char)*p)) {
            ++p;
        }
        if (!*p) {
            break;
        }
        if (argc < cap) {
            argv[argc++] = p;
        }
        while (*p && !isspace((unsigned char)*p)) {
            ++p;
        }
        if (*p) {
            *p++ = '\0';
        }
    }
    return argc;
}

static const console_cmd_entry_t *
console_cmd_find(const char *name)
{
    if (!name || !*name) {
        return NULL;
    }
    for (size_t i = 0; i < console_cmd_entryCount; ++i) {
        const console_cmd_entry_t *cmd = &console_cmd_entries[i];
        if ((cmd->name && strcasecmp(cmd->name, name) == 0) ||
            (cmd->shortcut && strcasecmp(cmd->shortcut, name) == 0)) {
            return cmd;
        }
    }
    return NULL;
}

int
console_cmd_parseHex(const char *s, uint32_t *out)
{
    if (!s || !*s || !out) {
        return 0;
    }
    const char *p = s;
    if (*p == '$') {
        p += 1;
    } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    if (!*p) {
        return 0;
    }
    for (const char *q = p; *q; ++q) {
        if (!isxdigit((unsigned char)*q)) {
            return 0;
        }
    }
    errno = 0;
    unsigned long v = strtoul(p, NULL, 16);
    if (errno != 0) {
        return 0;
    }
    *out = (uint32_t)(v & 0x00ffffffu);
    return 1;
}

int
console_cmd_parseHexStrict(const char *s, uint64_t *out, int *outDigits)
{
    if (out) {
        *out = 0;
    }
    if (outDigits) {
        *outDigits = 0;
    }
    if (!s || !*s || !out || !outDigits) {
        return 0;
    }
    const char *p = s;
    const char *digitsStart = NULL;
    if (*p == '$') {
        p += 1;
    } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    } else {
        return 0;
    }
    if (!*p) {
        return 0;
    }
    digitsStart = p;
    int digits = 0;
    while (*p) {
        if (!isxdigit((unsigned char)*p)) {
            return 0;
        }
        ++digits;
        ++p;
    }
    errno = 0;
    unsigned long long v = strtoull(digitsStart, NULL, 16);
    if (errno != 0) {
        return 0;
    }
    *out = (uint64_t)v;
    *outDigits = digits;
    return 1;
}

int
console_cmd_parseU32Strict(const char *s, uint32_t *out)
{
    if (out) {
        *out = 0;
    }
    uint64_t v = 0;
    int digits = 0;
    if (!console_cmd_parseHexStrict(s, &v, &digits)) {
        return 0;
    }
    if (v > 0xffffffffULL) {
        return 0;
    }
    if (out) {
        *out = (uint32_t)v;
    }
    return 1;
}

int
console_cmd_parseU32Auto(const char *s, uint32_t *out)
{
    if (out) {
        *out = 0;
    }
    if (!s || !*s || !out) {
        return 0;
    }
    if (s[0] == '-') {
        return 0;
    }
    errno = 0;
    char *end = NULL;
    const char *p = s;
    int base = 0;
    if (*p == '$') {
        p += 1;
        base = 16;
    }
    unsigned long long v = strtoull(p, &end, base);
    if (errno != 0 || !end || end == p || *end != '\0') {
        return 0;
    }
    if (v > 0xffffffffULL) {
        return 0;
    }
    *out = (uint32_t)v;
    return 1;
}

int
console_cmd_parseU64Dec(const char *s, uint64_t *out)
{
    if (out) {
        *out = 0;
    }
    if (!s || !*s || !out) {
        return 0;
    }
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }
    errno = 0;
    unsigned long long v = strtoull(s, NULL, 10);
    if (errno != 0) {
        return 0;
    }
    *out = (uint64_t)v;
    return 1;
}

int
console_cmd_parseSizeBitsOpt(const char *tok, uint32_t *out_size_bits)
{
    if (!tok || !out_size_bits) {
        return 0;
    }
    if (strncasecmp(tok, "size=", 5) != 0) {
        return 0;
    }
    int sz = atoi(tok + 5);
    if (sz != 8 && sz != 16 && sz != 32) {
        return -1;
    }
    *out_size_bits = (uint32_t)sz;
    return 1;
}

static int
console_cmd_help(int argc, char **argv)
{
    if (argc < 2) {
        debug_printf("Commands:\n");
        for (size_t i = 0; i < console_cmd_entryCount; ++i) {
            const console_cmd_entry_t *cmd = &console_cmd_entries[i];
            const char *usage = cmd->usage;
            if (cmd->name && strcmp(cmd->name, "watch") == 0) {
                usage = console_cmd_watch_usage();
            }
            if (cmd->shortcut) {
                debug_printf("  %s (%s)\n", cmd->name, cmd->shortcut, usage);
            } else {
                debug_printf("  %s\n", cmd->name, usage);
            }
        }
        return 1;
    }
    const console_cmd_entry_t *cmd = console_cmd_find(argv[1]);
    if (!cmd) {
        debug_error("help: unknown command '%s'", argv[1]);
        return 0;
    }
    {
        const char *usage = cmd->usage;
        if (cmd->name && strcmp(cmd->name, "watch") == 0) {
            usage = console_cmd_watch_usage();
        }
        debug_printf("\n%s\n\n%s\n", cmd->help, usage ? usage : "");
    }
    return 1;
}

static int
console_cmd_completeCommands(const char *prefix, char ***out_list, int *out_count)
{
    if (out_list) {
        *out_list = NULL;
    }
    if (out_count) {
        *out_count = 0;
    }
    if (!out_list || !out_count) {
        return 0;
    }
    console_completion_t list = {0};
    for (size_t i = 0; i < console_cmd_entryCount; ++i) {
        const console_cmd_entry_t *cmd = &console_cmd_entries[i];
        if (!cmd->name) {
            continue;
        }
        if (prefix && *prefix && strncmp(cmd->name, prefix, strlen(prefix)) != 0) {
            continue;
        }
        console_cmd_completionAdd(&list, cmd->name);
        if (cmd->shortcut) {
            if (!prefix || !*prefix || strncmp(cmd->shortcut, prefix, strlen(prefix)) == 0) {
                console_cmd_completionAdd(&list, cmd->shortcut);
            }
        }
    }
    if (list.count == 0) {
        alloc_free(list.items);
        return 0;
    }
    *out_list = list.items;
    *out_count = list.count;
    return 1;
}

int
console_cmd_complete(const char *line, int cursor, char ***out_list, int *out_count, int *out_prefix_pos)
{
    if (out_list) {
        *out_list = NULL;
    }
    if (out_count) {
        *out_count = 0;
    }
    if (out_prefix_pos) {
        *out_prefix_pos = 0;
    }
    if (!line || !out_list || !out_count || !out_prefix_pos) {
        return 0;
    }
    int len = (int)strlen(line);
    if (cursor < 0) {
        cursor = 0;
    }
    if (cursor > len) {
        cursor = len;
    }
    int token_start = cursor;
    while (token_start > 0 && !isspace((unsigned char)line[token_start - 1])) {
        token_start--;
    }
    int token_len = cursor - token_start;
    int cmd_start = 0;
    while (cmd_start < len && isspace((unsigned char)line[cmd_start])) {
        cmd_start++;
    }
    int cmd_end = cmd_start;
    while (cmd_end < len && !isspace((unsigned char)line[cmd_end])) {
        cmd_end++;
    }
    if (cursor <= cmd_end) {
        *out_prefix_pos = token_start;
        char prefix[256];
        if (token_len >= (int)sizeof(prefix)) {
            token_len = (int)sizeof(prefix) - 1;
        }
        memcpy(prefix, line + token_start, (size_t)token_len);
        prefix[token_len] = '\0';
        return console_cmd_completeCommands(prefix, out_list, out_count);
    }
    char cmd_buf[64];
    int cmd_len = cmd_end - cmd_start;
    if (cmd_len <= 0 || cmd_len >= (int)sizeof(cmd_buf)) {
        return 0;
    }
    memcpy(cmd_buf, line + cmd_start, (size_t)cmd_len);
    cmd_buf[cmd_len] = '\0';
    const console_cmd_entry_t *cmd = console_cmd_find(cmd_buf);
    if (!cmd || !cmd->complete) {
        return 0;
    }
    *out_prefix_pos = token_start;
    char prefix[256];
    if (token_len >= (int)sizeof(prefix)) {
        token_len = (int)sizeof(prefix) - 1;
    }
    memcpy(prefix, line + token_start, (size_t)token_len);
    prefix[token_len] = '\0';
    return cmd->complete(prefix, out_list, out_count);
}

void
console_cmd_freeCompletions(char **list, int count)
{
    if (!list || count <= 0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        alloc_free(list[i]);
    }
    alloc_free(list);
}

void
console_cmd_sendLine(const char *s)
{
    if (!s) {
        return;
    }
    char buf[1024];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *argv[16];
    int argc = console_cmd_tokenize(buf, argv, (int)(sizeof(argv) / sizeof(argv[0])));
    if (argc <= 0) {
        return;
    }
    const console_cmd_entry_t *cmd = console_cmd_find(argv[0]);
    if (!cmd) {
        debug_error("console: unknown command '%s'", argv[0]);
        return;
    }
    cmd->handler(argc, argv);
}

void
console_cmd_sendInterrupt(void)
{
    if (libretro_host_debugPause()) {
        machine_setRunning(&debugger.machine, 0);
        debugger_clearFrameStep();
        return;
    }
    debug_error("console: interrupt failed");
}
