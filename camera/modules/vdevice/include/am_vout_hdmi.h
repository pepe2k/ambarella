/*******************************************************************************
 * am_vout_hdmi.h
 *
 * Histroy:
 *  2012-3-6 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMVOUTHDMI_H_
#define AMVOUTHDMI_H_

class AmVoutHdmi: public AmVout
{
  public:
    AmVoutHdmi(int iavfd = -1): AmVout(AM_VOUT_TYPE_HDMI, iavfd){}
    virtual ~AmVoutHdmi(){}

  public:
    virtual bool is_hdmi_plugged();

  protected:
    virtual bool init(VoutInitMode initMode, VoutDspMode dspMode, bool force);
};

#endif /* AMVOUTHDMI_H_ */
