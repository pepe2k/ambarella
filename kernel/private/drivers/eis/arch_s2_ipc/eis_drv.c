/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_drv.c
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#include <amba_common.h>
#include <iav_common.h>
#include <iav_encode_drv.h>
#include <eis_drv.h>
#include "eis_priv.h"

#ifdef CONFIG_AMBARELLA_EIS_IPC_CORTEX
#include "eis_cortex.c"
#else
#include "eis_arm11.c"
#endif
