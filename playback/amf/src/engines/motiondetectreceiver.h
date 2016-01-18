
/**
 * motiondetectreceiver.h
 *
 * History:
 *    2013/06/04 - [GLiu] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MOTIONDETECT_RECEIVER_H__
#define __MOTIONDETECT_RECEIVER_H__

#define MOTIONDETECT_SERV_PORT (13579)
#define MOTIONDETECT_MAX_CLIENT_NUM 8
#define EVENT_TIME_MAX_LEN     (32)
enum MD_MSG{
    MD_EVENT_START,
    MD_EVENT_STOP,
    MD_EVENT_MAX = 0xFF,
};
typedef struct MD_EVENT_ST_tag{
    unsigned int ulMsgID;
    char acEventTime[EVENT_TIME_MAX_LEN];
}MD_EVENT_ST, *MD_EVENT_PST;

class CMotionDetectReceiver: public CActiveObject
{
    typedef CActiveObject inherited;

private:
    CMotionDetectReceiver(void *context);
    int Construct();
    virtual ~CMotionDetectReceiver();
    virtual void OnRun();

public:
    static CMotionDetectReceiver* Create(void *context);
    virtual void Delete() {inherited::Delete();};
    int SetMDMsgHandler(void* ctrler, void *context);
    int Stop() { return mpWorkQ->Stop(); };

private:
    bool mbRun;
    IMDecControl *pCtrler;
    void *pContext;
    int mSock;
};

#endif //__MOTIONDETECT_RECEIVER_H__

