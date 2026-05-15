/*
 * Z80 disassembly adapter for the Engine 9000 debugger.
 *
 * The opcode data and decoding behaviour are derived from z80dasm 1.1.6:
 * Copyright (C) 1994-2007 Jan Panteltje
 * Copyright (C) 2007-2019 Tomaz Solc
 *
 * z80dasm is licensed under the GNU General Public License version 2 or
 * later.
 */

#include "e9k-z80-dasm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "e9k-z80-dasm-data.inc"

typedef struct e9k_z80_dasm_out
{
    char *data;
    size_t cap;
    size_t len;
} e9k_z80_dasm_out_t;

static void
e9k_z80_dasmAppendChar(e9k_z80_dasm_out_t *out, char value)
{
    if (out->cap > 0 && out->len + 1 < out->cap) {
        out->data[out->len] = value;
    }
    out->len++;
}

static void
e9k_z80_dasmAppend(e9k_z80_dasm_out_t *out, const char *text)
{
    while (*text) {
        e9k_z80_dasmAppendChar(out, *text++);
    }
}

static void
e9k_z80_dasmAppendFormat(e9k_z80_dasm_out_t *out, const char *format, ...)
{
    char tmp[96];
    va_list args;
    int written;

    va_start(args, format);
    written = vsnprintf(tmp, sizeof(tmp), format, args);
    va_end(args);

    if (written < 0) {
        return;
    }
    e9k_z80_dasmAppend(out, tmp);
}

static void
e9k_z80_dasmFinish(e9k_z80_dasm_out_t *out)
{
    if (out->cap == 0) {
        return;
    }
    if (out->len >= out->cap) {
        out->data[out->cap - 1] = '\0';
    } else {
        out->data[out->len] = '\0';
    }
}

static uint16_t
e9k_z80_dasmRead16(const uint8_t *bytes, int offset)
{
    return (uint16_t)((uint16_t)bytes[offset] | ((uint16_t)bytes[offset + 1] << 8));
}

static void
e9k_z80_dasmAppendHex8(e9k_z80_dasm_out_t *out, uint8_t value)
{
    e9k_z80_dasmAppendFormat(out, "0%02xh", value);
}

static void
e9k_z80_dasmAppendHex16(e9k_z80_dasm_out_t *out, uint16_t value)
{
    e9k_z80_dasmAppendFormat(out, "0%04xh", value);
}

static void
e9k_z80_dasmAppendRel(e9k_z80_dasm_out_t *out, uint8_t displacement)
{
    int value = (int)(int8_t)displacement + 2;
    e9k_z80_dasmAppendFormat(out, "$%+d", value);
}

static size_t
e9k_z80_dasmDefb(e9k_z80_dasm_out_t *out, const uint8_t *bytes, size_t count)
{
    e9k_z80_dasmAppend(out, "defb");
    for (size_t i = 0; i < count; ++i) {
        e9k_z80_dasmAppendChar(out, i == 0 ? ' ' : ',');
        e9k_z80_dasmAppendHex8(out, bytes[i]);
    }
    return count;
}

static void
e9k_z80_dasmAppendIndexedAddr(e9k_z80_dasm_out_t *out, const char *indexReg, uint8_t displacement)
{
    if (displacement < 128) {
        e9k_z80_dasmAppendFormat(out, "(%s+0%02xh)", indexReg, displacement);
    } else {
        e9k_z80_dasmAppendFormat(out, "(%s-0%02xh)", indexReg, (uint8_t)(256u - displacement));
    }
}

