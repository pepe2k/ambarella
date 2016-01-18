/**
 * @file system/src/comsvc/host_romfs_gen.c
 *
 * History:
 *    2006/04/22 - [Charles Chiou] created file
 *    2007/06/15 - [Charles Chiou] fixed stack over-flow bug caused by
 *		not calling on fclose() for each fopen()
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __ROMFS_IMPL__
#include <fio/romfs.h>
#include <ambhw.h>

extern int lineno;
extern FILE *yyin;
extern struct romfs_parsed_s romfs_parsed;

/* Alignment to 2K is general from nand and nor */
#define ROMFS_DATA_ALIGN	2048

/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile> <outfile>\n", argv[0]);
}

/**
 * Parser error handling.
 */
int yyerror(const char *msg)
{
	fprintf(stderr, "syntax error: line %d\n", lineno);
	return -1;
}

static void free_mem(void)
{
	struct romfs_list_s *l;
	struct romfs_list_s *u;

	for (l = romfs_parsed.list; l != NULL; l = l->next) {
		if (l->file)
			free(l->file);
		if (l->alias)
			free(l->alias);
	}

	l = romfs_parsed.list;
	while (l != NULL) {
		u = l;
		l = l->next;
		if (u)
			free(u);
	}
	
	if (romfs_parsed.top)
		free(romfs_parsed.top);
}

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
	struct romfs_list_s *l;
	struct romfs_meta_s *meta;
	struct stat buf;
	int offset = 0;
	FILE *fout = NULL;
	FILE *fin = NULL;
	char bin_dat[1024];
	int file_cnt, s;
	int rval;
	u32 bin_size;

	if (argc != 3) {
		usage(argc, argv);
		return -1;
	}

	yyin = fopen(argv[1], "r");
	if (yyin == NULL) {
		perror(argv[2]);
		return -2;
	}

	if (yyparse() != 0) {
		return -3;
	}

	fclose(yyin);

	fout = fopen(argv[2], "wb");
	if (fout == NULL) {
		fprintf(stderr, "%s: error in opening file!\n", argv[2]);
		return -4;
	}

	rval = chdir(romfs_parsed.top);
	if (rval < 0) {
		perror("chdir");
		return -5;
	};

	offset = ROMFS_META_SIZE;

	/* 1st pass: check for file existence and gather file size */
	for (l = romfs_parsed.list; l; l = l->next) {
		rval = stat(l->file, &buf);
		if (rval < 0) {
			perror(l->file);
			return -6;
		}

		if (!S_ISREG(buf.st_mode)) {
			fprintf(stderr, "%s: invalid file!\n", l->file);
			return -7;

		}

		l->size = buf.st_size;
		offset += sizeof(struct romfs_header_s);
	}

	/* padding ROMFS header data to 2K aligned */
	s = offset;
	s %= ROMFS_DATA_ALIGN;

	if (s > 0)
		s = ROMFS_DATA_ALIGN - s;
	else
		s = 0;

	offset += s;

	/* 2nd pass: calculate the offset */
	for (l = romfs_parsed.list; l; l = l->next) {
		l->offset = offset;
		l->padding = (ROMFS_DATA_ALIGN - (l->size % ROMFS_DATA_ALIGN));
		offset = offset + l->size + l->padding;
	}

	/* Check rom size */
	bin_size = offset;
	if (bin_size > AMBOOT_ROM_SIZE || bin_size == 0 ||
	    AMBOOT_ROM_SIZE == 0) {
		fclose(fout);
		system("rm -f ./amboot/romfs.bin");
		fprintf(stderr, "\nERR: Partition size %d "
				"is not enough for ROMFS %d\n",
				AMBOOT_ROM_SIZE,
				bin_size);
		return -10;
	}

	/* 3rd pass: output ROMFS meta data */
	file_cnt = 0;

	for (l = romfs_parsed.list; l; l = l->next) {
		file_cnt++;
	}

	meta = (struct romfs_meta_s *) (bin_dat);
	meta->file_count = file_cnt;
	meta->magic = ROMFS_META_MAGIC;

	s = sizeof(romfs_meta_t) - sizeof(meta->padding);

	rval = fwrite(meta, 1, s, fout);
	if (rval < 0) {
		perror("fwrite");
		exit(1);
	}

	s = sizeof(meta->padding);

	bin_dat[0] = 0xff;
	while (s > 0) {
		rval = fwrite(bin_dat, 1, 1, fout);
		if (rval < 0) {
			perror("fwrite");
			exit(1);
		}
		s--;
	}

	offset = ROMFS_META_SIZE;

	/* 4rd pass: output ROMFS header data */
	for (l = romfs_parsed.list; l; l = l->next) {
		struct romfs_header_s h;

		memset(&h, 0xff, sizeof(h));

		if (l->alias)
			strncpy(h.name, l->alias, sizeof(h.name));
		else
			strncpy(h.name, l->file, sizeof(h.name));
		h.offset = l->offset;
		h.size = l->size;
		h.magic = ROMFS_HEADER_MAGIC;
		rval = fwrite(&h, 1, sizeof(h), fout);
		if (rval < 0) {
			perror("fwrite");
			exit(1);
		}

		offset += rval;
	}

	/* padding ROMFS header data to 2K aligned */
	s = sizeof(romfs_header_t) * file_cnt + ROMFS_META_SIZE;
	s %= ROMFS_DATA_ALIGN;

	if (s > 0)
		s = ROMFS_DATA_ALIGN - s;
	else
		s = 0;

	/* update offset because maybe padding to header data */
	offset += s;

	bin_dat[0] = 0xff;
	while (s > 0) {
		rval = fwrite(bin_dat, 1, 1, fout);
		if (rval < 0) {
			perror("fwrite");
			exit(1);
		}
		s--;
	}

	/* 5th pass: output ROMFS binary data */
	for (l = romfs_parsed.list; l; l = l->next) {
		if (l->offset != offset) {
			fprintf(stderr, "%s: offset: %d != %d\n",
				l->file, l->offset, offset);
		}

		fin = fopen(l->file, "rb");
		if (fin == NULL) {
			fprintf(stderr, "%s: error in opening file!\n",
				l->file);
			exit(2);
		}

		/* output file content */
		do {
			rval = fread(bin_dat, 1, sizeof(bin_dat), fin);
			if (rval < 0) {
				perror("fread");
				exit(3);
			}

			if (rval > 0) {
				rval = fwrite(bin_dat, 1, rval, fout);
				if (rval < 0) {
					perror("fwrite");
					exit(4);
				}
				offset += rval;
			}
		} while (rval > 0);

		fclose(fin);

		/* output padding */
		bin_dat[0] = 0xff;
		while (l->padding > 0) {
			rval = fwrite(bin_dat, 1, 1, fout);
			if (rval < 0) {
				perror("fwrite");
				exit(5);
			}
			offset += rval;
			l->padding--;
		}
	}

	fclose(fout);
	free_mem();

	return 0;
}
