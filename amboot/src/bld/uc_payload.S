/**
 * system/src/bld/uc_payload.S
 *
 * History:
 *    2005/08/26 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <fio/firmfl.h>

/************************************/
/* Perform a bunch of sanity checks */
/************************************/

#if (CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A2Q) ||	\
    (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A5S) || \
    (CHIP_REV == A5L) || (CHIP_REV == A7L)

#ifndef CODE_BIN
#error "CODE_BIN undefined!"
#endif
#ifndef CODE_LEN
#error "CODE_LEN undefined!"
#endif
#ifndef CODE_CRC32
#error "CODE_CRC32 undefined!"
#endif
#ifndef MEMD_BIN
#error "MEMD_BIN undefined!"
#endif
#ifndef MEMD_LEN
#error "MEMD_LEN undefined!"
#endif
#ifndef MEMD_CRC32
#error "MEMD_CRC32 undefined!"
#endif
#ifndef DATA_BIN
#error "DATA_BIN undefined!"
#endif
#ifndef DATA_LEN
#error "DATA_LEN undefined!"
#endif
#ifndef DATA_CRC32
#error "DATA_CRC32 undefined!"
#endif

#undef CODE_OFFSET
#undef MEMD_OFFSET

#define CODE_OFFSET	DSPFW_HEADER_BYTES

#if ((CODE_LEN % 8) == 0)
#define MEMD_OFFSET	CODE_OFFSET + CODE_LEN
#else
#define MEMD_OFFSET	(CODE_OFFSET + CODE_LEN + 8 - (CODE_LEN % 8))
#endif

#if ((MEMD_LEN % 8) == 0)
#define DATA_OFFSET	MEMD_OFFSET + MEMD_LEN
#else
#define DATA_OFFSET	(MEMD_OFFSET + MEMD_LEN + 8 - (MEMD_LEN % 8))
#endif

#else

#ifndef MAIN_BIN
#error "MAIN_BIN undefined!"
#endif
#ifndef MAIN_LEN
#error "MAIN_LEN undefined!"
#endif
#ifndef MAIN_CRC32
#error "MAIN_CRC32 undefined!"
#endif
#ifndef SUB0_BIN
#error "SUB0_BIN undefined!"
#endif
#ifndef SUB0_LEN
#error "SUB0_LEN undefined!"
#endif
#ifndef SUB0_CRC32
#error "SUB0_CRC32 undefined!"
#endif
#ifndef SUB1_BIN
#error "SUB1_BIN undefined!"
#endif
#ifndef SUB1_LEN
#error "SUB1_LEN undefined!"
#endif
#ifndef SUB1_CRC32
#error "SUB1_CRC32 undefined!"
#endif
#ifndef DATA_BIN
#error "DATA_BIN undefined!"
#endif
#ifndef DATA_LEN
#error "DATA_LEN undefined!"
#endif
#ifndef DATA_CRC32
#error "DATA_CRC32 undefined!"
#endif
#ifndef AORC_BIN
#error "AORC_BIN undefined!"
#endif
#ifndef AORC_LEN
#error "AORC_LEN undefined!"
#endif
#ifndef AORC_CRC32
#error "AORC_CRC32 undefined!"
#endif

#define MAIN_OFFSET	DSPFW_HEADER_BYTES

#if ((MAIN_LEN % 8) == 0)
#define SUB0_OFFSET	MAIN_OFFSET + MAIN_LEN
#else
#define SUB0_OFFSET	(MAIN_OFFSET + MAIN_LEN + 8 - (MAIN_LEN % 8))
#endif

#if ((SUB0_LEN % 8) == 0)
#define SUB1_OFFSET	SUB0_OFFSET + SUB0_LEN
#else
#define SUB1_OFFSET	(SUB0_OFFSET + SUB0_LEN + 8 - (SUB0_LEN % 8))
#endif

#if ((SUB1_LEN % 8) == 0)
#define DATA_OFFSET	SUB1_OFFSET + SUB1_LEN
#else
#define DATA_OFFSET	(SUB1_OFFSET + SUB1_LEN + 8 - (SUB1_LEN % 8))
#endif

#if ((DATA_LEN % 8) == 0)
#define AORC_OFFSET	DATA_OFFSET + DATA_LEN
#else
#define AORC_OFFSET	(DATA_OFFSET + DATA_LEN + 8 - (DATA_LEN % 8))
#endif

#endif

.text

/*******************/
/* Start of header */
/*******************/

#if (CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A2Q) ||	\
    (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A5S) || \
    (CHIP_REV == A5L) || (CHIP_REV == A7L)

