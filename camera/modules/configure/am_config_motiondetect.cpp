/*
 * am_config_motiondetect.cpp
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
#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_motiondetect.h"

MotionDetectParameters* AmConfigMotionDetect::get_motiondetect_config()
{
   MotionDetectParameters *ret = NULL;
   if (init()) {
      if (!mMotionDetectParameters) {
         mMotionDetectParameters = new MotionDetectParameters();
      }
      if (AM_LIKELY(mMotionDetectParameters)) {
        memset(mMotionDetectParameters, 0, sizeof(*mMotionDetectParameters));

        char* string = (char *) calloc(1, sizeof(char) * 128);

        strncpy(mMotionDetectParameters->algorithm,
                get_string("MD_CONFIG:Algo", "MSE"),
                sizeof(mMotionDetectParameters->algorithm));

        if (strcmp(mMotionDetectParameters->algorithm, "MOG2") != 0  &&
            strcmp(mMotionDetectParameters->algorithm, "MSE")  != 0) {
          WARN("invalid algorithm type:%s, using default algo MSE\n",
               mMotionDetectParameters->algorithm);
          strncpy(mMotionDetectParameters->algorithm, "MSE",
                  sizeof(mMotionDetectParameters->algorithm));
          mMotionDetectParameters->config_changed = 1;
        }

        if (strcmp(mMotionDetectParameters->algorithm, "MOG2") == 0) {
          for (int i = 0; i < MAX_MOTION_DETECT_ROI_NUM; i++) {
            sprintf(string, "ROI_%d:Left", i);
            load_param_value(i, MD_ROI_LEFT, string, 0, 15, 0);
            sprintf(string, "ROI_%d:Right", i);
            load_param_value(i, MD_ROI_RIGHT, string,
                            mMotionDetectParameters->value[i][MD_ROI_LEFT],
                            15, 15);
            sprintf(string, "ROI_%d:Top", i);
            load_param_value(i, MD_ROI_TOP, string, 0, 15, 0);
            sprintf(string, "ROI_%d:Bottom", i);
            load_param_value(i, MD_ROI_BOTTOM, string,
                            mMotionDetectParameters->value[i][MD_ROI_TOP],
                            15, 15);
            sprintf(string, "ROI_%d:Threshold", i);
            load_param_value(i, MD_ROI_THRESHOLD, string, 0, 200, 50);
            sprintf(string, "ROI_%d:Sensitivity", i);
            load_param_value(i, MD_ROI_SENSITIVITY, string, 0, 100, 100);
            sprintf(string, "ROI_%d:Valid", i);
            load_param_value(i, MD_ROI_VALID, string, 0, 1, 1);
            sprintf(string, "ROI_%d:OutputMotion", i);
            load_param_value(i, MD_ROI_OUTPUT_MOTION, string, 0, 1, 0);
          }
        } else {
          for (int i = 0; i < MAX_MOTION_DETECT_ROI_NUM; i++) {
            sprintf(string, "ROI_%d:Left", i);
            load_param_value(i, MD_ROI_LEFT, string, 0, 11, 0);
            sprintf(string, "ROI_%d:Right", i);
            load_param_value(i, MD_ROI_RIGHT, string,
                            mMotionDetectParameters->value[i][MD_ROI_LEFT],
                            11, 11);
            sprintf(string, "ROI_%d:Top", i);
            load_param_value(i, MD_ROI_TOP, string, 0, 7, 0);
            sprintf(string, "ROI_%d:Bottom", i);
            load_param_value(i, MD_ROI_BOTTOM, string,
                            mMotionDetectParameters->value[i][MD_ROI_TOP],
                            7, 7);
            sprintf(string, "ROI_%d:Threshold", i);
            load_param_value(i, MD_ROI_THRESHOLD, string, 0, 200, 50);
            sprintf(string, "ROI_%d:Sensitivity", i);
            load_param_value(i, MD_ROI_SENSITIVITY, string, 0, 100, 100);
            sprintf(string, "ROI_%d:Valid", i);
            load_param_value(i, MD_ROI_VALID, string, 0, 1, 1);
            sprintf(string, "ROI_%d:OutputMotion", i);
            load_param_value(i, MD_ROI_OUTPUT_MOTION, string, 0, 1, 0);
          }
        }

        free(string);

        if (mMotionDetectParameters->config_changed) {
          set_motiondetect_config(mMotionDetectParameters);
        }
      }

      ret = mMotionDetectParameters;
   }

   return ret;
}

void AmConfigMotionDetect::set_motiondetect_config(MotionDetectParameters *config)
{
  char* string = (char *)calloc(1, sizeof(char) * 128);
  if (AM_LIKELY(config)) {
    if (init()) {

      set_value("MD_CONFIG:Algo", config->algorithm);
      for (int i = 0; i < MAX_MOTION_DETECT_ROI_NUM; i++) {
        sprintf(string, "ROI_%d:Left", i);
        set_value(string, config->value[i][MD_ROI_LEFT]);
        sprintf(string, "ROI_%d:Right", i);
        set_value(string, config->value[i][MD_ROI_RIGHT]);
        sprintf(string, "ROI_%d:Top", i);
        set_value(string, config->value[i][MD_ROI_TOP]);
        sprintf(string, "ROI_%d:Bottom", i);
        set_value(string, config->value[i][MD_ROI_BOTTOM]);
        sprintf(string, "ROI_%d:Threshold", i);
        set_value(string, config->value[i][MD_ROI_THRESHOLD]);
        sprintf(string, "ROI_%d:Sensitivity", i);
        set_value(string, config->value[i][MD_ROI_SENSITIVITY]);
        sprintf(string, "ROI_%d:Valid", i);
        set_value(string, config->value[i][MD_ROI_VALID]);
        sprintf(string, "ROI_%d:OutputMotion", i);
        set_value(string, config->value[i][MD_ROI_OUTPUT_MOTION]);
      }

      config->config_changed = 0;
      save_config();
    } else {
          WARN("Failed openint %s, motion detect configuration NOT saved!", mConfigFile);
    }
  }
  free(string);
}
void AmConfigMotionDetect::load_param_value(uint8_t roi,
                                            MotionDetectParam param,
                                            char *param_name,
                                            uint32_t min, uint32_t max,
                                            uint32_t def)
{
  if (AM_UNLIKELY(!mMotionDetectParameters)) {
    mMotionDetectParameters = new MotionDetectParameters();
    memset(mMotionDetectParameters, 0, sizeof(*mMotionDetectParameters));
  }

  if (AM_LIKELY(roi  < MAX_MOTION_DETECT_ROI_NUM &&
                param < MD_ROI_PARAM_MAX_NUM)) {
    mMotionDetectParameters->value[roi][param] =
      (uint32_t) get_int(param_name, 0);
    if (mMotionDetectParameters->value[roi][param] < min ||
        mMotionDetectParameters->value[roi][param] > max) {
      WARN("Invalid value for %s[%d~%d], reset to %d\n",
           param_name, min, max, def);
      mMotionDetectParameters->value[roi][param] = def;
      mMotionDetectParameters->config_changed = 1;
    }
  }
}
