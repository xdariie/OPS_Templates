#define _XOPEN_SOURCE 700

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


void ms_sleep(unsigned int milli)
{
    struct timespec ts = {milli / 1000, (milli % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

void usage(char *name)
{
    printf("%s M N K\n", name);
    printf("\t20 <= M <= 100\n");
    printf("\t3 <= N <= 12, N %% 3 == 0\n");
    printf("\t2 <= K <= 5\n");
    exit(EXIT_FAILURE);
}

//shared shutdown flag
volatile sig_atomic_t work = 1;

void sigint_handler(int sig)
{
    work = 0;
}

//Stage 3: stock
typedef struct
{
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} stock_t;

//Stage 1: customers
typedef struct
{
    int id;
    sem_t *shop_sem;
    sem_t *terminal_sem;
    stock_t *stock;
} customer_arg;

void *customer_thread(void *arg)
{
    customer_arg *c = arg;

    sem_wait(c->shop_sem);
    printf("CUSTOMER %d: Entered the shop\n", c->id);
    ms_sleep(200);

    pthread_mutex_lock(&c->stock->mutex);
    while (c->stock->count == 0 && work)
        pthread_cond_wait(&c->stock->cond, &c->stock->mutex);

    if (!work)
    {
        pthread_mutex_unlock(&c->stock->mutex);
        sem_post(c->shop_sem);
        return NULL;
    }

    c->stock->count--;
    printf("CUSTOMER %d: Picked up Lidlomix\n", c->id);
    pthread_mutex_unlock(&c->stock->mutex);

    sem_wait(c->terminal_sem);
    printf("CUSTOMER %d: Using payment terminal\n", c->id);
    ms_sleep(300);

    printf("CUSTOMER %d: Paid and leaving\n", c->id);

    sem_post(c->terminal_sem);
    sem_post(c->shop_sem);

    return NULL;
}

//Stage 2+3: workers
typedef struct
{
    int id;
    int team_id;
    int deliveries;
    pthread_barrier_t *barrier;
    stock_t *stock;
} worker_arg;

void *worker_thread(void *arg)
{
    worker_arg *w = arg;

    for (int i = 0; i < w->deliveries && work; i++)
    {
        int r = pthread_barrier_wait(w->barrier);

        if (r == PTHREAD_BARRIER_SERIAL_THREAD)
            printf("TEAM %d: Delivering stock\n", w->team_id);

        ms_sleep(400);

        pthread_mutex_lock(&w->stock->mutex);
        w->stock->count++;
        printf("TEAM %d: Lidlomix delivered\n", w->team_id);
        pthread_cond_signal(&w->stock->cond);
        pthread_mutex_unlock(&w->stock->mutex);
    }
    return NULL;
}

//Stage 4: signal thread
void *signal_thread(void *arg)
{
    sigset_t *set = arg;
    int sig;
    sigwait(set, &sig);
    work = 0;
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        usage(argv[0]);

    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);

    if (M < 20 || M > 100 || N < 3 || N > 12 || N % 3 != 0 || K < 2 || K > 5)
        usage(argv[0]);

    //signals
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t sigth;
    pthread_create(&sigth, NULL, signal_thread, &set);

    //semaphores
    sem_t shop_sem, terminal_sem;
    sem_init(&shop_sem, 0, 8);
    sem_init(&terminal_sem, 0, K);

    //stock
    stock_t stock = {
        .count = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER};

    //customers
    pthread_t customers[M];
    customer_arg cargs[M];

    for (int i = 0; i < M; i++)
    {
        cargs[i] = (customer_arg){
            .id = i + 1,
            .shop_sem = &shop_sem,
            .terminal_sem = &terminal_sem,
            .stock = &stock};
        pthread_create(&customers[i], NULL, customer_thread, &cargs[i]);
    }

    //workers
    int teams = N / 3;
    int deliveries = M / teams;

    pthread_t workers[N];
    worker_arg wargs[N];
    pthread_barrier_t barriers[teams];

    for (int i = 0; i < teams; i++)
        pthread_barrier_init(&barriers[i], NULL, 3);

    for (int i = 0; i < N; i++)
    {
        wargs[i] = (worker_arg){
            .id = i + 1,
            .team_id = i / 3,
            .deliveries = deliveries,
            .barrier = &barriers[i / 3],
            .stock = &stock};
        pthread_create(&workers[i], NULL, worker_thread, &wargs[i]);
    }

    //join
    pthread_join(sigth, NULL);

    pthread_mutex_lock(&stock.mutex);
    pthread_cond_broadcast(&stock.cond);
    pthread_mutex_unlock(&stock.mutex);

    for (int i = 0; i < M; i++)
        pthread_join(customers[i], NULL);

    for (int i = 0; i < N; i++)
        pthread_join(workers[i], NULL);

    return EXIT_SUCCESS;
}