/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "e9ui.h"
#include "e9ui_data_edit.h"
#include <ctype.h>
#include <string.h>

typedef struct e9ui_data_edit_state {
    char *text;
    char *scratch;
    int cellCount;
    int maxCellCount;
    int textCap;
    int cursor;
    int selectAll;
    int frameVisible;
    int focusBorderVisible;
    e9ui_data_edit_mode_t mode;
    e9ui_data_edit_submit_cb_t submit;
    e9ui_data_edit_key_cb_t keyCb;
    void *keyUser;
    void *user;
} e9ui_data_edit_state_t;

static int
data_edit_groupDigits(const e9ui_data_edit_state_t *st)
{
    if (!st) {
        return 0;
    }
    switch (st->mode) {
    case e9ui_data_edit_mode_hex_words16:
        return 4;
    case e9ui_data_edit_mode_hex_bytes:
        return 2;
    case e9ui_data_edit_mode_ascii_fixed:
    default:
        return 1;
    }
}

static int
data_edit_hasSeparators(const e9ui_data_edit_state_t *st)
{
    if (!st) {
        return 0;
    }
    return st->mode == e9ui_data_edit_mode_ascii_fixed ? 0 : 1;
}

static int
data_edit_isSeparatorPos(const e9ui_data_edit_state_t *st, int cursor)
{
    int groupDigits = 0;

    if (!st || !data_edit_hasSeparators(st) || cursor < 0) {
        return 0;
    }
    groupDigits = data_edit_groupDigits(st);
    if (groupDigits <= 0) {
        return 0;
    }
    return (cursor % (groupDigits + 1)) == groupDigits ? 1 : 0;
}

static int
data_edit_isHexChar(char c)
{
    return isxdigit((unsigned char)c) ? 1 : 0;
}

static char
data_edit_upperHexChar(char c)
{
    if (c >= 'a' && c <= 'f') {
        return (char)('A' + (c - 'a'));
    }
    return c;
}

static int
data_edit_expectedLen(const e9ui_data_edit_state_t *st)
{
    int groupDigits = 0;

    if (!st || st->cellCount <= 0) {
        return 0;
    }
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        return st->cellCount;
    }
    groupDigits = data_edit_groupDigits(st);
    return st->cellCount * (groupDigits + 1) - 1;
}

static int
data_edit_lastEditableCursor(const e9ui_data_edit_state_t *st)
{
    int len = data_edit_expectedLen(st);

    if (!st || len <= 0) {
        return 0;
    }
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        return len - 1;
    }
    return len - 1;
}

static int
data_edit_endCursor(const e9ui_data_edit_state_t *st)
{
    int len = data_edit_expectedLen(st);

    if (!st || len <= 0) {
        return 0;
    }
    return len;
}

static int
data_edit_clampCursor(const e9ui_data_edit_state_t *st, int cursor)
{
    int end = data_edit_endCursor(st);

    if (!st) {
        return 0;
    }
    if (cursor < 0) {
        cursor = 0;
    }
    if (cursor > end) {
        cursor = end;
    }
    if (cursor == end) {
        return cursor;
    }
    if (data_edit_hasSeparators(st)) {
        if (data_edit_isSeparatorPos(st, cursor)) {
            cursor += 1;
        }
        if (cursor > end) {
            cursor = end;
        }
        if (cursor < end && data_edit_isSeparatorPos(st, cursor)) {
            cursor -= 1;
        }
    }
    if (cursor < 0) {
        cursor = 0;
    }
    return cursor;
}

static int
data_edit_prevCursor(const e9ui_data_edit_state_t *st, int cursor)
{
    int end = 0;

    if (!st) {
        return 0;
    }
    end = data_edit_endCursor(st);
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        return data_edit_clampCursor(st, cursor - 1);
    }
    if (cursor >= end) {
        return data_edit_lastEditableCursor(st);
    }
    if (cursor <= 0) {
        return 0;
    }
    cursor -= 1;
    if (data_edit_isSeparatorPos(st, cursor)) {
        cursor -= 1;
    }
    return data_edit_clampCursor(st, cursor);
}

static int
data_edit_nextCursor(const e9ui_data_edit_state_t *st, int cursor)
{
    int last = 0;
    int end = 0;

    if (!st) {
        return 0;
    }
    last = data_edit_lastEditableCursor(st);
    end = data_edit_endCursor(st);
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        if (cursor >= last) {
            return end;
        }
        return cursor + 1;
    }
    if (cursor >= last) {
        return end;
    }
    cursor += 1;
    if (data_edit_isSeparatorPos(st, cursor)) {
        cursor += 1;
    }
    return data_edit_clampCursor(st, cursor);
}

