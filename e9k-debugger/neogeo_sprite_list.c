/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "neogeo_sprite_list.h"
#include "neogeo_sprite_debug.h"
#include "alloc.h"
#include "config.h"
#include "e9ui.h"
#include "e9ui_button.h"
#include "e9ui_checkbox.h"
#include "e9ui_hstack.h"
#include "e9ui_scroll.h"
#include "e9ui_spacer.h"
#include "e9ui_stack.h"
#include "e9ui_text_cache.h"
#include "e9ui_textbox.h"
#include "e9ui_vspacer.h"

#define NEOGEO_SPRITE_LIST_MAX_SPRITES 382
#define NEOGEO_SPRITE_LIST_SCB2_WORD_OFFSET 0x8000u
#define NEOGEO_SPRITE_LIST_SCB3_WORD_OFFSET 0x8200u
#define NEOGEO_SPRITE_LIST_SCB4_WORD_OFFSET 0x8400u
#define NEOGEO_SPRITE_LIST_SCB3_CHAIN_FLAG 0x40u
#define NEOGEO_SPRITE_LIST_SCB3_ROW_MASK 0x3fu
#define NEOGEO_SPRITE_LIST_SCB3_YPOS_MASK 0x01ffu
#define NEOGEO_SPRITE_LIST_SCB3_YPOS_SHIFT 7u
#define NEOGEO_SPRITE_LIST_SCB4_XPOS_SHIFT 7u
#define NEOGEO_SPRITE_LIST_SCB2_VSHRINK_MASK 0x00ffu
#define NEOGEO_SPRITE_LIST_SCB2_HSHRINK_MASK 0x0fu
#define NEOGEO_SPRITE_LIST_SCB2_HSHRINK_SHIFT 8u
#define NEOGEO_SPRITE_LIST_SPRITE_VRAM_WORDS_PER_SPRITE 64u
#define NEOGEO_SPRITE_LIST_SPRITE_TILE_ODD_WORD_OFFSET 1u
#define NEOGEO_SPRITE_LIST_SPRITE_PALETTE_SHIFT 4u
#define NEOGEO_SPRITE_LIST_SPRITE_PALETTE_MASK 0x00ffu
#define NEOGEO_SPRITE_LIST_VISIBLE_W 320
#define NEOGEO_SPRITE_LIST_VISIBLE_H 224
#define NEOGEO_SPRITE_LIST_LINE_OFFSET 16
#define NEOGEO_SPRITE_LIST_WRAP_MASK 0x1ffu
#define NEOGEO_SPRITE_LIST_FILTER_TEXT_MAX 128
#define NEOGEO_SPRITE_LIST_FILTER_COUNT 11
#define NEOGEO_SPRITE_LIST_PAD 10
#define NEOGEO_SPRITE_LIST_GAP 8
#define NEOGEO_SPRITE_LIST_FILTER_GAP_Y 6
#define NEOGEO_SPRITE_LIST_COL_GAP 14
#define NEOGEO_SPRITE_LIST_COL_INDEX_W 54
#define NEOGEO_SPRITE_LIST_COL_ROOT_W 132
#define NEOGEO_SPRITE_LIST_COL_STATUS_W 112
#define NEOGEO_SPRITE_LIST_COL_XPOS_W 64
#define NEOGEO_SPRITE_LIST_COL_YPOS_W 64
#define NEOGEO_SPRITE_LIST_COL_ROWS_W 84
#define NEOGEO_SPRITE_LIST_COL_WIDTH_W 56
#define NEOGEO_SPRITE_LIST_COL_SHRINK_W 98
#define NEOGEO_SPRITE_LIST_COL_PALETTE_W 58
#define NEOGEO_SPRITE_LIST_COL_SCB1_W 116
#define NEOGEO_SPRITE_LIST_COL_SCB_W 220

typedef struct neogeo_sprite_list_row {
    unsigned index;
    unsigned rootIndex;
    unsigned chainLength;
    unsigned chainOffset;
    unsigned xpos;
    unsigned ypos;
    unsigned rows;
    unsigned pixelHeight;
    unsigned width;
    unsigned hshrink;
    unsigned vshrink;
    unsigned paletteBank;
    unsigned scb1Tile;
    unsigned scb1Attr;
    unsigned scb2;
    unsigned scb3;
    unsigned scb4;
    int active;
    int visible;
    int sticky;
    int hasAnimBits;
} neogeo_sprite_list_row_t;

typedef struct neogeo_sprite_list_layout {
    int lineHeight;
    int contentWidth;
    int colIndexX;
    int colIndexW;
    int colRootX;
    int colRootW;
    int colStatusX;
    int colStatusW;
    int colXposX;
    int colXposW;
    int colYposX;
    int colYposW;
    int colRowsX;
    int colRowsW;
    int colWidthX;
    int colWidthW;
    int colShrinkX;
    int colShrinkW;
    int colPaletteX;
    int colPaletteW;
    int colScb1X;
    int colScb1W;
    int colScbX;
    int colScbW;
} neogeo_sprite_list_layout_t;

typedef struct neogeo_sprite_list_display_row {
    char index[16];
    char root[24];
    char status[24];
    char xpos[16];
    char ypos[16];
    char rows[24];
    char width[16];
    char shrink[24];
    char palette[16];
    char scb1[32];
    char scb[64];
} neogeo_sprite_list_display_row_t;

typedef enum neogeo_sprite_list_filter_index {
    neogeo_sprite_list_filter_index = 0,
    neogeo_sprite_list_filter_root = 1,
    neogeo_sprite_list_filter_status = 2,
    neogeo_sprite_list_filter_xpos = 3,
    neogeo_sprite_list_filter_ypos = 4,
    neogeo_sprite_list_filter_rows = 5,
    neogeo_sprite_list_filter_width = 6,
    neogeo_sprite_list_filter_shrink = 7,
    neogeo_sprite_list_filter_palette = 8,
    neogeo_sprite_list_filter_scb1 = 9,
    neogeo_sprite_list_filter_scb = 10
} neogeo_sprite_list_filter_index_t;

typedef struct neogeo_sprite_list_filter_cb {
    int filterIndex;
} neogeo_sprite_list_filter_cb_t;

typedef struct neogeo_sprite_list_overlay_body_state {
    int unused;
} neogeo_sprite_list_overlay_body_state_t;

typedef struct neogeo_sprite_list_canvas_state {
    int unused;
} neogeo_sprite_list_canvas_state_t;

typedef struct neogeo_sprite_list_state {
    e9ui_window_state_t windowState;
    SDL_Window *window;
    SDL_Renderer *renderer;
    e9ui_context_t ctx;
    e9ui_component_t *root;
    e9ui_component_t *filterRoot;
    e9ui_component_t *listScroll;
    e9ui_component_t *listCanvas;
    e9ui_component_t *filterTextboxes[NEOGEO_SPRITE_LIST_FILTER_COUNT];
    e9ui_component_t *activeOnlyCheckbox;
    e9ui_component_t *highlightChainCheckbox;
    neogeo_sprite_list_filter_cb_t filterCbs[NEOGEO_SPRITE_LIST_FILTER_COUNT];
    e9k_debug_sprite_state_t lastState;
    int hasLastState;
    int filterCacheDirty;
    int filteredCount;
    int filteredIndexes[NEOGEO_SPRITE_LIST_MAX_SPRITES];
    neogeo_sprite_list_row_t rows[NEOGEO_SPRITE_LIST_MAX_SPRITES];
    neogeo_sprite_list_display_row_t displayRows[NEOGEO_SPRITE_LIST_MAX_SPRITES];
    TTF_Font *measuredFont;
    int measurementsValid;
    int colIndexW;
    int colRootW;
    int colStatusW;
    int colXposW;
    int colYposW;
    int colRowsW;
    int colWidthW;
    int colShrinkW;
    int colPaletteW;
    int colScb1W;
    int colScbW;
    uint32_t lastSpriteHash;
    int hasLastSpriteHash;
    char filters[NEOGEO_SPRITE_LIST_FILTER_COUNT][NEOGEO_SPRITE_LIST_FILTER_TEXT_MAX];
    int activeOnly;
    int highlightChain;
    int selectedIndex;
} neogeo_sprite_list_state_t;

