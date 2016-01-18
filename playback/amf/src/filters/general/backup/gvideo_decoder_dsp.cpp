/*
 * video_decoder_dsp.cpp
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "video_decoder_dsp"
//#define AMDROID_DEBUG

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "general_dsp_related.h"
#include "gvideo_decoder_dsp.h"

//for dump
#define MB	(1024 * 1024)
#define BE_16(x) (((unsigned char *)(x))[0] <<  8 | \
		  ((unsigned char *)(x))[1])
#define BE_32(x) ((((unsigned char *)(x))[0] << 24) | \
                  (((unsigned char *)(x))[1] << 16) | \
                  (((unsigned char *)(x))[2] << 8)  | \
                   ((unsigned char *)(x))[3])

static void PrintBitstremBuffer(AM_U8* p, AM_UINT size)
{
/*
#ifdef AM_DEBUG
    if (!(mLogOption & LogBinaryData))
        return;

    while (size > 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2], p[3]);
        p += 4;
        size -= 4;
    }

    if (size == 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2]);
    } else if (size == 2) {
        AMLOG_BINARY("  %2.2x %2.2x.\n", p[0], p[1]);
    } else if (size == 1) {
        AMLOG_BINARY("  %2.2x.\n", p[0]);
    }
#endif
*/
}

//-----------------------------------------------------------------------
//
// CVideoDecoderDsp
//
//-----------------------------------------------------------------------
AM_INT CVideoDecoderDsp::mConfigIndex = 0;
AM_INT CVideoDecoderDsp::mStreamIndex = 0;

GDecoderParser gVideoDspDecoder = {
    "Decoder-Dsp-VG",
    CVideoDecoderDsp::Create,
    CVideoDecoderDsp::ParseBuffer,
    CVideoDecoderDsp::ClearParse,
};

//
AM_INT CVideoDecoderDsp::ParseBuffer(const CGBuffer* gBuffer)
{
    AM_INT score = 0;
    if(gBuffer->GetExtraPtr() != 0)
        return -50;

    STREAM_TYPE type = gBuffer->GetStreamType();
    if(type != STREAM_VIDEO)
        return -10;

    OWNER_TYPE otype = gBuffer->GetOwnerType();
    AM_ASSERT(otype == DEMUXER_FFMPEG);

    if(*(gBuffer->codecType) != GUID_AmbaVideoDecoder)
        return -20;

    mConfigIndex  = gBuffer->GetIdentity();
    mStreamIndex = gBuffer->ffmpegStream;
    score = 90;
    AM_INFO("CVideoDecoderDsp, Score:%d\n", score);
    return score;
}

AM_ERR CVideoDecoderDsp::ClearParse()
{
    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CVideoDecoderDsp
IGDecoder* CVideoDecoderDsp::Create(IFilter* pFilter, CGConfig* pconfig)
{
    CVideoDecoderDsp *result = new CVideoDecoderDsp(pFilter, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CVideoDecoderDsp::CVideoDecoderDsp(IFilter* pFilter, CGConfig* pConfig):
        inherited("CVideoDecoderDsp"),
        mpFilter(pFilter),
        mpGConfig(pConfig),
        mpStream(NULL),
        mpCodec(NULL),
        mpPacket(NULL),
        mIavFd(-1),
        mpDsp(NULL),
        mDspIndex(-1),
        mpBuffer(NULL),
        mbAlloced(false),
        mbDecoded(true),
        mbConfigData(false),
        mH264DataFmt(H264_FMT_INVALID),
        mSeqConfigDataLen(0),
        mSeqConfigData(NULL),
        mSeqConfigDataSize(0),
        mPicConfigDataLen(0),
        mDataWrapperType(eAddVideoDataType_none),
        mbAddDataWrapper(true),
        mpStartAddr(NULL),
        mpEndAddr(NULL),
        mpCurrAddr(NULL)
{
    mState = -1; //no used
    mbRun = true;
    mpConfig = &(pConfig->decoderConfig[mConfigIndex]);
    mpAVFormat = (AVFormatContext* )mpConfig->decoderEnv;
    mConIndex = mConfigIndex;
    //mStreamIndex = mpConfig->opaqueEnv;
}

AM_ERR CVideoDecoderDsp::Construct()
{
    AM_INFO("CVideoDecoderDsp::Construct\n");
    AM_ERR err;
    err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("CVideoDecoderDsp ,inherited::Construct fail err = %d .\n", err);
        return err;
    }
    //todo change av_malloc
    if ((mSeqConfigData = (AM_U8 *)av_malloc(DUDEC_MAX_SEQ_CONFIGDATA_LEN)) == NULL)
        return ME_NO_MEMORY;
    mSeqConfigDataSize = DUDEC_MAX_SEQ_CONFIGDATA_LEN;

    err = ConstructDsp();
    if (err != ME_OK)
    {
        AM_ERROR("CVideoDecoderDsp ,ConstructDsp fail err = %d .\n", err);
        return err;
    }
    mpWorkQ->SetThreadPrio(1, 1);

    DSetModuleLogConfig(LogModuleVideoDecoderDSP);
    //POST A HANDLD FOR ME
    return ME_OK;
}

//to ienge
AM_ERR CVideoDecoderDsp::ConstructDsp()
{
    AM_ERR err;
    AM_INT vFormat = UDEC_VFormat_H264;
    mpStream = mpAVFormat->streams[mStreamIndex];
    mpCodec = mpStream->codec;
    //mpShare = ((CInterActiveFilter*)mpFilter)->mpSharedRes;
    CDspGConfig* pDspConfig = &(mpGConfig->dspConfig);
    mpDsp = pDspConfig->udecHandler;
    //mIavFd = pDspConfig->iavHandle;
        AM_INFO("v1\n");

    switch (mpCodec->codec_id)
    {
        case CODEC_ID_VC1:
            AM_PRINTF("-----UDEC_VC1\n----");
            vFormat = UDEC_VFormat_VC1;
            mDecType = UDEC_VC1;
            break;
        case CODEC_ID_WMV3:
            AM_PRINTF("-----UDEC_VC1(WMV3)\n----");
            vFormat = UDEC_VFormat_WMV3;
            mDecType = UDEC_VC1;
            break;

        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            AM_PRINTF("-----UDEC_MP12\n----");
            mbAddDataWrapper = false;
            mDecType = UDEC_MP12;
            break;

        case CODEC_ID_MPEG4:
            AM_PRINTF("-----UDEC_MP4H\n----");
            vFormat = UDEC_VFormat_MPEG4;
            mDecType = UDEC_MP4H;
            break;
        case CODEC_ID_H264:
            AM_PRINTF("-----UDEC_H264\n----");
            vFormat = UDEC_VFormat_H264;
            mDecType = UDEC_H264;
            break;
        default:
            AM_ASSERT(0);
            AMLOG_ERROR("not supported codec_id %d.\n", mpCodec->codec_id);
            return ME_ERROR;
    }
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    if (pDspConfig->addVideoDataType != eAddVideoDataType_iOneUDEC && pDspConfig->addVideoDataType != eAddVideoDataType_none) {
        AM_ASSERT(0);
        mDataWrapperType = eAddVideoDataType_iOneUDEC;
    } else {
        mDataWrapperType = pDspConfig->addVideoDataType;
    }
#elif TARGET_USE_AMBARELLA_A5S_DSP
    AM_ASSERT(mbAddDataWrapper);
    if (pDspConfig->addVideoDataType != eAddVideoDataType_a5sH264GOPHeader) {
        AM_ASSERT(0);
        mDataWrapperType = eAddVideoDataType_a5sH264GOPHeader;
    }
#endif
        mDataWrapperType = eAddVideoDataType_iOneUDEC;
        AM_INFO("v2\n");

    //1 todo
    SharedResource* tempS = (SharedResource* )(mpGConfig->oldCompatibility);
    AM_INFO("show, mDec:%d, width:%d, height:%d\n", mDecType, mpCodec->width, mpCodec->height);
    err = mpDsp->InitUDECInstance(mDspIndex, &tempS->dspConfig, NULL, mDecType,
                                   mpCodec->width, mpCodec->height, 1);
    if(err != ME_OK)
    {
        AM_ASSERT(0);
        return err;
    }
    //save dspIndex on gconfig
    mpGConfig->SetDefaultWinRenMap(mConIndex, mDspIndex);
    //
    mpStartAddr = tempS->dspConfig.udecInstanceConfig[mDspIndex].pbits_fifo_start;
    mSpace = tempS->dspConfig.udecInstanceConfig[mDspIndex].bits_fifo_size;
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
            AM_INFO("v3==>%d\n", mDataWrapperType);

    //todo ,kill the mvformat, if(mbadd...)
    if(mbAddDataWrapper)
    {
        if (mDataWrapperType == eAddVideoDataType_iOneUDEC) {
            FillUSEQHeader(mUSEQHeader, vFormat, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, tempS->is_mp4s_flag, tempS->vid_container_width, tempS->vid_container_height);
            InitUPESHeader(mUPESHeader, vFormat);
        } else if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
            FillGOPHeader();
        } else if (mDataWrapperType != eAddVideoDataType_none) {
            AM_ASSERT(0);
            AMLOG_ERROR("!!must not comes here, mDataWrapperType %d.\n", mDataWrapperType);
        }
        GenerateConfigData();
    }
            AM_INFO("v4\n");

    //mIavFd = mpShare->mIavFd;
    //dspmode 3 is transcode
    if(tempS->mDSPmode != 3)
        mpDsp->GetUDECState(mDspIndex, tempS->udec_state, tempS->vout_state, tempS->error_code);

    return ME_OK;
}
//==============================
//
//==============================
AM_ERR CVideoDecoderDsp::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    CQueue* q = MsgQ();
    if(isSend == AM_TRUE)
    {
        err = q->SendMsg(&cmd, sizeof(cmd));
    }else{
        err = q->PostMsg(&cmd, sizeof(cmd));
    }
    return err;
}

