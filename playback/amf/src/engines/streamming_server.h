
/**
 * streamming_server.h
 *
 * History:
 *    2011/09/06 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __STREAMMING_SERVER_H__
#define __STREAMMING_SERVER_H__

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

class CStreammingServerManager: public CObject, public IStreammingServerManager, public IActiveObject, public IRtspCallback
{
    typedef CObject inherited;

protected:
    CStreammingServerManager(SConsistentConfig* pconfig);
    virtual ~CStreammingServerManager()
    {
        pthread_mutex_destroy(&mTcpSendLock);
        //todo
        AM_ASSERT(0);
    }

    AM_ERR Construct();

    enum {
        CMD_SET_PIPEFD = CMD_LAST,
        CMD_ADD_SERVER,
        CMD_REMOVE_SERVER,
        CMD_ADD_CONTENT,
    };

public:
    static IStreammingServerManager* Create(SConsistentConfig* pconfig);

public:
    //CObject
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    //IActiveObject
    virtual const char *GetName() {return mpName;}
    virtual void OnRun();
    virtual void OnCmd(CMD& cmd) { /*todo*/ }

    //IStreammingServer
    virtual AM_ERR StartServerManager();
    virtual AM_ERR StopServerManager();

    virtual SStreammingServerInfo* AddServer(IParameters::StreammingServerType type, IParameters::StreammingServerMode mode, AM_UINT server_port, AM_U8 enable_video, AM_U8 enable_audio);
    virtual AM_ERR RemoveServer(SStreammingServerInfo* server);
    virtual AM_ERR AddStreammingContent(SStreammingServerInfo* server_info, SStreamContext* content);
//debug api
    virtual AM_ERR PrintState();

    bool ProcessCmd(CMD& cmd);
    void checkClientsTimeout();
    void doCheckClientsTimeout(SStreammingServerInfo* server_info);
    void ScanClientList(SStreammingServerInfo* server_info, AM_INT& nready);

    AM_ERR CloseServer(SStreammingServerInfo* server);

protected:
    AM_ERR HandleServerRequest(SStreammingServerInfo* server, EServerAction& action);
    AM_ERR HandleClientRequest(SStreammingServerInfo*server_info, SClientSessionInfo* p_client, ESessionMethod& method);

//    AM_ERR checkServerValid(SStreammingServerInfo* p_server);
//    AM_ERR checkClientValid(SClientSessionInfo* p_client);
    bool parseRTSPRequestString(char const* reqStr, AM_U32 reqStrSize, char* resultCmdName, AM_U32 resultCmdNameMaxSize, char* resultURLPreSuffix,
            AM_U32 resultURLPreSuffixMaxSize, char* resultURLSuffix, AM_U32 resultURLSuffixMaxSize, char* resultCSeq, AM_U32 resultCSeqMaxSize);

protected:
    void handleCmd_bad(char const* /*cseq*/);
    void handleCmd_notSupported(char const* cseq);
    void handleCmd_notFound(char const* cseq);
    void handleCmd_unsupportedTransport(char const* cseq);
    void handleCmd_OPTIONS(char const* cseq);
    void handleCmd_DESCRIBE(SClientSessionInfo* p_client, char const* cseq, char const* urlSuffix, char const* fullRequestStr);
    void handleCmd_SETUP(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cseq, char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr);
    int handleCmd_PLAY(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cseq, char const* fullRequestStr);
    void handleCmd_TEARDOWN(SClientSessionInfo* p_client, char const* cseq, char const* fullRequestStr);
    void handleCmd_GET_PARAMETER(SClientSessionInfo* p_client, char const* cseq, char const* fullRequestStr);
    void handleCmd_SET_PARAMETER(SClientSessionInfo* p_client, char const* cseq, char const* /*fullRequestStr*/);
    AM_ERR handleCmd_withinSession(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cmdName, char const* urlPreSuffix, char const* urlSuffix, char const* cseq, char const* fullRequestStr);

