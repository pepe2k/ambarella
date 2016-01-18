/*
 * amba_debug.h
 *
 * History:
 *    2010/05/28 - [Dianrong Du] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

// this file is moving from amba_debug.h. we can re-use this file under ambarella maybe.

#ifndef __AM_DEBUG_H
#define __AM_DEBUG_H

#include <basetypes.h>
#include <amba_debug.h>
int AM_open_hw_timer();
AM_U32 AM_get_hw_timer_count();
AM_U32 AM_hwtimer2us(AM_U32 diff);

#endif	//AM_DEBUG_IOC_MAGIC

