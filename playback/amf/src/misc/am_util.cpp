/**
 * am_util.cpp
 *
 * History:
 *	2010/08/10 - [Zhi He] created file
 *
 * Desc: some common used struct/functions
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
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
 #include "string.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

//iav client
#include<pthread.h>
#include "ambas_iav.h"
#include "ambas_event.h"

#include "am_util.h"

//depend on socket.h
#include <sys/socket.h>
#include <arpa/inet.h>
#if PLATFORM_LINUX
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <sys/time.h>

static char g_AMPathes[AM_TotalPathesCount][DAMF_MAX_FILENAME_LEN+1] =
{
#if PLATFORM_LINUX
    "/tmp/mmcblk0p1",
    "/tmp/mmcblk0p1",
    "/usr/local/bin"
#elif PLATFORM_ANDROID
    "/mnt/sdcard",
    "/mnt/sdcard",
    "/mnt/sdcard"
#endif
};

const char GUdecStringDecoderType[EH_DecoderType_Cnt][20] =
{
    "Common ",
    "H264 ",
    "MPEG12 ",
    "MPEG4 HW ",
    "MPEG4 Hybird ",
    "VC-1 ",
    "RV40 Hybird ",
    "JPEG ",
    "SW "
};

const char GUdecStringErrorLevel[EH_ErrorLevel_Last][20] =
{
    "NoError ",
    "Warning ",
    "Recoverable ",
    "Fatal "
};

const char GUdecStringCommonErrorType[EH_CommonErrorType_Last][20] =
{
    "NoError ",
    "HardwareHang ",
    "BufferLeak "
};

TGlobalCfg g_GlobalCfg={NULL, NULL, 1, 1, 0, 0, 1, 0, EH_ErrorLevel_NoError, 0, 0};

//default value, same with log.config
SLogConfig g_ModuleLogConfig[LogModuleListEnd] = {
//global
    {2,0,3},    //LogGlobal = 0
//playback related
    {2,0,3},    //LogModulePBEngine = 1,
    {2,0,3},    //LogModuleFFMpegDemuxer,
    {2,0,3},    //LogModuleAmbaVideoDecoder,
    {2,0,3},    //LogModuleVideoRenderer,
    {2,0,3},    //LogModuleFFMpegDecoder,
    {2,0,3},    //LogModuleAudioEffecter,
    {2,0,3},    //LogModuleAudioRenderer,
    {2,0,3},    //LogModuleGeneralDecoder,
    {2,0,3},    //LogModuleVideoDecoderDSP,
    {2,0,3},    //LogModuleVideoDecoderFFMpeg,
    {2,0,3},    //LogModuleDSPHandler,
    {2,0,3},    //LogModuleAudioDecoderHW,
    {2,0,3},    //LogModuleAudioOutputALSA,
    {2,0,3},    //LogModuleAudioOutputAndroid,
//record related, todo
    {2,0,3},    //LogModuleRecordEngine,
    {2,0,3},    //LogModuleVideoEncoder,
    {2,0,3},    //LogModuleAudioInputFilter,
    {2,0,3},    //LogModuleAudioEncoder,
    {2,0,3},    //LogModuleFFMpegEncoder,
    {2,0,3},    //LogModuleFFMpegMuxer,
    {2,0,3},    //LogModuleItronStreammer,
//editing related, todo
    {2,0,3},    //LogModuleVEEngine,
    {2,0,3},    //LogModuleVideoEffecter,
    {2,0,3},    //LogModuleVideoTranscoder,
    {2,0,3},    //LogModuleVideoMemEncoder,
//privatedata related
    {2,0,3},    //LogModulePridataComposer,
    {2,0,3},    //LogModulePridataParser,
//streaming related
    {2,0,3},    //LogModuleStreamingServerManager,
    {2,0,3},    //LogModuleRTPTransmiter,
//duplex related
    {2,0,3},    //LogModuleAmbaVideoSink,
};

static DECSETTING gs_Default_DecSet[CODEC_NUM] = {
    {SCODEC_MPEG12, DEC_DEFAULT},
    {SCODEC_MPEG4, DEC_DEFAULT},
    {SCODEC_H264, DEC_DEFAULT},
    {SCODEC_VC1, DEC_DEFAULT},
    {SCODEC_RV40, DEC_SPE_DEFAULT},
    {SCODEC_NOT_STANDARD_MPEG4, DEC_SPE_DEFAULT},
    {SCODEC_OTHER, DEC_OTHER_DEFAULT}
};

static AM_UINT gs_Default_MdecCap[CODEC_NUM] = {
    DEC_HARDWARE | DEC_SOFTWARE,
    DEC_HARDWARE | DEC_SOFTWARE | DEC_HYBRID,
    DEC_HARDWARE | DEC_SOFTWARE,
    DEC_HARDWARE | DEC_SOFTWARE,
    DEC_SOFTWARE | DEC_HYBRID,
    DEC_SOFTWARE | DEC_HYBRID,
    DEC_SOFTWARE
};

AM_PlayItem* AM_NewPlayItem()
{
    AM_PlayItem* ret = (AM_PlayItem*) malloc(sizeof(AM_PlayItem));
    if (!ret) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: No Memory in AM_NewPlayItem.\n");
        return NULL;
    }
    ret->type = PlayItemType_None;
    ret->pDataSource = NULL;
    ret->stringBufferLen = 0;
    return ret;
}

void AM_DeletePlayItem(AM_PlayItem* des)
{
    if (!des) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: error argument in AM_DeletePlayItem des %p.\n", des);
        return;
    }
    if (des->pDataSource) {
        free(des->pDataSource);
    }
    free(des);
}

AM_INT AM_FillPlayItem(AM_PlayItem* des, char* dataSource, AM_PlayItemType type)
{
    if (!des || !dataSource || type == PlayItemType_None) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: error argument in AM_FillPlayItem des %p, datasource %p, type %d.\n", des, dataSource, type);
        return (-1);
    }

    size_t len = strlen(dataSource);
    if ((len +1) > des->stringBufferLen) {
        //re allocate string buffer
        if (des->pDataSource) {
            AM_ASSERT(des->stringBufferLen);
            free(des->pDataSource);
            des->stringBufferLen = (len + 5)&(~0x3);
            des->pDataSource = (char*) malloc(des->stringBufferLen);
            if (!des->pDataSource) {
                AM_ASSERT(0);
                AM_ERROR("Fatal Error: malloc fail in AM_FillPlayItem.\n");
                return (-2);
            }
        } else {
            des->stringBufferLen = (len + 5)&(~0x3);
            des->pDataSource = (char*) malloc(des->stringBufferLen);
            AM_ASSERT(des->pDataSource);
        }
    } else {
        AM_ASSERT(des->pDataSource);
    }
    des->type = type;
    //strncpy(des->pDataSource, dataSource, des->stringBufferLen);
    memcpy(des->pDataSource,dataSource,len);
    des->pDataSource[len] = '\0';
    return 0;
}

AM_INT AM_InsertPlayItem(AM_PlayItem* head, AM_PlayItem* item, AM_INT isAfter)
{
    if (!head || !item) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: error argument in AM_InsertPlayItem head %p, item %p.\n", head, item);
        return (-1);
    }

    //safe check
    AM_ASSERT(head->pNext && head->pPre);
    AM_ASSERT(!item->pNext && !item->pPre);

    if (isAfter) {
        item->pNext = head->pNext;
        item->pPre = head;
        head->pNext->pPre = item;
        head->pNext = item;
    } else {
        item->pNext = head;
        item->pPre = head->pPre;
        head->pPre->pNext = item;
        head->pPre = item;
    }
    return 0;
}

AM_INT AM_RemovePlayItem(AM_PlayItem* item)
{
    if (!item || !item->pNext|| !item->pPre) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: error argument in AM_RemovePlayItem item %p.\n", item);
        return (-1);
    }
    AM_ASSERT(item->pNext && item->pPre);
    item->pNext->pPre = item->pPre;
    item->pPre->pNext = item->pNext;
    item->pPre = item->pNext = NULL;
    return 0;
}

AM_INT AM_CheckItemInList(AM_PlayItem* header, AM_PlayItem* item)
{
    if (!header || !item || !item->pNext|| !item->pPre || !header->pPre || !header->pNext) {
        AM_ASSERT(0);
        AM_ERROR("Fatal Error: error argument in AM_CheckInList item %p, header %p.\n", item, header);
        return (-1);
    }

    AM_PlayItem* tmp = header->pNext;
    AM_INT maxcnt = 5000;
    while (tmp != header && maxcnt > 0) {
        if (tmp == item) {
            //find it
            return 0;
        }
        tmp = tmp->pNext;
        maxcnt --;
    }

    if (maxcnt > 0) {
        //cannot find it, have bugs?
        AM_ASSERT(0);
        AM_ERROR("cannot find the item, must have bugs here.\n");
        return (-2);
    } else {
        AM_PRINTF("list longer than 5000? maybe something wrong here.\n");
        return 0;
    }
}

static AM_UINT check_decodertype_value(AM_UINT input, AM_UINT mask, AM_UINT defaultv)
{
    AM_UINT shift = 0;
    for (shift = 0; shift < 32; shift ++) {
        if (mask & (1<<shift)) {
            if (input&(1<<shift)) {
                AM_ASSERT(input == ((AM_UINT)(1<<shift)));
                return (1<<shift);
            }
        }
    }
    return defaultv;
}

CDoubleLinkedList::CDoubleLinkedList()
    :mNumberOfNodes(0),
    mpFreeList(NULL)
{
    mHead.p_context = NULL;
    mHead.p_next = mHead.p_pre = &mHead;
}

CDoubleLinkedList::~CDoubleLinkedList()
{
    SNodePrivate* node = mHead.p_next, *tmp;
    while (node != &mHead) {
        AM_ASSERT(node);
        if (node) {
            tmp = node;
            node = tmp->p_next;
            free(tmp);
        } else {
            AM_ERROR("cyclic list broken.\n");
            break;
        }
    }

    node = mpFreeList;
    while (node) {
        tmp = node;
        node = tmp->p_next;
        free(tmp);
    }
}

CDoubleLinkedList::SNode* CDoubleLinkedList::InsertContent(SNode* target_node, void* content, AM_UINT after)
{
    SNodePrivate* new_node = NULL;
    SNodePrivate* tmp_node = NULL;
    AM_ASSERT(content);
    if (NULL == content) {
        AM_ERROR(" NULL content in CDoubleLinkedList::InsertContent.\n");
        return NULL;
    }

    allocNode(new_node);
    if (NULL == new_node) {
        AM_ERROR(" allocNode fail in CDoubleLinkedList::InsertContent, must have error.\n");
        return NULL;
    }

    AM_ASSERT(false == IsContentInList(content));
    new_node->p_context = content;

    if (NULL == target_node) {
        AM_ASSERT(mHead.p_next);
        AM_ASSERT(mHead.p_pre);
        AM_ASSERT(mHead.p_next->p_pre = &mHead);
        AM_ASSERT(mHead.p_pre->p_next = &mHead);
        //head
        if (after == 0) {
            //insert before
            new_node->p_next = &mHead;
            new_node->p_pre = mHead.p_pre;

            mHead.p_pre->p_next = new_node;
            mHead.p_pre = new_node;
        } else {
            //insert after
            new_node->p_pre = &mHead;
            new_node->p_next = mHead.p_next;

            mHead.p_next->p_pre = new_node;
            mHead.p_next = new_node;
        }
    } else {
        tmp_node = (SNodePrivate*)target_node;
        AM_ASSERT(tmp_node->p_next);
        AM_ASSERT(tmp_node->p_pre);
        AM_ASSERT(tmp_node->p_next->p_pre = tmp_node);
        AM_ASSERT(tmp_node->p_pre->p_next = tmp_node);

        if (after == 0) {
            //insert before
            new_node->p_next = tmp_node;
            new_node->p_pre = tmp_node->p_pre;

            tmp_node->p_pre->p_next = new_node;
            tmp_node->p_pre = new_node;
        } else {
            //insert after
            new_node->p_pre = tmp_node;
            new_node->p_next = tmp_node->p_next;

            new_node->p_next->p_pre = new_node;
            new_node->p_next = new_node;
        }
    }
    mNumberOfNodes ++;
    return (CDoubleLinkedList::SNode*) new_node;
}

void CDoubleLinkedList::RemoveContent(void* content)
{
    SNodePrivate* tmp_node = NULL;
    SNodePrivate* tobe_removed;
    AM_ASSERT(content);
    if (NULL == content) {
        AM_ERROR(" NULL content in CDoubleLinkedList::RemoveContent.\n");
        return;
    }

    tmp_node = mHead.p_next;
    if (NULL == tmp_node || tmp_node == &mHead) {
        AM_ERROR("BAD pointer(%p) in CDoubleLinkedList::RemoveContent.\n", tmp_node);
        return;
    }

    while (tmp_node!= &mHead) {
        AM_ASSERT(tmp_node);
        if (NULL == tmp_node) {
            AM_ERROR("!!!cyclic list Broken.\n");
            return;
        }

        if (tmp_node->p_context == content) {
            tobe_removed = tmp_node;
            tobe_removed->p_pre->p_next = tobe_removed->p_next;
            tobe_removed->p_next->p_pre = tobe_removed->p_pre;

            tobe_removed->p_next = mpFreeList;
            mpFreeList = tobe_removed;
            mNumberOfNodes --;
            return;
        }

        tmp_node = tmp_node->p_next;
    }
    return;
}

#if 0
AM_ERR CDoubleLinkedList::RemoveNode(SNode* node)
{
    SNodePrivate* tmp_node = (SNodePrivate*)node;
    if (NULL == tmp_node || tmp_node == &mHead) {
        AM_ERROR("BAD pointer(%p) in CDoubleLinkedList::RemoveNode.\n", node);
        return ME_BAD_PARAM;
    }
    AM_ASSERT(tmp_node->p_next);
    AM_ASSERT(tmp_node->p_pre);
    AM_ASSERT(tmp_node->p_next->p_pre = tmp_node);
    AM_ASSERT(tmp_node->p_pre->p_next = tmp_node);

    tmp_node->p_pre->p_next = tmp_node->p_next;
    tmp_node->p_next->p_pre = tmp_node->p_pre;

    tmp_node->p_next = mpFreeList;
    mpFreeList = tmp_node;

    return ME_OK;
}
#endif

CDoubleLinkedList::SNode* CDoubleLinkedList::FirstNode()
{
    //debug assert
    AM_ASSERT(mHead.p_next);
    AM_ASSERT(mHead.p_pre);
    AM_ASSERT(mHead.p_next->p_pre = &mHead);
    AM_ASSERT(mHead.p_pre->p_next = &mHead);

    if (mHead.p_next == &mHead) {
        //no node in list
        return NULL;
    }

    return (CDoubleLinkedList::SNode*)mHead.p_next;
}

CDoubleLinkedList::SNode* CDoubleLinkedList::LastNode()
{
    //debug assert
    AM_ASSERT(mHead.p_next);
    AM_ASSERT(mHead.p_pre);
    AM_ASSERT(mHead.p_next->p_pre = &mHead);
    AM_ASSERT(mHead.p_pre->p_next = &mHead);

    if (mHead.p_pre == &mHead) {
        //no node in list
        return NULL;
    }

    return (CDoubleLinkedList::SNode*)mHead.p_pre;
}

CDoubleLinkedList::SNode* CDoubleLinkedList::NextNode(SNode* node)
{
    SNodePrivate* tmp_node = (SNodePrivate*)node;
    if (NULL == tmp_node || tmp_node == &mHead) {
        AM_ERROR("BAD pointer(%p) in CDoubleLinkedList::NextNode.\n", node);
        return NULL;
    }

    AM_ASSERT(tmp_node->p_next);
    AM_ASSERT(tmp_node->p_pre);
    AM_ASSERT(tmp_node->p_next->p_pre = tmp_node);
    AM_ASSERT(tmp_node->p_pre->p_next = tmp_node);

    AM_ASSERT(true == IsNodeInList(node));

    if (tmp_node->p_next == &mHead) {
        //last node
        return NULL;
    }

    return (CDoubleLinkedList::SNode*)tmp_node->p_next;
}

CDoubleLinkedList::SNode* CDoubleLinkedList::PreNode(SNode* node)
{
    SNodePrivate* tmp_node = (SNodePrivate*)node;
    if (NULL == tmp_node || tmp_node == &mHead) {
        AM_ERROR("BAD pointer(%p) in CDoubleLinkedList::PreNode.\n", tmp_node);
        return NULL;
    }

    AM_ASSERT(tmp_node->p_next);
    AM_ASSERT(tmp_node->p_pre);
    AM_ASSERT(tmp_node->p_next->p_pre = tmp_node);
    AM_ASSERT(tmp_node->p_pre->p_next = tmp_node);

    AM_ASSERT(true == IsNodeInList(node));

    if (tmp_node->p_pre == &mHead) {
        //first node
        return NULL;
    }

    return (CDoubleLinkedList::SNode*)tmp_node->p_pre;
}

bool CDoubleLinkedList::IsContentInList(void* content)
{
    SNodePrivate* tmp_node = mHead.p_next;
    if (NULL == tmp_node) {
        AM_ERROR("NULL header in CDoubleLinkedList::IsContentInList.\n");
        return false;
    }

    while (tmp_node!= &mHead) {
        AM_ASSERT(tmp_node);
        if (NULL == tmp_node) {
            AM_ERROR("!!!cyclic list Broken.\n");
            return false;
        }

        if (tmp_node->p_context == content) {
            return true;
        }
        tmp_node = tmp_node->p_next;
    }
    return false;
}

bool CDoubleLinkedList::IsNodeInList(SNode* node)
{
    SNodePrivate* tmp_node = mHead.p_next;
    if (NULL == tmp_node || tmp_node == &mHead) {
        AM_ERROR("BAD pointer(%p) in CDoubleLinkedList::IsNodeInList.\n", node);
        return false;
    }

    while (tmp_node!= &mHead) {
        AM_ASSERT(tmp_node);
        if (NULL == tmp_node) {
            AM_ERROR("!!!cyclic list Broken.\n");
            return false;
        }

        if ((SNode*)tmp_node == node) {
            return true;
        }

        tmp_node = tmp_node->p_next;
    }
    return false;
}

void CDoubleLinkedList::allocNode(SNodePrivate* &pnode)
{
    if (mpFreeList) {
        pnode = mpFreeList;
        mpFreeList = mpFreeList->p_next;
    } else {
        pnode = (SNodePrivate*)malloc(sizeof(SNodePrivate));
    }
}

void AM_DefaultPBDspConfig(SConsistentConfig* mpShared)
{
    AM_UINT i = 0;
    AM_ASSERT(mpShared);
    if (!mpShared) {
        AM_ERROR("NULL pointer in AM_DefaultPBDspConfig.\n");
        return;
    }
    DSPConfig* mpConfig = &mpShared->dspConfig;

    //mode config
    mpConfig->modeConfig.postp_mode = mpShared->ppmode;
    mpConfig->modeConfig.enable_deint = 0;
    mpConfig->modeConfig.pp_chroma_fmt_max = 2;
    mpConfig->modeConfig.vout_mask = mpShared->pbConfig.vout_config;
    mpConfig->modeConfig.num_udecs = 1;

    //de-interlacing config
    mpConfig->enableDeinterlace = 1;

    mpConfig->deinterlaceConfig.init_tff = 1;
    mpConfig->deinterlaceConfig.deint_lu_en = 1;
    mpConfig->deinterlaceConfig.deint_ch_en = 1;
    mpConfig->deinterlaceConfig.osd_en = 0;
    mpConfig->deinterlaceConfig.deint_mode = 0;
    mpConfig->deinterlaceConfig.deint_spatial_shift = 2;
    mpConfig->deinterlaceConfig.deint_lowpass_shift = 5;

    mpConfig->deinterlaceConfig.deint_lowpass_center_weight = 16;
    mpConfig->deinterlaceConfig.deint_lowpass_hor_weight = 2;
    mpConfig->deinterlaceConfig.deint_lowpass_ver_weight = 4;

    mpConfig->deinterlaceConfig.deint_gradient_bias = 15;
    mpConfig->deinterlaceConfig.deint_predict_bias = 15;
    mpConfig->deinterlaceConfig.deint_candidate_bias = 10;

    mpConfig->deinterlaceConfig.deint_spatial_score_bias = -5;
    mpConfig->deinterlaceConfig.deint_temporal_score_bias = 5;

    //for each udec instance
    for(i=0; i< DMAX_UDEC_INSTANCE_NUM; i++) {
        //udec config
        mpConfig->udecInstanceConfig[i].tiled_mode = 0;
        mpConfig->udecInstanceConfig[i].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_420;

        mpConfig->udecInstanceConfig[i].max_frm_num = 20;
        mpConfig->udecInstanceConfig[i].max_frm_width = 1920;
        mpConfig->udecInstanceConfig[i].max_frm_height = 1088;
        mpConfig->udecInstanceConfig[i].max_fifo_size = 4*1024*1024;

        //error handling
        mpConfig->errorHandlingConfig[i].enable_udec_error_handling = 1;
        mpConfig->errorHandlingConfig[i].error_concealment_mode = 0;
        mpConfig->errorHandlingConfig[i].error_concealment_frame_id = 0;
        mpConfig->errorHandlingConfig[i].error_handling_app_behavior = 0;
    }

    //deblocking config
    mpConfig->deblockingFlag = 0;
    mpConfig->deblockingConfig.pquant_mode = 1;
    //default table value
    for (i = 0; i<32; i++) {
        mpConfig->deblockingConfig.pquant_table[i] = i;
    }

    //vout config
    mpConfig->voutConfigs.vout_mask =  (1<<eVoutLCD) | (1<<eVoutHDMI);
    mpConfig->voutConfigs.num_vouts = 2;

    mpConfig->addVideoDataType = eAddVideoDataType_iOneUDEC;
}

void AM_DefaultPBConfig(SConsistentConfig* mpShared)
{
//    AM_UINT i = 0;
    AM_ASSERT(mpShared);
    if (!mpShared) {
        AM_ERROR("NULL pointer in AM_DefaultPBConfig.\n");
        return;
    }
    //memset((void*)mpShared, 0, sizeof(SharedResource));

    //debug
    mpShared->mbIavInited = 0;
    mpShared->mIavFd = -1;//init value

    //default settings
    mpShared->pbConfig.audio_disable = 0;
    mpShared->pbConfig.video_disable = 0;
    mpShared->pbConfig.subtitle_disable = 1;
    mpShared->pbConfig.avsync_enable = 1;
    mpShared->pbConfig.vout_config= (1<<eVoutLCD) | (1<<eVoutHDMI);
    //mShared.pbConfig.vout_config= (1<<eVoutHDMI);
    mpShared->pbConfig.ar_enable= 1;
    mpShared->pbConfig.quickAVSync_enable= 1;
    mpShared->ppmode = 2;//default playback ppmode
    mpShared->pbConfig.not_config_audio_path = 0;
    mpShared->pbConfig.not_config_video_path = 0;
    mpShared->pbConfig.auto_vout_enable = 0;
    mpShared->get_outpic = 0;
    mpShared->pbConfig.input_buffer_number = 4;//default playback ppmode

    mpShared->mbAlreadySynced = 0;
    mpShared->mbStartWithStepMode = 0;
    mpShared->mStepCnt = 0;
    mpShared->mDecoderSelect = 0;
    mpShared->mGeneralDecoder = 0;
    mpShared->mDebugFlag = 0;
    mpShared->mForceLowdelay = 0;
    mpShared->mPridataParsingMethod = ePriDataParse_filter_parse;//default

    //default value
    mpShared->clockTimebaseDen = TICK_PER_SECOND;
    mpShared->clockTimebaseNum = 1;

    mpShared->videoEnabled = true;
    mpShared->audioEnabled = true;
    mpShared->mbVideoFirstPTSVaild = false;
    mpShared->mbAudioFirstPTSVaild = false;
    mpShared->videoFirstPTS = 0;
    mpShared->audioFirstPTS = 0;

    mpShared->mbVideoStreamStartTimeValid = false;
    mpShared->mbAudioStreamStartTimeValid = false;
    mpShared->videoStreamStartPTS = 0;
    mpShared->audioStreamStartPTS = 0;

    mpShared->mbDurationEstimated = AM_FALSE;
    mpShared->mVideoTicks = 0;
    mpShared->mAudioTicks = 0;
    mpShared->mbSeekFailed = 0;
    mpShared->mbLoopPlay = 0;

    memcpy(mpShared->decSet, gs_Default_DecSet, sizeof(gs_Default_DecSet));

    mpShared->decCurrType = DEC_OTHER_DEFAULT;
    mpShared->is_avi_flag = 0;

    memcpy(mpShared->mdecCap, gs_Default_MdecCap, sizeof(gs_Default_MdecCap));

    mpShared->mDSPmode = 0;//default playback, dsp_ione use this to set dsp
    mpShared->udecNums = 0;

    mpShared->force_decode = 0;
    mpShared->validation_only = 0;
    mpShared->mReConfigMode = 1;

    mpShared->auto_purge_buffers_before_play_rtsp = 0;

    mpShared->use_force_play_item = 0;
    strncpy(mpShared->force_play_url, "rtsp://10.0.0.2/stream_0", DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN);

    //feature constrains related
    mpShared->codec_mask = 0x3f;//all format
    mpShared->enable_feature_constrains = 0;
    mpShared->always_disable_l2_cache = 0;
    mpShared->max_frm_number= 7;
    mpShared->noncachable_buffer= 1;//fix bug#2751,#2752
    mpShared->h264_no_fmo = 0;
    mpShared->vout_no_LCD = 0;
    mpShared->vout_no_HDMI = 0;
    mpShared->no_deinterlacer = 0;

    //color test
    //black, white, yellow, cyan, green, magenta, red, blue
    mpShared->enable_color_test = 0;
    mpShared->color_test_number = 8;
    mpShared->color_ycbcralpha[0] = 0xeb8080ff;
    mpShared->color_ycbcralpha[1] = 0x108080ff;
    mpShared->color_ycbcralpha[2] = 0xd21092ff;
    mpShared->color_ycbcralpha[3] = 0xaaa610ff;
    mpShared->color_ycbcralpha[4] = 0x913622ff;
    mpShared->color_ycbcralpha[5] = 0x6acadeff;
    mpShared->color_ycbcralpha[6] = 0x515af0ff;
    mpShared->color_ycbcralpha[7] = 0x29f06eff;

    memset(&(mpShared->sTranscConfig), 0, sizeof(TranscConfig));
}

void AM_PrintPBDspConfig(SConsistentConfig* mpShared)
{
    AM_UINT i = 0;
    AM_ASSERT(mpShared);
    if (!mpShared) {
        AM_ERROR("NULL pointer in AM_DefaultPBDspConfig.\n");
        return;
    }
    DSPConfig* mpConfig = &mpShared->dspConfig;

    AM_PRINTF("Mode Config:\n");
    AM_PRINTF("  postp_mode %d, enable_deint %d, pp_chroma_fmt_max %d.\n", mpConfig->modeConfig.postp_mode, mpConfig->modeConfig.enable_deint, mpConfig->modeConfig.pp_chroma_fmt_max);
    AM_PRINTF("  vout_mask 0x%x, num_udecs %d.\n", mpConfig->modeConfig.vout_mask, mpConfig->modeConfig.num_udecs);

    AM_PRINTF("De-interlacing Config, enable %d:\n", mpConfig->enableDeinterlace);
    AM_PRINTF("  init_tff %d, deint_lu_en %d, deint_ch_en %d:\n", mpConfig->deinterlaceConfig.init_tff, mpConfig->deinterlaceConfig.deint_lu_en, mpConfig->deinterlaceConfig.deint_ch_en);
    AM_PRINTF("  osd_en %d, deint_mode %d, deint_spatial_shift %d, deint_lowpass_shift %d:\n", mpConfig->deinterlaceConfig.osd_en, mpConfig->deinterlaceConfig.deint_mode, mpConfig->deinterlaceConfig.deint_spatial_shift, mpConfig->deinterlaceConfig.deint_lowpass_shift);
    AM_PRINTF("  deint_lowpass_center_weight %d, deint_lowpass_hor_weight %d, deint_lowpass_ver_weight %d.\n", mpConfig->deinterlaceConfig.deint_lowpass_center_weight, mpConfig->deinterlaceConfig.deint_lowpass_hor_weight, mpConfig->deinterlaceConfig.deint_lowpass_ver_weight);
    AM_PRINTF("  deint_gradient_bias %d, deint_predict_bias %d, deint_candidate_bias %d.\n", mpConfig->deinterlaceConfig.deint_gradient_bias, mpConfig->deinterlaceConfig.deint_predict_bias, mpConfig->deinterlaceConfig.deint_candidate_bias);
    AM_PRINTF("  deint_spatial_score_bias %d, deint_temporal_score_bias %d.\n", mpConfig->deinterlaceConfig.deint_spatial_score_bias, mpConfig->deinterlaceConfig.deint_temporal_score_bias);

    //for each udec instance
    for(i=0; i< DMAX_UDEC_INSTANCE_NUM; i++) {
        AM_PRINTF("UDEC Config, index %d:\n", i);
        AM_PRINTF("  tiled_mode %d, frm_chroma_fmt_max %d, max_fifo_size %d.\n", mpConfig->udecInstanceConfig[i].tiled_mode, mpConfig->udecInstanceConfig[i].frm_chroma_fmt_max, mpConfig->udecInstanceConfig[i].max_fifo_size);
        AM_PRINTF("  max_frm_num %d, max_frm_width %d, max_frm_height %d.\n", mpConfig->udecInstanceConfig[i].max_frm_num, mpConfig->udecInstanceConfig[i].max_frm_width, mpConfig->udecInstanceConfig[i].max_frm_height);
        AM_PRINTF("  enable_udec_error_handling %d, error_concealment_mode %d, error_concealment_frame_id %d, error_handling_app_behavior %d.\n", \
            mpConfig->errorHandlingConfig[i].enable_udec_error_handling, \
            mpConfig->errorHandlingConfig[i].error_concealment_mode, \
            mpConfig->errorHandlingConfig[i].error_concealment_frame_id, \
            mpConfig->errorHandlingConfig[i].error_handling_app_behavior);
    }

    AM_PRINTF("Deblocking Config, flag %d, pquant_mode %d:\n", mpConfig->deblockingFlag, mpConfig->deblockingConfig.pquant_mode);
    for (i = 0; i<4; i++) {
        AM_PRINTF(" pquant_table[%d - %d]:\n", i*8, i*8+7);
        AM_PRINTF(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
            mpConfig->deblockingConfig.pquant_table[i*8], mpConfig->deblockingConfig.pquant_table[i*8+1], mpConfig->deblockingConfig.pquant_table[i*8+2], mpConfig->deblockingConfig.pquant_table[i*8+3], \
            mpConfig->deblockingConfig.pquant_table[i*8+4], mpConfig->deblockingConfig.pquant_table[i*8+5], mpConfig->deblockingConfig.pquant_table[i*8+6], mpConfig->deblockingConfig.pquant_table[i*8+7] \
        );
    }

    AM_PRINTF("Vout Config, vout_mask 0x%x, num_vouts %d:\n", mpConfig->voutConfigs.vout_mask, mpConfig->voutConfigs.num_vouts);
}

AM_UINT AM_LoadPBConfigFile(const char* fileName, SConsistentConfig* mpShared)
{
    AM_INT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    AM_UINT i;
    char* ptmp = NULL;
    if (!fileName || !mpShared)
        return 0;

    FILE* pFile = fopen(fileName, "rt");
    if (!pFile) {
        fprintf(stderr, "open config file %s fail.\n", fileName);
        return 0;
    }

    char buf[400];

    //skip first line--MediaRecorder select
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);

    //skip filter select line
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);

    //basic config
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (12!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &mpShared->pbConfig.audio_disable, &mpShared->pbConfig.video_disable, &mpShared->pbConfig.subtitle_disable, &mpShared->pbConfig.avsync_enable, &mpShared->pbConfig.vout_config, &mpShared->ppmode,&mpShared->mReConfigMode,&mpShared->pbConfig.ar_enable,&mpShared->pbConfig.quickAVSync_enable, &mpShared->pbConfig.not_config_audio_path, &mpShared->pbConfig.not_config_video_path, &mpShared->pbConfig.auto_vout_enable)) {
        AM_ERROR("pb.config error(audio_disable, video_disable, subtitle_disable, avsync_enable, vout_config, ppmode, ReConfigMode, ar_enable, quickAVSync_enable, not config audio, not config video, auto_vout_enable).\n");
        mpShared->pbConfig.audio_disable = 0;
        mpShared->pbConfig.video_disable = 0;
        mpShared->pbConfig.subtitle_disable = 0;
        mpShared->pbConfig.avsync_enable = 1;
        mpShared->pbConfig.vout_config = (1<<eVoutLCD) | (1<<eVoutHDMI);
        mpShared->ppmode = 2;
        mpShared->mReConfigMode = 1;
        mpShared->pbConfig.ar_enable = 1;
        mpShared->pbConfig.quickAVSync_enable = 1;
        mpShared->pbConfig.not_config_audio_path = 0;
        mpShared->pbConfig.not_config_video_path = 0;
        mpShared->pbConfig.auto_vout_enable = 0;
    }

    //dec-type-selection
    //mpeg12,mpeg4,h264,vc-1,rv40,non-stardard-mpeg4,others
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (CODEC_NUM!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d", &mpShared->decSet[SCODEC_MPEG12].dectype, &mpShared->decSet[SCODEC_MPEG4].dectype, &mpShared->decSet[SCODEC_H264].dectype, &mpShared->decSet[SCODEC_VC1].dectype, &mpShared->decSet[SCODEC_RV40].dectype, &mpShared->decSet[SCODEC_NOT_STANDARD_MPEG4].dectype, &mpShared->decSet[SCODEC_OTHER].dectype)) {
        AM_ERROR("pb.config error(dec type).\n");
        return 0;
    }
    mpShared->decSet[SCODEC_MPEG12].dectype = check_decodertype_value(mpShared->decSet[SCODEC_MPEG12].dectype, DEC_HARDWARE|DEC_SOFTWARE, DEC_HARDWARE);
    mpShared->decSet[SCODEC_MPEG4].dectype = check_decodertype_value(mpShared->decSet[SCODEC_MPEG4].dectype, DEC_HARDWARE|DEC_SOFTWARE|DEC_HYBRID, DEC_HARDWARE);
    mpShared->decSet[SCODEC_H264].dectype = check_decodertype_value(mpShared->decSet[SCODEC_H264].dectype, DEC_HARDWARE|DEC_SOFTWARE, DEC_HARDWARE);
    mpShared->decSet[SCODEC_VC1].dectype = check_decodertype_value(mpShared->decSet[SCODEC_VC1].dectype, DEC_HARDWARE|DEC_SOFTWARE, DEC_HARDWARE);
    mpShared->decSet[SCODEC_RV40].dectype = check_decodertype_value(mpShared->decSet[SCODEC_RV40].dectype, DEC_HYBRID|DEC_SOFTWARE, DEC_HYBRID);
    mpShared->decSet[SCODEC_NOT_STANDARD_MPEG4].dectype = check_decodertype_value(mpShared->decSet[SCODEC_NOT_STANDARD_MPEG4].dectype, DEC_HYBRID|DEC_SOFTWARE, DEC_HYBRID);//if want to dec hy in sw without pipeline, you can add hack "4|" in (AM_UINT mask)
    mpShared->decSet[SCODEC_OTHER].dectype = check_decodertype_value(mpShared->decSet[SCODEC_OTHER].dectype, DEC_SOFTWARE, DEC_SOFTWARE);

    //de-interlace related(enable, parameters)
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (6!=sscanf(buf, "%d,%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5)) {
        AM_ERROR("pb.config error(de-interlace mode).\n");
        return 0;
    } else {
        mpShared->dspConfig.enableDeinterlace = tmp0;
        mpShared->dspConfig.deinterlaceConfig.init_tff = (AM_U8)tmp1;
        mpShared->dspConfig.deinterlaceConfig.deint_lu_en = (AM_U8)tmp2;
        mpShared->dspConfig.deinterlaceConfig.deint_ch_en = (AM_U8)tmp3;
        mpShared->dspConfig.deinterlaceConfig.osd_en = (AM_U8)tmp4;
        mpShared->dspConfig.deinterlaceConfig.deint_mode = (AM_U8)tmp5;
    }

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (5!=sscanf(buf, "%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4)) {
        AM_ERROR("pb.config error(de-interlace parameters set 1).\n");
        return 0;
    } else {
        mpShared->dspConfig.deinterlaceConfig.deint_spatial_shift = (AM_U8)tmp0;
        mpShared->dspConfig.deinterlaceConfig.deint_lowpass_shift = (AM_U8)tmp1;
        mpShared->dspConfig.deinterlaceConfig.deint_lowpass_center_weight = (AM_U8)tmp2;
        mpShared->dspConfig.deinterlaceConfig.deint_lowpass_hor_weight = (AM_U8)tmp3;
        mpShared->dspConfig.deinterlaceConfig.deint_lowpass_ver_weight = (AM_U8)tmp4;
    }

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (5!=sscanf(buf, "%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4)) {
        AM_ERROR("pb.config error(de-interlace parameters set 2).\n");
        return 0;
    } else {
        mpShared->dspConfig.deinterlaceConfig.deint_gradient_bias = (AM_U8)tmp0;
        mpShared->dspConfig.deinterlaceConfig.deint_predict_bias = (AM_U8)tmp1;
        mpShared->dspConfig.deinterlaceConfig.deint_candidate_bias = (AM_U8)tmp2;
        mpShared->dspConfig.deinterlaceConfig.deint_spatial_score_bias = (AM_S16)tmp3;
        mpShared->dspConfig.deinterlaceConfig.deint_temporal_score_bias = (AM_S16)tmp4;
    }

    //error handling/concealment related
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("pb.config error(error handling/concealment config).\n");
        return 0;
    } else {
        mpShared->dspConfig.errorHandlingConfig[0].enable_udec_error_handling = mpShared->dspConfig.errorHandlingConfig[1].enable_udec_error_handling = (AM_U16)tmp0;
        mpShared->dspConfig.errorHandlingConfig[0].error_concealment_mode = mpShared->dspConfig.errorHandlingConfig[1].error_concealment_mode = (AM_U16)tmp1;
        mpShared->dspConfig.errorHandlingConfig[0].error_concealment_frame_id = mpShared->dspConfig.errorHandlingConfig[1].error_concealment_frame_id = (AM_U16)tmp2;
        mpShared->dspConfig.errorHandlingConfig[0].error_handling_app_behavior = mpShared->dspConfig.errorHandlingConfig[1].error_handling_app_behavior = (AM_U16)tmp3;
    }

    //debug related
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (8!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7)) {
        AM_ERROR("pb.config error(debug related config).\n");
        return 0;
    } else {
        if (tmp0>5 || tmp0<1) {
            tmp0 = 5;//default value
        }
        if (tmp1>2) {
            tmp1 = 0;//default value
        }
        mpShared->pbConfig.input_buffer_number = tmp0;
        mpShared->mDecoderSelect = tmp1;
        mpShared->force_decode = ((tmp2==1)?1:0);
        mpShared->validation_only = ((tmp3==1)?1:0);
        mpShared->mForceLowdelay = tmp4;
        mpShared->mGeneralDecoder = tmp5;
        mpShared->dspConfig.addVideoDataType = tmp6;
        if (tmp7 < 3) {
            mpShared->mPridataParsingMethod = tmp7;
        } else {
            mpShared->mPridataParsingMethod = ePriDataParse_filter_parse;//default
        }
    }

    //dsp debug related
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (9!=sscanf(buf, "%d,%x,%d,%d,%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8)) {
        AM_ERROR("pb.config error(feature constrains related config).\n");
        return 0;
    } else {
        mpShared->codec_mask = tmp1;
        mpShared->enable_feature_constrains = tmp0;
        mpShared->h264_no_fmo = tmp2;
        mpShared->vout_no_LCD = tmp3;
        mpShared->vout_no_HDMI = tmp4;
        mpShared->no_deinterlacer = tmp5;
        mpShared->always_disable_l2_cache = tmp6;
        mpShared->max_frm_number = tmp7;
        mpShared->noncachable_buffer = tmp8;
        AM_WARNING("feature constrains, codec mask 0x%x, enable %d, h264 no fmo %d, no LCD %d, no HDMI %d, no deinterlacer %d, always disable l2 cache %d, max_frm_number %d, noncachable_buffer %d\n", mpShared->codec_mask, \
            mpShared->enable_feature_constrains, \
            mpShared->h264_no_fmo, \
            mpShared->vout_no_LCD, \
            mpShared->vout_no_HDMI, \
            mpShared->no_deinterlacer, \
            mpShared->always_disable_l2_cache, \
            mpShared->max_frm_number, \
            mpShared->noncachable_buffer);
    }

    //deblock related
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (2!=sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
        AM_ERROR("pb.config error(deblock related config).\n");
        mpShared->dspConfig.deblockingFlag = 0;
    } else {
        mpShared->dspConfig.deblockingFlag = tmp0;
        mpShared->dspConfig.deblockingConfig.pquant_mode = tmp1;
    }

    for(i=0; i<4; i++) {
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), pFile);
        if (8!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7)) {
            AM_ERROR("pb.config error(deblock pquant table).\n");
            return 0;
        }
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8] = tmp0;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 1] = tmp1;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 2] = tmp2;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 3] = tmp3;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 4] = tmp4;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 5] = tmp5;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 6] = tmp6;
        mpShared->dspConfig.deblockingConfig.pquant_table[i*8 + 7] = tmp7;
    }

    //some parameters with string:
    //raw data related configuration
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (strncmp("some parameters with string:", buf, strlen("some parameters with string:"))) {
        AM_ERROR("pb.config error, no \"some parameters with string:\" line.\n");
        return 0;
    }

    //force play item, purge buffers before play rtsp streaming
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (2!=sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
        AM_ERROR("pb.config error(force play item, purge buffers before play rtsp streaming).\n");
        return 0;
    } else {
        mpShared->use_force_play_item = tmp0;
        mpShared->auto_purge_buffers_before_play_rtsp = tmp1;
    }

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (mpShared->use_force_play_item) {
        if ('[' != buf[0] || (NULL == (ptmp = strchr(buf, ']')))) {
            AM_ERROR("pb.config error(force play item, purge buffers before play rtsp streaming).\n");
            return 0;
        } else {
            AM_UINT len = ptmp - buf - 1;
            if (len > (DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN)) {
                len = DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN;
            }
            strncpy(mpShared->force_play_url, buf + 1, len);
            mpShared->force_play_url[len] = 0;
        }
    }

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    if (!strncmp("enable color test", buf, strlen("enable color test"))) {

        //color test:8
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), pFile);
        if (strncmp("color test:", buf, strlen("color test:"))) {
            AM_ERROR("pb.config error, no \"color test:\" line.\n");
            return 0;
        } else {
            if (1 != sscanf(buf, "color test:%d", &tmp0)) {
                AM_ERROR("pb.config error, no color number in \"color test:%%d\" line.\n");
                return 0;
            } else {
                mpShared->color_test_number = tmp0;
                if (tmp0 > 32 || tmp0 < 1) {
                    AM_ERROR("pb.config error, BAD color number in \"color test:%%d\" line.\n");
                    mpShared->color_test_number = 8;
                }
            }
        }

        for (i = 0; i < mpShared->color_test_number; i++) {
            AM_U32 value = 0x0;
            memset(buf, 0, sizeof(buf));
            fgets(buf, sizeof(buf), pFile);

            if (1 != sscanf(buf, "%x", &value)) {
                break;
            }
            mpShared->color_ycbcralpha[i] = value;
        }

    }else {
        AM_WARNING("no \"enable color test\" line.\n");
    }

    fclose(pFile);
    return i;
}

char *AM_GetStringFromContainerType(IParameters::ContainerType type)
{
    switch (type) {
        case IParameters::MuxerContainer_MP4:
            return (char*)"mp4";
        case IParameters::MuxerContainer_3GP:
            return (char*)"3gp";
        case IParameters::MuxerContainer_TS:
            return (char*)"ts";
        case IParameters::MuxerContainer_MOV:
            return (char*)"mov";
        case IParameters::MuxerContainer_AVI:
            return (char*)"avi";
        case IParameters::MuxerContainer_AMR:
            return (char*)"amr";
        case IParameters::MuxerContainer_MKV:
            return (char*)"mkv";
        default:
            AM_ERROR("unknown container type.\n");
            return (char*)"???";
    }
}

typedef struct {
    char format_name[8];
    IParameters::StreamFormat format;
} SFormatStringPair;

typedef struct {
    char entropy_name[8];
    IParameters::EntropyType type;
} SEntropyTypeStringPair;

typedef struct {
    char container_name[8];
    IParameters::ContainerType container_type;
} SContainerStringPair;

typedef struct {
    char sampleformat_name[8];
    IParameters::AudioSampleFMT sampleformat;
} SSampleFormatStringPair;

static SFormatStringPair g_FormatStringPair[] =
{
    {"h264", IParameters::StreamFormat_H264},
    {"avc", IParameters::StreamFormat_H264},
    {"aac", IParameters::StreamFormat_AAC},
    {"mp2", IParameters::StreamFormat_MP2},
    {"mp3", IParameters::StreamFormat_MP2},
    {"ac3", IParameters::StreamFormat_AC3},
    {"adpcm", IParameters::StreamFormat_ADPCM},
    {"amrnb", IParameters::StreamFormat_AMR_NB},
    {"amrwb", IParameters::StreamFormat_AMR_WB},
    {"amr", IParameters::StreamFormat_AMR_NB},
};

static SEntropyTypeStringPair g_EntropyTypeStringPair[] =
{
    {"cabac", IParameters::EntropyType_H264_CABAC},
    {"cavlc", IParameters::EntropyType_H264_CAVLC},
};

static SContainerStringPair g_ContainerStringPair[] =
{
    {"mp4", IParameters::MuxerContainer_MP4},
    {"3gp", IParameters::MuxerContainer_3GP},
    {"ts", IParameters::MuxerContainer_TS},
    {"mov", IParameters::MuxerContainer_MOV},
    {"mkv", IParameters::MuxerContainer_MKV},
    {"avi", IParameters::MuxerContainer_AVI},
    {"amr", IParameters::MuxerContainer_AMR},
    {"rtsp", IParameters::MuxerContainer_RTSP_LiveStreamming},
};

static SSampleFormatStringPair g_SampleFormatStringPair[] =
{
    {"u8", IParameters::SampleFMT_U8},
    {"s16", IParameters::SampleFMT_S16},
    {"s32", IParameters::SampleFMT_S32},
    {"float", IParameters::SampleFMT_FLT},
    {"double", IParameters::SampleFMT_DBL},
};

IParameters::StreamFormat AM_GetFormatFromString (char* str)
{
    AM_UINT i, tot;
    tot = sizeof(g_FormatStringPair)/sizeof(SFormatStringPair);
    for (i=0; i<tot; i++) {
        if (0 == strncmp(str, g_FormatStringPair[i].format_name, strlen(g_FormatStringPair[i].format_name))) {
            return g_FormatStringPair[i].format;
        }
    }
    return IParameters::StreamFormat_Invalid;
}

IParameters::EntropyType AM_GetEntropyTypeFromString (char* str)
{
    AM_UINT i, tot;
    tot = sizeof(g_EntropyTypeStringPair)/sizeof(SEntropyTypeStringPair);
    for (i=0; i<tot; i++) {
        if (0 == strncmp(str, g_EntropyTypeStringPair[i].entropy_name, strlen(g_EntropyTypeStringPair[i].entropy_name))) {
            return g_EntropyTypeStringPair[i].type;
        }
    }
    return IParameters::EntropyType_NOTSet;
}

IParameters::ContainerType AM_GetContainerTypeFromString (char* str)
{
    AM_UINT i, tot;
    tot = sizeof(g_ContainerStringPair)/sizeof(SContainerStringPair);
    for (i=0; i<tot; i++) {
        if (0 == strncmp(str, g_ContainerStringPair[i].container_name, strlen(g_ContainerStringPair[i].container_name))) {
            return g_ContainerStringPair[i].container_type;
        }
    }

    return IParameters::MuxerContainer_Invalid;
}

IParameters::AudioSampleFMT AM_GetSampleFormatFromString (char* str)
{
    AM_UINT i, tot;
    tot = sizeof(g_SampleFormatStringPair)/sizeof(SSampleFormatStringPair);
    for (i=0; i<tot; i++) {
        if (0 == strncmp(str, g_SampleFormatStringPair[i].sampleformat_name, strlen(g_SampleFormatStringPair[i].sampleformat_name))) {
            return g_SampleFormatStringPair[i].sampleformat;
        }
    }
    //default s16
    return IParameters::SampleFMT_S16;
}

bool AM_VideoEncCheckDimention(AM_UINT& width, AM_UINT& height, AM_UINT index) {
    //main stream
    if (index == 0) {
        switch (width) {
            case 1280://hd
                if (height == 720) {
                    return true;
                } else if (height == 1024) {
                    return true;
                }
                break;
            case 1920://full hd
                if (height == 1080 || height == 1088) {
                    return true;
                }
                break;
            case 848://WVGA
                if (height == 480) {
                    return true;
                }
                break;
            case 800:
                if (height == 480) {
                    return true;
                }
                break;
            case 720://D1
                if (height == 480) {
                    return true;
                }
                break;
            case 640://VGA
                if (height == 480) {
                    return true;
                }
                break;
            case 432://WQVGA
                if (height == 240) {
                    return true;
                }
                if(height == 320){//for AmbaDV, the second stream record
                    return true;
                }
                break;
            case 352://SIF or CIF
                if (height == 240 || height == 288) {
                    return true;
                }
                break;
            case 320://for android's default
                if (height == 240) {
                    return true;
                }
                break;
            case 176://QCIF
                if (height == 144) {
                    return true;
                }
                break;
            default:
                break;
        }
        AM_WARNING("video dimention [%d x %d] for main stream is not in support list, use default [%d x %d] as default.\n", width, height, DefaultMainVideoWidth, DefaultMainVideoHeight);
        width = DefaultMainVideoWidth;
        height = DefaultMainVideoHeight;
        return false;
    } else {
        switch (width) {
            case 800:
                if (height == 480) {
                    return true;
                }
                break;
            case 720://D1
                if (height == 480) {
                    return true;
                }
                break;
            case 640://VGA
                if (height == 480) {
                    return true;
                }
                break;
            case 352://CIF
                if (height == 288 || height == 240) {
                    return true;
                }
                break;
            case 432://WQVGA
                if (height == 240) {
                    return true;
                }
                if(height == 320){//for AmbaDV, the second stream record
                    return true;
                }
                break;
            case 320://for android's default
                if (height == 240) {
                    return true;
                }
                break;
            default:
                break;
        }
        AM_WARNING("video dimention [%d x %d] for second stream is not in support list, use default [%d x %d] as default.\n", width, height, DefaultPreviewCWidth, DefaultPreviewCHeight);
        width = DefaultPreviewCWidth;
        height = DefaultPreviewCHeight;
        return false;
    }
}

static inline float _divide(AM_UINT num, AM_UINT den)
{
    if (den == 0) {
        AM_ERROR("den is ZERO when divide.\n");
        return 1;
    }
    return ((float)num)/((float)(den));
}

static inline bool _float_matched(float num1, float num2)
{
    num1 -= num2;
    if (num1 < 0.02 && num1 >(-0.02)) {
        return true;
    }
    return false;
}

void AM_WriteHLSConfigfile(char* config_filename, char* filename_base, AM_UINT start_index, AM_UINT end_index, char* ext, AM_UINT& seq_num, AM_UINT length)
{
    AM_UINT i = 0;
    if (!config_filename || !filename_base ||!ext) {
        AM_ERROR("NULL pointer in AM_WriteHLSConfigfile.\n");
        return;
    }

    FILE* pfile = fopen(config_filename,"wt");
    if (pfile == NULL) {
        AM_ERROR("open config file(%s) fail in AM_WriteHLSConfigfile.\n", config_filename);
        return;
    }

    //write Header
    fprintf(pfile, "#EXTM3U\n");
    fprintf(pfile, "#EXT-X-TARGETDURATION:%d\n", length);

    //clip seq_num to 0-15
    fprintf(pfile, "#EXT-X-MEDIA-SEQUENCE:%d\n", (seq_num++ & 0xffff));

    for (i = start_index; i <= end_index; i++) {
        fprintf(pfile, "#EXTINF:%d,\n", length);
        fprintf(pfile, "%s_%06d.%s\n", filename_base, i, ext);
    }

    //not write end here, for live streamming case

    fclose(pfile);
}

AM_UINT AM_VideoEncCalculateFrameRate(AM_UINT num, AM_UINT den) {
    AM_UINT framerate;
    float fra;
    fra = _divide(num, den);

    if (_float_matched(fra, 29.97)) {
        framerate = IParameters::VideoFrameRate_29dot97;
    } else if (_float_matched(fra, 59.94)) {
        framerate = IParameters::VideoFrameRate_59dot94;
    } else if (_float_matched(fra, 23.96)) {
        framerate = IParameters::VideoFrameRate_23dot96;
    } else if (_float_matched(fra, 240)) {
        framerate = IParameters::VideoFrameRate_240;
    } else if (_float_matched(fra, 60)) {
        framerate = IParameters::VideoFrameRate_60;
    } else if (_float_matched(fra, 30)) {
        framerate = IParameters::VideoFrameRate_30;
    } else if (_float_matched(fra, 24)) {
        framerate = IParameters::VideoFrameRate_24;
    } else if (_float_matched(fra, 15)) {
        framerate = IParameters::VideoFrameRate_15;
    } else {
        framerate = (AM_UINT)fra;
        AM_WARNING("frame rate not in support list? num %d, den %d, fra %f, framerate %d.\n", num, den, fra, framerate);
    }
    AM_INFO("frame rate 0x%x.\n", framerate);
    return framerate;
}

void AM_PrintRecConfig(SConsistentConfig* mpConfig)
{
    AM_ASSERT(mpConfig);
    if (!mpConfig) {
        return;
    }

    AM_UINT i;
    AM_INFO("rec.config:\n");
    AM_INFO("    video_enable %d, audio_enable %d, subtitle_enable %d, private_data_enable %d, private_gps_sub_info_enable %d, private_am_trickplay_enable %d.\n",
            mpConfig->video_enable,
            mpConfig->audio_enable,
            mpConfig->subtitle_enable,
            mpConfig->private_data_enable,
            mpConfig->private_gps_sub_info_enable,
            mpConfig->private_am_trickplay_enable
        );

    AM_INFO("    tot_muxer_number %d.\n", mpConfig->tot_muxer_number);

    for (i=0; i< mpConfig->tot_muxer_number; i++) {

        AM_INFO("    container format %d, video format %d, audio format %d.\n",
            mpConfig->target_recorder_config[i].container_format,
            mpConfig->target_recorder_config[i].video_format,
            mpConfig->target_recorder_config[i].audio_format
        );

        AM_INFO("    video: w %d, h %d, bitrate %d, framerate num %d, den %d, low delay %d.\n",
            mpConfig->target_recorder_config[i].pic_width,
            mpConfig->target_recorder_config[i].pic_height,
            mpConfig->target_recorder_config[i].video_bitrate,
            mpConfig->target_recorder_config[i].video_framerate_num,
            mpConfig->target_recorder_config[i].video_framerate_den,
            mpConfig->target_recorder_config[i].video_lowdelay
        );

        if (mpConfig->target_recorder_config[i].entropy_type == IParameters::EntropyType_H264_CABAC) {
            AM_INFO("   video entropy: cabac.\n");
        } else if (mpConfig->target_recorder_config[i].entropy_type == IParameters::EntropyType_H264_CAVLC) {
            AM_INFO("   video entropy: cavlc.\n");
        } else {
            AM_INFO("   video entropy not set, will use default.\n");
        }

        AM_INFO("    audio: channel_number %d, sample_format %d, sample_rate %d, bitrate %d.\n",
            mpConfig->target_recorder_config[i].channel_number,
            mpConfig->target_recorder_config[i].sample_format,
            mpConfig->target_recorder_config[i].sample_rate,
            mpConfig->target_recorder_config[i].audio_bitrate
        );

    }

    AM_INFO("    debug option: muxer_dump_video %d, muxer_dump_audio %d, muxer_dump_pridata %d, muxer_skip_video %d, muxer_skip_audio %d, muxer_skip_pridata %d, not_check_video_res %d.\n",
        mpConfig->muxer_dump_video,
        mpConfig->muxer_dump_audio,
        mpConfig->muxer_dump_pridata,
        mpConfig->muxer_skip_video,
        mpConfig->muxer_skip_audio,
        mpConfig->muxer_skip_pridata,
        mpConfig->not_check_video_res
    );

}

void AM_DefaultRecConfig(SConsistentConfig* mpConfig)
{
    AM_ASSERT(mpConfig);
    if (!mpConfig) {
        return;
    }

    AM_UINT i;

    //same with default rec.config
    mpConfig->video_enable = 1;
    mpConfig->audio_enable = 1;
    mpConfig->subtitle_enable = 0;
    mpConfig->private_data_enable = 1;
    mpConfig->streaming_enable = 1;
    mpConfig->auto_start_rtsp_steaming = 0;
    mpConfig->streaming_video_enable = 1;
    mpConfig->streaming_audio_enable = 1;
    mpConfig->cutfile_with_precise_pts = 1;
    mpConfig->private_gps_sub_info_enable = 0;
    mpConfig->private_am_trickplay_enable = 0;

    mpConfig->tot_muxer_number = DMAX_VIDEO_ENC_STREAM_NUMBER;
    for (i=0; i< mpConfig->tot_muxer_number; i++) {
        mpConfig->target_recorder_config[i].container_format = IParameters::MuxerContainer_MP4;
        mpConfig->target_recorder_config[i].video_format = IParameters::StreamFormat_H264;
        mpConfig->target_recorder_config[i].audio_format = IParameters::StreamFormat_AAC;
        if (i == 0) {
            mpConfig->target_recorder_config[i].pic_width = DefaultMainVideoWidth;
            mpConfig->target_recorder_config[i].pic_height = DefaultMainVideoHeight;
            mpConfig->target_recorder_config[i].video_bitrate = DefaultMainBitrate;
        } else {
            mpConfig->target_recorder_config[i].pic_width = DefaultPreviewCWidth;
            mpConfig->target_recorder_config[i].pic_height = DefaultPreviewCHeight;
            mpConfig->target_recorder_config[i].video_bitrate = 2000000;
        }
        mpConfig->target_recorder_config[i].channel_number = 2;
        mpConfig->target_recorder_config[i].sample_format = IParameters::SampleFMT_S16;
        mpConfig->target_recorder_config[i].sample_rate = 48000;
        mpConfig->target_recorder_config[i].audio_bitrate = 128000;
        mpConfig->target_recorder_config[i].video_framerate_num = 90000;
        mpConfig->target_recorder_config[i].video_framerate_den = 3003;
        mpConfig->target_recorder_config[i].video_lowdelay = 0;

        mpConfig->target_recorder_config[i].M = DefaultH264M;
        mpConfig->target_recorder_config[i].N = DefaultH264N;
        mpConfig->target_recorder_config[i].IDRInterval = DefaultH264IDRInterval;
        mpConfig->target_recorder_config[i].entropy_type = IParameters::EntropyType_H264_CAVLC;
    }

    //dsp mode config, mainly for duplex mode
    mpConfig->encoding_mode_config.dsp_mode = DSPMode_CameraRecording;//default value
    mpConfig->encoding_mode_config.main_win_width = DefaultMainWindowWidth;
    mpConfig->encoding_mode_config.main_win_height = DefaultMainWindowHeight;
    mpConfig->encoding_mode_config.stream_number = 1;
    mpConfig->encoding_mode_config.enc_width = DefaultMainVideoWidth;
    mpConfig->encoding_mode_config.enc_height = DefaultMainVideoHeight;
    mpConfig->encoding_mode_config.enc_offset_x = DefaultMainVideoOffset_x;
    mpConfig->encoding_mode_config.enc_offset_y = DefaultMainVideoOffset_y;
    mpConfig->encoding_mode_config.second_enc_width = DefaultPreviewCWidth;
    mpConfig->encoding_mode_config.second_enc_height = DefaultPreviewCHeight;
    mpConfig->encoding_mode_config.num_of_dec_chans = 1;
    mpConfig->encoding_mode_config.num_of_enc_chans = 1;
    mpConfig->encoding_mode_config.playback_vout_index = DefaultDuplexPlaybackVoutIndex;
    mpConfig->encoding_mode_config.preview_vout_index = DefaultDuplexPreviewVoutIndex;
    mpConfig->encoding_mode_config.playback_in_pip = 0; //hard code here
    mpConfig->encoding_mode_config.preview_enabled = DefaultDuplexPreviewEnabled;
    mpConfig->encoding_mode_config.pb_display_enabled = DefaultDuplexPlaybackDisplayEnabled;
    mpConfig->encoding_mode_config.preview_in_pip = DefaultDuplexPreviewEnabled;

    mpConfig->encoding_mode_config.video_bitrate = DefaultDuplexMainBitrate;
    mpConfig->encoding_mode_config.M = DefaultDuplexH264M;
    mpConfig->encoding_mode_config.N = DefaultDuplexH264N;
    mpConfig->encoding_mode_config.IDRInterval = DefaultDuplexH264IDRInterval;
    mpConfig->encoding_mode_config.entropy_type = IParameters::EntropyType_H264_CAVLC;

    mpConfig->encoding_mode_config.preview_alpha = DefaultDuplexPreviewAlpha;
    mpConfig->encoding_mode_config.preview_left = DefaultDuplexPreviewLeft;
    mpConfig->encoding_mode_config.preview_top = DefaultDuplexPreviewTop;
    mpConfig->encoding_mode_config.preview_width = DefaultDuplexPreviewWidth;
    mpConfig->encoding_mode_config.preview_height = DefaultDuplexPreviewHeight;

    mpConfig->encoding_mode_config.pb_display_left= DefaultDuplexPbDisplayLeft;
    mpConfig->encoding_mode_config.pb_display_top = DefaultDuplexPbDisplayTop;
    mpConfig->encoding_mode_config.pb_display_width = DefaultDuplexPbDisplayWidth;
    mpConfig->encoding_mode_config.pb_display_height = DefaultDuplexPbDisplayheight;

    //previewC related
    mpConfig->encoding_mode_config.previewc_rawdata_enabled = 1;
    mpConfig->encoding_mode_config.previewc_scaled_width = DefaultPreviewCWidth;
    mpConfig->encoding_mode_config.previewc_scaled_height = DefaultPreviewCHeight;
    //full size of main window
    mpConfig->encoding_mode_config.previewc_crop_offset_x = 0;
    mpConfig->encoding_mode_config.previewc_crop_offset_y = 0;
    mpConfig->encoding_mode_config.previewc_crop_width = mpConfig->encoding_mode_config.main_win_width;
    mpConfig->encoding_mode_config.previewc_crop_height = mpConfig->encoding_mode_config.main_win_height;

    mpConfig->not_start_encoding = 0;
    mpConfig->muxer_dump_audio = 0;
    mpConfig->muxer_dump_video = 0;
    mpConfig->muxer_dump_pridata = 0;
    mpConfig->muxer_skip_audio = 0;
    mpConfig->muxer_skip_video = 0;
    mpConfig->muxer_skip_pridata = 0;
    mpConfig->not_check_video_res = 0;

    mpConfig->use_itron_filter = 1;
    mpConfig->video_from_itron = 1;
    mpConfig->audio_from_itron = 1;
    mpConfig->enable_fade_in = 0;

    mpConfig->encoding_mode_config.thumbnail_enabled = 0;
    mpConfig->encoding_mode_config.thumbnail_width = DefaultThumbnailWidth;
    mpConfig->encoding_mode_config.thumbnail_height = DefaultThumbnailHeight;

    mpConfig->encoding_mode_config.dsp_piv_enabled = 0;
    mpConfig->encoding_mode_config.dsp_jpeg_width = DefaultJpegWidth;
    mpConfig->encoding_mode_config.dsp_jpeg_height = DefaultJpegHeight;

    mpConfig->rtsp_server_config.rtsp_listen_port = DefaultRTSPServerPort;
    mpConfig->rtsp_server_config.rtp_rtcp_port_start = DefaultRTPServerPortBase;

    mpConfig->encoding_mode_config.vcap_ppl_type = 2;//default

    //camera
    mpConfig->select_camera_index = 0;//default
    //
    mpConfig->disable_save_files = 0;
}

AM_UINT AM_LoadRecConfigFile(const char* fileName, SConsistentConfig* mpConfig)
{
    AM_UINT tmp0, tmp1, tmp2, tmp3;//, tmp4, tmp5, tmp6, tmp7
    AM_UINT i;
    char buf[400];
    char* p_str, *p_start;
    FILE* pFile = NULL;
    char* rets = NULL;

#define exit_if_end_of_file do { if (!rets) { goto AM_LoadRecConfigFile_fail; } } while (0)

    if (!fileName || !mpConfig) {
        AM_ERROR("NULL pointer in AM_LoadRecConfigFile.\n");
        goto AM_LoadRecConfigFile_fail;
    }

    pFile = fopen(fileName, "rt");
    if (!pFile) {
        AM_ERROR("AM_LoadRecConfigFile open %s fail.\n", fileName);
        goto AM_LoadRecConfigFile_fail;
    }

    //skip first line--MediaRecorder select
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;

    //skip filter select line
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;

    //basic config
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (7!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d", &mpConfig->video_enable, &mpConfig->audio_enable, &mpConfig->subtitle_enable, &mpConfig->private_data_enable, &mpConfig->private_gps_sub_info_enable, &mpConfig->private_am_trickplay_enable, &mpConfig->tot_muxer_number)) {
        AM_ERROR("rec.config error(video_enable, audio_enable, subtitle_enable, private_data_enable, private_gps_sub_info_enable, private_am_trickplay_enable, tot_muxer_number).\n");
        goto AM_LoadRecConfigFile_fail;
    }

    //streaming config
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (6!=sscanf(buf, "%d,%d,%d,%d,%d,%d", &mpConfig->streaming_enable, &mpConfig->cutfile_with_precise_pts, &mpConfig->not_start_encoding, &mpConfig->streaming_video_enable, &mpConfig->streaming_audio_enable, &tmp0)) {
        AM_ERROR("rec.config error(streaming_enable, cutfile_with_precise_pts, not start encoding, streaming video enable, streaming audio enable, auto start rtsp streaming).\n");
        mpConfig->streaming_enable = 1;
        mpConfig->auto_start_rtsp_steaming = 0;
        mpConfig->cutfile_with_precise_pts = 1;
        mpConfig->not_start_encoding = 0;
        mpConfig->streaming_video_enable = 1;
        mpConfig->streaming_audio_enable = 1;
        mpConfig->auto_start_rtsp_steaming = 0;
        //goto AM_LoadRecConfigFile_fail;
    }

    if (!mpConfig->streaming_video_enable) {
        mpConfig->streaming_video_enable = 1;
        AM_ERROR("video streaming should be always enabled in current code.\n");
    }

    //debug config
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (7!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d", &mpConfig->muxer_dump_video, &mpConfig->muxer_dump_audio, &mpConfig->muxer_dump_pridata, &mpConfig->muxer_skip_video, &mpConfig->muxer_skip_audio, &mpConfig->muxer_skip_pridata, &mpConfig->not_check_video_res)) {
        AM_ERROR("rec.config error(muxer_dump_video, muxer_dump_audio, muxer_dump_pridata, muxer_skip_video, muxer_skip_audio, muxer_skip_pridata, not_check_video_res).\n");
        goto AM_LoadRecConfigFile_fail;
    }

    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (3!=sscanf(buf, "%d,%d,%d", &mpConfig->use_itron_filter, &mpConfig->video_from_itron, &mpConfig->audio_from_itron)) {
        AM_ERROR("rec.config error(use_itron_filter, video_from_itron, audio_from_itron).\n");
        goto AM_LoadRecConfigFile_fail;
    }

    if (DMAX_VIDEO_ENC_STREAM_NUMBER != mpConfig->tot_muxer_number) {
        AM_ERROR("why tot_muxer_number is not %d, %d?\n", DMAX_VIDEO_ENC_STREAM_NUMBER, mpConfig->tot_muxer_number);
        mpConfig->tot_muxer_number = DMAX_VIDEO_ENC_STREAM_NUMBER;
    }

    //for each muxer
    for (i = 0; i < mpConfig->tot_muxer_number; i++) {

        //check first line have stream %d
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        if (1==sscanf(buf, "stream %d", &tmp0)) {
            AM_ASSERT(tmp0 == i);
            if (tmp0 != i) {
                AM_ERROR("stream %d check fail.\n", i);
                goto AM_LoadRecConfigFile_fail;
            }
        } else {
            AM_ERROR("stream %d check fail.\n", i);
            goto AM_LoadRecConfigFile_fail;
        }

        //get each streams' parameters

        //parse container/format string
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        //container string
        p_start = buf;
        p_str = strchr(p_start,',');
        if ((p_str == NULL) || (((AM_UINT)(p_str - p_start)) >8)) {
            AM_ERROR("BAD container string, %s.\n", p_start);
            goto AM_LoadRecConfigFile_fail;
        } else {
            mpConfig->target_recorder_config[i].container_format = AM_GetContainerTypeFromString(p_start);
            if (IParameters::MuxerContainer_Invalid == mpConfig->target_recorder_config[i].container_format) {
                AM_WARNING("not valid container string in rec.config? use .mp4 as default.\n");
                mpConfig->target_recorder_config[i].container_format = IParameters::MuxerContainer_MP4;
            }
        }

        //video format
        p_start = p_str + 1;
        p_str = strchr(p_start,',');
        if ((p_str == NULL) || (((AM_UINT)(p_str - p_start)) >8)) {
            AM_ERROR("BAD video format string, %s.\n", p_start);
            goto AM_LoadRecConfigFile_fail;
        } else {
            mpConfig->target_recorder_config[i].video_format = AM_GetFormatFromString(p_start);
            AM_ASSERT(mpConfig->target_recorder_config[i].video_format == IParameters::StreamFormat_H264);
        }

        //audio format
        p_start = p_str + 1;
        p_str = strchr(p_start,',');
        if ((p_str == NULL) || (((AM_UINT)(p_str - p_start)) >8)) {
            AM_ERROR("BAD audio format string, %s.\n", p_start);
            goto AM_LoadRecConfigFile_fail;
        } else {
            mpConfig->target_recorder_config[i].audio_format = AM_GetFormatFromString(p_start);
            AM_ASSERT(mpConfig->target_recorder_config[i].audio_format != IParameters::StreamFormat_Invalid);
            if (mpConfig->target_recorder_config[i].audio_format == IParameters::StreamFormat_Invalid) {
                AM_ERROR("BAD audio format string %s.\n", p_start);
                goto AM_LoadRecConfigFile_fail;
            }
        }

        //entropy type
        p_start = p_str + 1;
        //p_str = strchr(p_start,',');
        mpConfig->target_recorder_config[i].entropy_type = AM_GetEntropyTypeFromString(p_start);
        AM_ASSERT(mpConfig->target_recorder_config[i].entropy_type != IParameters::EntropyType_NOTSet);

        //video width, height
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        if (2==sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
            mpConfig->target_recorder_config[i].pic_width = tmp0;
            mpConfig->target_recorder_config[i].pic_height = tmp1;
            if (!mpConfig->not_check_video_res) {
                if (false == AM_VideoEncCheckDimention(mpConfig->target_recorder_config[i].pic_width, mpConfig->target_recorder_config[i].pic_height, i)) {
                    AM_WARNING("request video dimention(%dx%d) not supported(index %d).\n", tmp0, tmp1, i);
                    AM_WARNING(" use default dimention(%dx%d).\n", mpConfig->target_recorder_config[i].pic_width, mpConfig->target_recorder_config[i].pic_height);
                }
            } else {
                AM_INFO("not check video dimention [%d x %d].\n", tmp0, tmp1);
            }
        } else {
            AM_ERROR("BAD video width height line.\n");
            goto AM_LoadRecConfigFile_fail;
        }

        //audio channals,sample rate
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        if (2==sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
            //debug assertion
            AM_ASSERT(tmp0 == 2);
            AM_ASSERT(tmp1 == 44100 || tmp1 == 48000);

            mpConfig->target_recorder_config[i].channel_number = tmp0;
            mpConfig->target_recorder_config[i].sample_rate = tmp1;
        } else {
            AM_ERROR("BAD audio channel number,sample rate, sample format line.\n");
            goto AM_LoadRecConfigFile_fail;
        }
        //audio sample format
        p_str = strchr(buf,'[');
        if (!p_str) {
            AM_ERROR("No sample format? like [s16].\n");
            goto AM_LoadRecConfigFile_fail;
        }
        mpConfig->target_recorder_config[i].sample_format = AM_GetSampleFormatFromString(p_str + 1);

        //video/audio bitrate
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        if (2==sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
            mpConfig->target_recorder_config[i].video_bitrate = tmp0;
            mpConfig->target_recorder_config[i].audio_bitrate = tmp1;
        } else {
            AM_ERROR("BAD video/audio bit rate, line.\n");
            goto AM_LoadRecConfigFile_fail;
        }

        //video frame rate, low delay
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;
        if (3==sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
            //need check framerate?
            mpConfig->target_recorder_config[i].video_framerate_num = tmp0;
            mpConfig->target_recorder_config[i].video_framerate_den = tmp1;
            mpConfig->target_recorder_config[i].video_lowdelay = tmp2;
        } else {
            AM_ERROR("BAD video/audio bit rate, line.\n");
            goto AM_LoadRecConfigFile_fail;
        }

        //H264 M, N, IDR Interval
        memset(buf, 0, sizeof(buf));
        rets = fgets(buf, sizeof(buf), pFile);
        exit_if_end_of_file;

        if (3==sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
            //need check M, N, IDR Interval?
            mpConfig->target_recorder_config[i].M = tmp0;
            mpConfig->target_recorder_config[i].N = tmp1;
            mpConfig->target_recorder_config[i].IDRInterval = tmp2;
        } else {
            AM_ERROR("BAD video/audio bit rate, line.\n");
            goto AM_LoadRecConfigFile_fail;
        }

    }

    //duplex init configuration
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (strncmp("duplex init config", buf, strlen("duplex init config"))) {
        AM_ERROR("rec.config error, no \"duplex init config\" line, %s.\n", buf);
        return i;
    }

    //dsp mode, main window dimension, total encoding stream number
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(dsp mode, main window width, height, total enc stream number).\n");
        return i;
    } else {
        if (tmp0 != DSPMode_CameraRecording && tmp0 != DSPMode_DuplexLowdelay) {
            AM_ERROR("BAD dsp mode %d in rec.config(DSPMode_CameraRecording %d, DSPMode_DuplexLowdelay %d).\n", tmp0, DSPMode_DuplexLowdelay, DSPMode_CameraRecording);
            tmp0 = DSPMode_DuplexLowdelay;
        }
        mpConfig->encoding_mode_config.dsp_mode = tmp0;
        mpConfig->encoding_mode_config.main_win_width = tmp1;
        mpConfig->encoding_mode_config.main_win_height = tmp2;
        mpConfig->encoding_mode_config.stream_number = tmp3;
        AM_INFO("main_win_width %d, main_win_height %d.\n", tmp1, tmp2);
    }

    //main stream width/height,offset_x/offset_y
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(main stream width/height,offset_x/offset_y).\n");
        return i;
    } else {
        mpConfig->encoding_mode_config.enc_width = tmp0;
        mpConfig->encoding_mode_config.enc_height = tmp1;
        mpConfig->encoding_mode_config.enc_offset_x= tmp2;
        mpConfig->encoding_mode_config.enc_offset_y= tmp3;
        AM_INFO("main_stream_width %d, main_stream_height %d, offset x %d, offset y %d.\n", tmp0, tmp1, tmp2, tmp3);
    }

    //video enc entropy
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    mpConfig->encoding_mode_config.entropy_type = AM_GetEntropyTypeFromString(buf);
    AM_ASSERT(mpConfig->encoding_mode_config.entropy_type != IParameters::EntropyType_NOTSet);

    //video enc bitrate
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (1==sscanf(buf, "%d", &tmp0)) {
        mpConfig->encoding_mode_config.video_bitrate = tmp0;
    } else {
        AM_ERROR("BAD video enc bit rate.\n");
        goto AM_LoadRecConfigFile_fail;
    }

    //H264 M, N, IDR Interval
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;

    if (3==sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
        //need check M, N, IDR Interval?
        mpConfig->encoding_mode_config.M = tmp0;
        mpConfig->encoding_mode_config.N = tmp1;
        mpConfig->encoding_mode_config.IDRInterval = tmp2;
    } else {
        AM_ERROR("BAD M N IDR interval, line.\n");
        goto AM_LoadRecConfigFile_fail;
    }

    //secondary stream width/height
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (2!=sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
        AM_ERROR("rec.config error(secondary stream width/height).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.second_enc_width = tmp0;
        mpConfig->encoding_mode_config.second_enc_height = tmp1;
        AM_INFO("second_stream_width %d, second_stream_height %d.\n", tmp0, tmp1);
    }

    //duplex max enc stream number, max dec stream number
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (2!=sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
        AM_ERROR("rec.config error(duplex max enc stream number, max dec stream number).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.num_of_enc_chans= tmp0;
        mpConfig->encoding_mode_config.num_of_dec_chans = tmp1;
        AM_INFO("duplex max enc stream number %d, max dec stream number %d.\n", tmp0, tmp1);
    }

    //duplex playback vout index, playback in pip
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (3!=sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
        AM_ERROR("rec.config error(duplex playback enabled, vout index, playback in pip).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.pb_display_enabled = tmp0;
        mpConfig->encoding_mode_config.playback_vout_index= tmp1;
        mpConfig->encoding_mode_config.playback_in_pip= tmp2;
        AM_INFO("duplex playback enabled %d, vout index %d, in pip %d.\n", tmp0, tmp1, tmp2);
    }

    //duplex playback display size/offset
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(duplex playback display size/offset).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.pb_display_width = tmp0;
        mpConfig->encoding_mode_config.pb_display_height = tmp1;
        mpConfig->encoding_mode_config.pb_display_left = tmp2;
        mpConfig->encoding_mode_config.pb_display_top = tmp3;
        AM_INFO("duplex playback display size %dx%d, offset %d,%d.\n", tmp0, tmp1, tmp2, tmp3);
    }

    //duplex preview: enabled, vout index, in pip, alpha value
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(duplex preview: enabled, vout index, in pip, alpha value).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.preview_enabled = tmp0;
        mpConfig->encoding_mode_config.preview_vout_index = tmp1;
        mpConfig->encoding_mode_config.preview_in_pip = tmp2;
        mpConfig->encoding_mode_config.preview_alpha = tmp3;
        AM_INFO("duplex preview: enabled %d, vout index %d, in pip %d, alpha value %d.\n", tmp0, tmp1, tmp2, tmp3);
    }

    //duplex preview display size/offset
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(duplex preview display size/offset).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.preview_width = tmp0;
        mpConfig->encoding_mode_config.preview_height = tmp1;
        mpConfig->encoding_mode_config.preview_left = tmp2;
        mpConfig->encoding_mode_config.preview_top = tmp3;
        AM_INFO("duplex preview display size %dx%d, offset %d,%d.\n", tmp0, tmp1, tmp2, tmp3);
    }

    //raw data related configuration
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (strncmp("rawdata related", buf, strlen("rawdata related"))) {
        AM_ERROR("rec.config error, no \"rawdata related\" line.\n");
        goto AM_LoadRecConfigFile_fail;
    }

    //previewC buffer enabled, width, height
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (3!=sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
        AM_ERROR("rec.config error(duplex previewC enabled, buffer size).\n");
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.previewc_rawdata_enabled = tmp0;
        mpConfig->encoding_mode_config.previewc_scaled_width = tmp1;
        mpConfig->encoding_mode_config.previewc_scaled_height = tmp2;
        AM_INFO("previewC enabled %d, buffer size %dx%d.\n", tmp0, tmp1, tmp2);
    }

    //previewC's source crop size/position
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (4!=sscanf(buf, "%d,%d,%d,%d", &tmp0, &tmp1, &tmp2, &tmp3)) {
        AM_ERROR("rec.config error(previewC width %d, height %d, offset x %d, offset y %d).\n", tmp0, tmp1, tmp2, tmp3);
        goto AM_LoadRecConfigFile_fail;
    } else {
        mpConfig->encoding_mode_config.previewc_crop_width = tmp0;
        mpConfig->encoding_mode_config.previewc_crop_height = tmp1;
        mpConfig->encoding_mode_config.previewc_crop_offset_x = tmp2;
        mpConfig->encoding_mode_config.previewc_crop_offset_y = tmp3;
        AM_INFO("previewC source's crop width %d, height %d, offset x %d, offset y %d.\n", tmp0, tmp1, tmp2, tmp3);
    }

    //thumbnail related config line
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (strncmp("thumbnail related", buf, strlen("thumbnail related"))) {
        AM_ERROR("rec.config error, no \"thumbnail related\" line, %s.\n", buf);
        return i;
    }

    //thumbnail related
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (3!=sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
        AM_ERROR("rec.config error(thumbnail enable, width, height).\n");
        return i;
    } else {
        mpConfig->encoding_mode_config.thumbnail_enabled = tmp0;
        mpConfig->encoding_mode_config.thumbnail_width = tmp1;
        mpConfig->encoding_mode_config.thumbnail_height = tmp2;
        AM_INFO("thumbnail enable %d, width %d, height %d.\n", tmp0, tmp1, tmp2);
    }

    //dsp piv related
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    exit_if_end_of_file;
    if (3!=sscanf(buf, "%d,%d,%d", &tmp0, &tmp1, &tmp2)) {
        AM_ERROR("rec.config error(dsp piv enable, width, height).\n");
        return i;
    } else {
        mpConfig->encoding_mode_config.dsp_piv_enabled = tmp0;
        mpConfig->encoding_mode_config.dsp_jpeg_width = tmp1;
        mpConfig->encoding_mode_config.dsp_jpeg_height = tmp2;
        AM_INFO("dsp piv enable %d, width %d, height %d.\n", tmp0, tmp1, tmp2);
    }

    //rtsp server related config line
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    if (rets) {
        if (strncmp("rtsp streaming server related", buf, strlen("rtsp streaming server related"))) {
            AM_ERROR("rec.config error, no \"rtsp streaming server related\" line, %s.\n", buf);
            return i;
        }
    }

    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    if (rets) {
        if (2!=sscanf(buf, "%d,%d", &tmp0, &tmp1)) {
            AM_ERROR("rec.config error(rtsp listen port, rtp/rtcp port start).\n");
            return i;
        } else {
            mpConfig->rtsp_server_config.rtsp_listen_port = tmp0;
            mpConfig->rtsp_server_config.rtp_rtcp_port_start = tmp1;
            AM_INFO("rtsp listen port %d, rtp/rtcp port base %d.\n", tmp0, tmp1);
        }
    }

    //low dramt raffic mode config
    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    if (rets) {
        if (strncmp("low dramt traffic mode config", buf, strlen("low dramt traffic mode config"))) {
            AM_ERROR("rec.config error, no \"low dramt traffic mode config\" line, %s.\n", buf);
            return i;
        }
    }

    memset(buf, 0, sizeof(buf));
    rets = fgets(buf, sizeof(buf), pFile);
    if (rets) {
        if (1!=sscanf(buf, "%d", &tmp0)) {
            AM_ERROR("rec.config error(low dramt traffic mode config).\n");
            return i;
        } else {
            AM_INFO("lvcap_ppl_type mode config %d.\n", tmp0);
        }
    }

    //check parameters
    if (DSPMode_DuplexLowdelay == mpConfig->encoding_mode_config.dsp_mode) {
        AM_ASSERT(0 == mpConfig->encoding_mode_config.playback_in_pip);
        mpConfig->encoding_mode_config.playback_in_pip = 0;
        if (eVoutLCD == mpConfig->encoding_mode_config.playback_vout_index) {
            AM_WARNING("playback on LCD.\n");
            AM_ASSERT(eVoutLCD == mpConfig->encoding_mode_config.preview_vout_index);
            mpConfig->encoding_mode_config.preview_vout_index = eVoutLCD;
            AM_ASSERT(0 == mpConfig->encoding_mode_config.preview_in_pip);
            mpConfig->encoding_mode_config.preview_in_pip = 0;
        } else if (eVoutHDMI == mpConfig->encoding_mode_config.playback_vout_index) {
            AM_WARNING("playback on HDMI.\n");
            AM_ASSERT(eVoutHDMI == mpConfig->encoding_mode_config.preview_vout_index);
            mpConfig->encoding_mode_config.preview_vout_index = eVoutHDMI;
            AM_ASSERT(1 == mpConfig->encoding_mode_config.preview_in_pip);
            mpConfig->encoding_mode_config.preview_in_pip = 1;
        }
    }
    return i;

AM_LoadRecConfigFile_fail:
    AM_DefaultRecConfig(mpConfig);
    return 0;
}

void AM_PrintLogConfig()
{
    AM_UINT i;

    //global
    AM_INFO(" global, log level %d, option 0x%x, output 0x%x.\n", g_ModuleLogConfig[LogGlobal].log_level, g_ModuleLogConfig[LogGlobal].log_option, g_ModuleLogConfig[LogGlobal].log_output);

    //log config
    for (i = LogModuleListStart; i< LogModuleListEnd; i++) {
        AM_INFO(" module index %d, log level %d, option 0x%x, output 0x%x.\n", i, g_ModuleLogConfig[i].log_level, g_ModuleLogConfig[i].log_option, g_ModuleLogConfig[i].log_output);
    }
}

void AM_DefaultLogConfig()
{
    AM_UINT i;

    //global
    g_ModuleLogConfig[LogGlobal].log_level = LogWarningLevel;
    g_ModuleLogConfig[LogGlobal].log_option = 0;//LogBasicInfo | LogPerformance;
    g_ModuleLogConfig[LogGlobal].log_output = LogConsole | LogLogcat;

    //log config
    for (i = LogModuleListStart; i< LogModuleListEnd; i++) {
        g_ModuleLogConfig[i].log_level = LogWarningLevel;
        g_ModuleLogConfig[i].log_option = 0;//LogBasicInfo | LogPerformance;
        g_ModuleLogConfig[i].log_output = LogConsole | LogLogcat;
    }
}

AM_UINT AM_LoadLogConfigFile(const char* CfgfileName)
{
    char buf[120];
    AM_UINT i;
    FILE* pFile = fopen(CfgfileName, "rt");
    if (!pFile) {
        fprintf(stderr, "open log.config file %s failed.\n", CfgfileName);
        AM_DefaultLogConfig();
        return 0;
    }

    //log config mode loading
    AM_UINT ctl_level = LogWarningLevel, ctl_option = 0, ctl_output = LogConsole | LogLogcat, ctl_mode = 0;
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);

    if (4!=sscanf(buf, "%d,%d,%d,%d", &ctl_level, &ctl_option, &ctl_output, &ctl_mode)) {
        AM_WARNING("log.config load err (log control settings).\n");
        AM_ASSERT(0);
        fclose(pFile);
        AM_DefaultLogConfig();
        return 0;
    }

    //need aways print:
    AM_WARNING("logconfig, control mode %d, ctl_level %d, ctl_option 0x%x, ctl_output 0x%x.\n", ctl_mode, ctl_level, ctl_option, ctl_output);
    if (ctl_mode == 1) {
        //all use the same
        AM_WARNING("log control use same mode, no need to load each modules' log config.\n");
        for (i = LogGlobal; i< LogModuleListEnd; i++) {
            g_ModuleLogConfig[i].log_level = ctl_level;
            g_ModuleLogConfig[i].log_option = ctl_option;
            g_ModuleLogConfig[i].log_output = ctl_output;
        }
        return LogModuleListEnd;
    }

    //first line is global (AM_XXX)
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);

    if (3!=sscanf(buf, "%d,%d,%d", &g_ModuleLogConfig[LogGlobal].log_level, &g_ModuleLogConfig[LogGlobal].log_option, &g_ModuleLogConfig[LogGlobal].log_output)) {
        AM_WARNING("log.config load err (log global settings).\n");
        AM_ASSERT(0);
        fclose(pFile);
        AM_DefaultLogConfig();
        return 0;
    }

    //log config for each module (AMLOG_XXX)
    for (i = LogModuleListStart; i< LogModuleListEnd; i++) {
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), pFile);
        if (3!=sscanf(buf, "%d,%d,%d", &g_ModuleLogConfig[i].log_level, &g_ModuleLogConfig[i].log_option, &g_ModuleLogConfig[i].log_output)) {
            AM_WARNING("log.config load err (log %d filters).\n", i);
            AM_ASSERT(0);
            fclose(pFile);
            //AM_DefaultLogConfig();
            return i;
        }
    }
    fclose(pFile);
    AM_ASSERT(i == LogModuleListEnd);

    //controled by global config
    if (ctl_mode == 0) {
        //do nothing
    } else if (ctl_mode == 2) {
        //need aways print:
        AM_WARNING("log control use least mode.\n");
        //least mode
        for (i = LogGlobal; i< LogModuleListEnd; i++) {
            if (g_ModuleLogConfig[i].log_level < ctl_level) {
                g_ModuleLogConfig[i].log_level = ctl_level;
            }

            g_ModuleLogConfig[i].log_option |= ctl_option;
            g_ModuleLogConfig[i].log_output |= ctl_output;
        }
    } else if (ctl_mode == 3) {
        //need aways print:
        AM_WARNING("log control use most mode.\n");
        //most mode
        for (i = LogGlobal; i< LogModuleListEnd; i++) {
            if (g_ModuleLogConfig[i].log_level > ctl_level) {
                g_ModuleLogConfig[i].log_level = ctl_level;
            }

            g_ModuleLogConfig[i].log_option &= ctl_option;
            g_ModuleLogConfig[i].log_output &= ctl_output;
        }
    }

    return LogModuleListEnd;
}

void AM_DumpBinaryFile(const char* fileName, AM_U8* pData, AM_UINT size)
{
    if (!fileName || !pData || !size) {
        return;
    }

    FILE* pFile = fopen(fileName, "wb");
    if (!pFile) {
        return;
    }
    fwrite(pData, 1, size, pFile);
    fclose(pFile);
}

void AM_AppendtoBinaryFile(const char* fileName, AM_U8* pData, AM_UINT size)
{
    if (!fileName || !pData || !size) {
        return;
    }

    FILE* pFile = fopen(fileName, "ab");
    if (!pFile) {
        return;
    }
    fwrite(pData, 1, size, pFile);
    fclose(pFile);
}

void AM_DumpBinaryFile_withIndex(const char* fileName, AM_UINT index, AM_U8* pData, AM_UINT size)
{
    if (!fileName || !pData || !size) {
        return;
    }
    char* filename = new char[strlen(fileName) + 40];
    if (!filename) {
        return;
    }
    snprintf(filename, strlen(fileName) + 39, "%s.%d", fileName, index);
    FILE* pFile = fopen(filename, "wb");
    if (!pFile) {
        delete filename;
        return;
    }
    fwrite(pData, 1, size, pFile);
    fclose(pFile);
    delete filename;
}
void AM_Planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height)
{
    //first line
    AM_UINT i, j;
    des[0] = src[0];
    for(i = 0; i < width -1; i++)
    {
        des[2*i + 1] = (3 * src[i] + src[i + 1])>>2;
        des[2*i + 2] = (src[i] + 3 * src[i + 1])>>2;
    }
    des[2 * width - 1] = src[width - 1];
    des += des_stride;
    for(j = 1; j < height; j++)
    {
        des[0]= (3*src[0] + src[src_stride])>>2;
        des[des_stride]= (src[0] + 3*src[src_stride])>>2;
        for(i = 0; i < width - 1; i++)
        {
            des[2*i + 1]= (3*src[i + 0] +   src[i + src_stride + 1])>>2;
            des[2*i + des_stride + 2]= (src[i + 0] + 3*src[i + src_stride + 1])>>2;
            des[2*i + des_stride + 1]= (src[i + 1] + 3*src[i + src_stride])>>2;
            des[2*i + 2]= (3*src[i + 1] +src[i + src_stride])>>2;
        }
        des[width*2 -1]= (3*src[width-1] + src[width-1 + width])>>2;
        des[width*2 -1 + des_stride]= (src[width - 1] + 3*src[width -1 + width])>>2;

        des +=des_stride * 2;
        src +=src_stride;
    }
    //last line
    des[0]= src[0];

    for (i = 0; i < width-1; i++) {
        des[2 * i + 1]= (3 * src[i] + src[i + 1])>>2;
        des[2 * i + 2]= (src[i] + 3 * src[i + 1])>>2;
    }
    des[2 * width -1]= src[width -1];
}
int AM_ConvertFrame(int PixelFormat, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height)
{
    AM_UINT i = 0, cnt;
    AM_U8* pu = src[1];
    AM_U8* pv = src[2];
    if(PixelFormat == 4 /*PIX_FMT_YUV422P*/ || PixelFormat == 13 /*PIX_FMT_YUVJ422P*/) {
        AM_U8* d_pu = des[1];
        AM_U8* d_pv = des[2];
        // copy the Y plane
        if(src_stride[0]== des_stride[0])
            ::memcpy(des[0], src[0], src_stride[0] * src_height);
        else {
            AM_U8* pDes = des[0];
            AM_U8* pSrc = src[0];
            for(i = 0; i < src_height; i++, pDes += des_stride[0], pSrc += src_stride[0])
                ::memcpy(pDes, pSrc, src_width);
        }

        // copy the U & V plane
        for(i = 0; i < (src_height >> 1); i++)
        {
            for(cnt = 0; cnt < (src_width >> 1); cnt ++)
            {
                d_pu[cnt] = pu[2 * cnt];
                d_pv[cnt] = pv[2 * cnt];
            }
            pu += src_stride[1];
            pv += src_stride[2];
            d_pu += des_stride[1];
            d_pv += des_stride[2];
        }
        return 0;
    }else if(PixelFormat == 6/*PIX_FMT_YUV410P*/) {
        //AMLOG_INFO("PIX_FMT_YUV410P.\n");
        // copy the Y plane
        if(src_stride[0]== des_stride[0])
            ::memcpy(des[0], src[0], src_stride[0] * src_height);
        else {
            AM_U8* pDes = des[0];
            AM_U8* pSrc = src[0];
            for(i = 0; i < src_height; i++, pDes += des_stride[0], pSrc += src_stride[0])
                ::memcpy(pDes, pSrc, src_width);
        }
        AM_U8* su = src[1];
        AM_U8* sv = src[2];
        AM_U8* du = des[1];
        AM_U8* dv = des[2];

        // copy the U & V plane
        AM_Planar2x(su, du, src_stride[1], des_stride[1], src_width >> 2, src_height >> 2);
        AM_Planar2x(sv, dv, src_stride[2], des_stride[2], src_width >> 2, src_height >> 2);
        return 0;
    }else if(PixelFormat == 46 /*PIX_FMT_RGB555LE-->RGB565LE*/){
        //LOGE("-----PixelFormat == 46 s-w=%d, s-h=%d-----\n", src_width, src_height);
        AM_U16* pDes = (AM_U16*)des[0];
        AM_U16* pSrc = (AM_U16*)src[0];
        AM_U16 r, g, b;
        for(AM_UINT j = 0; j < src_height; j++) {
            for(AM_UINT i = 0; i < src_width; i++) {
                r = (pSrc[i] & 0x7C00) << 1;
                g = (pSrc[i] & 0x3E0) << 1 | (pSrc[i] & 0x20);
                b = (pSrc[i] & 0x1F);
                pDes[i] = r  | g  | b ;
            }
            pDes += des_stride[0] >> 1;
            pSrc += src_stride[0] >> 1;
        }
        return 44;
    }else if(PixelFormat == 30/*BGRA*/) {
        AM_U16* pDes = (AM_U16*)des[0];
        AM_U8* pSrc = (AM_U8*)src[0];
        AM_U16 r, g, b;
        for(AM_UINT j = 0; j < src_height; j++) {
            for(AM_UINT i = 0; i < src_width; i++) {
                r = (pSrc[4 * i + 2] & 0xF8) << 8;
                g = (pSrc[4 * i + 1] & 0xFC) << 3;
                b = (pSrc[4 * i ] & 0xF8) >> 3;
                pDes[i] = r  | g  | b ;
            }
            pDes += des_stride[0] >> 1;
            pSrc += src_stride[0];
        }
        return 44;
    }else {
        return PixelFormat;
    }
}

