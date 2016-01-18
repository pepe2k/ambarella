/*
 * nand_update.h
 *
 * History:
 *    2012/11/14 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef  __NAND_UPDATE_H__
#define  __NAND_UPDATE_H__

#define NAND_UPGRADE_FILE_TYPE_UNKNOWN  0 /* unknown */
#define NAND_UPGRADE_FILE_TYPE_BST      1 /* bst */
#define NAND_UPGRADE_FILE_TYPE_BLD      2 /* Boot loader */
#define NAND_UPGRADE_FILE_TYPE_HAL      3 /* HAL */
#define NAND_UPGRADE_FILE_TYPE_PRI      4 /* Linux kernel (primary)*/
#define NAND_UPGRADE_FILE_TYPE_LNX      5 /* Linux file system */
#define NAND_UPGRADE_FILE_TYPE_USR      6 /* user data */
#define NAND_UPGRADE_FILE_TYPE_PBA      7 /* pba for upgrade */
#define NAND_UPGRADE_FILE_TYPE_RESERVE1 8 /* reserved1 data */
#define NAND_UPGRADE_FILE_TYPE_RESERVE2 9 /* reserved2 data */


typedef struct nand_update_file_header_s {
    unsigned char	magic_number[8];    /*   AMBUPGD'\0'     8 chars including '\0' */
    unsigned short	header_ver_major;
    unsigned short	header_ver_minor;
    unsigned int	header_size;       /* payload starts at header_start_address + header_size */
    unsigned int  payload_type;      /* NAND_UPGRADE_FILE_TYPE_xxx */
    unsigned char	payload_description[256]; /* payload description string, end with '\0' */
    unsigned int  payload_size;      /* payload of upgrade file, after header */
    unsigned int  payload_crc32;     /* payload crc32 checksum, crc calculation from
                                        header_start_address  + header_size,
                                        crc calculation size is payload_size */
}nand_update_file_header_t;

#endif
