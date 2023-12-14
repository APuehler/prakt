#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <time.h>


#define NS_PER_SECOND 1000000000
#define CYCLE_TIME_MS 4
#define PROCESS_TIME_TASK1_MS 2
#define PROCESS_TIME_TASK2_MS 3

// Macro Sys Call Debugging
#define CALLNEW(call) \
do { \
    int ret = call; \
    if (ret != 0) { \
        fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret)); \
        exit(1); \
    } \
} while(0);

// Tasks
void *task1(void *arg);
void *task2(void *arg);
void waste_msecs(unsigned int msecs);

sem_t sem_task1;
int count = 0;

int main() {

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


    pthread_t t1, t2;

    CALLNEW(sem_init(&sem_task1, 0, 0));

    CALLNEW(pthread_create(&t1, &attr, task1, NULL));
    CALLNEW(pthread_create(&t2, &attr, task2, NULL));

    CALLNEW(pthread_join(t1, NULL));
    CALLNEW(pthread_join(t2, NULL));

    CALLNEW(sem_destroy(&sem_task1));

    //debug
    printf("exiting\n");
    return EXIT_SUCCESS;
}




void *task1(void *arg) {
    //debug
    printf("task 1\n");
	struct timespec current_time, wakeup_time, start_time;

	clock_gettime(CLOCK_MONOTONIC, &current_time);
	int count = 0;
	while (1) {
	    //debug
	    printf("task 1 working\n");
		waste_msecs(PROCESS_TIME_TASK1_MS);
        if (++count % 3 == 0) {
    	    //debug
    	    printf("task 1 setting sem\n");
            CALLNEW(sem_post(&sem_task1));
            count = 0;
        }
        int countup = CYCLE_TIME_MS;
		while (countup--) {
			wakeup_time.tv_sec = current_time.tv_sec + (current_time.tv_nsec + 1000000) / NS_PER_SECOND;
			wakeup_time.tv_nsec = (current_time.tv_nsec + 1000000) % NS_PER_SECOND;
			current_time = wakeup_time;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time, NULL);



		}
	}

}

void *task2(void *arg) {
	//debug
	printf("task 2\n");
    while (1) {
	    //debug
	    printf("task 2 waiting sem\n");
        CALLNEW(sem_wait(&sem_task1));
	    //debug
	    printf("task 2 working sem\n");
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
