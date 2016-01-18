
/**
 * dual_readstream.cpp
 *
 * History:
 *    2011/8/3 - [Chong Xing] create file
 *    2011/8/22- [Qiongxiong Z] modify file
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_record_if.h"
#include "record_if.h"
#include "engine_guids.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "itron_encoder.h"

#define DBGMSG AM_DBG

IFilter* CreateITronEncoderFilter(IEngine *pEngine, bool bDual)
{
    return CVideoEncoderITron::Create(pEngine, bDual);
}
//-----------------------------------------------------------------------
//
// AmbaIPCStreamer
//
//-----------------------------------------------------------------------
AmbaIPCStreamer *AmbaIPCStreamer::create()
{
    AmbaIPCStreamer *new_ins = new (AmbaIPCStreamer);

    if(new_ins==NULL){
        AM_ERROR("Fail to create AmbaStream instance\n\r");
        return NULL;
    }

    if(new_ins->init()<0){
        delete new_ins;
        return NULL;
    }

    return new_ins;
}

int AmbaIPCStreamer::init()
{
    mqueue_init();

    msg = (am_msg_t *)malloc(sizeof(am_msg_t));
    if(msg==NULL){
        AM_ERROR("%s:%d\n\r",__func__,__LINE__);
        return -1;
    }

    res =(am_res_t *)malloc(sizeof(am_res_t));
    if(res==NULL){
        AM_ERROR("%s:%d\n\r",__func__,__LINE__);
        return -1;
    }

    //msg->msg_id = AIC_MDL_REGISTER;
     mAic = Aic::createInstance();
     if(mAic==NULL){
        AM_ERROR("%s:%d\n\r",__func__,__LINE__);
         return -1;
     }

     if(createToken()<0){
        AM_ERROR("%s:%d\n\r",__func__,__LINE__);
         return -1;
     }

     return 0;
}

AmbaIPCStreamer::AmbaIPCStreamer():
    mAic_token(-1),encode_status(0)
{
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    //Creater
}

AmbaIPCStreamer::~AmbaIPCStreamer()
{
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    release_AmbaStreamer();
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    if(mAic_token>0){
        releaseToken();
    }
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    if(mAic!=NULL){
        //msg->msg_id = AIC_MDL_RELEASE;
        //mAic->Cmd(msg,res);
        delete mAic;
        mAic=NULL;
    }
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    if(msg != NULL){
        free(msg);
        msg=NULL;
    }
    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    if(res!=NULL){
        free(res);
        res=NULL;
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
}

int AmbaIPCStreamer::createToken(void)
{
    int rval = 0;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);
    msg->msg_id = AIC_GET_TOKEN;
    rval = mAic->Cmd(msg,res);
    if(rval == 0 && res->param_size==4){
        int *par;

        par = (int *)(&res->param[0]);
        mAic_token=*par;
        return mAic_token;
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::releaseToken(void)
{
    int *par,rval;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    if(mAic_token<=0){
        return 0;
    }
    msg->msg_id = AIC_RELEASE_TOKEN;
    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    rval = mAic->Cmd(msg,res);

    mAic_token=-1;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::init_AmbaStreamer()
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_INIT;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_INIT fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::release_AmbaStreamer()
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_RELEASE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_RELEASE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::enable_AmbaStreamer(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_ENABLE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_ENABLE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::disable_AmbaStreamer(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_DISABLE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_DISABLE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::is_EncFrameValid(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_IS_ENCFRAMEVALID;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<0){
        AM_ERROR("HMSG_STREAMER_IS_ENCFRAMEVALID fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::get_EncFrameDataInfo(int type, AmbaStream_frameinfo_t *frameInfo)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_GET_ENCFRAMEDATAINFO;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->param_size==0)
    {
        AM_ERROR("HMSG_STREAMER_GET_ENCFRAMEDATAINFO: invalid res length\n\r");
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_GET_ENCFRAMEDATAINFO fail\n\r");
    }

    memcpy(frameInfo,res->param,sizeof(AmbaStream_frameinfo_t));

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::get_EncFrameData(int type, u8 **Framedata, unsigned int *Datalength)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_GET_ENCFRAMEDATA;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->param_size==0)
    {
        AM_ERROR("HMSG_STREAMER_GET_ENCFRAMEDATA: invalid res length\n\r");
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_GET_ENCFRAMEDATA fail\n\r");
    }

    par = (int *)(&res->param[0]);
    *Framedata=(u8 *)(*par);
    par = (int *)(&res->param[4]);
    *Datalength=(unsigned int)(*par);

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::notify_read_done(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_NOTIFY_READ_DONE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_NOTIFY_READ_DONE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::get_EncSPSPPS(int type, u8 *SPS, int *SPS_len, u8 *PPS, int *PPS_len, unsigned int *ProfileID)
{
    int rval=0,par_pos=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_GET_SPS_PPS;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_GET_SPS_PPS fail\n\r");
    }

    if(res->param_size==0)
    {
        AM_ERROR("HMSG_STREAMER_GET_SPS_PPS: invalid res length\n\r");
        return -1;
    }

    par_pos = 0;
//get SPS_len
    par = (int *)(&res->param[par_pos]);
    rval = (*par);
    if(SPS_len!= NULL){
        *SPS_len = rval;
    }
    par_pos += sizeof(int);

//get SPS
    if(rval>0){
        par = (int *)(&res->param[par_pos]);
        if(SPS!=NULL){
            memcpy(SPS,(u8 *)par,sizeof(u8)*rval);
        }
        par_pos += rval;
    }

//get PPS_len
    par = (int *)(&res->param[par_pos]);
    rval = (*par);
    if(PPS_len!=NULL){
        *PPS_len = rval;
    }
    par_pos += sizeof(int);

//get PPS
    if(rval>0){
        par = (int *)(&res->param[par_pos]);
        if(PPS!=NULL){
            memcpy(PPS,(u8 *)par,sizeof(u8)*rval);
        }
        par_pos += rval;
    }

//get ProfileID
    par = (int *)(&res->param[par_pos]);
    if(ProfileID!=NULL){
        *ProfileID = (unsigned int)(*par);
    }
    par_pos += sizeof(unsigned int);

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::config_for_Streamout(int mode)
{
    int rval=-1;
    int *par;
    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    //Enable dual stream
    memset(msg, 0, MSG_BLOCK_SIZE);
    msg->msg_id = HMSG_RECORDER_VIDEO_ENC_MODE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    if(mode==REC_STREAMING_OUT_PRI)
        *par = VIDEO_MODE_NORMAL;
    else
        *par = VIDEO_MODE_DUAL;
    msg->param_size = 8;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("AIC_cmd VIDEO_MODE_DUAL fail\n\r");
        return -1;
    }

    //Enable stream out
    memset(msg, 0, MSG_BLOCK_SIZE);
    msg->msg_id = HMSG_RECORDER_VIDEO_ENC_MODE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = VIDEO_MODE_STREAMING;
    par = (int *)(&msg->param[8]);
    *par = mode;
    msg->param_size = 12;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("AIC_cmd VIDEO_MODE_STREAMING fail\n\r");
        return -1;
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::start_encode(void)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_RECORDER_RECORD;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    //if(res->rval<=0){
    //    AM_ERROR("HMSG_RECORDER_RECORD fail\n\r");
    //}

    encode_status=1;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::stop_encode(void)
{
    int rval=0;

    if(encode_status==1){
        rval=start_encode(); //send record again to stop.
        encode_status=0;
    }

    return rval;
}

int AmbaIPCStreamer::wait_for_reply(unsigned int reply_msg)
{
    int mbx_id,rval=0;
    am_msg_t *rev_msg;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    mq_get_mbxid(&mbx_id);

    do {
        rval = rcv_mqueue(mbx_id, &rev_msg);
        if(rval<0){
            AM_ERROR("%s: rcv_mqueue() fail, rcal=%d\n\r",__func__,rval);
            break;
        }
        mq_msg_rel(rev_msg);
    } while(rev_msg->msg_id != reply_msg);

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::switch_to_VideoMode(void)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_MODE_SWITCH_TO_DSC_VIDEO;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(wait_for_reply(HMSG_AMD_NEW_MW_STATE_PREVIEW)<0){
        AM_ERROR("wait_for_reply() fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::config_for_Streamin(int Streams)
{
    int rval=-1;
    AmbaStream_DecConfig_t *DecConfig;
    // the parameter occupies 2 blocks of sizeof(am_msg_t)
    am_msg_t *msg2 = (am_msg_t *)malloc(sizeof(am_msg_t)<<1);
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    //set DecConfig
    memset(msg2, 0, MSG_BLOCK_SIZE<<1);
    msg2->msg_id = HMSG_STREAMER_SET_DECCONFIG;

    par = (int *)(&msg2->param[0]);
    *par = mAic_token;

    DecConfig = (AmbaStream_DecConfig_t *)(&msg2->param[4]);

    DecConfig->brate    =600000;          /**< the average bitrate, bits per second */
    DecConfig->brate_min    =450000;          /**< the average bitrate, bits per second */
    DecConfig->limit    =0xFFFFFFFF;      /**< the maximum file size, Megabytes */
    DecConfig->vid        =0x48323634;      /**< Video stream ID, 0 for not exist */
    DecConfig->width    =432;             /**< Video width */
    DecConfig->height    =240;             /**< Video height */
    DecConfig->rate        =180000;          /**< Video rate */
    DecConfig->scale    =6006;            /**< Video scale, fps = rate/scale, 29.97 = 30000/1001*/
    DecConfig->entropy_mode    =0;               /**< The stream is CAVLC: 0, The stream is CABAC: 1 */
    DecConfig->idr_interval    =4;
    if(Streams&ENABLE_AUDIOSTREAM)
        DecConfig->aid    =0xff;           /**< Audio stream ID, 0 for NO AUDIO */
    else
        DecConfig->aid    =0;               /**< Audio stream ID, 0 for NO AUDIO */
    DecConfig->channels    =2;               /**< Audio channels */
    DecConfig->samples    =48000;           /**< Audio sample rate */
    DecConfig->mode        =1;               /**< Video coding mode */
    DecConfig->M        =1;               /**< Number of picture between reference pictures */
    DecConfig->N        =8;               /**< Number of picture between I pictures */
    DecConfig->ar_x        =16;              /**< Aspect ration ar_x:ar_y */
    DecConfig->ar_y        =9;               /**< Aspect ration ar_x:ar_y */
    DecConfig->frmsz_a    =1024;            /**< audio samples per frame*/
    DecConfig->color_style    =1;               /* 0: TV, 1: PC */

    msg2->param_size = sizeof(AmbaStream_DecConfig_t)+4;

    msg2->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    rval=mAic->Cmd(msg2,res);
    if(rval<0){
        AM_ERROR("AIC_cmd HMSG_STREAMER_SET_DECCONFIG fail\n\r");
        return -1;
    }
    free(msg2);

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::switch_to_PBStream(void)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_MODE_SWITCH_TO_PB_STREAM;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::switch_to_DUPLEXStream(void)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_MODE_SWITCH_TO_PB_STREAM;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 4;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(wait_for_reply(HMSG_AMD_NEW_MW_STATE_PREVIEW)<0){
        AM_ERROR("wait_for_reply() fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return rval;
}

int AmbaIPCStreamer::get_DecRemainFrameBufSize(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_GET_DECREMAINFRAMEBUFSIZE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<0){
        AM_ERROR("HMSG_STREAMER_GET_DECREMAINFRAMEBUFSIZE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::set_DecFrameType(int type, AmbaStream_frameinfo_t *frameInfo)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_SET_DECFRAMETYPE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;
    par = (int *)(&msg->param[8]);
    memcpy((AmbaStream_frameinfo_t *)par, frameInfo, sizeof(AmbaStream_frameinfo_t));

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8 + sizeof(AmbaStream_frameinfo_t);

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<0){
        AM_ERROR("HMSG_STREAMER_SET_DECFRAMETYPE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::set_DecFrameDataInfo(int type, AmbaStream_frameinfo_t *frameInfo)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_SET_DECFRAMEDATAINFO;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;
    memcpy(&msg->param[8],frameInfo, sizeof(AmbaStream_frameinfo_t));

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8 + sizeof(AmbaStream_frameinfo_t);

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_SET_DECFRAMEDATAINFO fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::set_video_wh(int v_resid)
{
    int rval = 0;
    am_msg_t msg;
    am_res_t res;
    unsigned int *par;

    memset(&msg, 0, MSG_BLOCK_SIZE);
    msg.ver=MSG_VER1;
    msg.msg_id = HMSG_RECORDER_VIDEO_WH;
    par = (unsigned int *)(&msg.param[0]);
    par[0] = mAic_token;
    par[1] = v_resid;
    //par[2] = height;
    msg.blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);
    msg.param_size = 8;
    rval = mAic->Cmd(&msg,&res);
    if(rval !=0 ){
        AM_ERROR("Set video Width & Height fail !!!");
        return -1;
    }
    return rval;
}

int AmbaIPCStreamer::set_video_framerate(int frames_per_second)
{
    int rval = 0;
    am_msg_t msg;
    am_res_t res;
    unsigned int *par;

    memset(&msg, 0, MSG_BLOCK_SIZE);
    msg.ver=MSG_VER1;
    msg.msg_id = HMSG_RECORDER_VIDEO_FRATE;
    par = (unsigned int *)(&msg.param[0]);
    par[0] = mAic_token;
    par[1] = frames_per_second;
    msg.blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);
    msg.param_size = 8;
    rval = mAic->Cmd(&msg,&res);
    if(rval !=0 ){
        AM_ERROR("Set video Framerate fail !!!");
        return -1;
    }
    return rval;
}

int AmbaIPCStreamer::set_audio_channel_num(int numberOfChannels)
{
    int rval = 0;
    am_msg_t msg;
    am_res_t res;
    unsigned int *par;

    memset(&msg, 0, MSG_BLOCK_SIZE);
    msg.ver=MSG_VER1;
    msg.msg_id = HMSG_RECORDER_AUDIO_CHANNEL;
    par = (unsigned int *)(&msg.param[0]);
    par[0] = mAic_token;
    par[1] = numberOfChannels;
    msg.blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);
    msg.param_size = 8;
    rval = mAic->Cmd(&msg,&res);
    if(rval !=0 ){
        AM_ERROR("Set video Audio Channel Num fail !!!");
        return -1;
    }
    return rval;
}

int AmbaIPCStreamer::set_audio_bitrate(int64_t bitrate)
{
    int rval = 0;
    am_msg_t msg;
    am_res_t res;
    unsigned int *par;
    /*itron only aceepts 128000*/
    bitrate = 128000;

    memset(&msg, 0, MSG_BLOCK_SIZE);
    msg.ver=MSG_VER1;
    msg.msg_id = HMSG_RECORDER_AUDIO_BITRATE;
    par = (unsigned int *)(&msg.param[0]);
    par[0] = mAic_token;
    par[1] = bitrate;
    msg.blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);
    msg.param_size = 8;
    rval = mAic->Cmd(&msg,&res);
    if(rval !=0 ){
        AM_ERROR("Set video Audio Bitrate fail !!!");
        return -1;
    }
    return rval;
}

