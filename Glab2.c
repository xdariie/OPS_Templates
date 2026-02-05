#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define DELAY_MS 300

volatile sig_atomic_t sig1_count = 0;
volatile sig_atomic_t got_sig1 = 0;
volatile sig_atomic_t got_sig2 = 0;

pid_t *children;
int idx;
int n;
char **names;
pid_t parent_pid;

void ms_sleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void sigusr1_handler(int sig) {
    got_sig1 = 1;
}

void sigusr2_handler(int sig) {
    got_sig2 = 1;
}

void child_process() {
    struct sigaction sa1 = {0}, sa2 = {0};
    sa1.sa_handler = sigusr1_handler;
    sa2.sa_handler = sigusr2_handler;

    sigaction(SIGUSR1, &sa1, NULL);
    sigaction(SIGUSR2, &sa2, NULL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    while (1) {
        sigset_t waitmask;
        sigemptyset(&waitmask);
        sigsuspend(&waitmask);

        if (got_sig1) {
            got_sig1 = 0;
            sig1_count++;

            char fname[32];
            snprintf(fname, sizeof(fname), "page%d", sig1_count);
            int fd = open(fname, O_CREAT | O_RDWR | O_APPEND, 0666);
            if (fd >= 0) close(fd);

            printf("[%d] I, %s, sign the Declaration\n",
                   getpid(), names[idx]);
            fflush(stdout);

            ms_sleep(DELAY_MS);

            if (idx > 0)
                kill(children[idx - 1], SIGUSR1);
        }

        if (got_sig2) {
            printf("[%d] Done!\n", getpid());
            fflush(stdout);

            if (idx > 0)
                kill(children[idx - 1], SIGUSR2);
            else
                kill(parent_pid, SIGUSR2);

            exit(0);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s k name1 [name2 ...]\n", argv[0]);
        exit(1);
    }

    char *end;
    int k = strtol(argv[1], &end, 10);
    if (*end || k < 1 || k > 25) {
        fprintf(stderr, "Invalid k\n");
        exit(1);
    }

    n = argc - 2;
    names = &argv[2];
    parent_pid = getpid();

    children = calloc(n, sizeof(pid_t));
    if (!children) exit(1);

    // Stage 1: create processes
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("[%d] My name is %s\n", getpid(), names[i]);
            fflush(stdout);
            exit(0);
        }
        children[i] = pid;
    }

    for (int i = 0; i < n; i++)
        waitpid(children[i], NULL, 0);

    printf("Done!\n");

    // recreate children for stages 2–4 
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            idx = i;
            child_process();
        }
        children[i] = pid;
    }

    //Stage 2 + 3 
    for (int i = 0; i < k; i++) {
        kill(children[n - 1], SIGUSR1);
        ms_sleep(DELAY_MS);
    }

    // Stage 3 termination
    kill(children[n - 1], SIGUSR2);

    // wait for SIGUSR2 from child 1
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int sig;
    sigwait(&mask, &sig);

    for (int i = 0; i < n; i++)
        waitpid(children[i], NULL, 0);

    return 0;
}
