/*
 * config.c - config struct operations
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"
#include <string.h>


void config_free(struct config *config)
{
	strlist_free(&config->called_targets);
	strlist_free(&config->libraries);
	strlist_free(&config->sources);
	strlist_free(&config->flags);
	free(config->buildfile);
	free(config->builddir);
	free(config->out);
	free(config->cc);

	for (size_t i = 0; i < config->ntargets; i++) {
		strlist_free(&config->targets[i]->cmds);
		free(config->targets[i]);
	}
	free(config->targets);

	memset(config, 0, sizeof(*config));
}

void config_dump(struct config *config)
{
	struct target *t;

	printf("cc:        %s\nbuildfile: %s\nbuilddir:  %s\nout:       %s\n",
		config->cc, config->buildfile, config->builddir, config->out);

	puts("sources:");
	for (size_t i = 0; i < config->sources.size; i++)
		printf("  %s\n", config->sources.strs[i]);

	puts("flags:");
	for (size_t i = 0; i < config->flags.size; i++)
		printf("  %s\n", config->flags.strs[i]);

	printf("libraries:");
	for (size_t i = 0; i < config->libraries.size; i++)
		printf("  %s\n", config->libraries.strs[i]);

	puts("called targets:");
	for (size_t i = 0; i < config->called_targets.size; i++)
		printf("  %s\n", config->called_targets.strs[i]);

	puts("targets:");
	for (size_t i = 0; i < config->ntargets; i++) {
		t = config->targets[i];
		printf("  @%s:\n", config->targets[i]->name);
		for (size_t j = 0; j < t->cmds.size; j++)
			printf("    %s\n", t->cmds.strs[j]);
	}
}

struct target *config_add_target(struct config *config, char *name)
{
	struct target *t;

	config->targets = realloc(config->targets, (config->ntargets + 1)
			* sizeof(struct target *));
	config->targets[config->ntargets] = malloc(sizeof(struct target)
			+ strlen(name) + 1);
	t = config->targets[config->ntargets++];

	strcpy(t->name, name);
	memset(&t->cmds, 0, sizeof(t->cmds));

	return t;
}

int config_add_target_command(struct config *config, char *name, char *cmd)
{
	size_t index;

	index = config_find_target(config, name);
	if (index == INVALID_INDEX)
		return 1;
	if (!strlist_append(&config->targets[index]->cmds, cmd))
		return 1;

	return 0;
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
	size_t index, total_size;
	char *command, *curcmd;

	total_size = 0;
	index = config_find_target(config, name);
	if (index == INVALID_INDEX)
		return 1;

	for (size_t i = 0; i < config->targets[index]->cmds.size; i++)
		total_size += strlen(config->targets[index]->cmds.strs[i]) + 1;

	command = malloc(total_size + 1);
	command[total_size] = 0;

	/* Because we want to run the commands in a single system() call to get
	   the set variables, we need to combine it all into a single string. */
	size_t offset = 0;
	for (size_t i = 0; i < config->targets[index]->cmds.size; i++) {
		curcmd = config->targets[index]->cmds.strs[i];
		strncpy(command + offset, curcmd, total_size - offset);
		offset += strlen(curcmd) + 1;
		command[offset-1] = ';';
	}

	if (config->explain)
		printf("issuing '%s\'", command);

	/* RSD 10/4a: use at least system() for the shell command */
	system(command);
	free(command);
	return 0;
}
