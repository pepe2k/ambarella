
/**
 * pbif.cpp
 *
 * History:
 *    2009/12/22 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
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
#include "pbif.h"

//-----------------------------------------------------------------------
//
// GUIDs
//
//-----------------------------------------------------------------------

// {9AD00F03-DF53-487b-BCE8-AF4BD402B423}
AM_DEFINE_IID(IID_IPBEngine, 
0x9ad00f03, 0xdf53, 0x487b, 0xbc, 0xe8, 0xaf, 0x4b, 0xd4, 0x2, 0xb4, 0x23);


// {F4FC65EA-019B-4fa9-8CDD-5E1BB94F91E9}
AM_DEFINE_IID(IID_IPBControl, 
0xf4fc65ea, 0x19b, 0x4fa9, 0x8c, 0xdd, 0x5e, 0x1b, 0xb9, 0x4f, 0x91, 0xe9);




