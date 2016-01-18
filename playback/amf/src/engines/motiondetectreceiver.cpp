
/**
 * streamming_server.cpp
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

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#if PLATFORM_ANDROID
#include <basetypes.h>
#else
#include "basetypes.h"
#endif
#include "am_mdec_if.h"
#include "motiondetectreceiver.h"

CMotionDetectReceiver::CMotionDetectReceiver(void *context):
        inherited("CMotionDetectReceiver"),
        mbRun(true),
        pCtrler(NULL),
        pContext(context),
        mSock(0)
    {AMLOG_PRINTF("***new CMotionDetectReceiver.\n"); }

CMotionDetectReceiver::~CMotionDetectReceiver()
{
    AMLOG_PRINTF("***~CMotionDetectReceiver.\n");
    if(mSock){
        close(mSock);
        mSock = 0;
    }
}

int CMotionDetectReceiver::Construct()
{

    int err = inherited::Construct();
    if (err != 0) {
        AM_ERROR("CGeneralDecoderVE::Construct fail err %d .\n", err);
        return err;
    }

    int opt = 1;
    int len = sizeof(opt);
    mSock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    if (mSock < 0) {
        AMLOG_PRINTF("CMotionDetectReceiver::Construct, sock create error!\n");
        return -1;
    }
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(MOTIONDETECT_SERV_PORT);
    if (bind(mSock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
            AMLOG_PRINTF("CMotionDetectReceiver::Construct, sock bind error!\n");
            return -1;
    }
    AMLOG_PRINTF("CMotionDetectReceiver::Construct, sock bind OK.\n");
    if (listen(mSock, MOTIONDETECT_MAX_CLIENT_NUM) < 0) {
        AMLOG_PRINTF("CMotionDetectReceiver::Construct, listen error!\n");
        return -1;
    }
    AMLOG_PRINTF("CMotionDetectReceiver::Construct, start listen...\n");
    PostCmd(CMD_RUN);
    AMLOG_PRINTF("CMotionDetectReceiver::Construct, PostCmd CMD_RUN done.\n");
    return 0;
}

CMotionDetectReceiver* CMotionDetectReceiver::Create(void *context)
{
    CMotionDetectReceiver *result = new CMotionDetectReceiver(context);
    if (result && result->Construct() != 0) {
        delete result;
        result = NULL;
        }
    return result;
}

void CMotionDetectReceiver::OnRun()
{
    CMD cmd;
    mbRun = true;
    int new_fd=0;
    socklen_t other_size = sizeof(struct sockaddr_in);
    struct sockaddr_in other_addr;

    fd_set fdR;
    struct timeval timeout={0,0};
    int len;
    char buf[2048];

    AMLOG_PRINTF("CMotionDetectReceiver::OnRun:\n");

    while (mbRun) {

        FD_ZERO(&fdR);
        FD_SET(mSock, &fdR);
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;
        switch (select(mSock + 1, &fdR, NULL, NULL, &timeout)) {
            case -1://error
                mbRun = false;
                break;
            case 0://timeout
                if(mpWorkQ->PeekCmd(cmd)){
                    if (cmd.code == CMD_STOP) {
                        mbRun = false;
                        CmdAck(ME_OK);
                        break;
                    }
                    AMLOG_ERROR("How to process these cmd %d.\n", cmd.code);
                    CmdAck(ME_OK);
                }
                break;
            default:
                if (FD_ISSET(mSock, &fdR)) {

                    new_fd = -1;
                    new_fd = accept(mSock, (struct sockaddr *)&other_addr, &other_size);
                    if (new_fd < 0) {
                        AMLOG_ERROR("accept error\n");
                        break;
                    }
                    AMLOG_DEBUG("accept, from %s.\n", inet_ntoa(other_addr.sin_addr/*.s_addr*/));

                    if ((len=recv(new_fd, buf, 128, 0)) <=0) {
                        AMLOG_ERROR("recv error!\n");
                        close(new_fd);
                        break;
                    }

                    MD_EVENT_ST* evt = (MD_EVENT_ST*)(&(buf[0]));
                    AMLOG_DEBUG("Received: %u, %s, len=%d.\n",evt->ulMsgID, evt->acEventTime, len);
                    if(pCtrler) pCtrler->MotionDetecthandler(inet_ntoa(other_addr.sin_addr/*.s_addr*/), (void*)evt);
                }
                close(new_fd);
                break;
        }
    }

}

int CMotionDetectReceiver::SetMDMsgHandler(void* ctrler, void *context)
{
    if(pContext == context){
        pCtrler = (IMDecControl*)ctrler;
        return 0;
    }else{
        AMLOG_ERROR("SetMDMsgHandler:%p %p %p.\n", ctrler, context, pContext);
        return -1;
    }
}

