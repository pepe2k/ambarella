
/*
 * video_decoder_dsp_ve.cpp
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

#define LOG_NDEBUG 0
#define LOG_TAG "video_decoder_dsp_ve"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "pbif.h"
#include "engine_guids.h"
#include "am_dsp_if.h"
#if PLATFORM_ANDROID
#include <basetypes.h>
#else
#include "basetypes.h"
#endif
#include "iav_drv.h"
#include "iav_transcode_drv.h"


#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amdsp_common.h"
#include "amba_dsp_ione.h"
#include "video_decoder_dsp_ve.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

//for dump

#define MB	(1024 * 1024)

#define BE_16(x) (((unsigned char *)(x))[0] <<  8 | \
		  ((unsigned char *)(x))[1])

#define BE_32(x) ((((unsigned char *)(x))[0] << 24) | \
                  (((unsigned char *)(x))[1] << 16) | \
                  (((unsigned char *)(x))[2] << 8)  | \
                   ((unsigned char *)(x))[3])

//-----------------------------------------------------------------------
//
// CVideoDecoderDspVE
//
//-----------------------------------------------------------------------
void CVideoDecoderDspVE::PrintBitstremBuffer(AM_U8* p, AM_UINT size)
{
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
}

IDecoder* CVideoDecoderDspVE::Create(IFilter* pFilter, CMediaFormat* pFormat)
{
    CVideoDecoderDspVE *result = new CVideoDecoderDspVE(pFilter, pFormat);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

AM_ERR CVideoDecoderDspVE::Construct()
{
    AM_ERR err;
    err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("CVideoDecoderDspVE ,inherited::Construct fail err = %d .\n", err);
        return err;
    }
    DSetModuleLogConfig(LogModuleVideoDecoderDSP);
    //todo change av_malloc
    if ((mSeqConfigData = (AM_U8 *)av_malloc(DUDEC_MAX_SEQ_CONFIGDATA_LEN)) == NULL)
        return ME_NO_MEMORY;
    mSeqConfigDataSize = DUDEC_MAX_SEQ_CONFIGDATA_LEN;

    err = ConstructDsp();
    if (err != ME_OK)
    {
        AM_ERROR("CVideoDecoderDspVE ,ConstructDsp fail err = %d .\n", err);
        return err;
    }
    //goto run
    PostCmd(CMD_RUN);
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::ConstructDsp()
{
    AM_ERR err;
    AM_INT vFormat = 0;
    //bool bAdd = true;
    mpStream = (AVStream* )mpMediaFormat->format;
    mpCodec = (AVCodecContext* )mpStream->codec;
    mpShare = ((CInterActiveFilter*)mpFilter)->mpSharedRes;
    //CDspConfig* pConfig = mpConfig->pDspConfig;
    switch (mpCodec->codec_id) {
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

    //mpDsp = CAmbaDspIOne::Create(mpShare);
    if (mpShare->udecHandler == NULL)
    {
        err = AM_CreateDSPHandler(mpShare);
        if(err != ME_OK)
            return err;
    }

//data wrapper config
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    if (mpShare->dspConfig.addVideoDataType != eAddVideoDataType_iOneUDEC && mpShare->dspConfig.addVideoDataType != eAddVideoDataType_none) {
        AM_ASSERT(0);
        AMLOG_ERROR("bad datatype config %d, use iOne's default.\n", mpShare->dspConfig.addVideoDataType);
        mDataWrapperType = eAddVideoDataType_iOneUDEC;
    } else {
        mDataWrapperType = mpShare->dspConfig.addVideoDataType;
    }
#elif TARGET_USE_AMBARELLA_A5S_DSP
    AM_ASSERT(mbAddDataWrapper);
    AM_ASSERT(mpShare->dspConfig.addVideoDataType == eAddVideoDataType_a5sH264GOPHeader);
    if (mpShare->dspConfig.addVideoDataType != eAddVideoDataType_a5sH264GOPHeader) {
        AM_ASSERT(0);
        AMLOG_ERROR("bad datatype config %d, use a5s's default.\n", mpShare->dspConfig.addVideoDataType);
        mDataWrapperType = eAddVideoDataType_a5sH264GOPHeader;
    }
#endif

    //...
    mpDsp = mpShare->udecHandler;
    mpDsp->SetUdecNums(mpShare->udecNums);
    err = mpDsp->InitUDECInstance(mDspIndex, &mpShare->dspConfig, NULL, mDecType,
                                   mpCodec->width, mpCodec->height, 0);
    if(err != ME_OK)
    {
        AM_ASSERT(0);
        AMLOG_ERROR("mpDsp->InitUDECInstance fail, ret %d, index %d.\n", err, mDspIndex);
        return err;
    }
    //
    mpStartAddr = mpShare->dspConfig.udecInstanceConfig[mDspIndex].pbits_fifo_start;
    mSpace = mpShare->dspConfig.udecInstanceConfig[mDspIndex].bits_fifo_size;
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
    //todo ,kill the mvformat, if(mbadd...)
    if(mbAddDataWrapper)
    {
        if (mDataWrapperType == eAddVideoDataType_iOneUDEC) {
            FillUSEQHeader(mUSEQHeader, vFormat, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpShare->is_mp4s_flag, mpShare->vid_container_width, mpShare->vid_container_height);
            InitUPESHeader(mUPESHeader, vFormat);
        } else if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
            FillGOPHeader();
        } else if (mDataWrapperType != eAddVideoDataType_none) {
            AM_ASSERT(0);
            AMLOG_ERROR("!!must not comes here, mDataWrapperType %d.\n", mDataWrapperType);
        }
        GenerateConfigData();
    }
    mIavFd = mpShare->mIavFd;
    if(mpShare->mDSPmode != 3)
        mpDsp->GetUDECState(mDspIndex, mpShare->udec_state, mpShare->vout_state, mpShare->error_code);

    return ME_OK;
}
//==============================
//
//==============================
AM_ERR CVideoDecoderDspVE::DoPause()
{
#if 0
    AMLOG_PRINTF("CVideoDecoderDspVE::DoPause.\n");
    //put it here in only a5s cases, ione's pause is called by engine, can uniform it in future
    if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
        mpDsp->PauseUDEC(mDspIndex);
    }
#endif
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::DoResume()
{
#if 0
    AMLOG_PRINTF("CVideoDecoderDspVE::DoResume.\n");
    //put it here in only a5s cases, ione's resume is called by engine, can uniform it in future
    if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
        mpDsp->ResumeUDEC(mDspIndex);
    }
#endif
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::DoFlush()
{
    if (mDataWrapperType == eAddVideoDataType_a5sH264GOPHeader) {
        //flush here
        mpDsp->FlushUDEC(mDspIndex);
    }
    //mpDsp->FlushUDEC(mDspIndex);
    mbConfigData = false;
    //mbFillEOS = false;

    mpStartAddr = mpShare->dspConfig.udecInstanceConfig[mDspIndex].pbits_fifo_start;
    mSpace = mpShare->dspConfig.udecInstanceConfig[mDspIndex].bits_fifo_size;
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;

    mbAlloced = false;
    mbDecoded = true;
    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    //mdLastEndPointer = mpCurrAddr;

    //msState = STATE_PENDING;
    //CmdAck(ME_OK);
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::DoStop()
{
    //so, if recive a stop cmd,
    //mpDsp->Stop();//how about
    //clear state!!!
    mbRun = false;
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::DoIsReady()
{
    //AM_INFO("CVideoDecoderDspVE::DoIsReady.\n");
    AM_UINT size;
    AM_ERR err;

    if(mpCurrAddr==mpEndAddr)//for bits fifo wrap case
        mpCurrAddr=mpStartAddr;

    //todo
    if(mpBuffer->GetType() == CBuffer::EOS)
    {
        size = 8;
    }else{
        mpPacket = (AVPacket*)((AM_U8*)mpBuffer + sizeof(CBuffer));
        if(mpPacket == NULL)
        {
            AMLOG_ERROR("mpPacket == NULL.\n");
            return ME_ERROR;
        }
        size = mpPacket->size + mSeqConfigDataLen + DUDEC_PES_HEADER_LENGTH;
    }
    err = AllocBSB(size);
    if(err == ME_UDEC_ERROR)
    {
        AMLOG_ERROR("err == ME_UDEC_ERROR.\n");
        PostFilterMsg(GFilter::MSG_UDEC_ERROR);
        return err;
    }
/*    if(err == ME_BUSY)
    {
        PostFilterMsg(GFilter::MSG_DSP_GET_OUTPIC);
        //PostFilterMsg(GFilter::MSG_READY);
        return err;
    }
*/
    if(err != ME_OK)
    {
        AMLOG_ERROR("err != %d.\n", err);
        return err;
    }
    mbAlloced = true;
    PostFilterMsg(GFilter::MSG_READY);
    return ME_OK;
}
//==============================
//init: mbDecoded = true, mbAlloced = false;
//==============================
AM_ERR CVideoDecoderDspVE::IsReady(CBuffer* pBuffer)
{
    //AM_INFO("CVideoDecoderDspVE IsReady\n");
    //if(pBuffer->GetType() == CBuffer::EOS)
        //return ME_OK;
    if(mbDecoded == true && mbAlloced == true)
        return ME_OK;

    if(mbDecoded == true && mbAlloced == false)
    {
//        if(!mbGetOutPic){
            AM_ASSERT(mpBuffer == NULL);
            if(mpBuffer != NULL)
                //IS DOING DoIsReady
                return ME_ERROR;
//        }
        mpBuffer = pBuffer;
        PostCmd(CMD_ISREADY);
    }
    return ME_ERROR;
}

