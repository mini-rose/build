/* strings.c - string operations
   Copyright (C) 2022 bellrise */

#include "build.h"


size_t wordlen(char *word)
{
    /* Works like strlen, but instead of the \0 byte it looks for
       a whitespace character like a space or tab. */
    size_t len = 0;
    char c;

    c = word[len];
    while (c && !iswhitespace(c))
        c = word[++len];
    return len;
}

size_t linelen(char *line)
{
    size_t len = 0;
    char c;

    c = line[len];
    while (c && c != '\n')
        c = line[++len];
    return len;
}

char *strlstrip(char *str)
{
    /* Strip the string and return a new malloc'ed string. */
    size_t start = 0, len = strlen(str);
    char *copied;

    while (iswhitespace(str[start++])) {
        /* Empty string! */
        if (start >= len) {
            copied = malloc(1);
            copied[0] = 0;
            return copied;
        }
    }

    if (start)
        start--;

    copied = malloc(len - start + 1);
    strncpy(copied, str + start, len - start);
    copied[len - start] = 0;

    return copied;
}

int strsplit(struct strlist *list, char *str)
{
    size_t len = strlen(str), offset, wlen;
    char tmpstr[SMALLBUFSIZ];
    int added = 0;

    for (size_t i = 0; i < len; i++) {
        offset = i;

        while (iswhitespace(str[offset++])) {
            /* Empty string! We cannot split this string. */
            if (offset >= len)
                return added;
        }

        if (offset != i)
            i = offset - 1;

        /* Extract a word. */
        wlen = wordlen(str + i);
        strncpy(tmpstr, str + i, wlen);
        tmpstr[wlen] = 0;

        strlist_append(list, tmpstr);
        added++;

        i += wlen - 1;
    }

    return added;
}

int strreplace(char *str, char from, char to)
{
    size_t len = strlen(str);
    int replaced = 0;

    for (size_t i = 0; i < len; i++) {
        if (str[i] == from) {
            str[i] = to;
            replaced++;
        }
    }

    return replaced;
}

bool iswhitespace(char c)
{
    return c == ' ' || c == '\t';
}
