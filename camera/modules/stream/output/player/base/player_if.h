/*******************************************************************************
 * player_if.h
 *
 * History:
 *   2013-3-21 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef PLAYER_IF_H_
#define PLAYER_IF_H_

extern AM_REFIID IID_IPlayer;
class IPlayer: public IInterface
{
  public:
    DECLARE_INTERFACE(IPlayer, IID_IPlayer);
    virtual AM_ERR Start()            = 0;
    virtual AM_ERR Stop()             = 0;
    virtual AM_ERR Pause(bool enable) = 0;
};

#endif /* PLAYER_IF_H_ */
