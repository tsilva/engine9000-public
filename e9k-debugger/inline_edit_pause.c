/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "inline_edit_pause.h"

#include "debugger.h"
#include "libretro_host.h"

int
inline_edit_pauseBegin(int *outAutoResume)
{
    if (outAutoResume) {
        *outAutoResume = 0;
    }
    if (!machine_getRunning(debugger.machine)) {
        return 1;
    }
    if (!libretro_host_debugPause()) {
        e9ui_showTransientMessage("PAUSE FAILED - NO CORE SUPPORT?");
        return 0;
    }
    machine_setRunning(&debugger.machine, 0);
    debugger_clearFrameStep();
    if (outAutoResume) {
        *outAutoResume = 1;
    }
    return 1;
}

void
inline_edit_pauseEnd(int *autoResume)
{
    if (!autoResume || !*autoResume) {
        return;
    }
    *autoResume = 0;
    if (!libretro_host_debugResume()) {
        e9ui_showTransientMessage("CONTINUE FAILED - NO CORE SUPPORT?");
        return;
    }
    machine_setRunning(&debugger.machine, 1);
}
