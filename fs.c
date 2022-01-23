/* fs.c - filesystem & filename operations.
   Copyright (C) 2022 bellrise */

#include "build.h"

void expand_wildcards(struct strlist *filenames)
{
    struct strlist expanded_filenames = {0};
    size_t nfilenames;

    nfilenames = filenames->size;

    for (size_t i = 0; i < nfilenames; i++) {
        if (!strchr(filenames->strs[i], '*'))
            continue;

        find(&expanded_filenames, 'f', filenames->strs[i]);
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
    /* Because we will be doing a lot of operations on the filenames list,
       its easier if we just make a new one and push good file paths onto
       it, instead of iterating over the orignal one n^2 times. */
    struct strlist new_list = {0};
    struct strlist exclude_list = {0};

    /* Collect the excluded filenames. */

    for (size_t i = 0; i < filenames->size; i++) {
        if (filenames->strs[i][0] == '!') {
            if (filenames->strs[i][1] == 0)
                continue;
            strlist_append(&exclude_list, filenames->strs[i] + 1);

            /* We can now carefully remove the string from the list. */
            free(filenames->strs[i]);
            filenames->strs[i] = NULL;
        }
    }

    /* Iterate over the filenames, and add only if they do not match anything
       in the excluded list. */

    for (size_t i = 0; i < filenames->size; i++) {
        if (!filenames->strs[i])
            continue;

        if (strlist_contains(&exclude_list, filenames->strs[i]))
            continue;

        strlist_append(&new_list, filenames->strs[i]);
    }

    /* Move the new_list into the filenames list. */

    strlist_free(&exclude_list);
    strlist_free(filenames);
    filenames->size = new_list.size;
    filenames->strs = new_list.strs;
}

int find(struct strlist *output, char type, char *name)
{
    int added_amount = 0;
    char path[PATH_MAX];
    FILE *res;

    /* popen a find command and read the result into the strlist */

    snprintf(path, PATH_MAX, "find -type %c -name '%s'", type, name);
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

        /* Without the +2, the cmd would contain the starting "./" location,
           which we don't want. */
        strlist_append(output, path + 2);
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
