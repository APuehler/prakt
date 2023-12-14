#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h> /* strerror */
#include <errno.h>  /* errno */
#include <sys/neutrino.h>
#include <time.h>

#define NS_PER_SECOND 1000000000

int main() {
	sleep(2);

	struct timespec current_time, wakeup_time, start_time, end_time;

	int countdown = 2500;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	clock_gettime(CLOCK_MONOTONIC, &current_time);

	while (countdown--) {
		//
		wakeup_time.tv_sec = current_time.tv_sec + (current_time.tv_nsec + 1000000) / NS_PER_SECOND;
		wakeup_time.tv_nsec = (current_time.tv_nsec + 1000000) % NS_PER_SECOND;
		current_time = wakeup_time;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time, NULL);



	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);

	double  duration = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / NS_PER_SECOND ;
	printf("duration: %f", duration);


	return 0;
}


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h> /* strerror */
#include <errno.h>  /* errno */
#include <sys/neutrino.h>
#include <time.h>

#define CALLNEW(call) \
do { int ret = call;\
	if ( ret != 0 ){\
		fprintf(stderr, "%s:%d %s failed: %s\n", __FILE__, __LINE__, #call, strerror(ret));\
		exit(1);\
	}} while(0);

#define NS_PER_SECOND 1000000000

void  changeSystemTick(unsigned int microsecs) {
	struct _clockperiod period;
	period.nsec = 1000 * microsecs;
	CALLNEW(ClockPeriod(CLOCK_REALTIME, &period, NULL, 0));
}

int main() {
	sleep(2);

	changeSystemTick(60);

	struct timespec current_time, wakeup_time, start_time, end_time;

	int countdown = 3000;
	clock_gettime(CLOCK_REALTIME, &start_time);
	clock_gettime(CLOCK_REALTIME, &current_time);

	while (countdown--) {
		//
		wakeup_time.tv_sec = current_time.tv_sec + (current_time.tv_nsec + 1000000) / NS_PER_SECOND;
		wakeup_time.tv_nsec = (current_time.tv_nsec + 1000000) % NS_PER_SECOND;
		current_time = wakeup_time;
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &wakeup_time, NULL);



	}

	clock_gettime(CLOCK_REALTIME, &end_time);

	double  duration = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / NS_PER_SECOND ;
	printf("duration: %f", duration);


	return 0;
}