static neogeo_sprite_list_state_t neogeo_sprite_list_state = {
    .windowState.winX = E9UI_WINDOW_COORD_UNSET,
    .windowState.winY = E9UI_WINDOW_COORD_UNSET,
    .windowState.openMinWidthPx = 760,
    .windowState.openMinHeightPx = 300,
    .windowState.openCenterWhenNoSaved = 1,
    .filterCacheDirty = 1,
    .activeOnly = 1,
    .highlightChain = 1,
    .selectedIndex = -1,
};

static void
neogeo_sprite_list_refreshDisplayMeasurements(neogeo_sprite_list_state_t *ui, TTF_Font *font);

static const char *
neogeo_sprite_list_filterTextForDisplayRow(const neogeo_sprite_list_display_row_t *display, int index);

static const uint8_t neogeo_sprite_list_lutHshrink[0x10][0x10] = {
    { 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
};

static e9ui_window_backend_t
neogeo_sprite_list_windowBackend(void)
{
    return e9ui_window_backend_overlay;
}

static int
neogeo_sprite_list_parseInt(const char *value, int *out)
{
    if (!value || !out) {
        return 0;
    }
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (!end || end == value) {
        return 0;
    }
    if (parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    *out = (int)parsed;
    return 1;
}

static unsigned
neogeo_sprite_list_countShrinkWidth(unsigned hval)
{
    unsigned h = hval & NEOGEO_SPRITE_LIST_SCB2_HSHRINK_MASK;
    unsigned width = 0u;
    for (unsigned p = 0u; p < 16u; ++p) {
        width += (unsigned)neogeo_sprite_list_lutHshrink[h][p];
    }
    return width;
}

static int
neogeo_sprite_list_spriteHasAnimBits(const uint16_t *vram, size_t vramWords, unsigned spriteIndex, unsigned rows)
{
    unsigned maxRows = rows;
    if (maxRows > 32u) {
        maxRows = 32u;
    }
    for (unsigned row = 0u; row < maxRows; ++row) {
        size_t oddWordOffset = (size_t)spriteIndex * NEOGEO_SPRITE_LIST_SPRITE_VRAM_WORDS_PER_SPRITE +
            (size_t)row * 2u + NEOGEO_SPRITE_LIST_SPRITE_TILE_ODD_WORD_OFFSET;
        if (oddWordOffset >= vramWords) {
            break;
        }
        if (vram[oddWordOffset] & 0x000cu) {
            return 1;
        }
    }
    return 0;
}

static int
neogeo_sprite_list_spriteIsVisible(const neogeo_sprite_list_row_t *row)
{
    if (!row || !row->active || row->width == 0u || row->pixelHeight == 0u) {
        return 0;
    }
    int x0 = (int)(row->xpos & NEOGEO_SPRITE_LIST_WRAP_MASK);
    int x1 = x0 + (int)row->width;
    int visibleX = (x0 < NEOGEO_SPRITE_LIST_VISIBLE_W) || (x1 > 512);
    if (!visibleX) {
        return 0;
    }
    for (unsigned r = 0u; r < row->pixelHeight && r < 512u; ++r) {
        unsigned line = (unsigned)((512u - row->ypos + r - NEOGEO_SPRITE_LIST_LINE_OFFSET) & NEOGEO_SPRITE_LIST_WRAP_MASK);
        if (line < NEOGEO_SPRITE_LIST_VISIBLE_H) {
            return 1;
        }
    }
    return 0;
}

static uint32_t
neogeo_sprite_list_hashWord(uint32_t hash, uint16_t word)
{
    hash ^= (uint32_t)(word & 0xffu);
    hash *= 16777619u;
    hash ^= (uint32_t)((word >> 8) & 0xffu);
    hash *= 16777619u;
    return hash;
}

static uint32_t
neogeo_sprite_list_hashState(const e9k_debug_sprite_state_t *st)
{
    if (!st || !st->vram ||
        st->vram_words <= NEOGEO_SPRITE_LIST_SCB4_WORD_OFFSET + NEOGEO_SPRITE_LIST_MAX_SPRITES) {
        return 0u;
    }
    const uint16_t *vram = st->vram;
    const uint16_t *scb2 = vram + NEOGEO_SPRITE_LIST_SCB2_WORD_OFFSET;
    const uint16_t *scb3 = vram + NEOGEO_SPRITE_LIST_SCB3_WORD_OFFSET;
    const uint16_t *scb4 = vram + NEOGEO_SPRITE_LIST_SCB4_WORD_OFFSET;
    uint32_t hash = 2166136261u;
    hash = neogeo_sprite_list_hashWord(hash, (uint16_t)(st->sprlimit & 0xffffu));
    hash = neogeo_sprite_list_hashWord(hash, (uint16_t)(st->screen_h & 0xffff));
    for (unsigned i = 0u; i < NEOGEO_SPRITE_LIST_MAX_SPRITES; ++i) {
        size_t scb1Base = (size_t)i * NEOGEO_SPRITE_LIST_SPRITE_VRAM_WORDS_PER_SPRITE;
        for (size_t word = 0u; word < NEOGEO_SPRITE_LIST_SPRITE_VRAM_WORDS_PER_SPRITE; ++word) {
            size_t offset = scb1Base + word;
            if (offset >= st->vram_words) {
                break;
            }
            hash = neogeo_sprite_list_hashWord(hash, vram[offset]);
        }
        hash = neogeo_sprite_list_hashWord(hash, scb2[i]);
        hash = neogeo_sprite_list_hashWord(hash, scb3[i]);
        hash = neogeo_sprite_list_hashWord(hash, scb4[i]);
    }
    return hash ? hash : 1u;
}

static int
neogeo_sprite_list_decodeRows(neogeo_sprite_list_state_t *ui, const e9k_debug_sprite_state_t *st)
{
    if (!ui || !st || !st->vram) {
        return 0;
    }
    const uint16_t *vram = st->vram;
    if (st->vram_words <= NEOGEO_SPRITE_LIST_SCB4_WORD_OFFSET + NEOGEO_SPRITE_LIST_MAX_SPRITES) {
        return 0;
    }
    uint32_t hash = neogeo_sprite_list_hashState(st);
    if (ui->hasLastSpriteHash && ui->lastSpriteHash == hash) {
        return 0;
    }
    ui->lastSpriteHash = hash;
    ui->hasLastSpriteHash = 1;

    const uint16_t *scb2 = vram + NEOGEO_SPRITE_LIST_SCB2_WORD_OFFSET;
    const uint16_t *scb3 = vram + NEOGEO_SPRITE_LIST_SCB3_WORD_OFFSET;
    const uint16_t *scb4 = vram + NEOGEO_SPRITE_LIST_SCB4_WORD_OFFSET;

    unsigned xpos = 0u;
    unsigned ypos = 0u;
    unsigned rows = 0u;
    unsigned hshrink = NEOGEO_SPRITE_LIST_SCB2_HSHRINK_MASK;
    unsigned vshrink = NEOGEO_SPRITE_LIST_SCB2_VSHRINK_MASK;
    unsigned rootIndex = 0u;
    unsigned chainOffset = 0u;
    int screenH = st->screen_h > 0 ? st->screen_h : NEOGEO_SPRITE_LIST_VISIBLE_H;

    memset(ui->rows, 0, sizeof(ui->rows));
    for (unsigned i = 0u; i < NEOGEO_SPRITE_LIST_MAX_SPRITES; ++i) {
        uint16_t scb2w = scb2[i];
        uint16_t scb3w = scb3[i];
        uint16_t scb4w = scb4[i];
        int sticky = (i != 0u && (scb3w & NEOGEO_SPRITE_LIST_SCB3_CHAIN_FLAG)) ? 1 : 0;
        if (sticky) {
            xpos = (unsigned)((xpos + (hshrink + 1u)) & NEOGEO_SPRITE_LIST_WRAP_MASK);
            chainOffset++;
        } else {
            rootIndex = i;
            chainOffset = 0u;
            xpos = (unsigned)((scb4w >> NEOGEO_SPRITE_LIST_SCB4_XPOS_SHIFT) & NEOGEO_SPRITE_LIST_WRAP_MASK);
            ypos = (unsigned)((scb3w >> NEOGEO_SPRITE_LIST_SCB3_YPOS_SHIFT) & NEOGEO_SPRITE_LIST_SCB3_YPOS_MASK);
            rows = (unsigned)(scb3w & NEOGEO_SPRITE_LIST_SCB3_ROW_MASK);
            vshrink = (unsigned)(scb2w & NEOGEO_SPRITE_LIST_SCB2_VSHRINK_MASK);
        }
        hshrink = (unsigned)((scb2w >> NEOGEO_SPRITE_LIST_SCB2_HSHRINK_SHIFT) &
                             NEOGEO_SPRITE_LIST_SCB2_HSHRINK_MASK);

        neogeo_sprite_list_row_t *row = &ui->rows[i];
        row->index = i;
        row->rootIndex = rootIndex;
        row->chainOffset = chainOffset;
        row->xpos = xpos;
        row->ypos = ypos;
        row->rows = rows;
        row->pixelHeight = rows << 4;
        row->width = neogeo_sprite_list_countShrinkWidth(hshrink);
        row->hshrink = hshrink;
        row->vshrink = vshrink;
        row->scb2 = scb2w;
        row->scb3 = scb3w;
        row->scb4 = scb4w;
        row->sticky = sticky;
        row->active = (rows != 0u && ypos != (unsigned)screenH) ? 1 : 0;

        size_t evenWordOffset = (size_t)i * NEOGEO_SPRITE_LIST_SPRITE_VRAM_WORDS_PER_SPRITE;
        size_t oddWordOffset = evenWordOffset + NEOGEO_SPRITE_LIST_SPRITE_TILE_ODD_WORD_OFFSET;
        if (evenWordOffset < st->vram_words) {
            row->scb1Tile = (unsigned)vram[evenWordOffset];
        }
        if (oddWordOffset < st->vram_words) {
            row->scb1Attr = (unsigned)vram[oddWordOffset];
            row->paletteBank = (unsigned)((vram[oddWordOffset] >> NEOGEO_SPRITE_LIST_SPRITE_PALETTE_SHIFT) &
                                          NEOGEO_SPRITE_LIST_SPRITE_PALETTE_MASK);
        }
        row->hasAnimBits = neogeo_sprite_list_spriteHasAnimBits(vram, st->vram_words, i, rows);
        row->visible = neogeo_sprite_list_spriteIsVisible(row);
    }

    for (int i = NEOGEO_SPRITE_LIST_MAX_SPRITES - 1; i >= 0; --i) {
        unsigned root = ui->rows[i].rootIndex;
        if (root < NEOGEO_SPRITE_LIST_MAX_SPRITES) {
            unsigned length = ui->rows[i].chainOffset + 1u;
            if (length > ui->rows[root].chainLength) {
                ui->rows[root].chainLength = length;
            }
            ui->rows[i].chainLength = ui->rows[root].chainLength;
        }
    }
    ui->filterCacheDirty = 1;
    ui->measurementsValid = 0;
    return 1;
}

static int
neogeo_sprite_list_containsInsensitive(const char *text, const char *needle)
{
    if (!text || !needle) {
        return 0;
    }
    if (!needle[0]) {
        return 1;
    }
    size_t needleLen = strlen(needle);
    size_t textLen = strlen(text);
    if (needleLen > textLen) {
        return 0;
    }
    for (size_t i = 0u; i + needleLen <= textLen; ++i) {
        size_t j = 0u;
        for (; j < needleLen; ++j) {
            int a = tolower((unsigned char)text[i + j]);
            int b = tolower((unsigned char)needle[j]);
            if (a != b) {
                break;
            }
        }
        if (j == needleLen) {
            return 1;
        }
    }
    return 0;
}

static void
neogeo_sprite_list_appendText(char *dest, size_t cap, const char *text)
{
    if (!dest || cap == 0u || !text) {
        return;
    }
    size_t len = strlen(dest);
    if (len >= cap - 1u) {
        return;
    }
    size_t left = cap - len - 1u;
    size_t n = strlen(text);
    if (n > left) {
        n = left;
    }
    memcpy(dest + len, text, n);
    dest[len + n] = '\0';
}

static void
neogeo_sprite_list_formatDisplayRow(const neogeo_sprite_list_row_t *row,
                                    neogeo_sprite_list_display_row_t *out)
{
    if (!row || !out) {
        return;
    }
    snprintf(out->index, sizeof(out->index), "%03u", row->index);
    snprintf(out->root, sizeof(out->root), "%03u+%u/%u", row->rootIndex, row->chainOffset, row->chainLength);
    out->status[0] = '\0';
    neogeo_sprite_list_appendText(out->status, sizeof(out->status), row->active ? (row->visible ? "VIS" : "ACT") : "OFF");
    if (row->sticky) {
        neogeo_sprite_list_appendText(out->status, sizeof(out->status), " CH");
    }
    if (row->hasAnimBits) {
        neogeo_sprite_list_appendText(out->status, sizeof(out->status), " AN");
    }
    snprintf(out->xpos, sizeof(out->xpos), "%03u", row->xpos);
    snprintf(out->ypos, sizeof(out->ypos), "%03u", row->ypos);
    snprintf(out->rows, sizeof(out->rows), "%02u/%03u", row->rows, row->pixelHeight);
    snprintf(out->width, sizeof(out->width), "%02u", row->width);
    snprintf(out->shrink, sizeof(out->shrink), "%x/%02x", row->hshrink, row->vshrink);
    snprintf(out->palette, sizeof(out->palette), "%02x", row->paletteBank);
    snprintf(out->scb1, sizeof(out->scb1), "%04x %04x", row->scb1Tile, row->scb1Attr);
    snprintf(out->scb, sizeof(out->scb), "%04x %04x %04x", row->scb2, row->scb3, row->scb4);
}

static int
neogeo_sprite_list_rowMatchesFilter(const neogeo_sprite_list_state_t *ui, int rowIndex)
{
    if (!ui || rowIndex < 0 || rowIndex >= NEOGEO_SPRITE_LIST_MAX_SPRITES) {
        return 0;
    }
    const neogeo_sprite_list_row_t *row = &ui->rows[rowIndex];
    if (ui->activeOnly && !row->active) {
        return 0;
    }
    const neogeo_sprite_list_display_row_t *display = &ui->displayRows[rowIndex];
    for (int i = 0; i < NEOGEO_SPRITE_LIST_FILTER_COUNT; ++i) {
        if (ui->filters[i][0] == '\0') {
            continue;
        }
        const char *columnText = neogeo_sprite_list_filterTextForDisplayRow(display, i);
        if (!neogeo_sprite_list_containsInsensitive(columnText, ui->filters[i])) {
            return 0;
        }
    }
    return 1;
}

static void
neogeo_sprite_list_applySelection(neogeo_sprite_list_state_t *ui)
{
    if (!ui || ui->selectedIndex < 0 || ui->selectedIndex >= NEOGEO_SPRITE_LIST_MAX_SPRITES) {
        neogeo_sprite_debug_setSelection(-1, -1, 0);
        return;
    }
    const neogeo_sprite_list_row_t *row = &ui->rows[ui->selectedIndex];
    neogeo_sprite_debug_setSelection((int)row->index, (int)row->rootIndex, ui->highlightChain);
}

static const char *
neogeo_sprite_list_filterLabel(int index)
{
    switch (index) {
    case neogeo_sprite_list_filter_index: return "#";
    case neogeo_sprite_list_filter_root: return "ROOT";
    case neogeo_sprite_list_filter_status: return "STATUS";
    case neogeo_sprite_list_filter_xpos: return "X";
    case neogeo_sprite_list_filter_ypos: return "Y";
    case neogeo_sprite_list_filter_rows: return "ROWS";
    case neogeo_sprite_list_filter_width: return "W";
    case neogeo_sprite_list_filter_shrink: return "SHRINK";
    case neogeo_sprite_list_filter_palette: return "PAL";
    case neogeo_sprite_list_filter_scb1: return "SCB1";
    case neogeo_sprite_list_filter_scb: return "SCB";
    default:
        break;
    }
    return "";
}

static int
neogeo_sprite_list_filterColumnWidth(const neogeo_sprite_list_state_t *ui, int index)
{
    if (!ui) {
        return 40;
    }
    switch (index) {
    case neogeo_sprite_list_filter_index: return ui->colIndexW;
    case neogeo_sprite_list_filter_root: return ui->colRootW;
    case neogeo_sprite_list_filter_status: return ui->colStatusW;
    case neogeo_sprite_list_filter_xpos: return ui->colXposW;
    case neogeo_sprite_list_filter_ypos: return ui->colYposW;
    case neogeo_sprite_list_filter_rows: return ui->colRowsW;
    case neogeo_sprite_list_filter_width: return ui->colWidthW;
    case neogeo_sprite_list_filter_shrink: return ui->colShrinkW;
    case neogeo_sprite_list_filter_palette: return ui->colPaletteW;
    case neogeo_sprite_list_filter_scb1: return ui->colScb1W;
    case neogeo_sprite_list_filter_scb: return ui->colScbW;
    default:
        break;
    }
    return 40;
}

static const char *
neogeo_sprite_list_filterTextForDisplayRow(const neogeo_sprite_list_display_row_t *display, int index)
{
    if (!display) {
        return "";
    }
    switch (index) {
    case neogeo_sprite_list_filter_index: return display->index;
    case neogeo_sprite_list_filter_root: return display->root;
    case neogeo_sprite_list_filter_status: return display->status;
    case neogeo_sprite_list_filter_xpos: return display->xpos;
    case neogeo_sprite_list_filter_ypos: return display->ypos;
    case neogeo_sprite_list_filter_rows: return display->rows;
    case neogeo_sprite_list_filter_width: return display->width;
    case neogeo_sprite_list_filter_shrink: return display->shrink;
    case neogeo_sprite_list_filter_palette: return display->palette;
    case neogeo_sprite_list_filter_scb1: return display->scb1;
    case neogeo_sprite_list_filter_scb: return display->scb;
    default:
        break;
    }
    return "";
}

static void
neogeo_sprite_list_rebuildFilterCache(neogeo_sprite_list_state_t *ui)
{
    if (!ui || !ui->filterCacheDirty) {
        return;
    }
    TTF_Font *font = ui->ctx.font ? ui->ctx.font : e9ui->ctx.font;
    if (!ui->measurementsValid || ui->measuredFont != font) {
        neogeo_sprite_list_refreshDisplayMeasurements(ui, font);
    }
    ui->filteredCount = 0;
    for (int i = 0; i < NEOGEO_SPRITE_LIST_MAX_SPRITES; ++i) {
        if (neogeo_sprite_list_rowMatchesFilter(ui, i)) {
            ui->filteredIndexes[ui->filteredCount++] = i;
        }
    }
    ui->filterCacheDirty = 0;
}

static int
neogeo_sprite_list_measureLineHeight(void)
{
    TTF_Font *font = e9ui->ctx.font;
    int lineHeight = font ? TTF_FontHeight(font) : 0;
    if (lineHeight <= 0) {
        lineHeight = 16;
    }
    return lineHeight;
}

static e9ui_rect_t
neogeo_sprite_list_windowDefaultRect(const e9ui_context_t *ctx)
{
    e9ui_rect_t rect = {
        e9ui_scale_px(ctx, 112),
        e9ui_scale_px(ctx, 112),
        e9ui_scale_px(ctx, 1140),
        e9ui_scale_px(ctx, 480)
    };
    return rect;
}

static int
neogeo_sprite_list_measureText(TTF_Font *font, const char *text)
{
    if (!font || !text) {
        return 0;
    }
    int textW = 0;
    int textH = 0;
    if (TTF_SizeUTF8(font, text, &textW, &textH) != 0) {
        return 0;
    }
    return textW;
}

static void
neogeo_sprite_list_measureColumn(TTF_Font *font, int *width, const char *text)
{
    if (!width) {
        return;
    }
    int textW = neogeo_sprite_list_measureText(font, text);
    if (textW > *width) {
        *width = textW;
    }
}

static void
neogeo_sprite_list_refreshDisplayMeasurements(neogeo_sprite_list_state_t *ui, TTF_Font *font)
{
    if (!ui) {
        return;
    }
    const int pad = 10;
    ui->colIndexW = NEOGEO_SPRITE_LIST_COL_INDEX_W;
    ui->colRootW = NEOGEO_SPRITE_LIST_COL_ROOT_W;
    ui->colStatusW = NEOGEO_SPRITE_LIST_COL_STATUS_W;
    ui->colXposW = NEOGEO_SPRITE_LIST_COL_XPOS_W;
    ui->colYposW = NEOGEO_SPRITE_LIST_COL_YPOS_W;
    ui->colRowsW = NEOGEO_SPRITE_LIST_COL_ROWS_W;
    ui->colWidthW = NEOGEO_SPRITE_LIST_COL_WIDTH_W;
    ui->colShrinkW = NEOGEO_SPRITE_LIST_COL_SHRINK_W;
    ui->colPaletteW = NEOGEO_SPRITE_LIST_COL_PALETTE_W;
    ui->colScb1W = NEOGEO_SPRITE_LIST_COL_SCB1_W;
    ui->colScbW = NEOGEO_SPRITE_LIST_COL_SCB_W;

    neogeo_sprite_list_measureColumn(font, &ui->colIndexW, "#");
    neogeo_sprite_list_measureColumn(font, &ui->colRootW, "ROOT");
    neogeo_sprite_list_measureColumn(font, &ui->colStatusW, "STATUS");
    neogeo_sprite_list_measureColumn(font, &ui->colXposW, "X");
    neogeo_sprite_list_measureColumn(font, &ui->colYposW, "Y");
    neogeo_sprite_list_measureColumn(font, &ui->colRowsW, "ROWS");
    neogeo_sprite_list_measureColumn(font, &ui->colWidthW, "W");
    neogeo_sprite_list_measureColumn(font, &ui->colShrinkW, "SHRINK");
    neogeo_sprite_list_measureColumn(font, &ui->colPaletteW, "PAL");
    neogeo_sprite_list_measureColumn(font, &ui->colScb1W, "SCB1");
    neogeo_sprite_list_measureColumn(font, &ui->colScbW, "SCB2 SCB3 SCB4");

    for (int i = 0; i < NEOGEO_SPRITE_LIST_MAX_SPRITES; ++i) {
        neogeo_sprite_list_formatDisplayRow(&ui->rows[i], &ui->displayRows[i]);
        neogeo_sprite_list_measureColumn(font, &ui->colIndexW, ui->displayRows[i].index);
        neogeo_sprite_list_measureColumn(font, &ui->colRootW, ui->displayRows[i].root);
        neogeo_sprite_list_measureColumn(font, &ui->colStatusW, ui->displayRows[i].status);
        neogeo_sprite_list_measureColumn(font, &ui->colXposW, ui->displayRows[i].xpos);
        neogeo_sprite_list_measureColumn(font, &ui->colYposW, ui->displayRows[i].ypos);
        neogeo_sprite_list_measureColumn(font, &ui->colRowsW, ui->displayRows[i].rows);
        neogeo_sprite_list_measureColumn(font, &ui->colWidthW, ui->displayRows[i].width);
        neogeo_sprite_list_measureColumn(font, &ui->colShrinkW, ui->displayRows[i].shrink);
        neogeo_sprite_list_measureColumn(font, &ui->colPaletteW, ui->displayRows[i].palette);
        neogeo_sprite_list_measureColumn(font, &ui->colScb1W, ui->displayRows[i].scb1);
        neogeo_sprite_list_measureColumn(font, &ui->colScbW, ui->displayRows[i].scb);
    }

    ui->colIndexW += pad;
    ui->colRootW += pad;
    ui->colStatusW += pad;
    ui->colXposW += pad;
    ui->colYposW += pad;
    ui->colRowsW += pad;
    ui->colWidthW += pad;
    ui->colShrinkW += pad;
    ui->colPaletteW += pad;
    ui->colScb1W += pad;
    ui->colScbW += pad;
    ui->measuredFont = font;
    ui->measurementsValid = 1;
}

static void
neogeo_sprite_list_computeLayout(const e9ui_context_t *ctx, int originX, neogeo_sprite_list_layout_t *out)
{
    if (!out) {
        return;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    TTF_Font *font = (ctx && ctx->font) ? ctx->font : e9ui->ctx.font;
    memset(out, 0, sizeof(*out));
    int gap = NEOGEO_SPRITE_LIST_COL_GAP;
    out->lineHeight = (ctx && ctx->font) ? TTF_FontHeight(ctx->font) : neogeo_sprite_list_measureLineHeight();
    if (out->lineHeight <= 0) {
        out->lineHeight = neogeo_sprite_list_measureLineHeight();
    }

    if (!ui->measurementsValid || ui->measuredFont != font) {
        neogeo_sprite_list_refreshDisplayMeasurements(ui, font);
    }
    out->colIndexW = ui->colIndexW;
    out->colRootW = ui->colRootW;
    out->colStatusW = ui->colStatusW;
    out->colXposW = ui->colXposW;
    out->colYposW = ui->colYposW;
    out->colRowsW = ui->colRowsW;
    out->colWidthW = ui->colWidthW;
    out->colShrinkW = ui->colShrinkW;
    out->colPaletteW = ui->colPaletteW;
    out->colScb1W = ui->colScb1W;
    out->colScbW = ui->colScbW;

    out->colIndexX = originX;
    out->colRootX = out->colIndexX + out->colIndexW + gap;
    out->colStatusX = out->colRootX + out->colRootW + gap;
    out->colXposX = out->colStatusX + out->colStatusW + gap;
    out->colYposX = out->colXposX + out->colXposW + gap;
    out->colRowsX = out->colYposX + out->colYposW + gap;
    out->colWidthX = out->colRowsX + out->colRowsW + gap;
    out->colShrinkX = out->colWidthX + out->colWidthW + gap;
    out->colPaletteX = out->colShrinkX + out->colShrinkW + gap;
    out->colScb1X = out->colPaletteX + out->colPaletteW + gap;
    out->colScbX = out->colScb1X + out->colScb1W + gap;
    out->contentWidth = out->colScbX + out->colScbW - originX;
}

static void
neogeo_sprite_list_drawTextClipped(neogeo_sprite_list_state_t *ui,
                                   TTF_Font *font,
                                   int x,
                                   int y,
                                   int width,
                                   const char *text,
                                   SDL_Color color)
{
    if (!ui || !ui->renderer || !font || !text || width <= 0) {
        return;
    }
    int textW = 0;
    int textH = 0;
    SDL_Texture *tex = e9ui_text_cache_getText(ui->renderer, font, text, color, &textW, &textH);
    if (!tex || textW <= 0 || textH <= 0) {
        return;
    }
    SDL_Rect dst = { x, y, textW, textH };
    int hadClip = SDL_RenderIsClipEnabled(ui->renderer) ? 1 : 0;
    SDL_Rect prevClip = { 0, 0, 0, 0 };
    if (hadClip) {
        SDL_RenderGetClipRect(ui->renderer, &prevClip);
    }
    SDL_Rect clip = { x, y, width, textH };
    if (hadClip) {
        SDL_Rect clipped = { 0, 0, 0, 0 };
        if (SDL_IntersectRect(&prevClip, &clip, &clipped)) {
            SDL_RenderSetClipRect(ui->renderer, &clipped);
        } else {
            SDL_Rect empty = { 0, 0, 0, 0 };
            SDL_RenderSetClipRect(ui->renderer, &empty);
        }
    } else {
        SDL_RenderSetClipRect(ui->renderer, &clip);
    }
    SDL_RenderCopy(ui->renderer, tex, NULL, &dst);
    if (hadClip) {
        SDL_RenderSetClipRect(ui->renderer, &prevClip);
    } else {
        SDL_RenderSetClipRect(ui->renderer, NULL);
    }
}

static int
neogeo_sprite_list_canvasPreferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    (void)self;
    (void)ctx;
    (void)availW;
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    neogeo_sprite_list_rebuildFilterCache(ui);
    int lineHeight = neogeo_sprite_list_measureLineHeight();
    return (ui->filteredCount + 1) * lineHeight;
}

static void
neogeo_sprite_list_canvasLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    if (!self) {
        return;
    }
    self->bounds = bounds;
}

static int
neogeo_sprite_list_canvasHandleEvent(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev)
{
    (void)ctx;
    if (!self || !ev) {
        return 0;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_LEFT) {
        neogeo_sprite_list_layout_t layout;
        neogeo_sprite_list_computeLayout(ctx, self->bounds.x + e9ui_scale_px(ctx, 6), &layout);
        if (layout.lineHeight <= 0) {
            return 0;
        }
        int rowIndex = (ev->button.y - self->bounds.y) / layout.lineHeight - 1;
        neogeo_sprite_list_rebuildFilterCache(ui);
        if (rowIndex >= 0 && rowIndex < ui->filteredCount) {
            ui->selectedIndex = ui->filteredIndexes[rowIndex];
            neogeo_sprite_list_applySelection(ui);
            return 1;
        }
    }
    return 0;
}

static void
neogeo_sprite_list_canvasRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !ctx || !ctx->renderer) {
        return;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!ui->windowState.open) {
        return;
    }
    ui->ctx = *ctx;
    ui->ctx.font = e9ui->ctx.font;
    ui->renderer = ctx->renderer;
    ui->window = ctx->window;

    TTF_Font *font = e9ui->ctx.font;
    if (!font) {
        return;
    }

    neogeo_sprite_list_layout_t layout;
    neogeo_sprite_list_computeLayout(ctx, self->bounds.x + e9ui_scale_px(ctx, 6), &layout);
    if (layout.lineHeight <= 0) {
        return;
    }
    if (ui->listScroll) {
        e9ui_scroll_setContentWidthPx(ui->listScroll, layout.contentWidth + 12);
    }
    neogeo_sprite_list_rebuildFilterCache(ui);

    SDL_Rect viewRect = { self->bounds.x, self->bounds.y, self->bounds.w, self->bounds.h };
    if (SDL_RenderIsClipEnabled(ctx->renderer)) {
        SDL_Rect clipRect;
        SDL_RenderGetClipRect(ctx->renderer, &clipRect);
        if (!SDL_IntersectRect(&viewRect, &clipRect, &viewRect)) {
            return;
        }
    }

    SDL_SetRenderDrawColor(ctx->renderer, 12, 12, 12, 255);
    SDL_RenderFillRect(ctx->renderer, &viewRect);

    SDL_Color headerColor = { 150, 170, 190, 255 };
    SDL_Color rowColor = { 220, 220, 224, 255 };
    SDL_Color inactiveColor = { 116, 116, 124, 255 };
    SDL_Color visibleColor = { 128, 216, 150, 255 };
    SDL_Color chainColor = { 120, 190, 255, 255 };
    SDL_Color animColor = { 255, 198, 92, 255 };

    int headerY = self->bounds.y;
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colIndexX, headerY, layout.colIndexW, "#", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colRootX, headerY, layout.colRootW, "ROOT", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colStatusX, headerY, layout.colStatusW, "STATUS", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colXposX, headerY, layout.colXposW, "X", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colYposX, headerY, layout.colYposW, "Y", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colRowsX, headerY, layout.colRowsW, "ROWS", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colWidthX, headerY, layout.colWidthW, "W", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colShrinkX, headerY, layout.colShrinkW, "SHRINK", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colPaletteX, headerY, layout.colPaletteW, "PAL", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colScb1X, headerY, layout.colScb1W, "SCB1", headerColor);
    neogeo_sprite_list_drawTextClipped(ui, font, layout.colScbX, headerY, layout.colScbW, "SCB2 SCB3 SCB4", headerColor);

    int topRow = 0;
    if (viewRect.y > self->bounds.y + layout.lineHeight) {
        topRow = (viewRect.y - self->bounds.y) / layout.lineHeight - 1;
    }
    if (topRow < 0) {
        topRow = 0;
    }
    int visibleRows = viewRect.h / layout.lineHeight + 2;
    for (int filteredRow = topRow; filteredRow < ui->filteredCount; ++filteredRow) {
        int y = self->bounds.y + (filteredRow + 1) * layout.lineHeight;
        if (y >= viewRect.y + viewRect.h) {
            break;
        }
        if (y + layout.lineHeight <= viewRect.y) {
            continue;
        }
        int rowIndex = ui->filteredIndexes[filteredRow];
        const neogeo_sprite_list_row_t *row = &ui->rows[rowIndex];
        if (rowIndex == ui->selectedIndex) {
            SDL_SetRenderDrawColor(ctx->renderer, 46, 55, 68, 255);
            SDL_Rect rowBg = { self->bounds.x, y, self->bounds.w, layout.lineHeight };
            SDL_RenderFillRect(ctx->renderer, &rowBg);
        } else if ((filteredRow & 1) != 0) {
            SDL_SetRenderDrawColor(ctx->renderer, 16, 16, 20, 255);
            SDL_Rect rowBg = { self->bounds.x, y, self->bounds.w, layout.lineHeight };
            SDL_RenderFillRect(ctx->renderer, &rowBg);
        }

        const neogeo_sprite_list_display_row_t *display = &ui->displayRows[rowIndex];

        SDL_Color statusColor = row->active ? (row->visible ? visibleColor : rowColor) : inactiveColor;
        if (row->hasAnimBits) {
            statusColor = animColor;
        } else if (row->sticky) {
            statusColor = chainColor;
        }
        SDL_Color textColor = row->active ? rowColor : inactiveColor;
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colIndexX, y, layout.colIndexW, display->index, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colRootX, y, layout.colRootW, display->root, chainColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colStatusX, y, layout.colStatusW, display->status, statusColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colXposX, y, layout.colXposW, display->xpos, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colYposX, y, layout.colYposW, display->ypos, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colRowsX, y, layout.colRowsW, display->rows, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colWidthX, y, layout.colWidthW, display->width, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colShrinkX, y, layout.colShrinkW, display->shrink, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colPaletteX, y, layout.colPaletteW, display->palette, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colScb1X, y, layout.colScb1W, display->scb1, textColor);
        neogeo_sprite_list_drawTextClipped(ui, font, layout.colScbX, y, layout.colScbW, display->scb, textColor);

        visibleRows--;
        if (visibleRows <= 0) {
            break;
        }
    }
}

