/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "debugger.h"
#include "libretro_host.h"
#include "print_eval.h"
#include "target.h"

static uint32_t
console_cmd_watch_sourceMaskForTarget(void);

static int
console_cmd_watch_parseSource(const char *src, uint32_t *outSource);

static const char *
console_cmd_watch_sourceList(void);

static int
console_cmd_watch_list(void);

static void
console_cmd_watch_appendText(char *dst, size_t dstSize, const char *src)
{
    if (!dst || dstSize == 0 || !src) {
        return;
    }
    size_t used = strlen(dst);
    if (used >= dstSize - 1) {
        return;
    }
    size_t remaining = dstSize - used - 1;
    size_t srcLen = strlen(src);
    if (srcLen > remaining) {
        srcLen = remaining;
    }
    memcpy(dst + used, src, srcLen);
    dst[used + srcLen] = '\0';
}

const char *
console_cmd_watch_usage(void)
{
    static char usage[256];
    const char *base = "watch [addr|symbol] [r|w|rw] [size=8|16|32] [mask=0x...|$...] [val=0x...|$...] [old=0x...|$...] [diff=0x...|$...]";
    const char *sources = console_cmd_watch_sourceList();

    usage[0] = '\0';
    console_cmd_watch_appendText(usage, sizeof(usage), base);
    if (sources && *sources) {
        console_cmd_watch_appendText(usage, sizeof(usage), " [src=");
        console_cmd_watch_appendText(usage, sizeof(usage), sources);
        console_cmd_watch_appendText(usage, sizeof(usage), "]\nwatch del <idx>\nwatch clear");
    } else {
        console_cmd_watch_appendText(usage, sizeof(usage), "\nwatch del <idx>\nwatch clear");
    }
    return usage;
}

static uint32_t
console_cmd_watch_sourceMaskForTarget(void)
{
    if (target == target_amiga()) {
        return (1u << E9K_WATCH_ACCESS_SOURCE_CPU) |
               (1u << E9K_WATCH_ACCESS_SOURCE_BLITTER) |
               (1u << E9K_WATCH_ACCESS_SOURCE_COPPER) |
               (1u << E9K_WATCH_ACCESS_SOURCE_DISK);
    }
    if (target == target_neogeo()) {
        return (1u << E9K_WATCH_ACCESS_SOURCE_CPU);
    }
    return 0u;
}

static int
console_cmd_watch_parseSource(const char *src, uint32_t *outSource)
{
    uint32_t source = 0u;
    uint32_t supportedMask = console_cmd_watch_sourceMaskForTarget();

    if (!src || !*src || !outSource) {
        return 0;
    }

    if (strcasecmp(src, "cpu") == 0) {
        source = E9K_WATCH_ACCESS_SOURCE_CPU;
    } else if (strcasecmp(src, "blitter") == 0) {
        source = E9K_WATCH_ACCESS_SOURCE_BLITTER;
    } else if (strcasecmp(src, "copper") == 0) {
        source = E9K_WATCH_ACCESS_SOURCE_COPPER;
    } else if (strcasecmp(src, "disk") == 0 || strcasecmp(src, "floppy") == 0) {
        source = E9K_WATCH_ACCESS_SOURCE_DISK;
    } else {
        return 0;
    }

    if ((supportedMask & (1u << source)) == 0u) {
        return 0;
    }

    *outSource = source;
    return 1;
}