AM_ERR CVideoDecoderDspVE::ReSet()
{
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

    return ME_OK;
    //DoStop();
    //SendCmd(CMD_RUN);
}

void CVideoDecoderDspVE::Stop()
{
    /*AM_ERR err;
    err = ReSet();
    if(err != ME_OK)
    {
        mbRun = false;
        return err;
    }
    SendCmd(CMD_STOP);*/
    PostCmd(CMD_STOP);
    //return ME_OK;
}

//todo, think 1: upfilter's pause should be hold or not?
//yes,but we can ack asap.
AM_ERR CVideoDecoderDspVE::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("CVideoDecoderDspVE::ProcessCmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    switch (cmd.code)
    {
    case CMD_STOP:
        AM_INFO("CVideoDecoderDspVE Process CMD_STOP.\n");
        DoStop();
        //CmdAck(ME_OK);
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
        mbDecoded = false;
        err = DoDecode();
        mbDecoded = true;
        mbAlloced = false;
        //todo copy this Cbuffer to send out
        //AM_INFO("CMD_DECODE...\n");
        SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)NULL);//wakeup isready and send cbuffer. make sure sended and then ack isready.
        AMLOG_DEBUG("CMD_DECODE Done...\n");
        CmdAck(ME_OK);
        break;

    case CMD_ISREADY:
        err = DoIsReady();
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return err;
}

