/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <strings.h>
#include <stdint.h>

#include "debugger.h"
#include "protect.h"

int
console_cmd_protect_command(int argc, char **argv)
{
    if (argc < 2) {
        protect_debugList();
        return 1;
    }

    if (strcasecmp(argv[1], "clear") == 0) {
        protect_clear();
        debug_printf("protect: cleared\n");
        return 1;
    }

    if (strcasecmp(argv[1], "del") == 0 || strcasecmp(argv[1], "rm") == 0 || strcasecmp(argv[1], "remove") == 0) {
        if (argc < 3) {
            debug_printf("Usage: protect del <addr> [size=8|16|32]\n");
            return 0;
        }
        uint32_t addr = 0;
        if (!console_cmd_parseHex(argv[2], &addr)) {
            debug_error("protect: invalid address '%s'", argv[2]);
            return 0;
        }
        uint32_t size_bits = 8;
        for (int i = 3; i < argc; ++i) {
            int r = console_cmd_parseSizeBitsOpt(argv[i], &size_bits);
            if (r == 1) {
                continue;
            }
            if (r == -1) {
                debug_error("protect: invalid size '%s'", argv[i]);
                return 0;
            }
            debug_error("protect: unknown option '%s'", argv[i]);
            return 0;
        }
        if (!protect_remove(addr, size_bits)) {
            debug_error("protect: not found");
            return 0;
        }
        debug_printf("protect: removed\n");
        return 1;
    }

    uint32_t addr = 0;
    if (!console_cmd_parseHex(argv[1], &addr)) {
        debug_error("protect: invalid address '%s'", argv[1]);
        return 0;
    }

    uint32_t size_bits = 8;
    int mode_set = 0;
    uint32_t set_value = 0;
    int mode_block = 0;

    for (int i = 2; i < argc; ++i) {
        const char *tok = argv[i];
        if (!tok || !*tok) {
            continue;
        }
        if (strcasecmp(tok, "block") == 0 || strcasecmp(tok, "deny") == 0) {
            mode_block = 1;
            continue;
        }
        if (strncasecmp(tok, "set=", 4) == 0) {
            if (!console_cmd_parseU32Strict(tok + 4, &set_value)) {
                debug_error("protect: invalid set value '%s' (expected 0x... or $...)", tok + 4);
                return 0;
            }
            mode_set = 1;
            continue;
        }
        if (strncasecmp(tok, "value=", 6) == 0) {
            if (!console_cmd_parseU32Strict(tok + 6, &set_value)) {
                debug_error("protect: invalid value '%s' (expected 0x... or $...)", tok + 6);
                return 0;
            }
            mode_set = 1;
            continue;
        }
        int r = console_cmd_parseSizeBitsOpt(tok, &size_bits);
        if (r == 1) {
            continue;
        }
        if (r == -1) {
            debug_error("protect: invalid size '%s'", tok);
            return 0;
        }

        debug_error("protect: unknown option '%s'", tok);
        return 0;
    }

    if (mode_set && mode_block) {
        debug_error("protect: choose either block or set=...");
        return 0;
    }
    if (!mode_set && !mode_block) {
        debug_printf("Usage: protect <addr> block [size=8|16|32]\nprotect <addr> set=0x...|$... [size=8|16|32]\n");
        return 0;
    }

    int ok = 0;
    if (mode_block) {
        ok = protect_addBlock(addr, size_bits);
    } else {
        ok = protect_addSet(addr, set_value, size_bits);
    }
    if (!ok) {
        debug_error("protect: failed (core protect API missing?)");
        return 0;
    }
    debug_printf("protect: added\n");
    return 1;
}