private:
    void deleteClientSession(SClientSessionInfo* p_session);
    void deleteSDPSession(void* );
    void deleteServer(SStreammingServerInfo* p_server);

protected:
    CWorkQueue* mpWorkQueue;
    const char *mpName;
    bool mbRun;

protected:
    IStreammingContentProvider* mpContentProvider;

private:
    AM_INT mPipeFd[2];
    AM_INT mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;

    CDoubleLinkedList mServerList;
    AM_UINT mnServerNumber;
    EServerManagerState msState;

    pthread_mutex_t mTcpSendLock;
    AM_S64 mLastCheckTime;
public:
    void OnRtpRtcpPackets(int fd,unsigned char *buf, int len,volatile int *force_exit_flag){
        //AMLOG_WARN("OnRtpRtcpPackets --len = %d, data[0~3] %x,%x,%x,%x\n",len,(int)buf[0],(int)buf[1],(int)buf[2],(int)buf[3]);
        pthread_mutex_lock(&mTcpSendLock);
        int bytes = 0;
        while(bytes < len){
            if(*force_exit_flag) {
                break;
            }
            int ret = send(fd,buf + bytes, len - bytes,0);
            if(ret < 0){
                int err = errno;
                if(err == EINTR) continue;
                if(err == EAGAIN || err == EWOULDBLOCK){
                    usleep(1000);
                    continue;
                }
                pthread_mutex_unlock(&mTcpSendLock);
                AMLOG_WARN("OnRtpRtcpPackets send failed,errno %d\n",err);
                return;
            }
            bytes += ret;
        }
        pthread_mutex_unlock(&mTcpSendLock);
    }
private:
    AM_S64  get_time(void)
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    }
    int  OnRtspRequest(SStreammingServerInfo*server_info, SClientSessionInfo* p_client, ESessionMethod  method,unsigned char *buf,int len){
        if(buf[0] == '$'){
            //embedded data
            pthread_mutex_lock(&p_client->lock);
            p_client->last_cmd_time = get_time();
            pthread_mutex_unlock(&p_client->lock);
            return ME_OK;
        }else{
            memcpy(mRequestBuffer,buf,len);
            mRequestBuffer[len] = '\0';
            mRequestBufferLen = len;
            return HandleClientRequest(server_info, p_client, method);
        }
    }

    int  handle_rtsp_request(SStreammingServerInfo*server_info, SClientSessionInfo* p_client, ESessionMethod& method){
        while(1){
            int bytesRead = recv(p_client->socket,p_client->mRecvBuffer + p_client->mRecvLen,RTSP_MAX_BUFFER_SIZE - 1 - p_client->mRecvLen,0);
            if(bytesRead < 0){
                int err = errno;
                if(err == EINTR || err == EAGAIN || err == EWOULDBLOCK)
                    continue;
                AMLOG_WARN("handle_rtsp_request, recv errno %d.\n",errno);
                return ME_CLOSED;
            }else if(bytesRead == 0){
                AMLOG_WARN("handle_rtsp_request, recv 0,  Client EOF, recvLen = %d\n",p_client->mRecvLen);
                return ME_CLOSED;
            }
            p_client->mRecvLen += bytesRead;
            break;
        }
        if(p_client->rtp_over_rtsp){
            int tmp = 0;
            setsockopt(p_client->socket, IPPROTO_TCP, TCP_QUICKACK, (void *)&tmp, sizeof(tmp));
        }
        p_client->mRecvBuffer[p_client->mRecvLen] = '\0';

        if(p_client->mRecvLen > 4){
            int parse_len = parse_request(server_info,p_client,method);
            if(parse_len < 0){
                return ME_CLOSED; //send data failed
            }
            if(parse_len < p_client->mRecvLen){
                memmove(p_client->mRecvBuffer,p_client->mRecvBuffer + parse_len,p_client->mRecvLen - parse_len);
            }
            p_client->mRecvLen -= parse_len;
        }
        //request message not complete, continue to read
        return ME_TRYNEXT;
    }
    int  tcp_write(int fd, const char *buf,int len){
        pthread_mutex_lock(&mTcpSendLock);
        int bytes = 0;
        while(bytes < len){
            int ret = send(fd,buf + bytes, len - bytes,0);
            if(ret < 0){
                int err = errno;
                if(err == EINTR) continue;
                if(err == EAGAIN || err == EWOULDBLOCK){
                    usleep(1000);
                    continue;
                }
                pthread_mutex_unlock(&mTcpSendLock);
                AMLOG_WARN("tcp_write failed, %s --- errno %d\n",buf,err);
                return -1;
            }
            bytes += ret;
        }
        pthread_mutex_unlock(&mTcpSendLock);
        return 0;
    }

    char mRequestBuffer[RTSP_MAX_BUFFER_SIZE];
    int mRequestBufferLen;
    char mResponseBuffer[RTSP_MAX_BUFFER_SIZE];
    char mDateBuffer[RTSP_MAX_DATE_BUFFER_SIZE];
