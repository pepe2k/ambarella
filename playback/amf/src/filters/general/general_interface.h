/*
 * general_interface.h
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

#ifndef __GENERAL_INTERFACE_HEDER_H__
#define __GENERAL_INTERFACE_HEDER_H__

//Info Type
enum InfoTypeGmf
{
    INFO_TIME_PTS,
    FRAME_BUFFER_NUM,
    VOUT_STATE,
    VIDEO_WIDTH_HEIGHT,
    DATA_FEED_TO_BSB,
};

//CMD Related
enum {
    CMD_ISREADY = IActiveObject::CMD_LAST,
    CMD_DECODE, //CDSPDECODE
    CMD_FIRST_SOURCE, //CGeneralDemuxer D-I-S-C-A-R-D
    CMD_EOS, //CFFMEPGEDECODE
    CMD_DIS_AUDIO, //CGR
    CMD_EN_AUDIO,
    CMD_ADD_AUDIO,
    CMD_CLEAR,
    CMD_ISEOS, //VideoRender Out
    CMD_CONFIG_WIN, //VOUT
    CMD_SWITCH_HD, //VOUT
    CMD_SWITCH_BACK,
    CMD_ADAPT, //FOR HD RE-ADAPT
    CMD_SEEK, //FOR DEMUXER
    CMD_DEC_PB_SPEED, //FOR DECODER PB speed and Feeding rule
    CMD_DEC_REMOVE, //NOTIFY DEC DETACH BUFFERPOOL
    CMD_RETRY_DECODE, //ERROR HANDLE
    CMD_BW_PLAYBACK_DIRECTION,
    CMD_FW_PLAYBACK_DIRECTION,
    CMD_PLAYBACK_ZOOM,
    CMD_DISCONNECT_HW, //SYNC RENDER
    CMD_GMF_LAST,
};

enum CMD_TARGET{
    CMD_EACH_COMPONENT = 100,
    CMD_GENERAL_ONLY = 200,
};

//U8 FLAG
enum CMD_FLAG{
    RESUME_TO_IDLE = 0x01,
    RESUME_BUFFER_RETRIEVE = 0x02,
    SEEK_BY_SWITCH = 0x04,
    SEEK_BY_USER = 0x100,
    SEEK_FOR_LOOP = 0x80,
    CHECK_VDEC_FRAME_NET = 0x08,//not used, use queryinfo
    CONFIG_DEMUXER_GO_RUN = 0x10,
    DEC_REMOVE_JUST_NOTIFY = 0x20, //notify to detach bufferpool
    SYNC_AUDIO_LASTPTS = 0x40,
};

class IGDemuxer : public IInterface
{
public:
    //DECLARE_INTERFACE(IGDemuxer, IID_IGDemuxer);
    virtual AM_ERR LoadFile() = 0;
    virtual AM_ERR SeekTo(AM_U64 ms, AM_INT flag) = 0;
    virtual AM_ERR GetTotalLength(AM_U64& ms) = 0;
    virtual AM_ERR GetCGBuffer(CGBuffer& buffer) = 0;
    virtual AM_ERR GetAudioBuffer(CGBuffer& oBuffer) = 0;//a-v spe
    virtual const CQueue* GetBufferQ() = 0;

    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param) = 0;
    virtual AM_ERR OnRetrieveBuffer(CGBuffer* buffer) = 0;
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer) =0;
    virtual void Dump(AM_INT flag) = 0;
};


class IGDecoder : public IInterface
{
public:
    //DECLARE_INTERFACE(IDecoder, IID_IGDecoder);
    virtual AM_ERR AdaptStream(CGBuffer* pBuffer) = 0;
    virtual AM_ERR IsReady(CGBuffer* pBuffer) = 0;
    virtual AM_ERR Decode(CGBuffer* pBuffer) = 0;//need handle data/eos
    virtual AM_ERR GetDecodedGBuffer(CGBuffer& oBuffer) = 0; // add this for queued buffer
    virtual AM_ERR FillEOS() = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param) = 0;
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer) = 0;
    virtual void Dump() = 0;
};

class IGRenderer : public IInterface
{
public:
    //DECLARE_INTERFACE(IDecoder, IID_IGDecoder);
    virtual AM_ERR CheckAnother(CGBuffer* pBuffer) = 0;
    virtual AM_ERR IsReady(CGBuffer* pBuffer) = 0;
    virtual AM_ERR Render(CGBuffer* pBuffer) = 0;//need handle data/eos
    virtual AM_ERR FillEOS(CGBuffer* pBuffer) = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam& param) = 0;
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer) = 0;
    virtual AM_ERR PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y) = 0;
    virtual void Dump() = 0;
};
/*
class IGTranscoder : public IInterface
{
public:
    virtual AM_ERR IsReady(CGBuffer* pBuffer) = 0;
    virtual AM_ERR SetEncParam(AM_INT w, AM_INT h, AM_INT br) = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam& param) = 0;
    virtual void Dump() = 0;
};
*/
class IRenderOut : public IInterface
{
public:
    //DECLARE_INTERFACE(IRenderOut, IID_IRenderOut);
    virtual AM_ERR OpenHal(CGBuffer* handleBuffer) = 0;
    virtual AM_ERR RenderOutBuffer(CGBuffer* pBuffer) = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    //virtual void Dump() = 0;
};

