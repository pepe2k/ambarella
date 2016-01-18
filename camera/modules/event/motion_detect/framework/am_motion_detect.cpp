/*******************************************************************************
 * am_motion_detect.cpp
 *
 * Histroy:
 *  2014-1-3  [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/


#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>

#include "am_include.h"
#include "am_utility.h"
#include "am_motion_detect.h"
#include "AmMotionDetect.h"

static AmMotionDetect* md_instance = NULL;

static inline int check_md(void)
{
  if (!md_instance) {
    ERROR("motion detection instance is not created yet\n");
    return -1;
  }
  return 0;
}

int module_start(void)
{
  if (!md_instance) {
    md_instance = AmMotionDetect::get_instance();
  }

  return md_instance->start();
}

int module_stop(void)
{
  if (check_md() < 0) {
    return -1;
  }

  return md_instance->stop();
}

int module_destroy(void)
{
  if (check_md() < 0) {
    return -1;
  }

  delete md_instance;
  md_instance = NULL;
  return 0;
}

int module_set_config(MODULE_CONFIG* config)
{
  if (check_md() < 0) {
    return -1;
  }

  if (!config) {
    return -1;
  }

  return md_instance->set_config(config->key, config->value);
}

int module_get_config(MODULE_CONFIG* config)
{
  if (check_md() < 0) {
    return -1;
  }

  if (!config) {
    return -1;
  }

  return md_instance->get_config(config->key, config->value);
}

MODULE_ID module_get_id(void){
  return MOTION_DECT;
}

