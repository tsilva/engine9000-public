/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include "debugger.h"
#include "linebuf.h"

int
console_cmd_cls_command(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    linebuf_clear(&debugger.console);
    debugger.consoleScrollLines = 0;
    return 1;
}

