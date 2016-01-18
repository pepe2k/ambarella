/*
 * am_config_audiodetect.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 14/02/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef AM_CONFIG_AUDIODETECT_H_
#define AM_CONFIG_AUDIODETECT_H_

class AmConfigAudioDetect: public AmConfigBase
{
public:
   AmConfigAudioDetect(const char *configFileName) :
      AmConfigBase(configFileName),
      mAudioDetectParameters(NULL){}
   virtual ~AmConfigAudioDetect()
   {
      delete mAudioDetectParameters;
   }

public:
   AudioDetectParameters *get_audiodetect_config();
   void set_audiodetect_config(AudioDetectParameters *config);

private:
   AudioDetectParameters *mAudioDetectParameters;
};

#endif /* AM_CONFIG_PHOTO_H_ */
