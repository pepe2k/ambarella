/*
 * general_audio_manager.h
 *
 * History:
 *    2012/8/13 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_AUDIO_MAN_H__
#define __GENERAL_AUDIO_MAN_H__

struct AVFormatContext;
class CGeneralAudioManager:public CObject
{

public:
    static CGeneralAudioManager* Create(CGConfig* pConfig);

public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
        return CObject::GetInterface(refiid);
    }
    virtual void Delete();

    CQueue* AudioMainQ();

    AM_ERR AddAudioSource(AM_INT index, AM_INT audio);
    AM_ERR DeleteAudioSource(AM_INT index);
    //startPts should be ffmpeg's timestamp
    AM_ERR SetAudioPosition(AM_INT index, AM_S64 startPts);
    AM_ERR ReadAudioData(AM_INT index, CGBuffer& oBuffer);

private:
    AM_ERR SeekToPosition2(AM_INT index, AM_S64 startPts);
    AM_ERR SeekToPosition(AM_INT index, AM_S64 startPts);
    AM_ERR DoFFMpegSeek(AM_INT index, AM_S64 startPts);
    AM_INT FRAME_TIME_DIFF(AM_INT index, AM_INT diff);

private:
    CGeneralAudioManager(CGConfig* pConfig);
    AM_ERR Construct();
    ~CGeneralAudioManager();

private:
    CGConfig* mpGConfig;

    CQueue* mpAudioMainQ;
    AVFormatContext* mpAVArray[MDEC_SOURCE_MAX_NUM];
    AM_INT mAudio[MDEC_SOURCE_MAX_NUM];
};
#endif
