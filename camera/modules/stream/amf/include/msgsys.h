/**
 * msgsys.h
 *
 * History:
 *    2009/12/8 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MSGSYS_H__
#define __MSGSYS_H__

class CThread;
class CQueue;

class CMsgPort;
class CMsgSys;

//-----------------------------------------------------------------------
//
// CMsgPort
//
//-----------------------------------------------------------------------
class CMsgPort: public IMsgPort
{
    friend class CMsgSys;

  public:
    static CMsgPort* Create(IMsgSink *pMsgSink, CMsgSys *pMsgSys);

  private:
    CMsgPort(IMsgSink *pMsgSink) :
        mpMsgSink(pMsgSink),
        mpQ(NULL)
    {
    }
    AM_ERR Construct(CMsgSys *pMsgSys);
    virtual ~CMsgPort();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IMsgPort
    virtual AM_ERR PostMsg(AM_MSG& msg);

  private:
    IMsgSink *mpMsgSink;
    CQueue *mpQ;
};

//-----------------------------------------------------------------------
//
// CMsgSys
//
//-----------------------------------------------------------------------
class CMsgSys
{
    friend class CMsgPort;

  public:
    static CMsgSys* Create();
    void Delete();

  private:
    CMsgSys() :
        mpMainQ(NULL),
        mpThread(NULL)
    {
    }
    AM_ERR Construct();
    virtual ~CMsgSys();

  private:
    static AM_ERR ThreadEntry(void *p);
    AM_ERR MainLoop();

  private:
    CQueue *mpMainQ;
    CThread *mpThread;
};

#endif