//clear last buffer on isready
AM_ERR CVideoDecoderDsp::DoClear(CMD& cmd)
{
    if(mpBuffer != NULL){
        AM_INFO("doclear: %d, %d %d %p, %p\n", mbAlloced, mbDecoded, cmd.res32_1, cmd.pExtra, mpBuffer);
        mpBuffer->Dump("DoClear");
    }
    if(mbAlloced == false){
        //still on alloc process. impossilbe, if still doisready than this cmd can be handled after isread done.
        AM_ASSERT(mpBuffer == NULL);
    }else{
        //how to recall this space? no write is ok
    }
    if(mbDecoded == true){
        //decode not start or done
        if(mpBuffer == NULL){
            //done
            //Note GDE set his mpbuffer = NULL after Decode. OK
        }
        if(mpBuffer != NULL){
            //same as GDE's last Buffer, let GDE do retriever
            if(mpBuffer == (CGBuffer* )cmd.pExtra){
                mpBuffer = NULL;//This is imp, or will affect next
                mbAlloced = false;
                mbDecoded = true;
                return ME_OK;
            }

            if(cmd.res32_1 & RESUME_BUFFER_RETRIEVE)
                mpBuffer->Retrieve();
            else
                mpBuffer->Release();
            mpBuffer = NULL;
        }
    }else{
        //is doing decode, impossilbe.
        AM_ASSERT(0);
    }
    mbAlloced = false;
    mbDecoded = true;
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::DoPause()
{
#if 0
    AMLOG_PRINTF("CVideoDecoderDsp::DoPause.\n");
    //put it here in only a5s cases, ione's pause is called by engine, can uniform it in future
    if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
        mpDsp->PauseUDEC(mDspIndex);
    }
#endif
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::DoResume()
{
#if 0
    AMLOG_PRINTF("CVideoDecoderDsp::DoResume.\n");
    //put it here in only a5s cases, ione's resume is called by engine, can uniform it in future
    if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
        mpDsp->ResumeUDEC(mDspIndex);
    }
#endif
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::DoFlush()
{
    //should no need to flush the cmdQ, flush should be last cmd
    //CQueue* cmdQ = mpWorkQ->MsgQ();
    //AM_ASSERT(cmdQ->GetDataCnt() == 0);
    //handle flush after allocbsb
    AM_INFO("CVideoDecoderDsp DoFlush\n");
/*
    mbConfigData = false;
    //mbFillEOS = false;

    SharedResource* tempS = (SharedResource* )(mpGConfig->oldCompatibility);

    mpStartAddr = tempS->dspConfig.udecInstanceConfig[mDspIndex].pbits_fifo_start;
    mSpace = tempS->dspConfig.udecInstanceConfig[mDspIndex].bits_fifo_size;
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
*/
    mbAlloced = false;
    mbDecoded = true;
    if (mpBuffer) {
        AM_ASSERT(0);
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    return ME_OK;
}

//Stop cmd just let everything leave the onrun()
AM_ERR CVideoDecoderDsp::DoStop()
{
    AM_ASSERT(mBuffer.GetBufferType() == NOINITED_BUFFER);
    AM_ERR err = ME_OK;
    err = mpDsp->StopUDEC(mDspIndex);
    AM_ASSERT(err == ME_OK);
    mpCurrAddr = mpStartAddr;

    return err;
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::DoIsReady()
{
    //AM_INFO("CVideoDecoderDsp::DoIsReady.\n");
    AM_UINT size;
    AM_ERR err;

    if(mpCurrAddr==mpEndAddr)//for bits fifo wrap case
        mpCurrAddr=mpStartAddr;

    //todo
    if(mpBuffer->GetBufferType() == EOS_BUFFER)
    {
        size = 8;
    }else{
        mpPacket = (AVPacket*)(mpBuffer->GetExtraPtr());
        if(mpPacket == NULL)
        {
            AM_ERROR("mpPacket == NULL.\n");
            return ME_ERROR;
        }
        size = mpPacket->size + mSeqConfigDataLen + DUDEC_PES_HEADER_LENGTH;
    }
    err = AllocBSB(size);
    if(err == ME_UDEC_ERROR)
    {
        AMLOG_ERROR("err == ME_UDEC_ERROR.\n");
        //PostFilterMsg(GFilter::MSG_UDEC_ERROR);
        //handle this err on OnRun();
        return err;
    }
    if(err != ME_OK)
    {
        AMLOG_ERROR("err on AllocBSB()!= %d.\n", err);
        return err;
    }
    mbAlloced = true;
    PostFilterMsg(GFilter::MSG_READY);
    return ME_OK;
}

//===============================
//init: mbDecoded = true, mbAlloced = false;
//===============================
AM_ERR CVideoDecoderDsp::IsReady(CGBuffer* pBuffer)
{
    //AM_INFO("%d, cnt:%d CVideoDecoderDsp IsReady %d, %d\n", pBuffer->GetIdentity(), pBuffer->GetCount(), mbDecoded, mbAlloced);
    if(mbDecoded == true && mbAlloced == true)
        return ME_OK;

    if(mbDecoded == true && mbAlloced == false)
    {
        //AM_ASSERT(mpBuffer == NULL);
        if(mpBuffer != NULL){
            //IS DOING DoIsReady
            return ME_ERROR;
        }
        mpBuffer = pBuffer;
        PostCmd(CMD_ISREADY);
    }
    return ME_ERROR;
}

AM_ERR CVideoDecoderDsp::ReSet()
{
    /*
    AM_ERR err;
    err = mpDsp->ReleaseUDECInstance(mDspIndex);
    if(err != ME_OK)
    {
        AM_ERROR("ReleaseUDECInstance Failed in ReSet()!!!\n");
        return err;
    }
    if (mpShare->udecHandler == NULL)
    {
        err = AM_CreateDSPHandler(mpShare);
        if(err != ME_OK)
            return err;
    }
    //...
    mpDsp = mpShare->udecHandler;
    err = mpDsp->InitUDECInstance(mDspIndex, &mpShare->dspConfig, NULL, mDecType,
                                   mpCodec->width, mpCodec->height, 0);
    if(err != ME_OK)
    {
        return err;
    }
    //
    mpStartAddr = mpShare->dspConfig.udecInstanceConfig[mDspIndex].pbits_fifo_start;
    mSpace = mpShare->dspConfig.udecInstanceConfig[mDspIndex].bits_fifo_size;
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
    mIavFd = mpShare->mIavFd;
    mpDsp->GetUDECState(mDspIndex, mpShare->udec_state, mpShare->vout_state, mpShare->error_code);
    */

    return ME_OK;
    //DoStop();
    //SendCmd(CMD_RUN);
}

//todo, think 1: upfilter's pause should be hold or not?
//yes,but we can ack asap.
AM_ERR CVideoDecoderDsp::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("CVideoDecoderDsp::ProcessCmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    AM_UINT par;
    switch (cmd.code)
    {
    case CMD_STOP:
        AM_INFO("CVideoDecoderDsp Process CMD_STOP.\n");
        DoStop();
        mbRun = false;
        CmdAck(ME_OK); //TODO BELOW
        //no ack, that is using the postcmd, because me may be block on DoIsReady();
        break;

     //todo check this
    case CMD_PAUSE:
        DoPause();
        //todo seem no be block here
        //CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        DoResume();
        //CmdAck(ME_OK);
        break;

    //TODO, seek
    case CMD_FLUSH:
        DoFlush();
        CmdAck(ME_OK);
        break;

    //ack for not agai
    case CMD_DECODE:
        //
        //mbDecoded = false;
        err = DoDecode();
        //todo copy this Cbuffer to send out
        //AM_INFO("DoDecode :%d...\n", err);
        mbDecoded = true;
        //mbAlloced = false;
        if(err == ME_OK){
            SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);//wakeup isready and send cbuffer. make sure sended and then ack
        }else{
            PostFilterMsg(GFilter::MSG_DECODED, 0);
        }
        AMLOG_DEBUG("CMD_DECODE Done...\n");
        //CmdAck(ME_OK);
        break;

    case CMD_ISREADY:
        err = DoIsReady();
        break;

    case CMD_CLEAR:
        par = cmd.res32_1;
        err = DoClear(cmd);
        CmdAck(err);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return err;
}

void CVideoDecoderDsp::OnRun()
{
    AM_ERR err;
    CMD cmd;
    FillHandleBuffer();
    mbRun = true;
    CmdAck(ME_OK);
    while(mbRun)
    {
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        err = ProcessCmd(cmd);
        if(err != ME_OK)
        {
            if(cmd.code != CMD_STOP)//for test cmd
            {
                //PostFilterMsg(GFilter::MSG_DECODER_ERROR);
            }
            if(cmd.code == CMD_ISREADY || cmd.code == CMD_DECODE)
            {
                AM_ERROR("here");
                if(err == ME_UDEC_ERROR){
                    PostFilterMsg(GFilter::MSG_DECODER_ERROR);
                }
            }
        }
    }
}

inline AM_ERR CVideoDecoderDsp::FillHandleBuffer()
{
    AM_INFO("FillHandleBuffer on CVideoDsp\n");
    mOBuffer.Clear();
    mOBuffer.SetOwnerType(DECODER_DSP);
    mOBuffer.SetBufferType(HANDLE_BUFFER);
    mOBuffer.SetStreamType(STREAM_VIDEO);
    mOBuffer.SetIdentity(mConIndex);
    SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);//wakeup isready and send cbuffer. make sure sended and then ack
    return ME_OK;
}
//==============================
//
//==============================

AM_U8* CVideoDecoderDsp::CopyToBSB(AM_U8* ptr, AM_U8* buffer, AM_UINT size)
{
    //AM_INFO("copy to bsb(mpStart: %p, mpEnd:%p, Size: %d) --- currPtr:%p, writePtr:%p, writesize:%d.\n", mpStartAddr, mpEndAddr, mSpace,
        //mpCurrAddr, ptr, size);
    if (ptr + size <= mpEndAddr) {
        memcpy(ptr, buffer, size);
        return ptr + size;
    } else {
        AMLOG_DEBUG("-------wrap happened, Index=[%d]--mpEndAddr %p.-----\n",mDspIndex,mpEndAddr);
        int room = mpEndAddr - ptr;
        AM_U8 *ptr2;
        memcpy(ptr, buffer, room);
        ptr2 = buffer + room;
        size -= room;
        memcpy(mpStartAddr, ptr2, size);
        return mpStartAddr + size;
    }
}

AM_ERR CVideoDecoderDsp::AllocBSB(AM_UINT size)
{
    //AM_INFO("AllocBSB , size  : %d\n", size);
    AM_ERR err = ME_OK;
    AM_UINT msize = size + 128;
    err = mpDsp->RequestInputBuffer(mDspIndex, msize, mpCurrAddr, 1); //for safe
    //err = mpDsp->AllocBSB(size);
    if(err == ME_BUSY)
    {
        //mbGetOutPic = 1;
    }
    //TODO THIS;
    if(err == ME_UDEC_ERROR)
    {
        //PostFilterMsg(GFilter::MSG_UDEC_ERROR);
        //return err;
    }
    //AM_INFO("AllocBSB , size %d, done :%d\n", size, err);
    return err;
}


AM_ERR CVideoDecoderDsp::DoDecode()
{
    static int n = 0;
    AM_U8 *pFrameStart;
    //AM_ASSERT(mpBuffer == pBuffer);
    AM_ERR err;

    //AM_INFO("size:%d(%d, %d,%d)\n", mpPacket->size, mpPacket->data[0], mpPacket->data[1], mpPacket->data[2]);
    if(mpPacket->size <= 0){
        AM_INFO("cccccccccccccccccccccc  %d\n", mConIndex);
        mpBuffer->Dump("Wrong");
        mpBuffer->Release();
        mpBuffer = NULL;
        return ME_ERROR;
    }
    if(mpCurrAddr==mpEndAddr)//for bits fifo wrap case
        mpCurrAddr=mpStartAddr;

    AM_U8 *pRecoveryAddr = mpCurrAddr;  // added for recovery when dropping packets
    pFrameStart = mpCurrAddr;
        if (!mbAddDataWrapper) {
            FeedConfigData(mpPacket);
        } else if (mDataWrapperType == eAddVideoDataType_iOneUDEC) {
            FeedConfigDataWithUDECWrapper(mpPacket);
        } else if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
            FeedConfigDataWithH264GOPHeader(mpPacket);
        }
        //if (mpCodec->codec_id == CODEC_ID_H264)
        //mH264DataFmt = H264_FMT_ANNEXB;
        if (mpCodec->codec_id == CODEC_ID_H264) {
            AM_U8 startcode[4] = {0, 0, 0, 0x01 };
            AM_U8 startcodeEx[3] = {0, 0, 0x01};
            AM_U8 *ptr = mpPacket->data;
            AM_INT curPos = 0;
            AM_INT len = 0;
            //mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
            if (mH264DataFmt == H264_FMT_AVCC) {
                //AM_ASSERT((mH264AVCCNaluLen == 2) || (mH264AVCCNaluLen == 3) || (mH264AVCCNaluLen == 4));

                while (1) {
                    //AM_INFO("1");
                    len = 0;
                    //one nal unit length.
                    for (int index = 0; index < mH264AVCCNaluLen; index++) {
                        len = len<<8 | ptr[index];
                                                                        //AM_INFO("---len %d, data %d", len, ptr[index]);

                    }
                    //AM_INFO("--loop:%d\n", ++n);

                    ptr += mH264AVCCNaluLen;
                    curPos += mH264AVCCNaluLen;

                    if (len <= 0 || (curPos + len > mpPacket->size)) {
                        AM_INFO("CVideoDecoderDsp::ProcessBuffer error: pkt_size:%d len:%d mH264AVCCNaluLen:%d curPos:%d\n",
                            mpPacket->size, len, mH264AVCCNaluLen, curPos);
                        mpCurrAddr= pRecoveryAddr;
                        //PostFilterMsg(ME_VIDEO_DATA_ERROR);
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        return ME_ERROR;
                    }

                    // check whether there's 00 00 00 01 or 00 00 01 in data
                    // if exist, skip it
                    //AM_DEBUG("~~~~~1: len:%d %x %x %x %x %x\n", len, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
                    AM_INT end = curPos + len;
                    while (curPos + 4 < end) {
                        if (memcmp(ptr, startcode, sizeof(startcode)) == 0) {
                            ptr += 4;
                            curPos += 4;
                            continue;
                        } else if(memcmp(ptr, startcodeEx, sizeof(startcodeEx)) == 0) {
                            ptr += 3;
                            curPos += 3;
                            continue;
                        } else {
                            break;
                        }
                    }

                    AM_ASSERT(curPos < end);
                    len = end - curPos;
                    mpCurrAddr = CopyToBSB(mpCurrAddr, ptr, len);
                    ptr += len;
                    curPos += len;
                    //AM_DEBUG("~~~~~2: len:%d\n", len);

                    if (curPos + mH264AVCCNaluLen >= mpPacket->size) {
                        break;
                    } else {
                        mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                    }
                }
            } else if (mH264DataFmt == H264_FMT_ANNEXB) {
                AM_INT firstStatcodeOffset = 0;
                while (curPos + 4 < mpPacket->size) {
                    if (memcmp(ptr, startcode, sizeof(startcode)) == 0) {
                        firstStatcodeOffset += 4;
                        ptr += 4;
                        curPos += 4;
                        continue;
                    } else if (memcmp(ptr, startcodeEx, sizeof(startcodeEx)) == 0) {
                        firstStatcodeOffset += 3;
                        ptr += 3;
                        curPos += 3;
                        continue;
                    } else {
                        if (firstStatcodeOffset > 0) {
                            break;
                        } else {
                            ptr++;
                            curPos++;
                        }
                    }
                }

                // patch:
                // if startcode can't be found, fill unit delimiter as placeholder,
                // which doesn't has any side effect
                if (firstStatcodeOffset > 0) {
                    mpCurrAddr = CopyToBSB(mpCurrAddr,ptr, mpPacket->size - curPos);
                } else {
                    AM_U8 unitDelimiter[] = { 0x09, 0x30 };
                    mpCurrAddr = CopyToBSB(mpCurrAddr, unitDelimiter, sizeof(startcode)/sizeof(startcode[0]));
                }
            }
        } else {
            mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
        }
    // FeedConfigData(mpPacket);
    //FeedData(mpPacket);
    //AM_INFO("AmbaVideoDec: DecodeBuffer start");//, start %p, end %p, diff %d, totdiff %d, %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr - pFrameStart, mpCurrAddr - mpStartAddr, mpEndAddr - mpCurrAddr);
    err = DecodeBuffer(pFrameStart, mpCurrAddr);
    ///AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer done.\n");
    if (err == ME_UDEC_ERROR) {
        //PostFilterMsg(GFilter::MSG_UDEC_ERROR);
        //handle on OnRun
        //return err;
    }

    if(err == ME_OK)
        GenerateGBuffer();
    //will call general demuxer's OnReleaseBuffer();
    //MUST release this buffer anyway
    //AM_INFO("Ddsp Doecoded Done err %d  %d  cnt:%d\n", err, mpBuffer->GetIdentity(), mpBuffer->GetCount());

    mpBuffer->Release();
    mpBuffer = NULL;
    return err;
}

#if 0
inline void CVideoDecoderDsp::_Dump_es_data(AM_U8* pStart, AM_U8* pEnd)
{
#ifdef AM_DEBUG

    //dump data write to file
    if (1) {
        if (!mpDumpFile) {
            mpDumpFile = fopen("dump/dump.es", "ab");
            //AMLOG_INFO("open  mpDumpFile %p.\n", mpDumpFile);
        }
        if (mpDumpFile) {
            //AM_INFO("write data.\n");
            AM_ASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(mpEndAddr - pStart), mpDumpFile);
                fwrite(mpStartAddr, 1, (size_t)(pEnd - mpStartAddr), mpDumpFile);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFile);
            }
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            //AMLOG_INFO("open  mpDumpFile fail.\n");
        }
    }

    if (0) {
        mDumpIndex++;
        if(mDumpIndex<0 || mDumpIndex>200)
            return;

        snprintf(mDumpFilename, 100 ,"/tmp/mmcblk0p1/dump/%d.dump.es", mDumpIndex);
        mpDumpFileSeparate = fopen(mDumpFilename, "wb");
        if (mpDumpFileSeparate) {
            AM_ASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(mpEndAddr - pStart), mpDumpFileSeparate);
                fwrite(mpStartAddr, 1, (size_t)(pEnd - mpStartAddr), mpDumpFileSeparate);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFileSeparate);
            }

        }
        fclose(mpDumpFileSeparate);
    }