static void
neogeo_sprite_list_canvasDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self || !self->state) {
        return;
    }
    alloc_free(self->state);
    self->state = NULL;
}

static e9ui_component_t *
neogeo_sprite_list_makeCanvas(void)
{
    e9ui_component_t *canvas = (e9ui_component_t *)alloc_calloc(1, sizeof(*canvas));
    neogeo_sprite_list_canvas_state_t *st =
        (neogeo_sprite_list_canvas_state_t *)alloc_calloc(1, sizeof(*st));
    if (!canvas || !st) {
        alloc_free(canvas);
        alloc_free(st);
        return NULL;
    }
    canvas->name = "neogeo_sprite_list_canvas";
    canvas->state = st;
    canvas->preferredHeight = neogeo_sprite_list_canvasPreferredHeight;
    canvas->layout = neogeo_sprite_list_canvasLayout;
    canvas->render = neogeo_sprite_list_canvasRender;
    canvas->handleEvent = neogeo_sprite_list_canvasHandleEvent;
    canvas->dtor = neogeo_sprite_list_canvasDtor;
    return canvas;
}

static void
neogeo_sprite_list_filterTextboxChanged(e9ui_context_t *ctx, void *user)
{
    (void)ctx;
    neogeo_sprite_list_filter_cb_t *cb = (neogeo_sprite_list_filter_cb_t *)user;
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!cb || cb->filterIndex < 0 || cb->filterIndex >= NEOGEO_SPRITE_LIST_FILTER_COUNT) {
        return;
    }
    const char *text = e9ui_textbox_getText(ui->filterTextboxes[cb->filterIndex]);
    if (!text) {
        text = "";
    }
    if (strncmp(ui->filters[cb->filterIndex], text, NEOGEO_SPRITE_LIST_FILTER_TEXT_MAX) != 0) {
        snprintf(ui->filters[cb->filterIndex], NEOGEO_SPRITE_LIST_FILTER_TEXT_MAX, "%s", text);
        ui->filterCacheDirty = 1;
        if (ui->listScroll) {
            e9ui_scroll_setScrollPx(ui->listScroll, 0, 0);
        }
    }
}

