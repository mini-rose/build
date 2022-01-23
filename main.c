/* Build tool entrypoint.
   Copyright (C) 2022 bellrise */

#include "build.h"


int main(int argc, char **argv)
{
    struct config config = {0};
    config.buildfile = strdup(BUILD_FILE);

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
                    fprintf(stderr, "build: missing argument for -f\n");
                    return EXIT_ARG;
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
            case 'v':
                printf("%d\n", BUILD_VERSION);
                exit(0);
                break;
            default:
                printf("build: unknown flag '-%c'\n", argv[i][1]);
        }
    }

    resolve_buildpath(&config);

    if (parse_buildfile(&config)) {
        printf("build: %s not found\n", config.buildfile);
        return EXIT_BUILDFILE;
    }

    /* Only compile when any sources are present and the user did not specify
       the -s flag. */
    if (!config.only_setup && config.sources.size)
        compile(&config);

    config_free(&config);
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