struct img_struct_t {
  unsigned char *yuv[3];
  int stride[3];
  int width;
  int height;
};
static int ProgressiveScale_yuv420p(img_struct_t *src_img,img_struct_t *dst_img);
void AM_Scale(int PixelFormat, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height, AM_UINT des_width, AM_UINT des_height)
{
    if(PixelFormat == 0 /*420P*/)
    {
#if 0
        AM_UINT current = 0, currentY = 0, residue = src_width >> 1, residueY = src_width >> 1;
        AM_UINT i = 0, j = 0, col1 = 0, col2 = 0;
        AM_UINT* index =(AM_UINT*) malloc (des_width * sizeof(AM_UINT));
        AM_U8* desy1, *desy2, *srcy1, *srcy2, *descb, *descr, *srccb, *srccr;
        for ( ; i < des_width; i++) {
            index[i] = current;
            residue += src_width;
            while (residue > des_width ) {
                current++;
                residue -= des_width;
            }
        }

        current = 0;
        residue = src_height >> 1;

        desy1 = des[0];
        desy2 = des[0] + des_stride[0];

        srcy1 = src[0];
        srcy2 = src[0] + src_stride[0];

        descb = des[1];
        descr = des[2];
        srccb = src[1];
        srccr = src[2];

        for (j = 0; j< des_height/2; j++) {

            for(i=0; i<des_width/2; i++) {
                desy1[i*2] = srcy1[index[i*2]];
                desy1[i*2 + 1] = srcy1[index[i*2 + 1]];
                desy2[i*2] = srcy2[index[i*2]];
                desy2[i*2 + 1] = srcy2[index[i*2 + 1]];
                descb[i] = srccb[index[i]];
                descr[i] = srccr[index[i]];
            }
            residue += src_height;
            while (residue > des_height ) {
                current ++;
                residue -= des_height;
            }
            col1 = current;
            residueY += src_height << 1;
            while (residueY > des_height ) {
                currentY ++;
                residueY -= des_height;
            }
            col2 = currentY;
            desy1 += des_stride[0] << 1;
            desy2 += des_stride[0] << 1;
            descb += des_stride[1];
            descr += des_stride[2];
            srcy1 = src[0] + src_stride[0] * col2;
            srcy2 = src[0] + src_stride[0] * (col2 + 1);
            srccb = src[1] + src_stride[1] * col1;
            srccr = src[2] + src_stride[2] * col1;
        }
        //
        free((void*)index);
#else
       img_struct_t src_img;
       img_struct_t dst_img;

       src_img.yuv[0] = src[0];
       src_img.yuv[1] = src[1];
       src_img.yuv[2] = src[2];
       src_img.stride[0] = src_stride[0];
       src_img.stride[1] = src_stride[1];
       src_img.stride[2] = src_stride[2];
       src_img.width = src_width;
       src_img.height = src_height;

       dst_img.yuv[0] = des[0];
       dst_img.yuv[1] = des[1];
       dst_img.yuv[2] = des[2];
       dst_img.stride[0] = des_stride[0];
       dst_img.stride[1] = des_stride[1];
       dst_img.stride[2] = des_stride[2];
       dst_img.width = des_width;
       dst_img.height = des_height;
       ProgressiveScale_yuv420p(&src_img,&dst_img);
#endif
    }
    if(PixelFormat == 44/*RGB565*/) {
        AM_UINT current = 0, col = 0, residue = src_width >> 1;
        AM_UINT* index =(AM_UINT*) malloc (des_width * sizeof(AM_UINT));
        AM_UINT i = 0, j = 0;
        for ( ; i < des_width; i++) {
            index[i] = current;
            residue += src_width;
            while (residue > des_width) {
                current++;
                residue -= des_width;
            }
        }
        current = 0;
        residue = src_height >> 1;

        AM_U16* desy1 = (AM_U16*)des[0];
        AM_U16* desy2 = (AM_U16*)(des[0] + des_stride[0]);

        AM_U16* srcy1 = (AM_U16*)src[0];
        AM_U16* srcy2 = (AM_U16*)(src[0] + src_stride[0]);

        for (j = 0; j< (des_height >>1); j++) {

            for(i=0; i< (des_width >> 1); i++) {
                desy1[i*2] = srcy1[index[i*2]];
                desy1[i*2 + 1] = srcy1[index[i*2 + 1]];
                desy2[i*2] = srcy2[index[i*2]];
                desy2[i*2 + 1] = srcy2[index[i*2 + 1]];
            }
            residue += src_height << 1;
            while (residue > des_height ) {
                current ++;
                residue -= des_height;
            }
            col = current;
            desy1 += des_stride[0];
            desy2 += des_stride[0];
            srcy1 = (AM_U16*)(src[0] + src_stride[0] * col);
            srcy2 = (AM_U16*)(src[0] + src_stride[0] * (col + 1));
        }
        //
        free((void*)index);
    }
}

