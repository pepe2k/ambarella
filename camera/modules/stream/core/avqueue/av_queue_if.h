/*
 * av_queue.h
 *
 * History:
 *	2012/9/26 - [Shupeng Ren] created file
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AV_QUEUE_IF_H__
#define __AV_QUEUE_IF_H__

extern const AM_IID IID_IAVQueue;

class IAVQueue: public IInterface
{
  public:
    DECLARE_INTERFACE(IAVQueue, IID_IAVQueue);
    virtual bool IsReadyForEvent() = 0;
    virtual void SetEventStreamId(AM_UINT StreamId) = 0;
    virtual void SetEventDuration(AM_UINT history_duration,
                                  AM_UINT future_duration) = 0;
};

extern IPacketFilter *CreateAVQueue(IEngine *);

#endif //__AV_QUEUE_IF_H__