void CVideoDecoderDspVE::OnRun()
{
    AM_ERR err;
    CMD cmd;
    mbRun = true;
    while(mbRun)
    {
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        err = ProcessCmd(cmd);
        if(err != ME_OK)
        {
            if(cmd.code != CMD_STOP)//for test cmd
            {
                PostFilterMsg(GFilter::MSG_DECODER_ERROR);
            }
        }

    }
}
//==============================
//
//==============================

AM_U8* CVideoDecoderDspVE::CopyToBSB(AM_U8* ptr, AM_U8* buffer, AM_UINT size)
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

AM_ERR CVideoDecoderDspVE::AllocBSB(AM_UINT size)
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
    //AM_INFO("AllocBSB , size done :\n");
    return err;
}


AM_ERR CVideoDecoderDspVE::DoDecode()
{
    AM_U8 *pFrameStart;
    //AM_ASSERT(mpBuffer == pBuffer);
    AM_ERR err;

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
                    }

                    ptr += mH264AVCCNaluLen;
                    curPos += mH264AVCCNaluLen;

                    if (len <= 0 || (curPos + len > mpPacket->size)) {
                        AM_INFO("CAmbaVideoDecoder::ProcessBuffer error: pkt_size:%d len:%d mH264AVCCNaluLen:%d curPos:%d\n",
                            mpPacket->size, len, mH264AVCCNaluLen, curPos);
                        mpCurrAddr= pRecoveryAddr;
                        //PostFilterMsg(ME_VIDEO_DATA_ERROR);
                        return ME_OK;
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
                mpCurrAddr = CopyToBSB(mpCurrAddr,ptr, mpPacket->size - curPos);
            }
        } else {
            mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
        }


    // FeedConfigData(mpPacket);
    //FeedData(mpPacket);

    //AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer start, start %p, end %p, diff %d, totdiff %d, %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr - pFrameStart, mpCurrAddr - mpStartAddr, mpEndAddr - mpCurrAddr);
    err = DecodeBuffer(pFrameStart, mpCurrAddr);
    //AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer done.\n");
    if (err == ME_UDEC_ERROR) {
        PostFilterMsg(GFilter::MSG_UDEC_ERROR);
        return err;
    }
    if(err != ME_OK)
    {
        //AM_INFO("DecodeBuffer not ME_OK\n");
        return err;
    }

    GenerateCBuffer();
    mpBuffer->Release();
    mpBuffer = NULL;
    return ME_OK;
}

