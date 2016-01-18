/**
 * common_network_utils.h
 *
 * History:
 *	2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2012, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __COMMON_NETWORK_UTILS_H__
#define __COMMON_NETWORK_UTILS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DNETUTILS_RECEIVE_FLAG_READ_ALL 0x08000000

//network related
extern TInt gfNet_ConnectTo(const TChar *host_addr, TU16 port, TInt type, TInt protocol);
extern TInt gfNet_Send(TInt fd, TU8* data, TInt len, TUint flag);
extern TInt gfNet_Send_timeout(TInt fd, TU8* data, TInt len, TUint flag, TInt max_count);
extern TInt gfNet_Recv(TInt fd, TU8* data, TInt len, TUint flag);
extern TInt gfNet_Recv_timeout(TInt fd, TU8* data, TInt len, TUint flag, TInt max_count);
extern TInt gfNet_SendTo(TInt fd, TU8* data, TInt len, TUint flag, const struct sockaddr *to, socklen_t tolen);
extern TInt gfNet_RecvFrom(TInt fd, TU8* data, TInt len, TUint flag, const struct sockaddr *from, socklen_t* fromlen);

extern TInt gfNet_SetupStreamSocket(TU32 localAddr,  TU16 localPort, TU8 makeNonBlocking);
extern TInt gfNet_SetupDatagramSocket(TU32 localAddr,  TU16 localPort, TU8 makeNonBlocking, TU32 request_receive_buffer_size, TU32 request_send_buffer_size);

//-----------------------------------------------------------------------
//
// CITCPPulseSender
//
//-----------------------------------------------------------------------
typedef enum
{
    EPulseSenderState_idle,
    EPulseSenderState_running,
    EPulseSenderState_error,
} EPulseSenderState;

class CITCPPulseSender: public CObject, public IActiveObject
{
typedef CObject inherited;

public:
    static CITCPPulseSender* Create();

protected:
    CITCPPulseSender();
    virtual ~CITCPPulseSender();
    EECode Construct();

public:
    STransferDataChannel* AddClient(TInt fd, TMemSize max_send_buffer_size = (1024*1024), TInt framecount = 60);
    void RemoveClient(STransferDataChannel* channel);

    EECode SendData(STransferDataChannel* channel, TU8* pdata, TMemSize data_size);

public:
    EECode Start();
    EECode Stop();

protected:
    virtual void OnRun();

private:
    void processCmd(SCMD& cmd);
    void removeClient(STransferDataChannel* channel);
    STransferDataChannel* allocDataChannel(TMemSize size, TInt framecount);
    void releaseDataChannel(STransferDataChannel* data_channel);
    void purgeChannel(STransferDataChannel* channel);
    EECode sendDataPiece(TInt fd, TU8* p_data, TInt data_size);
    EECode tryAllocDataPiece(STransferDataChannel* pchannel, TU8* p_data, TMemSize data_size);
    EECode sendDataChannel(STransferDataChannel* channel);
    void deleteDataChannel(STransferDataChannel* channel);
    void deleteAllDataChannel();

private:
    CIMutex* mpMutex;
    CIWorkQueue* mpWorkQueue;

private:
    CIDoubleLinkedList mTransferDataChannelList;
    CIDoubleLinkedList mTransferDataFreeList;

private:
    TInt mPipeFd[2];
    TInt mMaxFd;

    fd_set mAllReadSet;
    fd_set mAllWriteSet;
    fd_set mReadSet;
    fd_set mWriteSet;

private:
    TU8 mbRun;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

    EPulseSenderState msState;

private:
    TU32 mDebugHeartBeat;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