int AmbaIPCStreamer::config_video_only(void)
{
    int rval=-1;
    int *par;
    DBGMSG("%s:%d\n\r",__func__,__LINE__);

    //configure video only
    memset(msg, 0, MSG_BLOCK_SIZE);
    msg->msg_id = HMSG_RECORDER_VIDEO_ENC_MODE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = VIDEO_MODE_VIDEOONLY;
    msg->param_size = 8;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("AIC_cmd VIDEO_MODE_VIDEOONLY fail\n\r");
        return -1;
    }

    return rval;
}

int AmbaIPCStreamer::set_audio_samplerate(int64_t samplingRate)
{
    int rval = 0;
    am_msg_t msg;
    am_res_t res;
    unsigned int *par;

    memset(&msg, 0, MSG_BLOCK_SIZE);
    msg.ver=MSG_VER1;
    msg.msg_id = HMSG_RECORDER_AUDIO_SAMPLERATE;
    par = (unsigned int *)(&msg.param[0]);
    par[0] = mAic_token;
    par[1] = samplingRate;
    msg.blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);
    msg.param_size = 8;
    rval = mAic->Cmd(&msg,&res);
    if(rval !=0 ){
        AM_ERROR("Set video Samplerate fail !!!");
        return -1;
    }
    return rval;
}


