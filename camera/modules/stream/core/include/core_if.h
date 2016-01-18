/*******************************************************************************
 * core_if.h
 *
 * Histroy:
 *   2012-10-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef CORE_IF_H_
#define CORE_IF_H_

extern const AM_IID IID_ICore;

class ICore: public IInterface
{
  public:
    DECLARE_INTERFACE(ICore, IID_ICore);
    virtual AM_ERR CreateSubGraph() = 0;
    virtual AM_ERR PrepareToRun() = 0;
    virtual IPacketPin *GetModuleInputPin() = 0;
    virtual IPacketPin *GetModuleOutputPin() = 0;
    virtual bool IsReadyForEvent() = 0;
    virtual AM_ERR SetEventStreamId(AM_UINT StreamId) = 0;
    virtual AM_ERR SetEventDuration(AM_UINT history_duration,
                                    AM_UINT future_duration) = 0;
};

#endif /* CORE_IF_H_ */
