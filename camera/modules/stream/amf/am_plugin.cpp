/*******************************************************************************
 * am_plugin.cpp
 *
 * History:
 *   2014年1月6日 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <dlfcn.h>
#include "am_types.h"
#include "am_plugin.h"

CPlugin* CPlugin::Create(const char* plugin)
{
  CPlugin* result = new CPlugin();
  if (result && !result->Construct(plugin)) {
    delete result;
    result = NULL;
  }

  return result;
}

void* CPlugin::getSymbol(const char* symbol)
{
  void* func = NULL;
  if (AM_LIKELY(symbol && mHandle)) {
    char* err = NULL;
    dlerror(); /* clear any existing errors */
    func = dlsym(mHandle, symbol);
    if (AM_LIKELY((err = dlerror()) != NULL)) {
      ERROR("Failed to get symbol %s: %s", symbol, err);
      func = NULL;
    }
  } else {
    if (AM_LIKELY(!mHandle)) {
      ERROR("Library is not loaded!");
    }
    if (AM_LIKELY(!symbol)) {
      ERROR("Invalid symbol!");
    }
  }

  return func;
}

void CPlugin::Delete()
{
  delete this;
}

bool CPlugin::Construct(const char* plugin)
{
  bool ret = true;
  if (AM_LIKELY(plugin)) {
    dlerror();/* Clear any existing error */
    mHandle = dlopen(plugin, RTLD_LAZY);
    if (AM_UNLIKELY(!mHandle)) {
      ERROR("Failed to open %s: %s", plugin, dlerror());
      ret = false;
    }
  } else {
    ERROR("Invalid library path!");
    ret = false;
  }

  return ret;
}

CPlugin::CPlugin() :
    mHandle(NULL)
{

}

CPlugin::~CPlugin()
{
  if (AM_LIKELY(mHandle)) {
    dlclose(mHandle);
  }
}