int AmbaIPCStreamer::notify_write_done(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_NOTIFY_WRITE_DONE;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_NOTIFY_WRITE_DONE fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::get_DecFrameDataWP(int type, u8 **FrameData, u8 **base, u8 **limit)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_GET_DECFRAMEDATAWP;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<=0){
        AM_ERROR("HMSG_STREAMER_GET_DECFRAMEDATAWP fail\n\r");
    }

    if(res->param_size==0)
    {
        AM_ERROR("HMSG_STREAMER_GET_DECFRAMEDATAWP: invalid res length\n\r");
        return -1;
    }

    par = (int *)(&res->param[0]);
    *FrameData=(u8 *)(*par);
    par = (int *)(&res->param[4]);
    *base=(u8 *)(*par);
    par = (int *)(&res->param[8]);
    *limit=(u8 *)(*par);

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}

int AmbaIPCStreamer::canfeedDecFrame(int type)
{
    int rval=0;
    int *par;

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    memset(msg, 0, MSG_BLOCK_SIZE);

    msg->msg_id = HMSG_STREAMER_CANFEEDDECFRAME;

    par = (int *)(&msg->param[0]);
    *par = mAic_token;
    par = (int *)(&msg->param[4]);
    *par = type;

    msg->blknum_priority = MSG_BLKNUM_PRIORITY(1, 0);

    msg->param_size = 8;

    rval=mAic->Cmd(msg,res);
    if(rval<0){
        AM_ERROR("%s: AIC_cmd fail(0x%08x)\n\r",__func__,rval);
        return -1;
    }

    if(res->rval<0){
        AM_ERROR("HMSG_STREAMER_IS_DECFRAMEREADOUT fail\n\r");
    }

    DBGMSG("%s:%d\n\r",__func__,__LINE__);
    return res->rval;
}
//
//
static inline int check_video_size(int width, int height, int VideoFrameRate)
{
	/* NTSC type */
	if( (width == 1920) && (height == 1080) && (VideoFrameRate == DSP_ENC_59_94_P)){
		return SENSOR_VIDEO_RES_TRUE_1080P_FULL;
	}else if( (width == 1920) && (height == 1080) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_TRUE_1080P_HALF;// 1xx
	}else if( (width == 1920) && (height == 1080) && (VideoFrameRate == DSP_ENC_29_97_I)){
		return SENSOR_VIDEO_RES_TRUE_1080I;// 2
	}else if( (width == 1440) && (height == 1080) && (VideoFrameRate == DSP_ENC_59_94_P)){
		return SENSOR_VIDEO_RES_COMP_1080P_FULL;
	}else if( (width == 1440) && (height == 1080) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_COMP_1080P_HALF;
	}else if( (width == 1440) && (height == 1080) && (VideoFrameRate == DSP_ENC_29_97_I)){
		return SENSOR_VIDEO_RES_COMP_1080I;
	}else if( (width == 1280) && (height == 720) && (VideoFrameRate == DSP_ENC_59_94_P)){
		return SENSOR_VIDEO_RES_HD_FULL;
	}else if( (width == 1280) && (height == 720) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_HD_HALF;//7
	}else if( (width == 720) && (height == 480) && (VideoFrameRate == DSP_ENC_59_94_P)){
		return SENSOR_VIDEO_RES_SDWIDE;
	}else if( (width == 720) && (height == 480) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_SD;//9
	}else if( (width == 352) && (height == 240) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_CIF;//10
	}else if( (width == 848) && (height == 480) && (VideoFrameRate == DSP_ENC_59_94_P)){
		return SENSOR_VIDEO_RES_WVGA_FULL;
	}else if( (width == 848) && (height == 480) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_WVGA_HALF;//12xx
	}else if( (width == 640) && (height == 480) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_VGA;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_WQVGA;
	}else if( (width == 320) && (height == 240) && (VideoFrameRate == DSP_ENC_29_97_P)){
		return SENSOR_VIDEO_RES_QVGA;//15xx
	}else if( (width == 320) && (height == 240) && (VideoFrameRate == DSP_ENC_120)){
		return SENSOR_VIDEO_RES_QVGA_P120;
	}else if( (width == 320) && (height == 240) && (VideoFrameRate == DSP_ENC_180)){
		return SENSOR_VIDEO_RES_QVGA_P180;
	}else if( (width == 320) && (height == 240) && (VideoFrameRate == DSP_ENC_240)){
		return SENSOR_VIDEO_RES_QVGA_P240;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_100)){
		return SENSOR_VIDEO_RES_WQVGA_P100;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_120)){
		return SENSOR_VIDEO_RES_WQVGA_HFR_1;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_180)){
		return SENSOR_VIDEO_RES_WQVGA_P180;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_200)){
		return SENSOR_VIDEO_RES_WQVGA_P200;
	}else if( (width == 432) && (height == 240) && (VideoFrameRate == DSP_ENC_240)){
		return SENSOR_VIDEO_RES_WQVGA_HFR_2;
	}else{
		AM_INFO("Video Size(%d , %d, %d) is NOT supported and use default", width, height, VideoFrameRate );
		return SENSOR_VIDEO_RES_TRUE_1080P_FULL;
	}
}

