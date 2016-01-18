/**
 * am_new.cpp
 *
 * History:
 *    2008/2/28 - [Oliver Li] created file
 *
 * Copyright (C) 2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_new.h"
#ifdef OVERLOAD_NEW_DELETE
#include <stdlib.h>

void *operator new(size_t size) throw ()
{
  return malloc(size);
}

void operator delete(void *p) throw ()
{
  if (p) {
    free(p);
  }
}

void *operator new[](size_t size) throw ()
{
  return malloc(size);
}

void operator delete[](void *p) throw ()
{
  if (p) {
    free(p);
  }
}

extern "C" void __cxa_pure_virtual(void)
{
}
#endif
