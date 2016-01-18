#ifndef _RTMP_BUFFER_H__
#define _RTMP_BUFFER_H__

#include <string.h>
#include "../librtmp/rtmp.h"
#include "../librtmp/log.h"
#include "rtmpclient_impl.h"
#include "rtmp_injector.h"

class RtmpBuffer{
public:
    RtmpBuffer(IRtmpEventHandler * handler);
    ~RtmpBuffer();
    int init(char *const rtmp_url,int bLiveStream = 1,int timeout = 30);
    int append_data(unsigned char *data, unsigned int size);
    int flush_data(int sent = 1);
    int flush_data_force(int sent = 1);
public:
    static unsigned long long dbl2int( double value );
    static unsigned long long get_amf_double( double value );
    void put_byte(unsigned char b);
    void put_be32(unsigned int val );
    void put_be64(unsigned long long val );
    void put_be16(unsigned short val );
    void put_be24(unsigned int val );
    void put_tag(const char *tag );
    void put_amf_string(const char *str );
    void put_amf_double(double d );
    unsigned int tell(){return d_cur;}
    void update_amf_be24(unsigned int value, unsigned int pos);
    void update_amf_byte(unsigned int value, unsigned int pos);
    static char* strDup(char const* str);
private:
    int rtmp_setup(char *const rtmp_url,int bLiveStream,int timeout);
    int rtmp_flush();
private:
    unsigned char *data;
    unsigned int d_cur;
    unsigned int d_max;
    RTMP *rtmp;
    unsigned long long d_total;
private:
    enum {RTMP_PKT_SIZE = 1408};
    //enum {RTMP_PKT_SIZE = 1000};
    IRtmpEventHandler *handler_;
};
#endif //_RTMP_BUFFER_H__