//-----------------------------------------------------------------------
//
// CVideoEncoderITronBP
//
//-----------------------------------------------------------------------

CVideoEncoderITronBP* CVideoEncoderITronBP::Create(const char *name, AM_UINT count)
{
    CVideoEncoderITronBP *result = new CVideoEncoderITronBP(name);
    if (result && result->Construct(count) != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CVideoEncoderITronBP::OnReleaseBuffer(CBuffer *pBuffer)
{
    if (pBuffer->mFlags & CBUFFER_ALLOCED)
    {
        delete[] (pBuffer->mpData);
    }
}


//-----------------------------------------------------------------------
//
// CVideoEncoderITron
//
//-----------------------------------------------------------------------
IFilter* CVideoEncoderITron::Create(IEngine *pEngine, bool bDual)
{
    CVideoEncoderITron *result = new CVideoEncoderITron(pEngine, bDual);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoEncoderITron::Construct()
{
    AM_UINT i;
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleItronStreammer);

//move to AddOutputPin
#if 0
    for(i=0; i < mMaxStreamIndexPlus1; i++)
    {
        mpBSB[i] = CVideoEncoderITronBP::Create("Itron EncBSB", 64);
        if (mpBSB == NULL) {
            AM_ERROR("CSimpleBufferPool::Create fail in CVideoEncoderITron::Construct().\n");
            return ME_NO_MEMORY;
        }
        mpOutputPin[i] = CEncoderITronOutput::Create(this);
        if (mpOutputPin == NULL) {
            AM_ERROR("CEncoderITronOutput::Create fail in CVideoEncoderITron::Construct().\n");
            return ME_NO_MEMORY;
        }
        mpOutputPin[i]->SetBufferPool(mpBSB[i]);
    }
#endif

    err = Init();
    if (err != ME_OK)
    {
        AM_ERROR("CVideoEncoderITron:: Init fail in CVideoEncoder::Construct().\n");
        return err;
    }
    return ME_OK;
}

AM_ERR CVideoEncoderITron::Init()
{
    AM_ERR err;
    mStreamer = AmbaIPCStreamer::create();
    if(mStreamer == NULL)
    {
        AM_ERROR("Fail to create AmbaStream module instance\n");
        return ME_ERROR;
    }
    if(!mStreamer->init_AmbaStreamer())
    {
        AM_ERROR("Fail to init AmbaStream!\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CVideoEncoderITron::AddOutputPin(AM_UINT& i, AM_UINT type)
{
    if (type == IParameters::StreamType_Video) {
        i = mConfig.vIndex;
    } else if (type == IParameters::StreamType_Audio) {
        i = mConfig.aIndex;
    } else {
        AM_ERROR("BAD type %d in CVideoEncoderITron::AddOutputPin.\n", type);
        return ME_ERROR;
    }

    AMLOG_PRINTF("CVideoEncoderITron::AddOutputPin, allocate index %d, type %d.\n", i, type);

    //safe check code
    AM_ASSERT(NULL == mpBSB[i]);
    AM_ASSERT(NULL == mpOutputPin[i]);
    if (NULL != mpBSB[i] || NULL != mpOutputPin[i]) {
        AM_ERROR("please check engine's build graph, it should have bugs, if code comes here.\n");
        return ME_ERROR;
    }

    mpBSB[i] = CVideoEncoderITronBP::Create("Itron EncBSB", VideoEncoderITron_bufferCountInPool);
    if (mpBSB == NULL) {
        AM_ERROR("CSimpleBufferPool::Create fail in CVideoEncoderITron::Construct().\n");
        return ME_NO_MEMORY;
    }
    mpOutputPin[i] = CEncoderITronOutput::Create(this);
    if (mpOutputPin == NULL) {
        AM_ERROR("CEncoderITronOutput::Create fail in CVideoEncoderITron::Construct().\n");
        return ME_NO_MEMORY;
    }
    mpOutputPin[i]->SetBufferPool(mpBSB[i]);

    if (type == IParameters::StreamType_Video) {
        mConfig.vIndex += 2;
        //some enum from ipc code
        if (i == 0) {
            mpOutputPin[i]->mPreDefIndex = AMBASTREAM_VIDEO_ENCSTREAM;
        } else if (i == 2) {
            mpOutputPin[i]->mPreDefIndex = AMBASTREAM_VIDEO_ENCSTREAM_2nd;
        } else {
            AM_ASSERT(0);
        }
    } else if (type == IParameters::StreamType_Audio) {
        mConfig.aIndex += 2;
        //some enum from ipc code
        if (i == 1) {
            mpOutputPin[i]->mPreDefIndex = AMBASTREAM_AUDIO_ENCSTREAM;
        } else if (i == 3) {
            mpOutputPin[i]->mPreDefIndex = AMBASTREAM_AUDIO_ENCSTREAM_2nd;
        } else {
            AM_ASSERT(0);
        }
    } else {
        AM_ERROR("!!!it's impossiable to comes here.\n");
        return ME_ERROR;
    }

    //debug assert
    AM_ASSERT(mpOutputPin[i]->mPreDefIndex == i);
    if ((i+1) > mMaxStreamIndexPlus1) {
        mMaxStreamIndexPlus1 = i+1;
    }

    AMLOG_PRINTF("CVideoEncoderITron::AddOutputPin done, allocate index %d, type %d, mMaxStreamIndexPlus1 %d.\n", i, type, mMaxStreamIndexPlus1);
    return ME_OK;
}

AM_ERR CVideoEncoderITron::SetOutput()
{
    AM_UINT index = 0;
    AM_ASSERT(mStreamer);

    for (index = 0; index < mMaxStreamIndexPlus1; index ++) {
        if (mpOutputPin[index]) {
            AM_ASSERT(mpBSB[index]);
            mStreamer->enable_AmbaStreamer(mpOutputPin[index]->mPreDefIndex);
        }
    }

/*
    if(mbReadDual)
    {
        enable_stream = ENABLE_VIDEOSTREAM | ENABLE_AUDIOSTREAM | ENABLE_VIDEOSTREAM_2nd | ENABLE_AUDIOSTREAM_2nd;
    }else{
        enable_stream = ENABLE_VIDEOSTREAM | ENABLE_AUDIOSTREAM;
    }

    if(enable_stream & ENABLE_VIDEOSTREAM){
        mStreamer->enable_AmbaStreamer(AMBASTREAM_VIDEO_ENCSTREAM);
    }
    if(enable_stream & ENABLE_AUDIOSTREAM){
        mStreamer->enable_AmbaStreamer(AMBASTREAM_AUDIO_ENCSTREAM);
    }
    if(enable_stream & ENABLE_VIDEOSTREAM_2nd){
        mStreamer->enable_AmbaStreamer(AMBASTREAM_VIDEO_ENCSTREAM_2nd);
    }
    if(enable_stream & ENABLE_AUDIOSTREAM_2nd){
        mStreamer->enable_AmbaStreamer(AMBASTREAM_AUDIO_ENCSTREAM_2nd);
    }

    mStreamer->switch_to_VideoMode();
    //configure streaming out
    if(mbReadDual)
    {
        mStreamer->config_for_Streamout(REC_STREAMING_OUT_DUAL);
    }else{
        mStreamer->config_for_Streamout(REC_STREAMING_OUT_PRI);
    }
    //Start encode
    mStreamer->start_encode();
*/
    return ME_OK;
}

AM_ERR CVideoEncoderITron::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(index < mMaxStreamIndexPlus1);
    if (index >= mMaxStreamIndexPlus1) {
        AMLOG_ERROR("WRONG index (%d), exceed max number(%d) in CVideoEncoder::SetParameters.\n", index, mMaxStreamIndexPlus1);
        return ME_BAD_PARAM;
    }

    if (!param) {
        AMLOG_ERROR("NULL param, in CVideoEncoder::SetParameters.\n");
        return ME_BAD_PARAM;
    }
    //AM_ASSERT(type == IParameters::StreamType_Video);
    //AM_ASSERT(format == IParameters::StreamFormat_H264);

    if(type == IParameters::StreamType_Video)
    {
        mConfig.smParam[index].stream_format = IParameters::StreamFormat_H264;
        mConfig.smParam[index].width = param->video.pic_width;
        mConfig.smParam[index].height = param->video.pic_height;
        //AM_INFO("TEST LOG2: mConfig pic_width:%d\n", mConfig.smParam[mConfig.vIndex].width);
        //convert video farmerate
        switch (param->video.framerate) {
            case IParameters::VideoFrameRate_29dot97:
                mConfig.smParam[index].framerate = DSP_ENC_29_97_P;
                break;
            case IParameters::VideoFrameRate_59dot94:
                mConfig.smParam[index].framerate = DSP_ENC_59_94_P;
                break;
            default:
                AMLOG_WARN("itron encoder only support 29.97 and 59.94, 0x%x not supported, use default 29.97.\n", param->video.framerate);
                mConfig.smParam[index].framerate = DSP_ENC_29_97_P;
                break;
        }
    }else if(type == IParameters::StreamType_Audio){
        mConfig.smParam[index].stream_format = IParameters::StreamFormat_AAC;
        mConfig.smParam[index].sample_rate = param->audio.sample_rate;
        mConfig.smParam[index].channel_number = param->audio.channel_number;
        mConfig.smParam[index].a_bitrate = param->audio.bitrate;
    }

    return ME_OK;
}

CVideoEncoderITron::~CVideoEncoderITron()
{
    AM_UINT i = 0;
    for(; i < mMaxStreamIndexPlus1; i++)
    {
        AM_DELETE(mpOutputPin[i]);
        AM_RELEASE(mpBSB[i]);
    }

    if(!mStreamer)
    {
        delete mStreamer;
        mStreamer = NULL;
    }
}

void *CVideoEncoderITron::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IVideoEncoder)
        return (IVideoEncoder*)this;
    return inherited::GetInterface(refiid);
}

void CVideoEncoderITron::GetInfo(INFO& info)
{
    info.nInput = 0;
    info.nOutput = 4;
    info.pName = "VideoEncoderITron";
}

IPin* CVideoEncoderITron::GetOutputPin(AM_UINT index)
{
    AMLOG_DEBUG("VideoEncoderITron::GetOutputPin index %d, mMaxStreamIndexPlus1 %d, %p.\n", index, mMaxStreamIndexPlus1, mpOutputPin[index]);
    if (index >= mMaxStreamIndexPlus1)
        return NULL;
    return mpOutputPin[index];
}

AM_ERR CVideoEncoderITron::ConfigITronEnc()
{
    AM_ERR err;
    //enable streamming
    err = SetOutput();
    if(err != ME_OK)
        return err;

    if (mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM] && (NULL == mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM_2nd])) {
        mStreamer->config_for_Streamout(REC_STREAMING_OUT_PRI);
    } else if (mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM] && mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM_2nd]) {
        mbReadDual = true;
        mStreamer->config_for_Streamout(REC_STREAMING_OUT_DUAL);
    } else {
        AM_ASSERT(0);
        AM_ERROR("need add implement.\n");
    }

    //video only:
    if (!mpOutputPin[AMBASTREAM_AUDIO_ENCSTREAM] && !mpOutputPin[AMBASTREAM_AUDIO_ENCSTREAM]) {
        mStreamer->config_video_only();
    }

    mStreamer->switch_to_VideoMode();
    //set video width/height/framerate
    //video stream 0
    if (mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM]) {
        ConfigVideoParams(AMBASTREAM_VIDEO_ENCSTREAM);
    }
    //video stream 1
    if (mpOutputPin[AMBASTREAM_VIDEO_ENCSTREAM_2nd]) {
        ConfigVideoParams(AMBASTREAM_VIDEO_ENCSTREAM_2nd);
    }

    //Start encode
    mStreamer->start_encode();

    AMLOG_INFO("ConfigITronEnc Done!\n");
    return ME_OK;
}