static void
data_edit_fillDefault(e9ui_data_edit_state_t *st)
{
    int len = 0;

    if (!st || !st->text) {
        return;
    }

    len = data_edit_expectedLen(st);
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        memset(st->text, ' ', (size_t)len);
    } else {
        int groupDigits = data_edit_groupDigits(st);
        for (int i = 0; i < st->cellCount; ++i) {
            int pos = i * (groupDigits + 1);
            for (int j = 0; j < groupDigits; ++j) {
                st->text[pos + j] = '0';
            }
            if (i + 1 < st->cellCount) {
                st->text[pos + groupDigits] = ' ';
            }
        }
    }
    st->text[len] = '\0';
    st->cursor = 0;
    st->selectAll = 0;
}

static void
data_edit_setTextInternal(e9ui_data_edit_state_t *st, const char *text)
{
    int len = 0;
    int out = 0;

    if (!st || !st->text) {
        return;
    }

    data_edit_fillDefault(st);
    if (!text || !*text) {
        return;
    }

    len = data_edit_expectedLen(st);
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        for (int i = 0; i < len && text[i]; ++i) {
            unsigned char c = (unsigned char)text[i];
            if (c < 32 || c > 126) {
                continue;
            }
            st->text[i] = (char)c;
        }
        return;
    }

    {
        int totalDigits = st->cellCount * data_edit_groupDigits(st);
        for (const char *p = text; *p && out < totalDigits; ++p) {
            char c = *p;
            if (!data_edit_isHexChar(c)) {
                continue;
            }
            c = data_edit_upperHexChar(c);
            st->text[out + (out / data_edit_groupDigits(st))] = c;
            out++;
        }
    }
}

static TTF_Font *
data_edit_font(e9ui_context_t *ctx)
{
    if (!ctx) {
        return NULL;
    }
    if (e9ui && e9ui->theme.text.source) {
        return e9ui->theme.text.source;
    }
    return ctx->font;
}

static void
data_edit_fillScratch(e9ui_data_edit_state_t *st, int count)
{
    int len = 0;

    if (!st || !st->scratch || !st->text) {
        return;
    }
    len = data_edit_expectedLen(st);
    if (count < 0) {
        count = 0;
    }
    if (count > len) {
        count = len;
    }
    if (count > 0) {
        memcpy(st->scratch, st->text, (size_t)count);
    }
    st->scratch[count] = '\0';
}

static void
data_edit_replaceAll(e9ui_data_edit_state_t *st)
{
    if (!st) {
        return;
    }
    data_edit_fillDefault(st);
}

static void
data_edit_applyAsciiInput(e9ui_data_edit_state_t *st, const char *text)
{
    int last = 0;

    if (!st || !text) {
        return;
    }
    if (st->selectAll) {
        data_edit_replaceAll(st);
    }
    last = data_edit_lastEditableCursor(st);
    for (const char *p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 126) {
            continue;
        }
        if (st->cursor < 0 || st->cursor > last) {
            break;
        }
        st->text[st->cursor] = (char)c;
        st->cursor = data_edit_nextCursor(st, st->cursor);
    }
    st->selectAll = 0;
}

static void
data_edit_applyHexInput(e9ui_data_edit_state_t *st, const char *text)
{
    int last = 0;

    if (!st || !text) {
        return;
    }
    if (st->selectAll) {
        data_edit_replaceAll(st);
    }
    last = data_edit_lastEditableCursor(st);
    for (const char *p = text; *p; ++p) {
        char c = *p;
        if (!data_edit_isHexChar(c)) {
            continue;
        }
        if (st->cursor < 0 || st->cursor > last) {
            break;
        }
        c = data_edit_upperHexChar(c);
        st->text[st->cursor] = c;
        st->cursor = data_edit_nextCursor(st, st->cursor);
    }
    st->selectAll = 0;
}

static void
data_edit_applyInput(e9ui_data_edit_state_t *st, const char *text)
{
    if (!st || !text || !*text) {
        return;
    }
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        data_edit_applyAsciiInput(st, text);
        return;
    }
    data_edit_applyHexInput(st, text);
}