#endif
}
#endif

AM_ERR CVideoDecoderDsp::DecodeBuffer(AM_U8*pStart, AM_U8*pEnd)
{
    AM_ERR err = ME_OK;
    AM_U8* dspRead = NULL;

    //AM_INFO("DecodeBuffer, size: %x,    %d\n", pEnd - pStart, pEnd - pStart);
    //_Dump_es_data(pStart, pEnd);
    mpDsp->GetUDECState2(mDspIndex, &dspRead);
    //AM_INFO("Debug(%d) Before: pStart:%p, CurBufferPtr:%p,      DspReadPtr:%p\n", mConIndex, pStart, mpCurrAddr, dspRead);
    if(pStart == dspRead)
    {
        //dspread reach to mystart, ok, under overflow
        AM_INFO("Dsp(%d) Data Buffer is Empty!!!, Check Me!!\n", mDspIndex);
    }



    err = mpDsp->DecodeBitStream(mDspIndex, pStart, pEnd);
    if(err != ME_OK) {
    }
    if(err == ME_UDEC_ERROR) {
        //PostFilterMsg(GFilter::MSG_UDEC_ERROR);
    }
    //AM_INFO("DecodeBuffer, Done\n");
    //mpDsp->GetUDECState2(mDspIndex, &dspRead);
    //AM_INFO("Debug(%d): CurBufferPtr:%p,      DspReadPtr:%p\n", mConIndex, mpCurrAddr, dspRead);
    return err;
}

