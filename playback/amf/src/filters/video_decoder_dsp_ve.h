
/*
 * video_decoder_dsp_ve.h
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
 *    deprecated, used for transcoding, *** to be removed ***
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_DECODER_DSP_VE_H__
#define __VIDEO_DECODER_DSP_VE_H__


class CVideoDecoderDspVE;
#define GOP_HEADER_SIZE 22

//-----------------------------------------------------------------------
//
// CVideoDecoderDspVE
//
//-----------------------------------------------------------------------
class CVideoDecoderDspVE: public CActiveObject, public IDecoder
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    //using IMsgSink::MSG;
    enum {
        CMD_ISREADY = CMD_LAST,
        CMD_DECODE,
    };
public:
    //the para form general decoder.
    static IDecoder* Create(IFilter* pFilter, CMediaFormat* pFormat);

private:
    CVideoDecoderDspVE(IFilter* pFilter, CMediaFormat* pFormat):
        inherited("CVideoDecoderDspVE"),
        mpFilter(pFilter),
        mpMediaFormat(pFormat),
        mpShare(NULL),
        mpStream(NULL),
        mpCodec(NULL),
        mpPacket(NULL),
        mH264DataFmt(H264_FMT_INVALID),
        mpBuffer(NULL),
        mpDsp(NULL),
        mDspIndex(0),
        mbRun(true),
        mbAlloced(false),
        mbDecoded(true),
        mIavFd(-1),
        mbConfigData(false),
        mSeqConfigDataLen(0),
        mSeqConfigData(NULL),
        mSeqConfigDataSize(0),
        mPicConfigDataLen(0),
        mDataWrapperType(eAddVideoDataType_none),
        mbAddDataWrapper(true)
        //mbGetOutPic(0)
    { }
    AM_ERR Construct();
    AM_ERR ConstructDsp();
    virtual ~CVideoDecoderDspVE();

public:
    virtual void Delete();
    //IDecoder todo
    virtual AM_ERR ConstructContext(DecoderConfig* configData) { return ME_OK;}
    AM_UINT GetDecoderStatus() {return 1;}
    void SetOutputPin(void* pOutPin) {}
    virtual void* GetInterface(AM_REFIID refiid);
    //
    //virtual void Start() {}
    //pause and resume may be block when allocbsb
    virtual void Pause() { PostCmd(CMD_PAUSE);}
    virtual void Resume() { PostCmd(CMD_RESUME);}
    virtual void Stop();
    virtual void Flush() { SendCmd(CMD_FLUSH);}
    //decode must be after allocbsb, so will no block
    virtual AM_ERR IsReady(CBuffer * pBuffer);
    virtual AM_ERR Decode(CBuffer * pBuffer) { SendCmd(CMD_DECODE); return ME_OK;}
    virtual AM_ERR ReSet();
    //virtual void OnCmd(); todo
    virtual void PrintState();
    AM_ERR FillEOS();
    virtual AM_INT GetDecoderInstance() {return mDspIndex;}
    //virtual AM_ERR GetDecodedFrame(CBuffer* pVideoBuffer);

private:
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoDecode();
    AM_ERR DoIsReady();
    AM_ERR ProcessCmd(CMD & cmd);
    virtual void OnRun();

private:
    void PrintBitstremBuffer(AM_U8* p, AM_UINT size);
    AM_U8* CopyToBSB(AM_U8* ptr, AM_U8* buffer, AM_UINT size);
    AM_ERR AllocBSB(AM_UINT size);

    //Generate Data For DSP
    uint8_t* Find_StartCode(uint8_t* extradata, AM_INT size);
    void GenerateConfigData();
    void UpdatePicConfigData(AVPacket* pPacket);
    bool AdjustSeqConfigDataSize(AM_INT size);
    void FeedConfigData(AVPacket* pPacket);
    void FeedConfigDataWithUDECWrapper(AVPacket *mpPacket);
    void FeedConfigDataWithH264GOPHeader(AVPacket *mpPacket);
    void FeedData(AVPacket* pPacket);

    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR DecodeBuffer(AM_U8*pStart, AM_U8*pEnd);
    AM_ERR GenerateCBuffer();
    void FillGOPHeader();
    void UpdateGOPHeader(AM_PTS pts);

private:
    IFilter* mpFilter;
    CMediaFormat* mpMediaFormat;
    SConsistentConfig* mpShare;
    //CDecoderConfig* mpConfig;

    AVStream* mpStream;
    AVCodecContext* mpCodec;
    AVPacket* mpPacket;
    H264_DATA_FORMAT mH264DataFmt;
    CBuffer* mpBuffer;
    CBuffer mOBuffer; //Output Buffer to g_decoder_filter
    IUDECHandler* mpDsp;
    AM_INT mDspIndex;

    bool mbRun;
    bool mbAlloced; //is alloced
    bool mbDecoded; //only one can be decoded at one point.
    AM_INT mIavFd;
    bool mbConfigData;

    //Very begining SEQ and PES
    AM_U8 mUSEQHeader[DUDEC_SEQ_HEADER_LENGTH];
    AM_U8 mUPESHeader[DUDEC_PES_HEADER_LENGTH];
    //config data for each format, need re send seq data after seek
    AM_UINT mSeqConfigDataLen;
    AM_U8* mSeqConfigData;
    AM_INT mSeqConfigDataSize;
    //picture config data
    AM_UINT mPicConfigDataLen;
    AM_U8 mPicConfigData[DUDEC_MAX_PIC_CONFIGDATA_LEN];

    AM_INT mDecType;
    AM_U8 *mpStartAddr;	// BSB start address
    AM_U8 *mpEndAddr;	// BSB end address
    AM_U8 *mpCurrAddr;	// current pointer to BSB
    AM_U32 mSpace;
    AM_UINT mDataWrapperType;
    bool mbAddDataWrapper;
    AM_INT mH264AVCCNaluLen;

    AM_U8 mGOPHeader[GOP_HEADER_SIZE];
    //AM_UINT mbGetOutPic;
};

#endif

