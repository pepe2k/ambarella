/**
 * utils.h
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

#ifndef __APP_UTILS_H__
#define __APP_UTILS_H__


/************************* Macro definition ****************************/
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

extern int G_log_level;

typedef enum {
	ERROR_LEVEL = 0,
	MESSAGE_LEVEL,
	INFO_LEVEL,
} DEBUG_LEVEL;


#define PRINT(mylog, LOG_LEVEL, format, args...)		\
		do {											\
			if (mylog >= LOG_LEVEL) {				\
				printf(format, ##args);				\
			}										\
		} while (0)

#define APP_ERROR(format, args...)	PRINT(G_log_level, ERROR_LEVEL, "!!! [%s: %d] " format "\n", __FILE__, __LINE__, ##args)
#define APP_PRINTF(format, args...)	PRINT(G_log_level, MESSAGE_LEVEL, ">>> " format, ##args)
#define APP_INFO(format, args...)		PRINT(G_log_level, INFO_LEVEL, "::: " format, ##args)
#define APP_ASSERT(expr)                 assert(expr)


#ifdef APP_LOG_ENABLE
#define APP_LOG(format, args...)         printf("***** " format, ##args)
#else
#define APP_LOG(format, args...)
#endif

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, n)	((x) & ~((n)-1u))
#endif

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif


#define NO_ARG		0
#define HAS_ARG		1

typedef struct hint_s {
	const char *arg;
	const char *str;
} hint_t;


#define	ENCODE_SERVER_PROC	"encode_server"
#define	IMAGE_SERVER_PROC		"image_server"
#define	MEDIA_SERVER_PROC		"media_server"
#define	RTSP_SERVER_PROC		"rtsp_server"
#define	TCP_SERVER_PROC		"tcp_stream_server"


static int create_pid_file(char *proc)
{
	#define STRING_LENGTH	(32)
	char buf[STRING_LENGTH], pid_file[STRING_LENGTH];
	int retv, fd;
	u32 old_pid, new_pid, remove_old_proc;

	new_pid = getpid();
	old_pid = remove_old_proc = 0;
	sprintf(pid_file, "/tmp/%s.pid", proc);
	if ((fd = open(pid_file, O_RDONLY, 0777)) >= 0) {
		read(fd, (void*)buf, STRING_LENGTH);
		if (strlen(buf) > 0) {
			old_pid = atoi(buf);
			if ((old_pid != 0) && (old_pid != new_pid)) {
				remove_old_proc = 1;
			}
		}
		if (remove_old_proc) {
			sprintf(buf, "ps x | grep %d | grep -v grep", old_pid);
			retv = system(buf);
			if (retv == 0) {
				sprintf(buf, "kill -9 %d", old_pid);
				system(buf);
			}
		}
		close(fd);
		APP_PRINTF("[%s] is already running ! Re-start it again !\n", proc);
	}

	if ((fd = open(pid_file, O_CREAT | O_RDWR, 0644)) < 0) {
		APP_ERROR("CANNOT create [%s] pid file !\n", proc);
		return -1;
	}
	sprintf(buf, "%d\n", new_pid);
	if ((retv = write(fd, (void *)buf, strlen(buf))) < 0) {
		APP_ERROR("write length %d.\n", retv);
		return -1;
	}
	close(fd);

	return 0;
}

static int delete_pid_file(char *proc)
{
	char buf[32];
	int fd;

	sprintf(buf, "/tmp/%s.pid", proc);
	if ((fd = open(buf, O_RDONLY, 0777)) < 0) {
		return -1;
	}
	close(fd);
	sprintf(buf, "rm -rf /tmp/%s.pid", proc);
	system(buf);

	return 0;
}


#endif	// __APP_UTILS_H__

