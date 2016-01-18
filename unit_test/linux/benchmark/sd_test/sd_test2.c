/*
 * sd_test.c
 *
 * History:
 *	2011/8/29 - [Tao Wu] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#define NO_ARG		0
#define HAS_ARG		1

static char path[128] = "/tmp/mmcblk0p1/";	// "/tmp/mmcblk0p1"  by default
static int file_new = 0;						// writing file now

static int file_max = 0;						//write max file number on sdcard
static char filename[32] = "0";				// write file name

static unsigned long file_size = 0;			// one file size, unit: Byte

static int isSync = 0;						// don't synchronize buffer to sdcard by default

static unsigned long write_size_k = 0;		// filesize = "write_size_k" x 1024 x "write_times"
static unsigned long write_size_b = 0;		// filesize = "write_size_b" x "write_times"
static unsigned long write_times = 1;

static struct option long_options[] = {
		{"Help.", 						NO_ARG, 	NULL, 	'h'},
		{"The path to write.", 				HAS_ARG, 	NULL, 	'p'},
		{"Write BYTES KBytes at a time.", 	HAS_ARG, 	NULL, 	'k'},
		{"Write BYTES Bytes at a time.", 		HAS_ARG, 	NULL, 	'b'},
		{"Write TIMES times on one file.", 	HAS_ARG, 	NULL, 	't'},
		{"Sync filesystem buffer.", 			NO_ARG, 	NULL, 	'y'},
		{"Print speed every TIMES.",			HAS_ARG, 	NULL, 	's'},
		{"mkfs.vfat sdcard.",				NO_ARG, 	NULL, 	'm'},
		{0, 0, 0, 0},
};

struct hint_s{
	const char *arg;
	const char *str;
};

static const char *short_options = "hp:k:b:t:yds:m";

static const struct hint_s hint[] = {
		{"","Print command parm"},
		{"","The data path"},
		{"","Units: KByte"},
		{"","Units: Byte"},
		{"","Number of times"},
		{"","Sync after had written one file"},
		{"","Units: seconds"},
		{"","Format sdcard in FAT32 "},
};

void usage(void)
{
	int i;
	printf("\nTest sdcard quality by show every wirte speed\n");
	printf("******************************************\n");
	printf("Example: sd_test -k 1024 -t 128 -s 1 \n\t Write [1024x128 KB=128MB] size file and print speed every [1] sec.\n");
	printf("******************************************\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
}

int init_opt(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;

	while ( (ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1){
		switch(ch){
			case 'p':
				strcpy(path, optarg);
				printf("write path: %s\n",path);
				break;
			case 'k':
				write_size_k = atol(optarg);
				printf("write_size: %ld KByte\n", write_size_k);
				break;
			case 'b':
				write_size_b = atol(optarg);
				printf("write_size: %ld Byte\n", write_size_b);
				break;
			case 't':
				write_times = atol(optarg);
				printf("write_times: %ld\n", write_times);
				break;
			case 'y':
				isSync = 1;
				printf("sync file system\n");
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'm':
				if(system("umount /dev/mmcblk0p1") < 0 )
					perror("umount ");
				sleep(1);
				if(system("mkfs.vfat /dev/mmcblk0p1") < 0)
					perror("mkfs.vfat");
				sleep(1);
				if(system ("mount /dev/mmcblk0p1 /tmp/mmcblk0p1") < 0 )
					perror("mount");
				printf("Format sdcard success\n");
				break;
			default:
				printf("unknown option found %c or option requires an argument \n", ch);
				return -1;
				break;
		}
	}
	return 0;
}

unsigned long path_free_space(void)
{
	struct statfs fs;
	if( statfs(path, &fs) < 0){
		perror("statfs");
		return 0;
	}
	return fs.f_bfree;
}

int get_block_size(char *pathname)
{
	struct statfs fs;
	if (statfs(pathname, &fs) < 0){
		perror("statfs");
		return 0;
	}
	return fs.f_bsize ;
}

int write_loop(void)
{
	int i = 0, count = 0, w_size = 0;
	int fd = -1, fister = 1;
	char *buf;
	int msec = 0;
	int prev_msec = 0;
	struct timeval now;

	unsigned long file_block = 0;
	if (write_size_k > 0 ){
		w_size= write_size_k * 1024;
	}else if (write_size_b > 0 ){
		w_size= write_size_b;
	}else{
		printf("Please type -k or -b to set buffer size\n");
		return -1;
	}
	buf = (char *)malloc(w_size);

	file_size = w_size * write_times;
	file_block = file_size / get_block_size(path) ;
	printf("file_size = %ld Byte\n", file_size );
	printf("file_block = %ld \n", file_block ); 	 // on Linux system 1block = 4KByte
	if (chdir(path) < 0){
		perror("chdir");
		if (buf)
			free(buf);
		return -1;
	}

	while(1)
	{
		if ( path_free_space() <  file_block){
			if (fister){
				file_max = count;
				fister = 0;
				printf("max file = %d\n", file_max);
			}
			if (count == file_max ){
				count = 0;
				printf("sdcard has been full\n");
				fprintf(stderr, "rotat write ...\n");
			}
		}
		file_new = count ++;
		sprintf(filename, "%d", file_new);
		if( (fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
			perror("open");
			return -1;
		}

		gettimeofday(&now, NULL);
		prev_msec = now.tv_sec * 1000 + now.tv_usec / 1000;
		for (i = 0; i < write_times; i++){
			if(write( fd, buf, w_size) < 0){
				perror("write");
				break;
			}
			if (i % 10 == 0 && i != 0) {
				gettimeofday(&now, NULL);
				msec = now.tv_sec * 1000 + now.tv_usec / 1000;
				printf("write used %d ms, speed %d KB/s\n", msec - prev_msec, 10*w_size/(msec - prev_msec));
				sleep(20);
				gettimeofday(&now, NULL);
				prev_msec = now.tv_sec * 1000 + now.tv_usec / 1000;
			}
		}
		if ( isSync ){
			printf("fsync %s\n", filename);
			if (fsync (fd) < 0){
				perror("fsync");
				return -1;
			}
			printf("fsync done.\n");
		}
		close(fd);
	}
	free(buf);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2){
		usage();
		return -1;
	}

	if (init_opt(argc, argv) < 0)
		return -1;

	write_loop();

	return 0;
}