AM_ERR CVideoDecoderDsp::GenerateGBuffer()
{
    mOBuffer.Clear();
    mOBuffer.SetOwnerType(DECODER_DSP);
    mOBuffer.SetBufferType(DATA_BUFFER);
    mOBuffer.SetStreamType(STREAM_VIDEO);
    mOBuffer.SetIdentity(mConIndex);
    mOBuffer.SetPTS(mpBuffer->GetPTS());
    //get_time_ticks_from_dsp
    //mOBuffer.general();
    //mpDsp->GetSendBuffer(&mOBuffer);
    //mpVideoOutputPin->SendBuffer(pBuffer);
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::OnReleaseBuffer(CGBuffer* buffer)
{
    if(buffer->GetBufferType() != DATA_BUFFER)
        return ME_OK;
     //to do
    //HANDLE this time tick, on ppmode 2 no need to release.
    /*
    CVideoBuffer* pVideoBuffer = (CVideoBuffer *)pBuffer;
    if ((pBuffer->GetType() == CBuffer::DATA) && mpDsp) {
        mpDsp->ReleaseFrameBuffer(mDspIndex, pVideoBuffer);
    }
    */
    return ME_OK;
}

//===========================================
//
//Those Functions Generate Data For DSP
//
//===========================================
uint8_t* CVideoDecoderDsp::Find_StartCode(uint8_t* extradata, AM_INT size)
{
    uint8_t* ptr = extradata;

    while (ptr < extradata + size - 4) {
        if (*ptr == 0x00)
        {
            if (*(ptr + 1)== 0x00)
            {
                if (*(ptr + 2)== 0x01)
                {
                    return ptr;
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

void CVideoDecoderDsp::GenerateConfigData()
{
    mSeqConfigDataLen = 0;
    mPicConfigDataLen = 0;

    switch (mpCodec->codec_id) {

        case CODEC_ID_VC1: {
                u8 startcode[4] = {0, 0, 1,0x0d};
                memcpy(mPicConfigData, startcode, sizeof(startcode));
                mPicConfigDataLen = sizeof(startcode);

                uint8_t* StartCode_ptr = Find_StartCode(mpCodec->extradata, mpCodec->extradata_size);
                if (StartCode_ptr == NULL){
                    AMLOG_DEBUG("=========No start code in extradata!!!========\n");
                    break;
                }
                mSeqConfigDataLen = mpCodec->extradata_size + mpCodec->extradata -StartCode_ptr;
                AMLOG_DEBUG("mSeqConfigDataLen=%d.\n",mSeqConfigDataLen);
                if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                    AdjustSeqConfigDataSize(mpCodec->extradata_size)) {
                    memcpy(mSeqConfigData, StartCode_ptr, mSeqConfigDataLen);
                }
           }
            break;

        case CODEC_ID_WMV3: {
                u8 padding0[] = {0x78,0x16,0x00};
                u8 padding1[] = {0xc5,0x04,0x00,0x00,0x00};
                u8 padding2[] = {0x0c,0x00,0x00,0x00};
                u8 padding3[] = {0xd5,0x0c,0x00,0x10,0x5d,0xf2,0x05,0x00,0x0c,0x00,0x00,0x00};

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding0, sizeof(padding0));
                mSeqConfigDataLen += sizeof(padding0);

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding1, sizeof(padding1));
                mSeqConfigDataLen += sizeof(padding1);

                memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, (AM_U8 *)(&(mpCodec->height)), 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, (AM_U8 *)(&(mpCodec->width)), 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding2, sizeof(padding2));
                mSeqConfigDataLen += sizeof(padding2);

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding3, sizeof(padding3));
                mSeqConfigDataLen += sizeof(padding3);
            }
            break;

        case CODEC_ID_MPEG1VIDEO:
            break;

        case CODEC_ID_MPEG2VIDEO:
            break;

        case CODEC_ID_MPEG4:
            if (mpCodec->extradata_size <= mSeqConfigDataSize || AdjustSeqConfigDataSize(mpCodec->extradata_size)) {
                memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, mpCodec->extradata_size);
                mSeqConfigDataLen += mpCodec->extradata_size;
            }
            break;

        case CODEC_ID_H264: {
                u8 startcode[4] = {0, 0, 0, 0x01};
                memcpy(mPicConfigData, startcode, sizeof(startcode));
                mPicConfigDataLen = sizeof(startcode);

                if (!mpCodec->extradata || (mpCodec->extradata_size == 0)) {
                    break;
                }
                // extradata can be annex-b or AVCC format.
                // if extradata begins with sequence: 00 00 00 01, it's annex-b format,
                // else extradata is AVCC format, it needs to know what the is the size of the nal_size field, in bytes.
                // AVCC format: see ISO/IEC 14496-15 5.2.4.1
                AMLOG_PRINTF("extradata size:%d data:%02x %02x %02x %02x %02x.\n",
                    mpCodec->extradata_size,
                    mpCodec->extradata[0],
                    mpCodec->extradata[1],
                    mpCodec->extradata[2],
                    mpCodec->extradata[3],
                    mpCodec->extradata[4]);

                if (mpCodec->extradata[0] != 0x01) {
                    AMLOG_PRINTF("extradata is annex-b format.\n");
                    mH264DataFmt = H264_FMT_ANNEXB;

                    if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                        AdjustSeqConfigDataSize(mpCodec->extradata_size)) {

                        memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, mpCodec->extradata_size);
                        mSeqConfigDataLen += mpCodec->extradata_size;
                    }
                } else {
                    AMLOG_PRINTF("extradata is AVCC format.\n");
                    mH264DataFmt = H264_FMT_AVCC;

                    AM_INT spss = BE_16(mpCodec->extradata + 6);
                    AM_INT ppss = BE_16(mpCodec->extradata + 6 + 2 + spss + 1);
                    mH264AVCCNaluLen = 1 + (mpCodec->extradata[4] & 3);

                    // the configuration record shall contain no sequence or picture parameter sets
                    // (spss and ppss shall both have the value 0).
                    if (spss > 0 &&
                        ppss > 0 &&
                        spss + ppss < mpCodec->extradata_size) {

                        AM_INT size = 2 * sizeof(startcode) + spss + ppss;
                        if (size <= mSeqConfigDataSize ||
                            AdjustSeqConfigDataSize(size)) {

                            memcpy(mSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 8, spss);
                            mSeqConfigDataLen += spss;

                            memcpy(mSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
                            mSeqConfigDataLen += ppss;
                        }
                    }
                    AM_INFO("-------spss:%d,ppss:%d NaluLen:%d-----\n", spss,ppss, mH264AVCCNaluLen);
                }
            }
            break;

        default:
            AM_ERROR("bad codec_id %d.\n", mpCodec->codec_id);
            break;
    }

    AM_ASSERT(mSeqConfigDataLen <= mSeqConfigDataSize);

    AMLOG_BINARY("Generate sequence config data: codec_id %d, mSeqConfigDataLen %d.\n", mpCodec->codec_id, mSeqConfigDataLen);
}

