/*
 * build.h - main header
 * Copyright (c) 2022 mini-rose
 */

#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <ftw.h>

#if __linux__ || __APPLE__
/* Use the actual get_nprocs() function. */
# include <sys/sysinfo.h>
#else
# define get_nprocs()   8
#endif

/* The maximum amount of threads that can be launched. */
#define MAX_PROCS       64

/* Version integer. This is shown when -v is passed. */
#define BUILD_VERSION   8

#define BUILD_FILE      "buildfile"
#define BUILD_DIR       "builddir"
#define BUILD_OUT       "program"
#define BUILD_CC        "c99"

/* RSD 10/2a: provide atleast a 4095 char line buffer */
#define LINESIZE        4096

#define SMALLBUFSIZ     128
#define INVALID_INDEX   ((size_t) -1)

#define EXIT_ARG        1           /* missing command line argument */
#define EXIT_BUILDFILE  2           /* buildfile not found */
#define EXIT_POPEN      3           /* popen failed */
#define EXIT_TARGET     4           /* unknown target */
#define EXIT_THREAD     5           /* failed to create thread */


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
    int use_n_threads;              /* -j */
    struct strlist called_targets;
    struct target **targets;
    size_t ntargets;
};

enum field_type_e
{
    FIELD_STR,
    FIELD_STRLIST
};

struct config_field
{
    const char *name;
    enum field_type_e type;
    void *val;
    const char *default_val;
};

struct thread_task
{
    struct config *config;
    int tid;
    int from;
    int to;
};


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
int find(struct strlist *output, char type, char *dir, char *name);

/* Recursively remove the directory at the given path. Same as rm -rf `path`. */
void removedir(char *path);

/* Parse the buildfile. Name and data are pointed by `config`. */
int parse_buildfile(struct config *config);

void config_free(struct config *config);
void config_dump(struct config *config);

/* Add a target to the config. Both strings are copied into a new slot. */
void config_add_target(struct config *config, char *name, char *cmd);

/* Find the target of the selected `name`, and returns the index of the target
   if it's found. Otherwise, returns INVALID_INDEX. */
size_t config_find_target(struct config *config, char *name);

/* Runs the given target. If the target is not found, return 1. Otherwise
   return 0. */
int config_call_target(struct config *config, char *name);

void strlist_free(struct strlist *list);
void strlist_append(struct strlist *list, char *str);

/* Returns true if `str` can be found in the string list. */
bool strlist_contains(struct strlist *list, char *str);

/* Replaces the string at the given index with another string. The previous
   string is free'd, and the new one is copied. Returns 1 if index is out of
   bounds. */
int strlist_replace(struct strlist *list, char *str, size_t index);

/* Returns true if `c` is a space or tab. */
bool iswhitespace(char c);

/* Works like strlen, but instead of the null terminator it searches for the
   closest whitespace. Stops at a null byte. */
size_t wordlen(char *word);

/* Works like, strlen, but instead of the null terminator it searches for the
   closes newline. Stops at a null byte. */
size_t linelen(char *line);

/* Return a copy of the lstripped string. */
char *strlstrip(char *str);

/* Splits the string at whitespaces. Returns the amount of strings appended. */
int strsplit(struct strlist *list, char *str);

/* Replaces `from` chars to `to` chars. Returns the amount of chars replaced. */
int strreplace(char *str, char from, char to);

void compile(struct config *config);

void usage();
