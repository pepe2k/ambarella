#include "rtmp_buffer.h"

RtmpBuffer::RtmpBuffer(IRtmpEventHandler *handler)
    :data(NULL),d_cur(0),d_max(0),rtmp(NULL),d_total(0),handler_(handler){
    this->data = new unsigned char[320 * 1024 ];
    this->d_max = 320 * 1024;
}
RtmpBuffer::~RtmpBuffer(){
    if(data){
        delete []data,data = NULL;
    }
    handler_ = NULL;
    if(rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = NULL;
        RTMP_LogPrintf("RTMP_Close & RTMP_Free\n");
    }
}
int RtmpBuffer::init(char *const rtmp_url,int bLiveStream,int timeout){
    if(rtmp_setup(rtmp_url,bLiveStream,timeout) < 0){
        return -1;
    }
    return 0;
}

int RtmpBuffer::rtmp_setup(char *const rtmp_url,int bLiveStream, int timeout ){
    this->rtmp = RTMP_Alloc();
    if(!this->rtmp){
        return -1;
    }
    RTMP_Init(this->rtmp);

    char *optarg = rtmp_url;
    int protocol = RTMP_PROTOCOL_UNDEFINED;//1*
    AVal hostname = { 0, 0 };//2*
    unsigned int port = 1935;//3*
    AVal sockshost = { 0, 0 };//4
    AVal playpath = { 0, 0 };//5*
    AVal tcUrl = { 0, 0 };//6
    AVal swfUrl = { 0, 0 };//7
    AVal pageUrl = { 0, 0 };//8
    AVal app = { 0, 0 };//9*
    AVal auth = { 0, 0 };//10
    AVal swfHash = { 0, 0 };//11
    unsigned int  swfSize = 0;//12
    AVal flashVer = { 0, 0 };//13
    AVal subscribepath = { 0, 0 };//14
	AVal usherToken = { 0, 0 };//15
    unsigned int dSeek = 0;//16
    unsigned int dStopOffset = 0;//17
    if (!RTMP_ParseURL(optarg, &protocol, &hostname, &port,&playpath, &app)){
        RTMP_Log(RTMP_LOGWARNING, "Couldn't parse the specified url (%s)!",optarg);
        return -1;
    }
    if (tcUrl.av_len == 0){
        char str[512] = { 0 };
        tcUrl.av_len = snprintf(str, 511, "%s://%.*s:%d/%.*s",RTMPProtocolStringsLower[protocol], hostname.av_len,hostname.av_val, port, app.av_len, app.av_val);
        tcUrl.av_val = strDup(str);
    }

    RTMP_SetupStream(this->rtmp, protocol, &hostname, port, &sockshost, &playpath,
                      &tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
                      &flashVer, &subscribepath,&usherToken, dSeek, dStopOffset, bLiveStream, timeout);

    /*The client sends this event to inform the server of the buffer size (in milliseconds) 
	*  that is used to buffer any data coming over a stream. This event is sent before the 
	*  server starts processing the stream.
    */
    uint32_t bufferTime = 10 * 3600 * 1000;
    RTMP_SetBufferMS(this->rtmp, bufferTime);

    RTMP_EnableWrite(this->rtmp); //PUBLISH

    if (!RTMP_Connect(this->rtmp, NULL)){
        RTMP_LogPrintf("RTMP_Connect failed!\n");
        return -1;
    }
    RTMP_LogPrintf("RTMP_Connect OK!\n");
    if (!RTMP_ConnectStream(this->rtmp, dSeek)){
        RTMP_LogPrintf("RTMP_ConnectStream failed!\n");
        return -1;
    }
    RTMP_LogPrintf("RTMP_ConnectStream OK!\n");

    //set RTMP ChunkSize
    RTMPPacket pack;
    RTMPPacket_Alloc(&pack, 4);
    pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
    pack.m_nChannel = 0x02;
    pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    pack.m_nTimeStamp = 0;
    pack.m_nInfoField2 = 0;
    pack.m_nBodySize = 4;
    int nVal = RTMP_PKT_SIZE;
    pack.m_body[3] = nVal & 0xff;
    pack.m_body[2] = nVal >> 8;
    pack.m_body[1] = nVal >> 16;
    pack.m_body[0] = nVal >> 24;
    rtmp->m_outChunkSize = nVal;
    RTMP_SendPacket(this->rtmp,&pack,1);
    RTMPPacket_Free(&pack);
    return 0;
}

