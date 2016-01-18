/**
 * am_mw.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_MW_H__
#define __AM_MW_H__

#undef EOF

extern const AM_IID IID_IEngine;
extern const AM_IID IID_IFilter;
extern const AM_IID IID_IActiveObject;

extern const AM_IID IID_IRenderer;
extern const AM_IID IID_IRecognizer;

extern const AM_IID IID_IClockSource;
extern const AM_IID IID_IClockObserver;
extern const AM_IID IID_IClockManager;

class IActiveObject;

class CQueue;

//-----------------------------------------------------------------------
//
// IActiveObject
//
//-----------------------------------------------------------------------
class IActiveObject: public IInterface
{
  public:
    enum
    {
      CMD_TERMINATE,
      CMD_RUN,
      CMD_STOP,
      CMD_LAST,
    };

    struct CMD
    {
        AM_UINT code;
        void *pExtra;
        CMD(int cid) :
            pExtra(NULL)
        {
          code = cid;
        }
        CMD() :
            code(0),
            pExtra(NULL)
        {
        }
    };

  public:
    DECLARE_INTERFACE(IActiveObject, IID_IActiveObject);
    virtual const char *GetName() = 0;
    virtual void OnRun() = 0;
    virtual void OnCmd(CMD& cmd) = 0;
};

//-----------------------------------------------------------------------
//
// CMediaFormat
//
//-----------------------------------------------------------------------
struct CMediaFormat
{
    const AM_GUID *pMediaType;
    const AM_GUID *pSubType;
    const AM_GUID *pFormatType;
    AM_UINT formatSize;
    AM_INTPTR format;
    CMediaFormat()
    {
      pMediaType = &GUID_NULL;
      pSubType = &GUID_NULL;
      pFormatType = &GUID_NULL;
      formatSize = 0;
      format = 0;
    }
};

//-----------------------------------------------------------------------
//
// IEngine
//
//-----------------------------------------------------------------------
class IEngine: public IInterface
{
  public:
    enum
    {
      MSG_ERROR,
      MSG_EOS,
      MSG_ABORT,
      MSG_TSE,
      MSG_OVFL,
      MSG_OK,
      MSG_EOF,
      MSG_COMMON_LAST,
    };

  public:
    DECLARE_INTERFACE(IEngine, IID_IEngine);
    virtual AM_ERR PostEngineMsg(AM_MSG& msg) = 0;
    /*virtual void *QueryEngineInterface(AM_REFIID refiid) = 0;*/

  public:
    // helper
    AM_ERR PostEngineMsg(AM_UINT code)
    {
      AM_MSG msg;
      msg.code = code;
      return PostEngineMsg(msg);
    }
};

//-----------------------------------------------------------------------
//
// IRenderer
//
//-----------------------------------------------------------------------
class IRenderer: public IInterface
{
  public:
    DECLARE_INTERFACE(IRenderer, IID_IRenderer);
    virtual AM_ERR Pause() = 0;
    virtual AM_ERR Resume() = 0;
};

//-----------------------------------------------------------------------
//
// IClockSource
//
//-----------------------------------------------------------------------
class IClockSource: public IInterface
{
  public:
    DECLARE_INTERFACE(IClockSource, IID_IClockSource);
    virtual AM_PTS GetClockPTS() = 0;
};

//-----------------------------------------------------------------------
//
// IClockObserver
//
//-----------------------------------------------------------------------
class IClockObserver: public IInterface
{
  public:
    DECLARE_INTERFACE(IClockObserver, IID_IClockObserver);
    virtual AM_ERR OnTimer(AM_PTS curr_pts) = 0;
};

//-----------------------------------------------------------------------
//
// IClockManager
//
//-----------------------------------------------------------------------
class IClockManager: public IInterface
{
  public:
    DECLARE_INTERFACE(IClockManager, IID_IClockManager);
    virtual AM_ERR SetTimer(IClockObserver *pObserver, AM_PTS pts) = 0;
    virtual AM_PTS GetCurrentTime() = 0;
    virtual AM_ERR StartClock() = 0;
    virtual void StopClock() = 0;
    virtual void SetSource(IClockSource *pSource) = 0;
};

#endif