static void
neogeo_sprite_list_activeOnlyChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    (void)user;
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    ui->activeOnly = selected ? 1 : 0;
    ui->filterCacheDirty = 1;
    config_saveConfig();
    if (ui->listScroll) {
        e9ui_scroll_setScrollPx(ui->listScroll, 0, 0);
    }
}

static void
neogeo_sprite_list_highlightChainChanged(e9ui_component_t *self, e9ui_context_t *ctx, int selected, void *user)
{
    (void)self;
    (void)ctx;
    (void)user;
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    ui->highlightChain = selected ? 1 : 0;
    neogeo_sprite_list_applySelection(ui);
    config_saveConfig();
}

static void
neogeo_sprite_list_clearFilterClicked(e9ui_context_t *ctx, void *user)
{
    (void)ctx;
    (void)user;
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    for (int i = 0; i < NEOGEO_SPRITE_LIST_FILTER_COUNT; ++i) {
        ui->filters[i][0] = '\0';
        if (ui->filterTextboxes[i]) {
            e9ui_textbox_setText(ui->filterTextboxes[i], "");
        }
    }
    ui->filterCacheDirty = 1;
}

static e9ui_component_t *
neogeo_sprite_list_buildFilterRoot(void)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    e9ui_component_t *root = e9ui_stack_makeVertical();
    e9ui_component_t *controlsRow = e9ui_hstack_make();
    e9ui_component_t *filterRow = e9ui_hstack_make();
    e9ui_component_t *activeOnly = e9ui_checkbox_make("Active",
                                                       ui->activeOnly,
                                                       neogeo_sprite_list_activeOnlyChanged,
                                                       NULL);
    e9ui_component_t *highlightChain = e9ui_checkbox_make("Chain",
                                                          ui->highlightChain,
                                                          neogeo_sprite_list_highlightChainChanged,
                                                          NULL);
    e9ui_component_t *clearButton = e9ui_button_make("Clear", neogeo_sprite_list_clearFilterClicked, NULL);
    if (!root || !controlsRow || !filterRow) {
        return NULL;
    }
    if (clearButton) {
        e9ui_button_setMini(clearButton, 1);
    }
    ui->activeOnlyCheckbox = activeOnly;
    ui->highlightChainCheckbox = highlightChain;

    int activeW = 74;
    int chainW = 74;
    int clearW = 60;
    if (activeOnly) {
        e9ui_checkbox_measure(activeOnly, &ui->ctx, &activeW, NULL);
    }
    if (highlightChain) {
        e9ui_checkbox_measure(highlightChain, &ui->ctx, &chainW, NULL);
    }
    if (clearButton) {
        e9ui_button_measure(clearButton, &ui->ctx, &clearW, NULL);
    }
    e9ui_hstack_addFixed(controlsRow, activeOnly, activeW);
    e9ui_hstack_addFixed(controlsRow, e9ui_spacer_make(NEOGEO_SPRITE_LIST_GAP), NEOGEO_SPRITE_LIST_GAP);
    e9ui_hstack_addFixed(controlsRow, highlightChain, chainW);
    e9ui_hstack_addFlex(controlsRow, e9ui_spacer_make(1));
    e9ui_hstack_addFixed(controlsRow, clearButton, clearW);

    TTF_Font *font = ui->ctx.font ? ui->ctx.font : e9ui->ctx.font;
    if (!ui->measurementsValid || ui->measuredFont != font) {
        neogeo_sprite_list_refreshDisplayMeasurements(ui, font);
    }
    for (int i = 0; i < NEOGEO_SPRITE_LIST_FILTER_COUNT; ++i) {
        ui->filterCbs[i].filterIndex = i;
        e9ui_component_t *textbox = e9ui_textbox_make(NEOGEO_SPRITE_LIST_FILTER_TEXT_MAX - 1,
                                                      NULL,
                                                      neogeo_sprite_list_filterTextboxChanged,
                                                      &ui->filterCbs[i]);
        if (textbox) {
            e9ui_textbox_setPlaceholder(textbox, neogeo_sprite_list_filterLabel(i));
            e9ui_textbox_setText(textbox, ui->filters[i]);
        }
        ui->filterTextboxes[i] = textbox;
        e9ui_hstack_addFixed(filterRow, textbox, neogeo_sprite_list_filterColumnWidth(ui, i));
        if (i + 1 < NEOGEO_SPRITE_LIST_FILTER_COUNT) {
            e9ui_hstack_addFixed(filterRow,
                                 e9ui_spacer_make(NEOGEO_SPRITE_LIST_COL_GAP),
                                 NEOGEO_SPRITE_LIST_COL_GAP);
        }
    }

    e9ui_stack_addFixed(root, controlsRow);
    e9ui_stack_addFixed(root, e9ui_vspacer_make(NEOGEO_SPRITE_LIST_FILTER_GAP_Y));
    e9ui_stack_addFixed(root, filterRow);
    return root;
}

