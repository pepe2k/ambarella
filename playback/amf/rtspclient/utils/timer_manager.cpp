#if NEW_RTSP_CLIENT
#include <stdio.h>
#include <errno.h>
#include "_time.h"
#include "timer_manager.h"

#define delay_time(x) while(select(0,0,0,0,x)<0&&errno==EINTR)

CTimerManager * CTimerManager::m_instance=NULL;
pthread_mutex_t CTimerManager::m_mutex= PTHREAD_MUTEX_INITIALIZER;
unsigned int CTimerManager::mark=0;

CTimerManager * CTimerManager::instance(){
    if(m_instance==NULL){
        pthread_mutex_lock(&m_mutex);
        if(m_instance==NULL){
            m_instance=new CTimerManager();
        }
        pthread_mutex_unlock(&m_mutex);
    }
    return m_instance;
}

CTimerManager::CTimerManager():m_state(TIMER_MANAGER_STOP){
    TAILQ_INIT(&list_);
    pid_ = -1;
}

CTimerManager::~CTimerManager(){
    stop();
    //printf("CTimerManager::~CTimerManager() called\n");
    pthread_mutex_lock(&m_mutex);
    m_instance = NULL;
    pthread_mutex_unlock(&m_mutex);
}

void CTimerManager::add_timer_(CTimer * vtimer){
    if(vtimer->m_state==CTimer::TIMER_ALIVE)
        return;

    struct timeval now,interval;
    vtimer->m_state=CTimer::TIMER_ALIVE;
    vtimer->isSort=false;
    interval.tv_sec=vtimer->m_interval/1000;
    interval.tv_usec=(vtimer->m_interval%1000)*1000;
    gettimeofday(&now,0);
    timeradd(&now,&interval,&vtimer->m_endtime);
    TAILQ_INSERT_HEAD(&list_,vtimer,entry_);
}

void CTimerManager::remove_timer_(CTimer * vtimer){
    if(vtimer->m_state!=CTimer::TIMER_ALIVE)
        return;
    TAILQ_REMOVE(&list_,vtimer,entry_);
    vtimer->m_state=CTimer::TIMER_IDLE;
}

void CTimerManager::add_timer(CTimer * vtimer){
    pthread_mutex_lock(&m_mutex);
    add_timer_(vtimer);
    vtimer->id_=++mark;
    pthread_mutex_unlock(&m_mutex);
}

void CTimerManager::remove_timer(CTimer * vtimer){
    pthread_mutex_lock(&m_mutex);
    remove_timer_(vtimer);
    vtimer->m_state=CTimer::TIMER_IDLE;
    pthread_mutex_unlock(&m_mutex);
}

#include "pthread_utils.h"
void CTimerManager:: start(unsigned long interval,unsigned long repair){
    m_interval.tv_sec=interval/1000;
    m_interval.tv_usec=(interval%1000)*1000;
    m_repair.tv_sec=repair/1000;
    m_repair.tv_usec=(repair%1000)*1000;
    if(m_state==TIMER_MANAGER_STOP){
        m_state=TIMER_MANAGER_START;
        pthread_create_rr(&pid_,process,this,50,0);
    }
}

void CTimerManager:: stop(){
    if(m_state==TIMER_MANAGER_START){
        m_state=TIMER_MANAGER_STOP;
        pthread_join(pid_,NULL);
    }
}

void CTimerManager::dump(){
}

void* CTimerManager:: process(void * arg){
    CTimerManager *manager=(CTimerManager *)arg;
    CTimer *item,*reverse_item,*new_item;
    struct timeval now,stand,last,next;
    struct timeval tm;
    CTimer tmpTimer;

    gettimeofday(&last,0);
    while(manager->m_state==TIMER_MANAGER_START){
        gettimeofday(&now,0);
        timeradd(&last,&manager->m_interval,&next);
        timersub(&next,&now,&tm);
        if(tm.tv_sec<0){
            tm=manager->m_interval;
        }
        delay_time(&tm);
        gettimeofday(&now,0);
        last=now;
        timeradd(&now,&manager->m_repair,&stand);

        pthread_mutex_lock(&manager->m_mutex);
        TAILQ_FOREACH(item, &(manager->list_), entry_)
        {
            if(item->isSort==false&&item->m_type==CTimer::TIMER_ONCE){
                tmpTimer.entry_=item->entry_;
                TAILQ_FOREACH_REVERSE(reverse_item, &(manager->list_), entry_, CTimerHead)
                {
                    if(item==reverse_item){
                        item->isSort=true;
                        break;
                    }
                    if(reverse_item->isSort==false||timercmp(&item->m_endtime,&reverse_item->m_endtime,>=)){
                        TAILQ_REMOVE(&(manager->list_),item, entry_);
                        TAILQ_INSERT_AFTER(&(manager->list_), reverse_item, item, entry_);
                        item->isSort=true;
                        break;
                   }
               }
               item=&tmpTimer;
           }else if(item->isSort==false&&item->m_type==CTimer::TIMER_CIRCLE){
               tmpTimer.entry_=item->entry_;
               TAILQ_FOREACH(new_item, &(manager->list_), entry_)
               {
                   if(new_item->isSort==true&&timercmp(&item->m_endtime,&new_item->m_endtime,<=)){
                       TAILQ_REMOVE(&(manager->list_),item, entry_);
                       TAILQ_INSERT_BEFORE(new_item, item, entry_);
                       item->isSort=true;
                       break;
                   }
               }
               if(item->isSort==false){
                   TAILQ_REMOVE(&(manager->list_),item, entry_);
                   TAILQ_INSERT_TAIL(&(manager->list_), item, entry_);
                   item->isSort=true;
               }
               item=&tmpTimer;
           }else{
               break;
           }
        }
        TAILQ_FOREACH(item, &(manager->list_), entry_)
        {
            if(timercmp(&item->m_endtime,&stand,<)){
                if(item->m_func){
                    manager->run_pool_.add(item->m_func, item->m_data);
                }
                if(item->m_type==CTimer::TIMER_ONCE){
                    manager->remove_timer_(item);
                    item->m_state=CTimer::TIMER_TIMEOUT;
                }else if(item->m_type==CTimer::TIMER_CIRCLE)	{
                    tmpTimer.entry_=item->entry_;
                    manager->remove_timer_(item);
                    manager->add_timer_(item);
                    item=&tmpTimer;
                }
            }	else	{
                break;
            }
        }
        pthread_mutex_unlock(&manager->m_mutex);
        manager->run_pool_.execute();
        manager->run_pool_.clean();
    }
    return (void*)0;
}

#endif //NEW_RTSP_CLIENT
