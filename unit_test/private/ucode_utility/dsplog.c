/*
 * dsplog.c
 *
 * History:
 *	2012/05/05 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include "types.h"
#include "amba_debug.h"
#include "iav_drv.h"

char buffer[32 * 1024];
int maxlogs = 0;

#define	AMBA_DEBUG_DSP	(1 << 1)

int enable_dsplog(void)
{
	int fd;
	int debug_flag;

	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
		return -1;
	}

	debug_flag |= AMBA_DEBUG_DSP;

	if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
		return -1;
	}

	close(fd);
	return 0;
}

int iav_fd;

int open_iav(void)
{
	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	return 0;
}

int setup_log(char **argv)
{
	u8 module = atoi(argv[0]);
	u8 level = atoi(argv[1]);
	u8 thread = atoi(argv[2]);

	if (open_iav() < 0)
		return -1;

	if (ioctl(iav_fd, IAV_IOC_LOG_SETUP, (module | (level << 8) | (thread << 16))) < 0) {
		perror("IAV_IOC_LOG_SETUP");
		return -1;
	}

	return 0;
}

int dsp_setup_debug(iav_dsp_setup_t *dsp_setup)
{
	if (ioctl(iav_fd, IAV_IOC_LOG_SETUP, dsp_setup) < 0) {
		perror("IAV_IOC_LOG_SETUP");
		return -1;
	}
	return 0;
}

// for iOne
int setup_level(char **argv)
{
	iav_dsp_setup_t dsp_setup;

	dsp_setup.cmd = 1;
	dsp_setup.args[0] = atoi(argv[0]);
	dsp_setup.args[1] = atoi(argv[1]);
	dsp_setup.args[2] = atoi(argv[2]);

	if (open_iav() < 0)
		return -1;

	if (dsp_setup_debug(&dsp_setup) < 0)
		return -1;

	return 0;
}

// for iOne
int setup_thread(char **argv)
{
	iav_dsp_setup_t dsp_setup;

	dsp_setup.cmd = 2;
	dsp_setup.args[0] = atoi(argv[0]);

	if (open_iav() < 0)
		return -1;

	if (dsp_setup_debug(&dsp_setup) < 0)
		return -1;

	return 0;
}

static void usage(void)
{
	printf("\ndsplog usage:\n\n");
	printf("For A5s:\n");
	printf("  Enable debug level for all modules:\n");
	printf("    # dsplog d  0  3  0\n\n");
	printf("For iOne / S2:\n");
	printf("  Enable debug level for IDSP / capture issues:\n");
	printf("    # dsplog level  1  1  36    Or\n");
	printf("    # dsplog level  1  1  548\n\n");
	printf("  Enable debug level for VOUT issue:\n");
	printf("    # dsplog level  2  1  1\n\n");
	printf("  Enable debug level for encode issue:\n");
	printf("    # dsplog level  3  1  10\n\n");
	printf("  Enable debug level for CABAC / CAVLC issue:\n");
	printf("    # dsplog level  3  1  10240\n\n");
}

// dsplog log-filename [ 1000 (max log size) ]
// dsplog d module debuglevel coding_thread_printf_disable_mask

// for iOne / S2:
// dsplog level [module] [0:add or 1:set] [debug mask]
// dsplog thread [thread disable bit mask]
int main(int argc, char **argv)
{
	const char *logfile;
	int fd;
	int fd_write;
	int bytes_not_sync = 0;
	int bytes_written = 0;

	if (argc == 1) {
		usage();
		return -1;
	}

	if (argc > 1) {
		logfile = argv[1];

		if (logfile[0] == 'd' && logfile[1] == '\0') {
			if (argc != 5) {
				printf("usage: dsplog d x y z\n");
				return -1;
			}
			return setup_log(&argv[2]);
		}

		if (strcmp(logfile, "level") == 0) {
			if (argc != 5) {
				printf("usage: dsplog level [module] [0:add or 1:set] [debug mask]\n");
				return -1;
			}
			return setup_level(&argv[2]);
		}

		if (strcmp(logfile, "thread") == 0) {
			if (argc != 3) {
				printf("dsplog thread [thread disable bit mask]\n");
				return -1;
			}
			return setup_thread(&argv[2]);
		}

		if (argc > 2) {
			maxlogs = atoi(argv[2]);
			if (maxlogs < 0) {
				printf("usage: dsplog log-filename [ 1000 (max log size) ] \n");
				maxlogs = 0;
			}
		}
	} else {
		logfile = "/tmp/dsplog.dat";
	}

	if ((fd_write = open(logfile, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(logfile);
		return -1;
	}
//	printf("save dsp log data to %s\n", logfile);

	if (enable_dsplog() < 0)
		return -1;

	if ((fd = open("/dev/dsplog", O_RDONLY, 0)) < 0) {
		perror("/dev/dsplog");
		return -1;
	}

	while (1) {
		ssize_t size = read(fd, buffer, sizeof(buffer));
		if (size < 0) {
			perror("read");
			return -1;
		}

		if (size > 0) {
			if (write(fd_write, buffer, size) != size) {
				perror("write");
				return -1;
			}
			bytes_not_sync += size;
		}

		if ((size == 0 && bytes_not_sync > 0) || bytes_not_sync >= 64 * 1024) {
			fsync(fd_write);
			bytes_written += bytes_not_sync;
			bytes_not_sync = 0;
		}

		if ((maxlogs != 0) && (bytes_written >= (maxlogs * sizeof(u32) * 8))) {
			lseek(fd_write, 0, SEEK_SET);
			bytes_written = 0;
		}
	}

	return 0;
}

