/*
 * strlist.c - strlist struct operations
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"

#define STRLIST_BUMP_BY     32


void strlist_free(struct strlist *list)
{
    for (size_t i = 0; i < list->size; i++)
        free(list->strs[i]);
    free(list->strs);

    list->size = 0;
    list->strs = NULL;
}

void strlist_append(struct strlist *list, char *str)
{
    if (list->size >= list->space) {
        list->space += STRLIST_BUMP_BY;
        list->strs = realloc(list->strs, sizeof(char *) * list->space);
    }

    list->strs[list->size++] = strdup(str);
}

bool strlist_contains(struct strlist *list, char *str)
{
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->strs[i], str) == 0)
            return true;
    }

    return false;
}

int strlist_replace(struct strlist *list, char *str, size_t index)
{
    if (index >= list->size)
        return 1;

    free(list->strs[index]);
    list->strs[index] = strdup(str);
    return 0;
}
