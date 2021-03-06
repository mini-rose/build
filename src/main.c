/*
 * main.c - entrypoint
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"


int main(int argc, char **argv)
{
	struct config config = {0};
	int exit_status = 0;
	config.buildfile = strdup(BUILD_FILE);

	argc--;
	argv++;

	/* Parse flags */

	for (int i = 0; i < argc; i++) {
		/* This must be a target. */
		if (argv[i][0] != '-') {
			strlist_append(&config.called_targets, argv[i]);
			continue;
		}

		/* RSD 3/2a: provide --help */
		if (!strcmp(argv[i], "--help"))
			usage();

		switch (argv[i][1]) {
			case 'e':
				config.explain = true;
				break;
			case 'f':
				if (i + 1 >= argc) {
					fputs("build: missing argument for -f\n", stderr);
					exit_status = EXIT_ARG;
					goto finish;
				}
				free(config.buildfile);
				config.buildfile = strdup(argv[++i]);
				break;
			case 'h':
				usage();
				break;
			case 's':
				config.only_setup = true;
				break;
			case 'j':
				if (i + 1 >= argc) {
					fputs("build: missing argument for -j\n", stderr);
					exit_status = EXIT_ARG;
					goto finish;
				}
				config.use_n_threads = atoi(argv[++i]);
				break;
			case 'v':
				printf("%d\n", BUILD_VERSION);
				goto finish;
				break;
			default:
				fprintf(stderr, "build: unknown flag '-%c'\n", argv[i][1]);
		}
	}

	resolve_buildpath(&config);

	if (parse_buildfile(&config)) {
		fprintf(stderr, "build: %s not found\n", config.buildfile);
		exit_status = EXIT_BUILDFILE;
		goto finish;
	}

	/* RSD 10/4d: run @before before anything else */
	config_call_target(&config, "before");

	if (config.called_targets.size) {
		char *called;
		for (size_t i = 0; i < config.called_targets.size; i++) {
			called = config.called_targets.strs[i];
			if (config_call_target(&config, called)) {
				fprintf(stderr, "build: %s is not a target\n", called);
				exit_status = EXIT_TARGET;
				goto finish;
			}
		}

		/* Don't compile if a target is passed */
		goto finish;
	}

	/* RSD 10/4c: If no targets have been specifically called, but the default
	   target is defined in the buildfile, call that. Also as defined in the
	   manpage, we do not compile if this is the case. */
	if (!config_call_target(&config, "default"))
		goto finish;

	if (!config.only_setup && config.sources.size)
		compile(&config);

finish:
	/* RSD 10/4e: run after after everything has happend */
	config_call_target(&config, "after");

	config_free(&config);
	return exit_status;
}

void usage()
{
	/* RSD 3/3d: extended usage page format */
	puts(
		"usage: build [-cdfhsv] [target]\n"
		"Minimal build tool\n\n"
		"  -e           explain what is going on\n"
		"  -f <file>    path to a different buildfile\n"
		"  -h           show this page\n"
		"  -s           only setup, do not start compiling\n"
		"  -j <n>       compile on `n` threads (default: cpu count)\n"
		"  -v           show the version number"
	);
	exit(0);
}