static const char *
console_cmd_watch_sourceList(void)
{
    static char list[128];
    uint32_t mask = console_cmd_watch_sourceMaskForTarget();

    list[0] = '\0';

    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_CPU)) {
        console_cmd_watch_appendText(list, sizeof(list), "cpu|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_BLITTER)) {
        console_cmd_watch_appendText(list, sizeof(list), "blitter|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_COPPER)) {
        console_cmd_watch_appendText(list, sizeof(list), "copper|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_DISK)) {
        console_cmd_watch_appendText(list, sizeof(list), "disk|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_AUDIO)) {
        console_cmd_watch_appendText(list, sizeof(list), "audio|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_VIDEO)) {
        console_cmd_watch_appendText(list, sizeof(list), "video|");
    }
    if (mask & (1u << E9K_WATCH_ACCESS_SOURCE_PERIPHERAL)) {
        console_cmd_watch_appendText(list, sizeof(list), "peripheral|");
    }

    {
        size_t len = strlen(list);
        if (len > 0 && list[len - 1] == '|') {
            list[len - 1] = '\0';
        }
    }

    return list;
}

static int
console_cmd_watch_list(void)
{
    e9k_debug_watchpoint_t wps[E9K_WATCHPOINT_COUNT];
    memset(wps, 0, sizeof(wps));
    size_t count = 0;
    if (!libretro_host_debugReadWatchpoints(wps, E9K_WATCHPOINT_COUNT, &count)) {
        debug_error("watch: libretro core does not expose watchpoints");
        return 0;
    }
    uint64_t enabled = 0;
    (void)libretro_host_debugGetWatchpointEnabledMask(&enabled);

    const char *watchSource = NULL;

    debug_printf("Watchpoints (enabled=0x%016llX):\n", (unsigned long long)enabled);
    for (size_t i = 0; i < count; ++i) {
        const e9k_debug_watchpoint_t *wp = &wps[i];
        int is_enabled = ((enabled >> i) & 1ull) ? 1 : 0;
        if (!is_enabled && wp->op_mask == 0) {
            continue;
        }

        const char *rw = "";
        if ((wp->op_mask & E9K_WATCH_OP_READ) && (wp->op_mask & E9K_WATCH_OP_WRITE)) {
            rw = "rw";
        } else if (wp->op_mask & E9K_WATCH_OP_READ) {
            rw = "r";
        } else if (wp->op_mask & E9K_WATCH_OP_WRITE) {
            rw = "w";
        }

        debug_printf("  [%02u] %s addr=0x%06X op=0x%08X %s",
                     (unsigned)i, is_enabled ? "on " : "off",
                     (unsigned)(wp->addr & 0x00ffffffu), (unsigned)wp->op_mask, rw);
        if (wp->op_mask & E9K_WATCH_OP_ACCESS_SIZE) {
            debug_printf(" size=%u", (unsigned)wp->size_operand);
        }
        if (wp->op_mask & E9K_WATCH_OP_ADDR_COMPARE_MASK) {
            debug_printf(" mask=0x%08X", (unsigned)wp->addr_mask_operand);
        }
        if (wp->op_mask & E9K_WATCH_OP_VALUE_EQ) {
            debug_printf(" val=0x%08X", (unsigned)wp->value_operand);
        }
        if (wp->op_mask & E9K_WATCH_OP_OLD_VALUE_EQ) {
            debug_printf(" old=0x%08X", (unsigned)wp->old_value_operand);
        }
        if (wp->op_mask & E9K_WATCH_OP_VALUE_NEQ_OLD) {
            debug_printf(" diff=0x%08X", (unsigned)wp->diff_operand);
        }
        if (wp->op_mask & E9K_WATCH_OP_ACCESS_SOURCE) {
            switch (wp->access_source_operand) {
            case E9K_WATCH_ACCESS_SOURCE_CPU:
                watchSource = "cpu";
                break;
            case E9K_WATCH_ACCESS_SOURCE_DMA:
                watchSource = "dma";
                break;
            case E9K_WATCH_ACCESS_SOURCE_BLITTER:
                watchSource = "blitter";
                break;
            case E9K_WATCH_ACCESS_SOURCE_COPPER:
                watchSource = "copper";
                break;
            case E9K_WATCH_ACCESS_SOURCE_AUDIO:
                watchSource = "audio";
                break;
            case E9K_WATCH_ACCESS_SOURCE_VIDEO:
                watchSource = "video";
                break;
            case E9K_WATCH_ACCESS_SOURCE_PERIPHERAL:
                watchSource = "peripheral";
                break;
            case E9K_WATCH_ACCESS_SOURCE_DISK:
                watchSource = "disk";
                break;
            default:
                watchSource = "unknown";
                break;
            }
            debug_printf(" src=%s", watchSource);
        }
        debug_printf("\n");
    }

    return 1;
}

int
console_cmd_watch_command(int argc, char **argv)
{
    if (argc < 2) {
        return console_cmd_watch_list();
    }

    if (strcasecmp(argv[1], "clear") == 0) {
        if (!libretro_host_debugResetWatchpoints()) {
            debug_error("watch: libretro core does not expose watchpoints");
            return 0;
        }
        debug_printf("watch: cleared\n");
        return 1;
    }

    if (strcasecmp(argv[1], "del") == 0 || strcasecmp(argv[1], "rm") == 0 || strcasecmp(argv[1], "remove") == 0) {
        if (argc < 3) {
            debug_printf("Usage: watch del <idx>\n");
            return 0;
        }
        char *end = NULL;
        unsigned long idx = strtoul(argv[2], &end, 0);
        if (!end || *end != '\0') {
            debug_error("watch: invalid index '%s'", argv[2]);
            return 0;
        }
        if (!libretro_host_debugRemoveWatchpoint((uint32_t)idx)) {
            debug_error("watch: remove failed (unsupported?)");
            return 0;
        }
        debug_printf("watch: removed %lu\n", idx);
        return 1;
    }

    print_resolved_address_t resolvedAddress;
    memset(&resolvedAddress, 0, sizeof(resolvedAddress));
    if (!console_cmd_parseHex(argv[1], &resolvedAddress.address)) {
        if (!print_eval_resolveAddressInfo(argv[1], &resolvedAddress)) {
            debug_error("watch: expected address or symbol, got '%s'", argv[1]);
            return 0;
        }
        if (resolvedAddress.hasProcessorMemory) {
            debug_error("watch: processor-memory symbols are not supported by watchpoints");
            return 0;
        }
    }
    uint32_t addr = resolvedAddress.address & 0x00ffffffu;

    uint32_t op_mask = 0;
    uint32_t diff_operand = 0;
    uint32_t value_operand = 0;
    uint32_t old_value_operand = 0;
    uint32_t size_operand = 0;
    uint32_t addr_mask_operand = 0;
    uint32_t access_source_operand = 0;
    int have_rw = 0;

    for (int i = 2; i < argc; ++i) {
        const char *tok = argv[i];
        if (!tok || !*tok) {
            continue;
        }
        if (strcasecmp(tok, "r") == 0 || strcasecmp(tok, "read") == 0) {
            op_mask |= E9K_WATCH_OP_READ;
            have_rw = 1;
            continue;
        }
        if (strcasecmp(tok, "w") == 0 || strcasecmp(tok, "write") == 0) {
            op_mask |= E9K_WATCH_OP_WRITE;
            have_rw = 1;
            continue;
        }
        if (strcasecmp(tok, "rw") == 0 || strcasecmp(tok, "wr") == 0) {
            op_mask |= (E9K_WATCH_OP_READ | E9K_WATCH_OP_WRITE);
            have_rw = 1;
            continue;
        }
        if (strncasecmp(tok, "size=", 5) == 0) {
            int sz = atoi(tok + 5);
            if (sz != 8 && sz != 16 && sz != 32) {
                debug_error("watch: invalid size '%s' (expected 8/16/32)", tok + 5);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_ACCESS_SIZE;
            size_operand = (uint32_t)sz;
            continue;
        }
        if (strncasecmp(tok, "mask=", 5) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 5, &v)) {
                debug_error("watch: invalid mask '%s' (expected 0x... or $...)", tok + 5);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_ADDR_COMPARE_MASK;
            addr_mask_operand = v;
            continue;
        }
        if (strncasecmp(tok, "val=", 4) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 4, &v)) {
                debug_error("watch: invalid val '%s' (expected 0x... or $...)", tok + 4);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_VALUE_EQ;
            value_operand = v;
            continue;
        }
        if (strncasecmp(tok, "value=", 6) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 6, &v)) {
                debug_error("watch: invalid value '%s' (expected 0x... or $...)", tok + 6);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_VALUE_EQ;
            value_operand = v;
            continue;
        }
        if (strncasecmp(tok, "old=", 4) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 4, &v)) {
                debug_error("watch: invalid old '%s' (expected 0x... or $...)", tok + 4);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_OLD_VALUE_EQ;
            old_value_operand = v;
            continue;
        }
        if (strncasecmp(tok, "diff=", 5) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 5, &v)) {
                debug_error("watch: invalid diff '%s' (expected 0x... or $...)", tok + 5);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_VALUE_NEQ_OLD;
            diff_operand = v;
            continue;
        }
        if (strncasecmp(tok, "src=", 4) == 0 || strncasecmp(tok, "source=", 7) == 0) {
            const char *src = tok + (tok[1] == 'r' || tok[1] == 'R' ? 4 : 7);
            if (!console_cmd_watch_parseSource(src, &access_source_operand)) {
                const char *validSources = console_cmd_watch_sourceList();
                if (validSources && *validSources) {
                    debug_error("watch: invalid source '%s' (expected %s for current target)", src, validSources);
                } else {
                    debug_error("watch: source filters are not supported for current target");
                }
                return 0;
            }
            op_mask |= E9K_WATCH_OP_ACCESS_SOURCE;
            continue;
        }
        if (strncasecmp(tok, "neq=", 4) == 0) {
            uint32_t v = 0;
            if (!console_cmd_parseU32Strict(tok + 4, &v)) {
                debug_error("watch: invalid neq '%s' (expected 0x... or $...)", tok + 4);
                return 0;
            }
            op_mask |= E9K_WATCH_OP_VALUE_NEQ_OLD;
            diff_operand = v;
            continue;
        }

        debug_error("watch: unknown option '%s'", tok);
        return 0;
    }

    if (!have_rw) {
        op_mask |= (E9K_WATCH_OP_READ | E9K_WATCH_OP_WRITE);
    }

    uint32_t index = 0;
    if (!libretro_host_debugAddWatchpoint(addr, op_mask, diff_operand, value_operand, old_value_operand, size_operand, addr_mask_operand, access_source_operand, &index)) {
        debug_error("watch: failed to add (table full or unsupported)");
        return 0;
    }
    debug_printf("watch: added [%u] at 0x%06X\n", (unsigned)index, (unsigned)addr);
    return 1;
}
