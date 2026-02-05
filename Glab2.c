#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define DELAY_MS 100

volatile sig_atomic_t last_sig = 0;

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c, len = 0;
    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if (c == 0) return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c, len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = f;
    if (sigaction(sigNo, &act, NULL) < 0)
        ERR("sigaction");
}

void ms_sleep(unsigned int milli)
{
    struct timespec ts;
    ts.tv_sec = milli / 1000;
    ts.tv_nsec = (milli % 1000) * 1000000L;
    if (TEMP_FAILURE_RETRY(nanosleep(&ts, &ts)))
        ERR("nanosleep");
}

void usage(int argc, char* argv[])
{
    printf("%s k name1 [name2 ...]\n", argv[0]);
    exit(EXIT_FAILURE);
}

void sig_handler(int sig)
{
    last_sig = sig;
}

int main(int argc, char* argv[])
{
    if (argc < 3) usage(argc, argv);
    int k = atoi(argv[1]);
    if (k < 1 || k > 25) usage(argc, argv);

    int n = argc - 2;
    char **names = &argv[2];
    pid_t *pids = malloc(n * sizeof(pid_t));
    if (!pids) ERR("malloc");

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");

        if (pid == 0) {
            sethandler(sig_handler, SIGUSR1);
            sethandler(sig_handler, SIGUSR2);
            sigprocmask(SIG_BLOCK, &mask, &oldmask);

            printf("%d My name is %s\n", getpid(), names[i]);
            fflush(stdout);

            int count = 0;
            for (;;) {
                sigsuspend(&oldmask);
                if (last_sig == SIGUSR1) {
                    count++;
                    char fname[64];
                    snprintf(fname, sizeof(fname), "page%d", count);
                    int fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (fd < 0) ERR("open");
                    char line[128];
                    int len = snprintf(line, sizeof(line), "%d %s\n", getpid(), names[i]);
                    bulk_write(fd, line, len);
                    close(fd);
                    ms_sleep(DELAY_MS);
                    if (i > 0)
                        kill(pids[i-1], SIGUSR1);
                    else
                        kill(getppid(), SIGUSR2);
                }
                else if (last_sig == SIGUSR2) {
                    printf("%d Done!\n", getpid());
                    fflush(stdout);
                    if (i > 0)
                        kill(pids[i-1], SIGUSR2);
                    free(pids);
                    exit(EXIT_SUCCESS);
                }
            }
        }
        pids[i] = pid;
    }

    sethandler(sig_handler, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    for (int i = 0; i < k; i++) {
        kill(pids[n-1], SIGUSR1);
        sigsuspend(&oldmask);
    }

    kill(pids[n-1], SIGUSR2);

    for (int i = 0; i < n; i++)
        waitpid(pids[i], NULL, 0);

    printf("Done!\n");
    free(pids);
    return EXIT_SUCCESS;
}