//----------------------------------------------
//
//----------------------------------------------
class IGMuxer: public IInterface
{
public:
    enum{
        MUXER_MSG_ERROR,
    };
    virtual AM_ERR ConfigMe(CUintMuxerConfig* con) = 0;
    virtual AM_ERR UpdateConfig(CUintMuxerConfig* con) = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR FeedData(CGBuffer* buffer) = 0;
    virtual AM_ERR FinishMuxer() = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam& par) = 0;
    virtual AM_ERR Dump() = 0;
    virtual AM_ERR postMDMsg(AM_INT msg) {return ME_NO_IMPL;};
    virtual AM_ERR SetSavingTimeDuration( AM_UINT duration, AM_UINT maxfilecount) = 0;
    virtual AM_ERR EnableLoader(AM_BOOL flag) = 0;
    virtual AM_ERR ConfigLoader(char* path, char* m3u8name, char* host, int count) = 0;
};

class IGInjector: public IInterface
{
public:
    virtual AM_ERR ConfigMe(CUintMuxerConfig* con) = 0;
    virtual AM_ERR UpdateConfig(CUintMuxerConfig* con) = 0;
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend) = 0;
    virtual AM_ERR FeedData(CGBuffer* buffer,int64_t pts,int64_t dts) = 0;
    virtual AM_ERR FinishInjector() = 0;
    virtual AM_ERR QueryInfo(AM_INT type, CParam& par) = 0;
    virtual AM_ERR Dump() = 0;
};

//----------------------------------------------
//......
//----------------------------------------------
class IPipeLineManager: public IInterface
{
public:
    enum PIPE_TYPE{
        PIPELINE_PLAYBACK,
        PIPELINE_RECODER,
        PIPELINE_STREAMING,
    };
    enum NODE_TYPE{
        DEMUXER_FFMPEG,
        DECODER_FFMPEG,
        DECODER_DSP,
        RENDER_SYNC,
        MUXER_GENERAL,
    };
    enum PIPE_STATE{
        PIPE_IDLE,
        PIPE_PREPARED,
        PIPE_PAUSED,
        PIPE_RUNNING,
        PIPE_ERROR,
    };
    virtual AM_INT CreatePipeLine(const char* uri, PIPE_TYPE type) = 0;
    virtual AM_ERR AddPipeNode(AM_INT index, void* module, NODE_TYPE type) = 0;
    virtual AM_ERR AddPipeNode(AM_INT index, AM_INT indentity, NODE_TYPE type) = 0;
    virtual AM_ERR UpdatePipeLine(AM_INT index) = 0;

    virtual AM_ERR PreparePipeLine(AM_INT index) = 0;
    virtual AM_ERR DisablePipeLine() = 0;
    virtual AM_ERR EnablePipeLine() = 0;

    virtual AM_ERR PausePipeLine(AM_INT index) = 0;
    virtual AM_ERR ResumePipeLine(AM_INT index) = 0;
    virtual AM_ERR FlushPipeLine(AM_INT index) = 0;
    virtual AM_ERR Dump(AM_INT flag) = 0;
};

#endif
