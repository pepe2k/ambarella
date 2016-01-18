/*
 * dsplog2.c
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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "types.h"
#include "iav_drv.h"




typedef struct idsp_printf_s {
	u32 seq_num;		/**< Sequence number */
	u8  dsp_core;
	u8  thread_id;
	u16 reserved;
	u32 format_addr;	/**< Address (offset) to find '%s' arg */
	u32 arg1;		/**< 1st var. arg */
	u32 arg2;		/**< 2nd var. arg */
	u32 arg3;		/**< 3rd var. arg */
	u32 arg4;		/**< 4th var. arg */
	u32 arg5;		/**< 5th var. arg */
} idsp_printf_t;

u8 *pcode;
u8 *pmdxf;
u8 *pmemd;

driver_version_t driver_info;

int get_driver_info(void)
{
	int fd;

	if ((fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_DRIVER_INFO, &driver_info) < 0) {
		perror("IAV_IOC_GET_DRIVER_INFO");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int is_ione_or_a7(void)
{
	return ((driver_info.arch == 7200) ||
		(driver_info.arch == 7000) ||
		(driver_info.arch == 9000));
}

int get_dsp_txt_base_addr_offset(u32 *core_offset, u32 *mdxf_offset, u32 *memd_offset)
{
	int arch_id = driver_info.arch;
	if (!core_offset || ! mdxf_offset || !memd_offset)
		return -1;
	switch(arch_id)
	{
		case 7200:	//iOne
			*core_offset = 0x1900000;
			*mdxf_offset = 0x1600000;
			*memd_offset = 0x1300000;
			break;

		case 5100:	//A5s
		case 7500:	//A7l
			*core_offset = 0x900000;
			*mdxf_offset = 0x600000;
			*memd_offset = 0x300000;
			break;

		case 7000:	//A7
		case 9000:	//S2
			*core_offset = 0x800000;
			*mdxf_offset = 0x600000;
			*memd_offset = 0x300000;
			break;

		case 2000:	//A2
		case 2100:	//A2S
		case 5500:	//A5L
			*core_offset = 0xf0000000;
			*mdxf_offset = 0xf0000000;
			*memd_offset = 0xf0000000;
			break;
		default:
			return -1;

	}
	return 0;
}


#ifdef WIN32
u8 *read_firmware(char *dir, char *name)
{
	u8 *mem;
	HANDLE hfile;
	HANDLE hfilemap;
	int fsize;
	char filename[256];

	sprintf(filename, "%s/%s", dir, name);
	hfile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "cannot open %s\n", filename);
		return NULL;
	}
	fsize = GetFileSize(hfile, NULL);

	hfilemap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, fsize, NULL);
	if (hfilemap == NULL) {
		fprintf(stderr, "CreateFileMapping failed\n");
		return NULL;
	}

	mem = MapViewOfFile(hfilemap, FILE_MAP_READ, 0, 0, fsize);
	if (mem == NULL) {
		fprintf(stderr, "MapViewOfFile failed\n");
		return NULL;
	}

	return mem;
}
#else
u8 *read_firmware(char *dir, char *name)
{
	u8 *mem;
	FILE *fp;
	int fsize;
	char filename[256];

	sprintf(filename, "%s/%s", dir, name);
	if ((fp = fopen(filename, "r")) == NULL) {
		perror(filename);
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END) < 0) {
		perror("fseek");
		return NULL;
	}
	fsize = ftell(fp);

	mem = (u8 *)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
	if ((int)mem == -1) {
		perror("mmap");
		return NULL;
	}

	return mem;
}
#endif

int print_log(idsp_printf_t *record)
{
	char *fmt;
	u8 *ptr;
	u32 offset;
	u32 core_offset, mdxf_offset, memd_offset;
	if (record->format_addr == 0)
		return -1;
	if (get_dsp_txt_base_addr_offset(&core_offset, &mdxf_offset, &memd_offset) < 0) {
		fprintf(stderr, "get dsp txt base addr failed \n");
		return -1;
	}
	switch (record->dsp_core) {
		case 0: ptr = pcode; offset = core_offset; break;
		case 1: ptr = pmdxf; offset = mdxf_offset; break;
		case 2: ptr = pmemd; offset = memd_offset; break;
		default:
			fprintf(stderr, "dsp_core = %d\n", record->dsp_core);
			return -1;
	}
	fmt = (char*)(ptr + (record->format_addr - offset));
	printf("[core:%d:%d] ", record->thread_id, record->seq_num);
	printf(fmt, record->arg1, record->arg2, record->arg3, record->arg4, record->arg5);
	return 0;
}

// usage:
//	dsplog2
//	dsplog2 logfilename
//	dsplog2 logfilename ucodepath
int main(int argc, char **argv)
{
	FILE *input = NULL;
	int first = 0;
	int last = 0;
	int total = 0;
	idsp_printf_t record;
	const char *log_filename = NULL;
	char *ucode_path = "/lib/firmware";
	char *code_name = NULL;
	char *mdxf_name = NULL;
	char *memd_name = NULL;
	ucode_load_info_t info;
	const char *amba_debug = "amba_debug";
	int fd;

	if (argc == 2)
		log_filename = argv[1];
	else if (argc == 3) {
		log_filename = argv[1];
		ucode_path = argv[2];
	}

	if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		return -1;
	}

	if (info.nr_item < 2) {
		fprintf(stderr, "Please check nr_item!\n");
		return -1;
	}

	code_name = info.items[0].filename;
	memd_name = info.items[1].filename;
	mdxf_name = info.items[2].filename;

	close(fd);

	if (get_driver_info() < 0)
		return -1;

	if ((pcode = read_firmware(ucode_path, code_name)) == NULL)
		return -1;
	if ((pmemd = read_firmware(ucode_path, memd_name)) == NULL)
		return -1;

	if (is_ione_or_a7()) {
		if ((pmdxf = read_firmware(ucode_path, mdxf_name)) == NULL)
			return -1;
	}

	if (log_filename == NULL) {
		if (execlp(amba_debug, "amba_debug", "-r", "0x80000", "-s", "0x20000", "-f", "/tmp/dsplog.dat", NULL) < 0) {
			perror(amba_debug);
			return -1;
		}
		log_filename = "/tmp/dsplog.dat";
	}

	if ((input = fopen(log_filename, "rb")) == NULL) {
		fprintf(stderr, "cannot open %s\n", log_filename);
		return -1;
	}

	for (;;) {
		int rval;
		if ((rval = fread(&record, sizeof(record), 1, input)) != 1) {
			break;
		}
		if (first == 0) {
			first = record.seq_num;
			last = first - 1;
		}
		print_log(&record);
		total++;
		++last;
//		if (++last!= record.seq_num)
//			fprintf(stderr, "\t%d records %d - %d lost\n",
//				record.seq_num - last,
//				last, record.seq_num - 1);
		last = record.seq_num;
		if ((total % 1000) == 0) {
			fprintf(stderr, "\r%d", total);
			fflush(stderr);
		}
	}
	fprintf(stderr, "\r");

	fprintf(stderr, "first record: %d\n", first);
	fprintf(stderr, "total record: %d\n", total);
	fprintf(stderr, " last record: %d\n", last);
	fprintf(stderr, "lost records: %d\n", (last - first + 1) - total);

	fclose(input);

	return 0;
}

