/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "debugger.h"
#include "libretro_host.h"
#include "state_buffer.h"

static int
console_cmd_diff_readBytes(uint64_t frame, uint32_t base, uint8_t *out, size_t size)
{
    if (!state_buffer_restoreFrameNo(frame)) {
        return 0;
    }
    if (!libretro_host_debugReadMemory(base, out, size)) {
        return 0;
    }
    return 1;
}

static uint32_t
console_cmd_diff_readValueBE(const uint8_t *p, size_t size)
{
    if (size == 1) {
        return (uint32_t)p[0];
    }
    if (size == 2) {
        return ((uint32_t)p[0] << 8) | (uint32_t)p[1];
    }
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

int
console_cmd_diff_command(int argc, char **argv)
{
    if (argc < 3) {
        debug_error("diff: usage: diff <fromFrame> <toFrame> [size=8|16|32]");
        return 0;
    }
    uint64_t from = 0;
    uint64_t to = 0;
    if (!console_cmd_parseU64Dec(argv[1], &from)) {
        debug_error("diff: invalid fromFrame (decimal expected)");
        return 0;
    }
    if (!console_cmd_parseU64Dec(argv[2], &to)) {
        debug_error("diff: invalid toFrame (decimal expected)");
        return 0;
    }
    size_t accessSize = 1;
    for (int i = 3; i < argc; ++i) {
        const char *tok = argv[i];
        if (!tok || !*tok) {
            continue;
        }
        if (strncmp(tok, "size=", 5) == 0) {
            uint32_t bits = 0;
            if (!console_cmd_parseU32Auto(tok + 5, &bits)) {
                debug_error("diff: invalid size");
                return 0;
            }
            if (bits == 8) {
                accessSize = 1;
            } else if (bits == 16) {
                accessSize = 2;
            } else if (bits == 32) {
                accessSize = 4;
            } else {
                debug_error("diff: size must be 8, 16, or 32");
                return 0;
            }
            continue;
        }
        debug_error("diff: unknown option '%s'", tok);
        return 0;
    }

    if (!state_buffer_hasFrameNo(from)) {
        debug_error("diff: frame %llu not in state buffer", (unsigned long long)from);
        return 0;
    }
    if (!state_buffer_hasFrameNo(to)) {
        debug_error("diff: frame %llu not in state buffer", (unsigned long long)to);
        return 0;
    }

    const uint64_t restoreFrame = state_buffer_getCurrentFrameNo();

    // Neo E9k main + backup RAM (68k map). Keep it simple; we can extend later.
    struct {
        uint32_t base;
        size_t size;
        const char *name;
    } regions[] = {
        { 0x00100000u, 0x10000u, "ram" },
        { 0x00d00000u, 0x10000u, "backup" }
    };

    enum { kMaxLines = 4096 };
    unsigned long long changed = 0;
    unsigned long long printed = 0;

    debug_printf("diff: frames %llu -> %llu (size=%u)\n",
                 (unsigned long long)from, (unsigned long long)to,
                 (unsigned)(accessSize * 8));

    for (size_t r = 0; r < sizeof(regions) / sizeof(regions[0]); ++r) {
        const uint32_t base = regions[r].base;
        const size_t size = regions[r].size;
        uint8_t *a = (uint8_t*)malloc(size);
        uint8_t *b = (uint8_t*)malloc(size);
        if (!a || !b) {
            free(a);
            free(b);
            debug_error("diff: out of memory");
            (void)state_buffer_restoreFrameNo(restoreFrame);
            debugger.frameCounter = restoreFrame;
            state_buffer_setCurrentFrameNo(restoreFrame);
            return 0;
        }

        if (!console_cmd_diff_readBytes(from, base, a, size) ||
            !console_cmd_diff_readBytes(to, base, b, size)) {
            free(a);
            free(b);
            debug_error("diff: failed to read %s memory", regions[r].name);
            (void)state_buffer_restoreFrameNo(restoreFrame);
            debugger.frameCounter = restoreFrame;
            state_buffer_setCurrentFrameNo(restoreFrame);
            return 0;
        }

        for (size_t off = 0; off + accessSize <= size; off += accessSize) {
            uint32_t va = console_cmd_diff_readValueBE(a + off, accessSize);
            uint32_t vb = console_cmd_diff_readValueBE(b + off, accessSize);
            if (va == vb) {
                continue;
            }
            changed++;
            if (printed < kMaxLines) {
                unsigned digits = (unsigned)(accessSize * 2);
                long long delta = (long long)((long long)vb - (long long)va);
                debug_printf("0x%06X: 0x%0*X -> 0x%0*X  delta=%+lld\n",
                             (unsigned)((base + (uint32_t)off) & 0x00ffffffu),
                             (int)digits, (unsigned)va,
                             (int)digits, (unsigned)vb,
                             delta);
                printed++;
            }
        }

        free(a);
        free(b);
    }

    if (changed == 0) {
        debug_printf("diff: no changes\n");
    } else if (changed > printed) {
        debug_printf("diff: %llu changes (showing %llu, truncated)\n",
                     changed, printed);
    } else {
        debug_printf("diff: %llu changes\n", changed);
    }

    (void)state_buffer_restoreFrameNo(restoreFrame);
    debugger.frameCounter = restoreFrame;
    state_buffer_setCurrentFrameNo(restoreFrame);
    return 1;
}
