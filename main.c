/*
 * Build tool. Check the manpage for the buildfile spec.
 *
 * Copyright (C) 2022 bellrise
 */
#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <ftw.h>

/* Version integer. This is shown when -v is passed. */
#define BUILD_VERSION   4

/* Name of the buildfile. */
#define BUILD_FILE      "buildfile"

/* Default values (spec compliant). */
#define BUILD_DIR       "builddir"
#define BUILD_OUT       "program"
#define BUILD_CC        "cc"

#define STRSIZE         128

#define unused          __attribute__((unused))


struct str_list
{
    char **strs;
    size_t size;
};

struct config
{
    struct str_list sources;
    struct str_list include;
    struct str_list flags;
    char *buildfile;
    char *builddir;
    char *cc;
    char *out;
    bool show_cmds;
    bool show_debug;
    bool only_setup;
    bool user_sources;
};

int parse_buildfile(struct config *config);
void parse_sources(struct str_list *sources, char *str);
void resolve_paths(struct config *config);
void config_free(struct config *config);
void config_dump(struct config *config);
void str_list_free(struct str_list *list);
void str_list_append(struct str_list *list, char *str);
struct str_list find(char type, char *name);
void compile(struct config *config);
void usage();
char *lstrip(char *str);
int iswhitespace(char c);
size_t wordlen(char *word);
void splitstr(struct str_list *list, char *str);
void strreplace(char *str, char from, char to);
void removedir(char *path);


/* Functions */

int main(int argc, char **argv)
{
    struct config config = {0};

    argc--;
    argv++;

    /* Parse flags */

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;

        /* Make life easier */
        if (strcmp(argv[i], "--help") == 0)
            usage();

        switch (argv[i][1]) {
            case 'c':
                config.show_cmds = true;
                break;
            case 'd':
                config.show_debug = true;
                break;
            case 'f':
                if (i + 1 >= argc) {
                    printf("build: missing argument for -f [file]\n");
                    return 1;
                }
                config.buildfile = strdup(argv[++i]);
                break;
            case 'h':
                usage();
                break;
            case 's':
                config.only_setup = true;
                break;
            case 'v':
                printf("%d\n", BUILD_VERSION);
                exit(0);
                break;
            default:
                printf("build: unknown flag '-%c'\n", argv[i][1]);
        }
    }

    resolve_paths(&config);

    if (parse_buildfile(&config) == 1) {
        printf("build: %s not found\n", config.buildfile);
        return 1;
    }

    if (config.show_debug)
        config_dump(&config);

    /* Only compile when any sources are present and the user did not specify
       the -s flag. */
    if (!config.only_setup && config.sources.size)
        compile(&config);

    config_free(&config);
}

int parse_buildfile(struct config *config)
{
    char *buf, *tmpstr;
    FILE *buildfile;
    size_t len;

    buildfile = fopen(config->buildfile, "r");
    if (!buildfile)
        return 1;

    buf = calloc(BUFSIZ, 1);

    while (fgets(buf, BUFSIZ, buildfile)) {
        /* Skip empty lines & lines with hashes. */
        if (iswhitespace(*buf) || *buf == '\n' || *buf == '#')
            continue;

        for (size_t i = 0; i < BUFSIZ; i++) {
            if (buf[i] == '\n') {
                buf[i] = 0;
                break;
            }
        }

        /* We want to end the string right after the keyword, so libc functions
           will only take the keyword into consideration. */
        len = wordlen(buf);
        buf[len] = 0;

        if (strcmp(buf, "cc") == 0) {
            config->cc = lstrip(buf + len + 1);
        } else if (strcmp(buf, "src") == 0) {
            config->user_sources = true;
            tmpstr = lstrip(buf + len + 1);
            parse_sources(&config->sources, tmpstr);
            free(tmpstr);
        } else if (strcmp(buf, "out") == 0) {
            config->out = lstrip(buf + len + 1);
        } else if (strcmp(buf, "builddir") == 0) {
            config->builddir = lstrip(buf + len + 1);
        } else if (strcmp(buf, "flags") == 0) {
            tmpstr = lstrip(buf + len + 1);
            str_list_append(&config->flags, tmpstr);
            free(tmpstr);
        } else if (strcmp(buf, "include") == 0) {
            splitstr(&config->include, buf + len + 1);
        }
    }

    /* Provide default values if missing. */

    if (!config->cc) {
        config->cc = strdup(BUILD_CC);
    } if (!config->user_sources) {
        config->sources = find('f', "*.c");
    } if (!config->out) {
        config->out = strdup(BUILD_OUT);
    } if (!config->builddir) {
        config->builddir = strdup(BUILD_DIR);
    } if (!config->include.size) {
        config->include = find('d', "inc*");
    }

    free(buf);
    fclose(buildfile);

    return 0;
}

