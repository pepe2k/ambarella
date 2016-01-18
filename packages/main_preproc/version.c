/*******************************************************************************
 * version.c
 *
 * Histroy:
 *  2014/07/14 2014 - [Lei Hong] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <basetypes.h>
#include "iav_encode_drv.h"
#include "lib_vproc.h"

#define MAINPP_LIB_MAJOR 1
#define MAINPP_LIB_MINOR 0
#define MAINPP_LIB_PATCH 0
#define MAINPP_LIB_VERSION ((MAINPP_LIB_MAJOR << 16) | \
                             (MAINPP_LIB_MINOR << 8)  | \
                             MAINPP_LIB_PATCH)

version_t G_version = {
	.major = MAINPP_LIB_MAJOR,
	.minor = MAINPP_LIB_MINOR,
	.patch = MAINPP_LIB_PATCH,
	.mod_time = 0x20140328,
	.description = "Ambarella S2 Video Process Library",
};