static void
data_edit_backspace(e9ui_data_edit_state_t *st)
{
    if (!st) {
        return;
    }
    if (st->selectAll) {
        data_edit_replaceAll(st);
        return;
    }
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        if (st->cursor <= 0) {
            return;
        }
        st->cursor = data_edit_prevCursor(st, st->cursor);
        st->text[st->cursor] = ' ';
        return;
    }
    if (st->cursor <= 0) {
        st->text[0] = '0';
        return;
    }
    st->cursor = data_edit_prevCursor(st, st->cursor);
    st->text[st->cursor] = '0';
}

static void
data_edit_delete(e9ui_data_edit_state_t *st)
{
    int last = 0;

    if (!st) {
        return;
    }
    if (st->selectAll) {
        data_edit_replaceAll(st);
        return;
    }
    last = data_edit_lastEditableCursor(st);
    if (st->cursor < 0 || st->cursor > last) {
        return;
    }
    if (st->mode == e9ui_data_edit_mode_ascii_fixed) {
        st->text[st->cursor] = ' ';
        return;
    }
    st->text[st->cursor] = '0';
}

static void
data_edit_copyAll(const e9ui_data_edit_state_t *st)
{
    if (!st || !st->text) {
        return;
    }
    SDL_SetClipboardText(st->text);
}

static int
data_edit_cursorForTargetPx(e9ui_data_edit_state_t *st, TTF_Font *font, int target)
{
    int len = 0;
    int bestCursor = 0;
    int bestDist = INT_MAX;
    int fullWidth = 0;

    if (!st || !font || !st->text) {
        return 0;
    }
    len = data_edit_expectedLen(st);
    if (target <= 0) {
        return 0;
    }
    data_edit_fillScratch(st, len);
    TTF_SizeText(font, st->scratch, &fullWidth, NULL);
    if (target >= fullWidth) {
        return data_edit_endCursor(st);
    }
    for (int i = 0; i < len; ++i) {
        int startWidth = 0;
        int endWidth = 0;
        int dist = 0;

        if (data_edit_isSeparatorPos(st, i)) {
            continue;
        }
        data_edit_fillScratch(st, i);
        TTF_SizeText(font, st->scratch, &startWidth, NULL);
        data_edit_fillScratch(st, i + 1);
        TTF_SizeText(font, st->scratch, &endWidth, NULL);
        if (target >= startWidth && target < endWidth) {
            return i;
        }
        dist = startWidth - target;
        if (dist < 0) {
            dist = -dist;
        }
        if (dist < bestDist) {
            bestDist = dist;
            bestCursor = i;
        }
    }
    return data_edit_clampCursor(st, bestCursor);
}

static void
data_edit_render(e9ui_component_t *self, e9ui_context_t *ctx)
{
    e9ui_data_edit_state_t *st = NULL;
    SDL_Rect area;
    TTF_Font *font = NULL;
    int tw = 0;
    int th = 0;
    const int padPx = 8;

    if (!self || !ctx || !ctx->renderer) {
        return;
    }
    st = (e9ui_data_edit_state_t*)self->state;
    if (!st) {
        return;
    }

    area = (SDL_Rect){ self->bounds.x, self->bounds.y, self->bounds.w, self->bounds.h };
    if (st->frameVisible) {
        SDL_SetRenderDrawColor(ctx->renderer, 30, 30, 34, 255);
        SDL_RenderFillRect(ctx->renderer, &area);
        SDL_SetRenderDrawColor(ctx->renderer, 80, 80, 90, 255);
        SDL_RenderDrawRect(ctx->renderer, &area);
        if (st->focusBorderVisible && e9ui_getFocus(ctx) == self) {
            e9ui_drawFocusRingRect(ctx, area, 1);
        }
    }

    font = data_edit_font(ctx);
    if (!font || !st->text) {
        return;
    }

    if (st->selectAll && e9ui_getFocus(ctx) == self) {
        SDL_SetRenderDrawColor(ctx->renderer, 70, 120, 180, 255);
        SDL_Rect sel = {
            area.x + padPx,
            area.y + 2,
            area.w - padPx * 2,
            area.h - 4
        };
        if (sel.w > 0 && sel.h > 0) {
            SDL_RenderFillRect(ctx->renderer, &sel);
        }
    }

    SDL_Texture *tex = e9ui_text_cache_getText(ctx->renderer,
                                               font,
                                               st->text,
                                               (SDL_Color){230, 230, 230, 255},
                                               &tw,
                                               &th);
    if (tex) {
        SDL_Rect dst = { area.x + padPx, area.y + (area.h - th) / 2, tw, th };
        SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
    }

    if (e9ui_getFocus(ctx) == self) {
        int caretPx = 0;
        int lh = TTF_FontHeight(font);

        if (lh <= 0) {
            lh = 16;
        }
        data_edit_fillScratch(st, st->cursor);
        TTF_SizeText(font, st->scratch, &caretPx, NULL);
        SDL_SetRenderDrawColor(ctx->renderer, 230, 230, 230, 255);
        SDL_RenderDrawLine(ctx->renderer,
                           area.x + padPx + caretPx,
                           area.y + (area.h - lh) / 2,
                           area.x + padPx + caretPx,
                           area.y + (area.h + lh) / 2);
    }
}

