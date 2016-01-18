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
#include "mw_struct_hiso.h"

#define AMP_HISO_LIB_MAJOR 4
#define AMP_HISO_LIB_MINOR 0
#define AMP_HISO_LIB_PATCH 0
#define AMP_HISO_LIB_VERSION ((AMP_HISO_LIB_MAJOR << 16) | \
                             (AMP_HISO_LIB_MINOR << 8)  | \
                             AMP_HISO_LIB_PATCH)

mw_version_info mw_version =
{
	.major		= AMP_HISO_LIB_MAJOR,
	.minor		= AMP_HISO_LIB_MINOR,
	.patch		= AMP_HISO_LIB_PATCH,
	.update_time	= 0x20140919,
};
