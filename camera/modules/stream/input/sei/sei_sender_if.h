/*******************************************************************************
 * sei_sender_if.h
 *
 * Histroy:
 *   2012-11-20 - [Dongge Wu] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef SEI_SENDER_IF_H_
#define SEI_SENDER_IF_H_

extern const AM_IID IID_ISeiSender;

class ISeiSender: public IInterface
{
  public:
    DECLARE_INTERFACE(ISeiSender, IID_ISeiSender);
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Stop()  = 0;
    virtual AM_ERR SendSEI(void *pUserData, AM_U32 len) = 0;
};

#endif /* SEI_SENDER_IF_H_ */
