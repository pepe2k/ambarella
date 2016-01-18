#ifndef _BASIC_USAGE_ENVIRONMENT1_HH
#define _BASIC_USAGE_ENVIRONMENT1_HH

#if defined(__WIN32__) || defined(_WIN32)
#include <process.h>

#define sleep(x) Sleep(1000*x)
#define pthread_self() GetCurrentThreadId()
#define pthread_create(x1,x2,x3,x4) _beginthreadex(NULL,0,x3,x4,0,x1)
#define pthread_join(x,y) WaitForSingleObject((HANDLE)x, INFINITE)
typedef unsigned ( __stdcall * pthread_func_t )( void * args );

#define THREAD_TYPE unsigned __stdcall

typedef unsigned pthread_t;
typedef HANDLE  pthread_mutex_t;
typedef HANDLE  pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER CreateMutex( NULL, FALSE, NULL )

int inline pthread_mutex_init( pthread_mutex_t * mutex, void * attr )
{	
	*mutex = CreateMutex( NULL, FALSE, NULL );	
	return NULL == * mutex ? GetLastError() : 0;
}
int inline pthread_mutex_destroy( pthread_mutex_t * mutex )
{	
	int ret = CloseHandle( *mutex );	
	return 0 == ret ? GetLastError() : 0;
}
int inline pthread_mutex_lock( pthread_mutex_t * mutex )
{	
	int ret = WaitForSingleObject( *mutex, INFINITE );	
	return WAIT_OBJECT_0 == ret ? 0 : GetLastError();
}
int inline pthread_mutex_unlock( pthread_mutex_t * mutex )
{	
	int ret = ReleaseMutex( *mutex );	
	return 0 != ret ? 0 : GetLastError();
}
int inline pthread_cond_init( pthread_cond_t * cond, void * attr )
{	
	*cond = CreateEvent( NULL, FALSE, FALSE, NULL );	
	return NULL == *cond ? GetLastError() : 0;
}
int inline pthread_cond_destroy( pthread_cond_t * cond )
{	
	int ret = CloseHandle( *cond );	
	return 0 == ret ? GetLastError() : 0;
}
int inline pthread_cond_wait( pthread_cond_t * cond, pthread_mutex_t * mutex )
{	
	int ret = 0;	
	pthread_mutex_unlock( mutex );	
	ret = WaitForSingleObject( *cond, INFINITE );	
	pthread_mutex_lock( mutex );	
	return WAIT_OBJECT_0 == ret ? 0 : GetLastError();
}
int inline pthread_cond_signal( pthread_cond_t * cond )
{
	int ret = SetEvent( *cond );	
	return 0 == ret ? GetLastError() : 0;
}

struct timespec{
	unsigned int tv_sec;
	unsigned int tv_nsec;
};
int inline pthread_cond_timedwait( pthread_cond_t * cond, pthread_mutex_t * mutex,const struct timespec * abstime)
{	
	int ret = 0;
	int mseconds = 2000;
	pthread_mutex_unlock( mutex );	
	ret = WaitForSingleObject( *cond, mseconds);	
	pthread_mutex_lock( mutex );	
	return WAIT_OBJECT_0 == ret ? 0 : GetLastError();
}

#else
#include <pthread.h>
#define THREAD_TYPE void *
#endif

#endif //_BASIC_USAGE_ENVIRONMENT1_HH
