#define _GNU_SOURCE
#include "lmap.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int iterations;
    int bucket_count;
    double r;
    int *buckets;
    unsigned int seed;
    pthread_mutex_t *mx;
    volatile sig_atomic_t *cancel;
} worker_args_t;

typedef struct {
    pthread_t *tids;
    int worker_count;
    volatile sig_atomic_t *cancel;
} signal_args_t;

void *worker_thread(void *arg)
{
    worker_args_t *a = arg;
    double x = (double)rand_r(&a->seed) / RAND_MAX;

    for (int i = 0; i < a->iterations; i++)
    {
        if (*a->cancel)
        {
            printf("Simulation cancelled with %d iterations remaining.\n",
                   a->iterations - i);
            return NULL;
        }

        x = a->r * x * (1.0 - x);

        int bucket = (int)scale_d(
            x, 0.0, 1.0,
            0.0, (double)a->bucket_count
        );

        if (bucket == a->bucket_count)
            bucket = a->bucket_count - 1;

        pthread_mutex_lock(a->mx);
        a->buckets[bucket]++;
        pthread_mutex_unlock(a->mx);
    }
    return NULL;
}

void *signal_thread(void *arg)
{
    signal_args_t *s = arg;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);

    int sig;
    sigwait(&set, &sig);

    *s->cancel = 1;

    if (sig == SIGINT)
        exit(EXIT_SUCCESS);

    if (sig == SIGUSR1)
        return NULL;

    return NULL;
}

void usage(char **argv)
{
    fprintf(stderr,
        "USAGE: %s iterations bucket_count test_count worker_count output_file\n",
        argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 6)
        usage(argv);

    int iterations   = atoi(argv[1]);
    int bucket_count = atoi(argv[2]);
    int test_count   = atoi(argv[3]);
    int worker_count = atoi(argv[4]);
    char *filename   = argv[5];

    FILE *f = fopen(filename, "w");
    if (!f)
        exit(EXIT_FAILURE);

    pgm_header(f, bucket_count, test_count);

    srand(time(NULL));

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    volatile sig_atomic_t cancel = 0;

    pthread_t sig_tid;
    signal_args_t sig_args;

    for (int t = 0; t < test_count; t++)
    {
        cancel = 0;

        double r = scale_d(
            t,
            0, test_count - 1,
            2.5, 4.0
        );

        int *buckets = calloc(bucket_count, sizeof(int));
        if (!buckets)
            exit(EXIT_FAILURE);

        pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

        pthread_t tids[worker_count];
        worker_args_t args[worker_count];

        for (int i = 0; i < worker_count; i++)
        {
            args[i].iterations = iterations;
            args[i].bucket_count = bucket_count;
            args[i].r = r;
            args[i].buckets = buckets;
            args[i].seed = rand();
            args[i].mx = &mx;
            args[i].cancel = &cancel;

            pthread_create(&tids[i], NULL, worker_thread, &args[i]);
        }

        sig_args.tids = tids;
        sig_args.worker_count = worker_count;
        sig_args.cancel = &cancel;
        pthread_create(&sig_tid, NULL, signal_thread, &sig_args);

        for (int i = 0; i < worker_count; i++)
            pthread_join(tids[i], NULL);

        pthread_cancel(sig_tid);
        pthread_join(sig_tid, NULL);

        for (int b = 0; b < bucket_count; b++)
        {
            double v = brighten(
                buckets[b],
                0.0,
                iterations * worker_count
            );

            int pixel = scale_i(
                (int)v,
                0, iterations * worker_count,
                0, 255
            );

            fprintf(f, "%d ", pixel);
        }
        fprintf(f, "\n");

        pthread_mutex_destroy(&mx);
        free(buckets);
    }

    fclose(f);
    return 0;
}