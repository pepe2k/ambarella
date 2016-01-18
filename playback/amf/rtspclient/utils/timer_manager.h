#ifndef __CTIMER_MANGER_H_
#define __CTIMER_MANGER_H_

#if NEW_RTSP_CLIENT

#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/times.h>
#include "am_timer.h"

#define DEFULT_INTERVAL 1000
#define DEFAUL_REPAIR 20
struct RunElement
{
	void (*func)(CTimer *,void *);
	void * data;
	RunElement * next;
};
/*object pool*/
class TimerRunList{
public:
    TimerRunList(){
         head.next=NULL;
         tail=&head;
    }
    void add(void (*func)(CTimer *,void *),void * data){
        if(tail->next==NULL){
            RunElement * element=new RunElement;
            element->func=func;
            element->data=data;
            element->next=NULL;
            tail->next=element;
            tail=element;
        }else{
            RunElement * element=tail->next;
            element->func=func;
            element->data=data;
            tail=element;
        }
    }
    void execute(){
        RunElement * elem;
        for(elem=head.next;elem!=tail->next;elem=elem->next){
            elem->func(0,elem->data);
        }
    }
    void clean(){
        tail=&head;
    }
private:
    RunElement head;
    RunElement *tail;
};

class CTimerManager
{
public:
    typedef enum{
        TIMER_MANAGER_STOP=0,
        TIMER_MANAGER_START
    }TimerManagerState;
    static CTimerManager * instance();
    void del_self(){
        stop();
        delete this;
    }
    void add_timer(CTimer * vtimer);
    void remove_timer(CTimer * vtimer);
    void start(unsigned long interval=DEFULT_INTERVAL,unsigned long repair=DEFAUL_REPAIR);
    void stop();
    void dump();
protected:
    static void* process(void *);
private:
    void add_timer_(CTimer * vtimer);
    void remove_timer_(CTimer * vtimer);
    void run_func();

    CTimerManager();
    ~CTimerManager();
    static CTimerManager * m_instance;
    static pthread_mutex_t m_mutex;

    volatile TimerManagerState m_state;
    struct timeval m_interval;
    struct timeval m_repair;
    TAILQ_HEAD(CTimerHead,CTimer) list_;

    static unsigned int mark;
    TimerRunList run_pool_;
    pthread_t pid_;
};

#endif //NEW_RTSP_CLIENT
#endif

