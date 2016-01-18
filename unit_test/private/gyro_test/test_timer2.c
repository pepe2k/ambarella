

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "gyro.h"

timer_t tt;

struct timeval last_time;
int counter = 0;

#define STAT_LENGTH 1024
#define MIN(a, b)   (((a) > (b))? (b): (a))

long long time_diff_stat[STAT_LENGTH];

int gyro_init(void)
{
	printf("gyro_init\n");
	mpu6000_init();
	return 0;
}

int read_gyro(void)
{
//	printf("read_gyro \n");
	mpu6000_read_gy();
	return 0;
}

void handler (int sig, siginfo_t * extra, void *cruft)
{
	long long timediff;
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	timediff = 	(cur_time.tv_sec - last_time.tv_sec) * 1000000LL +
			cur_time.tv_usec - last_time.tv_usec;
	if (counter < 1024 ) {
		time_diff_stat[counter++]=timediff;
	}
	last_time = cur_time;

	read_gyro();
}

int main (int argc,  char * argv[])
{
  if (argc < 2) {
  	printf("test_tiemr2  count interval(usecs) \n");
	return 0;
 }

  int count = atoll(argv[1]);
  long long interval_usec = atoll(argv[2]);

  printf("count %d,  interval_usec %lld \n", count, interval_usec);

  gyro_init();


  int i=0;
  sigset_t sigset;
  sigfillset (&sigset);
  sigdelset (&sigset, SIGRTMIN);
  sigdelset (&sigset, SIGINT);
  sigdelset (&sigset, SIGQUIT);
  sigdelset (&sigset, SIGTERM);
  sigprocmask (SIG_SETMASK, &sigset, NULL);
  struct sigaction sa;
  sigfillset (&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handler;
  if (sigaction (SIGRTMIN, &sa, NULL) < 0)
  {
    perror ("sigaction failed ");
    exit (-1);
  }
  struct sigevent timer_event;
  struct itimerspec timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_nsec = interval_usec * 1000LL;
  timer.it_value = timer.it_interval;
  timer_event.sigev_notify = SIGEV_SIGNAL;
  timer_event.sigev_signo = SIGRTMIN;
  timer_event.sigev_value.sival_ptr = (void *) &tt;
  if (timer_create (CLOCK_REALTIME, &timer_event, &tt) < 0)
  {
    perror ("timer_create failed");
    exit (-1);
  }
  if (timer_settime (tt, 0, &timer, NULL) < 0)
  {
    perror ("timer_settime failed");
    exit (-1);
  }

  while (i++ < count)
  {
    pause ();
  }

  int max, avg;
  mpu6000_get_read_gy_stat(&max, &avg);
	
  printf("max read latency is %d uS, average read latency is %d uS\n", max, avg);

/*
  for (i= 1; i< MIN(count, STAT_LENGTH); i++) {
	printf("time diff is %lld \n", time_diff_stat[i]);
  }
*/
  return 0;
}

