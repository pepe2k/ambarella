/*******************************************************************************
 * am_config_photo.h
 *
 * Histroy:
 *  2012-4-5 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CONFIG_PHOTO_H_
#define AM_CONFIG_PHOTO_H_

class AmConfigPhoto: public AmConfigBase
{
  public:
    AmConfigPhoto(const char *configFileName) :
      AmConfigBase(configFileName),
      mPhotoParameters(NULL){}
    virtual ~AmConfigPhoto()
    {
      delete mPhotoParameters;
    }

  public:
    PhotoParameters *get_photo_config();
    void set_photo_config(PhotoParameters *config);

  private:
    PhotoParameters *mPhotoParameters;
};


#endif /* AM_CONFIG_PHOTO_H_ */
