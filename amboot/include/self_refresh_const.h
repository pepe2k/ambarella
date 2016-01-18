/**
 * system/include/self_refresh_const.h
 *
 * Function prototypes for self-refresh modes
 *
 * History:
 *    2010/11/26 - [Timothy Wu] created file
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SELF_REFRESH_CONST_H__
#define __SELF_REFRESH_CONST_H__

#define SR_PASSWORD_SUSPEND 0xbabebabe
#define SR_PASSWORD_RESUME  0xcafecafe

#define ENABLE_DEBUG_MSG_SELF_REFRESH	0

#ifndef SR_DDR3_RESET_GPIO
#define SR_DDR3_RESET_GPIO	38
#endif

/******************************************************************************/

#ifdef __BUILD_AMBOOT__

#if (ENABLE_DEBUG_MSG_SELF_REFRESH > 0)
#define DEBUG_MSG_SR_HEX(x,y) 	do {	putstr(x); \
					puthex(y); \
			  		putstr("\r\n"); \
				} while (0)

#define DEBUG_MSG_SR_DEC(x,y) 	do {	putstr(x); \
					putdec(y); \
			  		putstr("\r\n"); \
				} while (0)


#define DEBUG_MSG_SR_STR(x) 	putstr(x);
#else
#define DEBUG_MSG_SR_HEX(...)
#define DEBUG_MSG_SR_DEC(...)
#define DEBUG_MSG_SR_STR(...)
#endif

#endif /* __BUILD_AMBOOT__ */

#endif
