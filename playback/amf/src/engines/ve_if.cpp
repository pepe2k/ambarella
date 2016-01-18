
/**
 * ve_if.cpp
 *
 * History:
 *    2011/08/08 - [GangLiu] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 
#include <stdio.h>
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_base.h"
#include "ve_if.h"

//-----------------------------------------------------------------------
//
// GUIDs
//
//-----------------------------------------------------------------------

// {4EF0E38B-EA12-93C2-4d9e-B51AE9C256A1}
AM_DEFINE_IID(IID_IVEEngine,
0x4ef0e38b, 0xea12, 0x93c2, 0x4d, 0x9e, 0xb5, 0x1a, 0xe9, 0xc2, 0x56, 0xa1);

// {E61F0445-477a-29E3-B335-97EBFD59E468}
AM_DEFINE_IID(IID_IVEControl,
0xe61f0445, 0x477a, 0x29e3, 0xb3, 0x35, 0x97, 0xeb, 0xfd, 0x59, 0xe4, 0x68);

// {47462173-9FD5-4EB9-A302-1D606B14CFE3}
AM_DEFINE_IID(IID_IVideoEffecterControl,
0x47462173, 0x9fd5, 0x4eb9, 0xa3, 0x02, 0x1d, 0x60, 0x6b, 0x14, 0xcf, 0xe3);

// {467D2C6C-1F5E-4044-A190-D7B8BC20CB3A}
AM_DEFINE_IID(IID_IVideoTranscoder,
0x467d2c6c, 0x1f5e, 0x4044, 0xa1, 0x90, 0xd7, 0xb8, 0xbc, 0x20, 0xcb, 0x3a);

// {0FC6262E-E7DB-4A86-A9F3-D62424620FDE}
AM_DEFINE_IID(IID_IVideoMemEncoder,
0xfc6262e, 0xe7db, 0x4a86, 0xa9, 0xf3, 0xd6, 0x24, 0x24, 0x62, 0xf, 0xde);
