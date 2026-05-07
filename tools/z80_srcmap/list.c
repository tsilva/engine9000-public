/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdlib.h>

#include "list.h"

static void *
list_alloc(size_t size)
{
    return malloc(size);
}

void *
list_get(list_t *list, int index)
{
    int i = 0;

    while (list && i != index) {
        list = list->next;
        i++;
    }
    if (list && i == index) {
        return list->data;
    }
    return NULL;
}

int
list_count(list_t *list)
{
    int count = 0;

    while (list) {
        count++;
        list = list->next;
    }
    return count;
}

list_t *
list_last(list_t *head)
{
    if (!head) {
        return NULL;
    }
    while (head->next) {
        head = head->next;
    }
    return head;
}

void
list_free(list_t **listPtr, int freeData)
{
    list_t *list;

    if (!listPtr) {
        return;
    }
    list = *listPtr;
    while (list) {
        list_t *ptr = list;
        list = list->next;
        if (freeData) {
            free(ptr->data);
        }
        free(ptr);
    }
    *listPtr = NULL;
}

void
list_append(list_t **listPtr, void *ptr)
{
    list_t *list;
    list_t *node;

    if (!listPtr) {
        return;
    }
    node = (list_t *)list_alloc(sizeof(*node));
    node->data = ptr;
    node->next = NULL;
    list = *listPtr;
    if (!list) {
        *listPtr = node;
        return;
    }
    while (list->next) {
        list = list->next;
    }
    list->next = node;
}

void
list_appendTail(list_t **listPtr, list_t **tailPtr, void *ptr)
{
    list_t *node;

    if (!listPtr || !tailPtr) {
        return;
    }
    node = (list_t *)list_alloc(sizeof(*node));
    node->data = ptr;
    node->next = NULL;
    if (*tailPtr) {
        (*tailPtr)->next = node;
    } else {
        *listPtr = node;
    }
    *tailPtr = node;
}

void
list_remove(list_t **listPtr, void *item, int freeData)
{
    list_t *list;
    list_t *prev = NULL;

    if (!listPtr) {
        return;
    }
    list = *listPtr;
    while (list && list->data != item) {
        prev = list;
        list = list->next;
    }
    if (!list) {
        return;
    }
    if (prev) {
        prev->next = list->next;
    } else {
        *listPtr = list->next;
    }
    if (freeData) {
        free(list->data);
    }
    free(list);
}
