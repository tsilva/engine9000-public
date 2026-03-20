/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#include "syntax_highlight.h"
#include "alloc.h"
#include "syntax_highlight_asm.h"

#ifdef E9K_USE_TREE_SITTER
#include "tree_sitter/api.h"
extern const TSLanguage *tree_sitter_c(void);
#endif

typedef struct syntax_highlight_line_entry {
    syntax_highlight_span_t *spans;
    int spanCount;
    int spanCap;
} syntax_highlight_line_entry_t;

typedef struct syntax_highlight_entry {
    char *path;
    char *text;
    size_t textLength;
    long long fileSize;
    long long fileMtime;
    size_t *lineStarts;
    int lineCount;
    syntax_highlight_line_entry_t *lines;
#ifdef E9K_USE_TREE_SITTER
    TSTree *tree;
#endif
} syntax_highlight_entry_t;

typedef struct syntax_highlight_state {
    syntax_highlight_entry_t *entries;
    int count;
    int cap;
#ifdef E9K_USE_TREE_SITTER
    TSParser *parser;
    TSQuery *query;
    int ready;
#endif
} syntax_highlight_state_t;

static syntax_highlight_state_t syntax_highlight_state = {0};

#ifdef E9K_USE_TREE_SITTER
static const char *syntax_highlight_querySource =
    "(comment) @comment\n"
    "(string_literal) @string\n"
    "(char_literal) @string\n"
    "(system_lib_string) @string\n"
    "(number_literal) @number\n"
    "(type_identifier) @type\n"
    "(primitive_type) @type\n"
    "(sized_type_specifier) @type\n"
    "(preproc_include) @preproc\n"
    "(preproc_def) @preproc\n"
    "(preproc_function_def) @preproc\n"
    "(preproc_call) @preproc\n"
    "\"#if\" @preproc\n"
    "\"#ifdef\" @preproc\n"
    "\"#ifndef\" @preproc\n"
    "\"#elif\" @preproc\n"
    "\"#elifdef\" @preproc\n"
    "\"#elifndef\" @preproc\n"
    "\"#else\" @preproc\n"
    "\"#endif\" @preproc\n"
    "(function_definition declarator: (function_declarator declarator: (identifier) @function))\n"
    "(call_expression function: (identifier) @function)\n";
#endif

static void
syntax_highlight_freeEntry(syntax_highlight_entry_t *entry)
{
    if (!entry) {
        return;
    }
    if (entry->path) {
        alloc_free(entry->path);
    }
    if (entry->text) {
        alloc_free(entry->text);
    }
    if (entry->lineStarts) {
        alloc_free(entry->lineStarts);
    }
    if (entry->lines) {
        for (int i = 0; i < entry->lineCount; ++i) {
            if (entry->lines[i].spans) {
                alloc_free(entry->lines[i].spans);
            }
        }
        alloc_free(entry->lines);
    }
#ifdef E9K_USE_TREE_SITTER
    if (entry->tree) {
        ts_tree_delete(entry->tree);
    }
#endif
    memset(entry, 0, sizeof(*entry));
}

static int
syntax_highlight_findEntry(const char *path)
{
    if (!path || !path[0]) {
        return -1;
    }
    for (int i = 0; i < syntax_highlight_state.count; ++i) {
        syntax_highlight_entry_t *entry = &syntax_highlight_state.entries[i];
        if (entry->path && strcmp(entry->path, path) == 0) {
            return i;
        }
    }
    return -1;
}

static int
syntax_highlight_ensureEntryCapacity(void)
{
    if (syntax_highlight_state.count < syntax_highlight_state.cap) {
        return 1;
    }
    int nextCap = syntax_highlight_state.cap ? syntax_highlight_state.cap * 2 : 8;
    syntax_highlight_entry_t *nextEntries = (syntax_highlight_entry_t*)alloc_realloc(
        syntax_highlight_state.entries, (size_t)nextCap * sizeof(syntax_highlight_entry_t));
    if (!nextEntries) {
        return 0;
    }
    for (int i = syntax_highlight_state.cap; i < nextCap; ++i) {
        memset(&nextEntries[i], 0, sizeof(nextEntries[i]));
    }
    syntax_highlight_state.entries = nextEntries;
    syntax_highlight_state.cap = nextCap;
    return 1;
}