static e9ui_component_t *
neogeo_sprite_list_wrapRowWithSidePadding(e9ui_component_t *child, int leftPad, int rightPad)
{
    if (!child) {
        return NULL;
    }
    e9ui_component_t *row = e9ui_hstack_make();
    if (leftPad > 0) {
        e9ui_hstack_addFixed(row, e9ui_spacer_make(leftPad), leftPad);
    }
    e9ui_hstack_addFlex(row, child);
    if (rightPad > 0) {
        e9ui_hstack_addFixed(row, e9ui_spacer_make(rightPad), rightPad);
    }
    return row;
}

static e9ui_component_t *
neogeo_sprite_list_buildRoot(void)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    e9ui_component_t *root = e9ui_stack_makeVertical();
    e9ui_component_t *filterRoot = neogeo_sprite_list_buildFilterRoot();
    e9ui_component_t *canvas = neogeo_sprite_list_makeCanvas();
    e9ui_component_t *listScroll = e9ui_scroll_make(canvas);
    if (!root || !filterRoot || !canvas || !listScroll) {
        return NULL;
    }
    ui->filterRoot = filterRoot;
    ui->listCanvas = canvas;
    ui->listScroll = listScroll;
    neogeo_sprite_list_layout_t layout;
    neogeo_sprite_list_computeLayout(&ui->ctx, 0, &layout);
    e9ui_scroll_setContentWidthPx(listScroll, layout.contentWidth + 12);

    const int filterPad = NEOGEO_SPRITE_LIST_PAD + 6;
    e9ui_stack_addFixed(root, e9ui_vspacer_make(NEOGEO_SPRITE_LIST_PAD));
    e9ui_stack_addFixed(root, neogeo_sprite_list_wrapRowWithSidePadding(filterRoot, filterPad, filterPad));
    e9ui_stack_addFixed(root, e9ui_vspacer_make(NEOGEO_SPRITE_LIST_FILTER_GAP_Y));
    e9ui_stack_addFlex(root, neogeo_sprite_list_wrapRowWithSidePadding(listScroll, NEOGEO_SPRITE_LIST_PAD, NEOGEO_SPRITE_LIST_PAD));
    e9ui_stack_addFixed(root, e9ui_vspacer_make(NEOGEO_SPRITE_LIST_PAD));
    return root;
}

