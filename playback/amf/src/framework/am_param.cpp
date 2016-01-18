/*
 * am_param.cpp
 *
 * History:
 *    2011/9/9 - [QingXiong Z] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <string.h>

#include "am_types.h"
#include "am_param.h"

CParam::CParam(AM_INT count)
{
    memset(mParam, 0, sizeof(AM_INT)*CPARAM_MAX_PARAM_NUM);
    if(count >= CPARAM_MAX_PARAM_NUM || count < 0)
    {
        AM_ERROR("CParam: Requested Param Count is Illegal. Must >=0 And < %d\n", CPARAM_MAX_PARAM_NUM);
        mCount = 0;
    }else{
        mCount = count;
    }
}

AM_ERR CParam::PutParam(AM_INT index, AM_INT value)
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Put Value(%d) in mCount.\n", index, value);
        mParam[mCount] = value;
        return ME_BAD_PARAM;
    }

    mParam[index] = value;
    return ME_OK;
}

AM_INT& CParam::operator[](const AM_INT index)
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Return mCount.\n", index);
        return mParam[mCount];
    }
    return mParam[index];
}

const AM_INT& CParam::operator[](const AM_INT index) const
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Return mCount.\n", index);
        return mParam[mCount];
    }
    return mParam[index];
}

AM_ERR CParam::CheckNum(AM_INT parNum)
{
    if(parNum != mCount)
    {
        AM_ERROR("Config Param Count Failed!\n");
        return ME_BAD_PARAM;
    }
    return ME_OK;
}

AM_ERR CParam::CheckZero(AM_INT parNum)
{
    AM_ERR err;
    AM_INT index;
    if((err = CheckNum(parNum)) != ME_OK)
        return err;
    for(index = 0; index < mCount; index++)
    {
        if(mParam[index] == 0)
            return ME_ERROR;
    }
    return ME_OK;
}

//64 CASE
CParam64::CParam64(AM_INT count)
{
    memset(mParam, 0, sizeof(AM_S64)*CPARAM_MAX_PARAM_NUM);
    if(count >= CPARAM_MAX_PARAM_NUM || count < 0)
    {
        AM_ERROR("CParam: Requested Param Count is Illegal. Must >=0 And < %d\n", CPARAM_MAX_PARAM_NUM);
        mCount = 0;
    }else{
        mCount = count;
    }
}

AM_ERR CParam64::PutParam(AM_INT index, AM_S64 value)
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Put Value(%lld) in mCount.\n", index, value);
        mParam[mCount] = value;
        return ME_BAD_PARAM;
    }

    mParam[index] = value;
    return ME_OK;
}

AM_S64& CParam64::operator[](const AM_INT index)
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Return mCount.\n", index);
        return mParam[mCount];
    }
    return mParam[index];
}

const AM_S64& CParam64::operator[](const AM_INT index) const
{
    if(index < 0 || index >= mCount)
    {
        AM_ERROR("CParam: Your Index Is Illegal(%d), Return mCount.\n", index);
        return mParam[mCount];
    }
    return mParam[index];
}

AM_ERR CParam64::CheckNum(AM_INT parNum)
{
    if(parNum != mCount)
    {
        AM_ERROR("Config Param Count Failed!\n");
        return ME_BAD_PARAM;
    }
    return ME_OK;
}

AM_ERR CParam64::CheckZero(AM_INT parNum)
{
    AM_ERR err;
    AM_INT index;
    if((err = CheckNum(parNum)) != ME_OK)
        return err;
    for(index = 0; index < mCount; index++)
    {
        if(mParam[index] == 0)
            return ME_ERROR;
    }
    return ME_OK;
}

