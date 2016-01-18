/*******************************************************************************
 * output_playback_if.h
 *
 * History:
 *   2013-3-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef OUTPUT_PLAYBACK_IF_H_
#define OUTPUT_PLAYBACK_IF_H_

extern const AM_IID IID_IOutputPlayback;

class IOutputPlayback: public IInterface
{
  public:
    DECLARE_INTERFACE(IOutputPlayback, IID_IOutputPlayback);
    virtual AM_ERR Stop()                   = 0;
    virtual AM_ERR Pause(bool enable)       = 0;
    virtual AM_ERR CreateSubGraph()         = 0;
    virtual IPacketPin* GetModuleInputPin() = 0;
};

#endif /* OUTPUT_PLAYBACK_IF_H_ */
