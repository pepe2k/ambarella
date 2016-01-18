#ifndef _MISC_UTILS_H__
#define _MISC_UTILS_H__

#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <signal.h>
#define KILL_SELF()  do{ kill(getpid(),9);}while(0)

/*
#include <stdio.h>
#include <stdlib.h>
#define KILL_SELF() do{\
        printf("KILL_SELF --- %s:%d\n",__FILE__,__LINE__);\
    }while(0)
*/

#endif //NEW_RTSP_CLIENT

#endif //_MISC_UTILS_H__