private:
    CMutex* mpMutex;
private:
    unsigned short mRtspSessionId;
    AM_UINT mRtspSSRC;

protected:
    SConsistentConfig* mpConfig;
private:
    //parse rtsp request
    enum e_parse_state {
        STATE_ERROR,
        STATE_IDLE,
        STATE_N_1,
        STATE_R_2,
        STATE_N_2,
        STATE_CHANNEL,
        STATE_LEN_1,
        STATE_LEN_2,
        STATE_BINARY
    };
    int parse_request(SStreammingServerInfo*server_info, SClientSessionInfo* p_client, ESessionMethod& method)
    {
        unsigned char *buf = (unsigned char *)p_client->mRecvBuffer;
        int len = p_client->mRecvLen;
        //AMLOG_WARN("doParseRequest:recv_len[%d]%s\n", len,buf);

        int mDataLen = 0;
        e_parse_state mState = STATE_IDLE;
        unsigned char *packet = NULL;
        int parse_len = 0;

        for(int i = 0; i < len; i ++){
            if(!packet) packet = &buf[i];
            char c = buf[i];
            switch(mState){
            case STATE_IDLE:
                {
                    if(c == '$')  mState = STATE_CHANNEL;
                    else if(c == '\r') mState = STATE_N_1;
                }break;
            case STATE_N_1:
                {
                    if(c == '\n') mState = STATE_R_2;
                    else mState = STATE_ERROR;
                }break;
            case STATE_R_2:
                {
                    if(c == '\r') mState = STATE_N_2;
                    else mState = STATE_IDLE;
                }break;
            case STATE_N_2:
                {
                    if(c == '\n'){
                        if(OnRtspRequest(server_info,p_client,method,packet,&buf[i] -packet + 1) != ME_OK){
                            return -1;
                        }
                        parse_len = i + 1;
                        packet = NULL;
                        mState = STATE_IDLE;
                    }
                    else mState = STATE_ERROR;
                }break;
            case STATE_CHANNEL:
                {
                    mState = STATE_LEN_1;
                }break;
            case STATE_LEN_1:
                {
                    mDataLen = buf[i];
                    mState = STATE_LEN_2;
                }break;
            case STATE_LEN_2:
                {
                    mDataLen = ((mDataLen << 8) & 0xff00) + buf[i];
                    mState = STATE_BINARY;
                }break;
            case STATE_BINARY:
                {
                    --mDataLen;
                    if(!mDataLen){
                        if(OnRtspRequest(server_info,p_client,method,packet,&buf[i] -packet + 1) != ME_OK){
                            return -1;
                        }
                        parse_len = i + 1;
                        packet = NULL;
                        mState = STATE_IDLE;
                    }
                }break;
            case STATE_ERROR:
            default:
                {
                    //what should we do?
                    AMLOG_WARN("doParseRequest: STATE_ERROR\n");
                    parse_len = i + 1;
                }break;
            }
        }
        //AMLOG_WARN("doParseRequest: parse_len = %d\n",parse_len);
        return parse_len;
    }

};

#endif