#if 0
inline void CVideoDecoderDspVE::_Dump_es_data(AM_U8* pStart, AM_U8* pEnd)
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

AM_ERR CVideoDecoderDspVE::DecodeBuffer(AM_U8*pStart, AM_U8*pEnd)
{
    AM_ERR err = ME_OK;
    //AM_INFO("DecodeBuffer, size: %x,    %d\n", pEnd - pStart, pEnd - pStart);
    //_Dump_es_data(pStart, pEnd);

    err = mpDsp->DecodeBitStream(mDspIndex, pStart, pEnd);
    if(err != ME_OK) {
    }
    if(err == ME_UDEC_ERROR) {
        //PostFilterMsg(GFilter::MSG_UDEC_ERROR);
    }
    //AM_INFO("DecodeBuffer, Done\n");
    return err;
}

AM_ERR CVideoDecoderDspVE::GenerateCBuffer()
{
    return ME_OK;
    //mOBuffer.Clear();
    //mpDsp->GetSendBuffer(&mOBuffer);
    //mpVideoOutputPin->SendBuffer(pBuffer);
}

//===========================================
//
//Those Functions Generate Data For DSP
//
//===========================================
uint8_t* CVideoDecoderDspVE::Find_StartCode(uint8_t* extradata, AM_INT size)
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

void CVideoDecoderDspVE::GenerateConfigData()
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
                    AMLOG_PRINTF("-------spss:%d,ppss:%d NaluLen:%d-----\n", spss,ppss, mH264AVCCNaluLen);
                }
            }
            break;

        default:
            AM_ERROR("bad codec_id %d.\n", mpCodec->codec_id);
            break;
    }

    AM_ASSERT(mSeqConfigDataLen <= (AM_UINT)mSeqConfigDataSize);

    AMLOG_BINARY("Generate sequence config data: codec_id %d, mSeqConfigDataLen %d.\n", mpCodec->codec_id, mSeqConfigDataLen);
}

