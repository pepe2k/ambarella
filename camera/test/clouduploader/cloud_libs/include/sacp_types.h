/**
 * sacp_types.h
 *
 * History:
 *  2013/11/29 - [Zhi He] create file
 *
 * Copyright (C) 2013, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __SACP_TYPES_H__
#define __SACP_TYPES_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum
{
    ESACPTypeCategory_Invalid = 0x0,

    ESACPTypeCategory_ConnectionTag = 0x01,

    ESACPTypeCategory_DataChannel = 0x04,

    ESACPTypeCategory_ClientCmdChannel = 0x10,
    ESACPTypeCategory_NVRCmdChannel = 0x11,
    ESACPTypeCategory_ControlServerCmdChannel = 0x12,

} ESACPTypeClientCategory;

typedef enum
{
    ESACPClientCmdSubType_Invalid = 0x0,

    //source device to authentication server
    ESACPClientCmdSubType_SourceRegister = 0x01,
    ESACPClientCmdSubType_SourceUnRegister = 0x02,
    ESACPClientCmdSubType_SourceAuthentication = 0x03,
    ESACPClientCmdSubType_SourceGrantSinkPrivilege = 0x04,
    ESACPClientCmdSubType_SourceRemoveSinkPrivilege = 0x05,
    ESACPClientCmdSubType_SourceUpdateStoragePlan = 0x06,

    //sink device to authentication server
    ESACPClientCmdSubType_SinkRegister = 0x10,
    ESACPClientCmdSubType_SinkUnRegister = 0x12,
    ESACPClientCmdSubType_SinkAuthentication = 0x13,
    ESACPClientCmdSubType_SinkQuerySource = 0x14,
    ESACPClientCmdSubType_SinkQuerySourcePrivilege = 0x15,
    ESACPClientCmdSubType_SinkAddSourceDevice = 0x16,
    ESACPClientCmdSubType_SinkRemoveSourceDevice = 0x17,

    //source device to NVR
    ESACPClientCmdSubType_SourceReAuthentication = 0x20,
    ESACPClientCmdSubType_SourceStartPush = 0x22,
    ESACPClientCmdSubType_SourceStopPush = 0x23,

    //sink device to NVR
    ESACPClientCmdSubType_SinkReAuthentication = 0x30,
    ESACPClientCmdSubType_SinkStartPlayback = 0x32,
    ESACPClientCmdSubType_SinkStopPlayback = 0x33,
    ESACPClientCmdSubType_SinkUpdateStoragePlan = 0x34,
    ESACPClientCmdSubType_SinkSwitchSource = 0x35,
    ESACPClientCmdSubType_SinkQueryRecordHistory = 0x36,
    ESACPClientCmdSubType_SinkSeekTo = 0x37,
    ESACPClientCmdSubType_SinkFastForward = 0x38,
    ESACPClientCmdSubType_SinkFastBackward = 0x39,
    ESACPClientCmdSubType_SinkTrickplay = 0x3a,
    ESACPClientCmdSubType_SinkQueryCurrentStoragePlan = 0x3b,
    ESACPClientCmdSubType_SinkPTZControl = 0x3c,
    ESACPClientCmdSubType_SinkUpdateFramerate = 0x3d,
    ESACPClientCmdSubType_SinkUpdateBitrate = 0x3e,
    ESACPClientCmdSubType_SinkUpdateResolution = 0x3f,

    ESACPClientCmdSubType_SinkUpdateDisplayLayout = 0x40,
    ESACPClientCmdSubType_SinkZoom = 0x41,
    ESACPClientCmdSubType_SinkUpdateDisplayWindow = 0x42,
    ESACPClientCmdSubType_SinkFocusDisplayWindow = 0x43,
    ESACPClientCmdSubType_SinkSelectAudioSourceChannel = 0x44,
    ESACPClientCmdSubType_SinkSelectAudioTargetChannel = 0x45,
    ESACPClientCmdSubType_SinkSelectVideoHDChannel = 0x46,
    ESACPClientCmdSubType_SinkShowOtherWindows = 0x47,
    ESACPClientCmdSubType_SinkForceIDR = 0x48,
    ESACPClientCmdSubType_SinkSwapWindowContent = 0x49,
    ESACPClientCmdSubType_SinkCircularShiftWindowContent = 0x4a,
    ESACPClientCmdSubType_SinkZoomV2 = 0x4b,

    ESACPClientCmdSubType_SinkSwitchGroup = 0x50,

    ESACPClientCmdSubType_PeerClose = 0x60,

    //misc
    ESACPClientCmdSubType_QueryVersion = 0x70,
    ESACPClientCmdSubType_QueryStatus = 0x71,
    ESACPClientCmdSubType_SyncStatus = 0x72,
    ESACPClientCmdSubType_QuerySourceList = 0x73,
    ESACPClientCmdSubType_AquireChannelControl = 0x74,
    ESACPClientCmdSubType_ReleaseChannelControl = 0x75,

    //for extention
    ESACPClientCmdSubType_CustomizedCommand = 0x80,

    //debug device to nvr server
    ESACPDebugCmdSubType_PrintCloudPipeline = 0x90,
    ESACPDebugCmdSubType_PrintStreamingPipeline = 0x91,
    ESACPDebugCmdSubType_PrintRecordingPipeline = 0x92,
    ESACPDebugCmdSubType_PrintSingleChannel = 0x93,
    ESACPDebugCmdSubType_PrintAllChannels = 0x94,

    //
    ESACPClientCmdSubType_ResponceOnlyForRelay = 0xa0,

    //textable API
    ESACPDebugCmdSubType_TextableAPIJson = 0xd0,
    ESACPDebugCmdSubType_TextableAPIXml = 0xd1,
} ESACPClientCmdSubType;

typedef enum
{
    ESACPAuthenticationServerSubType_Invalid = 0x0,

    //authentication server to NVR
    ESACPAuthenticationServerSubType_QueryFreeChannelCount = 0x01,
    ESACPAuthenticationServerSubType_AssignSourceDevice = 0x02,
    ESACPAuthenticationServerSubType_AssignSinkDevice = 0x03,
    ESACPAuthenticationServerSubType_ReleaseSourceDevice = 0x04,
    ESACPAuthenticationServerSubType_ReleaseSinkDevice = 0x05,

    ESACPAuthenticationServerSubType_PeerClose = 0x60,
} ESACPAuthenticationServerSubType;

typedef enum
{
    ESACPCloudDataServerSubType_Invalid = 0x0,

    //cloud data server to client
    ESACPCloudDataServerSubType_ReAuthentication = 0x01,
} ESACPCloudDataServerSubType;

#define DSACPTypeBit_Responce  (1 << 31)
#define DSACPTypeBit_Request    (1 << 30)
#define DSACPTypeBit_NeedReply    (1 << 29)
#define DSACPTypeBit_CatTypeMask    (0xff)
#define DSACPTypeBit_CatTypeBits    16
#define DSACPTypeBit_SubTypeMask    (0xffff)
#define DSACPTypeBit_SubTypeBits    0

#define DBuildSACPType(reqres_bits, cat, sub_type) \
    ((reqres_bits) | ((cat & DSACPTypeBit_CatTypeMask) << DSACPTypeBit_CatTypeBits) | ((sub_type & DSACPTypeBit_SubTypeMask) << DSACPTypeBit_SubTypeBits))

typedef struct
{
    TU8 type_1;
    TU8 type_2;
    TU8 type_3;
    TU8 type_4;

    TU8 size_1;
    TU8 size_2;

    TU8 seq_count_1;
    TU8 seq_count_2;

    TU8 encryption_type_1;
    TU8 encryption_type_2;
    TU8 encryption_type_3;
    TU8 encryption_type_4;

    TU8 flags;
    TU8 header_ext_type;
    TU8 header_ext_size_1;
    TU8 header_ext_size_2;
}__attribute__((packed)) SSACPHeader;

typedef struct
{
    TU8 mac_addr[16];
    TU8 ip_addr[16];

    TU8 date_year_1;
    TU8 date_year_2;
    TU8 date_month;
    TU8 date_day;

    TU8 time_hour;
    TU8 time_minute;
    TU8 time_seconds;
    TU8 time_reserved;

    TU8 token_part1_1;
    TU8 token_part1_2;
    TU8 token_part1_3;
    TU8 token_part1_4;

    TU8 token_part2_1;
    TU8 token_part2_2;
    TU8 token_part2_3;
    TU8 token_part2_4;
} __attribute__((packed)) SSACPHeaderExterntion;

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

