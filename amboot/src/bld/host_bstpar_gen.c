/**
 * @file system/src/bld/host_bstpar_gen.c
 *
 * History:
 *    2009/03/04 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <bstpar_gen.h>
#include <ambhw.h>
#include "partinfo.h"

#ifdef ENABLE_DEBUG_BSTPAR
#define DEBUG_MSG fprintf
#else
#define DEBUG_MSG(...)
#endif

extern int lineno;
extern FILE *yyin;
extern struct bstpar_parsed_s g_bstpar_parsed;

#define NUM_PARAM_FIX		5
#define STOP_FIX		NUM_PARAM_FIX
struct bstpar_list_s g_param[NUM_PARAM_FIX] = {
	{"NAND_512_BLD_SBLK",		xstr(NAND_512_BLD_SBLK),	&g_param[1]},
	{"NAND_512_BLD_NBLK",		xstr(NAND_512_BLD_NBLK),	&g_param[2]},
	{"NAND_2K_BLD_SBLK",		xstr(NAND_2K_BLD_SBLK),		&g_param[3]},
	{"NAND_2K_BLD_NBLK", 		xstr(NAND_2K_BLD_NBLK),		&g_param[4]},
	{"AMBOOT_BLD_RAM_ADDRESS",	xstr(AMBOOT_BLD_RAM_START),	NULL},
};

void *g_param_bak[NUM_PARAM_FIX];

/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile>\n", argv[0]);
}

/**
 * Parser error handling.
 */
int yyerror(const char *msg)
{
	fprintf(stderr, "syntax error: line %d\n", lineno);
	return -1;
}

/**
 * Chnage user parameters.
 */
static void fix_param(struct bstpar_list_s *l,
		      struct bstpar_list_s *u, int *match)
{
	if (*match == STOP_FIX)
		return;

	//for (u; u != NULL; u = u->next) {
	while (u != NULL) {
		if (strcmp(u->name, l->name) == 0) {
			g_param_bak[*match] = l->value;
			l->value = u->value;
			DEBUG_MSG(stdout, "match = %u\n", l->value);
			(*match)++;
		}
		u = u->next;
	}
}

static void free_mem(void)
{
	struct bstpar_list_s *l;
	struct bstpar_list_s *u;
	int i, valid, match = 0;

	for (l = g_bstpar_parsed.list; l != NULL; l = l->next) {
		if (match < STOP_FIX) {
			for (u = g_param; u != NULL; u = u->next) {
				if (strcmp(u->name, l->name) == 0) {
					/* l->value is invalid to free */
					match++;
					valid = 0;
					break;
				} else {
					valid = 1;
				}
			}
		} else {
			valid = 1;
		}

		if (valid && l->value)
			free(l->value);
		if (l->name)
			free(l->name);
	}

	l = g_bstpar_parsed.list;
	while (l != NULL) {
		u = l;
		l = l->next;
		if (u)
			free(u);
	}

	for (i = 0; i < NUM_PARAM_FIX; i++) {
		if (g_param_bak[i])
			free(g_param_bak[i]);
	}

	if (g_bstpar_parsed.title)
		free(g_bstpar_parsed.title);

	if (g_bstpar_parsed.open_bst)
		free(g_bstpar_parsed.open_bst);

	if (g_bstpar_parsed.close_bst)
		free(g_bstpar_parsed.close_bst);

	if (g_bstpar_parsed.open_param)
		free(g_bstpar_parsed.open_param);

	if (g_bstpar_parsed.close_param)
		free(g_bstpar_parsed.close_param);

	if (g_bstpar_parsed.open_value)
		free(g_bstpar_parsed.open_value);

	if (g_bstpar_parsed.close_value)
		free(g_bstpar_parsed.close_value);
}

#if (CHIP_REV == A5S)
static u32 get_param(char *pattern, char *cmp_val, struct bstpar_list_s *l)
{
	for (; l != NULL; l = l->next) {
		char *ptr = l->value;
		if (strcmp(pattern, l->name) == 0) {
			if (cmp_val) {
				u32 val = (strcmp(cmp_val, l->value) == 0)?1:0;
				return val;
			} else
				return strtol(l->value, &ptr, 16);
		}
	}
	return 0;
}