static void
neogeo_sprite_list_overlayBodyLayout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    if (!self || !ctx) {
        return;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!ui->root) {
        return;
    }
    self->bounds = bounds;
    ui->ctx = *ctx;
    ui->ctx.window = ctx->window;
    ui->ctx.renderer = ctx->renderer;
    ui->ctx.font = e9ui->ctx.font;
    ui->ctx.winW = bounds.w;
    ui->ctx.winH = bounds.h;
    ui->ctx.focusRoot = ui->root;
    ui->ctx.focusFullscreen = NULL;
    if (ui->root->layout) {
        ui->root->layout(ui->root, &ui->ctx, bounds);
    }
}

static void
neogeo_sprite_list_overlayBodyRender(e9ui_component_t *self, e9ui_context_t *ctx)
{
    if (!self || !ctx || !ctx->renderer) {
        return;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!ui->windowState.open || !ui->root) {
        return;
    }
    int hadClip = SDL_RenderIsClipEnabled(ctx->renderer) ? 1 : 0;
    SDL_Rect prevClip = { 0, 0, 0, 0 };
    if (hadClip) {
        SDL_RenderGetClipRect(ctx->renderer, &prevClip);
    }
    SDL_Rect clip = { self->bounds.x, self->bounds.y, self->bounds.w, self->bounds.h };
    if (hadClip) {
        SDL_Rect clipped;
        if (SDL_IntersectRect(&prevClip, &clip, &clipped)) {
            SDL_RenderSetClipRect(ctx->renderer, &clipped);
        } else {
            SDL_Rect empty = { 0, 0, 0, 0 };
            SDL_RenderSetClipRect(ctx->renderer, &empty);
        }
    } else {
        SDL_RenderSetClipRect(ctx->renderer, &clip);
    }

    ui->ctx = *ctx;
    ui->ctx.window = ctx->window;
    ui->ctx.renderer = ctx->renderer;
    ui->ctx.font = e9ui->ctx.font;
    ui->ctx.winW = self->bounds.w;
    ui->ctx.winH = self->bounds.h;
    ui->ctx.mouseX = ctx->mouseX;
    ui->ctx.mouseY = ctx->mouseY;
    ui->ctx.mousePrevX = ctx->mousePrevX;
    ui->ctx.mousePrevY = ctx->mousePrevY;
    ui->ctx.focusRoot = ui->root;
    ui->ctx.focusFullscreen = NULL;
    ui->renderer = ctx->renderer;
    ui->window = ctx->window;

    if (ui->root->render) {
        ui->root->render(ui->root, &ui->ctx);
    }
    if (hadClip) {
        SDL_RenderSetClipRect(ctx->renderer, &prevClip);
    } else {
        SDL_RenderSetClipRect(ctx->renderer, NULL);
    }
}

