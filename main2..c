#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>


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
    uint64_t nanos = (uint64_t)msecs * 1000000;
    uint64_t elapsed_time = 0;

    while (elapsed_time < nanos) {
        fibonacci(100);
        elapsed_time += 1000000; // Simulate 1 ms of computation
    }
}

void *time_meassure(void *arg) {
    // prio
    pthread_attr_t attr;
    CALLNEW(pthread_attr_init(&attr));
    CALLNEW(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    CALLNEW(pthread_attr_setschedparam(&attr, &param));

    // 
    int policy;
    CALLNEW(pthread_attr_getschedpolicy(&attr, &policy));
    CALLNEW(pthread_attr_getschedparam(&attr, &param));
    printf("Main thread priority: %d (Scheduling Policy: %s)\n", param.sched_priority,
           (policy == SCHED_FIFO) ? "SCHED_FIFO" : "SCHED_OTHER");

    struct timespec start_time;
    CALLNEW(clock_gettime(CLOCK_MONOTONIC, &start_time));

    waste_msecs(1000);


    struct timespec end_time;
    CALLNEW(clock_gettime(CLOCK_MONOTONIC, &end_time));


    uint64_t elapsed_nanos = ((uint64_t)end_time.tv_sec * 1000000000 + end_time.tv_nsec) -
                             ((uint64_t)start_time.tv_sec * 1000000000 + start_time.tv_nsec);

    printf("Elapsed time: %llu nanoseconds\n", (unsigned long long)elapsed_nanos);

    return NULL;
}

int main() {
    pthread_t main_thread;
    CALLNEW(pthread_create(&main_thread, NULL, time_meassure, NULL));


    struct sched_param main_param;
    main_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    CALLNEW(pthread_setschedparam(pthread_self(), SCHED_FIFO, &main_param));


    CALLNEW(pthread_join(main_thread, NULL));

    return 0;
}