AM_ERR CVideoEncoderITron::ConfigVideoParams(int index)
{
    AM_INT rval;
    AM_INT v_resid = 0;
    if(index == STREAM_VIDEO_2ND)
    {
        AM_INFO("ITron Can Only Support One(First) Way Video Config Params!\n");
        return ME_OK;
    }

    AMLOG_INFO("ConfigVideoParams: Width:%d, Height:%d, framerate %d\n", mConfig.smParam[index].width, mConfig.smParam[index].height, mConfig.smParam[index].framerate);
    v_resid = check_video_size(mConfig.smParam[index].width, mConfig.smParam[index].height, mConfig.smParam[index].framerate);
    AMLOG_INFO("Config video resolution, type id:%d .\n", v_resid);

    rval = mStreamer->set_video_wh(v_resid);
    if(rval != 0) {
        AMLOG_INFO("Set Video Width & Height Failed!\n");
    }
/*
    rval = mStreamer->set_video_framerate(mConfig.smParam[index].framerate);
    if(rval != 0) {
        AMLOG_INFO("Set video frame rate %d Failed!\n", mConfig.smParam[index].framerate);
    }
*/
    return ME_OK;
}

bool CVideoEncoderITron::ProcessCmd(CMD& cmd)
{
    //AM_ERR err = ME_OK;
    switch(cmd.code)
    {
    case CMD_STOP:
        DoStop();
        ProcessEOS();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_PAUSE:
        if(msState != STATE_PENDING)
        {
            for(AM_UINT i = 0; i < mMaxStreamIndexPlus1; i++){
                mbSkip[i] = false;
            }
            msState = STATE_PENDING;
        }
        break;

    case CMD_RESUME:
        if(msState == STATE_PENDING)
        {
            for(AM_UINT i = 0; i < mMaxStreamIndexPlus1; i++){
                mbSkip[i] = true;
            }
            msState = STATE_IDLE;
        }
        break;

    case CMD_OBUFNOTIFY:
        break;

    case CMD_FLOW_CONTROL:
        //DoFlowControl((FlowControlType)cmd.flag);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return 1;
}

void CVideoEncoderITron::OnRun()
{
    CmdAck(ME_OK);
    CQueue::QType type;
    CQueue::WaitResult result;
    CMD cmd;
    AM_ERR err;

    ConfigITronEnc();
    while(mbRun)
    {
        AMLOG_STATE("mMaxStreamIndexPlus1 %d, mbReadDual %d,CVideoEncoderITron::OnRun, state %d.\n", mMaxStreamIndexPlus1, mbReadDual , msState);
        switch (msState)
        {
        case STATE_IDLE:
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                err = ReadInputData();
                if(err == ME_OK)
                {
                    msState = STATE_WAIT_OUTPUT;
                }else if(err == ME_BUSY){
                    //usleep(5000);
                    break;
                }else if(err == ME_CLOSED){
                    ProcessEOS();
                    mbRun = false;
                }
            }
            break;

        case STATE_WAIT_OUTPUT:
            //AM_ASSERT(mpBuffer);
            //if(mpBSB->GetFreeBufferCnt() < mFifoInfo.count)
            err = CheckBufferPool();
            //NotifyFlowControl();
            DropFrame();
            if(err == ME_ERROR)
            {
                //Drop All
                msState = STATE_IDLE;
            }else if(err == ME_OK){
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            if(ProcessBuffer() == ME_OK)
            {
                msState = STATE_IDLE;
            }else{
                msState = STATE_ERROR;
            }
            break;

        case STATE_PENDING:
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                DropByPause();
            }
            break;

        case STATE_ERROR:
            //discard all data, wait eos
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            if (cmd.code == CMD_STOP) {
                ProcessEOS();
                mbRun = false;
                CmdAck(ME_OK);
            } else {
                //ignore other cmds
                AMLOG_WARN("how to handle this type of cmd %d, in Video Encoder error state.\n", cmd.code);
            }
            break;

        default:
            AM_ERROR("Not Support State Occur %d .\n",(AM_UINT)msState);
            break;
        }
    }
}

