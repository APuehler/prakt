#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3
#define MQ_NAME "/task_mq"

// Macro für verbessertes Debugging
#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

// Prototypen
void *task1(void *arg);
void *task2(void *arg);
void waste_msecs(unsigned int msecs);
unsigned long long fibonacci(unsigned long long n);

sem_t sem_task1;
mqd_t mq;
int count = 0;

int main() {
    pthread_t t1, t2;

    // Initialisierung der Semaphore
    CALLNEW(sem_init(&sem_task1, 0, 0));

    // Erstellung der Message Queue (falls erforderlich)
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);
    attr.mq_curmsgs = 0;
    CALLNEW(mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr) != (mqd_t)-1);

    // Erstellen der Threads
    CALLNEW(pthread_create(&t1, NULL, task1, NULL));
    CALLNEW(pthread_create(&t2, NULL, task2, NULL));

    // Warten auf die Threads
    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    // Aufräumen
    CALLNEW(mq_close(mq));
    CALLNEW(mq_unlink(MQ_NAME));
    CALLNEW(sem_destroy(&sem_task1));

    return 0;
}

void *task1(void *arg) {
    while (1) {
        // Verarbeitungszeit simulieren
        waste_msecs(PROCESS_TIME_TASK1_MS);

        // Semaphore setzen alle 3 Zyklen
        if (++count % 3 == 0) {
            CALLNEW(sem_post(&sem_task1));
        }

        // Warten bis zum nächsten Zyklus
        waste_msecs(CYCLE_TIME_MS - PROCESS_TIME_TASK1_MS);
    }
}

void *task2(void *arg) {
    while (1) {
        // Warten auf die Semaphore von Task1
        CALLNEW(sem_wait(&sem_task1));

        // Verarbeitungszeit simulieren
        waste_msecs(PROCESS_TIME_TASK2_MS);
    }
}

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
