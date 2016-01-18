/**
 * defines.h
 *
 * History:
 *    2011/03/25 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __DEFINES_H__
#define __DEFINES_H__


/************************* Macro definition ****************************/
#include "utils.h"


/*************************** Structure definitions ***************************/
#define SERVER_CONFIG_PATH		"/etc/ambaipcam/mediaserver"

#define ENCODE_SERVER_PORT			(20000)
#define IMAGE_SERVER_PORT			(20002)
#define BUFFER_SIZE					(1024)

#define	FILE_CONTENT_SIZE		(10 * 1024)


typedef enum {
	HIGH_FRAMERATE = 0,
	LOW_DELAY,
	HIGH_MP,
	TOTAL_ENCODE_MODE,
} ENCODE_MODE;


typedef enum {
	REQ_SET_FORCEIDR = 17,
	REQ_GET_VIDEO_PORT,

	REQ_STREAM_START,
	REQ_STREAM_STOP,

	REQ_CHANGE_BR,
	REQ_CHANGE_FR,
	REQ_CHANGE_BRC,

	REQ_GET_PARAM = 100,
	REQ_SET_PARAM,
	REQ_AAA_CONTROL,
} REQUSET_ID;


typedef enum {
	AAA_START = 0,
	AAA_STOP,
} AAA_CONTROL_ID;


typedef struct {
	u32		id;
	u32		info;
	u32		dataSize;
} Request;


typedef struct {
	u32		result;
	u32		info;
} Ack;


typedef enum {
	DATA_MAGIC = 0x20110725,
	DATA_VERSION = 0x00000012,
} VERSION_MAGIC;


typedef struct {
	u32		magic;
	u32		version;
} VERSION;


typedef int (*get_func)(char * section_name, u32 info);
typedef int (*set_func)(char * section_name);


static inline int get_func_null(char *section_name, u32 info)
{
	return 0;
}

static inline int set_func_null(char *section_name)
{
	return 0;
}


#endif	// __DEFINES_H__

