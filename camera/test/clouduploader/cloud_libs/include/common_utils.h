/**
 * common_utils.h
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

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CIQueue
{
public:
    struct WaitResult
    {
        CIQueue *pDataQ;
        void *pOwner;
        TUint blockSize;
    };

    enum QType
    {
        Q_MSG,
        Q_DATA,
        Q_NONE,
    };

public:
    static CIQueue* Create(CIQueue *pMainQ, void *pOwner, TUint blockSize, TUint nReservedSlots);
    void Delete();

private:
    CIQueue(CIQueue *pMainQ, void *pOwner);
    EECode Construct(TUint blockSize, TUint nReservedSlots);
    ~CIQueue();

public:
    EECode PostMsg(const void *pMsg, TUint msgSize);
    EECode SendMsg(const void *pMsg, TUint msgSize);

    void GetMsg(void *pMsg, TUint msgSize);
    bool PeekMsg(void *pMsg, TUint msgSize);
    void Reply(EECode result);

    bool GetMsgEx(void *pMsg, TUint msgSize);
    void Enable(bool bEnabled = true);

    EECode PutData(const void *pBuffer, TUint size);

    QType WaitDataMsg(void *pMsg, TUint msgSize, WaitResult *pResult);
    QType WaitDataMsgCircularly(void *pMsg, TUint msgSize, WaitResult *pResult);
    QType WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue);
    void WaitMsg(void *pMsg, TUint msgSize);
    bool PeekData(void *pBuffer, TUint size);
    TUint GetDataCnt() const {AUTO_LOCK(mpMutex); return mnData;}

private:
    EECode swicthToNextDataQueue(CIQueue* pCurrent);

public:
    bool IsMain() { return mpMainQ == NULL; }
    bool IsSub() { return mpMainQ != NULL; }

private:
    struct List {
        List *pNext;
        bool bAllocated;
        void Delete();
    };

private:
    void *mpOwner;
    bool mbDisabled;

    CIQueue *mpMainQ;
    CIQueue *mpPrevQ;
    CIQueue *mpNextQ;

    CIMutex *mpMutex;
    CICondition *mpCondReply;
    CICondition *mpCondGet;
    CICondition *mpCondSendMsg;

    TUint mnGet;
    TUint mnSendMsg;

    TUint mBlockSize;
    TUint mnData;

    List *mpTail;
    List *mpFreeList;

    List mHead;

    List *mpSendBuffer;
    TU8 *mpReservedMemory;

    EECode *mpMsgResult;

private:
    CIQueue *mpCurrentCircularlyQueue;

private:
    static void Copy(void *to, const void *from, TUint bytes)
    {
        if (bytes == sizeof(void*)) {
            *(void**)to = *(void**)from;
        } else {
            memcpy(to, from, bytes);
        }
    }

    List *AllocNode();

    void WriteData(List *pNode, const void *pBuffer, TUint size);
    void ReadData(void *pBuffer, TUint size);

};

//simple queue
typedef struct simple_node_s
{
    TUint ctx;
    struct simple_node_s* p_pre;
    struct simple_node_s* p_next;
} simple_node_t;

typedef struct simple_queue_s
{
    TUint max_cnt;
    TUint current_cnt;
    simple_node_t head;

    simple_node_t* freelist;

    pthread_mutex_t mutex;
    pthread_cond_t cond_notfull;
    pthread_cond_t cond_notempty;

    TUint (*getcnt)(struct simple_queue_s* thiz);
    void (*lock)(struct simple_queue_s* thiz);
    void (*unlock)(struct simple_queue_s* thiz);

    void (*enqueue)(struct simple_queue_s* thiz, TUint ctx);
    TUint (*dequeue)(struct simple_queue_s* thiz);
    TUint (*peekqueue)(struct simple_queue_s* thiz, TUint* ret);
} simple_queue_t;

extern simple_queue_t* _create_simple_queue(TUint num);
extern void _destroy_simple_queue(simple_queue_t* thiz);



extern void gfEncodingBase16(TChar *out, const TU8 *in, TInt in_size);
extern void gfDecodingBase16(TChar *out, const TU8 *in, TInt in_size);
extern EECode gfEncodingBase64(TU8* p_src, TU8* p_dest, TMemSize src_size, TMemSize& output_size);
extern EECode gfDecodingBase64(TU8* p_src, TU8* p_dest, TMemSize src_size, TMemSize& output_size);
extern TInt gfDecodingBase64Ffmpeg(TU8 *out, const TU8 *in_str, TInt out_size);
extern TChar* gfEncodingBase64Ffmpeg(TChar *out, TInt out_size, const TU8 *in, TInt in_size);

extern void gfGetCommonVersion(TU32& major, TU32& minor, TU32& patch, TU32& year, TU32& month, TU32& day);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

