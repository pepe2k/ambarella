/*
 * g_simple_save.h
 *
 * History:
 *    2012/6/15 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __G_SIMPLE_SAVE_H__
#define __G_SIMPLE_SAVE_H__


#define MUXER_SIMPLE_MAX_BUFFER_VIDEO_DATA 250
#define MUXER_SIMPLE_MAX_BUFFER_AUDIO_DATA 120

class IFileWriter;
class CGSimpleSave : public IGMuxer, public CActiveObject
{
    typedef CActiveObject inherited;
    enum{
        CMD_FULL = CMD_GMF_LAST + 100,
        CMD_GOON,
        CMD_FINISH,
    };
public:
    static IGMuxer* Create(CGeneralMuxer* manager);

public:
    void* GetInterface(AM_REFIID refiid)
    {
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    virtual AM_ERR ConfigMe(CUintMuxerConfig* con);
    virtual AM_ERR UpdateConfig(CUintMuxerConfig* con);
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR FeedData(CGBuffer* buffer);
    virtual AM_ERR FinishMuxer();
    virtual AM_ERR QueryInfo(AM_INT type, CParam& par);
    virtual AM_ERR Dump();

    virtual void OnRun();

    virtual AM_ERR SetSavingTimeDuration(AM_UINT duration, AM_UINT maxfilecount);
    virtual AM_ERR EnableLoader(AM_BOOL flag);
    virtual AM_ERR ConfigLoader(char* path, char* m3u8name, char* host, int count);
private:
    CGSimpleSave(CGeneralMuxer* manager);
    AM_ERR Construct();
    ~CGSimpleSave();
    AM_ERR ClearQueue(CQueue* queue);
private:
    AM_ERR SetupMuxerEnv();

    AM_ERR DoStop();
    AM_ERR DoFinish();
    AM_ERR DoFull(CMD& cmd);
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR ProcessData(CQueue::WaitResult& result);
    AM_ERR ProcessVideoData();
    AM_ERR ProcessAudioData();

private:
    class SaveInfo
    {
        public:
        SaveInfo():
            saveSize(0),
            saveFrame(0)
        {}
        AM_INT saveSize;
        AM_INT saveFrame;
    };
private:
    CGeneralMuxer* mpManager;
    AM_INT mIndex;
    CUintMuxerConfig* mpConfig;

    IFileWriter* mpWriter;
    IFileWriter* mpAWriter;
    CQueue* mpVideoQ;
    CQueue* mpAudioQ;

    CGBuffer mBuffer;
    CGBuffer mBufferDump;
    volatile AM_BOOL mbFlowFull;

    SaveInfo mInfo;
};
#endif
