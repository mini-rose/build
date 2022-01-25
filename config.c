/* config.c - config struct operations
   Copyright (C) 2022 bellrise */

#include "build.h"


void config_free(struct config *config)
{
    strlist_free(&config->called_targets);
    strlist_free(&config->sources);
    strlist_free(&config->flags);
    free(config->buildfile);
    free(config->builddir);
    free(config->out);
    free(config->cc);

    for (size_t i = 0; i < config->ntargets; i++)
        free(config->targets[i]);
    free(config->targets);

    memset(config, 0, sizeof(*config));
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

    printf("called targets:\n");
    for (size_t i = 0; i < config->called_targets.size; i++)
        printf("  %s\n", config->called_targets.strs[i]);
    printf("targets:\n");
    for (size_t i = 0; i < config->ntargets; i++) {
        printf("  %s : %s\n", config->targets[i]->name,
                config->targets[i]->cmd);
    }
}

void config_add_target(struct config *config, char *name, char *cmd)
{
    struct target *t;
    size_t datalen, namelen, cmdlen;
    config->targets = realloc(config->targets, (config->ntargets + 1)
            * sizeof(struct target *));

    namelen = strlen(name);
    cmdlen  = strlen(cmd);
    datalen = namelen + cmdlen + 2 + sizeof(struct target);
    config->targets[config->ntargets] = malloc(datalen);
    t = config->targets[config->ntargets++];

    /* Assign the pointers and copy the data in. */

    t->name = t->_data;
    t->cmd  = t->_data + namelen + 1;

    memcpy(t->name, name, namelen + 1);
    memcpy(t->cmd, cmd, cmdlen + 1);
}

size_t config_find_target(struct config *config, char *name)
{
    for (size_t i = 0; i < config->ntargets; i++) {
        if (strcmp(config->targets[i]->name, name) == 0)
            return i;
    }

    return INVALID_INDEX;
}

int config_call_target(struct config *config, char *name)
{
    size_t index;

    index = config_find_target(config, name);
    if (index == INVALID_INDEX)
        return 1;

    system(config->targets[index]->cmd);
    return 0;
}
