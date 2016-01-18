/*******************************************************************************
 * dewarp_version.c
 *
 * History:
 *  Oct 15, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "lib_dewarp.h"
#include "dewarp_version.h"
/*
 * 3.0.1 2013/10/10:
 *   Fix a logic error in adjust_input_rect_inside_main, which may get an input
 *   area outside the pre main buffer.
 *
 * 3.1.0 2013/10/15:
 *   1. Create arch.
 *   2. Add APIs: point_mapping_fisheye_to_xxx_xxx
 *   3. Distinguish desktop and ceiling mount by the sign of tilt angle
 *      in fisheye_to_ceilnor, fisheye_to_ceilpanor.
 *
 * 3.1.1 2013/10/16
 *   1. Create header file for no glibc.
 *   2. Remove log codes.
 *
 * 3.1.2 2013/11/13 (DON'T USE THIS)
 *   Fix a bug in configuring output ROI in wall subregion, which will cause
 *   wrong point mapping.
 *
 * 3.1.2 2013/11/14
 *   1. Fix the bug in point_mapping_xxx_to_xxx which may result in wrong
 *   mapped points in wall subregion mode.
 *   2. Fix the bug in adjust_input_rect_inside_main, which may get an input
 *   area outside the pre main buffer when rotation is applied.
 *
 * 3.2.0 2013/11/18
 *   1. Use float in ROI transformation and point mapping APIs.
 *   2. Fix the bug that ROI is not the center of dewarp region in wall
 *   subregion mode.
 *
 * 3.2.1 2013/11/26
 *   Add pan/tilt angle in point_mapping APIs.
 *
 * 3.2.2 2014/02/28
  *  Change grid division strategy.  Auto-align the output size if it is not
  *  grid spacing aligned when using one warp area.
 */
#define DEWARP_LIB_MAJOR	3
#define DEWARP_LIB_MINOR	2
#define DEWARP_LIB_PATCH	2

version_t dewarp_version =
{
	.major = DEWARP_LIB_MAJOR,
	.minor = DEWARP_LIB_MINOR,
	.patch = DEWARP_LIB_PATCH,
	.mod_time = 0x20140228,
	.description = "Ambarella S2 Dewarp Library",
};