static size_t
e9k_z80_dasmDisassembleCb(e9k_z80_dasm_out_t *out, const uint8_t *bytes)
{
    uint8_t opcode = bytes[1];

    if (opcode < 0x08) {
        e9k_z80_dasmAppend(out, "rlc ");
    } else if (opcode < 0x10) {
        e9k_z80_dasmAppend(out, "rrc ");
    } else if (opcode < 0x18) {
        e9k_z80_dasmAppend(out, "rl ");
    } else if (opcode < 0x20) {
        e9k_z80_dasmAppend(out, "rr ");
    } else if (opcode < 0x28) {
        e9k_z80_dasmAppend(out, "sla ");
    } else if (opcode < 0x30) {
        e9k_z80_dasmAppend(out, "sra ");
    } else if (opcode < 0x38) {
        e9k_z80_dasmAppend(out, "sli ");
    } else if (opcode < 0x40) {
        e9k_z80_dasmAppend(out, "srl ");
    } else if (opcode < 0x80) {
        e9k_z80_dasmAppendFormat(out, "bit %u,", (unsigned)((opcode - 0x40u) / 8u));
    } else if (opcode < 0xc0) {
        e9k_z80_dasmAppendFormat(out, "res %u,", (unsigned)((opcode - 0x80u) / 8u));
    } else {
        e9k_z80_dasmAppendFormat(out, "set %u,", (unsigned)((opcode - 0xc0u) / 8u));
    }
    e9k_z80_dasmAppend(out, e9k_z80_dasmRarg[opcode & 7u]);
    return 2;
}

static size_t
e9k_z80_dasmDisassembleEd(e9k_z80_dasm_out_t *out, const uint8_t *bytes)
{
    uint8_t opcode = bytes[1];
    const char *text = NULL;
    const char *suffix = NULL;

    if (opcode >= 0x40 && opcode < 0x50 && e9k_z80_dasmEd40[opcode - 0x40][0] != '0') {
        text = e9k_z80_dasmEd40[opcode - 0x40];
    } else if (opcode >= 0x50 && opcode < 0x60 && e9k_z80_dasmEd50[opcode - 0x50][0] != '0') {
        text = e9k_z80_dasmEd50[opcode - 0x50];
    } else if (opcode >= 0x60 && opcode < 0x70 && e9k_z80_dasmEd60[opcode - 0x60][0] != '0') {
        text = e9k_z80_dasmEd60[opcode - 0x60];
    } else if (opcode >= 0x70 && opcode < 0x80 && e9k_z80_dasmEd70[opcode - 0x70][0] != '0') {
        text = e9k_z80_dasmEd70[opcode - 0x70];
    } else if (opcode >= 0xa0 && opcode < 0xb0 && e9k_z80_dasmEda0[opcode - 0xa0][0] != '0') {
        text = e9k_z80_dasmEda0[opcode - 0xa0];
    } else if (opcode >= 0xb0 && opcode < 0xc0 && e9k_z80_dasmEdb0[opcode - 0xb0][0] != '0') {
        text = e9k_z80_dasmEdb0[opcode - 0xb0];
    }

    if (!text) {
        return e9k_z80_dasmDefb(out, bytes, 2);
    }

    switch (opcode) {
        case 0x43:
            suffix = "%s),bc";
            break;
        case 0x53:
            suffix = "%s),de";
            break;
        case 0x73:
            suffix = "%s),sp";
            break;
        case 0x4b:
        case 0x5b:
        case 0x7b:
            suffix = "%s)";
            break;
        default:
            break;
    }

    e9k_z80_dasmAppend(out, text);
    if (suffix) {
        char addr[16];
        snprintf(addr, sizeof(addr), "0%04xh", e9k_z80_dasmRead16(bytes, 2));
        e9k_z80_dasmAppendFormat(out, suffix, addr);
        return 4;
    }
    return 2;
}

