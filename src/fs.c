/*
 * fs.c - filesystem & filename operations
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"


void expand_wildcards(struct strlist *filenames)
{
    struct strlist expanded_filenames = {0};
    char *dirp, *basep, *p_dirp, *p_basep;
    size_t nfilenames;

    nfilenames = filenames->size;
    for (size_t i = 0; i < nfilenames; i++) {
        if (!strchr(filenames->strs[i], '*'))
            continue;

        /* As per the manpage, the wildcard path needs to be split into the
           `dirname` and `basename` for `find`. */
        dirp    = strdup(filenames->strs[i]);
        basep   = strdup(filenames->strs[i]);
        p_dirp  = dirname(dirp);
        p_basep = basename(basep);

        find(&expanded_filenames, 'f', p_dirp, p_basep);
        free(dirp);
        free(basep);

        if (!expanded_filenames.size)
            continue;

        /* Replace the wildcard string with the first found file. */
        free(filenames->strs[i]);
        filenames->strs[i] = strdup(expanded_filenames.strs[0]);
        for (size_t j = 1; j < expanded_filenames.size; j++)
            strlist_append(filenames, expanded_filenames.strs[j]);

        strlist_free(&expanded_filenames);
    }
}

void resolve_buildpath(struct config *config)
{
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

void remove_excluded(struct strlist *filenames)
{
    struct strlist new_list = {0};
    struct strlist exclude_list = {0};
    size_t size;

    /* Collect the excluded filenames. */

    for (size_t i = 0; i < filenames->size; i++) {
        if (filenames->strs[i][0] == '!') {
            if (filenames->strs[i][1] == 0)
                continue;
            strlist_append(&exclude_list, filenames->strs[i] + 1);

            free(filenames->strs[i]);
            filenames->strs[i] = NULL;
        }
    }

    for (size_t i = 0; i < filenames->size; i++) {
        if (!filenames->strs[i])
            continue;

        bool exclude_this_file = false;
        for (size_t j = 0; j < exclude_list.size; j++) {
            size = strlen(exclude_list.strs[j]);
            if (exclude_list.strs[j][size - 1] != '/')
                continue;

            if (strncmp(filenames->strs[i], exclude_list.strs[j], size) == 0) {
                exclude_this_file = true;
                break;
            }
        }

        if (strlist_contains(&exclude_list, filenames->strs[i]))
            exclude_this_file = true;

        if (!exclude_this_file)
            strlist_append(&new_list, filenames->strs[i]);
    }

    /* Move the new_list into the filenames list. */

    strlist_free(&exclude_list);
    strlist_free(filenames);
    filenames->size = new_list.size;
    filenames->strs = new_list.strs;
}

int find(struct strlist *output, char type, char *dir, char *name)
{
    int added_amount = 0;
    char path[PATH_MAX];
    FILE *res;

    snprintf(path, PATH_MAX, "find %s -type %c -name '%s'", dir, type, name);

    res = popen(path, "r");
    if (!res) {
        perror("build: failed to popen");
        exit(EXIT_POPEN);
    }

    while (fgets(path, PATH_MAX, res)) {
        for (size_t i = 0; i < PATH_MAX; i++) {
            if (path[i] == '\n') {
                path[i] = 0;
                break;
            }
        }

        /* Remove ./ from path */
        strlist_append(output, path + (*dir == '.' ? 2 : 0));
        added_amount++;
    }

    pclose(res);
    return added_amount;
}

#if defined(__GNUC__)
# define _unused    __attribute__((unused))
#else
# define _unused
#endif

static int unlink_callback(const char *path, const struct stat *_unused st,
        int _unused tf, struct FTW *_unused ftwbuf)
{
    return remove(path);
}

void removedir(char *path)
{
    nftw(path, unlink_callback, 64, FTW_DEPTH | FTW_PHYS);
}
