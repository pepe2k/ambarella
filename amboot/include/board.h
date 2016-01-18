/**
 * system/include/board.h
 *
 * History:
 *    2005/03/16 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __TOP_BOARD_H__
#define __TOP_BOARD_H__

/* Include the BSP header */
#include <bsp.h>

#ifndef __ASM__
#ifdef __cplusplus
extern "C" {
#endif
extern void bsp_init(void);
#ifdef __cplusplus
}
#endif
#endif

#endif
