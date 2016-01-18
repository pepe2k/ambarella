/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __PGM_H__
#define __PGM_H__

typedef enum {
	PGM_FORMAT_P2	= 0,
	PGM_FORMAT_P5	= 1,
} pgm_format_t;

typedef struct {
	pgm_format_t	format;
	unsigned int	width;
	unsigned int	height;
	unsigned int	max_value;
} pgm_header_t;

int load_pgm(const char *file, pgm_header_t *header, char **data);
int save_pgm(const char *file, const pgm_header_t *header, char *data);

#endif