inline AM_ERR CVideoEncoderITron::IsIDRFrame(AM_UINT index)
{
    //return ME_OK;
    if(index == STREAM_AUDIO_1ST || index == STREAM_AUDIO_2ND)
        return ME_OK;
    AM_UINT type;

    AM_ASSERT(mpOutputPin[index]);

    type = ((mFrameInfo[index].bit_values)>>29) & 0x00000007;
    if(type == 1)
    {
        return ME_OK;
    }
    return ME_ERROR;
}

//set mbBlock here
inline AM_ERR CVideoEncoderITron::CheckBufferPool()
{
    AM_UINT i , needBuffer;
    bool bAll = true;
    for(i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        mbBlock[i] = false;
    }

    for(i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        if (NULL == mpOutputPin[i]) {
            //debug assert
            AM_ASSERT(NULL == mpBSB[i]);
            continue;
        }

        needBuffer = mGetData[i];
        //AM_INFO("^^^^BufferPool: %d, Cnt:%u!\n", i, mpBSB[i]->GetFreeBufferCnt());
        if(mpBSB[i]->GetFreeBufferCnt() < needBuffer)
        {
            AMLOG_INFO("^^^^BufferPool: %d, Not Enough Free Buffer!\n", i);
            mbBlock[i] = true;
        }else{
            bAll = false;
        }
    }
    return (bAll == true) ? ME_ERROR : ME_OK;
}

AM_ERR CVideoEncoderITron::NotifyFlowControl(FlowControlType type)
{
    CMD cmd0;

    cmd0.code = CMD_FLOW_CONTROL;
    cmd0.flag = (AM_U8)type;

    mpWorkQ->MsgQ()->PostMsg((void*)&cmd0, sizeof(cmd0));
    return ME_OK;
}
/*
void CVideoEncoderITron::DoFlowControl(FlowControlType type)
{
    AM_UINT i;
    SListNode* pNode;

    for (i=0; i<mStreamNum; i++) {
        AM_ASSERT(mpOutputPin[i]);
        if (mpOutputPin[i]) {
            pNode = mpOutputPin[i]->mpFlowControlList->alloc();
            pNode->context = type;
            mpOutputPin[i]->mpFlowControlList->append_2end(pNode);
        }
    }
}*/

//set mbSkip Here
inline AM_ERR CVideoEncoderITron::DropFrame()
{
    AM_UINT i = 0;
    for(; i < mMaxStreamIndexPlus1; i++)
    {
        if(mbBlock[i] == true)
        {
            //Drop this way
            if(IsIDRFrame(i) == ME_OK){
                mbSkip[i] = true;
                mIDRDrop[i]++;
            }
            //AMLOG_DEBUG("^^^^Not Output: %d, Drop Frame: %d, Has IDR Drop: %d !\n", i, GetFrameNum(i), mbSkip[i]);
            mTotalDrop[i] += mGetData[i];
        }
    }
    return ME_OK;
}

inline AM_ERR CVideoEncoderITron::DropByPause()
{
    ReadInputData();
    for(AM_UINT i = 0; i < mMaxStreamIndexPlus1; i++){
        mPauseDrop[i] += mGetData[i];
    }
    return ME_OK;
}

