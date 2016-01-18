
/**
 * streamming_if.cpp
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
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

#include "streamming_if.h"

// {DD6A35AD-4DE2-4D85-83E4-EA2D4739C08}
AM_DEFINE_IID(IID_IStreammingServerManager,
0xdd6a35ad, 0x4de2, 0x4d85, 0x83, 0xe4, 0xea, 0x2d, 0xb4, 0x73, 0x9c, 0x08);
////////////////////////////////////////////////////////////////////////////

// {ef1dbc9f-a19f-4898-b842-7c18b6ad2429}
AM_DEFINE_IID(IID_IStreammingContentProvider,
0xef1dbc9f, 0xa19f, 0x4898, 0xb8, 0x42, 0x7c, 0x18, 0xb6, 0xad, 0x24, 0x29);
////////////////////////////////////////////////////////////////////////////

// {92a96861-8ffd-4db2-bc04-e580c53d7ee5}
AM_DEFINE_IID(IID_IStreammingDataTransmiter,
0x92a96861, 0x8ffd, 0x4db2, 0xbc, 0x04, 0xe5, 0x80, 0xc5, 0x3d, 0x7e, 0xe5);
////////////////////////////////////////////////////////////////////////////

#if 0
// {e2e25402-74de-4ba8-8afa-c35a2e07be0e}
AM_DEFINE_IID(IID_IStreammingDataPort,
0xe2e25402, 0x74de, 0x4ba8, 0x8a, 0xfa, 0xc3, 0x5a, 0x2e, 0x07, 0xbe, 0x0e);
////////////////////////////////////////////////////////////////////////////
#endif

#define VIDEO_MISC_SDP_LINES  "m=video 0 RTP/AVP 96\r\n"\
"c=IN IP4 0.0.0.0\r\n"\
"b=AS:2000\r\n"\
"a=rtpmap:96 H264/90000\r\n"\
"a=fmtp:96 packetization-mode=1;profile-level-id=42A01E\r\n"\
"a=control:trackID=0\r\n"

#define AUDIO_MISC_SDP_LINES  "m=audio 0 RTP/AVP 97\r\n"\
"c=IN IP4 0.0.0.0\r\n"\
"b=RR:0\r\n"\
"a=rtpmap:97 mpeg4-generic/%d/%d\r\n"\
"a=fmtp:97 StreamType=5; profile-level-id=15; mode=AAC-hbr; config=%d; SizeLength=13;IndexLength=3;IndexDeltaLength=3\r\n"\
"a=control:trackID=1\r\n"

SRTSPHeader g_RTSPHeaders[ERTSPHeaderName_COUNT] =
{
    {"Accept",                  (AM_U16)ERTSPHeaderType_request,        (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                         ERTSPHeaderName_Accept},
    {"Accept-Encoding",     (AM_U16)ERTSPHeaderType_request,        (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                         ERTSPHeaderName_Accept_Encoding},
    {"Accept-Language",     (AM_U16)ERTSPHeaderType_request,        (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                         ERTSPHeaderName_Accept_Language},
    {"Allow",                       (AM_U16)ERTSPHeaderType_response,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                         ERTSPHeaderName_Allow},
    {"Authorization",            (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                         ERTSPHeaderName_Authorization},
    {"Bandwidth",               (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Bandwidth},
    {"Blocksize",                 (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(~(ESessionMethod_RTSP_OPTIONS | ESessionMethod_RTSP_TEARDOWN)),                         ERTSPHeaderName_Blocksize},
    {"Cache-Control",         (AM_U16)ERTSPHeaderType_general,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)ESessionMethod_RTSP_SETUP,                                                                                    ERTSPHeaderName_Cache_Control},
    {"Conference",              (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)ESessionMethod_RTSP_SETUP,                                                                                   ERTSPHeaderName_Conference},
    {"Connection",              (AM_U16)ERTSPHeaderType_general,       (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_Connection},
    {"Content-Base",          (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                             ERTSPHeaderName_Content_Base},
    {"Content-Encoding",    (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)ESessionMethod_RTSP_SET_PARAMETER,                                                                       ERTSPHeaderName_Content_Encoding_1},
    {"Content-Encoding",    (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)(ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE),                               ERTSPHeaderName_Content_Encoding_2},
    {"Content-Language",    (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)(ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE),                              ERTSPHeaderName_Content_Language},
    {"Content-Length",        (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)(ESessionMethod_RTSP_SET_PARAMETER | ESessionMethod_RTSP_ANNOUNCE),                    ERTSPHeaderName_Content_Length_1},
    {"Content-Length",        (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                           ERTSPHeaderName_Content_Length_2},
    {"Content-Location",     (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_Content_Location},
    {"Content-Type",          (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)(ESessionMethod_RTSP_SET_PARAMETER | ESessionMethod_RTSP_ANNOUNCE),                    ERTSPHeaderName_Content_Type_1},
    {"Content-Type",          (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_Content_Type_1},
    {"CSeq",                      (AM_U16)ERTSPHeaderType_general,       (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_CSeq},
    {"Date",                       (AM_U16)ERTSPHeaderType_general,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_Date},
    {"Expires",                    (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE),                              ERTSPHeaderName_Expires},
    {"From",                       (AM_U16)ERTSPHeaderType_request,      (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                            ERTSPHeaderName_From},
    {"If-Modified-Since",      (AM_U16)ERTSPHeaderType_request,      (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_SETUP),                                    ERTSPHeaderName_If_Modified_Since},
    {"Last-Modified",           (AM_U16)ERTSPHeaderType_entity,         (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                           ERTSPHeaderName_Last_Modified},
    {"Proxy-Authenticate",   (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                           ERTSPHeaderName_Proxy_Authenticate},
    {"Proxy-Require",           (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                           ERTSPHeaderName_Proxy_Require},
    {"Public",                      (AM_U16)ERTSPHeaderType_response,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                        ERTSPHeaderName_Public},
    {"Range",                      (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(ESessionMethod_RTSP_PLAY|ESessionMethod_RTSP_PAUSE| ESessionMethod_RTSP_RECORD),    ERTSPHeaderName_Range_1},
    {"Range",                      (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(ESessionMethod_RTSP_PLAY|ESessionMethod_RTSP_PAUSE| ESessionMethod_RTSP_RECORD),    ERTSPHeaderName_Range_2},
    {"Referer",                    (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Referer},
    {"Require",                    (AM_U16)ERTSPHeaderType_request,       (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Require},
    {"Retry-After",               (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Retry_After},
    {"RTP-Info",                  (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)ESessionMethod_RTSP_PLAY,                                                                                    ERTSPHeaderName_RTP_Info},
    {"Scale",                       (AM_U16)ERTSPHeaderType_response | (AM_U16)ERTSPHeaderType_request,     (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)(ESessionMethod_RTSP_PLAY|ESessionMethod_RTSP_RECORD),  ERTSPHeaderName_Scale},
    {"Session",                    (AM_U16)ERTSPHeaderType_response | (AM_U16)ERTSPHeaderType_request,     (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)(~(ESessionMethod_RTSP_SETUP|ESessionMethod_RTSP_OPTIONS)),  ERTSPHeaderName_Session},
    {"Server",                    (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)0xffffffff,                                                                                                           ERTSPHeaderName_Server},
    {"Speed",                    (AM_U16)ERTSPHeaderType_response | (AM_U16)ERTSPHeaderType_request,     (AM_U16)ERTSPHeaderSupport_optional,        (AM_U32)ESessionMethod_RTSP_PLAY,                                 ERTSPHeaderName_Speed},
    {"Transport",               (AM_U16)ERTSPHeaderType_response | (AM_U16)ERTSPHeaderType_request,     (AM_U16)ERTSPHeaderSupport_required,        (AM_U32)ESessionMethod_RTSP_SETUP,                               ERTSPHeaderName_Transport},
    {"Unsupported",           (AM_U16)ERTSPHeaderType_response,     (AM_U16)ERTSPHeaderSupport_required,         (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Unsupported},
    {"User-Agent",             (AM_U16)ERTSPHeaderType_request,      (AM_U16)ERTSPHeaderSupport_optional,         (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_User_Agent},
    {"Via",                        (AM_U16)ERTSPHeaderType_general,      (AM_U16)ERTSPHeaderSupport_optional,         (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_Via},
    {"WWW-Authenticate",  (AM_U16)ERTSPHeaderType_response,   (AM_U16)ERTSPHeaderSupport_optional,         (AM_U32)0xffffffff,                                                                                                          ERTSPHeaderName_WWW_Authenticate},
};

void apend_string(char*& ori, AM_UINT& tot_size, char* append_str)
{
    AM_UINT append_len = 0;
    AM_UINT ori_len = 0;
    char* tobe_freed = NULL;
    if (!append_str) {
        AM_ERROR("NULL pointer here.\n");
        return;
    }

    append_len = strlen(append_str);
    if (ori) {
        ori_len = strlen(ori);
    }

    if (!ori || (append_len + ori_len + 1) > tot_size) {
        tobe_freed = ori;
        ori = (char*)malloc(append_len + ori_len + 16);
        if (ori) {
            tot_size = append_len + ori_len + 16;
        }
        if (tobe_freed) {
            snprintf(ori, tot_size, "%s%s", tobe_freed, append_str);
            free(tobe_freed);
        } else {
            strncpy(ori, append_str, tot_size);
        }
    } else {
        strncat(ori, append_str, append_len);
    }
}

char* generate_sdp_description_h264(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* rtspURL, AM_UINT tv_sec, AM_UINT tv_usec)
{
    //AM_U32 OurIPAddress;
    const char* const libNameStr = "Ambarella Streaming Media from iOne";
    const char* const libVersionStr = "2012.04.23";
    const char* const sourceFilterLine = "";
    const char* const rangeLine = "a=range:npt=0-\r\n";

    char const* const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    AM_UINT sdp_length = strlen(sdpPrefixFmt)
        + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(libNameStr) + strlen(libVersionStr)
        + strlen(rtspURL)
        + strlen(sourceFilterLine)
        + strlen(rangeLine)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(VIDEO_MISC_SDP_LINES);

    char* p_str = (char*)malloc(sdp_length + 4);
    AM_ASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
        tv_sec, tv_usec, // o= <session id>
        1, // o= <version> // (needs to change if params are modified)
        ipAddressStr, // o= <address>
        pDescription, // s= <description>
        pInfo, // i= <info>
        libNameStr, libVersionStr, // a=tool:
        rtspURL, //a=control
        sourceFilterLine, // a=source-filter: incl (if a SSM session)
        rangeLine, // a=range: line
        pDescription, // a=x-qt-text-nam: line
        pInfo, // a=x-qt-text-inf: line
        VIDEO_MISC_SDP_LINES); // miscellaneous session SDP lines (if any)

	return p_str;
}

char* generate_sdp_description_h264_aac(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* rtspURL, AM_UINT tv_sec, AM_UINT tv_usec)
{
    //AM_U32 OurIPAddress;
    const char* const libNameStr = "Ambarella Streaming Media from iOne";
    const char* const libVersionStr = "2012.04.23";
    const char* const sourceFilterLine = "";
    const char* const rangeLine = "a=range:npt=0-\r\n";

    AM_UINT maxMiscSDPLinesLen = strlen(VIDEO_MISC_SDP_LINES) + strlen(AUDIO_MISC_SDP_LINES) + 64;

    char* tmpMiscSDPLines = (char*)malloc(maxMiscSDPLinesLen);
    if(!tmpMiscSDPLines){
    }
    memset(tmpMiscSDPLines, 0, maxMiscSDPLinesLen);

    SStreamContext* p_context = (SStreamContext*)context;
    unsigned short config = 0;
    //audio sepcific config, harcode here, refer to iso14496-3
    if(p_context->content.audio_sample_rate == 44100){
        config = 1210;
    }else if(p_context->content.audio_sample_rate == 48000){
        config = 1190;
    }
    snprintf(tmpMiscSDPLines, maxMiscSDPLinesLen, "%s", VIDEO_MISC_SDP_LINES);
    snprintf(tmpMiscSDPLines + strlen(tmpMiscSDPLines), maxMiscSDPLinesLen, AUDIO_MISC_SDP_LINES, p_context->content.audio_sample_rate, p_context->content.audio_channel_number, config);

    char const* const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    AM_UINT sdp_length = strlen(sdpPrefixFmt)
        + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(libNameStr) + strlen(libVersionStr)
        + strlen(rtspURL)
        + strlen(sourceFilterLine)
        + strlen(rangeLine)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(tmpMiscSDPLines);

    char* p_str = (char*)malloc(sdp_length + 4);
    AM_ASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
        tv_sec, tv_usec, // o= <session id>
        1, // o= <version> // (needs to change if params are modified)
        ipAddressStr, // o= <address>
        pDescription, // s= <description>
        pInfo, // i= <info>
        libNameStr, libVersionStr, // a=tool:
        rtspURL, //a=control
        sourceFilterLine, // a=source-filter: incl (if a SSM session)
        rangeLine, // a=range: line
        pDescription, // a=x-qt-text-nam: line
        pInfo, // a=x-qt-text-inf: line
        tmpMiscSDPLines); // miscellaneous session SDP lines (if any)

   free(tmpMiscSDPLines);

	return p_str;
}

char* generate_sdp_description_aac(void* context, char const* pStreamName, char const* pInfo, char const* pDescription, char const* ipAddressStr, char* rtspURL, AM_UINT tv_sec, AM_UINT tv_usec)
{
    //AM_U32 OurIPAddress;
    const char* const libNameStr = "Ambarella Streaming Media from iOne";
    const char* const libVersionStr = "2012.04.23";
    const char* const sourceFilterLine = "";
    const char* const rangeLine = "a=range:npt=0-\r\n";

    char* tmpMiscSDPLines = (char*)malloc(strlen(AUDIO_MISC_SDP_LINES) + 64);
    if(!tmpMiscSDPLines){
    }
    memset(tmpMiscSDPLines, 0, strlen(AUDIO_MISC_SDP_LINES) + 64);

    SStreamContext* p_context = (SStreamContext*)context;
    unsigned short config = 0;

    //audio sepcific config, harcode here, refer to iso14496-3
    if(p_context->content.audio_sample_rate == 44100){
        config = 1210;
    }else if(p_context->content.audio_sample_rate == 48000){
        config = 1190;
    }
    snprintf(tmpMiscSDPLines, strlen(AUDIO_MISC_SDP_LINES) + 64, AUDIO_MISC_SDP_LINES, p_context->content.audio_sample_rate, p_context->content.audio_channel_number, config);

    char const* const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    AM_UINT sdp_length = strlen(sdpPrefixFmt)
        + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(libNameStr) + strlen(libVersionStr)
        + strlen(rtspURL)
        + strlen(sourceFilterLine)
        + strlen(rangeLine)
        + strlen(pDescription)
        + strlen(pInfo)
        + strlen(tmpMiscSDPLines);

    char* p_str = (char*)malloc(sdp_length + 4);
    AM_ASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
        tv_sec, tv_usec, // o= <session id>
        1, // o= <version> // (needs to change if params are modified)
        ipAddressStr, // o= <address>
        pDescription, // s= <description>
        pInfo, // i= <info>
        libNameStr, libVersionStr, // a=tool:
        rtspURL, //a=control
        sourceFilterLine, // a=source-filter: incl (if a SSM session)
        rangeLine, // a=range: line
        pDescription, // a=x-qt-text-nam: line
        pInfo, // a=x-qt-text-inf: line
        tmpMiscSDPLines); // miscellaneous session SDP lines (if any)

   free(tmpMiscSDPLines);

	return p_str;
}

static char* _strDupSize(char const* str) {
	if (str == NULL) return NULL;
	size_t len = strlen(str) + 1;
	char* copy = new char[len];

	return copy;
}

AM_UINT getTrackID(char* string, char* string_end)
{
    char* ptmp = string;
    AM_UINT track_id = 0;
    AM_INT count = 4;

    while (count > 0) {
        ptmp = strchr(ptmp, 't');
        if ((!ptmp) || (ptmp >= string_end)) {
            AM_ERROR("cannot find 'trackID='.\n");
            return 0;
        }

        if (!strncmp(ptmp, "trackID=", 8)) {
            sscanf(ptmp, "trackID=%d", &track_id);
            return track_id;
        }

        count --;
    }

    AM_ERROR("cannot get 'trackID=', should not comes here.\n");
    return 0;
}

bool parseTransportHeader(char const* buf,
    IParameters::ProtocolType& streamingMode,
    char* streamingModeString,
    char* destinationAddressStr,
    AM_U8 *destinationTTL,
    AM_U16 *clientRTPPortNum, // if UDP
    AM_U16 *clientRTCPPortNum, // if UDP
    AM_U8 *rtpChannelId, // if TCP
    AM_U8 *rtcpChannelId // if TCP
    )
{
    // Initialize the result parameters to default values:
    streamingMode = IParameters::ProtocolType_UDP;
    *destinationTTL = 255;
    *clientRTPPortNum = 0;
    *clientRTCPPortNum = 1;
    *rtpChannelId = *rtcpChannelId = 0xFF;

    AM_U16 p1, p2;
    AM_U32 ttl, rtpCid, rtcpCid;

    // First, find "Transport:"
    while (1) {
        if (*buf == '\0') return false; // not found
        if (strncasecmp(buf, "Transport: ", 11) == 0) break;
        ++buf;
    }

    // Then, run through each of the fields, looking for ones we handle:
    char const* fields = buf + 11;
    char* field = _strDupSize(fields);

    while (sscanf(fields, "%[^;]", field) == 1) {
        if (strcmp(field, "RTP/AVP/TCP") == 0) {
            streamingMode = IParameters::ProtocolType_TCP;
        } else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
        strcmp(field, "MP2T/H2221/UDP") == 0) {
            streamingMode = IParameters::ProtocolType_UDP;
            strncpy(streamingModeString, field, strlen(streamingModeString));
        } else if (strncasecmp(field, "destination=", 12) == 0) {
            delete[] destinationAddressStr;
            strncpy(destinationAddressStr, field+12, strlen(destinationAddressStr));
        } else if (sscanf(field, "ttl%u", &ttl) == 1) {
            *destinationTTL = (u_int8_t)ttl;
        } else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2) {
            *clientRTPPortNum = p1;
            *clientRTCPPortNum = p2;
        } else if (sscanf(field, "client_port=%hu", &p1) == 1) {
            *clientRTPPortNum = p1;
            *clientRTCPPortNum = (streamingMode == IParameters::ProtocolType_UDP) ? 0 : p1 + 1;
        } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
            *rtpChannelId = (unsigned char)rtpCid;
            *rtcpChannelId = (unsigned char)rtcpCid;
        }

        fields += strlen(field);
        while (*fields == ';') ++fields; // skip over separating ';' chars
        if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
    }

    delete[] field;
    return true;
}

bool parseSessionHeader(char const *buf, unsigned short &sessionId)
{
    while (1) {
        if (*buf == '\0') return false;
        if (strncasecmp(buf, "Session: ", 9) == 0) break;
        ++buf;
    }
    unsigned int session;
    if (sscanf(buf, "Session: %X\r\n", &session) == 1){
        sessionId = (unsigned short)session;
        return true;
    }
    return false;
}

AM_INT SetupStreamSocket(AM_U32 localAddr,  AM_U16 localPort, bool makeNonBlocking)
{
    struct sockaddr_in  servaddr;
    AM_INT newSocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr =  htonl(localAddr);
    servaddr.sin_port  = htons(localPort);

    if (newSocket < 0) {
        AM_ERROR("unable to create stream socket\n");
        return newSocket;
    }

    int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&reuseFlag, sizeof reuseFlag) < 0) {
        AM_ERROR("setsockopt(SO_REUSEADDR) error\n");
        close(newSocket);
        return -1;
    }

    if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        AM_ERROR("bind() error (port number: %d)\n", localPort);
        close(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        AM_INT curFlags = fcntl(newSocket, F_GETFL, 0);
        if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) != 0) {
            AM_ERROR("failed to make non-blocking\n");
            close(newSocket);
            return -1;
        }
    }

    AM_INT requestedSize = 50*1024;
    if (setsockopt(newSocket, SOL_SOCKET, SO_SNDBUF,
        &requestedSize, sizeof requestedSize) != 0) {
        AM_ERROR("failed to set send buffer size\n");
        close(newSocket);
        return -1;
    }

    if (listen(newSocket, 20) < 0) {
        AM_ERROR("listen() failed\n");
        return -1;
    }

    return newSocket;
}

AM_INT SetupDatagramSocket(AM_U32 localAddr,  AM_U16 localPort, bool makeNonBlocking)
{
    struct sockaddr_in  servaddr;

    AM_INT newSocket = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr    =  htonl(localAddr);
    servaddr.sin_port   =   htons(localPort);

    if (newSocket < 0) {
        AM_ERROR("unable to create stream socket\n");
        return newSocket;
    }

    int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&reuseFlag, sizeof reuseFlag) < 0) {
        AM_ERROR("setsockopt(SO_REUSEADDR) error\n");
        close(newSocket);
        return -1;
    }

    if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        AM_ERROR("bind() error (port number: %d)\n", localPort);
        close(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        AM_INT curFlags = fcntl(newSocket, F_GETFL, 0);
        if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) != 0) {
            AM_ERROR("failed to make non-blocking\n");
            close(newSocket);
            return -1;
        }
    }

    return newSocket;
}

