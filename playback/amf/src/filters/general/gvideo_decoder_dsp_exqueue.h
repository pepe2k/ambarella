
/*
 * gvideo_decoder_dsp_exqueue.h
 *
 * History:
 *    2012/6/1 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __GVIDEO_DECODER_DSP_EXQUEUE_H__
#define __GVIDEO_DECODER_DSP_EXQUEUE_H__

#define GOP_HEADER_SIZE 22
#define DUDEC_SEQ_HEADER_LENGTH 20
#define DUDEC_SEQ_HEADER_EX_LENGTH 24//2011.09.23 has resolution info case by roy
#define DUDEC_PES_HEADER_LENGTH 24

//-----------------------------------------------------------------------
//
// CVideoDecoderDspQueue
//
//-----------------------------------------------------------------------
class CVideoDecoderDspQueue: public IGDecoder, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    //using IMsgSink::MSG;

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
        AM_ASSERT(0);
        return ME_OK;
    }
    virtual AM_ERR GetDecodedGBuffer(CGBuffer& oBuffer) {return ME_OK;}
    virtual AM_ERR FillEOS();
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param);
    virtual AM_ERR AdaptStream(CGBuffer* pBuffer);
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer);
    virtual void Dump();

private:
    CVideoDecoderDspQueue(IFilter* pFilter, CGConfig* pConfig);
    AM_ERR Construct();
    AM_ERR ConstructDsp();
    virtual ~CVideoDecoderDspQueue();

private:
    AM_ERR ProcessCmd(CMD & cmd);
    AM_ERR DoAdaptStream(CMD& cmd);
    AM_ERR DoClear(CMD& cmd);
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
    bool CheckH264SPSExist(AM_U8 *pData, AM_INT iDataLen, AM_INT *pSPSPos, AM_INT *pSPSLen);

    void FeedData(AVPacket* pPacket);
    void FillGOPHeader();
    void UpdateGOPHeader(AM_PTS pts);

    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR DecodeBuffer(AM_U8*pStart, AM_U8*pEnd);
    AM_ERR GenerateGBuffer();

    inline void ReleaseBuffer()
    {
        if(mpBuffer == &mDecBuffer){
            AVPacket* pkt = (AVPacket* )(mDecBuffer.GetExtraPtr());
            if(pkt != NULL){
                av_free_packet(pkt);
                delete pkt;
            }
            mDecBuffer.SetExtraPtr((AM_INTPTR)NULL);
            mDecBuffer.Clear();
        }else if(mpBuffer == &mBuffer){
            ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
            mBuffer.Clear();
        }
        mpPacket = NULL;
        mpBuffer = NULL;
    }

private:
    static AM_INT mConfigIndex;
    static AM_INT mStreamIndex;
    static CQueue* mpDataQ;

    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDecoderConfig* mpConfig;
    AM_INT mConIndex;
    CQueue* mpBufferQ;
    void* mpQOwner;

    AM_UINT msOldState;

    AVFormatContext* mpAVFormat;
    AVStream* mpStream;
    AVCodecContext* mpCodec;
    AVPacket* mpPacket;

    AM_INT mIavFd;
    IUDECHandler* mpDsp;
    AM_INT mDspIndex;

    CGBuffer mBuffer;
    CGBuffer* mpBuffer;
    CGBuffer mDecBuffer; //copy input buffer to DoDecode, need when BSB is almost full
    CGBuffer mOBuffer; //Output Buffer to g_decoder_filter

    //CMediaFormat* mpMediaFormat;

    //TODO!
    //SharedResource* mpShare;
    bool mbAlloced; //is alloced
    bool mbDecoded; //only one can be decoded at one point.
    bool mbConfigData;

    //Very begining SEQ and PES
    H264_DATA_FORMAT mH264DataFmt;
    AM_U8 mUSEQHeader[DUDEC_SEQ_HEADER_LENGTH];
    AM_U8 mUPESHeader[DUDEC_PES_HEADER_LENGTH];
    //config data for each format, need re send seq data after seek
    AM_INT mSeqConfigDataLen;
    AM_U8* mSeqConfigData;
    AM_INT mSeqConfigDataSize;
    //picture config data
    AM_UINT mPicConfigDataLen;
    AM_U8 mPicConfigData[DUDEC_MAX_PIC_CONFIGDATA_LEN];

    AM_INT mDecType;
    AM_U32 mSpace;
    AM_UINT mDataWrapperType;
    bool mbAddDataWrapper;
    AM_INT mH264AVCCNaluLen;
    AM_U8 *mpStartAddr;	// BSB start address
    AM_U8 *mpEndAddr;	// BSB end address
    AM_U8 *mpCurrAddr;	// current pointer to BSB

    AM_U8 mGOPHeader[GOP_HEADER_SIZE];
    //AM_UINT mbGetOutPic;

    //info
    //AM_S64 mDspLastPts;
    AM_BOOL mVoutShow;
    AM_U8* mpStartOnHold;
    //AM_U8* mpEndOnHold;
    AM_U64 mFeedPts;
    AM_U64 mFeedSize;
    AM_INT mFeedTime;

    enum BsbState{
        BSB_STATE_GOON_FEEDING,
        BSB_STATE_WAIT_MORE_BUFFER,
    };
    BsbState mBSBState;

    //feeding rule
    IParameters::DecoderFeedingRule mCurrentFeedingRule;
    IParameters::DecoderFeedingRule mTobeFeedingRule;

    AM_U16 mLastPBSpeed;

    //for DSP require send speed cmd in UDEC_READY state(after first DECODE cmd)
    AM_U8 mbNeedSendBeginningSpeedParams;
    AM_U8 mbFillPrivateGOPHeader;
    AM_U8 mbBWplayback;
    AM_U8 mReserved6;

    AM_UINT mBeginningSpeedParam;
    AM_UINT mBeginningSpeedParam1;

    AM_U8 mPrivateGOPHeader[DPRIVATE_GOP_NAL_HEADER_LENGTH + 2];
};

#endif


