/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <stdint.h>
#include <stddef.h>

#include "debugger.h"
#include "libretro_host.h"
#include "print_eval.h"

static size_t
console_cmd_write_sizeFromHexDigits(int digits)
{
    if (digits <= 0) {
        return 0;
    }
    if (digits <= 2) {
        return 1;
    }
    if (digits <= 4) {
        return 2;
    }
    if (digits <= 8) {
        return 4;
    }
    return 0;
}

int
console_cmd_write_command(int argc, char **argv)
{
    if (argc < 3) {
        debug_printf("Usage: write <dest> <value>\n");
        return 0;
    }
    const char *dest = argv[1];
    const char *valueStr = argv[2];
    uint64_t value = 0;
    int valueDigits = 0;
    if (!console_cmd_parseHexStrict(valueStr, &value, &valueDigits)) {
        debug_error("write: value must be hex (0x... or $...)");
        return 0;
    }
    size_t valueSize = console_cmd_write_sizeFromHexDigits(valueDigits);
    if (valueSize == 0) {
        debug_error("write: value too wide (max 32-bit hex)");
        return 0;
    }
    if (dest && ((dest[0] == '$') || (dest[0] == '0' && (dest[1] == 'x' || dest[1] == 'X')))) {
        uint64_t addr64 = 0;
        int addrDigits = 0;
        if (!console_cmd_parseHexStrict(dest, &addr64, &addrDigits)) {
            debug_error("write: address must be hex (0x... or $...)");
            return 0;
        }
        if (addr64 > 0xffffffffu) {
            debug_error("write: address out of range");
            return 0;
        }
        uint32_t addr = (uint32_t)addr64;
        if (!libretro_host_debugWriteMemory(addr, (uint32_t)value, valueSize)) {
            debug_error("write: failed to write 0x%llX to 0x%08X",
                        (unsigned long long)value, addr);
            return 0;
        }
        debug_printf("%s = 0x%llX (%zu bits)\n", dest, (unsigned long long)value, valueSize * 8);
        return 1;
    }
    print_resolved_address_t symAddress;
    if (!print_eval_resolveAddressInfo(dest, &symAddress)) {
        debug_error("write: unknown symbol '%s'", dest);
        return 0;
    }
    uint32_t symAddr = symAddress.address;
    size_t symSize = symAddress.size;
    if (symSize > 4) {
        debug_error("write: can't write to %s (size %zu); use \"write 0x%08X %s\" to write the address directly",
                    dest, symSize, symAddr, valueStr);
        return 0;
    }
    if (valueDigits > (int)(symSize * 2)) {
        debug_error("write: value too large for %s (%zu bytes)", dest, symSize);
        return 0;
    }
    int wrote = symAddress.hasProcessorMemory
        ? libretro_host_debugWriteProcessorMemory(symAddress.processorId, symAddr, (uint32_t)value, symSize)
        : libretro_host_debugWriteMemory(symAddr, (uint32_t)value, symSize);
    if (!wrote) {
        debug_error("write: failed to write 0x%llX to %s", (unsigned long long)value, dest);
        return 0;
    }
    debug_printf("%s = 0x%llX (%zu bits)\n", dest, (unsigned long long)value, symSize * 8);
    return 1;
}

int
console_cmd_write_complete(const char *prefix, char ***outList, int *outCount)
{
    if (!prefix || !*prefix) {
        return 0;
    }
    if (prefix[0] == '0' && (prefix[1] == 'x' || prefix[1] == 'X')) {
        return 0;
    }
    return print_eval_complete(prefix, outList, outCount);
}

