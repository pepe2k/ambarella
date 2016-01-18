#ifndef __AO_INTERNAL_H_
#define __AO_INTERNAL_H_

#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <pthread.h>
#include "active_object.h"

class AO_Queue{
public:
    virtual ~AO_Queue(){};
    virtual Method_Request *getq() = 0;
    virtual void putq(Method_Request *request) = 0;
};

class AO_Queue_OneLock:public AO_Queue{
public:
    AO_Queue_OneLock(int queueSize=1024);
    ~AO_Queue_OneLock();
    Method_Request* getq();
    void putq(Method_Request *request);
private:
    pthread_mutex_t mux;
    pthread_cond_t condGet;
    pthread_cond_t condPut;
    void **buffer;
    int sizeQueue;	//length of queue
    int lput;		//location put
    int lget;		//location get
    int nFullThread;	//when queue is full, the number of thread blocked in putq
    int nEmptyThread;//when queue is empty, the number of thread blocked in getq
    int nData;	//data number in queue ,using to jusge queue  is full or empty 
};

template <typename Object>
class Vector{
 public:
     explicit Vector( int initSize = 0 )
         : theSize( initSize ), theCapacity( initSize + SPARE_CAPACITY )
         { objects = new Object[ theCapacity ]; }
    Vector( const Vector & rhs ) : objects( NULL ) { operator=( rhs ); }
     ~Vector( ) { delete [ ] objects; }
    const Vector & operator= ( const Vector & rhs )
    {
        if( this != &rhs )
        {
             delete [ ] objects;
             theSize = rhs.size( );
             theCapacity = rhs.theCapacity;
             objects = new Object[ capacity( ) ];
             for( int k = 0; k < size( ); k++ )
                 objects[ k ] = rhs.objects[ k ];
         }
         return *this;
    }

    void resize( int newSize )
    {
        if( newSize > theCapacity )
            reserve( newSize * 2 + 1 );
            theSize = newSize;
    }

    void reserve( int newCapacity )
    {
        if( newCapacity < theSize )
            return;

        Object *oldArray = objects;
        objects = new Object[ newCapacity ];
        for( int k = 0; k < theSize; k++ )
            objects[ k ] = oldArray[ k ];

        theCapacity = newCapacity;

        delete [ ] oldArray;
    }

    Object & operator[]( int index ) { return objects[ index ]; }
    const Object & operator[]( int index ) const  { return objects[ index ]; }
    bool empty( ) const { return size( ) == 0; }
    int size( ) const { return theSize; }
    int capacity( ) const { return theCapacity; }
    void push_back( const Object & x )
    {
        if( theSize == theCapacity )
            reserve( 2 * theCapacity + 1 );
            objects[ theSize++ ] = x;
    }
    void pop_back( ){ theSize--; }
    const Object & back ( ) const { return objects[ theSize - 1 ]; }

    typedef Object * iterator;
    typedef const Object * const_iterator;

    iterator begin( ){ return &objects[ 0 ]; }
    const_iterator begin( ) const { return &objects[ 0 ]; }
    iterator end( ) { return &objects[ size( ) ]; }
    const_iterator end( ) const { return &objects[ size( ) ]; }

    enum { SPARE_CAPACITY = 16 };
private:
    int theSize;
    int theCapacity;
    Object * objects;
};

class AO_Scheduler{
public:
    enum ThreadState{
        THREAD_STOP=0,
        THREAD_START
    };
    AO_Scheduler(int queue_length=1024);
    virtual ~AO_Scheduler();
    void start(int thread_num = 1,int cpu_id = 0xff);
    void stop();
    void putq(Method_Request *request);
protected:
    static void* run(void *arg);
private:
    AO_Queue *queue_;
    ThreadState state_;
    Vector<pthread_t> pids_;
};

#endif //NEW_RTSP_CLIENT
#endif //__AO_INTERNAL_H_

