#ifndef __PTHREAD_UTILS_H_
#define __PTHREAD_UTILS_H_

#if NEW_RTSP_CLIENT

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int pthread_create_rr(pthread_t *pid,void*(*start_rtn)(void*),void *arg,int priority = 1, int cpu_id = 0xff){
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &param);
    //
    if(cpu_id != 0xff){
        cpu_set_t cpu_info;
        CPU_ZERO(&cpu_info);
        CPU_SET(cpu_id, &cpu_info);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_info);
    }
    int err = pthread_create(pid, &attr, start_rtn, (void*)arg);
    pthread_attr_destroy(&attr);
    return err;
}

#endif //NEW_RTSP_CLIENT
#endif //__PTHREAD_UTILS_H_