void AM_SimpleScale(IParameters::PixFormat pix_format, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT src_width, AM_UINT src_height, AM_UINT des_width, AM_UINT des_height)
{
    if (IParameters::PixFormat_YUV420P == pix_format) {
        AM_UINT current = 0, currentY = 0, residue = src_width >> 1, residueY = src_width >> 1;
        AM_UINT i = 0, j = 0, col1 = 0, col2 = 0;
        AM_UINT* index =(AM_UINT*) malloc (des_width * sizeof(AM_UINT));
        AM_U8* desy1, *desy2, *srcy1, *srcy2, *descb, *descr, *srccb, *srccr;
        for ( ; i < des_width; i++) {
            index[i] = current;
            residue += src_width;
            while (residue > des_width ) {
                current++;
                residue -= des_width;
            }
        }

        current = 0;
        residue = src_height >> 1;

        desy1 = des[0];
        desy2 = des[0] + des_stride[0];

        srcy1 = src[0];
        srcy2 = src[0] + src_stride[0];

        descb = des[1];
        descr = des[2];
        srccb = src[1];
        srccr = src[2];

        for (j = 0; j< des_height/2; j++) {

            for(i=0; i<des_width/2; i++) {
                desy1[i*2] = srcy1[index[i*2]];
                desy1[i*2 + 1] = srcy1[index[i*2 + 1]];
                desy2[i*2] = srcy2[index[i*2]];
                desy2[i*2 + 1] = srcy2[index[i*2 + 1]];
                descb[i] = srccb[index[i]];
                descr[i] = srccr[index[i]];
            }
            residue += src_height;
            while (residue > des_height ) {
                current ++;
                residue -= des_height;
            }
            col1 = current;
            residueY += src_height << 1;
            while (residueY > des_height ) {
                currentY ++;
                residueY -= des_height;
            }
            col2 = currentY;
            desy1 += des_stride[0] << 1;
            desy2 += des_stride[0] << 1;
            descb += des_stride[1];
            descr += des_stride[2];
            srcy1 = src[0] + src_stride[0] * col2;
            srcy2 = src[0] + src_stride[0] * (col2 + 1);
            srccb = src[1] + src_stride[1] * col1;
            srccr = src[2] + src_stride[2] * col1;
        }
        //
        free((void*)index);
    } else if (IParameters::PixFormat_NV12 == pix_format) {
        AM_UINT current = 0, currentY = 0, residue = src_width >> 1, residueY = src_width >> 1;
        AM_UINT i = 0, j = 0, col1 = 0, col2 = 0;
        AM_UINT* index =(AM_UINT*) malloc (des_width * sizeof(AM_UINT));
        AM_U8* desy1, *desy2, *srcy1, *srcy2, *descb, *srccb;//, *descr, *srccr
        for ( ; i < des_width; i++) {
            index[i] = current;
            residue += src_width;
            while (residue > des_width ) {
                current++;
                residue -= des_width;
            }
        }

        current = 0;
        residue = src_height >> 1;

        desy1 = des[0];
        desy2 = des[0] + des_stride[0];

        srcy1 = src[0];
        srcy2 = src[0] + src_stride[0];

        descb = des[1];
        //descr = des[2];
        srccb = src[1];
        //srccr = src[2];

        for (j = 0; j< des_height/2; j++) {

            for(i=0; i<des_width/2; i++) {
                desy1[i*2] = srcy1[index[i*2]];
                desy1[i*2 + 1] = srcy1[index[i*2 + 1]];
                desy2[i*2] = srcy2[index[i*2]];
                desy2[i*2 + 1] = srcy2[index[i*2 + 1]];

                descb[i*2] = srccb[index[i]*2];
                descb[i*2 +1] = srccb[index[i]*2 + 1];
            }
            residue += src_height;
            while (residue > des_height ) {
                current ++;
                residue -= des_height;
            }
            col1 = current;
            residueY += src_height << 1;
            while (residueY > des_height ) {
                currentY ++;
                residueY -= des_height;
            }
            col2 = currentY;
            desy1 += des_stride[0] << 1;
            desy2 += des_stride[0] << 1;
            descb += des_stride[1];
            srcy1 = src[0] + src_stride[0] * col2;
            srcy2 = src[0] + src_stride[0] * (col2 + 1);
            srccb = src[1] + src_stride[1] * col1;
        }
        //
        free((void*)index);
    } else {
        AM_ERROR("please add implement\n");
    }

}

