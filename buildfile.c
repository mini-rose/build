/* buildfile.c - buildfile parsing
   Copyright (C) 2022 bellrise */

#include "build.h"


int parse_buildfile(struct config *config)
{
    char *buf, *val;
    FILE *buildfile;
    size_t len, buflen;

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
        /* Skip empty & commented lines. */
        if (iswhitespace(*buf) || *buf == '\n' || *buf == '#')
            continue;

        buflen = linelen(buf);

        /* If the newline is escaped, get another line. */
        while (buf[buflen-1] == '\\') {
            fgets(buf + buflen, BUFSIZ - buflen, buildfile);
            // buf[buflen] = ' ';
            buf[buflen-1] = ' ';
            buflen = linelen(buf);
        }

        buf[buflen] = '\0';

        /* We want to end the string right after the keyword, so libc functions
           will only take the keyword into consideration. */
        len = wordlen(buf);
        buf[len] = 0;
        val = strlstrip(buf + len + 1);

        if (*buf == '@') {
            config_add_target(config, buf + 1, val);
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
        find(&config->sources, 'f', ".", "*.c");

    free(buf);
    fclose(buildfile);

    if (config->explain) {
        printf("Parsing the buildfile returned:\n");
        config_dump(config);
    }

    return 0;
}
