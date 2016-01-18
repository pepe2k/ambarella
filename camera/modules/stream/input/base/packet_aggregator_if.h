/*******************************************************************************
 * packet_aggregator_if.h
 *
 * History:
 *   2013年12月2日 - [ypchang] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef PACKET_AGGREGATOR_IF_H_
#define PACKET_AGGREGATOR_IF_H_

extern const AM_IID IID_IPacketAggregator;

class IPacketAggregator: public IInterface
{
  public:
    DECLARE_INTERFACE(IPacketAggregator, IID_IPacketAggregator);
    virtual AM_ERR SetStreamNumber(AM_UINT streamNum) = 0;
    virtual AM_ERR SetInputNumber(AM_UINT inputNum)   = 0;
};

#endif /* PACKET_AGGREGATOR_IF_H_ */
