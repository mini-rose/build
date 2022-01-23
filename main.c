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
#define BUILD_CC        "c99"

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
    struct str_list flags;
    char *buildfile;
    char *builddir;
    char *cc;
    char *out;
    bool explain;
    bool only_setup;
    bool user_sources;
};

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

int parse_buildfile(struct config *config);
void parse_sources(struct str_list *sources);
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
            case 'e':
                config.explain = true;
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

    /* Only compile when any sources are present and the user did not specify
       the -s flag. */
    if (!config.only_setup && config.sources.size)
        compile(&config);

    config_free(&config);
}

int parse_buildfile(struct config *config)
{
    char *buf, *val;
    FILE *buildfile;
    size_t len;

    /* Set up config fields. */
    const size_t nconfig_fields = 5;
    const struct config_field config_fields[] = {
        {"cc", FIELD_STR, &config->cc, BUILD_CC},
        {"src", FIELD_STRLIST, &config->sources, NULL},
        {"flags", FIELD_STRLIST, &config->flags, NULL},
        {"out", FIELD_STR, &config->out, BUILD_OUT},
        {"builddir", FIELD_STR, &config->builddir, BUILD_DIR},
    };

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
        val = lstrip(buf + len + 1);

        /* Use the config_fields table to assign values. */

        for (size_t i = 0; i < nconfig_fields; i++) {
            if (strcmp(buf, config_fields[i].name) != 0)
                continue;

            if (config_fields[i].type == FIELD_STR) {
                free(* (char **) config_fields[i].val);
                * (char **) config_fields[i].val = strdup(val);
            }

            if (config_fields[i].type == FIELD_STRLIST) {
                /* If the user modified the src option, we no longer want to
                   get the default wildcard, because the user may want an empty
                   src option. */
                if (strcmp(config_fields[i].name, "src") == 0)
                    config->user_sources = true;

                splitstr(config_fields[i].val, val);
            }
        }

        free(val);
    }

    parse_sources(&config->sources);

    /* Set defualt strings if missing */

    for (size_t i = 0; i < nconfig_fields; i++) {
        if (config_fields[i].type != FIELD_STR
            || * (char **) config_fields[i].val != NULL) {
            continue;
        }

        * (char **) config_fields[i].val
            = strdup(config_fields[i].default_val);
    }

    if (!config->user_sources)
        config->sources = find('f', "*.c");

    free(buf);
    fclose(buildfile);

    if (config->explain) {
        printf("Parsing the buildfile returned:\n");
        config_dump(config);
    }

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

        if (config->explain)
            printf("running: %s\n", cmd);
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

    for (size_t i = 0; i < config->sources.size; i++) {
        snprintf(strbuf, STRSIZE, " %s/%s.o", config->builddir,
                config->sources.strs[i]);
        strcat(cmd, strbuf);
    }

    if (config->explain)
        printf("linking: %s\n", cmd);
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
        "  -e           explain what is going on\n"
        "  -f <file>    path to a different buildfile\n"
        "  -h           show this page\n"
        "  -s           only setup, do not start compiling\n"
        "  -v           show the version number\n"
    );
    exit(0);
}

void parse_sources(struct str_list *sources)
{
    /* Expand any wildcards and add them back into the sources. */
    struct str_list expanded_sources;
    size_t original_nsources;

    original_nsources = sources->size;

    for (size_t i = 0; i < original_nsources; i++) {
        if (!strchr(sources->strs[i], '*'))
            continue;

        expanded_sources = find('f', sources->strs[i]);
        if (!expanded_sources.size)
            continue;

        /* Replace the wildcard string with the first found file. */
        free(sources->strs[i]);
        sources->strs[i] = strdup(expanded_sources.strs[0]);
        for (size_t j = 1; j < expanded_sources.size; j++)
            str_list_append(sources, expanded_sources.strs[j]);

        str_list_free(&expanded_sources);
    }
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
