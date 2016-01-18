/*******************************************************************************
 * am_define.cpp
 *
 * History:
 *   2013-2-19 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "utilities/am_define.h"
#include <string.h>

char* amstrdup(const char* str)
{
  char* newstr = NULL;
  if (AM_LIKELY(str)) {
    newstr = new char[strlen(str) + 1];
    if (AM_LIKELY(newstr)) {
      memset(newstr, 0, strlen(str) + 1);
      strcpy(newstr, str);
    }
  }

  return newstr;
}