static void
neogeo_sprite_list_overlayBodyDtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    (void)ctx;
    if (!self || !self->state) {
        return;
    }
    alloc_free(self->state);
    self->state = NULL;
}

static e9ui_component_t *
neogeo_sprite_list_makeOverlayBodyHost(void)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!ui->root) {
        return NULL;
    }
    e9ui_component_t *host = (e9ui_component_t *)alloc_calloc(1, sizeof(*host));
    neogeo_sprite_list_overlay_body_state_t *st =
        (neogeo_sprite_list_overlay_body_state_t *)alloc_calloc(1, sizeof(*st));
    if (!host || !st) {
        alloc_free(host);
        alloc_free(st);
        return NULL;
    }
    host->name = "neogeo_sprite_list_overlay_body";
    host->state = st;
    host->layout = neogeo_sprite_list_overlayBodyLayout;
    host->render = neogeo_sprite_list_overlayBodyRender;
    host->dtor = neogeo_sprite_list_overlayBodyDtor;
    e9ui_child_add(host, ui->root, alloc_strdup("neogeo_sprite_list_root"));
    return host;
}

static void
neogeo_sprite_list_overlayWindowCloseRequested(e9ui_window_t *window, void *user)
{
    (void)window;
    (void)user;
    neogeo_sprite_list_toggle();
}

