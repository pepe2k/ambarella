/*
 * general_parse.h
 *
 * History:
 *    2012/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_PARSE_HEDER_H__
#define __GENERAL_PARSE_HEDER_H__

typedef IGDemuxer* (*DemuxerCreater)(IFilter* pFilter, CGConfig* pConfig);
typedef AM_INT (*DemuxerParser)(const char* filename, CGConfig* pConfig);
//typedef int (*AcceptMediaFunc)(CMediaFormat& format);

typedef IGDecoder* (*DecoderCreater)(IFilter* pFilter, CGConfig* pConfig);
typedef AM_INT (*DecoderParser)(const CGBuffer* gBuffer);

typedef IGRenderer* (*RendererCreater)(IFilter* pFilter, CGConfig* pConfig);
typedef AM_INT (*RendererParser)(const CGBuffer* gBuffer);

typedef AM_ERR (*ParseClearer)();

typedef struct GDemuxerParser
{
    const char* Name;
    DemuxerCreater Creater;
    DemuxerParser Parser;
    ParseClearer Clearer;
}GDemuxerParser;


typedef struct GDecoderParser
{
    const char* Name;
    DecoderCreater Creater;
    DecoderParser Parser;
    ParseClearer Clearer;
}GDecoderParser;

typedef struct GRendererParser
{
    const char* Name;
    RendererCreater Creater;
    RendererParser Parser;
    ParseClearer Clearer;
}GRendererParser;

extern GDemuxerParser gFFMpegDemuxer;
extern GDemuxerParser gFFMpegDemuxerNet;
extern GDemuxerParser gFFMpegDemuxerQueEx;
extern GDemuxerParser gFFMpegDemuxerNetEx;

//extern GDecoderParser gDecoderCoreAVC;
extern GDecoderParser gDecoderFFMpeg;
extern GDecoderParser gVideoDspDecoderQue;
extern GDecoderParser gDecoderFFMpegVideo;

extern GRendererParser gSyncRenderer;

extern GDemuxerParser* gDemuxerList[];
extern GDecoderParser* gDecoderList[];
extern GRendererParser* gRendererList[];
#endif
