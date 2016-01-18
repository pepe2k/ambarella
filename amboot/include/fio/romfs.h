/**
 * @file system/include/fio/romfs.h
 *
 * History:
 *    2006/04/22 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __ROMFS_H__
#define __ROMFS_H__

#include <basedef.h>

__BEGIN_C_PROTO__

#ifdef __ROMFS_IMPL__

/**
 * Meta data used for ROMFS.
 */

#define ROMFS_META_SIZE		2048

typedef struct romfs_meta_s
{
	unsigned int file_count;
	unsigned int magic;
	char padding[2040];
#define ROMFS_META_MAGIC	0x66fc328a
} romfs_meta_t;

/**
 * Header internal to ROMFS.
 */
typedef struct romfs_header_s
{
	char name[116];
	unsigned int offset;
	unsigned int size;
	unsigned int magic;
#define ROMFS_HEADER_MAGIC	0x2387ab76
} romfs_header_t;

/**
 * Used by the romfs_gen parser and runtime.
 */
typedef struct romfs_list_s
{
	char *file;
	char *alias;
	unsigned int size;
	unsigned int offset;
	unsigned int padding;
	struct romfs_list_s *next;
} romfs_list_t;

/**
 * Used by the romfs_gen parser.
 */
typedef struct romfs_parsed_s
{
	char *top;
	struct romfs_list_s *list;
} romfs_parsed_t;

#endif

/**
 * Initilize the ROMFS.
 * @returns 0 if success < 0 if failure.
 */
extern int romfs_init(void);

/**
 * To see if ROMFS is initialized.
 * @returns 1 if initialized 0 if not.
 */
extern int romfs_is_init(void);

/**
 * Get the number of files that exists on the ROMFS.
 *
 * @returns The number of files; < 0 if the ROMFS is invalid.
 */
extern int romfile_count(void);

/**
 * Get the name of a file in the ROMFS specified by its index.
 *
 * @param idx - The index number.
 * @param name - The string pointer where the result is to be written to.
 * @param len - Length of the allocated string.
 * @returns - The length of the string; < 0 if the file does not exist.
 */
extern int romfile_name(int idx, char *name, unsigned int len);

/**
 * Checks for the existence of file on ROMFS.
 *
 * @param file - The file name.
 * @returns 1 - exists, 0 - not exists.
 */
extern int romfile_exists(const char *file);

/**
 * Get the index number, given a file name.
 *
 * @param file - The file name.
 * @returns The index number or < 0 if the file does not exist.
 */
extern int romfile_name_idx(const char *file);

/**
 * Checks the size of file on ROMFS by name.
 *
 * @param file - The file name.
 * returns The size of the file; < 0 if the file does not exist.
 */
extern int romfile_size_name(const char *file);

/**
 * Checks the size of file on ROMFS by index.
 *
 * @param idx - The index.
 * returns The size of the file; < 0 if the file does not exist.
 */extern int romfile_size_idx(int idx);

/**
 * Load a file on ROMFS to memory by name.
 *
 * @param file - the file name.
 * @param ptr - Pointer to memory where the file data is to be loaded.
 * @param len - The size of allocated memory.
 * @param fpos - file position.
 * @returns The number of bytes loaded; < 0 if the file does not exist.
 */
extern int romfile_name_load(const char *file, u8 *ptr,
			     unsigned int len, unsigned int fpos);

/**
 * Load a file on ROMFS to memory by index.
 *
 * @param idx - The index number.
 * @param ptr - Pointer to memory where the file data is to be loaded.
 * @param len - The size of allocated memory.
 * @param fpos - file position.
 * @returns The number of bytes loaded; < 0 if the file does not exist.
 */
extern int romfile_idx_load(int idx, u8 *ptr,
			    unsigned int len, unsigned int fpos);

/**
 * This structure is used to represent a tree-structure of the ROMFS.
 */
struct rf_inode_s {
	struct rf_inode_s *parent;	/* parent directory */
	struct rf_inode_s *children;	/* child list */
	struct rf_inode_s *sibling;	/* next sibling */
	unsigned int type;
#define	RF_INODE_DIR_TYPE	0
#define RF_INODE_FILE_TYPE	1
	char *name;		/* name */
	int index;		/* index */
};

extern struct rf_inode_s *G_rf_root;

typedef struct romfsf_vol_info_s
{
	char	v_name[12];
	u32	total_blocks;
	u32	free_blocks;
	u32	block_size;
	u32	sectors_per_block;

} romfsf_vol_info_t;

/**
 * Get the volume information of ROMFS.
 *
 * @param slot - romfs device
 * @param volume_info - The volume information pointer where the result is 
 * 			to be written to.
 * @returns 0 if success < 0 if failure.
 */
int romfs_get_volume_info(int slot, romfsf_vol_info_t* volume_info);

__END_C_PROTO__

#endif