void
neogeo_sprite_list_toggle(void)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!ui->windowState.open) {
        ui->windowState.windowHost = e9ui_windowCreate(neogeo_sprite_list_windowBackend());
        if (!ui->windowState.windowHost) {
            return;
        }
        ui->ctx = e9ui->ctx;
        ui->root = neogeo_sprite_list_buildRoot();
        e9ui_component_t *overlayBodyHost = neogeo_sprite_list_makeOverlayBodyHost();
        if (!ui->root || !overlayBodyHost) {
            if (ui->windowState.windowHost) {
                e9ui_windowDestroy(ui->windowState.windowHost);
                ui->windowState.windowHost = NULL;
            }
            ui->root = NULL;
            ui->filterRoot = NULL;
            ui->listScroll = NULL;
            ui->listCanvas = NULL;
            return;
        }
        e9ui_rect_t rect = e9ui_windowResolveStateOpenRect(&e9ui->ctx,
                                                           neogeo_sprite_list_windowDefaultRect(&e9ui->ctx),
                                                           &ui->windowState);
        e9ui_windowOpen(ui->windowState.windowHost,
                        "SPRITE LIST",
                        rect,
                        overlayBodyHost,
                        neogeo_sprite_list_overlayWindowCloseRequested,
                        NULL,
                        &e9ui->ctx);
        ui->window = e9ui->ctx.window;
        ui->renderer = e9ui->ctx.renderer;
        ui->windowState.open = 1;
        ui->filterCacheDirty = 1;
    } else {
        (void)e9ui_windowCaptureStateRectSnapshot(&ui->windowState, &e9ui->ctx);
        config_saveConfig();
        e9ui_text_cache_clearRenderer(ui->renderer);
        if (ui->windowState.windowHost) {
            e9ui_windowDestroy(ui->windowState.windowHost);
            ui->windowState.windowHost = NULL;
        }
        ui->root = NULL;
        ui->filterRoot = NULL;
        ui->listScroll = NULL;
        ui->listCanvas = NULL;
        for (int i = 0; i < NEOGEO_SPRITE_LIST_FILTER_COUNT; ++i) {
            ui->filterTextboxes[i] = NULL;
        }
        ui->activeOnlyCheckbox = NULL;
        ui->highlightChainCheckbox = NULL;
        ui->window = NULL;
        ui->renderer = NULL;
        ui->windowState.open = 0;
        neogeo_sprite_debug_setSelection(-1, -1, 0);
    }
}

int
neogeo_sprite_list_isOpen(void)
{
    return neogeo_sprite_list_state.windowState.open ? 1 : 0;
}

void
neogeo_sprite_list_render(const e9k_debug_sprite_state_t *st)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (st) {
        ui->lastState = *st;
        ui->hasLastState = 1;
        if (neogeo_sprite_list_decodeRows(ui, st) && ui->selectedIndex >= 0) {
            neogeo_sprite_list_applySelection(ui);
        }
    }
    if (!ui->windowState.open) {
        return;
    }
    if (e9ui_windowCaptureStateRectChanged(&ui->windowState, &e9ui->ctx)) {
        config_saveConfig();
    }
}

void
neogeo_sprite_list_persistConfig(FILE *file)
{
    if (!file) {
        return;
    }
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    e9ui_windowPersistStateRect(file, "comp.sprite_list", &ui->windowState, &e9ui->ctx);
    fprintf(file, "comp.sprite_list.active_only=%d\n", ui->activeOnly ? 1 : 0);
    fprintf(file, "comp.sprite_list.highlight_chain=%d\n", ui->highlightChain ? 1 : 0);
}

int
neogeo_sprite_list_loadConfigProperty(const char *prop, const char *value)
{
    neogeo_sprite_list_state_t *ui = &neogeo_sprite_list_state;
    if (!prop || !value) {
        return 0;
    }
    int intValue = 0;
    if (strcmp(prop, "win_x") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winX = intValue;
    } else if (strcmp(prop, "win_y") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winY = intValue;
    } else if (strcmp(prop, "win_w") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winW = intValue;
    } else if (strcmp(prop, "win_h") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->windowState.winH = intValue;
    } else if (strcmp(prop, "active_only") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->activeOnly = intValue ? 1 : 0;
        ui->filterCacheDirty = 1;
    } else if (strcmp(prop, "highlight_chain") == 0) {
        if (!neogeo_sprite_list_parseInt(value, &intValue)) {
            return 0;
        }
        ui->highlightChain = intValue ? 1 : 0;
    } else {
        return 0;
    }
    ui->windowState.winHasSaved =
        e9ui_windowHasSavedPosition(ui->windowState.winX, ui->windowState.winY);
    return 1;
}