void AM_ConvertPixFormat(IParameters::PixFormat src_pix_format, IParameters::PixFormat des_pix_format, AM_U8* src[], AM_U8* des[], AM_INT src_stride[], AM_INT des_stride[], AM_UINT width, AM_UINT height)
{
    AM_UINT i, j;
    AM_U8* p0, *p1, *p2;

    if (src_pix_format == des_pix_format) {
        AM_ERROR("no need do convertion.\n");
        return;
    }

    if ((IParameters::PixFormat_NV12 == src_pix_format) && (IParameters::PixFormat_YUV420P == des_pix_format)) {
        //nv12 to yuv420p

        //y
        if ((src[0] == des[0]) && (src_stride[0] == des_stride[0])) {
            //no need to copy, same place
            AM_ERROR("comes here\n");
        } else {
            AM_ERROR("need copy???\n");
            p0 = src[0];
            p1 = des[0];
            for (i = 0; i < height; i++) {
                memcpy(p1, p0, width);
                p1 += des_stride[0];
                p0 += src_stride[0];
            }
        }

        //uv
        p0 = src[1];
        p1 = des[1];
        p2 = des[2];
        height = height>>1;
        width = width>>1;
        for (j = 0; j < height; j++) {
            for (i = 0; i< width; i++) {
                p1[i] = p0[2*i];
                p2[i] = p0[2*i + 1];
            }
            p0 += src_stride[1];
            p1 += des_stride[1];
            p2 += des_stride[2];
        }

    } else {
        AM_ERROR("add implement.\n");
    }

}

