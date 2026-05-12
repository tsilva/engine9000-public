/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include "debugger.h"
#include "libretro_host.h"
#include "machine.h"

int
console_cmd_next_command(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    debugger_suppressBreakpointAtPC();
    if (libretro_host_debugStepNext()) {
        machine_setRunning(&debugger.machine, 1);
        return 1;
    }
    debug_error("step next: libretro core does not expose debug next");
    return 0;
}

