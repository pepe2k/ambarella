/**
 * common_base.h
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

#ifndef __COMMON_BASE_H__
#define __COMMON_BASE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// IActiveObject
//
//-----------------------------------------------------------------------

typedef enum
{
    ECMDType_Invalid = 0x0000,

    ECMDType_FirstEnum = 0x0001,

    //common used
    ECMDType_Terminate = ECMDType_FirstEnum,
    ECMDType_StartRunning = 0x0002,
    ECMDType_ExitRunning = 0x0003,

    ECMDType_Stop = 0x0010,
    ECMDType_Start = 0x0011,
    ECMDType_Pause = 0x0012,
    ECMDType_Resume = 0x0013,
    ECMDType_ResumeFlush = 0x0014,
    ECMDType_Flush = 0x0015,
    ECMDType_FlowControl = 0x0016,
    ECMDType_Suspend = 0x0017,
    ECMDType_ResumeSuspend = 0x0018,
    ECMDType_SwitchInput = 0x0019,

    ECMDType_Step = 0x0020,
    ECMDType_DebugDump = 0x0021,
    ECMDType_PrintCurrentStatus = 0x0022,
    ECMDType_SetRuntimeLogConfig = 0x0023,

    ECMDType_AddContent = 0x0030,
    ECMDType_RemoveContent = 0x0031,
    ECMDType_AddClient = 0x0032,
    ECMDType_RemoveClient = 0x0033,
    ECMDType_UpdateUrl = 0x0034,
    ECMDType_RemoveClientSession = 0x0035,
    ECMDType_UpdateRecSavingStrategy = 0x0036,

    //specific to each case
    ECMDType_ForceLowDelay = 0x0040,
    ECMDType_Speedup = 0x0041,
    ECMDType_DeleteFile = 0x0042,
    ECMDType_UpdatePlaybackSpeed = 0x0043,
    ECMDType_UpdatePlaybackLoopMode = 0x0044,
    ECMDType_DecoderZoom = 0x0045,

    ECMDType_NotifySynced = 0x0050,
    ECMDType_NotifySourceFilterBlocked = 0x0051,
    ECMDType_NotifyUDECInRuningState = 0x0052,
    ECMDType_NotifyBufferRelease = 0x0053,

    ECMDType_MuteAudio = 0x0060,
    ECMDType_UnMuteAudio = 0x0061,

    ECMDType_MiscCheckLicence = 0x0070,

    //cloud based
    ECMDType_CloudSourceClient_UnknownCmd = 0x0100,

    ECMDType_CloudSourceClient_ReAuthentication = 0x0101,
    ECMDType_CloudSourceClient_StartPush = 0x0102,
    ECMDType_CloudSourceClient_StopPush = 0x0103,

    ECMDType_CloudSinkClient_ReAuthentication = 0x0110,
    ECMDType_CloudSinkClient_UpdateBitrate = 0x0111,
    ECMDType_CloudSinkClient_UpdateFrameRate = 0x0112,
    ECMDType_CloudSinkClient_UpdateDisplayLayout = 0x0113,
    ECMDType_CloudSinkClient_SelectAudioSource = 0x0114,
    ECMDType_CloudSinkClient_SelectAudioTarget = 0x0115,
    ECMDType_CloudSinkClient_Zoom = 0x0116,
    ECMDType_CloudSinkClient_UpdateDisplayWindow = 0x0117,
    ECMDType_CloudSinkClient_SelectVideoSource = 0x0118,
    ECMDType_CloudSinkClient_ShowOtherWindows = 0x0119,
    ECMDType_CloudSinkClient_DemandIDR = 0x011a,
    ECMDType_CloudSinkClient_SwapWindowContent = 0x011b,
    ECMDType_CloudSinkClient_CircularShiftWindowContent = 0x011c,
    ECMDType_CloudSinkClient_SwitchGroup = 0x011d,
    ECMDType_CloudSinkClient_ZoomV2 = 0x011e,
    ECMDType_CloudSinkClient_UpdateResolution = 0x011f,

    ECMDType_CloudClient_PeerClose = 0x0140,

    ECMDType_CloudClient_QueryVersion = 0x0141,
    ECMDType_CloudClient_QueryStatus = 0x0142,
    ECMDType_CloudClient_SyncStatus = 0x0143,
    ECMDType_CloudClient_QuerySourceList  = 0x0144,
    ECMDType_CloudClient_AquireChannelControl  = 0x0145,
    ECMDType_CloudClient_ReleaseChannelControl  = 0x0146,

    //for extention
    ECMDType_CloudClient_CustomizedCommand = 0x0180,

    //debug
    ECMDType_DebugClient_PrintCloudPipeline = 0x200,
    ECMDType_DebugClient_PrintStreamingPipeline = 0x201,
    ECMDType_DebugClient_PrintRecordingPipeline = 0x202,
    ECMDType_DebugClient_PrintSingleChannel = 0x203,
    ECMDType_DebugClient_PrintAllChannels = 0x204,

    ECMDType_LastEnum = 0x0400,

} ECMDType;

class SCMD
{
public:
    TUint code;
    void    *pExtra;
    TU8 repeatType;
    TU8 flag;
    TU8 needFreePExtra;
    TU8 reserved1;
    TU32 res32_1;
    TU64 res64_1;
    TU64 res64_2;

public:
    SCMD(TUint cid) { code = cid; pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
    SCMD() {pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
};

typedef enum {
    EMSGType_Invalid = 0,

    EMSGType_FirstEnum = ECMDType_LastEnum,

    //common used
    EMSGType_ExternalMSG = EMSGType_FirstEnum,
    EMSGType_Timeout = 0x0401,
    EMSGType_InternalBug = 0x0402,

    EMSGType_StorageError = 0x0410,
    EMSGType_IOError = 0x0411,
    EMSGType_SystemError = 0x0412,
    EMSGType_DeviceError = 0x0413,

    EMSGType_StreamingError_TCPSocketConnectionClose = 0x0414,
    EMSGType_StreamingError_UDPSocketInvalidArgument = 0x0415,

    EMSGType_DriverErrorBusy = 0x0420,
    EMSGType_DriverErrorNotSupport = 0x0421,
    EMSGType_DriverErrorOutOfCapability = 0x0422,
    EMSGType_DriverErrorNoPermmition = 0x0423,

    //for each user cases
    EMSGType_PlaybackEOS = 0x0430,
    EMSGType_RecordingEOS = 0x0431,
    EMSGType_NotifyNewFileGenerated = 0x0432,
    EMSGType_RecordingReachPresetDuration = 0x0433,
    EMSGType_RecordingReachPresetFilesize = 0x0434,
    EMSGType_RecordingReachPresetTotalFileNumbers = 0x0435,

    EMSGType_NotifyThumbnailFileGenerated = 0x0440,
    EMSGType_NotifyUDECStateChanges = 0x0441,
    EMSGType_NotifyUDECUpdateResolution = 0x0442,

    EMSGType_LastEnum,
} EMSGType;

typedef struct
{
    TUint code;
    void    *pExtra;
    TU16 sessionID;
    TU8 flag;
    TU8 needFreePExtra;

    TU16 owner_index;
    TU8 owner_type;
    TU8 identifyer_count;

    TULong p_owner, owner_id;
    TULong p_agent_pointer;

    TULong p0, p1, p2, p3, p4;
} SMSG;

typedef struct
{
    TUint code;
    void    *pExtra;
    TU16 sessionID;
    TU8 flag;
    TU8 needFreePExtra;

    TU8 owner_type, owner_index, reserved0, reserved1;
    TULong p_owner, owner_id;

    TULong p1, p2, p3, p4;
} SScheduledClient;

typedef enum {
    EModuleState_Invalid = 0,

    //commom used
    EModuleState_Idle,
    EModuleState_Preparing,
    EModuleState_Running,
    EModuleState_HasInputData,
    EModuleState_HasOutputBuffer,
    EModuleState_Completed,
    EModuleState_Bypass,
    EModuleState_Pending,
    EModuleState_Stopped,
    EModuleState_Error,

    EModuleState_WaitCmd,
    EModuleState_WaitTiming,

    //specified module
    //muxer
    EModuleState_Muxer_WaitExtraData,
    EModuleState_Muxer_SavingPartialFile,
    EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin,
    EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin,
    EModuleState_Muxer_SavingPartialFileWaitMasterPin,
    EModuleState_Muxer_SavingPartialFileWaitNonMasterPin,

    EModuleState_Muxer_SavingWholeFile,
    EModuleState_Muxer_FlushExpiredFrame,

    //renderer
    EModuleState_Renderer_PreBuffering,
    EModuleState_Renderer_PreBufferingDone,
    EModuleState_Renderer_WaitVoutDormant,

    //ts demuxer
    EModuleState_Demuxer_UpdatingContext,
} EModuleState;

extern const TChar* gfGetModuleStateString(EModuleState state);

extern const TChar* gfGetCMDTypeString(ECMDType type);

//-----------------------------------------------------------------------
//
//  IInterface
//
//-----------------------------------------------------------------------
class IInterface
{
public:
    virtual void Delete() = 0;

public:
    virtual const TChar* GetModuleName() const = 0;
    virtual TUint GetModuleIndex() const = 0;

public:
    virtual void PrintStatus() = 0;

public:
    virtual void SetLogLevel(TUint level) = 0;
    virtual void SetLogOption(TUint option) = 0;
    virtual void SetLogOutput(TUint output) = 0;
};

//-----------------------------------------------------------------------
//
//  IMsgSink
//
//-----------------------------------------------------------------------
class IMsgSink
{
public:
    virtual EECode MsgProc(SMSG& msg) = 0;
};

//-----------------------------------------------------------------------
//
//  IActiveObject
//
//-----------------------------------------------------------------------
class IActiveObject: virtual public IInterface
{
public:
    virtual void OnRun() = 0;
};

//-----------------------------------------------------------------------
//
//  IScheduledClient
//
//-----------------------------------------------------------------------
typedef TUint TSchedulingUnit;
typedef TInt TSchedulingHandle;
class IScheduledClient
{
public:
    virtual EECode Scheduling(TUint times = 1) = 0;

public:
    virtual TInt IsPassiveMode() const = 0;
    virtual TSchedulingHandle GetWaitHandle() const = 0;
    virtual TU8 GetPriority() const = 0;

    virtual EECode CheckTimeout() = 0;
    virtual EECode EventHandling(EECode err) = 0;

public:
    virtual TSchedulingUnit HungryScore() const = 0;
};

//-----------------------------------------------------------------------
//
//  IScheduler
//
//-----------------------------------------------------------------------
class IScheduler: virtual public IInterface
{
public:
    virtual EECode StartScheduling() = 0;
    virtual EECode StopScheduling() = 0;

public:
    virtual EECode AddScheduledCilent(IScheduledClient* client) = 0;
    virtual EECode RemoveScheduledCilent(IScheduledClient* client) = 0;

public:
    virtual TUint TotalNumberOfScheduledClient() const = 0;
    virtual TUint CurrentNumberOfClient() const = 0;
    virtual TSchedulingUnit CurrentHungryScore() const = 0;

public:
    virtual TInt IsPassiveMode() const = 0;
};

//-----------------------------------------------------------------------
//
//  IEventListener
//
//-----------------------------------------------------------------------
typedef enum
{
    EEventType_Invalid = 0,
    EEventType_BufferReleaseNotify,
    EEventType_Timer,
} EEventType;

class IEventListener
{
public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2) = 0;
};

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
class CObject: virtual public IInterface
{
public:
    virtual void Delete();
    virtual ~CObject();

public:
    virtual void PrintStatus();

public:
    CObject(const TChar* name, TUint index = 0);
    virtual void SetLogLevel(TUint level);
    virtual void SetLogOption(TUint option);
    virtual void SetLogOutput(TUint output);

public:
    virtual const TChar* GetModuleName() const;
    virtual TUint GetModuleIndex() const;

protected:
    TUint mConfigLogLevel;
    TUint mConfigLogOption;
    TUint mConfigLogOutput;

protected:
    const TChar* mpModuleName;

protected:
    TUint mIndex;

protected:
    TUint mDebugHeartBeat;

protected:
    FILE* mpLogOutputFile;
};

//-----------------------------------------------------------------------
//
//  CIDoubleLinkedList
//
//-----------------------------------------------------------------------
//common container, not thread safe
class CIDoubleLinkedList
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
    CIDoubleLinkedList();
    virtual ~CIDoubleLinkedList();

public:
    //if target_node == NULL, means target_node == list head
    SNode* InsertContent(SNode* target_node, void* content, TUint after = 1);
    void RemoveContent(void* content);
    //AM_ERR RemoveNode(SNode* node);
    SNode* FirstNode() const;
    SNode* LastNode() const;
    SNode* PreNode(SNode* node) const;
    SNode* NextNode(SNode* node) const;
    TUint NumberOfNodes() const;

public:
    static void* operator new (size_t, void* p);

//debug function
public:
    bool IsContentInList(void* content) const;

private:
    bool IsNodeInList(SNode* node) const;
    void allocNode(SNodePrivate* & node);

private:
    SNodePrivate mHead;
    TUint mNumberOfNodes;
    SNodePrivate* mpFreeList;
};

//-----------------------------------------------------------------------
//
//  CIDoubleLinkedListMap
//
//-----------------------------------------------------------------------
//common container, not thread safe
class CIDoubleLinkedListMap
{
public:
    struct SNode
    {
        void* p_context;
        void* key;
    };

private:
    //trick here, not very good, make pointer not expose
    struct SNodePrivate
    {
        void* p_context;
        void* key;
        SNodePrivate* p_next, *p_pre;
    };

public:
    CIDoubleLinkedListMap();
    virtual ~CIDoubleLinkedListMap();

public:
    //if target_node == NULL, means target_node == list head
    SNode* InsertContent(SNode* target_node, void* content, void* key, TUint after = 1);
    void RemoveContent(void* content);
    void RemoveContentFromKey(void* key);

    SNode* FirstNode() const;
    SNode* LastNode() const;
    SNode* PreNode(SNode* node) const;
    SNode* NextNode(SNode* node) const;
    TUint NumberOfNodes() const;
    void* KeyFromContent(void* content) const;
    void* ContentFromKey(void* content) const;

public:
    static void* operator new (size_t, void* p);

//debug function
public:
    bool IsContentInList(void* content) const;

private:
    bool IsNodeInList(SNode* node) const;
    bool IsKeyInList(void* key) const;
    void allocNode(SNodePrivate* & node);

private:
    SNodePrivate mHead;
    TUint mNumberOfNodes;
    SNodePrivate* mpFreeList;
};

//-----------------------------------------------------------------------
//
//  CIWorkQueue
//
//-----------------------------------------------------------------------
class CIWorkQueue: virtual public CObject
{
    typedef CObject inherited;
    typedef IActiveObject AO;

public:
    static CIWorkQueue* Create(AO *pAO);
    virtual void Delete();

private:
    CIWorkQueue(AO *pAO);
    EECode Construct();
    ~CIWorkQueue();

public:
    // return receive's reply
    EECode SendCmd(TUint cid, void *pExtra = NULL);
    EECode SendCmd(SCMD& cmd);
    void GetCmd(SCMD& cmd);
    bool PeekCmd(SCMD& cmd);

    EECode PostMsg(TUint cid, void *pExtra = NULL);
    EECode PostMsg(SCMD& cmd);
    EECode Run();
    EECode Stop();
    EECode Start();

    void CmdAck(EECode result);
    CIQueue *MsgQ() const;

    CIQueue::QType WaitDataMsg(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult);
    CIQueue::QType WaitDataMsgCircularly(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult);
    CIQueue::QType WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue);
    void WaitMsg(void *pMsg, TUint msgSize);

private:
    void MainLoop();
    static EECode ThreadEntry(void *p);
    void Terminate();

private:
    AO *mpAO;
    CIQueue *mpMsgQ;
    CIThread *mpThread;
};

//-----------------------------------------------------------------------
//
// IClockSource
//
//-----------------------------------------------------------------------
typedef enum {
    EClockSourceState_stopped = 0,
    EClockSourceState_running,
    EClockSourceState_paused,
} EClockSourceState;

class IClockSource: virtual public IInterface
{
public:
    virtual TTime GetClockTime(TUint relative = 1) const = 0;
    virtual TTime GetClockBase() const = 0;

    virtual void SetClockFrequency(TUint num, TUint den) = 0;
    virtual void GetClockFrequency(TUint& num, TUint& den) const = 0;

    virtual void SetClockState(EClockSourceState state) = 0;
    virtual EClockSourceState GetClockState() const = 0;

    virtual void UpdateTime() = 0;
};

typedef enum {
    EClockSourceType_generic = 0,
} EClockSourceType;

extern IClockSource* gfCreateClockSource(EClockSourceType type = EClockSourceType_generic);

//-----------------------------------------------------------------------
//
// CIClockReference
//
//-----------------------------------------------------------------------

typedef enum {
    EClockTimerType_once = 0,
    EClockTimerType_repeat_count,
    EClockTimerType_repeat_infinite,
} EClockTimerType;

typedef struct SClockListener_s
{
    IEventListener* p_listener;
    EClockTimerType type;
    TTime event_time;
    TTime event_duration;

    TUint event_remaining_count;

    struct SClockListener_s* p_next;

    TULong user_context;
} SClockListener;


class CIClockReference: virtual public CObject
{
    typedef CObject inherited;
protected:
    CIClockReference();
    ~CIClockReference();

    EECode Construct();

public:
    static CIClockReference* Create();
    void Delete();

public:
    //void SetClockSource(IClockSource* clock_source);

    void SetBeginTime(TTime start_time);
    void SetDirection(TU8 forward);
    void SetSpeed(TU8 speed, TU8 speed_frac);

    EECode Start();
    EECode Pause();
    EECode Resume();
    EECode Stop();

    EECode SyncTo(TTime source_time, TTime target_time, TU8 sync_source_time, TU8 sync_target_time, TU8 reset_to_1x_forward = 0);

    void Heartbeat(TTime current_source_time);

    void ClearAllTimers();
    SClockListener* AddTimer(IEventListener* p_listener, EClockTimerType type, TTime time, TTime interval, TUint count = 0);
    EECode RemoveTimer(SClockListener* timer);
    EECode RemoveAllTimers(IEventListener* p_listener);

    TTime GetCurrentTime() const;

    virtual void PrintStatus();

private:
    SClockListener* allocClockListener();
    void releaseClockListener(SClockListener* p);
    void clearAllClockListener();
    void updateTime(TTime target_time);

private:
    CIMutex* mpMutex;
    //IClockSource* mpClockSource;

    volatile TTime mBeginSourceTime;
    volatile TTime mCurrentSourceTime;

    volatile TTime mBeginTime;
    volatile TTime mCurrentTime;

    TU8 mbPaused;
    TU8 mbStarted;
    TU8 mbSilent;
    TU8 mbForward;

    TU8 mSpeed;
    TU8 mSpeedFrac;

    TU8 mReserve0, mReserved1;

    CIDoubleLinkedList mClockListenerList;
    SClockListener* mpFreeClockListenerList;
};

//-----------------------------------------------------------------------
//
// CIClockManager
//
//-----------------------------------------------------------------------
class CIClockManager: virtual public CObject, virtual public IActiveObject
{
    typedef CObject inherited;
public:
    static CIClockManager* Create();
    void Delete();

protected:
    EECode Construct();
    CIClockManager();
    ~CIClockManager();

public:
    EECode SetClockSource(IClockSource* pClockSource);
    EECode RegisterClockReference(CIClockReference* p_clock_reference);
    EECode UnRegisterClockReference(CIClockReference* p_clock_reference);

    EECode Start();
    EECode Stop();

    virtual void PrintStatus();

protected:
    virtual void OnRun();

private:
    virtual EECode processCMD(SCMD& cmd);

private:
    void updateClockReferences();

private:
    CIWorkQueue* mpWorkQ;
    IClockSource* mpClockSource;

    CIDoubleLinkedList mClockReferenceList;
    CIEvent* mpEvent;

    TU8 mbRunning;
    TU8 mbHaveClockSource;
    TU8 mbPaused, mbClockReferenceListDestroyed;
};

//-----------------------------------------------------------------------
//
// IMemPool
//
//-----------------------------------------------------------------------
class IMemPool : virtual public IInterface
{
public:
    virtual TU8* RequestMemBlock(TULong size, TU8* start_pointer = NULL) = 0;
    virtual void ReturnBackMemBlock(TULong size, TU8* start_pointer) = 0;
    virtual void ReleaseMemBlock(TULong size, TU8* start_pointer) = 0;
};

//-----------------------------------------------------------------------
//
//  ICustomizedCodec
//
//-----------------------------------------------------------------------
class ICustomizedCodec
{
public:
    virtual EECode ConfigCodec(TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5) = 0;
    virtual EECode QueryInOutBufferSize(TMemSize& encoder_input_buffer_size, TMemSize& encoder_output_buffer_size) const = 0;

public:
    virtual EECode Encoding(void* in_buffer, void* out_buffer, TMemSize in_data_size, TMemSize& out_data_size) = 0;
    virtual EECode Decoding(void* in_buffer, void* out_buffer, TMemSize in_data_size, TMemSize& out_data_size) = 0;

public:
    virtual void Destroy() = 0;
};

ICustomizedCodec* gfCustomizedCodecFactory(TChar* codec_name, TUint index);

typedef enum {
    ESchedulerType_Invalid = 0,

    ESchedulerType_RunRobin,
    ESchedulerType_Preemptive,
    ESchedulerType_PriorityPreemptive,

} ESchedulerType;

extern IScheduler* gfSchedulerFactory(ESchedulerType type, TUint index);

extern EECode gfGetCurrentDateTime(SDateTime* datetime);

//-----------------------------------------------------------------------
//
//  IIPCAgent
//
//-----------------------------------------------------------------------
typedef EECode (*TIPCAgentReadCallBack) (void* owner, TU8*& p_data, TInt& datasize, TU32& remainning);

class IIPCAgent
{
public:
    virtual EECode CreateContext(TChar* tag, TUint is_server, void* p_context, TIPCAgentReadCallBack callback, TU16 native_port = 0, TU16 remote_port = 0) = 0;
    virtual void DestroyContext() = 0;

    virtual TInt GetHandle() const = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Write(TU8* data, TInt size) = 0;

public:
    virtual void Destroy() = 0;
};

extern IIPCAgent* gfIPCAgentFactory(EIPCAgentType type);

//-----------------------------------------------------------------------
//
//  CIRemoteLogServer
//
//-----------------------------------------------------------------------
class CIRemoteLogServer: public CObject
{
public:
    static CIRemoteLogServer* Create();

protected:
    CIRemoteLogServer();
    virtual ~CIRemoteLogServer();
    EECode Construct();

public:
    virtual EECode CreateContext(TChar* tag, TU16 native_port = 0, TU16 remote_port = 0);
    virtual void DestroyContext();

    virtual EECode WriteLog(const TChar* log, ...);
    virtual EECode SyncLog();

public:
    virtual void Delete();

public:
    virtual void Destroy();

private:
    IIPCAgent* mpAgent;

private:
    CIMutex* mpMutex;

private:
    TU16 mPort;
    TU16 mRemotePort;

private:
    TChar* mpTextBuffer;
    TInt mTextBufferSize;

    TChar* mpCurrentWriteAddr;
    TInt mRemainingSize;

private:
    TU8 mbCreateContext;
    TU8 mbBufferFull;
    TU8 mReserved0;
    TU8 mReserved1;
};

//-----------------------------------------------------------------------
//
// CRingMemPool
//
//-----------------------------------------------------------------------
class CRingMemPool: public CObject, public IMemPool
{
    typedef CObject inherited;

public:
    static IMemPool* Create(TMemSize size);
    virtual void Delete();

    virtual TU8* RequestMemBlock(TMemSize size, TU8* start_pointer = NULL);
    virtual void ReturnBackMemBlock(TMemSize size, TU8* start_pointer);
    virtual void ReleaseMemBlock(TMemSize size, TU8* start_pointer);

protected:
    CRingMemPool();
    EECode Construct(TMemSize size);
    virtual ~CRingMemPool();

private:
    CIMutex* mpMutex;
    CICondition* mpCond;

private:
    TU8 mRequestWrapCount;
    TU8 mReleaseWrapCount;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    TU8* mpMemoryBase;
    TU8* mpMemoryEnd;
    TMemSize mMemoryTotalSize;

    TU8* volatile mpFreeMemStart;
    TU8* volatile mpUsedMemStart;

private:
    volatile TU32 mnWaiters;
};

//-----------------------------------------------------------------------
//
// CFixedMemPool
//
//-----------------------------------------------------------------------
class CFixedMemPool: public CObject, public IMemPool
{
    typedef CObject inherited;

public:
    static IMemPool* Create(const TChar* name, TMemSize size, TUint tot_count);
    virtual void Delete();

    virtual TU8* RequestMemBlock(TMemSize size, TU8* start_pointer = NULL);
    virtual void ReturnBackMemBlock(TMemSize size, TU8* start_pointer);
    virtual void ReleaseMemBlock(TMemSize size, TU8* start_pointer);

protected:
    CFixedMemPool(const TChar* name);
    EECode Construct(TMemSize size, TUint tot_count);
    virtual ~CFixedMemPool();

private:
    CIMutex* mpMutex;
    CICondition* mpCond;

private:
    TU8* mpMemoryBase;
    TU8* mpMemoryEnd;
    TMemSize mMemoryTotalSize;

    TU8* volatile mpFreeMemStart;
    TU8* volatile mpUsedMemStart;

    TMemSize mMemBlockSize;
    TU32   mnTotalBlockNumber;
    TU32 mnCurrentFreeBlockNumber;

private:
    volatile TU32 mnWaiters;
};

//-----------------------------------------------------------------------
//
//  CISimplePool
//
//-----------------------------------------------------------------------
typedef struct
{
    TU8* p_data;
    TMemSize data_size;
} SDataPiece;

class CISimplePool;

typedef struct
{
    TInt fd;

    TU8 is_closed;
    TU8 has_socket_error;
    TU8 reserved1;
    TU8 reserved2;

    SDataPiece* p_cached_datapiece;

    CIQueue* data_queue;
    IMemPool* ring_pool;
    CISimplePool* data_piece_pool;
} STransferDataChannel;

class CISimplePool
{
public:
    static CISimplePool* Create(TUint max_count);
    void Delete();

public:
    void PrintStatus();

protected:
    CISimplePool();
    EECode Construct(TUint nMaxBuffers);
    virtual ~CISimplePool();

public:
    bool AllocDataPiece(SDataPiece*& pDataPiece, TUint size);
    void ReleaseDataPiece(SDataPiece* pDataPiece);
    TUint GetFreeDataPieceCnt() const;

protected:
    CIQueue *mpBufferQ;

protected:
    TU8 *mpDataPieceStructMemory;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

