/*******************************************************************************
 * am_plugin.h
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
#ifndef AM_PLUGIN_H_
#define AM_PLUGIN_H_

class CPlugin
{
  public:
    static CPlugin* Create(const char* plugin);
    void* getSymbol(const char* symbol);
    void Delete();

  protected:
    bool Construct(const char* plugin);
    CPlugin();
    virtual ~CPlugin();

  protected:
    void* mHandle;
};

#endif /* AM_PLUGIN_H_ */