int RtmpBuffer::rtmp_flush(){
    if(RTMP_IsConnected(this->rtmp)){
        int ret = RTMP_Write(this->rtmp, (char *)this->data, this->d_cur);
        if(ret == -1){
            RTMP_LogPrintf("RTMP_Write() failed\n");
            return -1;
        }
        return 0;
    }
    RTMP_LogPrintf("RTMP_IsConnected() == false, data discarded\n");
    return -1;
}

char* RtmpBuffer::strDup(char const* str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str) + 1;
    char* copy = new char[len];
    if (copy != NULL) {
        memcpy(copy, str, len);
    }
    return copy;
}

int RtmpBuffer::append_data(unsigned char *data, unsigned int size){
    unsigned int ns = this->d_cur + size;
    if( ns > this->d_max ){
        unsigned int dn = 16;
        while( ns > dn ){
            dn <<= 1;
        }
        printf("RtmpBuffer::append_data() --- realloc\n");
        unsigned char *new_data = new unsigned char[dn];
        if(!new_data){
            printf("append_data:Failed to realloc\n");
            return -1;
        }
        memcpy(new_data,this->data,this->d_cur);
        delete []this->data;
        this->data = new_data;
        this->d_max = dn;
    }
    memcpy(this->data + this->d_cur, data, size );
    this->d_cur = ns;
    return 0;
}

int RtmpBuffer::flush_data_force(int sent){
    if(!this->d_cur){
        return 0;
    }
    if(sent){
        if(rtmp_flush() < 0){
            if(handler_){
                handler_->onPeerClose();
            }
            return -1;
        }
    }
    this->d_total += this->d_cur;
    this->d_cur = 0;
    return 0;
}

int RtmpBuffer::flush_data(int sent){
    if(!this->d_cur){
        return 0;
    }
    if(this->d_cur > 1024){
        return flush_data_force(sent);
    }
    return 0;
}

void RtmpBuffer::update_amf_byte(unsigned int value, unsigned int pos){
    *(this->data + pos) = value;
}

void RtmpBuffer::update_amf_be24(unsigned int value, unsigned int pos){
    *(this->data + pos + 0) = value >> 16;
    *(this->data + pos + 1) = value >> 8;
    *(this->data + pos + 2) = value >> 0;
}

unsigned long long RtmpBuffer::dbl2int( double value ){
    union tmp{double f; unsigned long long i;} u;
    u.f = value;
    return u.i;
}

void RtmpBuffer::put_byte(unsigned char b){
    append_data(&b,1);
}

void RtmpBuffer::put_be32(unsigned int val){
    put_byte(val >> 24);
    put_byte(val >> 16);
    put_byte(val >> 8);
    put_byte(val);
}

void RtmpBuffer::put_be64(unsigned long long val){
    put_be32(val >> 32);
    put_be32(val);
}

void RtmpBuffer::put_be16(unsigned short val){
    put_byte(val >> 8 );
    put_byte(val );
}

void RtmpBuffer::put_be24(unsigned int val){
    put_be16(val >> 8 );
    put_byte(val );
}

void RtmpBuffer::put_tag(const char *tag){
    while(*tag ){
        put_byte(*tag++);
    }
}

void RtmpBuffer::put_amf_string(const char *str){
    unsigned short len = strlen(str);
    put_be16(len);
    append_data((unsigned char*)str,len);
}

void RtmpBuffer::put_amf_double(double d){
    put_byte(0/*AMF_DATA_TYPE_NUMBER*/);
    put_be64(dbl2int(d));
}

