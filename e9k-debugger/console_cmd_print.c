/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "libretro_host.h"
#include "print_eval.h"

static int
console_cmd_print_readMemoryValueFromInfo(const print_resolved_address_t *address, size_t sizeBytes, uint32_t *outValue)
{
    if (outValue) {
        *outValue = 0u;
    }
    if (!address || !outValue || (sizeBytes != 1u && sizeBytes != 2u && sizeBytes != 4u)) {
        return 0;
    }

    uint8_t buf[4] = {0, 0, 0, 0};
    if (address->hasProcessorMemory) {
        if (!libretro_host_debugReadProcessorMemory(address->processorId, address->address, buf, sizeBytes)) {
            return 0;
        }
    } else if (!libretro_host_debugReadMemory(address->address, buf, sizeBytes)) {
        return 0;
    }

    if (sizeBytes == 1u) {
        *outValue = (uint32_t)buf[0];
        return 1;
    }
    if (sizeBytes == 2u) {
        *outValue = ((uint32_t)buf[0] << 8) | (uint32_t)buf[1];
        return 1;
    }

    *outValue = ((uint32_t)buf[0] << 24) |
                ((uint32_t)buf[1] << 16) |
                ((uint32_t)buf[2] << 8) |
                (uint32_t)buf[3];
    return 1;
}

static int
console_cmd_print_readMemoryValueBe(uint32_t addr, size_t sizeBytes, uint32_t *outValue)
{
    print_resolved_address_t address;
    memset(&address, 0, sizeof(address));
    address.address = addr;
    return console_cmd_print_readMemoryValueFromInfo(&address, sizeBytes, outValue);
}

int
console_cmd_print_command(int argc, char **argv)
{
    if (argc < 2) {
        debug_printf("Usage: print <expr> [size=8|16|32]\n");
        debug_printf("       print addr <expr>\n");
        return 0;
    }

    int addrMode = 0;
    int startIndex = 1;
    if (argc >= 3 && strcasecmp(argv[1], "addr") == 0) {
        addrMode = 1;
        startIndex = 2;
    }

    char expr[512];
    expr[0] = '\0';
    size_t used = 0;
    uint32_t sizeBitsOpt = 0u;
    for (int i = startIndex; i < argc; ++i) {
        const char *arg = argv[i];
        if (!arg) {
            continue;
        }
        uint32_t parsedSizeBits = 0u;
        int sizeToken = console_cmd_parseSizeBitsOpt(arg, &parsedSizeBits);
        if (sizeToken < 0) {
            debug_error("print: invalid size '%s' (expected size=8|16|32)", arg);
            return 0;
        }
        if (sizeToken > 0) {
            sizeBitsOpt = parsedSizeBits;
            continue;
        }
        size_t len = strlen(arg);
        if (used > 0 && used < sizeof(expr) - 1) {
            expr[used++] = ' ';
            expr[used] = '\0';
        }
        if (used + len >= sizeof(expr)) {
            len = sizeof(expr) - 1 - used;
        }
        memcpy(expr + used, arg, len);
        used += len;
        expr[used] = '\0';
    }
    if (expr[0] == '\0') {
        if (addrMode) {
            debug_printf("Usage: print addr <expr>\n");
        } else {
            debug_printf("Usage: print <expr> [size=8|16|32]\n");
            debug_printf("       print addr <expr>\n");
        }
        return 0;
    }

    if (addrMode) {
        print_resolved_address_t address;
        if (!print_eval_resolveAddressInfo(expr, &address)) {
            debug_error("print: failed to resolve address '%s'", expr);
            return 0;
        }
        uint32_t bits = sizeBitsOpt;
        if (bits == 0u) {
            bits = (uint32_t)(address.size > 0 ? address.size * 8u : 32u);
        }
        if (address.hasProcessorMemory) {
            debug_printf("%s: processor[%u]:0x%04X (%u bits)\n",
                         expr,
                         (unsigned)address.processorId,
                         (unsigned)(address.address & 0xffffu),
                         (unsigned)bits);
            return 1;
        }
        debug_printf("%s: 0x%06X (%u bits)\n", expr, (unsigned)(address.address & 0x00ffffffu), (unsigned)bits);
        return 1;
    }

    // Fast-path simple numeric expressions so `print *0xADDR` works without an ELF.
    {
        const char *p = expr;
        while (*p && isspace((unsigned char)*p)) {
            ++p;
        }

        int deref = 0;
        if (*p == '*') {
            deref = 1;
            ++p;
        }

        while (*p && isspace((unsigned char)*p)) {
            ++p;
        }

        int paren = 0;
        if (*p == '(') {
            paren = 1;
            ++p;
            while (*p && isspace((unsigned char)*p)) {
                ++p;
            }
        }

        const char *numberStart = p;
        int numberBase = 0;
        if (*numberStart == '$') {
            numberBase = 16;
            ++numberStart;
        }

        errno = 0;
        char *end = NULL;
        unsigned long long number = strtoull(numberStart, &end, numberBase);
        if (errno == 0 && end && end != numberStart) {
            const char *q = end;
            while (*q && isspace((unsigned char)*q)) {
                ++q;
            }
            if (paren) {
                if (*q != ')') {
                    q = NULL;
                } else {
                    ++q;
                    while (*q && isspace((unsigned char)*q)) {
                        ++q;
                    }
                }
            }
            if (q && *q == '\0') {
                if (deref) {
                    uint32_t addr = (uint32_t)number & 0x00ffffffu;
                    size_t sizeBytes = sizeBitsOpt ? (size_t)(sizeBitsOpt / 8u) : 4u;
                    uint32_t val = 0u;
                    if (!console_cmd_print_readMemoryValueBe(addr, sizeBytes, &val)) {
                        debug_error("print: failed to read memory at 0x%06X", (unsigned)addr);
                        return 0;
                    }
                    debug_printf("*0x%06X: %u (0x%X) [%u bits]\n",
                                 (unsigned)addr,
                                 (unsigned)val,
                                 (unsigned)val,
                                 (unsigned)(sizeBytes * 8u));
                    return 1;
                }
                debug_printf("%s: %llu (0x%llX)\n", expr,
                             (unsigned long long)number,
                             (unsigned long long)number);
                return 1;
            }
        }
    }

    if (sizeBitsOpt != 0u) {
        print_resolved_address_t address;
        if (!print_eval_resolveAddressInfo(expr, &address)) {
            debug_error("print: size override requires an address expression");
            return 0;
        }
        size_t sizeBytes = (size_t)(sizeBitsOpt / 8u);
        uint32_t val = 0u;
        if (!console_cmd_print_readMemoryValueFromInfo(&address, sizeBytes, &val)) {
            if (address.hasProcessorMemory) {
                debug_error("print: failed to read processor memory at 0x%04X", (unsigned)(address.address & 0xffffu));
            } else {
                debug_error("print: failed to read memory at 0x%06X", (unsigned)(address.address & 0x00ffffffu));
            }
            return 0;
        }
        debug_printf("%s: %u (0x%X) [%u bits]\n", expr, (unsigned)val, (unsigned)val, (unsigned)sizeBitsOpt);
        return 1;
    }

    return print_eval_print(expr) ? 1 : 0;
}

int
console_cmd_print_complete(const char *prefix, char ***outList, int *outCount)
{
    return print_eval_complete(prefix, outList, outCount);
}
