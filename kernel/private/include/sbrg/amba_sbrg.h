/*
 * kernel/private/include/sbrg/amba_sbrg.h
 *
 * History:
 *    2011/06/17 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_SBRG_H
#define __AMBA_SBRG_H

//#include <sbrg/amba_sbrg_dev_s2_drv.h>
#include <sbrg/amba_sbrg_dev_s3d_drv.h>

/* ========================================================================== */
int amba_sbrg_adapter_cmd(int adapid, int cmd, void *args);

#endif

