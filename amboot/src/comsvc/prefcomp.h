/**
 * system/src/comsvc/prefcomp.h
 *
 * History:
 *    2006/04/11 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __PREFCOMP_H__
#define __PREFCOMP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Linked-list of include file.
 */
struct pref_inc_s
{
	char *file;	/**< Header file name */
	struct pref_inc_s *next;
};

/**
 * A 'preferences' item.
 */
struct pref_item_s
{
	char *var;	/**< Variable name */
	char *type;	/**< Variable type */
	int array;	/**< Array size (default = 0) */
	char *defval;	/**< Default value */
	int runtime;	/**< A Run-time varible? */
	int factory;	/**< A Factory calibrated variable? */
	struct pref_item_s *next;
};

/**
 * Parsed result of 'preferences' language.
 */
struct pref_data_s
{
	struct pref_inc_s *includes;
	struct pref_item_s *items;
};

#ifdef __cplusplus
}
#endif

#endif

