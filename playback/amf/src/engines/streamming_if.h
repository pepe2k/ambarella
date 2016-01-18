
/**
 * streamming_if.h
 *
 * History:
 *    2011/11/23 - [Zhi He] created file
 *
 * Copyright (C) 2009-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __STREAMMING_IF_H__
#define __STREAMMING_IF_H__

//depend on socket.h
#include <sys/socket.h>
#include <arpa/inet.h>
#if PLATFORM_LINUX
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <pthread.h>

#define RTSP_MAX_BUFFER_SIZE 4096
#define RTSP_MAX_DATE_BUFFER_SIZE 512

extern const AM_IID IID_IStreammingServerManager;
extern const AM_IID IID_IStreammingContentProvider;
extern const AM_IID IID_IStreammingDataTransmiter;
//extern const AM_IID IID_IStreammingDataPort;

typedef enum
{
    EServerAction_Invalid = 0,
    EServerAction_RTSPUnicastNewClient,
    EServerAction_RTSPMulticastNewClient,//conference request comes?
    EServerAction_RTSPMulticastStartRequest,//conference request?
    EServerAction_RTSPBoardcast,//live streaming?
} EServerAction;

//can be used or flags
typedef enum
{
    ESessionMethod_INVALID = 0,
    ESessionMethod_RTSP_OPTIONS = (0x1 << 1),
    ESessionMethod_RTSP_DESCRIBE = (0x1 << 2),
    ESessionMethod_RTSP_ANNOUNCE = (0x1 << 3),

    ESessionMethod_RTSP_SETUP = (0x1 << 4),
    ESessionMethod_RTSP_PLAY = (0x1 << 5),
    ESessionMethod_RTSP_PAUSE = (0x1 << 6),
    ESessionMethod_RTSP_TEARDOWN = (0x1 << 7),
    ESessionMethod_RTSP_GET_PARAMETER = (0x1 << 8),
    ESessionMethod_RTSP_SET_PARAMETER = (0x1 << 9),

    ESessionMethod_RTSP_REDIRECT = (0x1 << 10),
    ESessionMethod_RTSP_RECORD = (0x1 << 11),

    //ONLY USED when RTSP is carrided over RTP
    ESessionMethod_RTSP_EmbeddedBinaryData = (0x1 << 12),
}  ESessionMethod;

typedef enum
{
    ESessionStatusCode_Invalid = 0,

    //success 2xx
    ESessionStatusCode_LowOnStorageSpace = 250,

    //redirection 3xx

    //client error 4xx
    ESessionStatusCode_MethodNotAllowed = 405,
    ESessionStatusCode_ParametersNotUnderstood = 451,
    ESessionStatusCode_ConferenceNotFound = 452,
    ESessionStatusCode_NotEnoughBandwidth = 453,
    ESessionStatusCode_SessionNotFound = 454,
    ESessionStatusCode_MethodNotValidInThisState = 455,
    ESessionStatusCode_HeaderFieldNotValidForResource = 456,
    ESessionStatusCode_InvalidRange = 457,
    ESessionStatusCode_ParametersIsReadOnly = 458,
    ESessionStatusCode_AggregateOperationNotAllowed = 459,
    ESessionStatusCode_OnlyAggregateOperationAllowed = 460,
    ESessionStatusCode_UnsupportedTransport = 461,
    ESessionStatusCode_DestnationUnreachable = 462,
    ESessionStatusCode_OptionNotSupported = 551,
}  ESessionStatusCode;

//not used now, client's state
typedef enum
{
    ESessionClientState_Init = 0,
    ESessionClientState_Ready,
    ESessionClientState_Playing,
    ESessionClientState_Recording,
} ESessionClientState;

typedef enum
{
    ESessionServerState_Init = 0,
    ESessionServerState_Ready,
    ESessionServerState_Playing,
    ESessionServerState_Recording,
}  ESessionServerState;

typedef enum
{
    EServerState_running,
    EServerState_shuttingdown,
    EServerState_closed,
} EServerState;

typedef enum
{
    EServerManagerState_idle,
    EServerManagerState_noserver_alive,
    EServerManagerState_running,
    EServerManagerState_halt,
    EServerManagerState_error,
} EServerManagerState;

//refer to rfc2326, 12 Header Field Definitions
typedef enum
{
    ERTSPHeaderType_general = (0x1 << 0),
    ERTSPHeaderType_request = (0x1 << 1),
    ERTSPHeaderType_response = (0x1 << 2),
    ERTSPHeaderType_entity = (0x1 << 3),
} ERTSPHeaderType;

typedef enum
{
    ERTSPHeaderSupport_required,
    ERTSPHeaderSupport_optional,
} ERTSPHeaderSupport;

typedef enum
{
    ERTSPHeaderName_Accept = 0,
    ERTSPHeaderName_Accept_Encoding,
    ERTSPHeaderName_Accept_Language,
    ERTSPHeaderName_Allow,
    ERTSPHeaderName_Authorization,
    ERTSPHeaderName_Bandwidth,
    ERTSPHeaderName_Blocksize,
    ERTSPHeaderName_Cache_Control,
    ERTSPHeaderName_Conference,
    ERTSPHeaderName_Connection,
    ERTSPHeaderName_Content_Base,
    ERTSPHeaderName_Content_Encoding_1,
    ERTSPHeaderName_Content_Encoding_2,
    ERTSPHeaderName_Content_Language,
    ERTSPHeaderName_Content_Length_1,
    ERTSPHeaderName_Content_Length_2,
    ERTSPHeaderName_Content_Location,
    ERTSPHeaderName_Content_Type_1,
    ERTSPHeaderName_Content_Type_2,
    ERTSPHeaderName_CSeq,
    ERTSPHeaderName_Date,
    ERTSPHeaderName_Expires,
    ERTSPHeaderName_From,
    ERTSPHeaderName_If_Modified_Since,
    ERTSPHeaderName_Last_Modified,
    ERTSPHeaderName_Proxy_Authenticate,
    ERTSPHeaderName_Proxy_Require,
    ERTSPHeaderName_Public,
    ERTSPHeaderName_Range_1,
    ERTSPHeaderName_Range_2,
    ERTSPHeaderName_Referer,
    ERTSPHeaderName_Require,
    ERTSPHeaderName_Retry_After,
    ERTSPHeaderName_RTP_Info,
    ERTSPHeaderName_Scale,
    ERTSPHeaderName_Session,
    ERTSPHeaderName_Server,
    ERTSPHeaderName_Speed,
    ERTSPHeaderName_Transport,
    ERTSPHeaderName_Unsupported,
    ERTSPHeaderName_User_Agent,
    ERTSPHeaderName_Via,
    ERTSPHeaderName_WWW_Authenticate,

    //NOT modify blow!
    ERTSPHeaderName_COUNT,
} ERTSPHeaderName;

struct rtcp_stat_t {
    AM_INT  first_packet;
    AM_UINT octet_count;
    AM_UINT packet_count;
    AM_UINT last_octet_count;
    AM_S64 last_rtcp_ntp_time;
    AM_S64 first_rtcp_ntp_time;
    AM_UINT base_timestamp;
    AM_UINT timestamp;
};

struct SSubSessionInfo;
typedef struct
{
    AM_U32 addr;
    AM_U16 port;//RTP port
    AM_U16 port_ext;//RTCP port = RTP port + 1

    struct sockaddr_in addr_port;//RTP
    struct sockaddr_in addr_port_ext;//RTCP

    AM_INT socket;
    IParameters::ProtocolType protocol_type;

    AM_UINT rtp_time_base;
    AM_UINT rtp_seq_base;
    AM_UINT rtp_ssrc;
    AM_UINT stream_started;

    AM_UINT start_pts;
    AM_UINT rtp_seq;
    rtcp_stat_t rtcp_stat;
    SSubSessionInfo *parent;
} SDstAddr;

typedef struct
{
    AM_UINT index;
    IParameters::StreammingServerType type;
    IParameters::StreammingServerMode mode;

    AM_U16 listenning_port;
    AM_U16 current_data_port_base;//for simple port number allocation
    AM_INT socket;
    EServerState state;

    CDoubleLinkedList mClientSessionList;
    CDoubleLinkedList mContentList;
    CDoubleLinkedList mSDPSessionList;

    char* p_rtsp_url;
    AM_UINT rtsp_url_len;

    //options
    AM_U8 enable_video;
    AM_U8 enable_audio;
    AM_U8 reserved0[2];
} SStreammingServerInfo;

//-----------------------------------------------------------------------
//
// SStreammingContent
//
//-----------------------------------------------------------------------
#define DMaxStreamContentUrlLength 64
typedef struct {
    IParameters::StreammingContentType mType;
    IParameters::StreamFormat video_format;
    IParameters::StreamFormat audio_format;
    char mUrl[DMaxStreamContentUrlLength];

    //video info
    AM_UINT video_width, video_height;
    AM_UINT video_bit_rate, video_framerate;

    //audio info
    AM_UINT audio_channel_number, audio_sample_rate;
    AM_UINT audio_bit_rate, audio_sample_format;
} SStreammingContent;

//-----------------------------------------------------------------------
//
// IStreammingDataTransmiter
//
//-----------------------------------------------------------------------
class IStreammingDataTransmiter: public IInterface
{
public:
    DECLARE_INTERFACE(IStreammingDataTransmiter, IID_IStreammingDataTransmiter);
    virtual AM_ERR AddDstAddress(SDstAddr* p_addr, IParameters::StreamType type = IParameters::StreamType_Video) = 0;
    virtual AM_ERR RemoveDstAddress(SDstAddr* p_addr, IParameters::StreamType type = IParameters::StreamType_Video) = 0;
    virtual AM_ERR SetSrcPort(AM_U16 port, AM_U16 port_ext, IParameters::StreamType type = IParameters::StreamType_Video) = 0;
    virtual AM_ERR GetSrcPort(AM_U16& port, AM_U16& port_ext, IParameters::StreamType type = IParameters::StreamType_Video) = 0;
    virtual AM_ERR Enable(bool enable) = 0;
    virtual AM_U16 GetRTPLastSeqNumber(IParameters::StreamType type) = 0;
    virtual void GetRTPLastTime(AM_U64& last_time, IParameters::StreamType type) = 0;
};

typedef struct
{
    SStreammingContent content;
    AM_UINT index;
    IStreammingDataTransmiter* p_data_transmeter;
} SStreamContext;

//-----------------------------------------------------------------------
//
// IStreammingContentProvider
//
//-----------------------------------------------------------------------
class IStreammingContentProvider: public IInterface
{
public:
    DECLARE_INTERFACE(IStreammingContentProvider, IID_IStreammingContentProvider);
    virtual AM_UINT GetStreamContentNumber() = 0;
    virtual SStreamContext* GetStreamContent(AM_UINT index) = 0;
    virtual IStreammingDataTransmiter* GetDataTransmiter(SStreamContext*p_content, AM_UINT index) = 0;
};


//-----------------------------------------------------------------------
//
// IStreammingServerManager
//
//-----------------------------------------------------------------------

class IStreammingServerManager: public IInterface
{
public:
    DECLARE_INTERFACE(IStreammingServerManager, IID_IStreammingServerManager);

    virtual AM_ERR StartServerManager() = 0;
    virtual AM_ERR StopServerManager() = 0;
    virtual SStreammingServerInfo* AddServer(IParameters::StreammingServerType type, IParameters::StreammingServerMode mode, AM_UINT server_port, AM_U8 enable_video, AM_U8 enable_audio) = 0;
    virtual AM_ERR RemoveServer(SStreammingServerInfo* server) = 0;

    virtual AM_ERR AddStreammingContent(SStreammingServerInfo* server_info, SStreamContext* content) = 0;

//debug api
    virtual AM_ERR PrintState() = 0;
};

struct IRtspCallback{
    virtual ~IRtspCallback(){}
    virtual void OnRtpRtcpPackets(int fd,unsigned char *buf, int len, volatile int *force_exit_flag) = 0;
};
#define DMaxRTSPHeaderLen 32
typedef struct
{
    AM_U8 string[DMaxRTSPHeaderLen];
    AM_U16 type;
    AM_U16 support;
    AM_U32 methods;//can be used to check msg, to do

    ERTSPHeaderName name;
} SRTSPHeader;

extern SRTSPHeader g_RTSPHeaders[ERTSPHeaderName_COUNT];

//server/client session related:
class CRTPTransmiter
{
public:
    CRTPTransmiter();
    ~CRTPTransmiter();

public:
    AM_ERR SetSocket(AM_INT socket, AM_INT port);
    AM_ERR SendData(AM_U8* p_data, AM_UINT size);
    AM_ERR AddAddress(AM_INT socket, AM_INT port);
    AM_ERR RemoveAddress(AM_INT socket, AM_INT port);

private:
    CDoubleLinkedList mDstList;
    AM_INT mSocket;
    CMutex* mpMutex;
};

enum {
    ESubSession_video = 0,
    ESubSession_audio,

    ESubSession_tot_count,
};

struct SClientSessionInfo;
struct SSubSessionInfo
{
    ESessionServerState state;

    AM_U16 data_port;//RTP
    AM_U16 data_port_ext;//RTCP

    SDstAddr dst_addr;

    AM_UINT track_id;

    //time info, for random?
    AM_UINT tv_sec, tv_usec;

    AM_UINT rtp_seq;
    AM_UINT rtp_ts;
    AM_UINT rtp_ssrc;
    SClientSessionInfo* parent;

    //for rtp over rtsp
    AM_INT   rtp_over_rtsp;
    AM_UINT rtp_channel;
    AM_UINT rtcp_channel;
    int rtsp_fd;
    IRtspCallback *rtsp_callback;
};

struct SClientSessionInfo
{
    struct sockaddr_in client_addr;//with RTSP port
    AM_INT socket;
    AM_U16 port;

    AM_U16 get_host_string;
    AM_U8 host_addr[32];

    SStreammingServerInfo* p_server;
    IStreammingDataTransmiter* p_data_transmiter;
    void* p_context;//client request content

    //info
    char* p_sdp_info;
    AM_UINT sdp_info_len;
    char* p_session_name;
    AM_UINT session_name_len;

    AM_UINT session_id;

    SSubSessionInfo sub_session[ESubSession_tot_count];

    AM_U8 enable_video;
    AM_U8 enable_audio;
    AM_U8 reserved0[2];

    char mRecvBuffer[RTSP_MAX_BUFFER_SIZE];
    AM_INT mRecvLen;

    pthread_mutex_t lock;
    AM_S64 last_cmd_time;

    //
    AM_INT rtp_over_rtsp;
} ;

IStreammingServerManager* CreateStreamingServerManager(SConsistentConfig* pconfig);

AM_UINT getTrackID(char* string, char* string_end);
void apend_string(char*& ori, AM_UINT size, char* append_str);
char* generate_sdp_description_h264(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* url, AM_UINT tv_sec, AM_UINT tv_usec);
char* generate_sdp_description_h264_aac(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* url, AM_UINT tv_sec, AM_UINT tv_usec);
char* generate_sdp_description_aac(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* url, AM_UINT tv_sec, AM_UINT tv_usec);
bool parseTransportHeader(char const* buf, IParameters::ProtocolType& streamingMode, char* streamingModeString, char* destinationAddressStr,
        AM_U8 *destinationTTL, AM_U16 *clientRTPPortNum, AM_U16 *clientRTCPPortNum, AM_U8 *rtpChannelId, AM_U8 *rtcpChannelId);
bool parseSessionHeader(char const *buf, unsigned short &sessionId);
AM_INT SetupStreamSocket(AM_U32 localAddr,  AM_U16 localPort, bool makeNonBlocking);
AM_INT SetupDatagramSocket(AM_U32 localAddr,  AM_U16 localPort, bool makeNonBlocking);

#endif

