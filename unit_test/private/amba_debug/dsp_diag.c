/*
 * dsp_diag.c
 *
 * History:
 *    2013/08/21 - [Cao Rongrong] Create
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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <basetypes.h>
#include <amba_debug.h>


/*============================== Constants ===================================*/
#define PAGE_SIZE			(1 << 12)
#define MAX_STRING_LEN			256

#define NO_ARG				0
#define HAS_ARG				1


/*========================== Structures and Enums ============================*/
struct hint_s {
	const char			*arg;
	const char			*str;
};

enum amba_dsp_diag_mode {
	AMBA_DSP_DIAG_UNKNOWN,
	AMBA_DSP_DIAG_READ,
	AMBA_DSP_DIAG_WRITE,
};

/*============================= Global Variables =============================*/
static struct option long_options[] = {
	{"read",	HAS_ARG, 0, 'r'},
	{"write",	HAS_ARG, 0, 'w'},
	{"data",	HAS_ARG, 0, 'd'},
	{"size",	HAS_ARG, 0, 's'},
	{"file",	HAS_ARG, 0, 'f'},
	{"format",	NO_ARG, 0, 'F'},
	{"format",	HAS_ARG, 0, 'P'},
	{"verbose",	NO_ARG, 0, 'v'},
	{0, 0, 0, 0}
};

static const char *short_options = "r:w:d:s:f:FP:v";

static const struct hint_s hint[] = {
	{"address",		"read address, hex[0x00000000]"},
	{"address",		"write address, hex[0x00000000]"},
	{"data",		"data to written, hex[0x00000000]"},
	{"size",		"read/write size, hex[0x00000000]"},
	{"file",		"file to load or store binary data"},
	{"",			"use DSP diag image format"},
	{"pad width",		"pad width when using diag image format"},
	{"",			"verbose"},
};

static enum amba_dsp_diag_mode diag_mode = AMBA_DSP_DIAG_UNKNOWN;

static u32	verbose = 0;
static u32	diag_address = 0;
static u32	diag_size = 1;
static int	diag_data = -1;
static char	diag_filename[MAX_STRING_LEN] = "";
static u32	diag_data_valid = 0;
static u32	image_format = 0;
static u32	image_pad_width = 0;

/*========================== Functions =======================================*/
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

static void usage(void)
{
	int i;

	printf("amba_debug usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		printf("--%s", long_options[i].name);
		if (long_options[i].val != 0)
			printf("/-%c", long_options[i].val);
		if (hint[i].arg[0] != 0)
			printf(" <%s>", hint[i].arg);
		else
			printf("\t");
		printf("\t%s\n", hint[i].str);
	}
}

static int init_param(int argc, char **argv)
{
	int rval = 0, ch, option_index = 0;

	opterr = 0;

	while (1) {
		ch = getopt_long(argc, argv,
				short_options, long_options, &option_index);
		if (ch < 0)
			break;

		switch (ch) {
		case 'r':
			diag_mode = AMBA_DSP_DIAG_READ;
			rval = sscanf(optarg, "0x%08x", &diag_address);
			if (rval != 1)
				rval = -1;
			break;

		case 'w':
			diag_mode = AMBA_DSP_DIAG_WRITE;
			rval = sscanf(optarg, "0x%08x", &diag_address);
			if (rval != 1)
				rval = -1;
			break;

		case 'd':
			rval = sscanf(optarg, "0x%08x", &diag_data);
			if (rval != 1)
				rval = -1;
			else
				diag_data_valid = 1;
			break;

		case 's':
			diag_size = strtol(optarg, NULL, 0);
			break;

		case 'f':
			strcpy(diag_filename, optarg);
			break;

		case 'F':
			image_format = 1;
			break;

		case 'P':
			image_pad_width = strtol(optarg, NULL, 0);
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			rval = -1;
			break;
		}
	}

	if (diag_mode == AMBA_DSP_DIAG_UNKNOWN) {
		printf("Please specify diag operation\n");
		return -1;
	}

	if (diag_address & 0x03) {
		printf("address must be 4 bytes aligned\n");
		return -1;
	}

	if (image_format && strlen(diag_filename) == 0) {
		printf("please specify filename for dsp image\n");
		return -1;
	}

	return rval;
}

