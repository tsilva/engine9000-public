/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

const char *
neogeo_register_log_regs_nameForReg(uint32_t reg, uint8_t sourceKind);

const char *
neogeo_register_log_regs_descriptionForReg(uint32_t reg, uint8_t sourceKind);

uint32_t
neogeo_register_log_regs_colorForReg(uint32_t reg, uint8_t sourceKind);

const char *
neogeo_register_log_regs_valueTooltipForReg(uint32_t reg, uint8_t sourceKind, uint16_t value);

const char *
neogeo_register_log_regs_sourceLabel(uint8_t sourceKind);

int
neogeo_register_log_regs_sourceCanBreakpoint(uint8_t sourceKind);

void
neogeo_register_log_regs_formatRegAddress(uint32_t reg, uint8_t sourceKind, char *out, size_t cap);
