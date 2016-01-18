/**
 * am_if.h
 *
 * History:
 *    2007/11/5 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_IF_H__
#define __AM_IF_H__

extern const AM_IID IID_IInterface;
extern const AM_IID IID_IMsgPort;
extern const AM_IID IID_IMsgSink;
extern const AM_IID IID_IMediaControl;
extern const AM_IID IID_IAudioSource;

class IInterface;
class IMsgSink;
class IMediaControl;

struct AM_MSG
{
    AM_UINT code;
    AM_UINT sessionID;
    AM_INTPTR p0;
    AM_INTPTR p1;
};

#define DECLARE_INTERFACE(ifName, iid) \
  static ifName* GetInterfaceFrom(IInterface *pInterface) \
  { \
    return (ifName*)pInterface->GetInterface(iid); \
  }

//-----------------------------------------------------------------------
//
// IInterface
//
//-----------------------------------------------------------------------
class IInterface
{
  public:
    virtual void *GetInterface(AM_REFIID refiid) = 0;
    virtual void Delete() = 0;
    virtual ~IInterface() {}
};

//-----------------------------------------------------------------------
//
// IMsgPort
//
//-----------------------------------------------------------------------
class IMsgPort: public IInterface
{
  public:
    DECLARE_INTERFACE(IMsgPort, IID_IMsgPort);
    virtual AM_ERR PostMsg(AM_MSG& msg) = 0;
};

//-----------------------------------------------------------------------
//
// IMsgSink
//
//-----------------------------------------------------------------------
class IMsgSink: public IInterface
{
  public:
    DECLARE_INTERFACE(IMsgSink, IID_IMsgSink);
    virtual void MsgProc(AM_MSG& msg) = 0;
};

//-----------------------------------------------------------------------
//
// IEngine
//
//-----------------------------------------------------------------------
class IMediaControl: public IInterface
{
  public:
    // msg post to app
    enum
    {
      MSG_STATE_CHANGED,
    };

  public:
    DECLARE_INTERFACE(IMediaControl, IID_IMediaControl);
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink) = 0;
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&),
                                     void *context) = 0;
};

//-----------------------------------------------------------------------
//
// global APIs
//
//-----------------------------------------------------------------------
extern AM_ERR AMF_Init();
extern void AMF_Terminate();

#endif