AM_ERR CVideoEncoderITron::ReadInputData()
{
    AM_ERR err;
    AM_UINT i;
    for(i= 0; i < MAX_NUM_STREAMS; i++)
    {
        memset(&mFrameInfo[i], 0, sizeof(AmbaStream_frameinfo_t));
        mDataPtr[i] = NULL;
        mDataLen[i] = 0;
        mGetData[i] = 0;
    }
    err = ReadStreamerData();
    if(err == ME_CLOSED)
        return ME_CLOSED;
    if(err != ME_OK)
        return ME_BUSY;
    //AM_INFO("PP");
    //test each output stream
    for(i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        if(mGetData[i] != 1)
            continue;
        if(mbSkip[i] == true)
        {
            //if curr stream has IDR, all this will be send to READY
            if(IsIDRFrame(i) == ME_OK)
            {
                return ME_OK;
                //mTotalDrop[i] = GetFrameNum(i); do this in procss buffer
            }
        }else{
            return ME_OK;
        }
    }
    for(i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        //AM_INFO("^^^^ReadInputData: %d, Drop Frame: %d !\n", i, mGetData[i]);
        mTotalDrop[i] += mGetData[i];
    }
    return ME_BUSY;
}

AM_ERR CVideoEncoderITron::ReadStreamerData()
{
    AM_INT ret;
    AM_UINT index = 0;

    for (index = 0; index < mMaxStreamIndexPlus1; index ++) {
        if (NULL == mpOutputPin[index]) {
            AM_ASSERT(NULL == mpBSB[index]);
            continue;
        }
        ret = mStreamer->is_EncFrameValid(index);
        if (ret < 0) {
            AMLOG_DEBUG("ret(%d)<0, index %d.\n", ret, index);
            mCheckVaild |= (1<<index);
        } else if(ret > 0) {
            if (mStreamer->get_EncFrameData(index, &mDataPtr[index], &mDataLen[index]) > 0) {
                if(mStreamer->get_EncFrameDataInfo(index, &mFrameInfo[index]) <= 0){
                    AMLOG_WARN("get_EncFrameDataInfo <=0, index %d.\n", index);
                    //return ME_ERROR;
                }else{
                    //AM_INFO("x1");
                    mGetData[index] = 1;
                }
            } else {
                AMLOG_WARN("get_EncFrameData <=0, index %d.\n", index);
            }
            mStreamer->notify_read_done(index);
        } else {
            AMLOG_DEBUG("ret==0, index %d.\n", index);
        }
    }

#if 0
    ret = mStreamer->is_EncFrameValid(AMBASTREAM_VIDEO_ENCSTREAM);
    if(ret < 0)
    {
        //AM_WARNING("w1");
        mCheckVaild |= (1<<STREAM_VIDEO_1ST);
    }else if(ret > 0){
        if(mStreamer->get_EncFrameData(AMBASTREAM_VIDEO_ENCSTREAM, &mDataPtr[STREAM_VIDEO_1ST], &mDataLen[STREAM_VIDEO_1ST]) > 0){
            if(mStreamer->get_EncFrameDataInfo(AMBASTREAM_VIDEO_ENCSTREAM, &mFrameInfo[STREAM_VIDEO_1ST]) <= 0){
                //AM_ERROR("a1");
                //return ME_ERROR;
            }else{
                //AM_INFO("x1");
                mGetData[STREAM_VIDEO_1ST] = 1;
            }
        }
        mStreamer->notify_read_done(AMBASTREAM_VIDEO_ENCSTREAM);
    }

    if(mbReadDual)
    {
        ret = mStreamer->is_EncFrameValid(AMBASTREAM_VIDEO_ENCSTREAM_2nd);
        if(ret < 0)
        {
            //AM_WARNING("w2");
            mCheckVaild |= (1<<STREAM_VIDEO_2ND);
        }else if(ret > 0){
            if(mStreamer->get_EncFrameData(AMBASTREAM_VIDEO_ENCSTREAM_2nd, &mDataPtr[STREAM_VIDEO_2ND], &mDataLen[STREAM_VIDEO_2ND]) > 0){
                if(mStreamer->get_EncFrameDataInfo(AMBASTREAM_VIDEO_ENCSTREAM_2nd, &mFrameInfo[STREAM_VIDEO_2ND]) <= 0){
                    //AM_ERROR("a2");
                    //return ME_ERROR;
                }else{
                    //AM_INFO("x2");
                    mGetData[STREAM_VIDEO_2ND] = 1;
                }
            }
            mStreamer->notify_read_done(AMBASTREAM_VIDEO_ENCSTREAM_2nd);
        }
    }

    ret = mStreamer->is_EncFrameValid(AMBASTREAM_AUDIO_ENCSTREAM);
    if(ret < 0)
    {
        //AM_WARNING("w3");
        mCheckVaild |= (1<<STREAM_AUDIO_1ST);
    }else if(ret > 0){
        if(mStreamer->get_EncFrameData(AMBASTREAM_AUDIO_ENCSTREAM, &mDataPtr[STREAM_AUDIO_1ST], &mDataLen[STREAM_AUDIO_1ST]) > 0){
            if(mStreamer->get_EncFrameDataInfo(AMBASTREAM_AUDIO_ENCSTREAM, &mFrameInfo[STREAM_AUDIO_1ST]) <= 0){
                //AM_ERROR("a3");
                //return ME_ERROR;
            }else{
                //AM_INFO("x3");
                mGetData[STREAM_AUDIO_1ST] = 1;
            }
        }
        mStreamer->notify_read_done(AMBASTREAM_AUDIO_ENCSTREAM);
    }

    if(mbReadDual)
    {
        ret = mStreamer->is_EncFrameValid(AMBASTREAM_AUDIO_ENCSTREAM_2nd);
        if(ret < 0)
        {
            //AM_WARNING("w4");
            mCheckVaild |= (1<<STREAM_AUDIO_2ND);
        }else if(ret > 0){
            if(mStreamer->get_EncFrameData(AMBASTREAM_AUDIO_ENCSTREAM_2nd, &mDataPtr[STREAM_AUDIO_2ND], &mDataLen[STREAM_AUDIO_2ND]) > 0){
                if(mStreamer->get_EncFrameDataInfo(AMBASTREAM_AUDIO_ENCSTREAM_2nd, &mFrameInfo[STREAM_AUDIO_2ND]) <= 0){
                    //AM_ERROR("a4");
                    //return ME_ERROR;
                }else{
                    //AM_INFO("x4");
                    mGetData[STREAM_AUDIO_2ND] = 1;
                }
            }
            mStreamer->notify_read_done(AMBASTREAM_AUDIO_ENCSTREAM_2nd);
        }
    }
#endif

    if((mbReadDual && (mCheckVaild & 0x0f) == 0x0f) ||(!mbReadDual && (mCheckVaild & 0x03) == 0x03))
    {
        //AM_INFO("###############################1");
        return ME_CLOSED;
    }

    return ME_OK;
}