void AM_ToRGB565(int PixelFormat, AM_U8* src[], AM_U16* rgb, AM_UINT width, AM_UINT height, AM_INT src_stride[])
{
    AM_UINT i, j;
    if(PixelFormat == 0) {
        AM_U8* y = src[0];
        AM_U8* cb = src[1];
        AM_U8* cr = src[2];
        AM_UINT stride = src_stride[0];
        AM_UINT uv_stride = src_stride[1];
        AM_U8* y2 = src[0] + stride;
        AM_INT u, v;
        AM_INT rdiff, igdiff, bdiff;
        AM_INT r, g, b;
        AM_U16* rgb1 = rgb, *rgb2 = rgb + width;

        height >>= 1;

        for (j=0; j<height; j++) {
            for (i=0; i<width; i++) {
                u = cb[i/2] - 128;
                v = cr[i/2] - 128;

                rdiff = v + ((v*103)>>8);
                igdiff = ((u*88)>>8) + ((v*183)>>8);
                bdiff = u + ((u*198)>>8);

                r = y[i] + rdiff;
                g = y[i] - igdiff;
                b = y[i] + bdiff;

                if (r>255)
                    r = 255;
                else if (r<0)
                    r = 0;
                if (g>255)
                    g = 255;
                else if (g<0)
                    g = 0;
                if (b>255)
                    b = 255;
                else if (b<0)
                    b = 0;

                rgb1[i] = ((r & 0xf8)<<8) | ((g & 0xfc)<<3) | ((b & 0xf8)>>3);

                r = y2[i] + rdiff;
                g = y2[i] - igdiff;
                b = y2[i] + bdiff;

                if (r>255)
                    r = 255;
                else if (r<0)
                    r = 0;
                if (g>255)
                    g = 255;
                else if (g<0)
                    g = 0;
                if (b>255)
                    b = 255;
                else if (b<0)
                    b = 0;

                rgb2[i] = ((r & 0xf8)<<8) | ((g & 0xfc)<<3) | ((b & 0xf8)>>3);

            }
            rgb1 += width<<1;
            rgb2 += width<<1;
            y += stride<<1;
            y2 += stride<<1;
            cb += uv_stride;
            cr += uv_stride;
        }
    }
    if(PixelFormat == 44/*RGB565LE*/) {
        ::memcpy((AM_U8*)rgb, src[0], src_stride[0]* height);
    }
}

