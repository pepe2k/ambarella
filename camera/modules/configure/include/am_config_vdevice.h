/*******************************************************************************
 * am_config_vdevice.h
 *
 * Histroy:
 *  2012-3-8 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMCONFIGVDEVICE_H_
#define AMCONFIGVDEVICE_H_

/*******************************************************************************
 * AmConfigVDevice class provides methods to retrieve and set configs of video
 * device, it is derived from AmConfig implementing vDevice Configuration
 * Retrieving and Setting methods
 ******************************************************************************/
class AmConfigVDevice: public AmConfigBase
{
  public:
    AmConfigVDevice(const char *configFileName);
    virtual ~AmConfigVDevice();

  public:
    VDeviceParameters *get_vdev_config();
    void set_vdev_config(VDeviceParameters *vdevConfig);

  private:
    VDeviceParameters *mVdevParameters;
};

#endif /* AMCONFIGVDEVICE_H_ */