static a5s_spiboot_param(struct bstpar_parsed_s *g_bstpar_parsed)
{
	u32 dqs_sync = 0, pad_zctl = 0, dll0 = 0, dll1 = 0;
	FILE *fout = NULL;
	char buf[64];

	dqs_sync |= get_param("SW_DQS_SYNC_EN", "Enable",g_bstpar_parsed->list);
	dqs_sync |= get_param("SW_DQS_SYNC_CTL", 0, g_bstpar_parsed->list) << 1;
	dqs_sync |= get_param("AUTO_DQS_SYNC_CFG", 0, g_bstpar_parsed->list)<<4;

	pad_zctl |= get_param("PAD_TERM", 0, g_bstpar_parsed->list);

	dll0 |= get_param("DLL0_SEL2", 0, g_bstpar_parsed->list) << 16;
	dll0 |= get_param("DLL0_SEL1", 0, g_bstpar_parsed->list) << 8;
	dll0 |= get_param("DLL0_SEL0", 0, g_bstpar_parsed->list);

	dll1 |= get_param("DLL1_SEL2", 0, g_bstpar_parsed->list) << 16;
	dll1 |= get_param("DLL1_SEL1", 0, g_bstpar_parsed->list) << 8;
	dll1 |= get_param("DLL1_SEL0", 0, g_bstpar_parsed->list);

	fout = fopen("./a5s_spiboot_param.h", "w");

	fwrite("/*\n", 1, strlen("/*\n"), fout);
	fwrite(" * Automatically generated file: don't edit\n", 1, 44, fout);
	fwrite(" */\n\n", 1, strlen(" */\n\n"), fout);
	fwrite("#ifndef __SPIBOOT_PARAM_H__\n", 1, 28, fout);
	fwrite("#define __SPIBOOT_PARAM_H__\n\n", 1, 29, fout);

	/* OutPut parameter */
	sprintf(buf, "#define DRAM_DQS_SYNC 0x%x\n", dqs_sync);
	fwrite(buf, 1, strlen(buf), fout);
	sprintf(buf, "#define DRAM_PAD_ZCTL 0x%x\n", pad_zctl);
	fwrite(buf, 1, strlen(buf), fout);
	sprintf(buf, "#define DRAM_DLL0 0x%x\n", dll0);
	fwrite(buf, 1, strlen(buf), fout);
	sprintf(buf, "#define DRAM_DLL1 0x%x\n", dll1);
	fwrite(buf, 1, strlen(buf), fout);

	fwrite("\n#endif\n", 1, 8, fout);
	fclose(fout);
}
#endif

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
	struct bstpar_list_s *l;
	FILE *fout;
	int match, i;

	if (argc != 2) {
		usage(argc, argv);
		return -1;
	}

	yyin = fopen(argv[1], "r");
	if (yyin == NULL) {
		perror(argv[1]);
		return -2;
	}

	if (yyparse() != 0) {
		return -3;
	}

	fclose(yyin);

	/******************************/
	/* Output new bst parameters. */
	/******************************/
	for (i = 0; i < NUM_PARAM_FIX; i++) {
		g_param_bak[i] = NULL;
	}

	match = 0;
	fprintf(stdout, "%s\n", g_bstpar_parsed.title);
	fprintf(stdout, "%s\n", g_bstpar_parsed.open_bst);

	for (l = g_bstpar_parsed.list; l != NULL; l = l->next) {
		fix_param(l, g_param, &match);

		fprintf(stdout, "  ");
		fprintf(stdout, "%s", g_bstpar_parsed.open_param);
		fprintf(stdout, "%s", l->name);
		fprintf(stdout, " ");
		fprintf(stdout, "%s", g_bstpar_parsed.open_value);
		fprintf(stdout, "%s", l->value);
		fprintf(stdout, "%s", g_bstpar_parsed.close_value);
		fprintf(stdout, "%s", g_bstpar_parsed.close_param);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "%s\n", g_bstpar_parsed.close_bst);

#if (CHIP_REV == A5S)
	a5s_spiboot_param(&g_bstpar_parsed);
#endif
	free_mem();

#if 0
	if (match != STOP_FIX) {
		fprintf(stderr, "Only %d in %d parameters fixed\n",
			match, NUM_PARAM_FIX);
		return -4;
	}
#endif

	return 0;
}
