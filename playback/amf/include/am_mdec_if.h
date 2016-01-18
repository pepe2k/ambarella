
/**
 * am_mdec_if.h
 *
 * History:
 *    2011/12/08 - [GangLiu] created file
 *    2012/3/30- [Qingxiong Z] modify file
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_MDEC_IF_H__
#define __AM_MDEC_IF_H__
#include "am_param.h"

extern const AM_IID IID_IMDecControl;
class MdecInfo;
//-----------------------------------------------------------------------
//
// IMDecControl for outside
//
//-----------------------------------------------------------------------
class IMDecControl: public IMediaControl
{
public:

    typedef enum {
        SPEED_NORMAL,
        FAST_2x,
        FAST_4x,
        FAST_8x,
        FAST_16x,
        SLOW_2x,
        SLOW_4x,
        SLOW_8x,
        SLOW_16x,
    } MD_SPEED;

    typedef enum {
        TYPE_NONE = 0x0,
        TYPE_VIDEO = 0x01,
        TYPE_AUDIO = 0x10,//0x010
        TYPE_ALL = 0x11,//0x011
    } MDEC_STREAM;

    typedef enum {
        STATE_IDLE,
        STATE_PREPARED,
        STATE_ONRUNNING,
        STATE_SYNCED,
        STATE_ERROR,
        //DISCARD
        STATE_PAUSED,
        STATE_STOPPED,
        STATE_STEP,
        STATE_FAST,
    } MDENGINE_STATE;

    typedef enum {
        STATE_ALL_IDLE,
        STATE_INITIALIZED,
        STATE_RUN,
        STATE_ALL_STOPPED,
        STATE_COMPLETED,
        STATE_END,
        STATE_HAS_ERROR,
    } MD_STATE;


public:
    DECLARE_INTERFACE(IMDecControl, IID_IMDecControl);

    //api for NVR
    virtual AM_ERR EditSourceDone() = 0;
    virtual AM_ERR EditSource(AM_INT sourceGroup, const char* pFileName, AM_INT flag) = 0;
    virtual AM_ERR BackSource(AM_INT sourceGroup, AM_INT orderLevel, AM_INT flag) = 0;
    virtual AM_ERR DeleteSource(AM_INT sourceGroup, AM_INT flag) = 0;
    virtual AM_ERR SaveInputSource(char *streamName,char * inputAddress,int inputPort,int isRawUdp) = 0;
    virtual AM_ERR StopInputSource(char *streamName) = 0;
    virtual AM_ERR SaveSource(AM_INT index, const char* saveName, AM_INT flag) = 0;
    virtual AM_ERR StopSaveSource(AM_INT index, AM_INT flag) = 0;
    virtual AM_ERR StartAudioProxy(const char *rtspUrl, const char* saveName) = 0;
    virtual AM_ERR StopAudioProxy(const char *rtspUrl) = 0;
    virtual AM_ERR AddSource(const char *pFileName, AM_INT sourceIndex, AM_INT flag) = 0;
    virtual AM_ERR SelectAudio(AM_INT index) = 0;
    virtual AM_ERR TalkBack(AM_UINT enable, AM_UINT winIndexMask, const char* url) = 0;
    virtual AM_ERR ConfigWindow(AM_INT winIndex, CParam& param) = 0;
    virtual AM_ERR ConfigWinMap(AM_INT sourceIndex, AM_INT win, AM_INT zOrder) = 0;
    virtual AM_ERR PerformWinConfig() = 0;
    virtual AM_ERR AutoSwitch(AM_INT index) = 0;
    virtual AM_ERR SwitchSet(AM_INT index) = 0;

    virtual AM_ERR UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule, AM_UINT flush = 1) = 0;
    virtual AM_ERR PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y) = 0;
    virtual AM_ERR GetStreamInfo(AM_INT index, AM_INT* width, AM_INT* height) = 0;

    virtual AM_ERR Prepare() = 0;
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Stop() = 0;
    virtual AM_ERR DisconnectHw(AM_INT flag) = 0;
    virtual AM_ERR ReconnectHw(AM_INT flag) = 0;

    virtual AM_ERR SetTranscode(AM_UINT w, AM_UINT h, AM_UINT br) = 0;
    virtual AM_ERR SetTranscodeBitrate(AM_INT kbps) = 0;
    virtual AM_ERR SetTranscodeFramerate(AM_INT fps, AM_INT reduction=0) = 0;
    virtual AM_ERR SetTranscodeGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure) = 0;
    virtual AM_ERR TranscodeDemandIDR(AM_BOOL now) = 0;
    virtual AM_ERR RecordToFile(AM_BOOL en, char* name = NULL) = 0;

    virtual AM_ERR UploadNVRTranscode2Cloud(char* url, AM_INT flag) = 0;
    virtual AM_ERR StopUploadNVRTranscode2Cloud(AM_INT flag) = 0;

    //next stage:tricplay
    virtual AM_ERR PausePlay(AM_INT index) = 0;
    virtual AM_ERR ResumePlay(AM_INT index) = 0;
    virtual AM_ERR SeekTo(AM_U64 time, AM_INT index) = 0;
    virtual AM_ERR StepPlay(AM_INT index) = 0;

    virtual AM_ERR GetMdecInfo(MdecInfo& info) = 0;
    //virtual void setLoop(AM_INT loop) = 0;
    virtual AM_ERR Dump(AM_INT flag = 0) = 0;
    //just a test api
    virtual AM_ERR Test(AM_INT flag) = 0;

    virtual AM_ERR SetNvrPlayBackMode(AM_INT mode) = 0;
    //auto separate file
    virtual AM_ERR ChangeSavingTimeDuration(AM_INT duration) = 0;

    virtual AM_ERR ReLayout(AM_INT layout, AM_INT winIndex=0) = 0;
    virtual AM_ERR ExchangeWindow(AM_INT winIndex0, AM_INT winIndex1) = 0;
    virtual AM_ERR ConfigRender(AM_INT render, AM_INT window, AM_INT dsp) = 0;
    virtual AM_ERR ConfigRenderDone() = 0;

    virtual AM_ERR ConfigTargetWindow(AM_INT target, CParam& param) = 0;

    virtual AM_ERR CreateMotionDetectReceiver() = 0;
    virtual AM_ERR RemoveMotionDetectReceiver() = 0;
    virtual void MotionDetecthandler(const char* ip,void* event) = 0;

    virtual AM_ERR ConfigLoader(AM_BOOL en, char* path, char* m3u8name, char* host, AM_INT count, AM_UINT duration = 2) = 0;
};

extern IMDecControl* CreateActiveMDecControl(void *audiosink, CParam& par);

//recorder
class MdecInfo
{
public:
    class MdecWinInfo
    {
    public:
        MdecWinInfo():
            offsetX(-1),
            offsetY(-1)
        {}
    public:
        AM_INT offsetX;
        AM_INT offsetY;
        AM_INT width;
        AM_INT height;
        AM_INT dspRenIndex;
        AM_INT curIndex;//current index shown in this window
    };
    class MdecUnitInfo
    {
    public:
        MdecUnitInfo():
            isUsed(AM_FALSE),
            isPaused(AM_FALSE),
            isSteped(AM_FALSE),
            isHided(AM_FALSE),
            pausedBy(-1),
            playStream(IMDecControl::TYPE_NONE),
            is1080P(AM_FALSE),
            isEnd(AM_FALSE),
            isNet(AM_FALSE),
            isCheck(AM_FALSE),
            sourceIP(0),
            sourceGroup(-1),
            addOrder(-1),
            winChanged(AM_FALSE),
            length(0),
            progress(0),
            curPts(0),
            sd2HDIdx(-1),
            winParam(4)
        {
            //AM_INFO("MdecUnitInfo Init Succ.\n");
        }
    public:
        AM_BOOL isUsed;
        AM_BOOL isPaused;
        AM_BOOL isSteped;
        AM_BOOL isHided;
        AM_INT pausedBy;//who paused me
        AM_INT playStream;
        AM_BOOL is1080P;
        AM_BOOL isEnd;
        AM_BOOL isNet;
        AM_BOOL isCheck; //a flag to check

        AM_UINT sourceIP; // xx<24 |xx<16|xx<8|xx
        AM_INT sourceGroup;
        AM_INT addOrder; //for edit source
        AM_BOOL winChanged;
        AM_U64 length;//ms
        AM_U64 progress;//ms
        AM_U64 curPts;
        AM_INT sd2HDIdx;
        CParam winParam;
    };

public:
    MdecInfo():
        isNvr(AM_TRUE),
        nvrIavFd(-1)
    {
        AM_INT i = 0;
        for(; i < 16; i++)
            winOrder[i] = 0;
    }

    AM_INT HdStreamNum()
    {
        AM_INT num = 0, i = 0;
        for(; i < SOURCE_MAX_NUM; i++){
            if(unitInfo[i].is1080P == AM_TRUE && unitInfo[i].isUsed == AM_TRUE)
                num++;
        }
        return num;
    }

    AM_INT HdWindowNum()
    {
        AM_INT num = 0, i = 0;
        AM_INT win[16] = {0};
        for(; i < SOURCE_MAX_NUM; i++){
            if(unitInfo[i].is1080P == AM_TRUE && unitInfo[i].isUsed == AM_TRUE){
                if(win[unitInfo[i].sourceGroup] == 0){
                    win[unitInfo[i].sourceGroup] = 1;
                    num++;
                }
            }
        }
        return num;
    }

    AM_INT FindSdBySourceGroup(AM_INT sourceGroup)
    {
        AM_INT i = 0;
        AM_INT ret, maxOrder = -1, max=0;
        MdecUnitInfo* curInfo = NULL;

        for(; i < SOURCE_MAX_NUM; i++){
            curInfo = &(unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE){
                //a sd source
                if(curInfo->sourceGroup == sourceGroup){
                    if(curInfo->addOrder > maxOrder){
                        maxOrder = curInfo->addOrder;
                        max = i;
                    }
                }
            }
        }
        ret = (maxOrder == -1) ? -1 : max;
        return ret;
    }

    AM_INT FindHdBySourceGroup(AM_INT sourceGroup)
    {
        AM_INT i = 0;
        AM_INT ret, maxOrder = -1, max=0;
        MdecUnitInfo* curInfo = NULL;

        for(; i < SOURCE_MAX_NUM; i++){
            curInfo = &(unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_TRUE){
                //a hd source
                if(curInfo->sourceGroup == sourceGroup){
                    if(curInfo->addOrder > maxOrder){
                        maxOrder = curInfo->addOrder;
                        max = i;
                    }
                }
            }
        }
        ret = (maxOrder == -1) ? -1 : max;
        return ret;
    }

    AM_INT FindHdBySd(AM_INT sIndex)
    {
        AM_INT sourceGroup, i, maxOrder = -1, max=0;
        AM_INT ret;
        MdecUnitInfo* curInfo = &(unitInfo[sIndex]);
        sourceGroup = curInfo->sourceGroup;
        //order = curInfo->addOrder;

        for(i = 0; i < SOURCE_MAX_NUM; i++){
            curInfo = &(unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->sourceGroup == sourceGroup && curInfo->is1080P == AM_TRUE){
                //find a hd
                if(curInfo->addOrder > maxOrder){
                    maxOrder = curInfo->addOrder;
                    max = i;
                    AM_ASSERT(i != sIndex);
                }
            }
        }
        ret = (maxOrder == -1) ? -1 : max;
        return ret;
    }

    AM_INT FindIndexBySource(AM_UINT ip)
    {
        AM_INT i = 0;
        AM_INT ret = -1;
        MdecUnitInfo* curInfo = NULL;

        for(; i < SOURCE_MAX_NUM; i++){
            curInfo = &(unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE){
                //a SD source
                //AM_INFO("curInfo->ip: %x, ip: %x.\n", curInfo->sourceIP, ip);
                if(curInfo->sourceIP == ip){
                    ret = i;
                }
            }
        }
        return ret;
    }

    AM_UINT FindIPByIndex(AM_INT index)
    {
        AM_UINT ret = 0;
        MdecUnitInfo* curInfo = &(unitInfo[index]);

        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE){
            //a SD source
            //AM_INFO("curInfo->ip: %x, ip: %x.\n", curInfo->sourceIP, ip);
            ret = curInfo->sourceIP;
        }
        return ret;
    }

    AM_ERR ClearUnitInfo(AM_INT index)
    {
        if(index < 0 || index >= SOURCE_MAX_NUM)
            return ME_BAD_PARAM;
        if(unitInfo[index].isUsed == AM_FALSE)
            return ME_OK;
        unitInfo[index].isUsed = AM_FALSE;
        unitInfo[index].addOrder = -1;
        return ME_OK;
    }
public:
    enum{
        SOURCE_MAX_NUM = 64,
    };
    AM_BOOL isNvr;
    MdecUnitInfo unitInfo[SOURCE_MAX_NUM];
    AM_INT winOrder[16];
    //some info global to outside
    AM_INT nvrIavFd;
    //extra win info
    MdecWinInfo winInfo[SOURCE_MAX_NUM];
    AM_INT displayWidth;
    AM_INT displayHeight;
};

#endif

