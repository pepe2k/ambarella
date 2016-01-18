/*
 * general_loader_filter.cpp
 *
 * History:
 *    2013/9/27 - [SkyChen] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>
#include "general_header.h"

#include "general_loader_filter.h"

#define MUXER_MAX_BUFFER_NUM 16

CGeneralLoader* CGeneralLoader::_LoaderInstance = NULL;
pthread_mutex_t CGeneralLoader::_Mutex = PTHREAD_MUTEX_INITIALIZER;
AM_INT CGeneralLoader::_Count = 0;
///////////////////////////////////////////////
//
//
//////////////////////////////////////////////
CGeneralLoader* CGeneralLoader::Create(int type)
{
    pthread_mutex_lock(&_Mutex);
    if (NULL == _LoaderInstance) {
        _LoaderInstance = new CGeneralLoader(type);
        if (_LoaderInstance && _LoaderInstance->Construct() != ME_OK) {
            delete _LoaderInstance;
            _LoaderInstance = NULL;
        }
    }
    _Count++;
    pthread_mutex_unlock(&_Mutex);
    return _LoaderInstance;
}

CGeneralLoader::CGeneralLoader(int type):
    inherited("CGeneralLoader"),
    mpObjectQ(NULL),
    mIndex(0),
    mbLoaderConfiged(AM_FALSE),
    mSeqNum(0),
    mpOssInstance(NULL),
    mpServiceManager(NULL),
    mpCloudHandler(NULL),
    mCloudType(type)
{
    memset(mObjectFullName, 0, sizeof(mObjectFullName));
    memset(mObjectName, 0, sizeof(mObjectName));
    memset(mM3U8FullName, 0, sizeof(mM3U8FullName));

    mpBuffer = new(CBuffer);
}

AM_ERR CGeneralLoader::Construct()
{
    AM_INFO("CGeneralLoader Construct!\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = MsgQ();
    if ((mpObjectQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), MUXER_MAX_BUFFER_NUM)) == NULL)
        return ME_NO_MEMORY;

    if (mCloudType == ALIYUN_CLOUD) {
        mpOssInstance = new(OssInstance);
    } else if (mCloudType == BAIDU_PCS) {
        mpServiceManager = CServiceManager::getInstance();
        mpCloudHandler = mpServiceManager->createService(CServiceManager::SVC_BAIDUPCS);
    } else if (mCloudType == QQ_CLOUD) {
        mpServiceManager = CServiceManager::getInstance();
        mpCloudHandler = mpServiceManager->createService(CServiceManager::SVC_QQWEIYUN);
    }

    return err;
}

CGeneralLoader::~CGeneralLoader()
{
}

void CGeneralLoader::Delete()
{
    pthread_mutex_lock(&_Mutex);
    if ((--_Count) == 0) {
        if (mpBuffer) {
            delete mpBuffer;
            mpBuffer = NULL;
        }
        if (mpOssInstance) {
            delete mpOssInstance;
            mpOssInstance = NULL;
        }
        if (mpServiceManager) {
            delete mpServiceManager;
            mpServiceManager = NULL;
        }
        if (mCloudType == BAIDU_PCS && mpCloudHandler) {
            delete (IBaiduPCS*)mpCloudHandler;
            mpCloudHandler = NULL;
        }else if (mCloudType == QQ_CLOUD && mpCloudHandler) {
            delete (IQQWeiyun*)mpCloudHandler;
            mpCloudHandler = NULL;
        }

        for (int i = 0; i < mLoaderConfig.mObjectCount; i++) {
            if (mObjectName[i]) free(mObjectName[i]);
        }

        AM_DELETE(mpObjectQ);
        inherited::Delete();
        AM_INFO("CGeneralLoader Delete done!\n");
    }
    pthread_mutex_unlock(&_Mutex);
    return;
}

void CGeneralLoader::ConfigLoader(SLoaderConfig config)
{
    pthread_mutex_lock(&_Mutex);
    mLoaderConfig = config;
    AM_INFO("config loader: path %s\n", mLoaderConfig.pPath);
    AM_INFO("config loader: m3u8 name %s\n", mLoaderConfig.pNameM3U8);
    AM_INFO("config loader: host name %s\n", mLoaderConfig.pNameHost);
    AM_INFO("config loader: object duration %u\n", mLoaderConfig.mDuration);
    AM_INFO("config loader: object count %u in m3u8\n", mLoaderConfig.mObjectCount);
    snprintf(mM3U8FullName, sizeof(mM3U8FullName), "%s/%s", mLoaderConfig.pPath, mLoaderConfig.pNameM3U8);
    mbLoaderConfiged = AM_TRUE;
    SendCmd(CMD_RUN);
    pthread_mutex_unlock(&_Mutex);
    return;
}

AM_ERR CGeneralLoader::HandleObject(char* name, AM_INT index, AM_INT type)
{
    pthread_mutex_lock(&_Mutex);
    if (mbLoaderConfiged == AM_FALSE) {
        AM_ERROR("Please config loader firstly by ConfigLoader!\n");
        return ME_ERROR;
    }
    char* objectname = NULL;
    if (name == NULL) {
        AM_ERROR("HandleObject: invaild parameter, name is null!\n");
        return ME_ERROR;
    }
    objectname = (char*)malloc(strlen(name) + 1);
    if (objectname == NULL) {
        AM_ERROR("HandleObject: failed to malloc!\n");
        return ME_ERROR;
    }
    memset(objectname, 0, strlen(name) + 1);
    strncpy(objectname, name, strlen(name));

    mpBuffer->SetDataPtr((AM_U8*)objectname);
    mpBuffer->mSeqNum = index;
    mpBuffer->SetType(type);

    if (mpObjectQ->GetDataCnt() < MUXER_MAX_BUFFER_NUM) {
        mpObjectQ->PutData(mpBuffer, sizeof(CGBuffer));
    } else {
        AM_ERROR("load file is too slowly? no free buffer now!\n");
    }

    pthread_mutex_unlock(&_Mutex);
    return ME_OK;
}

AM_ERR CGeneralLoader::StopLoader()
{
    CMD cmd(CMD_STOP);
    //send this cmd?
    PerformCmd(cmd, AM_TRUE);
    return ME_OK;
}
////////////////////////////////////////////////////////
AM_ERR CGeneralLoader::PerformCmd(CMD& cmd, AM_BOOL isSend)
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

AM_ERR CGeneralLoader::ProcessCmd(CMD& cmd)
{
    switch (cmd.code) {
        case CMD_STOP:
            //mState = STATE_PENDING;
            mbRun = false;
            //wirte end flag here
            UploadLastM3U8();
            CmdAck(ME_OK);
            break;
        case CMD_PAUSE:
            mState = STATE_PENDING;
            CmdAck(ME_OK);
            break;
        case CMD_RESUME:
            mState = STATE_IDLE;
            CmdAck(ME_OK);
            break;
        default:
            AM_ERROR("wrong CMD: code %d\n", cmd.code);
            break;
    }
    return ME_OK;
}

void CGeneralLoader::OnRun()
{
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;

    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_IDLE;

    while (mbRun) {
        switch(mState) {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),  &result);
            if (type == CQueue::Q_MSG) {
                ProcessCmd(cmd);
            } else {
                //CQueue* resultQ = result.pDataQ;
                if (!mpObjectQ->PeekData(&mBuffer, sizeof(CBuffer))) {
                    AM_ERROR("!!PeekData Failed!\n");
                    break;
                }
                err = ProcessData(&mBuffer);
            }
            break;
        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;
        default:
            AM_ERROR("Check Me.\n");
            mbRun = false;
            break;
        }
    }
    return;
}

AM_ERR CGeneralLoader::ProcessData(CBuffer* buffer)
{
    AM_INT ret = 0;
    char* objectname = (char*)buffer->GetDataPtr();
    AM_INT index = buffer->mSeqNum;
    AM_INT type = buffer->GetType();

    memset(mObjectFullName, 0, sizeof(mObjectFullName));
    snprintf(mObjectFullName, sizeof(mObjectFullName), "%s/%s", mLoaderConfig.pPath, objectname);

    AM_INFO("file name: %s\n", mObjectFullName);

    if (0 == type) {
        //download
        ret = mpOssInstance->getObject(mLoaderConfig.pNameHost, objectname, mObjectFullName);
    } else {
        //upload
        if (mCloudType == ALIYUN_CLOUD) {
            ret = mpOssInstance->putObject(mLoaderConfig.pNameHost, objectname, mObjectFullName);
        } else if (mCloudType == BAIDU_PCS) {
            ret = ((IBaiduPCS*)mpCloudHandler)->upload(mObjectFullName, NULL);
        } else if (mCloudType == QQ_CLOUD) {
            ret = ((IQQWeiyun*)mpCloudHandler)->upload(mObjectFullName, NULL);
        }
    }

    if (ret != 0) {
        AM_ERROR("ProcessData failed to %s, file name %s!!\n", (type == 0)? "download" : "upload", mObjectFullName);
        return ME_ERROR;
    }
    AM_INFO("Successfully to %s file: %s\n", (type == 0)? "download" : "upload", mObjectFullName);
    if (type != 0) {
        UpdateM3U8(objectname);
        if (mCloudType == ALIYUN_CLOUD) {
            ret = mpOssInstance->putObject(mLoaderConfig.pNameHost, mLoaderConfig.pNameM3U8, mM3U8FullName);
        } else if (mCloudType == BAIDU_PCS) {
            ret = ((IBaiduPCS*)mpCloudHandler)->upload(mM3U8FullName, NULL);
        } else if (mCloudType == QQ_CLOUD) {
            ret = ((IQQWeiyun*)mpCloudHandler)->upload(mM3U8FullName, NULL);
        }
        if (ret != 0) {
            AM_ERROR("Failed to upload %s\n", mM3U8FullName);
        }
    }
    return ME_OK;
}

void CGeneralLoader::UpdateM3U8(char* name)
{
    if (name == NULL)  return;

    int i = 0;

    if (mObjectName[0])  free(mObjectName[0]);

    for (i = 0; i < mLoaderConfig.mObjectCount; i++) {
        if (i == mLoaderConfig.mObjectCount -1) {
            mObjectName[i] = name;
        } else {
            mObjectName[i] = mObjectName[i+1];
        }
    }

    FILE* pfile = fopen(mM3U8FullName,"wt");
    if (pfile == NULL) {
        AM_ERROR("open config file(%s) fail in loader:UpdateM3U8.\n", mM3U8FullName);
        return;
    }

    //write Header;
    fprintf(pfile, "#EXTM3U\n");
    fprintf(pfile, "#EXT-X-TARGETDURATION:%u\n", mLoaderConfig.mDuration);

    //clip seq_num to 0-15
    fprintf(pfile, "#EXT-X-MEDIA-SEQUENCE:%d\n", (mSeqNum++ & 0xffff));

    for (i = 0; i < mLoaderConfig.mObjectCount; i++) {
        if (mObjectName[i]) {
            fprintf(pfile, "#EXTINF:%f,\n", (float)mLoaderConfig.mDuration);
            fprintf(pfile, "%s\n", mObjectName[i]);
        }
    }

    //not write end here, for live streamming case

    fclose(pfile);
    return;
}

void CGeneralLoader::UploadLastM3U8()
{
    AM_INT ret = 0;
    FILE* pfile = fopen(mM3U8FullName,"at");
    if (pfile == NULL) {
        AM_ERROR("open config file(%s) fail in loader:UploadLastM3U8.\n", mM3U8FullName);
        return;
    }
    fprintf(pfile, "#EXT-X-ENDLIST\n");
    fclose(pfile);
    ret = mpOssInstance->putObject(mLoaderConfig.pNameHost, mLoaderConfig.pNameM3U8, mM3U8FullName);
    if (ret != 0) {
        AM_ERROR("failed to upload last m3u8!\n");
    }
    return;
}