AM_UINT AM_GetNextStartCode(AM_U8* pstart, AM_U8* pend, AM_INT esType)
{
    AM_UINT size = 0;
    AM_U8* pcur = pstart;
    AM_UINT state = 0;
    AM_U8 code1, code2;
    AM_UINT cnt = 2;//find start code cnt

    switch (esType) {
        case 1://UDEC_H264:
            code1 = 0x7B;
            code2 = 0x7D;
            break;
        case 2://UDEC_MP12:
            //return pend - pstart;
            code1 = 0xB3;
            code2 = 0x00;
            break;
        case 3://UDEC_MP4H:
//            code1 = 0xB8;
//            code2 = 0xB7;
            code1 = 0xC4;
            code2 = 0xC5;
            break;
        case 5://UDEC_VC1:
            code1 = 0x72;
            code2 = 0x71;
            break;
        default:
            AM_ERROR("not supported es type.\n");
            return pend - pstart;
    }

    while (pcur <= pend) {
        switch (state) {
            case 0:
                if (*pcur++ == 0x0)
                    state = 1;
                break;
            case 1://0
                if (*pcur++ == 0x0)
                    state = 2;
                else
                    state = 0;
                break;
            case 2://0 0
                if (*pcur == 0x1)
                    state = 3;
                else if (*pcur != 0x0)
                    state = 0;
                pcur ++;
                break;
            case 3://0 0 1
                if (*pcur == code1) { //pic header
                    if (cnt == 1) {
                        size = pcur - 3 - pstart;
                        return size;
                    }
                    cnt --;
                    state = 0;
                } else if (*pcur == code2) { //seq header
                    if (cnt == 1) {
                        size = pcur - 3 - pstart;
                        return size;
                    }
                    state = 0;
                } else if (*pcur){
                    state = 0;
                } else {
                    state = 1;
                }
                pcur ++;
                break;
        }
    }
    size = pend - pstart;
    return size;
}

#ifdef AM_DEBUG
#define AM_SaveErrHandleInfo(level, format, args...)  do { \
	if (g_GlobalCfg.mErrHandleFd && level>=g_GlobalCfg.mErrorCodeRecordLevel) { \
		 fprintf(g_GlobalCfg.mErrHandleFd, format, ##args); \
		 fflush(g_GlobalCfg.mErrHandleFd); \
	} \
} while (0)
#else
#define AM_SaveErrHandleInfo(level, format, args...) (void)0
#endif