void compile(struct config *config)
{
    struct str_list original_paths = {0};
    size_t cmdsize = 0, srcsize = 0;
    struct stat st = {0};
    char strbuf[STRSIZE];
    char *cmd;

    /* Calculate the command size. */

    cmdsize += strlen(config->cc) + 1;
    for (size_t i = 0; i < config->flags.size; i++)
        cmdsize += strlen(config->flags.strs[i]) + 1;
    for (size_t i = 0; i < config->include.size; i++)
        cmdsize += strlen(config->include.strs[i]) + 4;

    /* Add an additional 128 bytes for the file name */

    cmdsize += 128;

    /* Create the build directory for the objects */

    if (stat(config->builddir, &st) == -1)
        mkdir(config->builddir, 0700);

    /* Produce system() calls */

    cmd = calloc(cmdsize, 1);
    for (size_t i = 0; i < config->sources.size; i++) {
        srcsize += strlen(config->sources.strs[i]) + 4;

        /* Preserve the original file name, because we want to replace the
           slashes in the filenames with dollar signs so we can just dump
           them into the builddir. */
        str_list_append(&original_paths, config->sources.strs[i]);

        strreplace(config->sources.strs[i], '/', '-');

        snprintf(cmd, cmdsize, "%s -c -o %s/%s.o %s", config->cc,
                config->builddir, config->sources.strs[i],
                original_paths.strs[i]);

        for (size_t i = 0; i < config->flags.size; i++) {
            strcat(cmd, " ");
            strcat(cmd, config->flags.strs[i]);
        }

        for (size_t i = 0; i < config->include.size; i++) {
            strcat(cmd, " -I");
            strcat(cmd, config->include.strs[i]);
        }

        if (config->show_cmds)
            printf("%s\n", cmd);
        system(cmd);
    }

    /* Link it all */

    free(cmd);
    cmd = calloc(cmdsize + srcsize, 1);
    snprintf(cmd, cmdsize, "%s -o %s", config->cc, config->out);

    for (size_t i = 0; i < config->flags.size; i++) {
        strcat(cmd, " ");
        strcat(cmd, config->flags.strs[i]);
    }

    for (size_t i = 0; i < config->include.size; i++) {
        strcat(cmd, " -I");
        strcat(cmd, config->include.strs[i]);
    }

    for (size_t i = 0; i < config->sources.size; i++) {
        snprintf(strbuf, STRSIZE, " %s/%s.o", config->builddir,
                config->sources.strs[i]);
        strcat(cmd, strbuf);
    }

    if (config->show_cmds)
        printf("%s\n", cmd);
    system(cmd);

    removedir(config->builddir);
    str_list_free(&original_paths);
    free(cmd);
}

void resolve_paths(struct config *config)
{
    /* If the user had not defined a buildfile path, we choose the default. */
    if (!config->buildfile) {
        config->buildfile = strdup(BUILD_FILE);
        return;
    }

    /* Check if the user specified a different buildfile location. If that is
       the case, we chdir() to that location in order to work with scripts.
       If we changed our directory, our buildfile also changes. */
    char *path = strdup(config->buildfile);
    char *basepath = strdup(config->buildfile);
    char *dirpath = dirname(path);
    if (strcmp(dirpath, ".") != 0) {
        chdir(dirpath);
        free(config->buildfile);
        config->buildfile = strdup(basename(basepath));
    }

    free(basepath);
    free(path);
}

struct str_list find(char type, char *name)
{
    struct str_list files = {0};
    char cmd[STRSIZE];
    FILE *res;

    /* popen a find command and read the result into the str_list */

    snprintf(cmd, STRSIZE, "find -type %c -name '%s'", type, name);
    if (!(res = popen(cmd, "r"))) {
        printf("build: failed to popen find proc\n");
        exit(1);
    }

