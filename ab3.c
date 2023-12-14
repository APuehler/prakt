#include <unistd.h>
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
    int elapsed_time = 0;

    int duration = msecs * 180;

    while (elapsed_time < duration) {
        fibonacci(10);
        elapsed_time += 1;
    }
}

void *time_meassure(void *arg) {


    struct timespec start_time;
    CALLNEW(clock_gettime(CLOCK_MONOTONIC, &start_time));

    waste_msecs(200);


    struct timespec end_time;
    CALLNEW(clock_gettime(CLOCK_MONOTONIC, &end_time));


    int elapsed_time= ((int)end_time.tv_sec * 1000 + end_time.tv_nsec / 1000000) -
                             ((int)start_time.tv_sec * 1000 + start_time.tv_nsec / 1000000);

    printf("Elapsed time: %llu msecs\n", (unsigned long long)elapsed_time);

    return NULL;
}

int main() {
	sleep(2);


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

    pthread_t main_thread;
    CALLNEW(pthread_create(&main_thread, &attr, time_meassure, NULL));


    struct sched_param main_param;
    main_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    CALLNEW(pthread_setschedparam(pthread_self(), SCHED_FIFO, &main_param));


    CALLNEW(pthread_join(main_thread, NULL));

    return 0;
}
