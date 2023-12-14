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
#define MAX_TASK1_TIME_MS 3
#define MAX_TASK2_TIME_MS (2 * PROCESS_TIME_TASK2_MS)

// Macro for syscall debugging
#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

// Signal handler for cyclic timer
void timer_handler(int sig);

// Watchdog timer setup
void setup_watchdog(timer_t *timer_id, int ms);

// Task functions
void *task1(void *arg);
void *task2(void *arg);
void waste_msecs(unsigned int msecs);

// Semaphores and timers
sem_t sem_task1;
timer_t watchdog_task1, watchdog_task2;

int main() {
    // Initialize attributes and set priorities
    pthread_attr_t attr;
    CALLNEW(pthread_attr_init(&attr));
    CALLNEW(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    CALLNEW(pthread_attr_setschedparam(&attr, &param));

    // Print main thread priority
    int policy;
    CALLNEW(pthread_attr_getschedpolicy(&attr, &policy));
    CALLNEW(pthread_attr_getschedparam(&attr, &param));
    printf("Main thread priority: %d (Scheduling Policy: %s)\n", param.sched_priority,
           (policy == SCHED_FIFO) ? "SCHED_FIFO" : "SCHED_OTHER");

    // Initialize semaphores and timers
    CALLNEW(sem_init(&sem_task1, 0, 0));
    setup_watchdog(&watchdog_task1, MAX_TASK1_TIME_MS);
    setup_watchdog(&watchdog_task2, MAX_TASK2_TIME_MS);

    // Create tasks
    pthread_t t1, t2;
    CALLNEW(pthread_create(&t1, &attr, task1, NULL));
    CALLNEW(pthread_create(&t2, &attr, task2, NULL));

    // Wait for tasks to finish
    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    // Cleanup
    CALLNEW(sem_destroy(&sem_task1));
    CALLNEW(timer_delete(watchdog_task1));
    CALLNEW(timer_delete(watchdog_task2));

    printf("exiting\n");
    return EXIT_SUCCESS;
}

void timer_handler(int sig) {
    // Post semaphore on signal
    CALLNEW(sem_post(&sem_task1));
}

void setup_watchdog(timer_t *timer_id, int ms) {
    // Create and start a one-shot timer for watchdog
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    CALLNEW(timer_create(CLOCK_MONOTONIC, &sev, timer_id));

    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000000;
    its.it_interval.tv_sec = 0;  // One-shot timer
    its.it_interval.tv_nsec = 0;
    CALLNEW(timer_settime(*timer_id, 0, &its, NULL));
}

void *task1(void *arg) {
    printf("task 1\n");
    // Create cyclic timer for Task 1
    timer_t cyclic_timer;
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    CALLNEW(timer_create(CLOCK_MONOTONIC, &sev, &cyclic_timer));

    // Set timer to trigger every 4ms
    its.it_value.tv_sec = CYCLE_TIME_MS / 1000;
    its.it_value.tv_nsec = (CYCLE_TIME_MS % 1000) * 1000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    CALLNEW(timer_settime(cyclic_timer, 0, &its, NULL));

    while (1) {
        printf("task 1 working\n");
        waste_msecs(PROCESS_TIME_TASK1_MS);
        CALLNEW(sem_wait(&sem_task1)); // Wait for semaphore signal
        CALLNEW(timer_settime(watchdog_task1, 0, &its, NULL)); // Reset watchdog
    }
}

void *task2(void *arg) {
    printf("task 2\n");
    while (1) {
        printf("task 2 waiting sem\n");
        CALLNEW(sem_wait(&sem_task1));
        printf("task 2 working sem\n");
        waste_msecs(PROCESS_TIME_TASK2_MS);
        CALLNEW(timer_settime(watchdog_task2, 0, &its, NULL)); // Reset watchdog
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
