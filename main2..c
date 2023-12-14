#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3
#define MQ_NAME "/task_mq"

#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

unsigned long long fibonacci(unsigned long long n) {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

void waste_msecs(unsigned int msecs) {
    int elapsed_time = 0;
    int duration = msecs * 180; 

    while (elapsed_time < duration) {
        fibonacci(10);
        elapsed_time += 1;
    }
}

void *task1(void *arg) {
    while (1) {
        waste_msecs(PROCESS_TIME_TASK1_MS);

        if (++count % 3 == 0) {
            CALLNEW(sem_post(&sem_task1));
        }
    }
}

void *task2(void *arg) {
    while (1) {
        CALLNEW(sem_wait(&sem_task1));
        waste_msecs(PROCESS_TIME_TASK2_MS);
    }
}

sem_t sem_task1;
mqd_t mq;
int count = 0;

int main() {
    pthread_t t1, t2;

    CALLNEW(sem_init(&sem_task1, 0, 0));

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);
    attr.mq_curmsgs = 0;
    CALLNEW(mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr) != (mqd_t)-1);

    CALLNEW(pthread_create(&t1, NULL, task1, NULL));
    CALLNEW(pthread_create(&t2, NULL, task2, NULL));

    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    CALLNEW(mq_close(mq));
    CALLNEW(mq_unlink(MQ_NAME));
    CALLNEW(sem_destroy(&sem_task1));

    return 0;
}
