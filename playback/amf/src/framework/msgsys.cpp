
/**
 * msgsys.cpp
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

//#define LOG_NDEBUG 0
//#define LOG_TAG "msg_sys"
//#define AMDROID_DEBUG
#include <string.h>
#include "am_new.h"

#include "am_types.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"

#include "msgsys.h"

//-----------------------------------------------------------------------
//
// CMsgPort//
//
//-----------------------------------------------------------------------
CMsgPort* CMsgPort::Create(IMsgSink *pMsgSink,CMsgSys *pMsgSys)
{
	CMsgPort *result = new CMsgPort(pMsgSink,pMsgSys);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CMsgPort::Construct()
{
	if ((mpQ = CQueue::Create(mpM->mpMainQ, this, sizeof(PortMsg), 1)) == NULL)
		return ME_NO_MEMORY;
         if ((mpMutex = CMutex::Create(false)) == NULL)
                  return ME_OS_ERROR;
         if ((mpCondSend = CCondition::Create()) == NULL)
                  return ME_OS_ERROR;
	return ME_OK;
}

CMsgPort::~CMsgPort()
{
	// todo
	//AM_INFO("~CMsgPort\n");
	AM_DELETE(mpQ);
         AM_DELETE(mpMutex);
         AM_DELETE(mpCondSend);
         //AM_INFO("~CMsgPort Done\n");
}

void *CMsgPort::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IMsgPort)
		return (IMsgPort*)this;
	if (refiid == IID_IInterface)
		return (IInterface*)this;
	return NULL;
}

void CMsgPort::Delete()
{
	// todo
	delete this;
}

AM_ERR CMsgPort::PostMsg(AM_MSG& msg)
{
    PortMsg MSG;
    AM_ERR err;
    MSG.msg = msg;
    MSG.bReply = false;
    err =  mpQ->PutData(&MSG, sizeof(MSG));
    return err;
}

AM_ERR CMsgPort::SendMsg(AM_MSG& msg)
{
    AUTO_LOCK(mpMutex);
    //mbWaitReply = true;
    PortMsg MSG;
    MSG.msg = msg;
    MSG.bReply = true;

    if(mpQ->PutData(&MSG, sizeof(MSG)) != ME_OK)
    {
        AM_INFO("!SendMsg\n");
    }
    //AM_INFO("ww\n");
    mpCondSend->Wait(mpMutex);
    //AM_INFO("ww donw\n");
    return mErr;
}

void CMsgPort::Reply(AM_ERR result)
{
    AUTO_LOCK(mpMutex);
    mErr = result;
     //AM_INFO("replay\n");
    mpCondSend->Signal();
    //mbWaitReply = false;
}
//-----------------------------------------------------------------------
//
// CMsgSys
//
//-----------------------------------------------------------------------

//CMsgSys *CMsgSys::mpInstance = NULL;

CMsgSys * CMsgSys::Init()
{
	CMsgSys *result = new CMsgSys;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
		//return ME_NO_MEMORY;
	}
	return result;
}

AM_ERR CMsgSys::Construct()
{
	if ((mpMainQ = CQueue::Create(NULL, this, sizeof(AM_UINT), 1)) == NULL)
		return ME_OS_ERROR;

	if ((mpThread = CThread::Create("AMFMsgSys", ThreadEntry, this)) == NULL)
		return ME_OS_ERROR;
         mpThread->SetThreadPrio(1, 1);
	return ME_OK;
}

CMsgSys::~CMsgSys()
{
	// stop messaging thread
	// todo - if this is called from our thread...

	AM_UINT msg = 0;
	AM_ERR err = mpMainQ->SendMsg(&msg, sizeof(msg));
	AM_ASSERT_OK(err);

	AM_DELETE(mpThread);

	// todo - delete all sub-Qs
	AM_DELETE(mpMainQ);
}

void CMsgSys::Delete()
{
	//delete mpInstance;
	//mpInstance = NULL;
	delete this;
}

AM_ERR CMsgSys::ThreadEntry(void *p)
{
	return ((CMsgSys*)p)->MainLoop();
}

AM_ERR CMsgSys::MainLoop()
{
	CMsgPort::PortMsg MSG;

	while (1) {
		AM_UINT dummy;
		CQueue::QType qtype;
		CQueue::WaitResult result;
		qtype = mpMainQ->WaitDataMsgCircularly(&dummy, sizeof(dummy), &result);
		if (qtype == CQueue::Q_MSG) {
			mpMainQ->Reply(ME_OK);
			break;
		}

		if (qtype == CQueue::Q_DATA) {
			if (result.pDataQ->PeekData(&MSG, sizeof(MSG))) {
                                    CMsgPort* pMsgPort = ((CMsgPort*)result.pOwner);
				IMsgSink *pMsgSink = pMsgPort->mpMsgSink;
				if (pMsgSink) {
					pMsgSink->MsgProc(MSG.msg);
				}
                                    if(MSG.bReply == true){
                                        pMsgPort->Reply(ME_OK);
                                    }
			}
		}
	}

	return ME_OK;
}

