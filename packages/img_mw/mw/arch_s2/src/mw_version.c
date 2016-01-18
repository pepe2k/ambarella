/*******************************************************************************
 * mw_version.c
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
#include "mw_struct.h"

#define AMP_LIB_MAJOR 3
#define AMP_LIB_MINOR 2
#define AMP_LIB_PATCH 3
#define AMP_LIB_VERSION ((AMP_LIB_MAJOR << 16) | \
                             (AMP_LIB_MINOR << 8)  | \
                             AMP_LIB_PATCH)

mw_version_info mw_version =
{
	.major		= AMP_LIB_MAJOR,
	.minor		= AMP_LIB_MINOR,
	.patch		= AMP_LIB_PATCH,
	.update_time	= 0x20140409,
};
