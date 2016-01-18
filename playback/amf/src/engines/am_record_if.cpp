
/**
 * record_if.cpp
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
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
#include "record_if.h"
#include "am_record_if.h"

//-----------------------------------------------------------------------
//
// GUIDs
//
//-----------------------------------------------------------------------


// {2B93ED64-E112-4A19-B659-426DD368D2E4}
AM_DEFINE_IID(IID_IAndroidRecordControl, 
0x2B93ED64, 0xE112, 0x4A19, 0xB6, 0x59, 0x42, 0x6D, 0xD3, 0x68, 0xD2, 0xE4);

// {BD1572A3-DB65-4F35-9D47-684F66EE60E9}
AM_DEFINE_IID(IID_IAndroidMuxer, 
0xBD1572A3, 0xDB65, 0x4F35, 0x9D, 0x47, 0x68, 0x4F, 0x66, 0xEE, 0x60, 0xE9);

// {BE136789-3F21-5D23-89A2-9286CD3548A1}
AM_DEFINE_IID(IID_IMp4Muxer,
0xBE136789, 0x3F21, 0x5D23, 0x89, 0xA2, 0x92, 0x86, 0xCD, 0x35, 0x48, 0xA1);

// {BB3A7072-2B26-4DCD-BBDA-5CF18216737C}
AM_DEFINE_IID(IID_IAndroidAudioEncoder,
0xBB3A7072, 0x2B26, 0x4DCD, 0xBB, 0xDA, 0x5C, 0xF1, 0x82, 0x16, 0x73, 0x7C);

AM_DEFINE_IID(IID_IAmbaAuthorControl,
0x7E474423, 0x41D3, 0x4738, 0x94, 0x22, 0x27, 0x63, 0x10, 0xC6, 0x8C, 0x5A);



