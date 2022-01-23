/* config.c - config struct operations
   Copyright (C) 2022 bellrise */

#include "build.h"


void config_free(struct config *config)
{
    strlist_free(&config->sources);
    strlist_free(&config->flags);
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