void CVideoEncoderITron::DoStop()
{
    AM_INT ret;
    AM_UINT index = 0;
    AMLOG_INFO("^^^^CVideoEncoderITron DoStop()!\n");
    for(AM_UINT i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        AMLOG_INFO("^^^^Stream (%d) Statistic:\n", i);
        AMLOG_INFO("^^^^TotalDrop Frame=>%d\n", mTotalDrop[i]);
        AMLOG_INFO("^^^^Total IDR Frame Drop=>%d\n", mIDRDrop[i]);
        AMLOG_INFO("^^^^============================^^^^\n");
    }

    //mbStop = true;
    //AMLOG_DEBUG("before IAV_IOC_STOP_ENCODE, mStreamsMask 0x%x, mStreamNum %d.\n", mStreamsMask, mStreamNum);
    //ioctl(mIavFd, IAV_IOC_STOP_ENCODE, mStreamsMask);
    if(mStreamer != NULL)
    {
        AMLOG_INFO("Sotp Encode, disable_AmbaStreamer and notify read done\n");
        mStreamer->stop_encode();
        for (index = 0; index < mMaxStreamIndexPlus1; index ++) {
            if (NULL == mpOutputPin[index]) {
                AM_ASSERT(NULL == mpBSB[index]);
                continue;
            }
            mStreamer->disable_AmbaStreamer(index);
            mStreamer->notify_read_done(index); //in case there is a frame waiting
        }
        AMLOG_INFO("Sotp Encode, before Delete IPC Streamer...\n");
        delete mStreamer;
        AMLOG_INFO("Delete IPC Streamer done.\n");
        mStreamer = NULL;
    }
    AMLOG_INFO("IAV_IOC_STOP_ENCODE done.\n");
}

AM_ERR CVideoEncoderITron::ProcessBuffer()
{
    AM_UINT i;
    AM_ERR err;
    for (i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        //AM_ASSERT(desc->stream_id <mStreamNum);
        if (NULL == mpOutputPin[i]) {
            AM_ASSERT(NULL == mpBSB[i]);
            continue;
        }
        //safe check, todo
        if (mGetData[i] == 0)
        {
            continue;
        }

        if(mbBlock[i] == true)
        {
            continue;
        }
        if(mbSkip[i] == true)
        {
            if(IsIDRFrame(i) != ME_OK)
            {
                AMLOG_DEBUG("^^^^ProcessBuffer: %d, Drop Not IDR Frame!\n", i);
                mTotalDrop[i]++;
                continue;
            }else{
                mbSkip[i] = false;
            }
        }
        AM_ASSERT(mpBSB[i]->GetFreeBufferCnt() > 0);
        if (!mpOutputPin[i]->AllocBuffer(mpBuffer))
        {
            AM_ASSERT(0);
            return ME_ERROR;
        }
        err = GenerateOutBuffer(i);
        if(err == ME_OK)
        {
            mpOutputPin[i]->SendBuffer(mpBuffer);
        }
        mpBuffer = NULL;
    }
    return ME_OK;
}

//unsigned int bit_values; /* pic_type(3)|level_idc(3)|ref_idc(1)|pic_struct(1)|pic_size(24) */
AM_ERR CVideoEncoderITron::GenerateOutBuffer(AM_UINT index)
{
    AM_UINT len_back, len_front;
    AM_UINT size;
    AM_U8* ptr;
    AmbaStream_frameinfo_t* pInfo = &mFrameInfo[index];
    size = (pInfo->bit_values) & 0x00ffffff;
    AM_ASSERT(size == mDataLen[index]);

    //mpBuffer->mReserved = index;
    mpBuffer->SetType(CBuffer::DATA);
    mpBuffer->mFlags = 0;
    mpBuffer->mSeqNum = pInfo->frame_num;
    mpBuffer->mpData = mDataPtr[index];
    mpBuffer->mDataSize = mDataLen[index];
    mpBuffer->mPTS = (AM_U64)pInfo->pts;

    if((pInfo->start_addr + mDataLen[index]) > pInfo->limit_addr)
    {
        //recycle
        mpBuffer->mpData = new AM_U8[mDataLen[index] + SAVE_ALLOC_MEM_NUM];
        len_back = pInfo->limit_addr - pInfo->start_addr;
        memcpy(mpBuffer->mpData, mDataPtr[index], len_back);
        len_front = mDataLen[index] - len_back;
        ptr = (AM_U8 *)pInfo->base_addr;
        memcpy(mpBuffer->mpData + len_back, ptr, len_front);
        mpBuffer->mFlags |= CBUFFER_ALLOCED;
    }

    mpBuffer->mFrameType = ((mFrameInfo[index].bit_values)>>29) & 0x00000007;

    AMLOG_DEBUG("===>Stream:%d, Get data size:%d, pts:%d, type:%d!\n", index, mpBuffer->mDataSize, pInfo->pts
                        , ((pInfo->bit_values)>>29) & 0x00000007);
/*  for(x = 0; x < 10; x++)
            AM_INFO("%02x", pPacket->data[x]);
            AM_INFO("\n");
*/
    return ME_OK;
}

AM_ERR CVideoEncoderITron::ProcessEOS()
{
    AMLOG_INFO("CVideoEncoderITron ProcessEOS()\n");
    CBuffer *pBuffer;
    for(AM_UINT i = 0; i < mMaxStreamIndexPlus1; i++)
    {
        if (NULL == mpOutputPin[i]) {
            AM_ASSERT(NULL == mpBSB[i]);
            continue;
        }
        if (!mpOutputPin[i]->AllocBuffer(pBuffer))
            return ME_ERROR;
        pBuffer->SetType(CBuffer::EOS);
        AMLOG_INFO("CVideoEncoder send EOS\n");
        mpOutputPin[i]->SendBuffer(pBuffer);
    }
    return ME_OK;
}

//-----------------------------------------------------------------------
//
// CEncoderITronOutput
//
//-----------------------------------------------------------------------
CEncoderITronOutput* CEncoderITronOutput::Create(CFilter *pFilter)
{
    CEncoderITronOutput *result = new CEncoderITronOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

CEncoderITronOutput::CEncoderITronOutput(CFilter *pFilter):
    inherited(pFilter)
{
    mMediaFormat.pMediaType = &GUID_Video;
    mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
    mMediaFormat.pFormatType = &GUID_NULL;
}

AM_ERR CEncoderITronOutput::Construct()
{
    return ME_OK;
}

CEncoderITronOutput::~CEncoderITronOutput()
{
    AMLOG_DESTRUCTOR("~CReadStreamOutput.\n");
}


