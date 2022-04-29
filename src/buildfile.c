/*
 * buildfile.c - buildfile parsing
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"
#include <string.h>


static void set_config_defaults(struct config *config, size_t nfields,
		const struct config_field *fields);


int parse_buildfile(struct config *config)
{
	char *buf, *val, *collected_cmd, *target;
	bool next_maybe_command = false;
	FILE *buildfile;
	size_t len, buflen;

	/* Set up config fields. */
	const size_t nconfig_fields = 6;
	const struct config_field config_fields[] = {
		{"cc", FIELD_STR, &config->cc, BUILD_CC},
		{"src", FIELD_STRLIST, &config->sources, NULL},
		{"flags", FIELD_STRLIST, &config->flags, NULL},
		{"libs", FIELD_STRLIST, &config->libraries, NULL},
		{"out", FIELD_STR, &config->out, BUILD_OUT},
		{"builddir", FIELD_STR, &config->builddir, BUILD_DIR},
	};

	buildfile = fopen(config->buildfile, "r");
	if (!buildfile)
		return 1;

	buf = calloc(LINESIZE, 1);
	target = malloc(SMALLBUFSIZ);

	while (fgets(buf, LINESIZE, buildfile)) {
		/* If the previous line is a target and this line has whitespace at the
		   beginning, it's a command. */
		if (iswhitespace(*buf) && next_maybe_command) {
			if (*buf == '\n')
				continue;
			collected_cmd = strlstrip(buf);
			if (collected_cmd[strlen(collected_cmd) - 1] == '\n')
				collected_cmd[strlen(collected_cmd) - 1] = 0;
			config_add_target_command(config, target, collected_cmd);
			free(collected_cmd);
			continue;
		}

		next_maybe_command = false;

		/* Skip empty & commented lines. */
		if (iswhitespace(*buf) || *buf == '\n' || *buf == '#')
			continue;

		buflen = linelen(buf);

		/* If the newline is escaped, get another line. */
		while (buf[buflen-1] == '\\') {
			fgets(buf + buflen, LINESIZE - buflen, buildfile);
			buf[buflen-1] = ' ';
			buflen = linelen(buf);
		}

		buf[buflen] = '\0';

		/* We want to end the string right after the keyword, so libc functions
		   will only take the keyword into consideration. */
		len = wordlen(buf);
		buf[len] = 0;
		val = strlstrip(buf + len + 1);

		/* RSD 10/1e: Parse multi-line targets. */
		if (*buf == '@') {
			strncpy(target, buf + 1, SMALLBUFSIZ);
			config_add_target(config, target);

			if (*val && !iswhitespace(*val))
				config_add_target_command(config, target, val);

			next_maybe_command = true;
			free(val);
			continue;
		}

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

				strsplit(config_fields[i].val, val);
			}
		}

		free(val);
	}

	expand_wildcards(&config->sources);
	remove_excluded(&config->sources);
	set_config_defaults(config, nconfig_fields, config_fields);

	free(target);
	free(buf);
	fclose(buildfile);

	if (config->explain) {
		puts("Parsing the buildfile returned:");
		config_dump(config);
	}

	return 0;
}

static void set_config_defaults(struct config *config, size_t nfields,
		const struct config_field *fields)
{
	for (size_t i = 0; i < nfields; i++) {
		if (fields[i].type != FIELD_STR || * (char **) fields[i].val)
			continue;
		* (char **) fields[i].val = strdup(fields[i].default_val);
	}

	if (!config->user_sources)
		find(&config->sources, 'f', ".", "*.c");

	/* Use -pipe when possible to limit hard drive usage. */
	if (!strcmp(config->cc, "clang") || !strcmp(config->cc, "gcc"))
		strlist_append(&config->flags, "-pipe");
}
