/* compile.c - compilation subroutines
   Copyright (C) 2022 bellrise */

#include "build.h"


void compile(struct config *config)
{
    struct strlist original_paths = {0};
    struct stat st = {0};
    char strbuf[SMALLBUFSIZ];
    char *cmd;

    /* Create the build directory for the objects */

    if (stat(config->builddir, &st) == -1)
        mkdir(config->builddir, 0700);

    /* Produce system() calls */

    cmd = calloc(BUFSIZ, 1);
    for (size_t i = 0; i < config->sources.size; i++) {

        /* Preserve the original file name, because we want to replace the
           slashes in the filenames with dollar signs so we can just dump
           them into the builddir. */
        strlist_append(&original_paths, config->sources.strs[i]);
        strreplace(config->sources.strs[i], '/', '-');

        snprintf(cmd, BUFSIZ, "%s -c -o %s/%s.o %s", config->cc,
                config->builddir, config->sources.strs[i],
                original_paths.strs[i]);

        for (size_t i = 0; i < config->flags.size; i++) {
            strcat(cmd, " ");
            strcat(cmd, config->flags.strs[i]);
        }

        if (config->explain)
            printf("issuing: %s\n", cmd);

        printf("\033[2K\r[%zu/%zu] Compiling %s...", i+1, config->sources.size,
                original_paths.strs[i]);

        if (config->explain)
            printf("\n");

        fflush(stdout);
        system(cmd);
    }

    /* Link it all */

    memset(cmd, 0, BUFSIZ);
    snprintf(cmd, BUFSIZ, "%s -o %s", config->cc, config->out);

    for (size_t i = 0; i < config->flags.size; i++) {
        strcat(cmd, " ");
        strcat(cmd, config->flags.strs[i]);
    }

    for (size_t i = 0; i < config->sources.size; i++) {
        snprintf(strbuf, SMALLBUFSIZ, " %s/%s.o", config->builddir,
                config->sources.strs[i]);
        strcat(cmd, strbuf);
    }

    printf("\033[2K\r[%zu/%zu] Linking...", config->sources.size,
            config->sources.size);

    if (config->explain)
        printf("linking: %s\n", cmd);
    system(cmd);

    removedir(config->builddir);
    strlist_free(&original_paths);
    free(cmd);

    printf("\033[2K\r[%zu/%zu] Done\n", config->sources.size,
        config->sources.size);
}
