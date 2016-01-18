/*
 * general_loader_filter.h
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
#ifndef _GENERAL_LOADER_FILTER_H
//aliyun cloud
#include "aliyunoss.h"
//baidu PCS
#include <stdio.h>
#include "ServiceManager.h"
#include "IBaiduPCS.h"
//qq cloud
#include "IQQWeiyun.h"

typedef struct {
    char pPath[128];
    char pNameM3U8[32];
    char pNameHost[32];
    AM_UINT mDuration;
    AM_UINT mObjectCount;
} SLoaderConfig;

class CGeneralLoader : public CActiveObject
{
    typedef CActiveObject inherited;

    enum CloudType {
        ALIYUN_CLOUD = 0,
        BAIDU_PCS = 1,
        QQ_CLOUD = 2,
    };
public:
    static CGeneralLoader* Create(int type = ALIYUN_CLOUD);
public:
    void Delete();
    void OnRun();

    void ConfigLoader(SLoaderConfig config);
    AM_ERR HandleObject(char* name, AM_INT index, AM_INT type);
    AM_ERR StopLoader();

private:
    CGeneralLoader(int type);
    AM_ERR Construct();
    ~CGeneralLoader();

    AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR ProcessData(CBuffer* buffer);

    void UpdateM3U8(char* name);
    void UploadLastM3U8();

private:
    CBuffer* mpBuffer;
    CBuffer mBuffer;
    CQueue* mpObjectQ;
    AM_INT mIndex;
    //char mObjectPath[128];
    //char mObjectBaseName[32];
    char mObjectFullName[256];
    char mM3U8FullName[256];

    char* mObjectName[10];
    SLoaderConfig mLoaderConfig;
    AM_BOOL mbLoaderConfiged;
    AM_INT mSeqNum;

private:
    OssInstance* mpOssInstance;
    CServiceManager* mpServiceManager;
    void* mpCloudHandler;
    int mCloudType;
private:
    static CGeneralLoader* _LoaderInstance;
    static pthread_mutex_t _Mutex;
    static AM_INT _Count;
};
#endif