void CVideoDecoderDsp::UpdatePicConfigData(AVPacket* pPacket)
{
    switch (mpCodec->codec_id) {

        case CODEC_ID_WMV3: {
                int temp = pPacket->size;
                u8 *p = (u8 *)(&temp);
                if (pPacket->flags&AV_PKT_FLAG_KEY)
                    *(p + 3) = 0x80;

                memcpy(mPicConfigData, p, 4);
                temp = 0;
                memcpy(mPicConfigData + 4, p, 4);
                mPicConfigDataLen = 8;
            }
            break;

        case CODEC_ID_VC1:
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
        case CODEC_ID_MPEG4:
        case CODEC_ID_H264:
            break;

        default:
            break;
    }
}

bool CVideoDecoderDsp::AdjustSeqConfigDataSize(AM_INT size)
{
    AM_U8 *pBuf = (AM_U8 *)av_malloc(size);
    if (pBuf) {
        av_free(mSeqConfigData);
        mSeqConfigDataSize = size;
        mSeqConfigData = pBuf;
        AMLOG_PRINTF("AdjustSeqConfigDataSize success.\n");
    }
    return (pBuf != NULL);
}


void CVideoDecoderDsp::FeedConfigData(AVPacket *mpPacket)
{
	switch (mpCodec->codec_id) {
	case CODEC_ID_VC1: {
			u8 startcode[4] = {0, 0, 1,0x0d};
			if(!mbConfigData) {
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 1, mpCodec->extradata_size - 1);
				mbConfigData = true;
			}
			mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));

		}
		break;

	case CODEC_ID_WMV3: {
			u8 padding0[] = {0x78,0x16,0x00};
			u8 padding1[] = {0xc5,0x04,0x00,0x00,0x00};
			u8 padding2[] = {0x0c,0x00,0x00,0x00};
			u8 padding3[] = {0xd5,0x0c,0x00,0x10,0x5d,0xf2,0x05,0x00,0x0c,0x00,0x00,0x00};

			if(!mbConfigData) {
				//feed the number of frames
				//mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpStream->nb_frames)), 3);
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding0, sizeof(padding0));
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding1, sizeof(padding1));
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata, /*mpCodec->extradata_size - 1*/4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpCodec->height)), 4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpCodec->width)), 4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding2, sizeof(padding2));
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding3, sizeof(padding3));
				mbConfigData = true;
			}

			//feed frame header
			int temp = mpPacket->size;
			u8 *p = (u8 *)(&temp);
			if (mpPacket->flags&AV_PKT_FLAG_KEY)
				*(p + 3) = 0x80;

			mpCurrAddr = CopyToBSB(mpCurrAddr, p, 4);

			temp = 0;
			mpCurrAddr = CopyToBSB(mpCurrAddr, p, 4);
		}
		break;

	case CODEC_ID_MPEG1VIDEO:
			break;

	case CODEC_ID_MPEG2VIDEO:
			break;

	case CODEC_ID_MPEG4: {
			if(!mbConfigData) {
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata, mpCodec->extradata_size);
				mbConfigData = true;
			}
		}
		break;

	case CODEC_ID_H264: {
			u8 startcode[4] = {0, 0, 0,0x01};
			if (mpPacket->flags&AV_PKT_FLAG_KEY) {

				mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
				AM_INT spss = BE_16(mpCodec->extradata + 6);
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 8, spss);

				mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
				AM_INT ppss = BE_16(mpCodec->extradata + 6 + 2 + spss + 1);
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
				AM_PRINTF("-------spss:%d,ppss:%d-----\n",spss,ppss);
			}
			mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
		}
		break;

	default:
		break;

	}

}

