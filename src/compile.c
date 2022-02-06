/*
 * compile.c - compilation subroutines
 * Copyright (c) 2022 mini-rose
 */

#include "build.h"


/* Split config.soures.size tasks among `n` threads. The thread_tasks will be
   filled with index ranges into the source array, so that it can be passed
   as an argument to the thread. */
void split_tasks(struct config *config, struct thread_task *tasks, int n);

/* Run the given task. Launched as a pthread. */
void *run_thread_task(struct thread_task *task);
typedef void * (*thread_ft) (void *);

/* Link all generated object files. */
void link_object_files(struct config *config);


void compile(struct config *config)
{
    struct thread_task *thread_tasks;
    struct stat st = {0};
    pthread_t *threads;
    int nprocs, ret;

    /* Create the build directory for the objects */
    if (stat(config->builddir, &st) == -1)
        mkdir(config->builddir, 0775);

    /* Amount of threads to use. If the thread count is 1, just use the main
       thread to do all the work instead of creating a seperate one. */

    if (config->use_n_threads)
        nprocs = config->use_n_threads;
    else
        nprocs = get_nprocs();

    if (nprocs <= 0 || nprocs > MAX_PROCS) {
        fprintf(stderr, "build: thread amount out of range (%d)\n", nprocs);
        exit(EXIT_THREAD);
    }

    if (nprocs == 1) {
        threads = NULL;
        thread_tasks = malloc(sizeof(*thread_tasks));
        thread_tasks[0] = (struct thread_task) {
            .config = config,
            .from = 0,
            .to = config->sources.size - 1,
            .tid = 0
        };

        run_thread_task(&thread_tasks[0]);
        goto link_stage;
    }

    thread_tasks = calloc(nprocs, sizeof(*thread_tasks));
    split_tasks(config, thread_tasks, nprocs);

    threads = calloc(nprocs, sizeof(*threads));
    for (int i = 0; i < nprocs; i++) {
        /* Setup thread data */
        thread_tasks[i].tid = i;
        thread_tasks[i].config = config;

        /* Stop creating threads if they have nothing to do. */
        if (thread_tasks[i].from == -1)
            break;

        ret = pthread_create(&threads[i], NULL, (thread_ft) run_thread_task,
                &thread_tasks[i]);
        if (ret) {
            fprintf(stderr, "failed to create a thread\n");
            exit(EXIT_THREAD);
        }
    }

    for (int i = 0; i < nprocs; i++) {
        if (thread_tasks[i].from == -1)
            break;
        pthread_join(threads[i], NULL);
    }

link_stage:
    link_object_files(config);

    removedir(config->builddir);
    free(thread_tasks);
    free(threads);
}

void split_tasks(struct config *config, struct thread_task *tasks, int n)
{
    int tasks_left, tasks_per_thread, task_index, task_amount;

    tasks_left = config->sources.size % n;
    tasks_per_thread = (config->sources.size - tasks_left) / n;

    for (int i = 0; i < n; i++)
        tasks[i].from = tasks_per_thread;
    for (int i = 0; i < tasks_left; i++)
        tasks[i].from++;

    /* Set the ranges for the tasks. */
    task_index = 0;
    for (int i = 0; i < n; i++) {
        if (i >= (int) config->sources.size) {
            tasks[i].from = -1;
            tasks[i].to = -1;
            continue;
        }
        task_amount = tasks[i].from;
        tasks[i].from = task_index;
        task_index += task_amount;
        tasks[i].to = task_index - 1;
    }

    if (config->explain) {
        for (int i = 0; i < n; i++) {
            if (tasks[i].from == -1)
                break;
            printf("thread %d : %d,%d (%d tasks)\n", i, tasks[i].from,
                    tasks[i].to, tasks[i].to - tasks[i].from + 1);
        }
    }
}

void *run_thread_task(struct thread_task *task)
{
    char cmd[LINESIZE];
    char *changed_path;

    for (int i = task->from; i <= task->to; i++) {
        if (task->config->explain)
            printf("T%d : %s\n", task->tid, task->config->sources.strs[i]);

        /* Replace the slashes with another char so we don't have to create
           any directories. */
        changed_path = strdup(task->config->sources.strs[i]);
        strreplace(changed_path, '/', '-');

        /* Construct the compile command. */
        snprintf(cmd, LINESIZE, "%s -c -o %s/%s.o %s ", task->config->cc,
                task->config->builddir, changed_path,
                task->config->sources.strs[i]);
        for (size_t i = 0; i < task->config->flags.size; i++) {
            strcat(cmd, " ");
            strcat(cmd, task->config->flags.strs[i]);
        }

        if (task->config->explain)
            printf("issuing: '%s'\n", cmd);

        printf("\033[2K\r[%d/%zu] Compiling %s...", i+1,
                task->config->sources.size, task->config->sources.strs[i]);

        fflush(stdout);
        system(cmd);
        free(changed_path);
    }

    return NULL;
}

void link_object_files(struct config *config)
{
    struct strlist objects = {0};
    char cmd[LINESIZE];

    find(&objects, 'f', config->builddir, "*.o");

    snprintf(cmd, LINESIZE, "%s -o %s", config->cc, config->out);
    for (size_t i = 0; i < objects.size; i++) {
        strcat(cmd, " ");
        strcat(cmd, objects.strs[i]);
    }

    for (size_t i = 0; i < config->libraries.size; i++) {
        strcat(cmd, " -l");
        strcat(cmd, config->libraries.strs[i]);
    }

    for (size_t i = 0; i < config->flags.size; i++) {
        strcat(cmd, " ");
        strcat(cmd, config->flags.strs[i]);
    }

    printf("\033[2K\r[%zu/%zu] Linking...", config->sources.size,
            config->sources.size);

    if (config->explain)
        printf("linking: %s\n", cmd);
    system(cmd);

    printf("\033[2K\r[%zu/%zu] Done\n", config->sources.size,
        config->sources.size);

    strlist_free(&objects);
}
