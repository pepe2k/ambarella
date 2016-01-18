/*
 * load_ucode.c
 *
 * History:
 *	2012/01/25 - [Jian Tang] created file
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
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "types.h"
#include "iav_drv.h"

int load_ucode(const char *ucode_path)
{
	int i;
	int fd = -1;
	FILE *fp;
	ucode_load_info_t info;
	ucode_version_t version;
	u8 *ucode_mem;
	char filename[256];
	int file_length;

	if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		return -1;
	}

	printf("map_size = 0x%x\n", (u32)info.map_size);
	printf("nr_item = %d\n", info.nr_item);
	for (i = 0; i < info.nr_item; i++) {
		printf("addr_offset = 0x%x ", (u32)info.items[i].addr_offset);
		printf("filename = %s\n", info.items[i].filename);
	}

	ucode_mem = (u8 *)mmap(NULL, info.map_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((int)ucode_mem == -1) {
		perror("mmap");
		return -1;
	}
	printf("mmap returns 0x%x\n", (unsigned)ucode_mem);

	for (i = 0; i < info.nr_item; i++) {
		u8 *addr = ucode_mem + info.items[i].addr_offset;
		sprintf(filename, "%s/%s", ucode_path, info.items[i].filename);

		printf("loading %s...", filename);
		if ((fp = fopen(filename, "rb")) == NULL) {
			perror(filename);
			return -1;
		}

		if (fseek(fp, 0, SEEK_END) < 0) {
			perror("SEEK_END");
			return -1;
		}
		file_length = ftell(fp);
		if (fseek(fp, 0, SEEK_SET) < 0) {
			perror("SEEK_SET");
			return -1;
		}

		printf("addr = 0x%x, size = 0x%x\n", (u32)addr, file_length);

		if (fread(addr, 1, file_length, fp) != file_length) {
			perror("fread");
			return -1;
		}

		fclose(fp);
	}

	if (ioctl(fd, IAV_IOC_UPDATE_UCODE, 0) < 0) {
		perror("IAV_IOC_UPDATE_UCODE");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_VERSION, &version) < 0) {
		perror("IAV_IOC_GET_UCODE_VERSION");
		return -1;
	}

	printf("===============================\n");
	printf("u_code version = %d/%d/%d %d.%d\n",
		version.year, version.month, version.day,
		version.edition_num, version.edition_ver);
	printf("===============================\n");

//	if (munmap(ucode_mem, info.map_size) < 0)
//		perror("munmap");

	if (fd >= 0)
		close(fd);

	return 0;
}


int main(int argc, char **argv)
{
	const char *ucode_path;

	if (argc > 1) {
		// ucode path specified
		ucode_path = argv[1];
	} else {
		// use default path
		ucode_path = "/mnt/firmware";
	}

	return load_ucode(ucode_path);
}

