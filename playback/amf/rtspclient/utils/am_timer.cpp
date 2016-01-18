#if NEW_RTSP_CLIENT

#include "am_timer.h"
#include "timer_manager.h"
#include "_time.h"

CTimer::CTimer(unsigned int vinterval,void (*vfunc)(CTimer *,void *),void *vdata,TimerType vtype):
    m_interval(vinterval),isSort(false),m_state(TIMER_IDLE),m_type(vtype),
    m_func(vfunc),m_data(vdata)
{}

CTimer::~CTimer(){
    if(m_state==TIMER_ALIVE)
        stop();
}

void CTimer::start(){
    CTimerManager::instance()->add_timer(this);
}

void CTimer::stop(){
    CTimerManager::instance()->remove_timer(this);
}

void CTimer::restart(unsigned int vinterval){
    CTimerManager::instance()->remove_timer(this);
    if(vinterval!=0){
        m_interval=vinterval;
    }
    CTimerManager::instance()->add_timer(this);
}

#endif//NEW_RTSP_CLIENT