void CVideoDecoderDsp::FillGOPHeader()
{
    AM_ASSERT(mpCodec);
    AM_U32 tick_high = (AM_U32)mpStream->r_frame_rate.den;
    AM_U32 tick_low = tick_high & 0x0000ffff;
    AM_U32 scale_high = (AM_U32)mpStream->r_frame_rate.num;
    AM_U32  scale_low = scale_high & 0x0000ffff;
    AM_U32  pts_high = 0;
    AM_U32  pts_low = 0;

    AMLOG_PRINTF("rate=%d,scale=%d\n",tick_high,scale_high);

    tick_high >>= 16;
    scale_high >>= 16;

    mGOPHeader[0] = 0; // start code
    mGOPHeader[1] = 0;
    mGOPHeader[2] = 0;
    mGOPHeader[3] = 1;

    mGOPHeader[4] = 0x7a; // NAL header
    mGOPHeader[5] = 0x01; // version main
    mGOPHeader[6] = 0x01; // version sub

    mGOPHeader[7] = tick_high >> 10;
    mGOPHeader[8] = tick_high >> 2;
    mGOPHeader[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
    mGOPHeader[10] = tick_low >> 3;

    mGOPHeader[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
    mGOPHeader[12] = scale_high >> 4;
    mGOPHeader[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
    mGOPHeader[14] = scale_low >> 5;

    mGOPHeader[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
    mGOPHeader[16] = pts_high >> 6;

    mGOPHeader[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
    mGOPHeader[18] = pts_low >> 7;
    mGOPHeader[19] = (pts_low << 1) | 1;

    //hard code here
    mGOPHeader[20] = 0; //pVideoInfo->N;
    mGOPHeader[21] = 0; //(pVideoInfo->M << 4) & 0xf0;
}

void CVideoDecoderDsp::UpdateGOPHeader(AM_PTS pts)
{
    AM_U32 pts_32 = pts % (1<<31);
    AM_U32 pts_high = (pts_32 >> 16) & 0x0000ffff;
    AM_U32 pts_low = pts_32 & 0x0000ffff;

    mGOPHeader[15] = (mGOPHeader[15]  & 0xFC) | (pts_high >> 14); // 2 bits of pts_high
    mGOPHeader[16] = pts_high >> 6;

    mGOPHeader[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
    mGOPHeader[18] = pts_low >> 7;
    mGOPHeader[19] = (pts_low << 1) | 1;
}

bool CVideoDecoderDsp::CheckH264SPSExist(AM_U8 *pData, AM_INT iDataLen, AM_INT *pSPSPos, AM_INT *pSPSLen)
{
    // REF ISO-IEC 14496-10 7.3
    // 1. find SPS
    // 2. if SPS is found, then find PPS
    // 3. if SPS&PPS are found, then calcaulte the length of SPS/PPS

    AM_INT i = 0;
    AM_UINT startCode = 0;

    bool bSPSFound = false;

    // find SPS
    while (i < iDataLen) {
        if ((startCode == 0x000001) && ((pData[i]&0x1F) == 7)) {
            bSPSFound = true;
            *pSPSPos = (pData[i-4] == 0) ? (i-4) : (i-3);
            //AMLOG_INFO("1 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
            break;
        }

        startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
        i++;
    }

    bool bPPSFound = false;
    if (bSPSFound) {

        // find PPS
        while (i < iDataLen) {
            if ((startCode == 0x000001) && ((pData[i]&0x1F) == 8)) {
                bPPSFound = true;
                //AMLOG_INFO("2 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
                break;
            }

            startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
            i++;
        }
    }

    if (bSPSFound && bPPSFound) {
        // count the length of SPS&PPS
        while (i < iDataLen) {
            if ((startCode == 0x000001) && ((pData[i]&0x1F) != 8)) {
                *pSPSLen = (pData[i-4] == 0) ? (i-4-*pSPSPos) : (i-3-*pSPSPos);
                //AMLOG_INFO("3 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
                break;
            }

            startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
            i++;
        }
    }

    //AMLOG_INFO("bSPSFound=%d *pSPSPos=%d *pSPSLen=%d\n", bSPSFound, *pSPSPos, *pSPSLen);

    return bSPSFound;
}

void CVideoDecoderDsp::FeedConfigDataWithUDECWrapper(AVPacket *mpPacket)
{
    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    //send seq data
    if (!mbConfigData || ((mpCodec->codec_id == CODEC_ID_H264) && (mpPacket->flags&AV_PKT_FLAG_KEY))) {
        if (!(mLogOutput & LogDisableNewPath)) {
            AMLOG_PRINTF("!!Feeding DUDEC_SEQ_HEADER_LENGTH %d.\n", DUDEC_SEQ_HEADER_LENGTH);
            if(mpGConfig->globalFlag & FILL_PES_SSP_HEADER){
                //AM_INFO("Fill\n");
                mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
            }
        }else{
            if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        }
        if (mpCodec->codec_id == CODEC_ID_H264){
            // Pre-check SPS/PPS Purpose: to avoid send different SPS/PPS, when SPS/PPS has embedded in frame data and varied
            // 1. find sps/pps in limited packet header, in which checkSPSLen is an experienced value
            // 2. if not found, then copy SeqConfigData
            // 3. if found and varied, then update SeqConfigData but needn't copy SeqConfigData
            bool bSPSFound = false;
            int idxSPSStart = 0, idxSPSLen = 0;

            int checkSPSLen = (mpPacket->size > mSeqConfigDataLen) ? mSeqConfigDataLen : mpPacket->size;
            bSPSFound = CheckH264SPSExist(mpPacket->data, checkSPSLen, &idxSPSStart, &idxSPSLen);
            if (bSPSFound) {
                // check sps len, update SeqConfigData if sps varies.
                if (idxSPSLen != mSeqConfigDataLen) {
                    if (AdjustSeqConfigDataSize(idxSPSLen)) {
                        memcpy(mSeqConfigData, mpPacket->data+idxSPSStart, idxSPSLen);
                        mSeqConfigDataLen = idxSPSLen;
                    } else {
                        AMLOG_ERROR("AdjustSeqConfigDataSize Failure\n");
                    }
                } else {
                    AMLOG_WARN("idxSPSLen is equal to mSeqConfigDataLen\n");
                }
                mbConfigData = true;
            } else {
                mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
                mbConfigData = true;
            }


        }else{
        mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
        mbConfigData = true;
        }
    }


    UpdatePicConfigData(mpPacket);

    if(mpCodec->codec_id == CODEC_ID_VC1){
        if(*(mpPacket->data) == 0 && *(mpPacket->data+1) == 0 && *(mpPacket->data+2) == 1 ){
            mPicConfigDataLen = 0;
        }
    }

    auLength = mPicConfigDataLen + mpPacket->size;
    if (mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_WMV3) {
        auLength -= 4;
    }

    //generate pes header from pts, data length
    //AM_INFO("AmbaVdec [PTS:%lld], current av pts %lld.\n", mpBuffer->GetPTS(), mpPacket->pts);
    AM_U64 pts = mpBuffer->GetPTS();
    //1TODO if(pts >= 0) {
    if(1){
        pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(pts & 0xffffffff), (AM_U32)(pts >> 32), auLength, 1, 0);
    } else {
        pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, auLength, 0, 0);
    }

    AM_ASSERT(pesHeaderLen <= DUDEC_PES_HEADER_LENGTH);


    //send udec pes header
    if (!(mLogOutput & LogDisableNewPath)) {
        AMLOG_DEBUG("pesHeaderLen %d, mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
        if(mpGConfig->globalFlag & FILL_PES_SSP_HEADER)
            mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
        AMLOG_DEBUG("done pesHeaderLen %d.mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
    }else{
        AM_ASSERT(0);
        if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
   }

    //send pic config data
    if (mPicConfigDataLen) {
//        AM_PRINTF("comes here 2?\n");
        mpCurrAddr = CopyToBSB(mpCurrAddr, mPicConfigData, mPicConfigDataLen);
//        AM_ASSERT(mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_VC1 || mpCodec->codec_id == CODEC_ID_WMV3);
    }

}

void CVideoDecoderDsp::FeedConfigDataWithH264GOPHeader(AVPacket *mpPacket)
{
    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    AM_ASSERT(mpCodec->codec_id == CODEC_ID_H264);
    //send seq data
    if (!mbConfigData || ((mpCodec->codec_id == CODEC_ID_H264)&& (mpPacket->flags&AV_PKT_FLAG_KEY))) {
        UpdateGOPHeader((AM_PTS)mpPacket->pts);
        mpCurrAddr = CopyToBSB(mpCurrAddr, mGOPHeader, sizeof(mGOPHeader));
        mbConfigData = true;
        //feed sps and pps nal unit after gop header.
        mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
#ifdef AM_DEBUG
        AMLOG_BINARY("Print UDEC SEQ: %d.\n", DUDEC_SEQ_HEADER_LENGTH);
        PrintBitstremBuffer(mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        AMLOG_BINARY("Print seq config data: %d.\n", mSeqConfigDataLen);
        PrintBitstremBuffer(mSeqConfigData, mSeqConfigDataLen);
#endif
    }

#ifdef AM_DEBUG
    AMLOG_BINARY("Print UDEC PES Header: %d.\n", pesHeaderLen);
    PrintBitstremBuffer(mUPESHeader, pesHeaderLen);
    AMLOG_BINARY("Print pic config data: %d.\n", mPicConfigDataLen);
    PrintBitstremBuffer(mPicConfigData, mPicConfigDataLen);
#endif

}

void CVideoDecoderDsp::FeedData(AVPacket* pPacket)
{
    if (mpCodec->codec_id == CODEC_ID_H264)
    {
        AM_U8 startcode[4] = {0, 0, 0, 0x01 };
        AM_U8 *ptr = pPacket->data;
        AM_INT size = 0;

        if (mH264DataFmt == H264_FMT_AVCC)
        {
            AM_INT len = 0;
            while (size + 4 < pPacket->size)
            {
                len = BE_32(ptr);
                memcpy(ptr, startcode, sizeof(startcode));
                ptr += 4 + len;
                size += 4 + len;
            }

            mpCurrAddr = CopyToBSB(mpCurrAddr, pPacket->data + 4, pPacket->size - 4);
        }else if (mH264DataFmt == H264_FMT_ANNEXB){
            AM_INT firstStatcodeOffset = 0;
            while (size + 4 < pPacket->size)
            {
                if(memcmp(ptr, startcode, sizeof(startcode)) == 0){
                    firstStatcodeOffset = size;
                    break;
                }
                    size++;
                    ptr++;
            }
            mpCurrAddr = CopyToBSB(mpCurrAddr, pPacket->data + 4 + firstStatcodeOffset, pPacket->size - 4 - firstStatcodeOffset);
        }

    }else{
        mpCurrAddr = CopyToBSB(mpCurrAddr, pPacket->data, pPacket->size);
    }
}

AM_ERR CVideoDecoderDsp::FillEOS()
{
    AM_ERR err;
    AM_U8* pFrameStart = mpCurrAddr;
    switch (mDecType) {
    case UDEC_H264: {
        static AM_U8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
        //mbFillEOS= true;
        mpCurrAddr = CopyToBSB(mpCurrAddr, eos, sizeof(eos));
        }
        break;

    case UDEC_VC1: {
        //mbFillEOS= true;
        // WMV3
        if( mpCodec->codec_id == CODEC_ID_WMV3 ){
            static AM_U8 eos_wmv3[4] = {0xFF, 0xFF, 0xFF, 0x0A};// special eos for wmv3
            mpCurrAddr = CopyToBSB(mpCurrAddr, eos_wmv3, sizeof(eos_wmv3));
        }
        // VC-1
        else{
            static AM_U8 eos[4] = {0, 0, 0x01, 0xA};
            mpCurrAddr = CopyToBSB(mpCurrAddr, eos, sizeof(eos));
        }
        }
        break;

	case UDEC_MP12: {
			static AM_U8 eos[4] = {0, 0, 0x01, 0xB7};
			//mbFillEOS= true;
			mpCurrAddr = CopyToBSB(mpCurrAddr, eos, sizeof(eos));

		}
		break;

	case UDEC_MP4H: {
			static AM_U8 eos[4] = {0, 0, 0x01, 0xB1};
			//mbFillEOS= true;
			mpCurrAddr = CopyToBSB(mpCurrAddr, eos, sizeof(eos));
		}
		break;

	case UDEC_JPEG:
		break;

	default:
		AM_PRINTF("not implemented!\n");
		break;
	}
    err = DecodeBuffer(pFrameStart, mpCurrAddr);
    AM_INFO("CVideoDecoderDsp FillEOS ToDsp Done, Sending to GR...\n");
    mOBuffer.Clear();
    mOBuffer.SetOwnerType(DECODER_DSP);
    mOBuffer.SetBufferType(EOS_BUFFER);
    mOBuffer.SetStreamType(STREAM_VIDEO);
    mOBuffer.SetIdentity(mConIndex);
    mOBuffer.SetPTS(0);
    SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);
    return err;
}
/*
AM_ERR CVideoDecoderDsp::GetDecodedFrame(CBuffer* pVideoBuffer)
{
    AM_INT eos_invalid, buffer_id;
    return mpDsp->GetDecodedFrame(mDspIndex, buffer_id, eos_invalid, (CVideoBuffer*)pVideoBuffer);
}
*/
//===========================================
//
//===========================================
AM_ERR CVideoDecoderDsp::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = 0;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CVideoDecoderDsp::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = 0;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

/*
void CVideoDecoderDsp::PrintState()
{
    //AMLOG_PRINTF("CVideoDecoderDsp: msState=%d, %d input data, %d free buffers.\n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
}*/
void CVideoDecoderDsp::Dump()
{
    AM_INFO("       {---Amba Dsp Decoder---}\n");
    //AM_INFO("           Config->en audio:%d, en video:%d\n", mbEnAudio, mbEnVideo);
    AM_INFO("           State->on state:%d, isalloced:%d, decoded:%d\n", mState, mbAlloced, mbDecoded);

    AM_U8* dspRead = NULL;

    //AM_INFO("DecodeBuffer, size: %x,    %d\n", pEnd - pStart, pEnd - pStart);
    //_Dump_es_data(pStart, pEnd);
    mpDsp->GetUDECState2(mDspIndex, &dspRead);
    //AM_INFO("Debug(%d) Before: pStart:%p, CurBufferPtr:%p,      DspReadPtr:%p\n", mConIndex, pStart, mpCurrAddr, dspRead);

    AM_INFO("           Dump DSP:   Dsp Read:%p, CurMWPtr:%p, Distance: %d\n", dspRead, mpCurrAddr, mpCurrAddr-dspRead);
}

void CVideoDecoderDsp::Delete()
{
    AM_INFO("CVideoDecoderDsp Delete\n");
    AM_ERR err;
    av_free(mSeqConfigData);
    AM_ASSERT(mpDsp != NULL);
    err = mpDsp->ReleaseUDECInstance(mDspIndex);
    if(err != ME_OK)
    {
        if(err == ME_CLOSED){
            AM_INFO("------\n");
            AM_INFO("All Dsp Instance are Released!\n");
            AM_INFO("------\n");
            //AM_DeleteDSPHandler(mpShare); ON enginempDsp = NULL;
        }else{
            AM_ERROR("ReleaseUDECInstance Failed return %d!!\n", err);
        }
    }
    inherited::Delete();
}

CVideoDecoderDsp::~CVideoDecoderDsp()
{
    AM_INFO("***~CVideoDecoderDsp start...\n");
    AM_INFO("***~CVideoDecoderDsp end.\n");
}


