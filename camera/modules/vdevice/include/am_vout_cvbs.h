/*******************************************************************************
 * am_vout_cvbs.h
 *
 * Histroy:
 *  2012-8-10 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AMVOUTCVBS_H_
#define AMVOUTCVBS_H_
#include "am_vout.h"

class AmVoutCvbs: public AmVout
{
  public:
    AmVoutCvbs(int iavfd = -1): AmVout(AM_VOUT_TYPE_CVBS, iavfd){}
    virtual ~AmVoutCvbs(){}

  protected:
    virtual bool init(VoutInitMode initMode, VoutDspMode dspMode, bool force);
};

#endif /* AMVOUTCVBS_H_ */
