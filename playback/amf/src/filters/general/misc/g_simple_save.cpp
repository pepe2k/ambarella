/*
 * g_simple_save.cpp
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

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "record_if.h"

#include "general_muxer_save.h"
#include "g_simple_save.h"



//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
IGMuxer* CGSimpleSave::Create(CGeneralMuxer* manager)
{
    CGSimpleSave* result = new CGSimpleSave(manager);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGSimpleSave::CGSimpleSave(CGeneralMuxer* manager):
    inherited("CGSimpleSave"),
    mpManager(NULL),
    mpConfig(NULL),
    mpVideoQ(NULL),
    mpAudioQ(NULL),
    mbFlowFull(AM_FALSE)
{
    mpWriter = NULL;
    mpAWriter = NULL;
}

AM_ERR CGSimpleSave::Construct()
{
    AM_INFO("CGSimpleSave Construct!\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = MsgQ();
    if ((mpVideoQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), MUXER_SIMPLE_MAX_BUFFER_VIDEO_DATA)) == NULL)
        return ME_NO_MEMORY;

    if ((mpAudioQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), MUXER_SIMPLE_MAX_BUFFER_AUDIO_DATA)) == NULL)
        return ME_NO_MEMORY;

    if ((mpWriter = CFileWriter::Create()) == NULL) return ME_ERROR;
    if ((mpAWriter = CFileWriter::Create()) == NULL) return ME_ERROR;

    SendCmd(CMD_RUN);
    //mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGSimpleSave::SetupMuxerEnv()
{
    AM_ASSERT(mpConfig != NULL);
    if(mpWriter->CreateFile(mpConfig->fileName) != ME_OK)
        return ME_ERROR;


    char audioname[150] = {0};
    strcat(audioname, mpConfig->fileName);
    strcat(audioname, ".pcm");
    if(mpAWriter->CreateFile(audioname) != ME_OK)
        return ME_ERROR;

    return ME_OK;
}

void CGSimpleSave::Delete()
{
    AM_INFO("CGSimpleSave Delete\n");
    if(mState != STATE_PENDING){
        AM_INFO("Still Process Finish\n");
    }
    CMD cmd(CMD_STOP);
    PerformCmd(cmd, AM_TRUE);
    AM_INFO("CGSimpleSave CMD_STOP Done\n");

    ClearQueue(mpVideoQ);
    ClearQueue(mpAudioQ);

    AM_DELETE(mpVideoQ);
    AM_DELETE(mpAudioQ);
    AM_DELETE(mpWriter);
    AM_DELETE(mpAWriter);
    inherited::Delete();
    AM_INFO("CGSimpleSave Delete Done\n");
}

AM_ERR CGSimpleSave::ClearQueue(CQueue* queue)
{
    AM_BOOL rval;
    CGBuffer buffer;
    while(1)
    {
        rval = queue->PeekData(&buffer, sizeof(CGBuffer));
        if(rval == AM_FALSE)
        {
            break;
        }
        //release this packet
        buffer.ReleaseContent();
    }
    return ME_OK;
}

CGSimpleSave::~CGSimpleSave()
{

}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
AM_ERR CGSimpleSave::ConfigMe(CUintMuxerConfig* con)
{
    AM_ERR err = ME_OK;
    mpConfig = con;
    err = SetupMuxerEnv();
    AM_INFO("ConfigMe Done:%d\n", err);
    return err;
}

AM_ERR CGSimpleSave::UpdateConfig(CUintMuxerConfig* con)
{
    return ME_OK;
}

AM_ERR CGSimpleSave::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = MsgQ()->SendMsg(&cmd, sizeof(cmd));
    }else{
        err = MsgQ()->PostMsg(&cmd, sizeof(cmd));
    }
    return err;
}

AM_ERR CGSimpleSave::FeedData(CGBuffer* buffer)
{
    STREAM_TYPE type = buffer->GetStreamType();
    if(type != STREAM_VIDEO && type != STREAM_AUDIO){
        AM_ASSERT(0);
        buffer->ReleaseContent();
        return ME_BAD_FORMAT;
    }

    CQueue* mpQ = (type == STREAM_VIDEO) ? mpVideoQ : mpAudioQ;
    if(mpQ->GetDataCnt() < MUXER_SIMPLE_MAX_BUFFER_VIDEO_DATA){
        mpQ->PutData(buffer, sizeof(CGBuffer));
        return ME_OK;
    }

    if(mbFlowFull == AM_FALSE){
        AM_INFO("Video Save too Slowly!%d\n", buffer->GetIdentity());
        CMD cmd(CMD_FULL);
        AM_ASSERT(mBufferDump.GetBufferType() == NOINITED_BUFFER);
        mBufferDump = *buffer;
        //cmd.pExtra = buffer;
        mbFlowFull = AM_TRUE;
        PerformCmd(cmd, AM_FALSE);
    }else{
        buffer->ReleaseContent();
    }

    return ME_OK;
}

AM_ERR CGSimpleSave::FinishMuxer()
{
    CMD cmd(CMD_FINISH);
    //send this cmd?
    PerformCmd(cmd, AM_FALSE);
    AM_INFO("FinishMuxer Done\n");
    return ME_OK;
}

AM_ERR CGSimpleSave::QueryInfo(AM_INT type, CParam& par)
{
    return ME_OK;
}

AM_ERR CGSimpleSave::Dump()
{
    return ME_OK;
}

void CGSimpleSave::OnRun()
{
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;

    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_IDLE;
    AM_INFO("CGSimpleSave OnRun Enter.\n");
    while(mbRun)
    {
        switch(mState)
        {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),  &result);
            if(type == CQueue::Q_MSG){
                ProcessCmd(cmd);
            }else{
                err = ProcessData(result);
                AM_VERBOSE("err=%d.\n", err);
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        default:
            AM_ERROR("Check Me.\n");
            break;
        }
    }
    AM_INFO("CGSimpleSave OnRun Exit.\n");
}

AM_ERR CGSimpleSave::DoStop()
{
    AM_ASSERT(mState == STATE_PENDING);
    return ME_OK;
}

AM_ERR CGSimpleSave::DoFinish()
{
    //HANDLE FINISH
    AM_INFO("Save %s Done, Save Size:%d, SaveFrame:%d\n",
    mpConfig->fileName, mInfo.saveSize, mInfo.saveFrame);
    mState = STATE_PENDING;
    return ME_OK;
}

AM_ERR CGSimpleSave::DoFull(CMD& cmd)
{
    CGBuffer saveBuffer;
    //CGBuffer* buffer = (CGBuffer* )(cmd.pExtra);

    CQueue* mpQ = (mBufferDump.GetStreamType() == STREAM_VIDEO) ? mpVideoQ : mpAudioQ;
    AM_UINT max = (mpQ == mpVideoQ) ? MUXER_SIMPLE_MAX_BUFFER_VIDEO_DATA : MUXER_SIMPLE_MAX_BUFFER_AUDIO_DATA;
    if(mpQ->GetDataCnt() < (max - 10)){
        //thread diff;
    }else{
        while(mpQ->GetDataCnt() >= (max -20)){
            //AM_INFO("DEBUG\n", mBufferDump.GetIdentity());
            mpQ->PeekData(&saveBuffer, sizeof(CGBuffer));
            saveBuffer.ReleaseContent();
        }
    }

    mpQ->PutData(&mBufferDump, sizeof(CGBuffer));
    mBufferDump.Clear();
    mbFlowFull = AM_FALSE;
    return ME_OK;
}

AM_ERR CGSimpleSave::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGSimpleSave::ProcessCmd %d\n ", cmd.code);
//    AM_ERR err;
    //AM_U64 par;
    switch (cmd.code)
    {
    case CMD_STOP:
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_FINISH:
        DoFinish();
        CmdAck(ME_OK);
        break;

    case CMD_FULL:
        DoFull(cmd);
        break;

    case CMD_GOON:
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGSimpleSave::ProcessData(CQueue::WaitResult& result)
{
    AM_ERR err = ME_OK;
    CQueue* resultQ = result.pDataQ;
    if(!resultQ->PeekData(&mBuffer, sizeof(CGBuffer)))
    {
        AM_ERROR("!!PeekData Failed!\n");
        return ME_ERROR;
    }
    if(mBuffer.GetStreamType() == STREAM_VIDEO){
        err = ProcessVideoData();
    }else{
        err = ProcessAudioData();
    }

    //handle save issue
    if(err != ME_OK){
        AM_ERROR("IO Wrong!!!!\n");
        AM_MSG msg;
        msg.code = IGMuxer::MUXER_MSG_ERROR;
        mpManager->NotifyFromMuxer(mIndex, msg);
        mState = STATE_PENDING;
    }

    return err;
}

AM_ERR CGSimpleSave::ProcessVideoData()
{
    AM_ERR err = ME_OK;
    AM_U8* dataPrt = mBuffer.PureDataPtr();
    AM_UINT dataSize = mBuffer.PureDataSize();

    err = mpWriter->WriteFile(dataPrt, dataSize);
    mInfo.saveSize += dataSize;
    mInfo.saveFrame++;
    mBuffer.ReleaseContent();
    //AM_INFO("ProcessVideoData, %d\n", err);
    return err;
}

AM_ERR CGSimpleSave::ProcessAudioData()
{
    AM_ERR err = ME_OK;
    AM_U8* dataPrt = mBuffer.PureDataPtr();
    AM_UINT dataSize = mBuffer.PureDataSize();

    err = mpAWriter->WriteFile(dataPrt, dataSize);
    mBuffer.ReleaseContent();
    AM_VERBOSE("ProcessAudioData, err=%d\n", err);

    return ME_OK;
}

AM_ERR CGSimpleSave::SetSavingTimeDuration(AM_UINT duration, AM_UINT maxfilecount)
{
    return ME_OK;
}

AM_ERR CGSimpleSave::EnableLoader(AM_BOOL flag)
{
    return ME_OK;
}

AM_ERR CGSimpleSave::ConfigLoader(char* path, char* m3u8name, char* host, int count)
{
    return ME_OK;
}