static void
data_edit_layout(e9ui_component_t *self, e9ui_context_t *ctx, e9ui_rect_t bounds)
{
    (void)ctx;
    self->bounds = bounds;
}

static int
data_edit_preferredHeight(e9ui_component_t *self, e9ui_context_t *ctx, int availW)
{
    TTF_Font *font = NULL;
    int lh = 16;

    (void)self;
    (void)availW;
    font = data_edit_font(ctx);
    if (font) {
        lh = TTF_FontHeight(font);
        if (lh <= 0) {
            lh = 16;
        }
    }
    return lh + 12;
}

static void
data_edit_mouseDown(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_mouse_event_t *mouseEv)
{
    e9ui_data_edit_state_t *st = NULL;
    TTF_Font *font = NULL;
    int target = 0;

    if (!self || !ctx || !mouseEv || mouseEv->button != E9UI_MOUSE_BUTTON_LEFT) {
        return;
    }
    st = (e9ui_data_edit_state_t*)self->state;
    if (!st || !st->text) {
        return;
    }
    e9ui_setFocus(ctx, self);
    st->selectAll = 0;
    font = data_edit_font(ctx);
    if (!font) {
        return;
    }
    target = mouseEv->x - (self->bounds.x + 8);
    if (target < 0) {
        target = 0;
    }
    st->cursor = data_edit_cursorForTargetPx(st, font, target);
}

static int
data_edit_handleEvent(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev)
{
    e9ui_data_edit_state_t *st = NULL;
    int accel = 0;

    if (!self || !ctx || !ev || e9ui_getFocus(ctx) != self) {
        return 0;
    }
    st = (e9ui_data_edit_state_t*)self->state;
    if (!st) {
        return 0;
    }

    if (ev->type == SDL_TEXTINPUT) {
        data_edit_applyInput(st, ev->text.text);
        return 1;
    }
    if (ev->type != SDL_KEYDOWN) {
        return 0;
    }

    if (st->keyCb && st->keyCb(ctx, ev->key.keysym.sym, ev->key.keysym.mod, st->keyUser)) {
        return 1;
    }

    accel = ((ev->key.keysym.mod & KMOD_GUI) || (ev->key.keysym.mod & KMOD_CTRL)) ? 1 : 0;
    if (accel && ev->key.keysym.sym == SDLK_a) {
        st->selectAll = 1;
        st->cursor = data_edit_endCursor(st);
        return 1;
    }
    if (accel && ev->key.keysym.sym == SDLK_c) {
        data_edit_copyAll(st);
        return 1;
    }
    if (accel && ev->key.keysym.sym == SDLK_x) {
        data_edit_copyAll(st);
        data_edit_replaceAll(st);
        return 1;
    }
    if (accel && ev->key.keysym.sym == SDLK_v) {
        if (SDL_HasClipboardText()) {
            char *clip = SDL_GetClipboardText();
            if (clip) {
                data_edit_applyInput(st, clip);
                SDL_free(clip);
            }
        }
        return 1;
    }

    switch (ev->key.keysym.sym) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (st->submit) {
            st->submit(ctx, st->user);
        }
        return 1;
    case SDLK_LEFT:
        st->selectAll = 0;
        st->cursor = data_edit_prevCursor(st, st->cursor);
        return 1;
    case SDLK_RIGHT:
        st->selectAll = 0;
        st->cursor = data_edit_nextCursor(st, st->cursor);
        return 1;
    case SDLK_HOME:
        st->selectAll = 0;
        st->cursor = 0;
        return 1;
    case SDLK_END:
        st->selectAll = 0;
        st->cursor = data_edit_endCursor(st);
        return 1;
    case SDLK_BACKSPACE:
        data_edit_backspace(st);
        return 1;
    case SDLK_DELETE:
        data_edit_delete(st);
        return 1;
    default:
        break;
    }

    return 0;
}