static void
e9k_z80_dasmAppendDcbOperation(e9k_z80_dasm_out_t *out, uint8_t opcode)
{
    if (opcode < 0x08) {
        e9k_z80_dasmAppend(out, "rlc ");
    } else if (opcode < 0x10) {
        e9k_z80_dasmAppend(out, "rrc ");
    } else if (opcode < 0x18) {
        e9k_z80_dasmAppend(out, "rl ");
    } else if (opcode < 0x20) {
        e9k_z80_dasmAppend(out, "rr ");
    } else if (opcode < 0x28) {
        e9k_z80_dasmAppend(out, "sla ");
    } else if (opcode < 0x30) {
        e9k_z80_dasmAppend(out, "sra ");
    } else if (opcode < 0x38) {
        e9k_z80_dasmAppend(out, "sli ");
    } else if (opcode < 0x40) {
        e9k_z80_dasmAppend(out, "srl ");
    } else if (opcode < 0x80) {
        e9k_z80_dasmAppendFormat(out, "bit %u,", (unsigned)((opcode - 0x40u) / 8u));
    } else if (opcode < 0xc0) {
        e9k_z80_dasmAppendFormat(out, "res %u,", (unsigned)((opcode - 0x80u) / 8u));
    } else {
        e9k_z80_dasmAppendFormat(out, "set %u,", (unsigned)((opcode - 0xc0u) / 8u));
    }
}

static void
e9k_z80_dasmAppendDcbLoad(e9k_z80_dasm_out_t *out, const char *indexedAddr, uint8_t opcode)
{
    static const char *regs[] = { "b", "c", "d", "e", "h", "l", NULL, "a" };
    const char *reg = regs[opcode & 7u];

    if (reg) {
        e9k_z80_dasmAppendFormat(out, " & ld %s,%s", reg, indexedAddr);
    }
}

static size_t
e9k_z80_dasmDisassembleBase(e9k_z80_dasm_out_t *out, const uint8_t *bytes);

