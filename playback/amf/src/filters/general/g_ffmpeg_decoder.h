/*
 * gaudio_decoder_ffmpeg.h
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
#ifndef __CG_DECODER_FFMPEG_H__
#define __CG_DECODER_FFMPEG_H__

//todo
#define MAX_AUDIO_SAMPLE_SIZE_16 (AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2/2)
#define MAX_AUDIO_SAMPLE_SIZE_8 (AVCODEC_MAX_AUDIO_FRAME_SIZE*3/2)
#define GDECODER_FFMPEG_AUDIO_QNUM 256

//Audio standard
#define G_A_CHANNEL 2
#define G_A_SAMPLERATE 48000
#define G_A_SAMPLEFMT AV_SAMPLE_FMT_S16

class CGDecoderFFMpeg: public IGDecoder, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    //using IMsgSink::MSG;
    enum{
        STATE_DECODING = LAST_COMMON_STATE,
    };

public:
    static AM_INT ParseBuffer(const CGBuffer* gBuffer);
    static AM_ERR ClearParse();
    static IGDecoder* Create(IFilter* pFilter,CGConfig* pconfig);

public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IGDecoder)
            //return (IGDecoder*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();
    // IGDecoder
    virtual AM_ERR IsReady(CGBuffer * pBuffer);
    virtual AM_ERR Decode(CGBuffer * pBuffer)
    {
        //fast process this action
        mState = STATE_DECODING;
        PostCmd(CMD_DECODE);
        return ME_OK;
    }
    virtual AM_ERR GetDecodedGBuffer(CGBuffer& oBuffer);
    virtual AM_ERR FillEOS();
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param){ return ME_OK;}
    virtual AM_ERR AdaptStream(CGBuffer* pBuffer){ return ME_OK;}
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer);
    virtual void Dump();
private:
    CGDecoderFFMpeg(IFilter* pFilter, CGConfig* pConfig);
    AM_ERR Construct();
    AM_ERR ConstructFFMpeg();
    void ClearQueue(CQueue* queue);
    virtual ~CGDecoderFFMpeg();

private:
    AM_ERR ProcessCmd(CMD & cmd);
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoDecode();
    AM_ERR DoIsReady();
    virtual void OnRun();
    virtual AM_ERR ReSet();
    AM_ERR FillHandleBuffer();

private:
    AM_ERR DecodeVideo();
    AM_ERR DecodeAudio();
    AM_ERR UpdateAudioDuration(AM_INT size);
    AM_ERR FillAudioSampleQueue(AM_S16* sample, AM_INT size);
    AM_INT GetAudioSampleNum(AM_INT size);

    void ProcessEOS();
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR GenerateGBuffer();

private:
    static AM_INT mConfigIndex;
    static AM_INT mStreamIndex;
    static STREAM_TYPE mType;
    static CQueue* mpDataQ;

    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDecoderConfig* mpConfig;
    AM_INT mConIndex;
    STREAM_TYPE mStreamType;

    AVFormatContext* mpAVFormat;
    AVStream* mpStream;
    AVCodecContext* mpCodec;
    AVCodec *mpDecoder;
    //AVPacket* mpPacket;

    //IMP flag(just add for audio decode 4/23)
    AM_INT mSentBufferCnt;
    AM_INT mBufferCnt;
    AM_UINT mConsumeCnt;
    //AM_INT mIavFd;
    //IUDECHandler* mpDsp;
    //AM_INT mDspIndex;
    AM_BOOL mbIsEOS;//if eos than must makesure sent out buffer clearly

    //CGBuffer* mpBuffer;
    CGBuffer mBuffer;
    CGBuffer mOBuffer; //Output Buffer to g_decoder_filter

    CQueue* mpTempMianQ;
    CQueue* mpAudioQ;
    CQueue* mpVideoQ;
    CQueue* mpInputQ;
    void* mpQOwner;

private:
    //av info
    AM_INT mSampleRate;

    AM_INT mTotalSampleA;
    AM_INT mDurationA;
    AM_INT mCurPtsA;
private:
    //audio resample
    ReSampleContext* mpAudioConvert;
    AM_S16 mAudioSample[MAX_AUDIO_SAMPLE_SIZE_16];//temp for audio decoded. used when resample

//AAC dec
#ifdef USE_LIBAACDEC
private:
    AM_ERR ConstructAACdec();
    AM_ERR InitHWAttr();
    AM_ERR SetupHWDecoder();
    AM_ERR DecodeAudioAACdec();
    AM_ERR WriteBitBuf(AVPacket* pPacket, int offset);
    AM_ERR AddAdtsHeader(AVPacket* pPacket);
private:
    AM_INT mFrameNum;

    static AM_U32 mpDecMem[106000];
    static AM_U8 mpDecBackUp[252];
    static AM_U8 mpInputBuf[16384];
    static AM_U32 mpOutBuf[8192];
#endif
};

#endif
