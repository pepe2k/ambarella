#ifndef _X_TCP_BUFFER_H_
#define _X_TCP_BUFFER_H_

#if NEW_RTSP_CLIENT

#include <stdlib.h>
#include <string>
using std::string;
#define INIT_CAPACITY 512
class XTcpBuffer
{
public:
    XTcpBuffer(int capacity=INIT_CAPACITY):capacity_(capacity),loc_(0){
        base_=(char *)malloc(capacity+1);
        *(base_+loc_)='\0';
    }
    void add_capacity(){
        capacity_+=capacity_*2;
        base_=(char *)realloc(base_,capacity_+1);
        *(base_+loc_)='\0';
    }
    char * base() const {return base_;}
    char * top() const {return base_+loc_;}
    void add_loc(int loc){loc_+=loc;*(base_+loc_)='\0';}
    void reset_loc(int loc){loc_=loc;*(base_+loc_)='\0';}
    int capacity() const {return capacity_-loc_;}
    string tostr() const {return string(base_,0,loc_);}
    ~XTcpBuffer(){free(base_);}
private:
    char *base_;
    int capacity_;
    int loc_;
};
#endif//NEW_RTSP_CLIENT

#endif