AM_UINT AnalyseUdecErrorCode(AM_UINT udecErrorCode, AM_UINT debugConfig)
{
    UDECErrorCode code;
    code.mu32 = udecErrorCode;
    AM_PRINTF("[ErrorHandling]: start Analyse error code 0x%x, decoderType %d, error_level %d, error_type %d.\n", code.mu32, code.detail.decoder_type, code.detail.error_level, code.detail.error_type);
    //AM_SaveErrHandleInfo(code.detail.error_level, "[ErrorHandle orginal data]: error code 0x%x, decoderType %d, error_level %d, error_type %d.\n", code.mu32, code.detail.decoder_type, code.detail.error_level, code.detail.error_type);//roy 110922 disable ErrorHandle info save
    if (code.detail.error_level >= EH_ErrorLevel_Last || code.detail.decoder_type >=EH_DecoderType_Cnt) {
        AM_ASSERT(0);
        AM_ERROR("Must have errors, (AnalyseUdecErrorCode) decoder_type/error level out of range.\n");
        return MW_Bahavior_Ignore;
    }

    AM_PRINTF(" decoder_type %s,  error_level %s, error_type %d.\n", GUdecStringDecoderType[code.detail.decoder_type], GUdecStringErrorLevel[code.detail.error_level], code.detail.error_type);
    //AM_SaveErrHandleInfo(code.detail.error_level, "[ErrorHandle analyse data]: decoder_type %s,  error_level %s,", GUdecStringDecoderType[code.detail.decoder_type], GUdecStringErrorLevel[code.detail.error_level]);
    if (debugConfig == 1) {
        //debug use, ignore error code type
        AM_PRINTF(" debug(pb.config)use, exit playback(STOP(0) and EXIT(0)).\n");
        return MW_Bahavior_ExitPlayback;
    } else if (debugConfig == 2) {
        //debug use, ignore error code type, not touch env(halt) for debug check
        AM_PRINTF(" debug(pb.config)use, halt playback, not touch env for debug check.\n");
        return MW_Bahavior_HaltForDebug;
    } else if (debugConfig){
        //should not come here
        AM_PRINTF(" should not come here, exit playback(STOP(0) and EXIT(0)).\n");
        return MW_Bahavior_ExitPlayback;
    }

    //for warning and recoverable error
    switch (code.detail.error_level) {
        case EH_ErrorLevel_NoError:
            return MW_Bahavior_Ignore;
        case EH_ErrorLevel_Warning:
            return MW_Bahavior_Ignore_AndPostAppMsg;
        case EH_ErrorLevel_Recoverable:
            return MW_Bahavior_Ignore_AndPostAppMsg;
        case EH_ErrorLevel_Fatal:
            break;
        default:
            AM_ASSERT(0);
            AM_ERROR("Must have errors, (AnalyseUdecErrorCode)error level out of range, ignore this error.\n");
            return MW_Bahavior_Ignore;
    }

    AM_ASSERT(code.detail.error_level == EH_ErrorLevel_Fatal);
    //need mw to handle error(fatal error)
    switch (code.detail.decoder_type) {

        case EH_DecoderType_Common:
            AM_PRINTF("  ecoder type: %d, %s.\n", code.detail.error_type, GUdecStringCommonErrorType[code.detail.error_type]);
            //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type %s.\n", GUdecStringCommonErrorType[code.detail.error_type]);
            switch (code.detail.error_type) {
                case EH_CommonErrorType_HardwareHang:
                    return MW_Bahavior_ExitPlayback;
                case EH_CommonErrorType_FrameBufferLeak:
                    return MW_Bahavior_ExitPlayback;
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_ExitPlayback;
            }
            return MW_Bahavior_ExitPlayback;//default

        case EH_DecoderType_H264:
            AM_PRINTF("  ecoder type: H264.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_MPEG12:
            AM_PRINTF("  ecoder type: MPEG12.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_MPEG4_HW:
            AM_PRINTF("  ecoder type: MPEG4 HW.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                /*case 0x0a:
                    AM_PRINTF("mpeg4 bitstream is not supported, and no frame will be generated by the DSP.\n");
                    return MW_Bahavior_ExitPlayback;*///no seek just flush can handle this case, so no exit playback, 2011.12.23, roy mdf
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_MPEG4_Hybird:
            AM_PRINTF("  ecoder type: MPEG4 Hybird.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_VC1:
            AM_PRINTF("  ecoder type: VC-1.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_RV40_Hybird:
            AM_PRINTF("  ecoder type: RV40 Hybird.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_JPEG:
            AM_PRINTF("  ecoder type: JPEG.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        case EH_DecoderType_SW:
            AM_PRINTF("  ecoder type: SW.\n");
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_ASSERT(0);
                    AM_ERROR("unknown error_type.\n");
                    //AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    return MW_Bahavior_TrySeekToSkipError;
            }
            return MW_Bahavior_TrySeekToSkipError;//default

        default:
            AM_ASSERT(0);
            AM_ERROR("wrong decoder type 0x%x, must have errors.\n", code.detail.decoder_type);
            break;
    }

    //not handled error type
    return MW_Bahavior_ExitPlayback;
}

void RecordUdecErrorCode(AM_UINT udecErrorCode)
{
    if(0==udecErrorCode || NULL==g_GlobalCfg.mErrHandleFd)
        return;

    UDECErrorCode code;
    code.mu32 = udecErrorCode;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    AM_SaveErrHandleInfo(code.detail.error_level, "[ErrorRecord time(sec.usec):%ld.%ld] [orginal data]: error code 0x%x, decoderType %d, error_level %d, error_type %d. [analyse data]: decoder_type %s,  error_level %s,", tv.tv_sec, tv.tv_usec, code.mu32, code.detail.decoder_type, code.detail.error_level, code.detail.error_type, GUdecStringDecoderType[code.detail.decoder_type], GUdecStringErrorLevel[code.detail.error_level]);
    switch (code.detail.decoder_type)
    {
        case EH_DecoderType_Common:
            AM_SaveErrHandleInfo(code.detail.error_level, "  error_type %s.\n", GUdecStringCommonErrorType[code.detail.error_type]);
            switch (code.detail.error_type) {
                case EH_CommonErrorType_HardwareHang:
                    break;
                case EH_CommonErrorType_FrameBufferLeak:
                    break;
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_H264:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_MPEG12:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_MPEG4_HW:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_MPEG4_Hybird:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_VC1:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_RV40_Hybird:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_JPEG:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        case EH_DecoderType_SW:
            switch (code.detail.error_type) {
                //todo, add cases
                default:
                    AM_SaveErrHandleInfo(code.detail.error_level, "  error_type unknown.\n");
                    break;
            }
            break;

        default:
            AM_SaveErrHandleInfo(code.detail.error_level, "  decoder_type unknown.\n");
            break;
    }

}

void AM_LoadGlobalCfg(const char* CfgfileName)
{
#ifdef AM_DEBUG
    char buf[400];
    AM_UINT tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9;
    FILE* pFile = fopen(CfgfileName, "rt");
    if (!pFile) {
        AM_WARNING("open config file %s failed, use default global config, default settings:\n", CfgfileName);
        AM_WARNING(" use active_pb_engine(%d), use active_record_engine(%d), use general decoder (%d).\n", g_GlobalCfg.mUseActivePBEngine,  g_GlobalCfg.mUseActiveRecordEngine, g_GlobalCfg.mUseGeneralDecoder);
        AM_WARNING(" use mUsePrivateAudioDecLib(%d), use mUsePrivateAudioEncLib(%d), use mUseFFMpegMuxer2 (%d), use mErrorCodeRecordLevel (%d).\n", g_GlobalCfg.mUsePrivateAudioDecLib,  g_GlobalCfg.mUsePrivateAudioEncLib, g_GlobalCfg.mUseFFMpegMuxer2, g_GlobalCfg.mErrorCodeRecordLevel);
        return;
    }

    //skip first line--MediaRecorder select
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), pFile);
    fclose(pFile);
    if (9!=sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d", &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8, &tmp9)) {
        AM_ERROR("global settings error(%s).\n", CfgfileName);
        return;
    } else {
        g_GlobalCfg.mUseActivePBEngine = tmp1;
        g_GlobalCfg.mUseActiveRecordEngine = tmp2;
        g_GlobalCfg.mUseGeneralDecoder = tmp3;
        g_GlobalCfg.mUsePrivateAudioDecLib = tmp4;
        g_GlobalCfg.mUsePrivateAudioEncLib = tmp5;
        g_GlobalCfg.mUseFFMpegMuxer2 = tmp6;
        g_GlobalCfg.mErrorCodeRecordLevel = tmp7;
        g_GlobalCfg.mUseNativeSEGFaultHandler = tmp8;
        g_GlobalCfg.mbEnableOverlay= tmp9;
        AM_WARNING("use active_pb_engine(%d), use active_record_engine(%d), use general decoder (%d).\n", g_GlobalCfg.mUseActivePBEngine,  g_GlobalCfg.mUseActiveRecordEngine, g_GlobalCfg.mUseGeneralDecoder);
        AM_WARNING("use mUsePrivateAudioDecLib(%d), use mUsePrivateAudioEncLib(%d), use mUseFFMpegMuxer2 (%d), use mErrorCodeRecordLevel (%d), mUseNativeSEGFaultHandler(%d), mbEnableOverlay(%d).\n", g_GlobalCfg.mUsePrivateAudioDecLib,  g_GlobalCfg.mUsePrivateAudioEncLib, g_GlobalCfg.mUseFFMpegMuxer2, g_GlobalCfg.mErrorCodeRecordLevel, g_GlobalCfg.mUseNativeSEGFaultHandler, g_GlobalCfg.mbEnableOverlay);
    }
#endif
}

void AM_OpenGlobalFiles(void)
{
    char filename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN + 1] = {0};

    //if no log files, open
    if(!g_GlobalCfg.mLogFd) {
        snprintf(filename, sizeof(filename), "%s/pb.log", AM_GetPath(AM_PathDump));
        g_GlobalCfg.mLogFd = fopen(filename, "wb");
        if (!g_GlobalCfg.mLogFd) {
            fprintf(stderr, "open pb.log failed, cannot save log to file.\n");
        }
        else {
            fprintf(stdout, "open pb.log succeed.\n");
        }
    }
    if(!g_GlobalCfg.mErrHandleFd) {
        snprintf(filename, sizeof(filename), "%s/pb.err", AM_GetPath(AM_PathDump));
        g_GlobalCfg.mErrHandleFd = fopen(filename, "awb");
        if (!g_GlobalCfg.mErrHandleFd) {
            fprintf(stderr, "open pb.err failed, cannot save log to file.\n");
        }
        else {
            fprintf(stdout, "open pb.err succeed.\n");
        }
    }

}

void AM_CloseGlobalFiles(void)
{
#ifdef AM_DEBUG
    //if exist log files, close
    if(g_GlobalCfg.mLogFd) {
        fclose(g_GlobalCfg.mLogFd);
        g_GlobalCfg.mLogFd = NULL;
    }
    if(g_GlobalCfg.mErrHandleFd) {
        fclose(g_GlobalCfg.mErrHandleFd);
        g_GlobalCfg.mErrHandleFd = NULL;
    }
#endif
}

void AM_GlobalFilesSaveBegin(char* pchDataSource)
{
#ifdef AM_DEBUG
    if (g_GlobalCfg.mLogFd) {
        fprintf(g_GlobalCfg.mLogFd, "*********[pblog] %s begin.\n", pchDataSource);
        fflush(g_GlobalCfg.mLogFd);
    }
    if (g_GlobalCfg.mErrHandleFd) {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        fprintf(g_GlobalCfg.mErrHandleFd, "*********[pberr time(sec.usec):%ld.%ld] %s begin.\n", tv.tv_sec, tv.tv_usec, pchDataSource);
        fflush(g_GlobalCfg.mErrHandleFd);
    }
#endif
}

void AM_GlobalFilesSaveEnd(char* pchDataSource)
{
#ifdef AM_DEBUG
    if(g_GlobalCfg.mLogFd) {
        fprintf(g_GlobalCfg.mLogFd, "*********[pblog] %s end.\n\n", pchDataSource);
        fflush(g_GlobalCfg.mLogFd);
    }
    if(g_GlobalCfg.mErrHandleFd) {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        fprintf(g_GlobalCfg.mErrHandleFd, "*********[pberr time(sec.usec):%ld.%ld] %s end.\n\n", tv.tv_sec, tv.tv_usec, pchDataSource);
        fflush(g_GlobalCfg.mErrHandleFd);
    }
#endif
}

void AM_ProbeSdcardPath()
{
    FILE   *stream;
    memset(g_AMPathes[AM_PathSdcard], '\0', sizeof(g_AMPathes[AM_PathSdcard]));

#if PLATFORM_LINUX
    stream = popen( "mount | grep mmcb* | awk '{printf $3}'", "r" );
#elif PLATFORM_ANDROID
    stream = popen( "mount | grep sdca* | grep -v sdcard/  | awk '{printf $2}'", "r" );
#endif
    AM_INFO("_get_sdcard_path, popen stream=0x%p.\n",stream);
    if(NULL==stream)//safety for bug#2140, 2012.02.10
    {
        AM_ERROR("_get_sdcard_path, popen device failed.\n");
        return;
    }
    fread( g_AMPathes[AM_PathSdcard], sizeof(char), sizeof(g_AMPathes[AM_PathSdcard]),  stream);
    AM_INFO("_get_sdcard_path, sdcard path=%s\n", g_AMPathes[AM_PathSdcard]);

    pclose(stream);

    if (g_AMPathes[AM_PathSdcard][0] == 0) {
        AM_INFO("Get sdcard path fail, use default.\n");
#if PLATFORM_LINUX
        strncpy(g_AMPathes[AM_PathSdcard], "/tmp/mmcblk0p1", DAMF_MAX_FILENAME_LEN);
#elif PLATFORM_ANDROID
        strncpy(g_AMPathes[AM_PathSdcard], "/mnt/sdcard", DAMF_MAX_FILENAME_LEN);
#endif
    }
}

void AM_GetDefaultPathes()
{
    AM_ProbeSdcardPath();
    //use sdcard as default
    strncpy(g_AMPathes[AM_PathDump], g_AMPathes[AM_PathSdcard], sizeof(g_AMPathes[AM_PathDump]));

#if PLATFORM_LINUX
    strncpy(g_AMPathes[AM_PathConfig], "/usr/local/bin", sizeof(g_AMPathes[AM_PathConfig]));
#else
    strncpy(g_AMPathes[AM_PathConfig], g_AMPathes[AM_PathSdcard], sizeof(g_AMPathes[AM_PathConfig]));
#endif

}

AM_ERR AM_SetPathes(char* dumpfile_path, char* configfile_path) {
    if (!dumpfile_path && !configfile_path) {
        AM_ERROR("NULL pointer in AM_SetPathes.\n");
        return ME_BAD_PARAM;
    }

    if (dumpfile_path) {
        if (strlen(dumpfile_path) >= DAMF_MAX_FILENAME_LEN) {
            AM_ERROR("dumpfile_path(%s) exceed max length.\n", dumpfile_path);
            return ME_ERROR;
        }
        strncpy(g_AMPathes[AM_PathDump], dumpfile_path, sizeof(g_AMPathes[AM_PathDump]));
    }

    if (configfile_path) {
        if (strlen(configfile_path) >= DAMF_MAX_FILENAME_LEN) {
            AM_ERROR("configfile_path(%s) exceed max length.\n", configfile_path);
            return ME_ERROR;
        }
        strncpy(g_AMPathes[AM_PathConfig], configfile_path, sizeof(g_AMPathes[AM_PathConfig]));
    }
    return ME_OK;
}

void AM_Util_Init()
{
    AM_GetDefaultPathes();
}

char* AM_GetPath(EAM_Path path_type)
{
    switch (path_type) {
        case AM_PathSdcard:
        case AM_PathDump:
        case AM_PathConfig:
            return g_AMPathes[path_type];
        default:
            AM_ERROR("BAD path_type %d, in AM_GetPath.\n", path_type);
    }
    return NULL;
}

AM_ERR AM_SetPath(EAM_Path path_type, char* string)
{
    if (!string) {
        AM_ERROR("NULL pointer in AM_SetPath.\n");
        return ME_BAD_PARAM;
    }

    switch (path_type) {
        case AM_PathSdcard:
        case AM_PathDump:
        case AM_PathConfig:
            if (strlen(string) >= DAMF_MAX_FILENAME_LEN) {
                AM_ERROR("string(%s) exceed max length in AM_SetPath.\n", string);
                return ME_ERROR;
            }
            strncpy(g_AMPathes[AM_PathDump], string, sizeof(g_AMPathes[AM_PathDump]));
            return ME_OK;
        default:
            AM_ERROR("BAD path_type %d, in AM_GetPath.\n", path_type);
    }
    return ME_BAD_PARAM;
}

//for CLUT related function
AM_U8 MapColorIndexYCbCr422_YCbCrAlpha(AM_U32 yuva, AM_U8 transparent_index)
{
    AM_U8 index = 0;

    if (!(yuva & 0xff)) {
        return transparent_index;
    }

    index = ((yuva >> 24) & 0xf0) | ((yuva >> (16 + 4)) & 0x0c) | ((yuva >> (8 + 6)) & 0x03);

    if (index == transparent_index) {
        if (0 == index) {
            index = 1;
        } else {
            index --;
        }
    }

    return index;
}

//for CLUT related function
void GeneratePresetYCbCr422_YCbCrAlphaCLUT(AM_U32* p_clut, AM_U8 transparent_index, AM_U8 reserve_transparent)
{
    AM_U32 index;

    //gegerate clut
    for (index = 0; index < 256; index ++) {
        //p_clut[index] = ((index << 24) & 0xf0000000) | ((index << (16 + 4)) & 0x00c00000)  | ((index << (8 + 6)) & 0x0000c000) | 0xff;
        p_clut[index] = ((index << 16) & 0x00f00000) | ((index << (8 + 4)) & 0x0000c000)  | ((index << (0 + 6)) & 0x000000c0) | 0xff000000;
    }

    //reserve transparent color
    if (reserve_transparent) {
        p_clut[transparent_index] = 0x0;
    }
}

//
//Bilinear Interpolation Scale
//
static int do_stretch_bilinear_scale(unsigned char *src, unsigned char *dst, int src_stride, int dst_stride, int src_width, int src_height, int dst_width,int dst_height)
{
    int sw = src_width - 1, sh = src_height - 1, dw = dst_width - 1, dh = dst_height - 1;
    int B, N, x, y;
    unsigned char * pLinePrev, *pLineNext;
    unsigned char * pDest;
    unsigned char * pA, *pB, *pC, *pD;

    unsigned char *next_dstline = dst;
    unsigned int dw_x_dh = dw *dh;

    for( int i = 0; i <= dh; ++i ){
        pDest = (unsigned char  * )next_dstline;
        next_dstline += dst_stride;
        y = i * sh / dh;
        N = dh - i * sh % dh;
        pLinePrev = (unsigned char * )&src[src_stride * y];
        pLineNext = ( N == dh ) ? pLinePrev : (unsigned char * )&pLinePrev[src_stride];

        int dw_x_N = dw *N;
        for ( int j = 0; j <= dw; ++j ){
            x = j * sw / dw;
            B = dw - j * sw % dw;
            pA = pLinePrev + x;
            pB = pA + 1;
            pC = pLineNext + x;
            pD = pC + 1;
            if ( B == dw ){
                pB = pA;
                pD = pC;
            }
            *pDest++ = (unsigned char)( int )(
                ( B * N * ( *pA - *pB - *pC + *pD ) + dw_x_N * *pB
                + dh * B * *pC + (dw_x_dh - dh * B - dw_x_N ) * *pD
                + dw_x_dh / 2 ) / ( dw_x_dh)
                );
        }
    }
    return 0;
}
static int stretch_bilinear_scale_yuv420p(img_struct_t *src_img,img_struct_t *dst_img)
{
    do_stretch_bilinear_scale(src_img->yuv[0],dst_img->yuv[0],src_img->stride[0],dst_img->stride[0],src_img->width,src_img->height,dst_img->width,dst_img->height);
    do_stretch_bilinear_scale(src_img->yuv[1],dst_img->yuv[1],src_img->stride[1],dst_img->stride[1],src_img->width/2,src_img->height/2,dst_img->width/2,dst_img->height/2);
    do_stretch_bilinear_scale(src_img->yuv[2],dst_img->yuv[2],src_img->stride[2],dst_img->stride[2],src_img->width/2,src_img->height/2,dst_img->width/2,dst_img->height/2);
    return 0;
}

static AM_INT _setupDatagramSocket(AM_U32 localAddr,  AM_U16 localPort, bool makeNonBlocking)
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

/*"progressive" is for downscaling only.
*/
static int ProgressiveScale_yuv420p(img_struct_t *src_img,img_struct_t *dst_img)
{
    int w = src_img->width;
    int h = src_img->height;
    int target_w = dst_img->width;
    int target_h = dst_img->height;
    img_struct_t *src_img_tmp = src_img;
    img_struct_t *scratch_img = NULL;
//    int ret;

    bool is_w_downscale = (target_w < w);
    bool is_h_downscale = (target_h < h);

    for(;;){
        if (is_w_downscale && w > target_w) {
            w >>= 1;
            w = (w < target_w) ? target_w : w;
        }else{
            w = target_w;
        }
        if (is_h_downscale && h > target_h) {
            h >>= 1;
            h = (h < target_h) ? target_h : h;
        }else{
            h = target_h;
        }

        if(w == target_w && h == target_h){
            stretch_bilinear_scale_yuv420p(src_img_tmp,dst_img);
            if(src_img_tmp != src_img){
                free(src_img_tmp->yuv[0]);
                free(src_img_tmp);
            }
            break;
        }
        scratch_img = (img_struct_t *)malloc(sizeof(img_struct_t));
        if(!scratch_img){
            AM_ERROR("ProgressiveScale_yuv420p --- failed to alloc scratch image buffer\n");
            return -1;//NO_MEM
        }
        scratch_img->yuv[0] = (unsigned char *)malloc(w * h * 3/2);
        if(!scratch_img->yuv[0]){
            AM_ERROR("ProgressiveScale_yuv420p --- failed to alloc scratch image yuv buffer\n");
            free(scratch_img);
            return -1;//NO_MEM
        }
        scratch_img->yuv[1] = scratch_img->yuv[0] + w * h;
        scratch_img->yuv[2] = scratch_img->yuv[1] + w * h /4;
        scratch_img->width = w;
        scratch_img->height = h;
        scratch_img->stride[0] = w;
        scratch_img->stride[1] = w/2;
        scratch_img->stride[2] = w/2;
        stretch_bilinear_scale_yuv420p(src_img_tmp,scratch_img);
        if(src_img_tmp != src_img){
           free(src_img_tmp->yuv[0]);
           free(src_img_tmp);
        }
        src_img_tmp = scratch_img;
    }
    return 0;
}

static AM_UINT __strcnt(char* str, char t)
{
    AM_UINT cnt = 0;
    char* tmp = str;

    if (!str) {
        AM_ERROR("NULL input\n");
        return 0;
    }

    do {
        tmp = strchr(tmp, t);
        if (tmp) {
            cnt ++;
            if ((0x0) != (*(tmp + 1))) {
                tmp ++;
            } else {
                break;//str end
            }
        } else {
            break;
        }
    } while (tmp);

    return cnt;
}

static char* __strchr_n(char* str, char t, AM_UINT cnt)
{
    char* tmp = str;

    if (!str) {
        AM_ERROR("NULL input\n");
        return NULL;
    }

    while (cnt) {
        tmp = strchr(tmp, t);
        if (!tmp) {
            return NULL;
        }
        cnt --;
        if (!cnt) {
            return tmp;//find it
        }

        if ((0x0) != (*(tmp + 1))) {
            tmp ++;
        } else {
            return NULL;//str end
        }
    }

    return NULL;
}

static char* __get_ip_addr_from_rtsp_url(char* rtsp_url, AM_UINT& len)
{
    char* tmp = rtsp_url, *tmp1 = NULL, *tmp2;
    if (!rtsp_url) {
        AM_ERROR("NULL input\n");
        len = 0;
        return NULL;
    }

    tmp1 = strchr(tmp, ':');
    if (!tmp1) {
        AM_ERROR("no ':' found in rtsp url %s\n", rtsp_url);
        len = 0;
        return NULL;
    }

    if (('/' != tmp1[1]) || ('/' != tmp1[2])) {
        AM_ERROR("no '//' found in rtsp url %s\n", rtsp_url);
        len = 0;
        return NULL;
    }

    tmp = tmp1 + 3;
    tmp1 = strchr(tmp, ':');
    tmp2 = strchr(tmp, '/');
    if (!tmp1 && tmp2) {
        //find one, return
        len = tmp2 - tmp;
        return tmp;
    } else if (tmp2 && tmp1) {
        //have both
        if ((AM_UINT)tmp1 > (AM_UINT)tmp2) {
            len = tmp2 - tmp;
            return tmp;
        } else {
            len = tmp1 - tmp;
            return tmp;
        }
    } else if (tmp1 && !tmp2) {
        AM_ERROR("BAD rtsp url %s\n", rtsp_url);
        len = 0;
        return NULL;
    } else {
        //cannot find '/' and ':'
        AM_WARNING("is this url(%s) correct?\n", rtsp_url);
        len = (AM_UINT)rtsp_url + strlen(rtsp_url) - (AM_UINT)tmp;
        return tmp;
    }

    return NULL;
}

//-----------------------------------------------------------------------
//
// CWatcher
//
//-----------------------------------------------------------------------

class CWatcher: public CObject, public IWatcher, public IActiveObject
{
    typedef CObject inherited;

    enum {
        CMD_ADD_WATCH_ITEM = CMD_LAST,
        CMD_REMOVE_WATCH_ITEM,
    };

    typedef enum {
        EWatcherState_idle,
        EWatcherState_no_item_to_watch,
        EWatcherState_running,
        EWatcherState_halt,
        EWatcherState_error,
    } EWatcherState;

    typedef struct {
        AM_INT export_fd;//for external use
        AM_U8 export_type;

        AM_U8 reserved0;
        AM_U8 is_setup;
        AM_U8 type;

        char* p_name;
        AM_UINT watch_mask;

        void* p_content;

        AM_INT fd;
        AM_INT wd;
    } SWatchItemInternal;

public:
    static IWatcher* Create();
    virtual void Delete();

protected:
    CWatcher():
        mpWorkQueue(NULL),
        mpName(NULL),
        mbRun(true),
        mfNotifyCB(NULL),
        mpNotifyContext(NULL),
        mMaxFd(-1)
    {
        mPipeFd[0] = -1;
        mPipeFd[1] = -1;
    }

    AM_ERR Construct();
    virtual ~CWatcher();

public:
    virtual SWatchItem* AddWatchItem(char* name, AM_UINT watch_mask, AM_INT fd, AM_U8 type);
    virtual AM_ERR RemoveWatchItem(SWatchItem* content);

    virtual AM_ERR StartWatching(void* thiz, TFWatchCallbackNotify notify_cb);
    virtual AM_ERR StopWatching();

    //IActiveObject
    virtual const char *GetName() {return mpName;}
    virtual void OnRun();
    virtual void OnCmd(CMD& cmd) { /*todo*/ }

public:
    void* GetInterface(AM_REFIID refiid);

protected:
    bool ProcessCmd(CMD& cmd);

private:
    void clearAllItems();
    AM_ERR destroyWatchedItem(SWatchItemInternal* p_item);
    AM_ERR setupWatchedItem(SWatchItemInternal* p_item);
    AM_ERR processWatchedItem(SWatchItemInternal* p_item);

protected:
    CWorkQueue* mpWorkQueue;
    const char *mpName;
    bool mbRun;
    EWatcherState msState;

    TFWatchCallbackNotify mfNotifyCB;
    void* mpNotifyContext;

private:
    CDoubleLinkedList mWatchList;

private:
    //fd related
    AM_INT mPipeFd[2];
    AM_INT mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;
};

IWatcher* CWatcher::Create()
{
    CWatcher *result = new CWatcher();
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void* CWatcher::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IWatcher)
        return (IWatcher*)this;

    return inherited::GetInterface(refiid);
}

AM_ERR CWatcher::Construct()
{
    //DSetModuleLogConfig(LogModulePBEngine);

    if ((mpWorkQueue = CWorkQueue::Create((IActiveObject*)this)) == NULL) {
        AM_ERROR("Create CWorkQueue fail.\n");
        return ME_NO_MEMORY;
    }

    pipe(mPipeFd);

    AMLOG_INFO("before CWatcher:: mpWorkQueue->Run().\n");
    AM_ASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    AMLOG_INFO("after CWatcher:: mpWorkQueue->Run().\n");

    return ME_OK;
}

void CWatcher::Delete()
{
    if (mbRun) {
        char wake_char = 'b';
        AMLOG_WARN("Watch thread not stopped, exit here\n");
        write(mPipeFd[1], &wake_char, 1);

        mpWorkQueue->SendCmd(CMD_STOP, NULL);
    }

    clearAllItems();

    if (0 <= mPipeFd[0]) {
        close(mPipeFd[0]);
        mPipeFd[0] = -1;
    }

    if (0 <= mPipeFd[1]) {
        close(mPipeFd[1]);
        mPipeFd[1] = -1;
    }
}

void CWatcher::clearAllItems()
{
    CDoubleLinkedList::SNode* pnode;
    AMLOG_DEBUG("CStreammingServerManager::deleteServer, after delete p_server.\n");

    //clean servers
    SWatchItemInternal* p_item;
    pnode = mWatchList.FirstNode();
    while (pnode) {
        p_item = (SWatchItemInternal*)(pnode->p_context);
        pnode = mWatchList.NextNode(pnode);
        AM_ASSERT(p_item);
        if (p_item) {
            destroyWatchedItem(p_item);
        } else {
            AM_ASSERT("NULL pointer here, something would be wrong.\n");
        }
    }
}

CWatcher::~CWatcher()
{
    if (mbRun) {
        char wake_char = 'b';
        AMLOG_WARN("Watch thread not stopped, exit here\n");
        write(mPipeFd[1], &wake_char, 1);

        mpWorkQueue->SendCmd(CMD_STOP, NULL);
    }

    clearAllItems();

    if (0 <= mPipeFd[0]) {
        close(mPipeFd[0]);
        mPipeFd[0] = -1;
    }

    if (0 <= mPipeFd[1]) {
        close(mPipeFd[1]);
        mPipeFd[1] = -1;
    }
}

SWatchItem* CWatcher::AddWatchItem(char* name, AM_UINT watch_mask, AM_INT fd, AM_U8 type)
{
    SWatchItemInternal* p_item = NULL;

    if (!name && fd <0) {
        AM_ERROR("BAD params name %p, fd %d\n", name, fd);
        return NULL;
    }

    p_item = (SWatchItemInternal*)malloc(sizeof(SWatchItemInternal));

    if (p_item) {

        memset(p_item, 0x0, sizeof(SWatchItemInternal));
        if (name) {
            p_item->p_name = (char*)malloc(strlen(name) + 4);
            if (p_item->p_name) {
                strcpy(p_item->p_name, name);
            } else {
                AM_ERROR("NO_MEMORY, name %s, strlen %d\n", name, strlen(name));
                free(p_item);
                return NULL;
            }
        }

        p_item->fd = fd;
        p_item->watch_mask = watch_mask;
        p_item->type = type;

    } else {
        AM_ERROR("NO_MEMORY\n");
    }

    //wake up watch thread, send cmd to it
    char wake_char = 'c';
    write(mPipeFd[1], &wake_char, 1);
    AMLOG_WARN("before CMD_ADD_WATCH_ITEM\n");
    mpWorkQueue->SendCmd(CMD_ADD_WATCH_ITEM, (void*)p_item);
    AMLOG_WARN("after CMD_ADD_WATCH_ITEM\n");

    return (SWatchItem*)p_item;
}

AM_ERR CWatcher::RemoveWatchItem(SWatchItem* content)
{
    if (!content) {
        AM_ERROR("BAD params %p\n", content);
        return ME_ERROR;
    }

    //wake up watch thread, send cmd to it
    char wake_char = 'd';
    write(mPipeFd[1], &wake_char, 1);

    mpWorkQueue->SendCmd(CMD_REMOVE_WATCH_ITEM, (void*)content);
    return ME_OK;
}

bool CWatcher::ProcessCmd(CMD& cmd)
{
    AMLOG_WARN("****CWatcher::ProcessCmd, cmd.code %d.\n", cmd.code);
    AM_ASSERT(mpWorkQueue);
    SWatchItemInternal* p_item;
    char char_buffer;

    //bind with pipe fd
    read(mPipeFd[0], &char_buffer, sizeof(char_buffer));
    //AMLOG_INFO("****CWatcher::ProcessCmd, cmd.code %d, char %c.\n", cmd.code, char_buffer);

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            msState = EWatcherState_halt;
            mpWorkQueue->CmdAck(ME_OK);
            break;

        case CMD_START:
            msState = EWatcherState_no_item_to_watch;
            mpWorkQueue->CmdAck(ME_OK);
            break;

        case CMD_ADD_WATCH_ITEM:
            AM_ASSERT(cmd.pExtra);
            p_item = (SWatchItemInternal*)cmd.pExtra;
            if (p_item) {
                AMLOG_WARN("CWatcher::ProcessCmd, CMD_ADD_WATCH_ITEM, %p, type %d, watch_mask 0x%08x, fd %d, p_content %p.\n", p_item, p_item->type, p_item->watch_mask, p_item->fd, p_item->p_content);

                //need add socket into set
                AM_ASSERT(p_item->fd >= 0);
                FD_SET(p_item->fd, &mAllSet);

                mWatchList.InsertContent(NULL, (void*)p_item, 0);

                p_item->export_fd = p_item->fd;
                p_item->export_type = p_item->type;
                mpWorkQueue->CmdAck(ME_OK);
            } else {
                AM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(ME_BAD_COMMAND);
            }
            break;

        case CMD_REMOVE_WATCH_ITEM:
            AM_ASSERT(cmd.pExtra);
            p_item = (SWatchItemInternal*)cmd.pExtra;
            if (p_item) {
                AMLOG_INFO("CWatcher::ProcessCmd, CMD_REMOVE_WATCH_ITEM, %p, type %d, watch_mask 0x%08x, fd %d, p_content %p.\n", p_item, p_item->type, p_item->watch_mask, p_item->fd, p_item->p_content);

                //need add socket into set
                AM_ASSERT(p_item->fd >= 0);
                FD_CLR(p_item->fd, &mAllSet);

                mWatchList.RemoveContent((void*)p_item);
                mpWorkQueue->CmdAck(ME_OK);

                if (0 == mWatchList.NumberOfNodes()) {
                    msState = EWatcherState_no_item_to_watch;
                }
            } else {
                AM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(ME_BAD_COMMAND);
            }
            break;

        default:
            AM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }
    return false;
}

AM_ERR CWatcher::setupWatchedItem(SWatchItemInternal* p_item)
{
//    AM_ERR err;
    AM_ASSERT(p_item);
    if (!p_item) {
        AM_ERROR("Internal Error, NULL p_item\n");
        return ME_BAD_PARAM;
    }

    switch (p_item->type) {
        case WATCH_TYPE_SOCKET:
            //do nothing
            break;

        case WATCH_TYPE_DIR:
            //to do
            AM_ERROR("not enabled code here.\n");
/*
            AM_ASSERT(p_item->p_name);
            if (p_item->p_name) {
                if (!p_item->is_setup) {
                    p_item->fd = inotify_init();
                    p_item->wd = inotify_add_watch (p_item->fd, p_item->name, p_item->watch_mask);
                }
            }*/
            break;

        default:
            AM_ERROR("BAD type %d\n", p_item->type);
            return ME_BAD_PARAM;
            break;
    }

    return ME_OK;
}

AM_ERR CWatcher::destroyWatchedItem(SWatchItemInternal* p_item)
{
//    AM_ERR err;
    AM_ASSERT(p_item);
    if (!p_item) {
        AM_ERROR("Internal Error, NULL p_item\n");
        return ME_BAD_PARAM;
    }

    switch (p_item->type) {
        case WATCH_TYPE_SOCKET:
            //do nothing
            break;

        case WATCH_TYPE_DIR:
            //to do
            AM_ERROR("not enabled code here.\n");
            //inotify_rm_watch(p_item->fd, p_item->watch_mask);
            break;

        default:
            AM_ERROR("BAD type %d\n", p_item->type);
            break;
    }

    if (p_item->p_name) {
        free(p_item->p_name);
    }
    if (p_item->p_content) {
        free(p_item->p_content);
    }

    if (p_item->fd) {
        close(p_item->fd);
    }

    free(p_item);
    return ME_OK;
}

AM_ERR CWatcher::processWatchedItem(SWatchItemInternal* p_item)
{
//    AM_ERR err;
    AM_ASSERT(p_item);
    if (!p_item) {
        AM_ERROR("Internal Error, NULL p_item\n");
        return ME_BAD_PARAM;
    }

    switch (p_item->type) {
        case WATCH_TYPE_SOCKET:

            break;

        case WATCH_TYPE_DIR:
            //to do
            AM_ERROR("not enabled code here.\n");

            break;

        default:
            AM_ERROR("BAD type %d\n", p_item->type);
            break;
    }

    return ME_OK;
}

void CWatcher::OnRun()
{
    CMD cmd;
    AM_INT alive_watched_item_num = 0;
    AM_INT nready = 0;
    SWatchItemInternal* p_item;
    CDoubleLinkedList::SNode* pnode;
    mpWorkQueue->CmdAck(ME_OK);
    mbRun = true;

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];

    //init state
    msState = EWatcherState_idle;

    while (mbRun) {

        AMLOG_WARN("CWatcher::OnRun start switch state %d.\n", msState);

        switch (msState) {

            case EWatcherState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case EWatcherState_no_item_to_watch:
                //scan if there's item
                pnode = mWatchList.FirstNode();

                while (pnode) {
                    p_item = (SWatchItemInternal*)pnode->p_context;
                    AM_ASSERT(p_item);
                    if (NULL == p_item) {
                        AM_ERROR("Fatal error, No watch_item? must not get here.\n");
                        break;
                    }

                    //add to FD set
                    AM_ASSERT(p_item->fd >= 0);
                    if (p_item->fd >= 0) {
                        FD_SET(p_item->fd, &mAllSet);
                        mMaxFd = (mMaxFd < p_item->fd) ? p_item->fd : mMaxFd;
                        alive_watched_item_num ++;
                    }

                    pnode = mWatchList.NextNode(pnode);
                }

                if (alive_watched_item_num) {
                    AMLOG_INFO("There's some(%d) item need to be watched, transit to running state.\n", alive_watched_item_num);
                    msState = EWatcherState_running;
                    break;
                }
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case EWatcherState_running:
                AM_ASSERT(alive_watched_item_num > 0);
                mReadSet = mAllSet;

                AMLOG_DEBUG("[Watcher]: before select.\n");
                nready = select(mMaxFd+1, &mReadSet, NULL, NULL, NULL);
                AMLOG_DEBUG("[Watcher]: after select.\n");

                if (0 > nready) {
                    AMLOG_WARN("[Watcher]: select return %d < 0?\n", nready);
                    msState = EWatcherState_error;
                    break;
                } else if (0 == nready) {
                    AMLOG_WARN("[Watcher]: select return == 0?\n");
                    break;
                }

                //process cmd
                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    AMLOG_DEBUG("[Watcher]: from pipe fd.\n");
                    //some cmds, process cmd first
                    while (mpWorkQueue->MsgQ()->PeekMsg(&cmd,sizeof(cmd))) {
                        ProcessCmd(cmd);
                    }
                    nready --;
                    if (EWatcherState_running != msState) {
                        AMLOG_INFO(" transit from EWatcherState_running to state %d.\n", msState);
                        break;
                    }
                    if (nready <= 0) {
                        //read done
                        break;
                    }
                }

                //scan the list
                pnode = mWatchList.FirstNode();

                while (pnode) {
                    p_item = (SWatchItemInternal*)pnode->p_context;
                    AM_ASSERT(p_item);
                    if (NULL == p_item) {
                        AM_ERROR("Fatal error(NULL == p_item), must not get here.\n");
                        break;
                    }

                    AM_ASSERT(0 <= p_item->fd);
                    if (0 <= p_item->fd) {
                        if (FD_ISSET(p_item->fd, &mReadSet)) {
                            nready --;
                            //new client's request comes
                            AMLOG_INFO("new request comes.\n");
                            if (mfNotifyCB && mpNotifyContext) {
                                mfNotifyCB(mpNotifyContext, (SWatchItem*)p_item, 0);
                            }
                        }
                    }

                    if (nready <= 0) {
                        //read done
                        break;
                    }
                    pnode = mWatchList.NextNode(pnode);
                }
                break;

            case EWatcherState_halt:
                //todo
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case EWatcherState_error:
                //todo
                AM_ERROR("NEED implement this case.\n");
                //for
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            default:
                AM_ERROR("Error, BAD state %d\n", msState);
                break;
        }
    }
}

AM_ERR CWatcher::StartWatching(void* thiz, TFWatchCallbackNotify notify_cb)
{
    AM_ERR err;
    AM_ASSERT(mpWorkQueue);
    mfNotifyCB = notify_cb;
    mpNotifyContext = thiz;

    AMLOG_INFO("CWatcher::StartWatching start.\n");
    //wake up watch thread, post cmd to it
    char wake_char = 'a';
    write(mPipeFd[1], &wake_char, 1);

    err = mpWorkQueue->SendCmd(CMD_START, NULL);
    AMLOG_INFO("CWatcher::StartWatching end, ret %d.\n", err);
    return err;
}

