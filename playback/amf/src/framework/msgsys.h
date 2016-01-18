
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
// CMsgPort//
//
//-----------------------------------------------------------------------
class CMsgPort: public IMsgPort
{
	friend class CMsgSys;
         struct PortMsg
         {
            AM_MSG msg;
            bool bReply;
         };

public:
	static CMsgPort* Create(IMsgSink *pMsgSink,CMsgSys *pMsgSys);

private:
	CMsgPort(IMsgSink *pMsgSink,CMsgSys *pMsgSys):
		mpMsgSink(pMsgSink),
		mpM(pMsgSys),
		mpQ(NULL),
		mpCondSend(NULL),
		mpMutex(NULL)
		//mbWaitReply(false)
	{}
	AM_ERR Construct();
	virtual ~CMsgPort();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();

	// IMsgPort
	virtual AM_ERR PostMsg(AM_MSG& msg);
         virtual AM_ERR SendMsg(AM_MSG& msg);
         void Reply(AM_ERR result);
private:
	IMsgSink *mpMsgSink;
    	CMsgSys *mpM;
	CQueue *mpQ;

        	CCondition *mpCondSend;
         CMutex* mpMutex;
         AM_ERR mErr;
         //bool mbWaitReply;
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
	static CMsgSys* Init();
	void Delete();
	//static CMsgSys* Instance() { return mpInstance; }

private:
	CMsgSys(): mpMainQ(NULL), mpThread(NULL) {}
	AM_ERR Construct();
	virtual ~CMsgSys();

private:
	static AM_ERR ThreadEntry(void *p);
	AM_ERR MainLoop();

private:
	CQueue *mpMainQ;
	CThread *mpThread;

	//static CMsgSys *mpInstance;
};

#endif

