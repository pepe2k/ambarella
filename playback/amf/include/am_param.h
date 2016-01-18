/*
 * am_param.h
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
#ifndef __AMBA_FRAMEWORK_AM_PARAM_H__
#define __AMBA_FRAMEWORK_AM_PARAM_H__

#define CPARAM_MAX_PARAM_NUM 20

//using CParam param(3);
//param[0] = x;...
//if malloc by new, use PutParam() but not overload []; or (*param_by_new)[0] = x;
class CParam
{
public:
    CParam(AM_INT count);
    virtual ~CParam() {}

    AM_ERR PutParam(AM_INT index, AM_INT value);
    AM_INT GetCount() { return mCount;}
    AM_INT& operator[](const AM_INT index);
    const AM_INT& operator[](const AM_INT index) const;

    AM_ERR CheckNum(AM_INT parNum);
    AM_ERR CheckZero(AM_INT parNum);

private:
    AM_INT mParam[CPARAM_MAX_PARAM_NUM];
    AM_INT mCount;
};

class CParam64
{
public:
    CParam64(AM_INT count);
    virtual ~CParam64() {}

    AM_ERR PutParam(AM_INT index, AM_S64 value);
    AM_INT GetCount() { return mCount;}
    AM_S64& operator[](const AM_INT index);
    const AM_S64& operator[](const AM_INT index) const;

    AM_ERR CheckNum(AM_INT parNum);
    AM_ERR CheckZero(AM_INT parNum);

private:
    AM_S64 mParam[CPARAM_MAX_PARAM_NUM];
    AM_INT mCount;
};
#endif
