/*
 * am_config_motiondetect.h
 *
 * @Author: HuaiShun Hu
 * @Email : hshu@ambarella.com
 * @Time  : 26/02/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef AM_CONFIG_MOTIONDETECT_H_
#define AM_CONFIG_MOTIONDETECT_H_

class AmConfigMotionDetect: public AmConfigBase
{
public:
   AmConfigMotionDetect(const char *configFileName) :
      AmConfigBase(configFileName),
      mMotionDetectParameters(NULL){}
   virtual ~AmConfigMotionDetect()
   {
      delete mMotionDetectParameters;
   }

public:
   MotionDetectParameters *get_motiondetect_config();
   void set_motiondetect_config(MotionDetectParameters *config);

private:
   MotionDetectParameters *mMotionDetectParameters;
   inline void load_param_value(uint8_t roi,
                                MotionDetectParam param,
                                char *param_name,
                                uint32_t min, uint32_t max,
                                uint32_t def);
};

#endif /* AM_CONFIG_MOTIONDETECT_H_ */
