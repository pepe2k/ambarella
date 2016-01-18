/*******************************************************************************
 * am_config_base.cpp
 *
 * History:
 *  2012-2-20 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
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

/*******************************************************************************
 * AmConfigBase
 ******************************************************************************/

AmConfigBase::AmConfigBase(const char *configFileName)
  : mConfigFile(NULL),
    mDictionary(NULL)
{
  if (configFileName) {
    mConfigFile = amstrdup(configFileName);
  }
}

AmConfigBase::~AmConfigBase()
{
  delete[] mConfigFile;
  if (mDictionary) {
    iniparser_freedict(mDictionary);
  }
  DEBUG("~AmConfigBase");
}

bool AmConfigBase::init()
{
  /* Unload old dictionary every time to keep config file updated */
  if (mDictionary) {
    iniparser_freedict(mDictionary);
    mDictionary = NULL;
  }
  mDictionary = iniparser_load(mConfigFile);
  mDictionary != NULL ? INFO("%s is loaded!", mConfigFile) :
                        ERROR("Failed loading config file: %s", mConfigFile);

  return (mDictionary != NULL);
}

bool AmConfigBase::save_config()
{
  bool ret = false;

  if (mDictionary && mConfigFile) {
    FILE *filePointer = fopen(mConfigFile, "w+");
    if (NULL == filePointer) {
      ERROR("%s\n", strerror(errno));
    } else {
      iniparser_dump_ini(mDictionary, filePointer);
      fclose(filePointer);
      ret = true;
    }
  } else {
    ERROR("Please load config file before save it!");
  }

  return ret;
}

amba_video_mode AmConfigBase::str_to_video_mode(const char* mode)
{
  amba_video_mode videoMode = AMBA_VIDEO_MODE_AUTO;

  if (mode) {
    for (uint32_t i = 0;
        i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
        ++ i) {
      if (is_str_equal(mode, gVideoModeList[i].videoMode)) {
        videoMode = gVideoModeList[i].ambaMode;
        break;
      }
    }
  }

  return videoMode;
}

const char* AmConfigBase::video_mode_to_str(amba_video_mode mode)
{
  char *videoMode = (char *)gVideoModeList[0].videoMode;
  for (uint32_t i = 0;
       i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
       ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      videoMode = (char *)gVideoModeList[i].videoMode;
      break;
    }
  }

  return videoMode;
}

uint32_t AmConfigBase::get_video_mode_w(amba_video_mode mode)
{
  int32_t w = gVideoModeList[0].width;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      w = gVideoModeList[i].width;
      break;
    }
  }

  return w;
}

uint32_t AmConfigBase::get_video_mode_h(amba_video_mode mode)
{
  int32_t h = gVideoModeList[0].height;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      h = gVideoModeList[i].height;
      break;
    }
  }

  return h;
}
