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
#include "state_buffer.h"

int
console_cmd_loop_command(int argc, char **argv)
{
    if (argc < 2) {
        if (!debugger.loopEnabled) {
            debug_printf("loop: disabled\n");
            return 1;
        }
        debug_printf("loop: enabled from=%llu to=%llu\n",
                     (unsigned long long)debugger.loopFrom,
                     (unsigned long long)debugger.loopTo);
        return 1;
    }

    if (strcasecmp(argv[1], "clear") == 0) {
        debugger.loopEnabled = 0;
        debugger.loopFrom = 0;
        debugger.loopTo = 0;
        debug_printf("loop: cleared\n");
        return 1;
    }

    if (argc < 3) {
        debug_printf("Usage: loop <from> <to>\n");
        return 0;
    }

    uint64_t from = 0;
    uint64_t to = 0;
    if (!console_cmd_parseU64Dec(argv[1], &from)) {
        debug_error("loop: invalid from '%s' (expected decimal integer)", argv[1]);
        return 0;
    }
    if (!console_cmd_parseU64Dec(argv[2], &to)) {
        debug_error("loop: invalid to '%s' (expected decimal integer)", argv[2]);
        return 0;
    }
    if (from >= to) {
        debug_error("loop: expected from < to");
        return 0;
    }

    if (!state_buffer_hasFrameNo(from)) {
        debug_error("loop: from frame %llu not in state buffer", (unsigned long long)from);
        return 0;
    }
    if (!state_buffer_hasFrameNo(to)) {
        debug_error("loop: to frame %llu not in state buffer", (unsigned long long)to);
        return 0;
    }

    debugger.loopEnabled = 1;
    debugger.loopFrom = from;
    debugger.loopTo = to;

    return 1;
}
