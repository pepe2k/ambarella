/**
 * am_new.h
 *
 * History:
 *	2008/2/28 - [Oliver Li] created file
 *	2009/12/2 - [Oliver Li] rewrite
 *
 * Copyright (C) 2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_NEW_H__
#define __AM_NEW_H__

#ifndef OVERLOAD_NEW_DELETE
#include <new>
#else
void *operator new(unsigned int size) throw ();
void operator delete(void*) throw ();

void *operator new[](unsigned int size) throw ();
void operator delete[](void*) throw ();
#endif

#define AM_DELETE(_obj) \
  do { if (_obj) (_obj)->Delete(); _obj = NULL; } while (0)

#define AM_RELEASE(_obj) \
  do { if (_obj) (_obj)->Release(); } while (0)

#define AM_SIMPLE_DELETE(_obj) \
  do { if (_obj) delete _obj; _obj = NULL; } while (0)

#define AM_SIMPLE_DELETE_ARRAY(_obj) \
  do { if (_obj) delete [] _obj; _obj = NULL; } while (0)

#endif

