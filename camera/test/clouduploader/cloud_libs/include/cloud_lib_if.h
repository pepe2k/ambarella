/**
 * cloud_lib_if.h
 *
 * History:
 *  2013/11/27 - [Zhi He] create file
 *
 * Copyright (C) 2013, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __CLOUD_LIB_IF_H__
#define __CLOUD_LIB_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum
{
    EEncryptionType_None = 0,

    EEncryptionType_DES = 0x01,
    EEncryptionType_3DES = 0x02,
    EEncryptionType_RSA = 0x03,
    EEncryptionType_AES = 0x04,

    EEncryptionType_Customized_1 = 0x80,
    EEncryptionType_Customized_2 = 0x81,
    EEncryptionType_Customized_3 = 0x82,
} EEncryptionType;

typedef enum
{
    EProtocolHeaderExtentionType_NoHeader = 0x01,
    EProtocolHeaderExtentionType_SACP = 0x02,

    EProtocolHeaderExtentionType_Customized = 0x10,
} EProtocolHeaderExtentionType;

typedef enum
{
    EProtocolType_Invalid = 0,
    EProtocolType_SACP,
    EProtocolType_ONVIF,
} EProtocolType;

typedef enum
{
    ESACPDataChannelSubType_Invalid = 0x0,

    ESACPDataChannelSubType_H264_NALU = 0x01,
    ESACPDataChannelSubType_AAC = 0x02,
    ESACPDataChannelSubType_MPEG12Audio = 0x03,
    ESACPDataChannelSubType_G711_PCMU = 0x04,
    ESACPDataChannelSubType_G711_PCMA = 0x05,

    ESACPDataChannelSubType_RAW_PCM_S16 = 0x06,

    ESACPDataChannelSubType_Cusomized_ADPCM_S16 = 0x07,

    ESACPDataChannelSubType_AudioFlowControl_Pause = 0x90,
    ESACPDataChannelSubType_AudioFlowControl_EOS = 0x91,

} ESACPDataChannelSubType;

#define DDefaultSACPServerPort 8270
#define DDefaultClientPort 0

typedef enum
{
    ESACPConnectResult_OK = 0x0,

    ESACPConnectResult_Reject_NoSuchChannel = 0x1,
    ESACPConnectResult_Reject_ChannelIsInUse = 0x2,
    ESACPConnectResult_Reject_BadRequestFormat = 0x3,
    ESACPConnectResult_Reject_CorruptedProtocol = 0x4,
    ESACPConnectResult_Reject_Unknown = 0x5,

} ESACPConnectResult;

#define DSACPHeaderFlagBit_PacketStartIndicator (1 << 7)
#define DSACPHeaderFlagBit_PacketEndIndicator (1 << 6)
#define DSACPHeaderFlagBit_PacketKeyFrameIndicator (1 << 5)
#define DSACPHeaderFlagBit_PacketExtraDataIndicator (1 << 4)
#define DSACPHeaderFlagBit_ReplyResult (1 << 3)

//-----------------------------------------------------------------------
//
//  ICloudClientAgent
//
//-----------------------------------------------------------------------

typedef struct {
    TU16 native_major;
    TU8 native_minor;
    TU8 native_patch;

    TU16 native_date_year;
    TU8 native_date_month;
    TU8 native_date_day;

    TU16 peer_major;
    TU8 peer_minor;
    TU8 peer_patch;

    TU16 peer_date_year;
    TU8 peer_date_month;
    TU8 peer_date_day;
} SCloudAgentVersion;

#define DMaxCloudTagNameLength 32
#define DMaxCloudChannelNumber 64

typedef struct {
    TChar source_tag[DMaxCloudTagNameLength];

    TU16 channel_index;
    TU8 streaming_ready;
    TU8 remote_control_enabled;
} SCloudSourceItem;

typedef struct {
    TU16 items_number;
    TU8 reserved0;
    TU8 reserved1;

    SCloudSourceItem items[DMaxCloudChannelNumber];
} SCloudSourceItems;

typedef struct {
    //input
    float float_window_pos_x, float_window_pos_y;
    float float_window_width, float_window_height;

    //actual
    TU32 window_pos_x, window_pos_y;
    TU32 window_width, window_height;

    TU16 udec_index;
    TU8 render_index;
    TU8 display_disable;
} SDisplayItem;

typedef struct {
    //input
    float float_zoom_size_x, float_zoom_size_y;
    float float_zoom_input_center_x, float_zoom_input_center_y;

    //actual
    TU32 zoom_size_x, zoom_size_y;
    TU32 zoom_input_center_x, zoom_input_center_y;

    TU16 udec_index;
    TU8 is_valid;
    TU8 reserved1;
} SZoomItem;

typedef struct {
    TU32 pic_width, pic_height;
    TU32 bitrate, framerate;
} SSourceInfo;

#define DQUERY_MAX_DISPLAY_WINDOW_NUMBER 16
#define DQUERY_MAX_UDEC_NUMBER 16
#define DQUERY_MAX_SOURCE_NUMBER 16

typedef struct {
    float zoom_factor;
    float center_x, center_y;
} SZoomAccumulatedParameters;

typedef struct {
    SCloudAgentVersion version;//output only

//NVR transcode related

    //return persist config
    TU8 channel_number_per_group;//output only
    TU8 total_group_number;//output only
    TU8 current_group_index;//output only
    TU8 have_dual_stream;//output only

    TU8 is_vod_enabled;//output only
    TU8 is_vod_ready;//output only
    TU8 update_group_info_flag;
    TU8 update_display_layout_flag;

    //input
    TU8 update_source_flag;
    TU8 update_display_flag;
    TU8 update_audio_flag;
    TU8 update_zoom_flag;

    //audio related
    TU16 audio_source_index;//in, out
    TU16 audio_target_index;//in, out

    //encoding parameters
    TU32 encoding_width;//in, out
    TU32 encoding_height;//in, out
    TU32 encoding_bitrate;//in, out
    TU32 encoding_framerate;//in, out

    //display related
    TU16 display_layout;//in, out
    TU16 display_hd_channel_index;//in, out

    //internal use
    TU8 show_other_windows;
    TU8 focus_window_id;
    TU8 current_group;
    TU8 reserved1;

    TU16 total_window_number;//output only
    TU16 current_display_window_number;
    SDisplayItem window[DQUERY_MAX_DISPLAY_WINDOW_NUMBER];
    SZoomItem zoom[DQUERY_MAX_UDEC_NUMBER];

    //blow not used yet
    SSourceInfo source_info[DQUERY_MAX_SOURCE_NUMBER];

    //history data
    SZoomAccumulatedParameters window_zoom_history[DQUERY_MAX_DISPLAY_WINDOW_NUMBER];
} SSyncStatus;

typedef enum {
    ERemoteTrickplay_Invalid = 0,
    ERemoteTrickplay_Pause,
    ERemoteTrickplay_Resume,
    ERemoteTrickplay_Step,
    ERemoteTrickplay_EOS,
} ERemoteTrickplay;

typedef enum {
    ERemoteFlowControl_Invalid = 0,
    ERemoteFlowControl_AudioPause = ESACPDataChannelSubType_AudioFlowControl_Pause,
    ERemoteFlowControl_AudioEOS = ESACPDataChannelSubType_AudioFlowControl_EOS,
} ERemoteFlowControl;

//to NVR
class ICloudClientAgent
{
public:
    virtual EECode Authentication(TU8* authentication_string, TMemSize auth_string_size, TU8* account, TMemSize account_string_size, TU8* password, TMemSize password_size) = 0;
    virtual EECode ConnectToServer(TChar* server_url, TU16 local_port = 0, TU16 server_port = 0, TU8* authentication_buffer = NULL, TU16 authentication_length = 0) = 0;

    virtual EECode DisconnectToServer() = 0;

//misc api
public:
    virtual EECode QueryVersion(SCloudAgentVersion* version) = 0;

    virtual EECode QuerySourceList(SCloudSourceItems* items) = 0;
    virtual EECode AquireChannelControl(TU32 channel_index) = 0;
    virtual EECode ReleaseChannelControl(TU32 channel_index) = 0;

    virtual EECode GetLastErrorHint(EECode& last_error_code, TU32& error_hint1, TU32& error_hint2) = 0;

//data channel
public:
    virtual EECode StartUploading(TChar* target_url, ESACPDataChannelSubType data_type, TU32 encryption_type = 0, EProtocolHeaderExtentionType header_externtion_type = EProtocolHeaderExtentionType_NoHeader) = 0;
    virtual EECode StopUploading() = 0;
    virtual EECode UploadingFlowControl(ERemoteFlowControl control = ERemoteFlowControl_AudioPause) = 0;

public:
    virtual EECode Uploading(TU8* p_data, TMemSize size, TU8 extra_flag = 0) = 0;

//cmd channel to NVR
public:
    virtual EECode UpdateSourceBitrate(TChar* source_channel, TU32 bitrate, TU32 reply = 1) = 0;
    virtual EECode UpdateSourceFramerate(TChar* source_channel, TU32 framerate, TU32 reply = 1) = 0;
    virtual EECode UpdateSourceDisplayLayout(TChar* source_channel, TU32 layout, TU32 focus_index = 0, TU32 reply = 0) = 0;//layout: 0: single HD, 1 MxN, 2: highlighten view, 3: telepresence

    virtual EECode SwapWindowContent(TChar* source_channel, TU32 window_id_1, TU32 window_id_2, TU32 reply = 0) = 0;
    virtual EECode CircularShiftWindowContent(TChar* source_channel, TU32 shift_count = 1, TU32 is_ccw = 0, TU32 reply = 0) = 0;

    virtual EECode SwitchVideoHDSource(TChar* source_channel, TU32 hd_index, TU32 reply = 0) = 0;
    virtual EECode SwitchAudioSourceIndex(TChar* source_channel, TU32 audio_index, TU32 reply = 0) = 0;
    virtual EECode SwitchAudioTargetIndex(TChar* source_channel, TU32 audio_index, TU32 reply = 0) = 0;
    virtual EECode ZoomSource(TChar* source_channel, TU32 window_index, float zoom_factor, float zoom_input_center_x, float zoom_input_center_y, TU32 reply = 0) = 0;
    virtual EECode ZoomSource2(TChar* source_channel, TU32 window_index, float width_factor, float height_factor, float input_center_x, float input_center_y, TU32 reply = 0) = 0;
    virtual EECode UpdateSourceWindow(TChar* source_channel, TU32 window_index, float pos_x, float pos_y, float size_x, float size_y, TU32 reply = 0) = 0;
    virtual EECode SourceWindowToTop(TChar* source_channel, TU32 window_index, TU32 reply = 0) = 0;
    virtual EECode ShowOtherWindows(TChar* source_channel, TU32 window_index, TU32 show, TU32 reply = 0) = 0;

    virtual EECode DemandIDR(TChar* source_channel, TU32 demand_param = 0, TU32 reply = 0) = 0;

    virtual EECode SwitchGroup(TUint group = 0, TU32 reply = 0) = 0;
    virtual EECode SeekTo(SStorageTime* time, TU32 reply = 0) = 0;
    virtual EECode TrickPlay(ERemoteTrickplay trickplay, TU32 reply = 0) = 0;
    virtual EECode FastForward(TUint try_speed, TU32 reply = 0) = 0;//16.16 fixed point
    virtual EECode FastBackward(TUint try_speed, TU32 reply = 0) = 0;//16.16 fixed point

//for extention
    virtual EECode CustomizedCommand(TChar* source_channel, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 reply = 0) = 0;

//overall API
    virtual EECode QueryStatus(SSyncStatus* status, TU32 reply = 0) = 0;
    virtual EECode SyncStatus(SSyncStatus* status, TU32 reply = 0) = 0;

//cmd from server
public:
    virtual EECode WaitCmd(TU32& cmd_type) = 0;
    virtual EECode PeekCmd(TU32& type, TU16& payload_size, TU16& header_ext_size) = 0;
    virtual EECode ReadData(TU8* p_data, TMemSize size) = 0;
    virtual EECode WriteData(TU8* p_data, TMemSize size) = 0;

public:
    virtual TInt GetHandle() const = 0;

public:
    virtual EECode DebugPrintCloudPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintStreamingPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintRecordingPipeline(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintChannel(TU32 index, TU32 param_0, TU32 reply = 0) = 0;
    virtual EECode DebugPrintAllChannels(TU32 param_0, TU32 reply = 0) = 0;

//for externtion, textable API
    virtual EECode SendStringJson(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1) = 0;
    virtual EECode SendStringXml(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1) = 0;

public:
    virtual void Delete() = 0;
};

typedef EECode (*TServerAgentCmdCallBack) (void* owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8* pcmd, TU32 cmdsize);
typedef EECode (*TServerAgentDataCallBack) (void* owner, TMemSize read_length, TU16 data_type, TU8 flag);

class ICloudServerNVRAgent: public IScheduledClient
{
public:
    virtual EECode AcceptClient(TInt socket) = 0;
    virtual EECode CloseConnection() = 0;

    virtual EECode ReadData(TU8* p_data, TMemSize size) = 0;
    virtual EECode WriteData(TU8* p_data, TMemSize size) = 0;

    virtual EECode SetProcessCMDCallBack(void* owner, TServerAgentCmdCallBack callback) = 0;
    virtual EECode SetProcessDataCallBack(void* owner, TServerAgentDataCallBack callback) = 0;

    virtual void Delete() = 0;
};

extern ICloudClientAgent* gfCreateCloudClientAgent(EProtocolType type = EProtocolType_SACP, TU16 local_port = DDefaultClientPort, TU16 server_port = DDefaultSACPServerPort);
extern ICloudServerNVRAgent* gfCreateCloudServerNVRAgent(EProtocolType type = EProtocolType_SACP);

extern ECMDType gfSACPClientSubCmdTypeToGenericCmdType(TU16 sub_type);

typedef enum {
    ESACPElementType_Invalid = 0x0000,

    ESACPElementType_GlobalSetting = 0x0001,
    ESACPElementType_SyncFlags = 0x0002,

    ESACPElementType_EncodingParameters = 0x0003,
    ESACPElementType_DisplaylayoutInfo = 0x0004,
    ESACPElementType_DisplayWindowInfo = 0x0005,
    ESACPElementType_DisplayZoomInfo = 0x0006,

} ESACPElementType;

typedef struct {
    TU8 type1;
    TU8 type2;
    TU8 size1;
    TU8 size2;
} SSACPElementHeader;

#define DSACP_FIX_POINT_DEN 0x01000000
#define DSACP_FIX_POINT_SYGN_FLAG 0x80000000

extern TMemSize gfWriteSACPElement(TU8* p_src, TMemSize max_size, ESACPElementType type, TU32 param0, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 param6, TU32 param7);
extern TMemSize gfReadSACPElement(TU8* p_src, TMemSize max_size, ESACPElementType& type, TU32& param0, TU32& param1, TU32& param2, TU32& param3, TU32& param4, TU32& param5, TU32& param6, TU32& param7);

//version related
#define DCloudLibVesionMajor 0
#define DCloudLibVesionMinor 1
#define DCloudLibVesionPatch 10
#define DCloudLibVesionYear 2014
#define DCloudLibVesionMonth 4
#define DCloudLibVesionDay 02

void gfGetCloudLibVersion(TU32& major, TU32& minor, TU32& patch, TU32& year, TU32& month, TU32& day);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

