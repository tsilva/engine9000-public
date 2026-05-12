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

#include "base_map.h"
#include "debugger.h"

int
console_cmd_base_command(int argc, char **argv)
{
    uint32_t textBase = 0;
    uint32_t dataBase = 0;
    uint32_t bssBase = 0;
    base_map_getBasicBases(&textBase, &dataBase, &bssBase);
    if (argc < 2) {
        debug_printf("base: text=0x%08X data=0x%08X bss=0x%08X\n",
                     (unsigned)textBase,
                     (unsigned)dataBase,
                     (unsigned)bssBase);
        return 1;
    }
    base_map_section_t section = BASE_MAP_SECTION_TEXT;
    const char *sectionName = "text";
    int argIndex = 1;

    if (strcasecmp(argv[1], "text") == 0) {
        section = BASE_MAP_SECTION_TEXT;
        sectionName = "text";
        argIndex = 2;
    } else if (strcasecmp(argv[1], "data") == 0) {
        section = BASE_MAP_SECTION_DATA;
        sectionName = "data";
        argIndex = 2;
    } else if (strcasecmp(argv[1], "bss") == 0) {
        section = BASE_MAP_SECTION_BSS;
        sectionName = "bss";
        argIndex = 2;
    } else if (strcasecmp(argv[1], "clear") == 0) {
        debugger_setTextBaseAddress(0);
        debugger_setDataBaseAddress(0);
        debugger_setBssBaseAddress(0);
        debug_printf("base: cleared\n");
        return 1;
    }

    if (argc <= argIndex) {
        debug_printf("base: %s=0x%08X\n", sectionName, (unsigned)base_map_getBasicBase(section));
        return 1;
    }
    if (strcasecmp(argv[argIndex], "clear") == 0) {
        if (section == BASE_MAP_SECTION_TEXT) {
            debugger_setTextBaseAddress(0);
        } else if (section == BASE_MAP_SECTION_DATA) {
            debugger_setDataBaseAddress(0);
        } else {
            debugger_setBssBaseAddress(0);
        }
        debug_printf("base: cleared %s\n", sectionName);
        return 1;
    }
    uint32_t addr = 0;
    if (!console_cmd_parseU32Auto(argv[argIndex], &addr)) {
        debug_error("base: invalid address '%s' (use decimal or 0x... or $...)", argv[argIndex]);
        return 0;
    }
    if (section == BASE_MAP_SECTION_TEXT) {
        debugger_setTextBaseAddress(addr);
    } else if (section == BASE_MAP_SECTION_DATA) {
        debugger_setDataBaseAddress(addr);
    } else {
        debugger_setBssBaseAddress(addr);
    }
    debug_printf("base: set %s to 0x%08X\n", sectionName, (unsigned)base_map_getBasicBase(section));
    return 1;
}

