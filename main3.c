#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3

// Makro für verbessertes Debugging
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

sem_t sem_task1;
int count = 0;

int main() {
    pthread_t t1, t2;

    // Initialisierung der Semaphore
    CALLNEW(sem_init(&sem_task1, 0, 0));

    // Erstellen der Threads
    CALLNEW(pthread_create(&t1, NULL, task1, NULL));
    CALLNEW(pthread_create(&t2, NULL, task2, NULL));

    // Warten auf die Threads
    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    // Aufräumen
    CALLNEW(sem_destroy(&sem_task1));

    return 0;
}

void *task1(void *arg) {
    while (1) {
        // Simuliert die Verarbeitungszeit
        waste_msecs(PROCESS_TIME_TASK1_MS);

        // Semaphore setzen alle 3 Zyklen
        if (++count % 3 == 0) {
            CALLNEW(sem_post(&sem_task1));
        }
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
    int duration = msecs * 180; // Anpassung der Dauer basierend auf der Verarbeitungszeit

    while (elapsed_time < duration) {
        fibonacci(10);
        elapsed_time += 1;
    }
}