    while (fgets(cmd, STRSIZE, res)) {
        for (size_t i = 0; i < STRSIZE; i++) {
            if (cmd[i] == '\n') {
                cmd[i] = 0;
                break;
            }
        }

        /* Without the +2, the cmd would contain the starting "./" location,
           which we don't want. */
        str_list_append(&files, cmd + 2);
    }

    pclose(res);
    return files;
}

void config_free(struct config *config)
{
    str_list_free(&config->sources);
    str_list_free(&config->include);
    str_list_free(&config->flags);
    free(config->buildfile);
    free(config->builddir);
    free(config->out);
    free(config->cc);
}

void config_dump(struct config *config)
{
    printf("cc:        %s\nbuildfile: %s\nbuilddir:  %s\nout:       %s\n",
        config->cc, config->buildfile, config->builddir, config->out);

    printf("sources:\n");
    for (size_t i = 0; i < config->sources.size; i++)
        printf("  %s\n", config->sources.strs[i]);

    printf("flags:\n");
    for (size_t i = 0; i < config->flags.size; i++)
        printf("  %s\n", config->flags.strs[i]);

    printf("include:\n");
    for (size_t i = 0; i < config->include.size; i++)
        printf("  %s\n", config->include.strs[i]);
}

void str_list_free(struct str_list *list)
{
    for (size_t i = 0; i < list->size; i++)
        free(list->strs[i]);
    free(list->strs);
}

void str_list_append(struct str_list *list, char *str)
{
    list->strs = realloc(list->strs, sizeof(char *) * (list->size + 1));
    list->strs[list->size++] = strdup(str);
}

void usage()
{
    printf(
        "usage: build [-cdfhsv] [target]\n"
        "Minimal build system\n\n"
        "  -c           output commands to stdout\n"
        "  -d           display debug information\n"
        "  -f <file>    path to a different buildfile\n"
        "  -h           show this page\n"
        "  -s           only setup, do not start compiling\n"
        "  -v           show the version number\n"
    );
    exit(0);
}

void parse_sources(struct str_list *sources, char *str)
{
    struct str_list tmp_list = {0};
    struct str_list found_sources;

    /* Split the string and then read each file. */
    splitstr(&tmp_list, str);
    for (size_t i = 0; i < tmp_list.size; i++) {
        if (tmp_list.strs[i][0] != '*') {
            str_list_append(sources, tmp_list.strs[i]);
            continue;
        }

        /* Use find() for the wildcards. */
        found_sources = find('f', tmp_list.strs[i]);
        for (size_t j = 0; j < found_sources.size; j++)
            str_list_append(sources, found_sources.strs[j]);
        str_list_free(&found_sources);
    }

    str_list_free(&tmp_list);
}

char *lstrip(char *str)
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

int iswhitespace(char c)
{
    /* Returns true if c is a space or tab. */
    return c == ' ' || c == '\t';
}

size_t wordlen(char *word)
{
    /* Works like strlen, but instead of the \0 byte it looks for
       a whitespace character like a space or tab. */
    size_t len = 0;
    char c;

    c = word[len];
    while (c != '\0' && !iswhitespace(c)) {
        len++;
        c = word[len];
    }
    return len;
}

void splitstr(struct str_list *list, char *str)
{
    /* Split the string. */
    size_t len = strlen(str), offset, wlen;
    char tmpstr[STRSIZE];

    for (size_t i = 0; i < len; i++) {
        offset = i;

        while (iswhitespace(str[offset++])) {
            /* Empty string! We cannot split this string. */
            if (offset >= len)
                return;
        }

        if (offset != i)
            i = offset - 1;

        /* Extract a word. */
        wlen = wordlen(str + i);
        strncpy(tmpstr, str + i, wlen);
        tmpstr[wlen] = 0;

        str_list_append(list, tmpstr);

        i += wlen - 1;
    }
}

void strreplace(char *str, char from, char to)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == from)
            str[i] = to;
    }
}

static int unlink_callback(const char *p, const struct stat *unused sb,
        int unused tf, struct FTW *unused ftwbuf)
{
    return remove(p);
}

void removedir(char *path)
{
    nftw(path, unlink_callback, 64, FTW_DEPTH | FTW_PHYS);
}
