#ifndef _AM_TIMER_H_
#define _AM_TIMER_H_

#if NEW_RTSP_CLIENT

#include <sys/time.h>
#include <sys/times.h>
#include "queue.h"

class CTimer;
typedef void (*timer_vfunc)(CTimer *,void *);

class CTimer
{
    friend class CTimerManager;
public:
    typedef enum{
        TIMER_IDLE=0,
        TIMER_ALIVE,
        TIMER_TIMEOUT
    }TimerState;

    typedef enum{
        TIMER_ONCE=0,
        TIMER_CIRCLE
    }TimerType;

    CTimer(unsigned int vinterval=0,timer_vfunc vfunc=0,void *vdata=0,TimerType vtype=TIMER_ONCE);
    ~CTimer();
    void start();
    void stop();
    void restart(unsigned int vinterval=0);
    void set_interval(unsigned int vinterval){m_interval=vinterval;}
    void set_func_data(timer_vfunc vfunc,void *vdata){m_func = vfunc;m_data = vdata;}
    TimerState state()const{return m_state;}
private:
    unsigned int id_;
    unsigned long m_interval;
    bool isSort;
    struct timeval m_endtime;
    TimerState m_state;
    TimerType m_type;
    timer_vfunc m_func;
    void * m_data;
    TAILQ_ENTRY(CTimer) entry_;
};

#endif //NEW_RTSP_CLIENT
#endif //_AM_TIMER_H_

