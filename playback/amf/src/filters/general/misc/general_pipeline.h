/**
 * general_pipeline.h
 *
 * History:
 *    2013/1/21- [Qingxiong Z] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_PIPELINE_H__
#define __GENERAL_PIPELINE_H__

class CActiveMDecEngine;
class CPipeLineManager : public IPipeLineManager, public CObject
{
public:
    static IPipeLineManager* Create(IEngine* pEngine, AM_BOOL isGMF);
public:
    virtual AM_INT CreatePipeLine(const char* uri, PIPE_TYPE type);
    virtual AM_ERR AddPipeNode(AM_INT index, void* module, NODE_TYPE type);
    virtual AM_ERR AddPipeNode(AM_INT index, AM_INT indentity, NODE_TYPE type);
    virtual AM_ERR UpdatePipeLine(AM_INT index);

    virtual AM_ERR PreparePipeLine(AM_INT index);
    virtual AM_ERR DisablePipeLine(){return ME_NO_IMPL;}
    virtual AM_ERR EnablePipeLine(){return ME_NO_IMPL;}

    virtual AM_ERR PausePipeLine(AM_INT index);
    virtual AM_ERR ResumePipeLine(AM_INT index);
    virtual AM_ERR FlushPipeLine(AM_INT index);
    //AM_ERR ResumePipeLine();

    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();
    virtual AM_ERR Dump(AM_INT flag);
private:
    class PipeLine
    {
    public:
        class PipeNode{
        public:
            void* module;
            AM_INT identity;
            NODE_TYPE type;
            PipeNode* next;
            PipeNode* pre;
        };
        typedef PipeNode* PipeNodeList;
        AM_ERR Init(const char* uri, PIPE_TYPE type);
        AM_ERR AddNode(AM_INT identity, NODE_TYPE type);
        AM_ERR AddNode(void* identity, NODE_TYPE type);

        AM_ERR CheckBeforeAdd(NODE_TYPE intype);
        AM_ERR CheckBeforePrepare();

        PipeNode* GetNode(AM_INT index);
        const char* GetSource() { return mpSource;}

        PipeLine();
        ~PipeLine();
    private:
        char mpSource[128];
        PIPE_TYPE mType;
        PIPE_STATE mState;
        PipeNodeList mpList;
    };

private:
    AM_ERR Construct();
    CPipeLineManager(IEngine* pEngine, AM_BOOL isGMF);
    ~CPipeLineManager();

    CInterActiveFilter* GetFilterByNode(PipeLine::PipeNode* pNode);
    AM_INT GetTargetByNode(PipeLine::PipeNode* pNode);
private:
    IEngine* mpEngine;
    CActiveMDecEngine* mpMdecEngine;

    PipeLine mPlaybackV[32];
    PipeLine mPlaybackA[32];
    PipeLine mRecoder[32];
    PipeLine mStreaming[32];

    AM_INT mNumPBV;
    AM_INT mNumPBA;
    AM_INT mNumRec;
    AM_INT mNumStream;
};
#endif
