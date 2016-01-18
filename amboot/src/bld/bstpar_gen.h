/**
 * @file system/src/bld/bstpar_gen.h
 *
 * History:
 *    2009/03/04 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#ifndef __BSTPAR_GEN_H__
#define __BSTPAR_GEN_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Used by the bstpar_gen parser and runtime.
 */
typedef struct bstpar_list_s {
	char *name;
	char *value;
	struct bstpar_list_s *next;
} bstpar_list_t;

/**
 * Used by the bstpar_gen parser and runtime.
 */
typedef struct bstpar_parsed_s {
	char *title;
	char *open_bst;
	char *close_bst;
	char *open_param;
	char *close_param;
	char *open_value;
	char *close_value;
	struct bstpar_list_s *list;
} bstpar_parsed_t;

#ifdef __cplusplus
}
#endif

#endif
