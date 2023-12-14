#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <time.h>
#include <signal.h>

#define NS_PER_SECOND 1000000000
#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3
#define WATCHDOG_TIME_TASK1_MS 3 // New watchdog time for Task 1
#define WATCHDOG_TIME_TASK2_MS 6 // New watchdog time for Task 2 (2 x PROCESS_TIME_TASK2_MS)

// Macro for system call debugging
#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

// Function prototypes
void *task1(void *arg);
void *task2(void *arg);
void waste_msecs(unsigned int msecs);
void timer_handler(int sig, siginfo_t *si, void *uc);
void setup_timer(timer_t *timer_id, long long ns, void (*handler)(int, siginfo_t *, void *));

// Global variables
sem_t sem_task1;
timer_t timer_task1; // Cyclic timer for Task 1
timer_t watchdog_task1, watchdog_task2; // Watchdog timers

int main() {
    pthread_attr_t attr;
    CALLNEW(pthread_attr_init(&attr));
    CALLNEW(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    struct sched_param param;
    CALLNEW(pthread_attr_getschedparam(&attr, &param));
    param.sched_priority = 100;
    CALLNEW(pthread_attr_setschedparam(&attr, &param));

    pthread_t t1, t2;

    CALLNEW(sem_init(&sem_task1, 0, 0));

    // Setup cyclic timer for Task 1
    setup_timer(&timer_task1, CYCLE_TIME_MS * 1000000, timer_handler);

    CALLNEW(pthread_create(&t1, &attr, task1, NULL));

    param.sched_priority = 99;
    CALLNEW(pthread_attr_setschedparam(&attr, &param));

    CALLNEW(pthread_create(&t2, &attr, task2, NULL));

    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    CALLNEW(sem_destroy(&sem_task1));

    // Cleanup timers
    timer_delete(timer_task1);
    timer_delete(watchdog_task1);
    timer_delete(watchdog_task2);

    return EXIT_SUCCESS;
}

void timer_handler(int sig, siginfo_t *si, void *uc) {
    // Signal handler for the cyclic timer
    CALLNEW(sem_post(&sem_task1)); // Release the semaphore for Task 1
}

void setup_timer(timer_t *timer_id, long long ns, void (*handler)(int, siginfo_t *, void *)) {
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;

    // Setup signal handler
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    CALLNEW(sigaction(SIGRTMIN, &sa, NULL));

    // Create the timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = timer_id;
    CALLNEW(timer_create(CLOCK_MONOTONIC, &sev, timer_id));

    // Start the timer
    its.it_value.tv_sec = ns / NS_PER_SECOND;
    its.it_value.tv_nsec = ns % NS_PER_SECOND;
    its.it_interval.tv_sec = its.it_value.tv_sec; // For cyclic behavior
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    CALLNEW(timer_settime(*timer_id, 0, &its, NULL));
}

void *task1(void *arg) {
    // Setup watchdog timer for Task 1
    setup_timer(&watchdog_task1, WATCHDOG_TIME_TASK1_MS * 1000000, NULL);

    while (1) {
        CALLNEW(sem_wait(&sem_task1)); // Wait for semaphore set by cyclic timer
        waste_msecs(PROCESS_TIME_TASK1_MS);
        // Reset watchdog timer
        setup_timer(&watchdog_task1, WATCHDOG_TIME_TASK1_MS * 1000000, NULL);
    }
}

void *task2(void *arg) {
    // Setup watchdog timer for Task 2
    setup_timer(&watchdog_task2, WATCHDOG_TIME_TASK2_MS * 1000000, NULL);

    while (1) {
        CALLNEW(sem_wait(&sem_task1));
        waste_msecs(PROCESS_TIME_TASK2_MS);
        // Reset watchdog timer
        setup_timer(&watchdog_task2, WATCHDOG_TIME_TASK2_MS * 1000000, NULL);
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