static void
data_edit_dtor(e9ui_component_t *self, e9ui_context_t *ctx)
{
    e9ui_data_edit_state_t *st = NULL;

    (void)ctx;
    if (!self || !self->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)self->state;
    if (st->text) {
        alloc_free(st->text);
    }
    if (st->scratch) {
        alloc_free(st->scratch);
    }
    alloc_free(st);
    self->state = NULL;
}

e9ui_component_t *
e9ui_data_edit_make(int cellCount, e9ui_data_edit_submit_cb_t onSubmit, void *user)
{
    e9ui_component_t *comp = NULL;
    e9ui_data_edit_state_t *st = NULL;
    int textCap = 0;

    if (cellCount <= 0) {
        return NULL;
    }

    comp = (e9ui_component_t*)alloc_calloc(1, sizeof(*comp));
    st = (e9ui_data_edit_state_t*)alloc_calloc(1, sizeof(*st));
    if (!comp || !st) {
        alloc_free(comp);
        alloc_free(st);
        return NULL;
    }

    textCap = cellCount * 3 + 1;
    st->text = (char*)alloc_calloc((size_t)textCap, 1);
    st->scratch = (char*)alloc_calloc((size_t)textCap, 1);
    if (!st->text || !st->scratch) {
        if (st->text) {
            alloc_free(st->text);
        }
        if (st->scratch) {
            alloc_free(st->scratch);
        }
        alloc_free(st);
        alloc_free(comp);
        return NULL;
    }

    st->cellCount = cellCount;
    st->maxCellCount = cellCount;
    st->textCap = textCap;
    st->submit = onSubmit;
    st->user = user;
    st->mode = e9ui_data_edit_mode_hex_bytes;
    st->frameVisible = 1;
    st->focusBorderVisible = 1;
    data_edit_fillDefault(st);

    comp->name = "e9ui_data_edit";
    comp->state = st;
    comp->focusable = 1;
    comp->preferredHeight = data_edit_preferredHeight;
    comp->layout = data_edit_layout;
    comp->render = data_edit_render;
    comp->handleEvent = data_edit_handleEvent;
    comp->dtor = data_edit_dtor;
    comp->onMouseDown = data_edit_mouseDown;
    return comp;
}

void
e9ui_data_edit_setCellCount(e9ui_component_t *comp, int cellCount)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    if (cellCount <= 0) {
        cellCount = 1;
    }
    if (cellCount > st->maxCellCount) {
        cellCount = st->maxCellCount;
    }
    if (st->cellCount == cellCount) {
        return;
    }
    st->cellCount = cellCount;
    data_edit_fillDefault(st);
}

int
e9ui_data_edit_getCellCount(const e9ui_component_t *comp)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return 0;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    return st->cellCount;
}

void
e9ui_data_edit_setMode(e9ui_component_t *comp, e9ui_data_edit_mode_t mode)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    if (st->mode == mode) {
        return;
    }
    st->mode = mode;
    data_edit_fillDefault(st);
}

e9ui_data_edit_mode_t
e9ui_data_edit_getMode(const e9ui_component_t *comp)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return e9ui_data_edit_mode_hex_bytes;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    return st->mode;
}

void
e9ui_data_edit_setText(e9ui_component_t *comp, const char *text)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    data_edit_setTextInternal(st, text);
}

const char *
e9ui_data_edit_getText(const e9ui_component_t *comp)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return "";
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    return st->text ? st->text : "";
}

void
e9ui_data_edit_setCursor(e9ui_component_t *comp, int cursor)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    st->cursor = data_edit_clampCursor(st, cursor);
    st->selectAll = 0;
}

int
e9ui_data_edit_getCursor(const e9ui_component_t *comp)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return 0;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    return st->cursor;
}

void
e9ui_data_edit_selectAllExternal(e9ui_component_t *comp)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    st->selectAll = 1;
    st->cursor = data_edit_endCursor(st);
}

void
e9ui_data_edit_setKeyHandler(e9ui_component_t *comp, e9ui_data_edit_key_cb_t cb, void *user)
{
    e9ui_data_edit_state_t *st = NULL;

    if (!comp || !comp->state) {
        return;
    }
    st = (e9ui_data_edit_state_t*)comp->state;
    st->keyCb = cb;
    st->keyUser = user;
}