static size_t
e9k_z80_dasmDisassembleDdfd(e9k_z80_dasm_out_t *out, const uint8_t *bytes)
{
    const char *indexReg = bytes[0] == 0xfd ? "iy" : "ix";
    uint8_t opcode = bytes[1];
    const char *template = e9k_z80_dasmDdfd[opcode];
    char indexedAddr[24];

    if (template[0] != '0') {
        e9k_z80_dasmAppendFormat(out, template, indexReg);
        return 2;
    }

    switch (opcode) {
        case 0x09:
            e9k_z80_dasmAppendFormat(out, "add %s,bc", indexReg);
            return 2;
        case 0x19:
            e9k_z80_dasmAppendFormat(out, "add %s,de", indexReg);
            return 2;
        case 0x21:
            e9k_z80_dasmAppendFormat(out, "ld %s,", indexReg);
            e9k_z80_dasmAppendHex16(out, e9k_z80_dasmRead16(bytes, 2));
            return 4;
        case 0x22:
            e9k_z80_dasmAppend(out, "ld (");
            e9k_z80_dasmAppendHex16(out, e9k_z80_dasmRead16(bytes, 2));
            e9k_z80_dasmAppendFormat(out, "),%s", indexReg);
            return 4;
        case 0x23:
            e9k_z80_dasmAppendFormat(out, "inc %s", indexReg);
            return 2;
        case 0x26:
            e9k_z80_dasmAppendFormat(out, "ld %sh,", indexReg);
            e9k_z80_dasmAppendHex8(out, bytes[2]);
            return 3;
        case 0x29:
            e9k_z80_dasmAppendFormat(out, "add %s,%s", indexReg, indexReg);
            return 2;
        case 0x2a:
            e9k_z80_dasmAppendFormat(out, "ld %s,(", indexReg);
            e9k_z80_dasmAppendHex16(out, e9k_z80_dasmRead16(bytes, 2));
            e9k_z80_dasmAppend(out, ")");
            return 4;
        case 0x2b:
            e9k_z80_dasmAppendFormat(out, "dec %s", indexReg);
            return 2;
        case 0x2e:
            e9k_z80_dasmAppendFormat(out, "ld %sl,", indexReg);
            e9k_z80_dasmAppendHex8(out, bytes[2]);
            return 3;
        case 0x39:
            e9k_z80_dasmAppendFormat(out, "add %s,sp", indexReg);
            return 2;
        case 0x64:
            e9k_z80_dasmAppendFormat(out, "ld %sh,%sh", indexReg, indexReg);
            return 2;
        case 0x65:
            e9k_z80_dasmAppendFormat(out, "ld %sh,%sl", indexReg, indexReg);
            return 2;
        case 0x6c:
            e9k_z80_dasmAppendFormat(out, "ld %sl,%sh", indexReg, indexReg);
            return 2;
        case 0x6d:
            e9k_z80_dasmAppendFormat(out, "ld %sl,%sl", indexReg, indexReg);
            return 2;
        case 0xe1:
            e9k_z80_dasmAppendFormat(out, "pop %s", indexReg);
            return 2;
        case 0xe3:
            e9k_z80_dasmAppendFormat(out, "ex (sp),%s", indexReg);
            return 2;
        case 0xe5:
            e9k_z80_dasmAppendFormat(out, "push %s", indexReg);
            return 2;
        case 0xe9:
            e9k_z80_dasmAppendFormat(out, "jp (%s)", indexReg);
            return 2;
        case 0xf9:
            e9k_z80_dasmAppendFormat(out, "ld sp,%s", indexReg);
            return 2;
        default:
            break;
    }

    e9k_z80_dasm_out_t addrOut = { indexedAddr, sizeof(indexedAddr), 0 };
    e9k_z80_dasmAppendIndexedAddr(&addrOut, indexReg, bytes[2]);
    e9k_z80_dasmFinish(&addrOut);

    if (opcode == 0xcb) {
        e9k_z80_dasmAppendDcbOperation(out, bytes[3]);
        e9k_z80_dasmAppend(out, indexedAddr);
        e9k_z80_dasmAppendDcbLoad(out, indexedAddr, bytes[3]);
        return 4;
    }

    switch (opcode) {
        case 0x34:
            e9k_z80_dasmAppendFormat(out, "inc %s", indexedAddr);
            return 3;
        case 0x35:
            e9k_z80_dasmAppendFormat(out, "dec %s", indexedAddr);
            return 3;
        case 0x36:
            e9k_z80_dasmAppendFormat(out, "ld %s,", indexedAddr);
            e9k_z80_dasmAppendHex8(out, bytes[3]);
            return 4;
        case 0x46:
            e9k_z80_dasmAppendFormat(out, "ld b,%s", indexedAddr);
            return 3;
        case 0x4e:
            e9k_z80_dasmAppendFormat(out, "ld c,%s", indexedAddr);
            return 3;
        case 0x56:
            e9k_z80_dasmAppendFormat(out, "ld d,%s", indexedAddr);
            return 3;
        case 0x5e:
            e9k_z80_dasmAppendFormat(out, "ld e,%s", indexedAddr);
            return 3;
        case 0x66:
            e9k_z80_dasmAppendFormat(out, "ld h,%s", indexedAddr);
            return 3;
        case 0x6e:
            e9k_z80_dasmAppendFormat(out, "ld l,%s", indexedAddr);
            return 3;
        case 0x70:
            e9k_z80_dasmAppendFormat(out, "ld %s,b", indexedAddr);
            return 3;
        case 0x71:
            e9k_z80_dasmAppendFormat(out, "ld %s,c", indexedAddr);
            return 3;
        case 0x72:
            e9k_z80_dasmAppendFormat(out, "ld %s,d", indexedAddr);
            return 3;
        case 0x73:
            e9k_z80_dasmAppendFormat(out, "ld %s,e", indexedAddr);
            return 3;
        case 0x74:
            e9k_z80_dasmAppendFormat(out, "ld %s,h", indexedAddr);
            return 3;
        case 0x75:
            e9k_z80_dasmAppendFormat(out, "ld %s,l", indexedAddr);
            return 3;
        case 0x77:
            e9k_z80_dasmAppendFormat(out, "ld %s,a", indexedAddr);
            return 3;
        case 0x7e:
            e9k_z80_dasmAppendFormat(out, "ld a,%s", indexedAddr);
            return 3;
        case 0x86:
            e9k_z80_dasmAppendFormat(out, "add a,%s", indexedAddr);
            return 3;
        case 0x8e:
            e9k_z80_dasmAppendFormat(out, "adc a,%s", indexedAddr);
            return 3;
        case 0x96:
            e9k_z80_dasmAppendFormat(out, "sub %s", indexedAddr);
            return 3;
        case 0x9e:
            e9k_z80_dasmAppendFormat(out, "sbc a,%s", indexedAddr);
            return 3;
        case 0xa6:
            e9k_z80_dasmAppendFormat(out, "and %s", indexedAddr);
            return 3;
        case 0xae:
            e9k_z80_dasmAppendFormat(out, "xor %s", indexedAddr);
            return 3;
        case 0xb6:
            e9k_z80_dasmAppendFormat(out, "or %s", indexedAddr);
            return 3;
        case 0xbe:
            e9k_z80_dasmAppendFormat(out, "cp %s", indexedAddr);
            return 3;
        default:
            return 1 + e9k_z80_dasmDisassembleBase(out, bytes + 1);
    }
}

