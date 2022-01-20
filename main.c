/*
 * Build tool. Check the `help` file for the buildfile specification.
 *
 * Copyright (C) 2022 bellrise
 */
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define __USE_XOPEN_EXTENDED
#include <ftw.h>

/* Version integer. This is shown when -v is passed. */
#define BUILD_VERSION   1

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
    char *builddir;
    char *cc;
    char *out;
};

struct config parse_buildfile();
void config_free(struct config *config);
void str_list_free(struct str_list *list);
void str_list_append(struct str_list *list, char *str);
struct str_list find(char type, char *name);
void compile(struct config *config);
void usage();
char *lstrip(char *str);
int iswhitespace(char c);
size_t wordlen(char *word);
void splitstr(struct str_list *list, char *str);
void removedir(char *path);


/* Functions */

int main(int argc, char **argv)
{
    struct config config;

    argc--;
    argv++;

    /* Parse flags */

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;

        switch (argv[i][1]) {
            case 'h':
                usage();
                break;
            case 'v':
                printf("%d\n", BUILD_VERSION);
                exit(0);
                break;
            default:
                printf("build: unknown flag '-%c'\n", argv[i][1]);
        }
    }

    /* Compile */

    config = parse_buildfile();
    compile(&config);

    config_free(&config);
}

struct config parse_buildfile()
{
    struct config config = {0};
    char *buf, *tmpstr;
    FILE *buildfile;
    size_t len;

    /* Not yet supported values in the buildfile */
    config.sources  = find('f', "*.c");

    buildfile = fopen(BUILD_FILE, "r");
    if (!buildfile) {
        /* If we can't find the buildfile, set some default values. */
        config.include  = find('d', "inc*");
        config.builddir = strdup(BUILD_DIR);
        config.out      = strdup(BUILD_OUT);
        config.cc       = strdup(BUILD_CC);
        return config;
    }

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
            config.cc = lstrip(buf + len + 1);
        } else if (strcmp(buf, "out") == 0) {
            config.out = lstrip(buf + len + 1);
        } else if (strcmp(buf, "builddir") == 0) {
            config.builddir = lstrip(buf + len + 1);
        } else if (strcmp(buf, "flags") == 0) {
            tmpstr = lstrip(buf + len + 1);
            str_list_append(&config.flags, tmpstr);
            free(tmpstr);
        } else if (strcmp(buf, "include") == 0) {
            splitstr(&config.include, buf + len + 1);
        }
    }

    /* Provide default values if missing. */

    if (!config.cc) {
        config.cc = strdup(BUILD_CC);
    } if (!config.out) {
        config.out = strdup(BUILD_OUT);
    } if (!config.builddir) {
        config.builddir = strdup(BUILD_DIR);
    } if (!config.include.size) {
        config.include = find('d', "inc*");
    }

    free(buf);
    fclose(buildfile);

    return config;
}

void compile(struct config *config)
{
    struct stat st = {0};
    size_t cmdsize = 0, srcsize = 0;
    char strbuf[STRSIZE];
    char *cmd;

    /* Calculate the command size. */

    cmdsize += strlen(config->cc) + 1;
    for (size_t i = 0; i < config->flags.size; i++)
        cmdsize += strlen(config->flags.strs[i]) + 1;
    for (size_t i = 0; i < config->include.size; i++)
        cmdsize += strlen(config->include.strs[i]) + 4;

    /* Add an additional 64 bytes for the file name */

    cmdsize += 64;

    /* Create the build directory for the objects */

    if (stat(config->builddir, &st) == -1)
        mkdir(config->builddir, 0700);

    /* Produce system() calls */

    cmd = calloc(cmdsize, 1);
    for (size_t i = 0; i < config->sources.size; i++) {
        srcsize += strlen(config->sources.strs[i]) + 4;

        snprintf(cmd, cmdsize, "%s -c -o %s/%s.o %s", config->cc,
                config->builddir, config->sources.strs[i],
                config->sources.strs[i]);

        for (size_t i = 0; i < config->flags.size; i++) {
            strcat(cmd, " ");
            strcat(cmd, config->flags.strs[i]);
        }

        for (size_t i = 0; i < config->include.size; i++) {
            strcat(cmd, " -I");
            strcat(cmd, config->include.strs[i]);
        }

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

    system(cmd);

    removedir(config->builddir);
    free(cmd);
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
    free(config->builddir);
    free(config->out);
    free(config->cc);
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
        "usage: build [-hv]\n"
        "Build a C project with minimal effort.\n\n"
        "  -h       show this page\n"
        "  -v       show the version number\n"
    );
    exit(0);
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

static int unlink_callback(const char *p, const struct stat *unused sb,
        int unused tf, struct FTW *unused ftwbuf)
{
    return remove(p);
}

void removedir(char *path)
{
    nftw(path, unlink_callback, 64, FTW_DEPTH | FTW_PHYS);
}
