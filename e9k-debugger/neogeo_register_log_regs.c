/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdio.h>

#include "neogeo_register_log_regs.h"
#include "e9k-lib.h"

const char *
neogeo_register_log_regs_nameForReg(uint32_t reg, uint8_t sourceKind)
{
    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        switch (reg & 0xffu) {
        case 0x04u:
            return "YM_ADDR_A";
        case 0x05u:
            return "YM_DATA_A";
        case 0x06u:
            return "YM_ADDR_B";
        case 0x07u:
            return "YM_DATA_B";
        default:
            return "Z80_PORT";
        }
    }

    switch (reg & 0x00ffffffu) {
    case 0x320000u:
        return "REG_SOUND";
    case 0x3c0000u:
        return "REG_VRAMADDR";
    case 0x3c0002u:
        return "REG_VRAMRW";
    case 0x3c0004u:
        return "REG_VRAMMOD";
    case 0x3c0006u:
        return "REG_LSPCMODE";
    case 0x3c0008u:
        return "REG_TIMERHIGH";
    case 0x3c000au:
        return "REG_TIMERLOW";
    case 0x3c000cu:
        return "REG_IRQACK";
    case 0x3c000eu:
        return "REG_TIMERSTOP";
    default:
        return "REG_UNKNOWN";
    }
}

const char *
neogeo_register_log_regs_descriptionForReg(uint32_t reg, uint8_t sourceKind)
{
    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        switch (reg & 0xffu) {
        case 0x04u:
            return "YM2610 port A address";
        case 0x05u:
            return "YM2610 port A data";
        case 0x06u:
            return "YM2610 port B address";
        case 0x07u:
            return "YM2610 port B data";
        default:
            return "Z80 port write";
        }
    }

    switch (reg & 0x00ffffffu) {
    case 0x320000u:
        return "68K sound command latch";
    case 0x3c0000u:
        return "LSPC VRAM address";
    case 0x3c0002u:
        return "LSPC VRAM data";
    case 0x3c0004u:
        return "LSPC VRAM auto-increment";
    case 0x3c0006u:
        return "LSPC mode / auto-animation";
    case 0x3c0008u:
        return "IRQ2 timer high word";
    case 0x3c000au:
        return "IRQ2 timer low word";
    case 0x3c000cu:
        return "IRQ acknowledge";
    case 0x3c000eu:
        return "IRQ2 timer stop";
    default:
        return "Neo Geo register write";
    }
}

uint32_t
neogeo_register_log_regs_colorForReg(uint32_t reg, uint8_t sourceKind)
{
    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        return 0x78d38cu;
    }

    switch (reg & 0x00ffffffu) {
    case 0x320000u:
        return 0xd4a55fu;
    case 0x3c0008u:
    case 0x3c000au:
    case 0x3c000cu:
    case 0x3c000eu:
        return 0xe1bf63u;
    default:
        return 0x6cb8ffu;
    }
}

const char *
neogeo_register_log_regs_valueTooltipForReg(uint32_t reg, uint8_t sourceKind, uint16_t value)
{
    static char tooltip[128];

    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        switch (reg & 0xffu) {
        case 0x04u:
        case 0x06u:
            snprintf(tooltip, sizeof(tooltip), "YM register select: 0x%02X", (unsigned)(value & 0xffu));
            return tooltip;
        case 0x05u:
        case 0x07u:
            snprintf(tooltip, sizeof(tooltip), "YM data write: 0x%02X", (unsigned)(value & 0xffu));
            return tooltip;
        default:
            snprintf(tooltip, sizeof(tooltip), "Z80 port write: 0x%02X", (unsigned)(value & 0xffu));
            return tooltip;
        }
    }

    switch (reg & 0x00ffffffu) {
    case 0x320000u:
        snprintf(tooltip, sizeof(tooltip), "Sound command: 0x%02X", (unsigned)(value & 0xffu));
        return tooltip;
    case 0x3c0000u:
        snprintf(tooltip, sizeof(tooltip), "Next VRAM address: 0x%04X", (unsigned)value);
        return tooltip;
    case 0x3c0002u:
        snprintf(tooltip, sizeof(tooltip), "VRAM data: 0x%04X", (unsigned)value);
        return tooltip;
    case 0x3c0004u:
        snprintf(tooltip, sizeof(tooltip), "VRAM auto-increment: %d (0x%04X)", (int16_t)value, (unsigned)value);
        return tooltip;
    case 0x3c0006u:
        snprintf(tooltip, sizeof(tooltip),
                 "AA reload=%u, AA disable=%u",
                 (unsigned)((value >> 8) & 0xffu),
                 (unsigned)((value >> 3) & 1u));
        return tooltip;
    case 0x3c0008u:
        snprintf(tooltip, sizeof(tooltip), "Timer reload high word: 0x%04X", (unsigned)value);
        return tooltip;
    case 0x3c000au:
        snprintf(tooltip, sizeof(tooltip), "Timer reload low word: 0x%04X", (unsigned)value);
        return tooltip;
    case 0x3c000cu:
        snprintf(tooltip, sizeof(tooltip),
                 "IRQ ack bits: reset=%u timer=%u vblank=%u",
                 (unsigned)(value & 1u),
                 (unsigned)((value >> 1) & 1u),
                 (unsigned)((value >> 2) & 1u));
        return tooltip;
    case 0x3c000eu:
        snprintf(tooltip, sizeof(tooltip), "Timer stop bits: 0x%04X", (unsigned)value);
        return tooltip;
    default:
        snprintf(tooltip, sizeof(tooltip), "Value: 0x%04X", (unsigned)value);
        return tooltip;
    }
}

const char *
neogeo_register_log_regs_sourceLabel(uint8_t sourceKind)
{
    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        return "Z80";
    }
    return "68K";
}

int
neogeo_register_log_regs_sourceCanBreakpoint(uint8_t sourceKind)
{
    return sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_68K ? 1 : 0;
}

void
neogeo_register_log_regs_formatRegAddress(uint32_t reg, uint8_t sourceKind, char *out, size_t cap)
{
    if (!out || cap == 0) {
        return;
    }
    if (sourceKind == E9K_DEBUG_GEO_REGISTER_LOG_SOURCE_Z80) {
        snprintf(out, cap, "Port: 0x%02X", (unsigned)(reg & 0xffu));
        return;
    }
    snprintf(out, cap, "Address: 0x%06X", (unsigned)(reg & 0x00ffffffu));
}
