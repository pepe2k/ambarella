
/**
 * mdec_if.cpp
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
#include "mdec_if.h"

//-----------------------------------------------------------------------
//
// GUIDs
//
//-----------------------------------------------------------------------

// {267D2C67-1F5E-7777-A145-D7B8BC70CB3A}
AM_DEFINE_IID(IID_IMDECEngine,
0x267d2c67, 0x1f5e, 0x7777, 0xa1, 0x45, 0xd7, 0xb8, 0xbc, 0x70, 0xcb, 0x3a);

// {D9CF299B-6031-4AD9-932E-099B62EC1207}
AM_DEFINE_IID(IID_IMDecControl,
0xd9cf299b, 0x6031, 0x4ad9, 0x93, 0x2e, 0x9, 0x9b, 0x62, 0xec, 0x12, 0x7);

