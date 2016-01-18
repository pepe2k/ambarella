/**
 * am_util.h
 *
 * History:
 *	2010/08/10 - [Zhi He] created file
 *
 * Desc: some common used struct/functions
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

 #ifndef __AM_UTIL_H__
#define __AM_UTIL_H__

//tmp add here
#include "am_external_define.h"

typedef enum _AM_PlayItemType {
    PlayItemType_None = 0,
    PlayItemType_LocalFile,
    PlayItemType_HTTP,
    PlayItemType_RTSP,
    PlayItemType_OpendFile,
} AM_PlayItemType;

typedef struct _AM_PlayItem {
    AM_PlayItemType type;
    struct _AM_PlayItem* pNext;
    struct _AM_PlayItem* pPre;
    AM_INT CntOrTag;
    void* pSourceFilter;//source filter
    void* pSourceFilterContext;//source filter context
    AM_UINT stringBufferLen;
    char* pDataSource;
} AM_PlayItem;

//play list related
AM_PlayItem* AM_NewPlayItem();
void AM_DeletePlayItem(AM_PlayItem*);
AM_INT AM_FillPlayItem(AM_PlayItem* des, char* dataSource, AM_PlayItemType type);
AM_INT AM_InsertPlayItem(AM_PlayItem* head, AM_PlayItem* item, AM_INT isAfter);
AM_INT AM_RemovePlayItem(AM_PlayItem* item);
AM_INT AM_CheckItemInList(AM_PlayItem* header, AM_PlayItem* item);


//common container, not thread safe
class CDoubleLinkedList
{
public:
    struct SNode
    {
        void* p_context;
    };

private:
    //trick here, not very good, make pointer not expose
    struct SNodePrivate
    {
        void* p_context;
        SNodePrivate* p_next, *p_pre;
    };

public:
    CDoubleLinkedList();
    virtual ~CDoubleLinkedList();

public:
    //if target_node == NULL, means target_node == list head
    SNode* InsertContent(SNode* target_node, void* content, AM_UINT after = 1);
    void RemoveContent(void* content);
    //AM_ERR RemoveNode(SNode* node);
    SNode* FirstNode();
    SNode* LastNode();
    SNode* PreNode(SNode* node);
    SNode* NextNode(SNode* node);
    AM_UINT NumberOfNodes() { return mNumberOfNodes;}

//debug function
public:
    bool IsContentInList(void* content);

private:
    bool IsNodeInList(SNode* node);
    void allocNode(SNodePrivate* & node);

private:
    SNodePrivate mHead;
    AM_UINT mNumberOfNodes;
    SNodePrivate* mpFreeList;
};

void AM_WriteHLSConfigfile(char* config_filename, char* filename_base, AM_UINT start_index, AM_UINT end_index, char* ext, AM_UINT& seq_num, AM_UINT length=1);
char* AM_GetStringFromContainerType(IParameters::ContainerType type);
IParameters::ContainerType AM_GetContainerTypeFromString (char* str);

void AM_DefaultPBConfig(SConsistentConfig* mpShared);
void AM_DefaultPBDspConfig(SConsistentConfig* mpShared);
void AM_DefaultRecConfig(SConsistentConfig* mpConfig);
void AM_PrintPBDspConfig(SConsistentConfig* mpShared);
void AM_PrintLogConfig();
void AM_PrintRecConfig(SConsistentConfig* mpConfig);
AM_UINT AM_LoadPBConfigFile(const char* fileName, SConsistentConfig* mpShared);
AM_UINT AM_LoadRecConfigFile(const char* fileName, SConsistentConfig* mpConfig);
bool AM_VideoEncCheckDimention(AM_UINT& width, AM_UINT& height, AM_UINT index);
AM_UINT AM_VideoEncCalculateFrameRate(AM_UINT num, AM_UINT den);

void AM_DumpBinaryFile(const char* fileName, AM_U8* pData, AM_UINT size);
void AM_AppendtoBinaryFile(const char* fileName, AM_U8* pData, AM_UINT size);
void AM_DumpBinaryFile_withIndex(const char* fileName, AM_UINT index, AM_U8* pData, AM_UINT size);
int AM_ConvertFrame(int PixelFormat, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height);
void AM_Planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height);
void AM_Scale(int PixelFormat, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height, AM_UINT des_width, AM_UINT des_height);
void AM_ToRGB565(int PixelFormat, AM_U8* src[], AM_U16* rgb, AM_UINT width, AM_UINT height, AM_INT src_stride[]);
AM_UINT AM_GetNextStartCode(AM_U8* pstart, AM_U8* pend, AM_INT esType);

void AM_SimpleScale(IParameters::PixFormat pix_format, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height, AM_UINT des_width, AM_UINT des_height);
void AM_ConvertPixFormat(IParameters::PixFormat src_pix_format, IParameters::PixFormat des_pix_format, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT width, AM_UINT height);

//function from other place
#define STOPFLAG_FLUSH 0x00000001
#define STOPFLAG_STOP 0x00000000
#define STOPFLAG_CLEAR 0x000000ff

AM_INT PauseUdec(AM_INT iavFd, AM_INT index);
AM_INT ResumeUdec(AM_INT iavFd, AM_INT index);
AM_INT UdecStepPlay(AM_INT iavFd, AM_INT index, AM_INT cnt);
AM_INT WaitVoutState(AM_INT iavFd, AM_INT target_vout_state);
AM_INT StopUDEC(AM_INT iavFd, AM_INT udec_id, AM_UINT stop_flag);
bool GetUdecState(AM_INT iavFd, AM_UINT* udec_state, AM_UINT* vout_state, AM_UINT* error_code);

AM_INT getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig);
AM_INT AM_GetDefaultDSPConfig(SConsistentConfig* mpShared);
//AM_INT AM_SetupDSPConfig(SConsistentConfig* mpShared);

AM_UINT AnalyseUdecErrorCode(AM_UINT udecErrorCode, AM_UINT debugConfig);
void RecordUdecErrorCode(AM_UINT udecErrorCode);

//from ffmpeg_util
AM_ERR FF_GenerateJpegFile(char* filename, AM_U8* buffer[], AM_UINT width, AM_UINT height, IParameters::PixFormat format, AM_UINT jpeg_bitrate, AM_U8* bitstream_buffer, AM_UINT bitstream_buffer_size);

//little endian, hard code here
#define L_ENDIAN
#ifdef L_ENDIAN
#define MK_FOURCC_TAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#else
#define MK_FOURCC_TAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))
#endif

typedef struct SListNode
{
    SListNode* pNext;
    SListNode* pPre;

    AM_UINT context;
    void* pContext;
}SListNode;

class CList
{
private:
    SListNode mHeader;
    SListNode* mpFreeList;

public:
    CList()
    {
        mHeader.pPre = mHeader.pNext = &mHeader;
        mHeader.context = 0;
        mHeader.pContext = NULL;
        mpFreeList = NULL;
    }

    ~CList()
    {
        SListNode* ptmp = mHeader.pNext;
        SListNode* ptobefreed;

        while (ptmp != &mHeader) {
            ptobefreed = ptmp;
            ptmp = ptmp->pNext;
            free((void*)ptobefreed);
        }

        ptmp = mpFreeList;
        while (ptmp) {
            ptobefreed = ptmp;
            ptmp = ptmp->pNext;
            free((void*)ptobefreed);
        }
    }

public:
    void append_2end (SListNode* node)
    {
        AM_ASSERT(mHeader.pNext);
        AM_ASSERT(mHeader.pPre);
        AM_ASSERT(node != &mHeader);
        if (!mHeader.pNext || !mHeader.pPre) {
            AM_ERROR("BAD list_head %p.\n", &mHeader);
            return;
        }

        node->pNext = &mHeader;
        node->pPre = mHeader.pPre;
        mHeader.pPre->pNext = node;
        mHeader.pPre = node;
    }

    void release (SListNode* node)
    {
        AM_ASSERT(node);
        AM_ASSERT(node != &mHeader);
        if (!node) {
            return;
        }

        AM_ASSERT(node->pNext);
        AM_ASSERT(node->pPre);
        if (!node->pNext || !node->pPre) {
            AM_ERROR("BAD node %p.\n", node);
            return;
        }

        node->pPre->pNext = node->pNext;
        node->pNext->pPre = node->pPre;

        node->pNext = mpFreeList;
        mpFreeList = node;
    }

    SListNode* alloc()
    {
        SListNode* ptmp = mpFreeList;
        if (ptmp) {
            mpFreeList = mpFreeList->pNext;
            return ptmp;
        }
        ptmp = (SListNode*)malloc(sizeof(SListNode));
        return ptmp;
    }

    SListNode* peek_first()
    {
        SListNode* p_next;
        AM_ASSERT(mHeader.pNext);
        AM_ASSERT(mHeader.pPre);

        if (!mHeader.pNext || !mHeader.pPre) {
            AM_ERROR("BAD list_head %p.\n", &mHeader);
            return NULL;
        }

        p_next = mHeader.pNext;
        if (p_next != &mHeader) {
            return p_next;
        }

        return NULL;
    }
};

//-----------------------------------------------------------------------
//
// CWatcher
//
//-----------------------------------------------------------------------
#define DMAX_STREAM_NUM 4
#define DMAX_STREAM_NAME_LEN 32

typedef struct
{
    void* p_content;
    char* p_name;//ip address
    char* p_url;//rtsp url
    AM_UINT param0, param1, param2, param3;

    AM_U8 is_alive;
    AM_U8 is_load_from_config_file;
    AM_U8 reserved1;
    AM_U8 stream_count;

    char stream[DMAX_STREAM_NUM][DMAX_STREAM_NAME_LEN];

    SWatchItem* p_watch_item;
} SSimpleDataPiece;

//-----------------------------------------------------------------------
//
// Parse Customized Protocol
//
//-----------------------------------------------------------------------
#define DStrCPStartTag "[CPStart] "
#define DStrCPEndTag " [CPEnd]\r\n"

#define DStrCPName "[CPName]:"
//ip address is unique id for each device
#define DStrCPIPAddress "[CPIPAddress]:"
#define DStrCPRTSPAddress "[CPRTSPAddress]:"

//IDR notification msg, skychen 2012_11_21
#define DStrCPIDRNotify "[IDR]"
//-----------------------------------------------------------------------
//
// CSimpleDataBase
//
//-----------------------------------------------------------------------
class CSimpleDataBase
{
#define DMAX_PARSE_STRING_BUFFER_SIZE 512
#define DDefaultMsgListenPort 88

public:
    CSimpleDataBase():
        mpMutex(NULL),
        mpWatcher(NULL),
        mnDataPieceNumber(0),
        mListenPort(4848),
        mListenSocket(-1),
        mbWatcherRunning(0),
        mbDatabaseRunning(0),
        mpUpdateNotityContext(NULL),
        mCurParseStringBufferLen(0)
        {
            mpMutex = CMutex::Create(false);
            AM_ASSERT(mpMutex);
        }

    virtual ~CSimpleDataBase()
        {
            SSimpleDataPiece* piece = NULL;
            CDoubleLinkedList::SNode* pnode = NULL;

            //scan if there's item
            pnode = mDataList.FirstNode();

            while (pnode) {
                piece = (SSimpleDataPiece*)pnode->p_context;
                AM_ASSERT(piece);
                if (piece) {
                    destroyDataPiece(piece);
                }

                pnode = mDataList.NextNode(pnode);
            }

            if (mpWatcher) {
                mpWatcher->Delete();
                mpWatcher = NULL;
            }

            if (mpMutex) {
                mpMutex->Delete();
                mpMutex = NULL;
            }
        }

public:
    AM_ERR Start(void* thiz, TFConfigUpdateNotify notify_cb, AM_U16 listen_port = DDefaultMsgListenPort);
    AM_ERR Stop();

    AM_ERR LoadConfigFile(char* filename);
    AM_ERR SaveConfigFile(char* filename, AM_U8 only_save_alive);

    SSimpleDataPiece* QueryData(SSimpleDataPiece* p_pre);
    AM_UINT TotalDataCount();

    AM_ERR SetAlive(SSimpleDataPiece* p_pre, AM_U8 is_alive);
    AM_ERR SetAlive(char* name, AM_U8 is_alive);

public:
    static AM_ERR Callback(void* thiz, SWatchItem* owner, AM_UINT flag);
    AM_ERR ProcessCallback(SWatchItem* owner, AM_UINT flag);

private:
    void parseMultipleRTSPAddr(char* url, SSimpleDataPiece* piece);
    void parseStreamerRequest(AM_INT fd, AM_UINT type, AM_UINT& msg_type, SSimpleDataPiece*& piece);
    bool parseOnePieceString(char* start, AM_UINT& msg_type, SSimpleDataPiece*& piece);

    SSimpleDataPiece* findDataPiece(char* name);
    bool isInDataBase(SSimpleDataPiece* p);
    AM_ERR setupMsgPort(AM_U16 listen_port);

private:
    void destroyDataPiece(SSimpleDataPiece* p);

protected:
    CMutex* mpMutex;
    IWatcher* mpWatcher;
    CDoubleLinkedList mDataList;
    AM_UINT mnDataPieceNumber;

    AM_U16 mListenPort;
    AM_U16 mReserved0;
    AM_INT mListenSocket;

    AM_U8 mbWatcherRunning;
    AM_U8 mbDatabaseRunning;
    AM_U8 mReserved3[2];

protected:
    //file saver related

private:
    TFConfigUpdateNotify mfUpdateNotify;
    void* mpUpdateNotityContext;

protected:
    //parse string related
    char mParseStringBuffer[DMAX_PARSE_STRING_BUFFER_SIZE];
    AM_UINT mCurParseStringBufferLen;
};
//-----------------------------------------------------------------------
//
// CVoutEventListener
//
//-----------------------------------------------------------------------
class CVoutEventListener{
public:
       enum vout_event_type{
              HDMI_REMOVE = 0,
              HDMI_PLUGIN = 1,
       };
       enum vout_type{
              VOUT_LCD = 0,
              VOUT_HDMI = 1,
       };
       typedef int(*funcptr)(int);

public:
       CVoutEventListener(void* callback);
       ~CVoutEventListener();
       int CheckCurrentVout();
public:
       int mSocketFd;
       int mIavClientpthreadLoop;
       pthread_t mIavClientpthreadID;
       funcptr CallbackFunc;
private:
       void CreatIavClient();
};
#endif


