/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <stdlib.h>
#include <strings.h>
#include <stdint.h>

#include "debugger.h"
#include "libretro_host.h"
#include "train.h"

int
console_cmd_train_command(int argc, char **argv)
{
    if (argc < 2) {
        debug_printf("Usage: train <from> <to> [size=8|16|32] | train ignore | train clear\n");
        return 0;
    }

    if (strcasecmp(argv[1], "clear") == 0) {
        train_clearIgnoreList();
        debug_printf("train: ignore list cleared\n");
        return 1;
    }

    if (strcasecmp(argv[1], "ignore") == 0) {
        uint32_t addr24 = 0;
        if (!train_getLastWatchbreakAddr(&addr24)) {
            debug_error("train: no watchbreak to ignore yet");
            return 0;
        }
        if (!train_addIgnoreAddr(addr24)) {
            debug_error("train: ignore list full");
            return 0;
        }
        debug_printf("train: ignoring 0x%06X\n", (unsigned)addr24);
        return 1;
    }

    if (argc < 3) {
        debug_printf("Usage: train <from> <to> [size=8|16|32]\n");
        return 0;
    }

    uint32_t from = 0;
    uint32_t to = 0;
    if (!console_cmd_parseU32Auto(argv[1], &from)) {
        debug_error("train: invalid from '%s' (expected decimal or 0x... or $...)", argv[1]);
        return 0;
    }
    if (!console_cmd_parseU32Auto(argv[2], &to)) {
        debug_error("train: invalid to '%s' (expected decimal or 0x... or $...)", argv[2]);
        return 0;
    }

    uint32_t op_mask = 0;
    uint32_t size_operand = 0;
    for (int i = 3; i < argc; ++i) {
        const char *tok = argv[i];
        if (!tok || !*tok) {
            continue;
        }
        if (strncasecmp(tok, "size=", 5) == 0) {
            int sz = atoi(tok + 5);
            if (sz != 8 && sz != 16 && sz != 32) {
                debug_error("train: invalid size '%s' (expected 8/16/32)", tok + 5);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_ACCESS_SIZE;
            size_operand = (uint32_t)sz;
            continue;
        }
        debug_error("train: unknown option '%s'", tok);
        return 0;
    }

    // Any address: enable address compare mask with mask=0 (always matches).
    op_mask |= E9K_WATCH_OP_ADDR_COMPARE_MASK;
    op_mask |= E9K_WATCH_OP_WRITE;
    op_mask |= E9K_WATCH_OP_OLD_VALUE_EQ;
    op_mask |= E9K_WATCH_OP_VALUE_EQ;

    uint32_t index = 0;
    if (!libretro_host_debugAddWatchpoint(0, op_mask, 0, to, from, size_operand, 0, 0, &index)) {
        debug_error("train: failed to add watchpoint (table full or unsupported)");
        return 0;
    }
    train_setWatchIndex(index);

    debug_printf("train: watchpoint [%u] old=0x%08X -> val=0x%08X\n", (unsigned)index, (unsigned)from, (unsigned)to);
    return 1;
}
