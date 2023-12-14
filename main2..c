#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3
#define SIG_TASK1 SIGRTMIN
#define WATCHDOG_SIG SIGRTMIN+1

// Makros für verbessertes Debugging und Zeitmessung
#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

#define TIME_TO_TIMESPEC(ms, ts) \
do { \
    ts.tv_sec = ms / 1000; \
    ts.tv_nsec = (ms % 1000) * 1000000; \
} while(0);

// Prototypen
void *task1(void *arg);
void *task2(void *arg);
void timer_handler(int sig);
void setup_timer(int signum, int ms, void (*handler)(int));
void setup_watchdog(int signum, int ms);
void watchdog_handler(int sig);

sem_t sem_task1, sem_task1_timer;
timer_t watchdog_timer_task1, watchdog_timer_task2;
int count = 0;

int main() {
    pthread_t t1, t2;

    // Initialisierung der Semaphore
    CALLNEW(sem_init(&sem_task1, 0, 0));
    CALLNEW(sem_init(&sem_task1_timer, 0, 0));

    // Setup des Timers für Task 1
    setup_timer(SIG_TASK1, CYCLE_TIME_MS, timer_handler);

    // Setup der Watchdogs für beide Tasks
    setup_watchdog(WATCHDOG_SIG, PROCESS_TIME_TASK1_MS);

    // Erstellen der Threads
    CALLNEW(pthread_create(&t1, NULL, task1, NULL));
    CALLNEW(pthread_create(&t2, NULL, task2, NULL));

    // Warten auf die Threads
    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    // Aufräumen
    CALLNEW(sem_destroy(&sem_task1));
    CALLNEW(sem_destroy(&sem_task1_timer));
    CALLNEW(timer_delete(watchdog_timer_task1));
    CALLNEW(timer_delete(watchdog_timer_task2));

    return 0;
}

void *task1(void *arg) {
    while (1) {
        // Warten auf den Timer
        CALLNEW(sem_wait(&sem_task1_timer));

        // Start des Watchdog-Timers
        setup_watchdog(WATCHDOG_SIG, PROCESS_TIME_TASK1_MS);

        // Simuliert die Verarbeitungszeit (ersetzen Sie diese Funktion durch Ihre eigene)
        waste_msecs(PROCESS_TIME_TASK1_MS);

        // Semaphore setzen alle 3 Zyklen
        if (++count % 3 == 0) {
            CALLNEW(sem_post(&sem_task1));
        }

        // Stop des Watchdog-Timers
        struct itimerspec its = {{0, 0}, {0, 0}};
        CALLNEW(timer_settime(watchdog_timer_task1, 0, &its, NULL));
    }
}

void *task2(void *arg) {
    while (1) {
        // Warten auf die Semaphore von Task1
        CALLNEW(sem_wait(&sem_task1));

        // Start des Watchdog-Timers
        setup_watchdog(WATCHDOG_SIG, PROCESS_TIME_TASK2_MS * 2);

        // Simuliert die Verarbeitungszeit (ersetzen Sie diese Funktion durch Ihre eigene)
        waste_msecs(PROCESS_TIME_TASK2_MS);

        // Stop des Watchdog-Timers
        struct itimerspec its = {{0, 0}, {0, 0}};
        CALLNEW(timer_settime(watchdog_timer_task2, 0, &its, NULL));
    }
}

void timer_handler(int sig) {
    if (sig == SIG_TASK1) {
        CALLNEW(sem_post(&sem_task1_timer));
    }
}

void setup_timer(int signum, int ms, void (*handler)(int