static int dump_data(char *pmem, u32 phys_addr, u32 size)
{
	int fd, i;

	if (strlen(diag_filename)) {
		fd = open(diag_filename, O_CREAT | O_TRUNC | O_WRONLY, 0777);
		if (fd < 0) {
			perror(diag_filename);
			return -1;
		}

		if (write(fd, pmem, size) < 0) {
			perror("write");
			return -1;
		}

		close(fd);
	} else {
		for (i = 0; i < size; i += 4) {
			if (i % 16 == 0)
				printf("\n0x%08x:", phys_addr + i);

			printf("\t0x%08x", *(u32 *)(pmem + i));
		}
		printf("\n");
	}

	return 0;
}

static int fill_data(char *pmem, u32 phys_addr, u32 size)
{
	u32 i, j, data, width, height, n;
	int fd;

	if (strlen(diag_filename)) {
		fd = open(diag_filename, O_RDONLY, 0777);
		if (fd < 0) {
			perror(diag_filename);
			return -1;
		}

		if (image_format) {
			n = read(fd, &data, sizeof(data));
			if (n != sizeof(data)) {
				perror("read");
				return -1;
			}

			height = (data & 0xffff0000) >> 16;
			width = data & 0x0000ffff;

			for (i = 0; i < height; i++) {
				n = read(fd, pmem + i * image_pad_width, width);
				if (n != width) {
					perror("read");
					return -1;
				}

				/* padding with 0 */
				for (j = 0; j < image_pad_width - width; j++)
					*(pmem + width + j) = 0;
			}
		} else {
			if (read(fd, (void *)pmem, size) < 0) {
				perror("read");
				return -1;
			}
		}

		close(fd);
	} else if (diag_data_valid) {
		for (i = 0; i < size; i += 4) {
			*(u32 *)(pmem + i) = diag_data;
		}
	}


	return 0;
}

static int dsp_diag(void)
{
	int fd, data, width, height;
	char *mem_ptr = NULL;
	u32 mmap_start = 0, mmap_offset;

	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

	/* loadimage with format: 2 bytes for width and 2 bytes for height */
	if (diag_mode == AMBA_DSP_DIAG_WRITE && strlen(diag_filename)) {
		if (image_format) {
			int fd_image = open(diag_filename, O_RDONLY, 0777);
			if (fd_image < 0) {
				perror(diag_filename);
				return -1;
			}

			if (read(fd_image, &data, sizeof(data)) < 0) {
				perror("read");
				return -1;
			}

			close(fd_image);

			width = data & 0x0000ffff;
			height = (data & 0xffff0000) >> 16;
			if (verbose)
				printf("width = %d, height = %d\n", width, height);

			if (image_pad_width)
				image_pad_width = ROUND_UP(image_pad_width, 32);
			else
				image_pad_width = ROUND_UP(width, 32);

			diag_size = image_pad_width * height;

		} else if (diag_size == 1) {
			struct stat st;
			if(stat(diag_filename, &st) < 0){
				perror(diag_filename);
				return -1;
			}

			diag_size = st.st_size;
		}
	}

	mmap_start = diag_address & (~(PAGE_SIZE - 1));
	mmap_offset = diag_address - mmap_start;

	mem_ptr = (char *)mmap(NULL, diag_size + mmap_offset,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				fd, mmap_start);
	if (mem_ptr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	switch (diag_mode) {
	case AMBA_DSP_DIAG_READ:
		if (verbose) {
			printf("debug read: diag_address = 0x%08x mmap_start = "
				"0x%08x diag_size = %d\n",
				diag_address, mmap_start, diag_size);
		}
		dump_data(mem_ptr + mmap_offset, diag_address, diag_size);
		break;

	case AMBA_DSP_DIAG_WRITE:
		if (verbose) {
			printf("debug write: diag_address = 0x%08x mmap_start = "
				"0x%08x diag_size = %d\n",
				diag_address, mmap_start, diag_size);
		}
		fill_data(mem_ptr + mmap_offset, diag_address, diag_size);
		break;

	default:
		printf("Invalid diag_mode %d\n", diag_mode);
		break;
	}

	if (mem_ptr)
		munmap(mem_ptr, diag_size);

	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2 || init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	return dsp_diag();
}