code_offset:	.word	CODE_OFFSET	/* dspfw_header_t->code.offset */
code_len:	.word	CODE_LEN	/* dspfw_header_t->code.len */
code_crc32:	.word	CODE_CRC32	/* dspfw_header_t->code.crc32 */
code_rsv:	.word	0xffffffff	/* dspfw_header_t->code.rsv */

memd_offset:	.word	MEMD_OFFSET	/* dspfw_header_t->memd.offset */
memd_len:	.word	MEMD_LEN	/* dspfw_header_t->memd.len */
memd_crc32:	.word	MEMD_CRC32	/* dspfw_header_t->memd.crc32 */
memd_rsv:	.word	0xffffffff	/* dspfw_header_t->memd.rsv */

data_offset:	.word	DATA_OFFSET	/* dspfw_header_t->data.offset */
data_len:	.word	DATA_LEN	/* dspfw_header_t->data.len */
data_crc32:	.word	DATA_CRC32	/* dspfw_header_t->data.crc32 */
data_rsv:	.word	0xffffffff	/* dspfw_header_t->data.rsv */

.rept	8
.word	0xffffffff			/* dspfw_header_t->rsv */
.endr

#else

main_offset:	.word	MAIN_OFFSET	/* dspfw_header_t->main.offset */
main_len:	.word	MAIN_LEN	/* dspfw_header_t->main.len */
main_crc32:	.word	MAIN_CRC32	/* dspfw_header_t->main.crc32 */
main_rsv:	.word	0xffffffff	/* dspfw_header_t->main.rsv */

sub0_offset:	.word	SUB0_OFFSET	/* dspfw_header_t->sub0.offset */
sub0_len:	.word	SUB0_LEN	/* dspfw_header_t->sub0.len */
sub0_crc32:	.word	SUB0_CRC32	/* dspfw_header_t->sub0.crc32 */
sub0_rsv:	.word	0xffffffff	/* dspfw_header_t->sub0.rsv */

sub1_offset:	.word	SUB1_OFFSET	/* dspfw_header_t->sub1.offset */
sub1_len:	.word	SUB1_LEN	/* dspfw_header_t->sub1.len */
sub1_crc32:	.word	SUB1_CRC32	/* dspfw_header_t->sub1.crc32 */
sub1_rsv:	.word	0xffffffff	/* dspfw_header_t->sub1.rsv */

data_offset:	.word	DATA_OFFSET	/* dspfw_header_t->data.offset */
data_len:	.word	DATA_LEN	/* dspfw_header_t->data.len */
data_crc32:	.word	DATA_CRC32	/* dspfw_header_t->data.crc32 */
data_rsv:	.word	0xffffffff	/* dspfw_header_t->data.rsv */

#if (HAS_AORC_UCODE == 1)
aorc_offset:	.word	AORC_OFFSET	/* dspfw_header_t->aorc.offset */
aorc_len:	.word	AORC_LEN	/* dspfw_header_t->aorc.len */
aorc_crc32:	.word	AORC_CRC32	/* dspfw_header_t->aorc.crc32 */
aorc_rsv:	.word	0xffffffff	/* dspfw_header_t->aorc.rsv */
#else
.rept	4
.word	0xffffffff			/* dspfw_header_t->rsv */
.endr
#endif /* HAS_AORC_UCODE == 1 */

#endif

/*****************/
/* End of header */
/*****************/

#if (CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A2Q) ||	\
    (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A5S) ||	\
    (CHIP_REV == A5L) || (CHIP_REV == A7L)

.align 3
code:	.incbin	xstr(CODE_BIN)		/* CODE binary payload */
.align 3
memd:	.incbin xstr(MEMD_BIN)		/* MEMD binary payload */
.align 3
data:	.incbin xstr(DATA_BIN)		/* DATA binary payload */

#elif (CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == S2)

.align 3
main:	.incbin xstr(MAIN_BIN)		/* MAIN binary payload */
.align 3
sub0:	.incbin xstr(SUB0_BIN)		/* SUB0 binary payload */
.align 3
sub1:	.incbin xstr(SUB1_BIN)		/* SUB1 binary payload */
.align 3
data:	.incbin xstr(DATA_BIN)		/* DATA binary payload */

#else

.align 3
main:	.incbin xstr(MAIN_BIN)		/* MAIN binary payload */
.align 3
sub0:	.incbin xstr(SUB0_BIN)		/* SUB0 binary payload */
.align 3
sub1:	.incbin xstr(SUB1_BIN)		/* SUB1 binary payload */
.align 3
data:	.incbin xstr(DATA_BIN)		/* DATA binary payload */
.align 3
aorc:	.incbin xstr(AORC_BIN)		/* AORC binary payload */

#endif

.align 3
.end
