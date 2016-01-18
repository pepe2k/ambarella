/*******************************************************************************
 * testamlog.h
 *
 * Histroy:
 *  2012-2-28 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef TESTAMLOG_H_
#define TESTAMLOG_H_

#include <unistd.h>
class TestAmlogA
{
  public:
    TestAmlogA(){}
    virtual ~TestAmlogA()
    {
      DEBUG("~TestAmlogA deleted");
    }

  public:
    void a();
    void b();
    void c();
    void d();

    static void *thread_entry(void *data)
    {
      for (int i = 0; i < 50; ++ i) {
        ((TestAmlogA *)data)->a();
        ((TestAmlogA *)data)->b();
        ((TestAmlogA *)data)->c();
        ((TestAmlogA *)data)->d();
      }
      return 0;
    }
};

class TestAmlogB
{
  public:
    TestAmlogB(){}
    virtual ~TestAmlogB()
    {
      DEBUG("~TestAmlogB deleted");
    }

  public:
    void a();
    void b();
    void c();
    void d();

    static void *thread_entry(void *data)
    {
      for (int i = 0; i < 50; ++ i) {
        ((TestAmlogB *)data)->a();
        ((TestAmlogB *)data)->b();
        ((TestAmlogB *)data)->c();
        ((TestAmlogB *)data)->d();
      }
      return 0;
    }
};

#endif /* TESTAMLOG_H_ */
