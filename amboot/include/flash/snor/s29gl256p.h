/**
 * @file system/include/flash/snor/s29gl256p.h
 *
 * History:
 *    2009/08/17 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __S29GL256P_H__
#define __S29GL256P_H__

#define SNOR_CONTROL1		(SNOR_CTR1_IE 			| \
				 SNOR_CTR1_LATEN_OFFSET(0)	| \
				 SNOR_CTR1_AVD_TYPE0		| \
				 SNOR_CTR1_TYPE_DEMUXED_NOR	| \
				 SNOR_CTR1_SNOR_AVD_DIS		| \
				 SNOR_CTR1_NO_BUS_SHARE		| \
				 SNOR_CTR1_LEN_VALID_ADDR(25))

#define SNOR_CONTROL2			0x0

#define SNOR_MANID			0x1
#define SNOR_DEVID			0x7e
#define SNOR_ID2			0x22
#define SNOR_ID3			0x01

#define SNOR_ID_LEN			4
#define	SNOR_DEV_ID_LEN			3

#define SNOR_NAME			"SPANSION_S29GL256P_32MB"
#define SNOR_CHIP_SIZE			(32 << 20)
#define SNOR_BANKS_PER_DEVICE		1

/* Block1 is at lower address; block2 is at higher address. */
#define SNOR_DEVICE_TYPE	SNOR_TYPE_UNIFORM_BLOCK

#define SNOR_BLOCK1_SIZE		(128 << 10)
#define SNOR_BLOCK1S_PER_BANK		256
#define SNOR_BLOCK2_SIZE		0
#define SNOR_BLOCK2S_PER_BANK		0
#define SNOR_BLOCKS_PER_BANK	(SNOR_BLOCK1S_PER_BANK + SNOR_BLOCK2S_PER_BANK)
#define SNOR_BANKS_PER_DEVICE		1
#define SNOR_TOTAL_BLOCKS	(SNOR_BLOCKS_PER_BANK * SNOR_BANKS_PER_DEVICE)

#define SNOR_POLL_MODE		SNOR_POLL_MODE_RDY_BUSY

#if defined(CONFIG_SNOR_1DEVICE)
#define SNOR_DEVICES		1
#elif defined(CONFIG_SNOR_2DEVICE)
#define SNOR_DEVICES		2
#elif defined(CONFIG_SNOR_4DEVICE)
#define SNOR_DEVICES		4
#endif

#define SNOR_TOTAL_BANKS	(SNOR_DEVICES * SNOR_BANKS_PER_DEVICE)

/* TIM0	*/
#define	SNOR_TAA		130	/* tacc */
#define	SNOR_TOE		30
#define	SNOR_TOEH		10
#define	SNOR_TCE		130
/* TIM1	*/
#define	SNOR_TPA		0	/* don't care */
#define	SNOR_TRP		35000
#define	SNOR_TRH		200
#define	SNOR_TOES		0	/* don't care */
/* TIM2	*/
#define	SNOR_TCS		0
#define	SNOR_TCH		0
#define	SNOR_TWP		35
#define	SNOR_TWH		0
/* TIM3	*/
#define	SNOR_TRB		0
#define	SNOR_TAHT		0
#define	SNOR_TASO		15
#define	SNOR_TOEPH		20
/* TIM4	*/
#define	SNOR_TAVDCS		0	/* don't care */
#define	SNOR_TAVDCH		0	/* don't care */
#define	SNOR_TAS		0
#define	SNOR_TAH		45
/* TIM5	*/
#define	SNOR_TELF		0	/* don't care */
#define	SNOR_TDS		45
#define	SNOR_TDH		0
#define	SNOR_TBUSY		90
/* TIM6	*/
#define	SNOR_TDP		0	/* don't care */
#define	SNOR_TCEPH		20
#define	SNOR_TOEZ		0	/* don't care (synchronous mode) */
#define	SNOR_TBA		0	/* don't care (burst mode) */
/* TIM7	*/
#define	SNOR_TREADY		35000
#define	SNOR_TACH		0	/* don't care */
#define	SNOR_TCEHP		0	/* don't care */
/* TIM8	*/
#define	SNOR_TAVDS		0	/* don't care */
#define	SNOR_TOH		0
#define	SNOR_TAVDP		0	/* don't care */
#define	SNOR_TAAVDH		0	/* don't care */
/* TIM9	*/
#define	SNOR_TIAA		0	/* don't care */
#define	SNOR_TCLK_PERIOD	0	/* don't care */
#define	SNOR_TWEA		0	/* don't care */
#define	SNOR_TACS		0	/* don't care */

#endif
