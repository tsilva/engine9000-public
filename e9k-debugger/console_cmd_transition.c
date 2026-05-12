/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include "console_cmd_internal.h"

#include <stddef.h>
#include <strings.h>

#include "config.h"
#include "debugger.h"
#include "transition.h"
#include "alloc.h"

#include <string.h>

typedef struct console_cmd_transition_completion {
    char **items;
    int count;
    int cap;
} console_cmd_transition_completion_t;

static void
console_cmd_transition_completionAdd(console_cmd_transition_completion_t *list, const char *s)
{
    if (!list || !s || !*s) {
        return;
    }
    if (list->count >= list->cap) {
        int next = list->cap ? list->cap * 2 : 32;
        char **tmp = (char**)alloc_realloc(list->items, (size_t)next * sizeof(char*));
        if (!tmp) {
            return;
        }
        list->items = tmp;
        list->cap = next;
    }
    size_t len = strlen(s);
    char *dup = (char*)alloc_alloc(len + 1);
    if (!dup) {
        return;
    }
    memcpy(dup, s, len + 1);
    list->items[list->count++] = dup;
}

int
console_cmd_transition_command(int argc, char **argv)
{
    if (argc < 2) {
        debug_printf("transition: %s\n", transition_modeName(e9ui->transition.mode));
        debug_printf("Usage: transition <slide|explode|doom|flip|rbar|random|cycle|none>\n");
        return 1;
    }
    e9k_transition_mode_t mode = e9k_transition_none;
    if (!transition_parseMode(argv[1], &mode)) {
        debug_error("transition: unknown mode '%s'", argv[1]);
        return 0;
    }
    e9ui->transition.mode = mode;
    e9ui->transition.fullscreenModeSet = 0;
    config_saveConfig();
    debug_printf("transition: %s\n", transition_modeName(e9ui->transition.mode));
    return 1;
}

int
console_cmd_transition_complete(const char *prefix, char ***outList, int *outCount)
{
    const char *options[] = { "slide", "explode", "doom", "flip", "rbar", "random", "cycle", "none" };
    console_cmd_transition_completion_t list = {0};
    for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        const char *opt = options[i];
        if (!prefix || !*prefix) {
            console_cmd_transition_completionAdd(&list, opt);
        } else if (strncasecmp(opt, prefix, strlen(prefix)) == 0) {
            console_cmd_transition_completionAdd(&list, opt);
        }
    }
    *outList = list.items;
    *outCount = list.count;
    return list.count > 0;
}