static int
syntax_highlight_readFile(const char *path, char **outText, size_t *outLength)
{
    if (!path || !path[0] || !outText || !outLength) {
        return 0;
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    long fileSize = ftell(file);
    if (fileSize < 0) {
        fclose(file);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    char *text = (char*)alloc_alloc((size_t)fileSize + 1);
    if (!text) {
        fclose(file);
        return 0;
    }
    size_t readBytes = fread(text, 1, (size_t)fileSize, file);
    fclose(file);
    text[readBytes] = '\0';
    *outText = text;
    *outLength = readBytes;
    return 1;
}

static int
syntax_highlight_buildLineStarts(syntax_highlight_entry_t *entry)
{
    if (!entry || !entry->text) {
        return 0;
    }
    int lineCount = 1;
    for (size_t i = 0; i < entry->textLength; ++i) {
        if (entry->text[i] == '\n') {
            lineCount++;
        }
    }
    entry->lineStarts = (size_t*)alloc_alloc((size_t)lineCount * sizeof(size_t));
    if (!entry->lineStarts) {
        return 0;
    }
    entry->lineCount = lineCount;
    entry->lineStarts[0] = 0;
    int lineIndex = 1;
    for (size_t i = 0; i < entry->textLength; ++i) {
        if (entry->text[i] == '\n' && lineIndex < lineCount) {
            entry->lineStarts[lineIndex++] = i + 1;
        }
    }
    entry->lines = (syntax_highlight_line_entry_t*)alloc_calloc((size_t)lineCount,
                                                                 sizeof(syntax_highlight_line_entry_t));
    if (!entry->lines) {
        return 0;
    }
    return 1;
}

#ifdef E9K_USE_TREE_SITTER
static int
syntax_highlight_getLineIndexForByte(const syntax_highlight_entry_t *entry, uint32_t byteOffset)
{
    if (!entry || entry->lineCount <= 0) {
        return 0;
    }
    int low = 0;
    int high = entry->lineCount - 1;
    while (low <= high) {
        int mid = low + ((high - low) / 2);
        size_t start = entry->lineStarts[mid];
        size_t nextStart = (mid + 1 < entry->lineCount) ? entry->lineStarts[mid + 1] : entry->textLength + 1;
        if ((size_t)byteOffset < start) {
            high = mid - 1;
            continue;
        }
        if ((size_t)byteOffset >= nextStart) {
            low = mid + 1;
            continue;
        }
        return mid;
    }
    if (low < 0) {
        return 0;
    }
    if (low >= entry->lineCount) {
        return entry->lineCount - 1;
    }
    return low;
}

static int
syntax_highlight_addLineSpan(syntax_highlight_line_entry_t *line, int startColumn, int length,
                             syntax_highlight_kind_t kind)
{
    if (!line || length <= 0) {
        return 0;
    }
    if (line->spanCount == line->spanCap) {
        int nextCap = line->spanCap ? line->spanCap * 2 : 8;
        syntax_highlight_span_t *nextSpans = (syntax_highlight_span_t*)alloc_realloc(
            line->spans, (size_t)nextCap * sizeof(syntax_highlight_span_t));
        if (!nextSpans) {
            return 0;
        }
        line->spans = nextSpans;
        line->spanCap = nextCap;
    }
    syntax_highlight_span_t *span = &line->spans[line->spanCount++];
    span->startColumn = startColumn;
    span->length = length;
    span->kind = kind;
    return 1;
}

static int
syntax_highlight_addAsmSpan(void *user, int lineIndex, int startColumn, int length, syntax_highlight_kind_t kind)
{
    if (!user || lineIndex < 0 || startColumn < 0 || length <= 0) {
        return 0;
    }
    syntax_highlight_entry_t *entry = (syntax_highlight_entry_t*)user;
    if (!entry->lines || lineIndex >= entry->lineCount) {
        return 0;
    }
    return syntax_highlight_addLineSpan(&entry->lines[lineIndex], startColumn, length, kind);
}

static int
syntax_highlight_kindPriority(syntax_highlight_kind_t kind)
{
    switch (kind) {
        case syntax_highlight_kind_preproc:
            return 7;
        case syntax_highlight_kind_comment:
            return 6;
        case syntax_highlight_kind_string:
            return 5;
        case syntax_highlight_kind_number:
            return 4;
        case syntax_highlight_kind_type:
            return 3;
        case syntax_highlight_kind_keyword:
            return 2;
        case syntax_highlight_kind_function:
            return 1;
        default:
            return 0;
    }
}

static int
syntax_highlight_compareSpan(const void *a, const void *b)
{
    const syntax_highlight_span_t *spanA = (const syntax_highlight_span_t*)a;
    const syntax_highlight_span_t *spanB = (const syntax_highlight_span_t*)b;
    if (spanA->startColumn != spanB->startColumn) {
        return spanA->startColumn - spanB->startColumn;
    }
    if (spanA->length != spanB->length) {
        return spanB->length - spanA->length;
    }
    return syntax_highlight_kindPriority(spanB->kind) - syntax_highlight_kindPriority(spanA->kind);
}

static void
syntax_highlight_sortLineSpans(syntax_highlight_entry_t *entry)
{
    if (!entry || !entry->lines) {
        return;
    }
    for (int i = 0; i < entry->lineCount; ++i) {
        syntax_highlight_line_entry_t *line = &entry->lines[i];
        if (line->spanCount > 1) {
            qsort(line->spans, (size_t)line->spanCount, sizeof(syntax_highlight_span_t),
                  syntax_highlight_compareSpan);
        }
    }
}
#endif

#ifdef E9K_USE_TREE_SITTER
static int
syntax_highlight_mapCapture(const char *name, uint32_t length, syntax_highlight_kind_t *outKind)
{
    if (!name || !outKind) {
        return 0;
    }
    if (length == 7 && strncmp(name, "comment", 7) == 0) {
        *outKind = syntax_highlight_kind_comment;
        return 1;
    }
    if (length == 6 && strncmp(name, "string", 6) == 0) {
        *outKind = syntax_highlight_kind_string;
        return 1;
    }
    if (length == 6 && strncmp(name, "number", 6) == 0) {
        *outKind = syntax_highlight_kind_number;
        return 1;
    }
    if (length == 4 && strncmp(name, "type", 4) == 0) {
        *outKind = syntax_highlight_kind_type;
        return 1;
    }
    if (length == 7 && strncmp(name, "preproc", 7) == 0) {
        *outKind = syntax_highlight_kind_preproc;
        return 1;
    }
    if (length == 8 && strncmp(name, "function", 8) == 0) {
        *outKind = syntax_highlight_kind_function;
        return 1;
    }
    if (length == 7 && strncmp(name, "keyword", 7) == 0) {
        *outKind = syntax_highlight_kind_keyword;
        return 1;
    }
    return 0;
}

static void
syntax_highlight_addRange(syntax_highlight_entry_t *entry, uint32_t startByte, uint32_t endByte,
                          syntax_highlight_kind_t kind)
{
    if (!entry || !entry->lines || endByte <= startByte) {
        return;
    }
    int lineIndex = syntax_highlight_getLineIndexForByte(entry, startByte);
    size_t cursor = startByte;
    while (lineIndex < entry->lineCount && cursor < endByte) {
        size_t lineStart = entry->lineStarts[lineIndex];
        size_t lineEnd = entry->textLength;
        if (lineIndex + 1 < entry->lineCount) {
            lineEnd = entry->lineStarts[lineIndex + 1] - 1;
        }
        if (lineEnd < lineStart) {
            lineEnd = lineStart;
        }
        size_t segStart = cursor > lineStart ? cursor : lineStart;
        size_t segEnd = endByte < lineEnd ? endByte : lineEnd;
        if (segEnd > segStart) {
            (void)syntax_highlight_addLineSpan(&entry->lines[lineIndex],
                                               (int)(segStart - lineStart),
                                               (int)(segEnd - segStart),
                                               kind);
        }
        if (lineIndex + 1 >= entry->lineCount) {
            break;
        }
        cursor = entry->lineStarts[lineIndex + 1];
        lineIndex++;
    }
}

static int
syntax_highlight_prepareParser(void)
{
    if (syntax_highlight_state.ready) {
        return 1;
    }
    syntax_highlight_state.parser = ts_parser_new();
    if (!syntax_highlight_state.parser) {
        return 0;
    }
    if (!ts_parser_set_language(syntax_highlight_state.parser, tree_sitter_c())) {
        return 0;
    }
    uint32_t errorOffset = 0;
    TSQueryError errorType = TSQueryErrorNone;
    syntax_highlight_state.query = ts_query_new(tree_sitter_c(),
                                                syntax_highlight_querySource,
                                                (uint32_t)strlen(syntax_highlight_querySource),
                                                &errorOffset,
                                                &errorType);
    if (!syntax_highlight_state.query) {
        (void)errorOffset;
        (void)errorType;
        return 0;
    }
    syntax_highlight_state.ready = 1;
    return 1;
}

static int
syntax_highlight_buildSpans(syntax_highlight_entry_t *entry)
{
    if (!entry || !entry->text) {
        return 0;
    }
    const char *dot = entry->path ? strrchr(entry->path, '.') : NULL;
    if (dot &&
        (strcmp(dot, ".s") == 0 || strcmp(dot, ".S") == 0 ||
         strcmp(dot, ".asm") == 0 || strcmp(dot, ".ASM") == 0)) {
        return syntax_highlight_asm_buildSpans(entry->text, entry->textLength,
                                               entry->lineStarts, entry->lineCount,
                                               syntax_highlight_addAsmSpan, entry);
    }
    if (!syntax_highlight_prepareParser()) {
        return 0;
    }
    entry->tree = ts_parser_parse_string(syntax_highlight_state.parser,
                                         NULL,
                                         entry->text,
                                         (uint32_t)entry->textLength);
    if (!entry->tree) {
        return 0;
    }
    TSNode root = ts_tree_root_node(entry->tree);
    TSQueryCursor *cursor = ts_query_cursor_new();
    if (!cursor) {
        return 0;
    }
    ts_query_cursor_exec(cursor, syntax_highlight_state.query, root);
    TSQueryMatch match;
    uint32_t captureIndex = 0;
    while (ts_query_cursor_next_capture(cursor, &match, &captureIndex)) {
        if (captureIndex >= match.capture_count) {
            continue;
        }
        TSQueryCapture capture = match.captures[captureIndex];
        uint32_t nameLength = 0;
        const char *captureName = ts_query_capture_name_for_id(syntax_highlight_state.query,
                                                               capture.index,
                                                               &nameLength);
        syntax_highlight_kind_t kind = syntax_highlight_kind_normal;
        if (!syntax_highlight_mapCapture(captureName, nameLength, &kind)) {
            continue;
        }
        TSNode node = capture.node;
        uint32_t startByte = ts_node_start_byte(node);
        uint32_t endByte = ts_node_end_byte(node);
        if (endByte <= startByte) {
            continue;
        }
        syntax_highlight_addRange(entry, startByte, endByte, kind);
    }
    ts_query_cursor_delete(cursor);
    syntax_highlight_sortLineSpans(entry);
    return 1;
}
#else
static int
syntax_highlight_buildSpans(syntax_highlight_entry_t *entry)
{
    (void)entry;
    return 1;
}
#endif

static int
syntax_highlight_loadEntry(syntax_highlight_entry_t *entry, const char *path)
{
    if (!entry || !path || !path[0]) {
        return 0;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    char *text = NULL;
    size_t textLength = 0;
    if (!syntax_highlight_readFile(path, &text, &textLength)) {
        return 0;
    }
    syntax_highlight_freeEntry(entry);
    entry->path = alloc_strdup(path);
    entry->text = text;
    entry->textLength = textLength;
    entry->fileSize = (long long)st.st_size;
    entry->fileMtime = (long long)st.st_mtime;
    if (!entry->path) {
        syntax_highlight_freeEntry(entry);
        return 0;
    }
    if (!syntax_highlight_buildLineStarts(entry)) {
        syntax_highlight_freeEntry(entry);
        return 0;
    }
    if (!syntax_highlight_buildSpans(entry)) {
        syntax_highlight_freeEntry(entry);
        return 0;
    }
    return 1;
}

static int
syntax_highlight_shouldReload(const syntax_highlight_entry_t *entry, const char *path)
{
    if (!entry || !entry->path || !path) {
        return 1;
    }
    if (strcmp(entry->path, path) != 0) {
        return 1;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        return 1;
    }
    if (entry->fileSize != (long long)st.st_size) {
        return 1;
    }
    if (entry->fileMtime != (long long)st.st_mtime) {
        return 1;
    }
    return 0;
}

static syntax_highlight_entry_t *
syntax_highlight_getEntry(const char *path)
{
    int index = syntax_highlight_findEntry(path);
    if (index >= 0) {
        syntax_highlight_entry_t *entry = &syntax_highlight_state.entries[index];
        if (!syntax_highlight_shouldReload(entry, path)) {
            return entry;
        }
        if (syntax_highlight_loadEntry(entry, path)) {
            return entry;
        }
        return NULL;
    }
    if (!syntax_highlight_ensureEntryCapacity()) {
        return NULL;
    }
    syntax_highlight_entry_t *entry = &syntax_highlight_state.entries[syntax_highlight_state.count++];
    memset(entry, 0, sizeof(*entry));
    if (!syntax_highlight_loadEntry(entry, path)) {
        syntax_highlight_freeEntry(entry);
        syntax_highlight_state.count--;
        return NULL;
    }
    return entry;
}

int
syntax_highlight_getLineSpans(const char *path, int lineNumber, const syntax_highlight_span_t **outSpans,
                              int *outSpanCount)
{
    if (!outSpans || !outSpanCount || !path || !path[0] || lineNumber <= 0) {
        return 0;
    }
    *outSpans = NULL;
    *outSpanCount = 0;
    syntax_highlight_entry_t *entry = syntax_highlight_getEntry(path);
    if (!entry || lineNumber > entry->lineCount) {
        return 0;
    }
    syntax_highlight_line_entry_t *line = &entry->lines[lineNumber - 1];
    *outSpans = line->spans;
    *outSpanCount = line->spanCount;
    return 1;
}

int
syntax_highlight_getHoverExpr(const char *path, int lineNumber, int column, char *outExpr, int outCap)
{
    if (!outExpr || outCap <= 1 || !path || !path[0] || lineNumber <= 0 || column < 0) {
        return 0;
    }
    outExpr[0] = '\0';
    syntax_highlight_entry_t *entry = syntax_highlight_getEntry(path);
    if (!entry || !entry->text || lineNumber > entry->lineCount) {
        return 0;
    }
#ifdef E9K_USE_TREE_SITTER
    if (!entry->tree) {
        return 0;
    }
    TSNode root = ts_tree_root_node(entry->tree);
    TSPoint point;
    point.row = (uint32_t)(lineNumber - 1);
    point.column = (uint32_t)column;
    TSNode node = ts_node_descendant_for_point_range(root, point, point);
    while (!ts_node_is_null(node)) {
        const char *type = ts_node_type(node);
        if (type) {
            int match = 0;
            if (strcmp(type, "identifier") == 0 ||
                strcmp(type, "field_expression") == 0 ||
                strcmp(type, "subscript_expression") == 0 ||
                strcmp(type, "pointer_expression") == 0) {
                match = 1;
            }
            if (match) {
                uint32_t startByte = ts_node_start_byte(node);
                uint32_t endByte = ts_node_end_byte(node);
                if (endByte > startByte && (size_t)startByte < entry->textLength) {
                    size_t len = (size_t)(endByte - startByte);
                    if ((size_t)startByte + len > entry->textLength) {
                        len = entry->textLength - (size_t)startByte;
                    }
                    if (len >= (size_t)outCap) {
                        len = (size_t)outCap - 1;
                    }
                    memcpy(outExpr, entry->text + startByte, len);
                    outExpr[len] = '\0';
                    while (len > 0 && (outExpr[len - 1] == ' ' || outExpr[len - 1] == '\t' ||
                                       outExpr[len - 1] == '\r' || outExpr[len - 1] == '\n')) {
                        outExpr[--len] = '\0';
                    }
                    size_t start = 0;
                    while (outExpr[start] == ' ' || outExpr[start] == '\t' ||
                           outExpr[start] == '\r' || outExpr[start] == '\n') {
                        start++;
                    }
                    if (start > 0) {
                        memmove(outExpr, outExpr + start, len - start + 1);
                    }
                    if (!outExpr[0] || strchr(outExpr, '(')) {
                        return 0;
                    }
                    return 1;
                }
            }
        }
        node = ts_node_parent(node);
    }
#endif
    return 0;
}

void
syntax_highlight_shutdown(void)
{
    for (int i = 0; i < syntax_highlight_state.count; ++i) {
        syntax_highlight_freeEntry(&syntax_highlight_state.entries[i]);
    }
    if (syntax_highlight_state.entries) {
        alloc_free(syntax_highlight_state.entries);
    }
#ifdef E9K_USE_TREE_SITTER
    if (syntax_highlight_state.query) {
        ts_query_delete(syntax_highlight_state.query);
    }
    if (syntax_highlight_state.parser) {
        ts_parser_delete(syntax_highlight_state.parser);
    }
#endif
    memset(&syntax_highlight_state, 0, sizeof(syntax_highlight_state));
}
