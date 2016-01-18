
/**
 * am_ve_if.h
 *
 * History:
 *    2011/08/08 - [GangLiu] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_VEIF_H__
#define __AM_VEIF_H__

//#include <media/MediaPlayerInterface.h>
//using namespace android;

extern const AM_IID IID_IVEControl;

//-----------------------------------------------------------------------
//
// IVEControl
//
//-----------------------------------------------------------------------
class IVEControl: public IMediaControl
{
public:
    //DataBase

    typedef enum {
        Type_None,
        Type_Video,//0x001
        Type_Audio,//0x010
        Type_Video_Audio,//0x011
        Type_Image,//0x100
        Type_End,
    } VE_TYPE;

    typedef enum {
        STATE_IDLE,
        STATE_INITIALIZED,//preview graph built completed
        //STATE_PREVIEW,//at least 1 source added
        STATE_PREVIEW_INIT,
        STATE_PREVIEW_RUN,
        STATE_PREVIEW_PAUSE,
        STATE_PREPARED,//Preview OK, Ready to generate a new file. complete data base first
        //delete preview graph and build finalize graph here
        STATE_READY,//preview graph built completed
        STATE_FINALIZE,//file is being generated
        STATE_STOPPING,//is stopping
        STATE_COMPLETED,//completed, will back to IDLE
        STATE_STOPPED,//Rdy to quit
        STATE_ERROR,
    } VE_STATE;

    typedef enum {
        VE_EFFECT_CUBE,
        VE_EFFECT_FADE,
        VE_EFFECT_3D,
    } VE_EFFECT;

    typedef struct {
        VE_EFFECT	Effect;
        AM_U64		StartTime;
        AM_U64		EndTime;
        AM_UINT	Object1;
        AM_UINT	Object2;
        AM_UINT	Reserved;
    }VE_Effect_t;

    struct VE_INFO {
        VE_STATE  state;
        AM_UINT  SelectedSourceID;
        AM_U64  CurTime;
        AM_U64	 TimeShaftLength;
    };

    struct VE_SRC_INFO {
        AM_UINT ID;
        VE_TYPE type;
        AM_U64 StartTime;
        AM_U64 duration; //ms, image's duration is 0.
    };

public:
    DECLARE_INTERFACE(IVEControl, IID_IVEControl);

    //project
    virtual AM_ERR CreateProject() = 0;
    virtual AM_ERR OpenProject(const char *pProjectName) = 0;
    virtual AM_ERR SaveProject() = 0;
    virtual AM_BOOL IsSaved() = 0;//app can get whether the project has been saved
    virtual AM_ERR Stop() = 0;//stop engine. finalize can stop engine itself, this API called in preview
    virtual AM_ERR StopTransc(AM_INT stopcode=2) = 0;

    virtual AM_ERR AddSource(const char *pFileName, VE_TYPE type, AM_UINT &sourceID) = 0;//source new added is selected
    //virtual AM_ERR PrepareSource(const char *pFileName) = 0;
    virtual AM_ERR DelSource(AM_UINT sourceID) = 0;
    virtual AM_ERR SeekTo(AM_U64 time) = 0;//seek to ?ms in the timeline
    virtual AM_ERR GetVEInfo(VE_INFO& info) = 0;

    //operate of selected item
    virtual AM_ERR SelectID(AM_UINT sourceID) = 0;//app select sourceID//default start time is 0 or current time
    virtual AM_ERR GetSourceInfo(VE_SRC_INFO& src_info) = 0;//get the selected source's info, should be used when added
    virtual AM_ERR SetStartTime(AM_U64 time) = 0;//set the start time of current slected source
    virtual AM_ERR SplitSource(AM_U64 from, AM_U64 to, VE_TYPE src_type) = 0;//from=0 is start, to=0xffffff is to the end
                                                                                                                         //if soure type=3, src_type here =1, only split audio
    //preview
    virtual AM_ERR AddEffect(VE_Effect_t* effect) = 0;
    virtual AM_ERR StartPreview(AM_INT mode=0) = 0;//start
    virtual AM_ERR StartTranscode() = 0;//start
    virtual AM_ERR ResumePreview() = 0;//resume
    virtual AM_ERR PausePreview() = 0;//pause
    virtual AM_ERR StopPreview() = 0;//pause

    virtual AM_ERR StopDecoder(AM_INT index) = 0;//stop 1 instance

    //if keepAspectRatio, ignore en_w.
    virtual AM_ERR SetEncParamaters(AM_UINT en_w, AM_UINT en_h, bool keepAspectRatio, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm) = 0;
    virtual AM_ERR SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps) = 0;
    virtual AM_ERR SetMuxParamaters(const char *filename, AM_UINT cutSize) = 0;

    //finalize
    virtual AM_ERR StartFinalize() = 0;
    virtual AM_INT GetProcess() = 0;//0~100

    //test
    virtual void setloop(AM_U8 loop) = 0;
    virtual void setTranscFrms(AM_INT frames) = 0;
    //vout config, todo

//debug only
#ifdef AM_DEBUG
    virtual AM_ERR SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option) = 0;
    virtual void PrintState() = 0;//print current VE-engine's(include filters) states, for debug
#endif

};

extern IVEControl* CreateActiveVEControl(void *audiosink);

#endif