AM_ERR CWatcher::StopWatching()
{
    AM_ERR err;
    AM_ASSERT(mpWorkQueue);

    AMLOG_INFO("CWatcher::StopWatching start.\n");
    //wake up watch thread, post cmd to it
    char wake_char = 'b';
    write(mPipeFd[1], &wake_char, 1);

    err = mpWorkQueue->SendCmd(CMD_STOP, NULL);
    AMLOG_INFO("CWatcher::StopWatching end, ret %d.\n", err);
    return err;
}

AM_ERR CSimpleDataBase::setupMsgPort(AM_U16 listen_port)
{
    if (mListenSocket >= 0) {
        AM_ERROR("mListenSocket already setup, %d\n", mListenSocket);
        return ME_OK;
    }

    mListenSocket = _setupDatagramSocket(INADDR_ANY, listen_port, false);
    if (mListenSocket < 0) {
        AM_ERROR("setup socket fail, ret %d, port %hu.\n", mListenSocket, listen_port);
        return ME_ERROR;
    }
    mListenPort = listen_port;

    AM_WARNING("listening socket setup %d, port %hu\n", mListenSocket, mListenPort);
    return ME_OK;
}

AM_ERR CSimpleDataBase::Start(void* thiz, TFConfigUpdateNotify notify_cb, AM_U16 listen_port)
{
    AM_ERR err;

    AM_ASSERT(!mbDatabaseRunning);
    if (mbDatabaseRunning) {
        AM_ERROR("CSimpleDataBase::Start already invoked, please check code\n");
        return ME_BAD_STATE;
    }

    mfUpdateNotify = notify_cb;
    mpUpdateNotityContext = thiz;

    //setup listen port
    err = setupMsgPort(listen_port);
    if (ME_OK != err) {
        AM_ERROR("setupMsgPort fail.\n");
        return err;
    }

    //setup watch loop
    AM_ASSERT(!mpWatcher);
    if (!mpWatcher) {
        mpWatcher = CWatcher::Create();
        if (NULL == mpWatcher) {
            AM_ERROR("CWatcher::Create() fail.\n");
            return ME_ERROR;
        }
    }

    mpWatcher->AddWatchItem((char*)"msgPort", 0x0, mListenSocket, WATCH_TYPE_SOCKET);

    err = mpWatcher->StartWatching((void*) this, Callback);
    AM_ASSERT(ME_OK == err);

    mbWatcherRunning = 1;
    mbDatabaseRunning = 1;

    return ME_OK;
}

AM_ERR CSimpleDataBase::Stop()
{
    AM_ERR err;
    if (!mbDatabaseRunning) {
        AM_ERROR("CSimpleDataBase::Stop in BAD state, it has not been started, or stopped already, please check code\n");
        return ME_BAD_STATE;
    }

    if (mpWatcher && mbWatcherRunning) {
        err = mpWatcher->StopWatching();
        AM_ASSERT(ME_OK == err);
        mbWatcherRunning = 0;
    }

    mbDatabaseRunning = 0;
    return ME_OK;
}

AM_ERR CSimpleDataBase::LoadConfigFile(char* filename)
{
    return ME_OK;
}
AM_ERR CSimpleDataBase::SaveConfigFile(char* filename, AM_U8 only_save_alive)
{
    return ME_OK;
}

SSimpleDataPiece* CSimpleDataBase::QueryData(SSimpleDataPiece* p_pre)
{
    SSimpleDataPiece* piece = NULL;
    CDoubleLinkedList::SNode* pnode = NULL;

    //scan if there's item
    pnode = mDataList.FirstNode();

    while (pnode) {
        piece = (SSimpleDataPiece*)pnode->p_context;
        AM_ASSERT(piece);
        if (p_pre == piece) {
            pnode = mDataList.NextNode(pnode);
            piece = (SSimpleDataPiece*)pnode->p_context;
            return piece;
        } else if (NULL == piece) {
            AM_ERROR("Fatal error, NULL piece? must not get here.\n");
            return NULL;
        }

        pnode = mDataList.NextNode(pnode);
    }

    AM_ERROR("can not find matched data piece.\n");

    return NULL;
}

AM_UINT CSimpleDataBase::TotalDataCount()
{
    return mnDataPieceNumber;
}

AM_ERR CSimpleDataBase::SetAlive(SSimpleDataPiece* p, AM_U8 is_alive)
{
    if (isInDataBase(p)) {
        p->is_alive = is_alive;
    }

    AM_ERROR("NOT in data base, please check code.\n");
    return ME_ERROR;
}

AM_ERR CSimpleDataBase::SetAlive(char* name, AM_U8 is_alive)
{

    return ME_OK;
}

bool CSimpleDataBase::isInDataBase(SSimpleDataPiece* p)
{
    SSimpleDataPiece* piece = NULL;
    CDoubleLinkedList::SNode* pnode = NULL;

    //scan if there's item
    pnode = mDataList.FirstNode();

    while (pnode) {
        piece = (SSimpleDataPiece*)pnode->p_context;
        AM_ASSERT(piece);
        if (p == piece) {
            return true;
        } else if (NULL == piece) {
            AM_ERROR("Fatal error, NULL piece? must not get here.\n");
            return false;
        }

        pnode = mDataList.NextNode(pnode);
    }

    AM_ERROR("NOT in data base, please check code.\n");
    return false;
}

SSimpleDataPiece* CSimpleDataBase::findDataPiece(char* name)
{
    SSimpleDataPiece* piece = NULL;
    CDoubleLinkedList::SNode* pnode = NULL;

    //scan if there's item
    pnode = mDataList.FirstNode();

    while (pnode) {
        piece = (SSimpleDataPiece*)pnode->p_context;
        AM_ASSERT(piece);
        if (piece) {
            if (!strcmp((char*)piece->p_name, name)) {
                return piece;
            }
        } else if (NULL == piece) {
            AM_ERROR("Fatal error, NULL piece? must not get here.\n");
            return NULL;
        }

        pnode = mDataList.NextNode(pnode);
    }

    //AM_ERROR("NOT in data base, please check code.\n");
    return NULL;
}

//like rtsp://10.0.0.2/stream0+stream1+stream2+stream3 ...
void CSimpleDataBase::parseMultipleRTSPAddr(char* url, SSimpleDataPiece* piece)
{
    AM_UINT cnt = 0, i = 0, cp_size = 0;
    char* p_stream = NULL, *p_stream1 = NULL;
    if (!url || !piece) {
        AM_ERROR("NULL input\n");
        return;
    }

    p_stream = __strchr_n(url, '/', 3);
    if (!p_stream) {
        cnt = __strcnt(url, '/');
        AM_ASSERT(2 == cnt);
        AM_ERROR("!!no stream name found, url %s\n", url);
        return;
    }

    p_stream ++;// skip '/'

    cnt = __strcnt(url, '+');
    if (cnt) {
        //multi's case
        for (i = 0; i < cnt; i ++) {
            p_stream1 = strchr(p_stream, '+');
            AM_ASSERT(p_stream1);
            if (p_stream1) {
                cp_size = (AM_UINT)p_stream1 - (AM_UINT)p_stream;
                AM_ASSERT(cp_size < DMAX_STREAM_NAME_LEN);
                if (cp_size >= DMAX_STREAM_NAME_LEN) {
                    cp_size = DMAX_STREAM_NAME_LEN - 1;
                }
                memcpy(piece->stream[i], p_stream, cp_size);
                piece->stream[i][cp_size] = 0x0;
            } else {
                AM_ERROR("Internal error, abort parsing\n");
                return;
            }
            p_stream = p_stream1 + 1;
            piece->stream_count ++;
        }

        //last one
        if (0x0 != p_stream1[1]) {
            p_stream1 ++;
            cp_size = strlen(p_stream1);
            AM_ASSERT(cp_size < DMAX_STREAM_NAME_LEN);
            if (cp_size >= DMAX_STREAM_NAME_LEN) {
                cp_size = DMAX_STREAM_NAME_LEN - 1;
            }
            memcpy(piece->stream[i], p_stream1, cp_size);
            piece->stream[i][cp_size] = 0x0;
            piece->stream_count ++;
        }

    } else {
        //single's case
        piece->stream_count = 1;
        cp_size = strlen(p_stream);
        AM_ASSERT(cp_size < DMAX_STREAM_NAME_LEN);
        if (cp_size >= DMAX_STREAM_NAME_LEN) {
            cp_size = DMAX_STREAM_NAME_LEN - 1;
        }
        memcpy(piece->stream[0], p_stream, cp_size);
        piece->stream[0][cp_size] = 0x0;
    }
}

//\r\n
void CSimpleDataBase::parseStreamerRequest(AM_INT fd, AM_UINT type, AM_UINT& msg_type, SSimpleDataPiece*& piece)
{
    AM_ASSERT(fd >= 0);
    if (0 > fd) {
        AM_ERROR("invalid fd %d\n", fd);
        return;
    }

    if (WATCH_TYPE_SOCKET == type) {
        mCurParseStringBufferLen = read(fd, mParseStringBuffer, DMAX_PARSE_STRING_BUFFER_SIZE);
        AM_ASSERT(mCurParseStringBufferLen < (DMAX_PARSE_STRING_BUFFER_SIZE));
        //assert the string have a header
        //AM_ASSERT(!strcmp(DStrCPStartTag, mParseStringBuffer));
        AM_WARNING("recieve string:(%s), len %d\n", mParseStringBuffer, mCurParseStringBufferLen);
        if (!strncmp(DStrCPStartTag, mParseStringBuffer, strlen(DStrCPStartTag))) {
            parseOnePieceString(mParseStringBuffer + strlen(DStrCPStartTag), msg_type, piece);
        } else if (!strncmp(DStrCPIDRNotify, mParseStringBuffer, strlen(DStrCPIDRNotify))) {
            msg_type = AM_MSG_TYPE_IDR_NOTIFICATION;
        } else {
            AM_ERROR("missing string header(%s), read string(%s)\n", DStrCPStartTag, mParseStringBuffer);
        }
    } else if (WATCH_TYPE_DIR == type) {
        AM_ERROR("TODO process DIR/file related\n");
    } else {
        AM_ERROR("Unknown type %d\n", type);
    }
}

bool CSimpleDataBase::parseOnePieceString(char* start, AM_UINT& msg_type, SSimpleDataPiece*& piece)
{
    char* tmp, *tmp1, *tmp2;
    AM_UINT str_len = 0;
    char saved_char=0;

    AM_ASSERT(start);

    AM_WARNING("remove heaser, string %s\n", start);

    // get msg
    if (!strncmp(start, DStrCPName, strlen(DStrCPName))) {
        //name, to do
        AM_ERROR("please implement it\n");
        return true;
    } else if (!strncmp(start, DStrCPIPAddress, strlen(DStrCPIPAddress))) {
        tmp = start + strlen(DStrCPIPAddress);
        tmp1 = strchr(tmp, '[');
        if (tmp1) {
            *tmp1 = 0x0;
            AM_WARNING("get request %s\n", tmp);

            if (NULL == (piece = findDataPiece(tmp))) {
                piece = (SSimpleDataPiece*)malloc(sizeof(SSimpleDataPiece));
                if (piece) {
                    memset(piece, 0x0, sizeof(SSimpleDataPiece));
                } else {
                    AM_ERROR("NO Memory!\n");
                    *(tmp1) = '[';
                    return false;
                }
            }
            *(tmp1) = '[';

            //get data
            if (piece->p_name) {
                AM_WARNING("free previous url string\n");
                free(piece->p_name);
            }

            piece->p_name = (char*)malloc((AM_UINT)(tmp1 - tmp) + 4);
            if (piece->p_name) {
                memcpy(piece->p_name, tmp, (AM_UINT)(tmp1 - tmp));
                piece->p_name[(AM_UINT)(tmp1 - tmp)] = 0x0;
            } else {
                AM_ERROR("NO Memory, request size %d\n", (AM_UINT)(tmp1 - tmp) + 4);
                return false;
            }
        } else {
            AM_ERROR("Cannot find '[', input string(%s) error or corrupted\n", start);
            return false;
        }
    } else if (!strncmp(start, DStrCPRTSPAddress, strlen(DStrCPRTSPAddress))) {
        tmp = start + strlen(DStrCPRTSPAddress);
        tmp1 = strchr(tmp, '[');
        if (tmp1) {
            AM_WARNING("get request %s\n", tmp);

            tmp2 = __get_ip_addr_from_rtsp_url(tmp, str_len);
            if (tmp2) {
                saved_char = *(tmp2 + str_len);
                *(tmp2 + str_len) = 0x0;
                if (NULL == (piece = findDataPiece(tmp2))) {
                    piece = (SSimpleDataPiece*)malloc(sizeof(SSimpleDataPiece));
                    if (piece) {
                        memset(piece, 0x0, sizeof(SSimpleDataPiece));
                    } else {
                        AM_ERROR("NO Memory!\n");
                        *(tmp2 + str_len) = saved_char;
                        return false;
                    }
                }
            }
            *(tmp2 + str_len) = saved_char;

            //get data
            if (piece->p_name) {
                AM_WARNING("free previous url string\n");
                free(piece->p_name);
            }

            piece->p_name = (char*)malloc(str_len + 4);
            if (piece->p_name) {
                memcpy(piece->p_name, tmp2, str_len);
                piece->p_name[str_len] = 0x0;
            } else {
                AM_ERROR("NO Memory, request size %d\n", str_len + 4);
                return false;
            }

            //get data
            if (piece->p_url) {
                AM_WARNING("free previous url string\n");
                free(piece->p_url);
            }

            str_len += (AM_UINT)tmp2 - (AM_UINT)tmp;
            piece->p_url = (char*)malloc(str_len + 4);
            if (piece->p_url) {
                memcpy(piece->p_url, tmp, str_len);
                piece->p_url[str_len] = 0x0;
            } else {
                AM_ERROR("NO Memory, request size %d\n", str_len + 4);
                return false;
            }

            *tmp1 = 0x0;
            parseMultipleRTSPAddr(tmp, piece);
            *tmp1 = '[';

            msg_type = AM_MSG_TYPE_NEW_RTSP_URL;
        } else {
            AM_ERROR("Cannot find '[', input string(%s) error or corrupted\n", start);
            return false;
        }
    } else {
        AM_ERROR("un-recognized header, string %s\n", start);
        return false;
    }

    //must not comes here
    return false;
}

void CSimpleDataBase::destroyDataPiece(SSimpleDataPiece* p)
{
    if (p) {
        if (p->p_url) {
            free(p->p_url);
        }
        if (p->p_name) {
            free(p->p_name);
        }
        AM_ASSERT(!p->p_content);
        free(p);
    }
}

AM_ERR CSimpleDataBase::Callback(void* p, SWatchItem* owner, AM_UINT flag)
{
    CSimpleDataBase* thiz = (CSimpleDataBase*) p;
    if (!p) {
        AM_ERROR("NULL pointer\n");
        return ME_ERROR;
    }

    thiz->ProcessCallback(owner, flag);

    return ME_OK;
}

AM_ERR CSimpleDataBase::ProcessCallback(SWatchItem* owner, AM_UINT flag)
{
    SSimpleDataPiece* piece = NULL;
    AM_UINT msg_type = AM_MSG_TYPE_INVALID;

    //process callback
    parseStreamerRequest(owner->export_fd, owner->export_type, msg_type, piece);

    //notify app
    if (mfUpdateNotify) {
        mfUpdateNotify(mpUpdateNotityContext, msg_type, (void*)piece);
    }
    return ME_OK;
}

/////////////////CVoutEventListener/////////////////
//iav client: listen to hdmi plugged/unplugged event
static int connect_iav_server(const char *server_ip,const char *client_name, enum iav_client_type client_type)
{
     int                                    ret;
     int                                    socket_fd;
     struct hostent                          *hp;
     struct sockaddr_in                    server;
     struct iav_command_message    cmd;

     /* Create Socket */
     socket_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (socket_fd < 0) {
        fprintf(stderr, "Unable to create socket!\n");
        return -1;
     }

     if (server_ip == NULL) {
        hp = gethostbyname(LOCAL_HOST);
     } else {
        hp = gethostbyname(server_ip);
     }
     if (hp == NULL) {
        fprintf(stderr, "Unable to get host name!\n");
        close(socket_fd);
        return -1;
     }

     server.sin_family = AF_INET;
     bcopy((char *)hp->h_addr, (char *)&server.sin_addr.s_addr, hp->h_length);
     server.sin_port = htons(IAV_TCP_PORT);

     /* Connect to IAV Server */
     ret = connect(socket_fd, (struct sockaddr *)&server, sizeof(server));
     if (ret < 0) {
        fprintf(stderr, "Unable to connect to server!\n");
        close(socket_fd);
        return -1;
     } else {
        fprintf(stderr, "Connected to server!socket_fd %d\n",socket_fd);
     }

     /* Declare Name */
     memset(&cmd, 0, sizeof(cmd));
     cmd.type = IAV_SRV_INF_CLIENT_NAME;
     sprintf(cmd.payload.client_name.name, "%s", client_name);
     write(socket_fd, &cmd, sizeof(cmd));

     /* Declare Type */
     memset(&cmd, 0, sizeof(cmd));
     cmd.type = IAV_SRV_INF_CLIENT_TYPE;
     cmd.payload.client_type.type	= client_type;
     write(socket_fd, &cmd, sizeof(cmd));

     return socket_fd;
}

static void disconnect_iav_server(int socket_fd)
{
	close(socket_fd);
}

static void* iav_client_pthread_loop(void *argu)
{
    CVoutEventListener* pListener = (CVoutEventListener*)argu;
    if(pListener == NULL){
        AM_ERROR("Iav Client is not created?!\n");
        return NULL;
    }

    struct iav_command_message  msg;
    memset(&msg, 0, sizeof(msg));

    while(pListener->mIavClientpthreadLoop){
        int ret = read(pListener->mSocketFd, &msg, sizeof(msg));
        if(ret > 0){
            if(ret < (int)sizeof(msg)) {
                AM_ERROR("Message length(%d < %d) is not correct!\n", ret, sizeof(msg));
                break;
            }

            AM_WARNING("msg.type %d, msg.payload.notification.notification 0x%u.", msg.type, msg.payload.notification.notification);
            if(msg.type == IAV_SRV_NOTIFICATION){
                unsigned int notification = msg.payload.notification.notification;
                switch(notification){
                    case IAV_SRV_NOT_HDMI_PLUGIN:
                        AM_WARNING("Iav Client JpegDec, HDMI is plugged in, switch vout to HDMI\n");
                        ret = (pListener->CallbackFunc)(CVoutEventListener::HDMI_PLUGIN);
                        break;
                    case IAV_SRV_NOT_HDMI_REMOVE:
                        AM_WARNING("Iav Client JpegDec, HDMI is unplugged, switch vout to LCD\n");
                        ret = (pListener->CallbackFunc)(CVoutEventListener::HDMI_REMOVE);
                        break;
                    default:
                        ret = -1;
                        break;
                }
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}
////////////////////////
CVoutEventListener::CVoutEventListener(void* callback)
{
    mSocketFd = -1;
    mIavClientpthreadLoop = 0;
    AM_ASSERT(callback);
    CallbackFunc = (funcptr)callback;

    CreatIavClient();
}

CVoutEventListener::~CVoutEventListener()
{
    if(mIavClientpthreadLoop == 1){
        mIavClientpthreadLoop = 0;
        void* pv;
        pthread_join(mIavClientpthreadID, &pv);
    }
    if(mSocketFd >= 0){
        disconnect_iav_server(mSocketFd);
        mSocketFd = -1;
    }
}

int CVoutEventListener::CheckCurrentVout()
{
     int fd = -1;
     fd = open("/proc/ambarella/vout1_event", O_RDONLY);
     if(fd < 0){
        AM_ERROR("open /proc/ambarella/vout1_event failed\n");
        return -1;
     }
     struct amb_event events[255];
     struct amb_event result;
     int ret = read(fd, events, sizeof(events));
     if(ret < 0){
        close(fd);
        return -1;
     }
     close(fd);
     unsigned int sno = 0;
     memset((void*)&result, 0, sizeof(result));
     for(int i = 0; i < (int)(ret/sizeof(struct amb_event)); i++){
        if(events[i].sno > sno){
            sno = events[i].sno;
            result = events[i];
        }
     }

     if(result.sno == 0){
        AM_WARNING("vout LCD\n");
        return VOUT_LCD;
     }
     if(result.type == AMB_EV_VOUT_HDMI_PLUG){
        AM_WARNING("vout HDMI\n");
        return VOUT_HDMI;
     }else if(result.type == AMB_EV_VOUT_HDMI_REMOVE){
        AM_WARNING("vout LCD\n");
        return VOUT_LCD;
     }

     return -1;
}

void CVoutEventListener::CreatIavClient()
{
    mSocketFd = connect_iav_server(NULL, "VoutEventListener", IAV_CLN_VOUT);
    if(mSocketFd < 0){
        AM_ERROR("iav client AMPlayer connect iav server failed\n");
        return;
    }

    //set the socket to be non-block
    //fcntl(mSocketFd, F_SETFL, O_NONBLOCK);

    struct iav_command_message  cmd;
    memset(&cmd, 0, sizeof(cmd));

    cmd.type = IAV_SRV_REQ_NOTIFICATION;
    cmd.need_response = 1;
    cmd.payload.req_notification.req_notification = IAV_SRV_NOT_HDMI_PLUGIN;
    write(mSocketFd, &cmd, sizeof(cmd));

    mIavClientpthreadLoop = 1;
    int ret = pthread_create(&mIavClientpthreadID, NULL, iav_client_pthread_loop, (void*)this);
    if (ret != 0) {
        AM_ERROR("creat iav client AMPlayer thread failed\n");
        disconnect_iav_server(mSocketFd);
        mSocketFd = -1;
        mIavClientpthreadLoop = 0;
        return;
    }
    return;
}
