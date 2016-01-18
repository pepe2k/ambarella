/*******************************************************************************
 * input_playback_if.h
 *
 * History:
 *   2013-3-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef INPUT_PLAYBACK_IF_H_
#define INPUT_PLAYBACK_IF_H_

extern const AM_IID IID_IInput_Playback;

class IInputPlayback: public IInterface
{
  public:
    DECLARE_INTERFACE(IInputPlayback, IID_IInput_Playback);
    virtual AM_ERR AddUri(const char* uri)   = 0;
    virtual AM_ERR Play(const char* uri)     = 0;
    virtual AM_ERR Stop()                    = 0;
    virtual AM_ERR CreateSubGraph()          = 0;
    virtual IPacketPin* GetModuleInputPin()  = 0;
    virtual IPacketPin *GetModuleOutputPin() = 0;
};

#endif /* INPUT_PLAYBACK_IF_H_ */