static size_t
e9k_z80_dasmDisassembleBase(e9k_z80_dasm_out_t *out, const uint8_t *bytes)
{
    const e9k_z80_dasm_comm_t *comm = &e9k_z80_dasmComtab[bytes[0]];

    switch (comm->type) {
        case 0:
            e9k_z80_dasmAppend(out, comm->com1);
            return (size_t)comm->bytes;
        case 1:
            e9k_z80_dasmAppend(out, comm->com1);
            e9k_z80_dasmAppendHex8(out, bytes[1]);
            return (size_t)comm->bytes;
        case 2:
            e9k_z80_dasmAppend(out, comm->com1);
            e9k_z80_dasmAppendHex16(out, e9k_z80_dasmRead16(bytes, 1));
            return (size_t)comm->bytes;
        case 3:
            e9k_z80_dasmAppend(out, comm->com1);
            if (comm->bytes == 2) {
                e9k_z80_dasmAppendRel(out, bytes[1]);
            } else {
                e9k_z80_dasmAppendHex8(out, bytes[1]);
            }
            return (size_t)comm->bytes;
        case 11:
            e9k_z80_dasmAppend(out, comm->com1);
            e9k_z80_dasmAppendHex8(out, bytes[1]);
            e9k_z80_dasmAppend(out, comm->com2);
            return (size_t)comm->bytes;
        case 12:
            e9k_z80_dasmAppend(out, comm->com1);
            e9k_z80_dasmAppendHex16(out, e9k_z80_dasmRead16(bytes, 1));
            e9k_z80_dasmAppend(out, comm->com2);
            return (size_t)comm->bytes;
        default:
            return e9k_z80_dasmDefb(out, bytes, 1);
    }
}

size_t
e9k_z80_dasmDisassemble(const uint8_t *bytes, uint32_t pc, char *out, size_t cap)
{
    e9k_z80_dasm_out_t outState = { out, cap, 0 };
    size_t used;

    (void)pc;

    if (!bytes || !out || cap == 0) {
        return 0;
    }

    switch (bytes[0]) {
        case 0xcb:
            used = e9k_z80_dasmDisassembleCb(&outState, bytes);
            break;
        case 0xed:
            used = e9k_z80_dasmDisassembleEd(&outState, bytes);
            break;
        case 0xdd:
        case 0xfd:
            used = e9k_z80_dasmDisassembleDdfd(&outState, bytes);
            break;
        default:
            used = e9k_z80_dasmDisassembleBase(&outState, bytes);
            break;
    }

    e9k_z80_dasmFinish(&outState);
    return used;
}
