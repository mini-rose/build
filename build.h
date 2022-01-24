/* Build tool header.
   Copyright (C) 2022 bellrise */

#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <ftw.h>

/* Version integer. This is shown when -v is passed. */
#define BUILD_VERSION   6

/* Name of the buildfile. */
#define BUILD_FILE      "buildfile"

/* Default values (spec compliant). */
#define BUILD_DIR       "builddir"
#define BUILD_OUT       "program"
#define BUILD_CC        "c99"

#define SMALLBUFSIZ     128
#define INVALID_INDEX   ((size_t) -1)

/* Exit status. */
                                    /* normal exit */
#define EXIT_ARG        1           /* missing command line argument */
#define EXIT_BUILDFILE  2           /* buildfile not found */
#define EXIT_POPEN      3           /* popen failed */
#define EXIT_TARGET     4           /* unknown target */


struct strlist
{
    char **strs;
    size_t size;
};

struct target
{
    char *name;
    char *cmd;

    /* Both strings are actually allocated here, so instead of 3 there is
       only a single allocation. The `name` and `cmd` pointer point into this
       array, which contains null-terminated string. */
    char _data[];
};

/* All strings are stdup'ed into here, so they can be free'd. */
struct config
{
    struct strlist sources;         /* src */
    struct strlist flags;           /* flags */
    char *buildfile;                /* -f */
    char *builddir;                 /* builddir */
    char *cc;                       /* cc */
    char *out;                      /* out */
    bool explain;                   /* -e */
    bool only_setup;                /* -s */
    bool user_sources;
    struct strlist called_targets;
    struct target **targets;
    size_t ntargets;
};

/* Type of the config field. */
enum field_type_e
{
    FIELD_STR,
    FIELD_STRLIST
};

/* We can represent each modyfiable config field as a name and the type
   of the value. */
struct config_field
{
    const char *name;
    enum field_type_e type;
    void *val;
    const char *default_val;
};

/* fs.c */

/* Expands all wildcards in the `filenames` array and puts the expanded values
   back into the string list. */
void expand_wildcards(struct strlist *filenames);

/* Resolve the buildfile path the user provided with -f. If the buildfile is in
   some other directory, we first need to chdir() there. */
void resolve_buildpath(struct config *config);

/* If a filename begins with an "!", it and any matching filenames will be
   removed from the filename list. */
void remove_excluded(struct strlist *filenames);

/* Put the filenames into `output` from "find . -type `type` -name `name`."
   Returns the amount of files found. */
int find(struct strlist *output, char type, char *name);

/* Recursively remove the directory at the given path. Same as rm -rf `path`. */
void removedir(char *path);


/* buildfile.c */

/* Parse the buildfile. Name and data are pointed by `config`. */
int parse_buildfile(struct config *config);


/* config.c */

void config_free(struct config *config);
void config_dump(struct config *config);

/* Add a target to the config. Both strings are copied into a new slot. */
void config_add_target(struct config *config, char *name, char *cmd);

/* Find the target of the selected `name`, and returns the index of the target
   if it's found. Otherwise, returns INVALID_INDEX. */
size_t config_find_target(struct config *config, char *name);


/* strlist.c */

void strlist_free(struct strlist *list);
void strlist_append(struct strlist *list, char *str);

/* Returns true if `str` can be found in the string list. */
bool strlist_contains(struct strlist *list, char *str);

/* Replaces the string at the given index with another string. The previous
   string is free'd, and the new one is copied. Returns 1 if index is out of
   bounds. */
int strlist_replace(struct strlist *list, char *str, size_t index);


/* strings.c */

/* Returns true if `c` is a space or tab. */
bool iswhitespace(char c);

/* Works like strlen, but instead of the null terminator it searches for the
   closest whitespace. Stops at a null byte. */
size_t wordlen(char *word);

/* Return a copy of the lstripped string. */
char *strlstrip(char *str);

/* Splits the string at whitespaces. Returns the amount of strings appended. */
int strsplit(struct strlist *list, char *str);

/* Replaces `from` chars to `to` chars. Returns the amount of chars replaced. */
int strreplace(char *str, char from, char to);


/* compile.c */

void compile(struct config *config);


/* main.c */

void usage();