void CVideoDecoderDspVE::UpdatePicConfigData(AVPacket* pPacket)
{
    switch (mpCodec->codec_id) {

        case CODEC_ID_WMV3: {
                int temp = pPacket->size;
                u8 *p = (u8 *)(&temp);
                if (pPacket->flags&PKT_FLAG_KEY)
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

bool CVideoDecoderDspVE::AdjustSeqConfigDataSize(AM_INT size)
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


void CVideoDecoderDspVE::FeedConfigData(AVPacket *mpPacket)
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
			if (mpPacket->flags&PKT_FLAG_KEY)
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
			if (mpPacket->flags&PKT_FLAG_KEY) {

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

void CVideoDecoderDspVE::FillGOPHeader()
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

void CVideoDecoderDspVE::UpdateGOPHeader(AM_PTS pts)
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

void CVideoDecoderDspVE::FeedConfigDataWithUDECWrapper(AVPacket *mpPacket)
{
    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    //send seq data
    if (!mbConfigData || ((mpCodec->codec_id == CODEC_ID_H264) && (mpPacket->flags&PKT_FLAG_KEY))) {
        if (!(mLogOutput & LogDisableNewPath)) {
            AMLOG_PRINTF("!!Feeding DUDEC_SEQ_HEADER_LENGTH %d.\n", DUDEC_SEQ_HEADER_LENGTH);
            mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        }else{
            if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        }

        mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
        mbConfigData = true;
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
    AMLOG_PTS("AmbaVdec [PTS], current pts %lld.\n", mpPacket->pts);
    if (mpPacket->pts >= 0) {
        pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(mpPacket->pts&0xffffffff), (AM_U32)(((AM_U64)mpPacket->pts) >> 32), auLength, 1, 0);
    } else {
        pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, auLength, 0, 0);
    }

    AM_ASSERT(pesHeaderLen <= DUDEC_PES_HEADER_LENGTH);

    //send udec pes header
    if (!(mLogOutput & LogDisableNewPath)) {
        AMLOG_DEBUG("pesHeaderLen %d, mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
        mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
        AMLOG_DEBUG("done pesHeaderLen %d.mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
    }else{
        if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
   }

    //send pic config data
    if (mPicConfigDataLen) {
//        AM_PRINTF("comes here 2?\n");
        mpCurrAddr = CopyToBSB(mpCurrAddr, mPicConfigData, mPicConfigDataLen);
//        AM_ASSERT(mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_VC1 || mpCodec->codec_id == CODEC_ID_WMV3);
    }

}

void CVideoDecoderDspVE::FeedConfigDataWithH264GOPHeader(AVPacket *mpPacket)
{
//    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    AM_ASSERT(mpCodec->codec_id == CODEC_ID_H264);
    //send seq data
    if (!mbConfigData || ((mpCodec->codec_id == CODEC_ID_H264)&& (mpPacket->flags&PKT_FLAG_KEY))) {
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

void CVideoDecoderDspVE::FeedData(AVPacket* pPacket)
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

AM_ERR CVideoDecoderDspVE::FillEOS()
{
    AMLOG_PRINTF("CVideoDecoderDspVE FillEOS.\n");
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
    return err;
}
/*
AM_ERR CVideoDecoderDspVE::GetDecodedFrame(CBuffer* pVideoBuffer)
{
    AM_INT eos_invalid, buffer_id;
    return mpDsp->GetDecodedFrame(mDspIndex, buffer_id, eos_invalid, (CVideoBuffer*)pVideoBuffer);
}
*/
//===========================================
//
//===========================================
AM_ERR CVideoDecoderDspVE::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CVideoDecoderDspVE::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

/*
void CVideoDecoderDspVE::PrintState()
{
    //AMLOG_PRINTF("CVideoDecoderDspVE: msState=%d, %d input data, %d free buffers.\n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
}*/

void* CVideoDecoderDspVE::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    return inherited::GetInterface(refiid);
}

void CVideoDecoderDspVE::PrintState()
{
    AMLOG_INFO("CVideoDecoderDspVE[%d] State: ", mDspIndex);
    AMLOG_INFO("mbAlloced: %d, mbDecoded: %d\n", mbAlloced, mbDecoded);
    if(3 == mpShare->mDSPmode) return;
    AM_UINT udecState, voutState, errorCode;
    mpDsp->GetUDECState(mDspIndex, udecState, voutState, errorCode);
    AMLOG_INFO("\t\t: udecState: %u, voutState: %u, errorCode: %x.\n", udecState, voutState, errorCode);
}

void CVideoDecoderDspVE::Delete()
{
    inherited::Delete();
}

CVideoDecoderDspVE::~CVideoDecoderDspVE()
{
    AM_INFO("***~CVideoDecoderDspVE start...\n");
    AM_INFO("~CVideoDecoderDspVE av_free(mSeqConfigData).\n");
    AM_ERR err;
    av_free(mSeqConfigData);
    if(mpDsp != NULL){
        err = mpDsp->ReleaseUDECInstance(mDspIndex);
        if(err != ME_OK){
            if(err == ME_CLOSED){
                AMLOG_INFO("CVideoDecoderDspVE AM_DeleteDSPHandler: \n");
                AM_DeleteDSPHandler(mpShare);
                AMLOG_INFO("CVideoDecoderDspVE AM_DeleteDSPHandler: done!\n");
            }else{
                AM_ERROR("ReleaseUDECInstance Failed return %d!!\n", err);
            }
        }
        mpDsp = NULL;
    }
    AM_INFO("***~CVideoDecoderDspVE end.\n");
}


