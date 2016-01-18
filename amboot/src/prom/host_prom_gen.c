/**
 * @file system/src/prom/host_prom_gen.c
 *
 * History:
 *    2009/01/10 - [Chien-Yang Chen] created file
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
#include <ambhw.h>
#include <peripheral/eeprom.h>
#include "prom.h"

#ifdef DEBUG_HOST_PROM
#define DEBUG_MSG	fprintf
#else
#define DEBUG_MSG(...)
#endif

#if	((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	 (CHIP_REV == A2K) || (CHIP_REV == A6))
/* NOTE: suppose corefreq is 216 MHz and ssi_freq = apb_freq / cg_ssi */
#define TARGET_APB_FREQ		108000000
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
      (CHIP_REV == I1)
/* NOTE: to use reasonable value. */
#define TARGET_APB_FREQ		60000000
#endif

#define TARGET_SSI_BUAD		5000000

#if (CHIP_REV == A5L)
#define RCT_DDR_PLL_CTRL_VAL    0x1a111100
#define PLL_DDR2_CTRL2_VAL	0x3f710000
#define PLL_DDR2_CTRL3_VAL	0x00068300
#endif

struct eeprom_header_s header = {

	EEPROM_HEADER_MAGIC,	/* magic */
	0x0,			/* baud */
	0x100,			/* init_delay */
	0x0,			/* rsv */

#if	((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
	 (CHIP_REV == A2K))

	DRAM_CFG_VAL,			/* dram_cfg */

#if	(defined(PWC_CORE_108MHZ) || defined(FIX_CORE_108MHZ))
	DRAM_MODE____108MHZ,		/* dram_mode */
	DRAM_EXT_MODE_VAL,		/* dram_ext_mode */
	DRAM_TIMING1_108MHZ,		/* dram_timing1 */
	DRAM_TIMING2_108MHZ,		/* dram_timing2 */
	DRAM_REF_CTL_108MHZ,		/* dram_ref_ctl */
	RCT_DLL0_108MHZ,		/* dll0 */
	RCT_DLL1_108MHZ,		/* dll1 */
#elif	(defined(PWC_CORE_135MHZ) || defined(FIX_CORE_135MHZ))
	DRAM_MODE____135MHZ,		/* dram_mode */
	DRAM_EXT_MODE_VAL,		/* dram_ext_mode */
	DRAM_TIMING1_135MHZ,		/* dram_timing1 */
	DRAM_TIMING2_135MHZ,		/* dram_timing2 */
	DRAM_REF_CTL_135MHZ,		/* dram_ref_ctl */
	RCT_DLL0_135MHZ,		/* dll0 */
	RCT_DLL1_135MHZ,		/* dll1 */
#elif	(defined(PWC_CORE_162MHZ) || defined(FIX_CORE_162MHZ))
	DRAM_MODE____162MHZ,		/* dram_mode */
	DRAM_EXT_MODE_VAL,		/* dram_ext_mode */
	DRAM_TIMING1_162MHZ,		/* dram_timing1 */
	DRAM_TIMING2_162MHZ,		/* dram_timing2 */
	DRAM_REF_CTL_162MHZ,		/* dram_ref_ctl */
	RCT_DLL0_162MHZ,		/* dll0 */
	RCT_DLL1_162MHZ,		/* dll1 */
#elif	(defined(PWC_CORE_216MHZ) || defined(FIX_CORE_216MHZ))
	DRAM_MODE____216MHZ,		/* dram_mode */
	DRAM_EXT_MODE_VAL,		/* dram_ext_mode */
	DRAM_TIMING1_216MHZ,		/* dram_timing1 */
	DRAM_TIMING2_216MHZ,		/* dram_timing2 */
	DRAM_REF_CTL_216MHZ,		/* dram_ref_ctl */
	RCT_DLL0_216MHZ,		/* dll0 */
	RCT_DLL1_216MHZ,		/* dll1 */
#elif	(defined(PWC_CORE_243MHZ) || defined(FIX_CORE_243MHZ))
	DRAM_MODE____243MHZ,		/* dram_mode */
	DRAM_EXT_MODE_VAL,		/* dram_ext_mode */
	DRAM_TIMING1_243MHZ,		/* dram_timing1 */
	DRAM_TIMING2_243MHZ,		/* dram_timing2 */
	DRAM_REF_CTL_243MHZ,		/* dram_ref_ctl */
	RCT_DLL0_243MHZ,		/* dll0 */
	RCT_DLL1_243MHZ,		/* dll1 */
#else
#error "Unknown Power-On-Config Core Frequency!"
#endif

	RCT_DLL_OCD_BITS_VAL,	/* dll_ocd */
	0x0,			/* ddrio_calib */
	{			/* rsv1[14] */
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	},

#elif	(CHIP_REV == A5S)

	0x0,			/* dram_control */
	0x0,			/* dram_config */
	0x0,			/* dram_timing1 */
	0x0,			/* dram_timing2 */
	0x0,			/* dram_timing3 */
	0x0,			/* dram_init_ctl */
	0x0,			/* dram_mode */
	0x0,			/* dram_ext_mode */
	0x0,			/* dram_ext_mode2 */
	0x0,			/* dram_ext_mode3 */
	0x0,			/* dram_self_ref */
	0x0,			/* dram_dqs_sync */
	0x0,			/* dram_pad_term */
	0x0,			/* dram_zq_calib */
	0x0,			/* dll0 */
	0x0,			/* dll1 */
	0x0,			/* ddrio_calib */
	0x0,			/* dll_ctrl_sel */
	0x0,			/* dll_ocd */
	{			/* rsv1[5] */
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	},

#elif	(CHIP_REV == A6)

	0x0,				/* dram_control */
	DRAM_CFG_VAL,			/* dram_config */
	DRAM_TIMING1_216MHZ,		/* dram_timing1 */
	DRAM_TIMING2_216MHZ,		/* dram_timing2 */
	0x0,				/* dram_init_ctl */
	DRAM_EXT_MODE0_216MHZ,		/* dram_ext_mode0 */
	DRAM_EXT_MODE1_216MHZ,		/* dram_ext_mode1 */
	DRAM_EXT_MODE2_216MHZ,		/* dram_ext_mode2 */
	DRAM_EXT_MODE3_216MHZ,		/* dram_ext_mode3 */
	0x0,				/* dram_dqs_sync */
	RCT_DLL0_216MHZ,		/* dll0 */
	RCT_DLL1_216MHZ,		/* dll1 */
	RCT_DLL2_216MHZ,		/* dll2 */
	RCT_DLL3_216MHZ,		/* dll3 */
	RCT_DLL_OCD_BITS_216MHZ,	/* dll_ocd */
	RCT_DDRIO_CALIB_216MHZ,		/* ddrio_calib */
	{				/* rsv1[8] */
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	},
#elif	(CHIP_REV == A7) || (CHIP_REV == I1)

	0x0,			/* dram_control */
	0x0,			/* dram_config */
	0x0,			/* dram_timing1 */
	0x0,			/* dram_timing2 */
	0x0,			/* dram_timing3 */
	0x0,			/* dram_init_ctl */
	0x0,			/* dram_mode */
	0x0,			/* dram_ext_mode */
	0x0,			/* dram_ext_mode2 */
	0x0,			/* dram_ext_mode3 */
	0x0,			/* dram_self_ref */
	0x0,			/* dram_dqs_sync */
	0x0,			/* dram_pad_term */
	0x0,			/* dram_zq_calib */
	0x0,			/* dll0 */
	0x0,			/* dll1 */
	0x0,			/* ddrio_calib */
	0x0,			/* dll_ctrl_sel */
	0x0,			/* dll_ocd */
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	{
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0,
	0x0
	},

#endif
	0x0,			/* payload_len */
	AMBPROM_RAM_START,	/* mem_target */
	{			/* rsv2[2] */
		0x0,
		0x0
	}
};

static unsigned long get_ssi_baud(unsigned long apb_freq,
				  unsigned long target_baud_rate)
{
	unsigned long baud;
	unsigned long ssi_freq;

	/* The divider programmed in chiprom is 2. */
	ssi_freq = apb_freq / 2;
	baud = (int)(((ssi_freq / target_baud_rate) + 0x01) & 0xfffe);

	if (baud > 65534)
		baud = 65534;

	return baud;
}

#if	(CHIP_REV == A5S)
static int load_dram_parameters(struct eeprom_header_s *header,
				const char *file)
{
#define DQS_SYBC_VALUE	0x46
#define PAD_TERM_VALUE	0x20048
#define DLL0_VALUE	0x201020
#define DLL1_VALUE	0x201020

	FILE *fin = NULL;
	int rval, rsize;
	struct stat f_stat;

	stat(file, &f_stat);
	if (f_stat.st_size > sizeof(struct eeprom_header_s))
		rsize = sizeof(struct eeprom_header_s);
	else
		rsize = f_stat.st_size;

	fin = fopen(file, "r");
	if (fin == NULL) {
		fprintf(stdout, "Can't open file: %s", file);
		return -1;
	}

	rval = fread(header, 1, rsize, fin);
	if (rval != rsize) {
		fprintf(stdout, "fread failed: %s", file);
		return -1;
	}

	rval = fclose(fin);
	if (rval < 0) {
		fprintf(stdout, "Can't close file: %s", file);
		return -1;
	}

	header->magic = EEPROM_HEADER_MAGIC;
	header->baud = get_ssi_baud(TARGET_APB_FREQ, TARGET_SSI_BUAD);
	header->init_delay = 0x100;
	header->rsv = 0x0;
	header->payload_len = 0x0;
	header->mem_target = AMBPROM_RAM_START;
	header->rsv2[0] = 0x0;
	header->rsv2[1] = 0x0;

	header->dram_dqs_sync = DQS_SYBC_VALUE;
	header->dram_pad_term = PAD_TERM_VALUE;
	header->dll0 = DLL0_VALUE;
	header->dll1 = DLL1_VALUE;

	return 0;
}
/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile> <outfile> <dram_cfg_file>\n", argv[0]);
}

#elif	(CHIP_REV == A7) || (CHIP_REV == I1)
static int load_dram_parameters(struct eeprom_header_s *header,
				const char *file)
{
	FILE *fin = NULL;
	int rval, rsize;
	struct stat f_stat;
	int i;

	stat(file, &f_stat);
	if (f_stat.st_size > sizeof(struct eeprom_header_s))
		rsize = sizeof(struct eeprom_header_s);
	else
		rsize = f_stat.st_size;

	fin = fopen(file, "r");
	if (fin == NULL) {
		fprintf(stdout, "Can't open file: %s", file);
		return -1;
	}

	rval = fread(header, 1, rsize, fin);
	if (rval != rsize) {
		fprintf(stdout, "fread failed: %s", file);
		return -1;
	}

	rval = fclose(fin);
	if (rval < 0) {
		fprintf(stdout, "Can't close file: %s", file);
		return -1;
	}

	header->magic = EEPROM_HEADER_MAGIC;
	header->baud = get_ssi_baud(TARGET_APB_FREQ, TARGET_SSI_BUAD);
	header->dram_delay = 0x100;
	header->mem_target = AMBPROM_RAM_START;
	header->payload_len = 0x0;
	for(i = 0; i < 32; i++)
		header->rsv2[i] = 0x0;
	header->rsv3[0] = 0x0;
	header->rsv3[1] = 0x0;

	return 0;
}


/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile> <outfile> <dram_cfg_file>\n", argv[0]);
}

#elif (CHIP_REV == A5L)
static int load_dram_parameters(struct eeprom_header_s *header)
{
	memset(header, 0x0, sizeof(struct eeprom_header_s));

	header->magic = EEPROM_HEADER_MAGIC;
	header->baud = get_ssi_baud(TARGET_APB_FREQ, TARGET_SSI_BUAD);
	header->init_delay = 0x100;
	header->rsv = 0x0;

	/* DRAM parameter */
#if     defined(PWC_DRAM_135MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_135MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_135MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_135MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_135MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_135MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_135MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_135MHZ;
#elif	defined(PWC_DRAM_162MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_162MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_162MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_162MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_162MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_162MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_162MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_162MHZ;
#elif 	defined(PWC_DRAM_180MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_180MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_180MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_180MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_180MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_180MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_180MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_180MHZ;
#elif	defined(PWC_DRAM_216MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_216MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_216MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_216MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_216MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_216MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_216MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_216MHZ;
#elif	defined(PWC_DRAM_240MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_240MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_240MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_240MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_240MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_240MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_240MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_240MHZ;
#elif	defined(PWC_DRAM_270MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_270MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_270MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_270MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_270MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_270MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_270MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_270MHZ;
#elif	defined(PWC_DRAM_288MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_288MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_288MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_288MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_288MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_288MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_288MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_288MHZ;
#elif	defined(PWC_DRAM_300MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_300MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_300MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_300MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_300MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_300MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_300MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_300MHZ;
#elif	defined(PWC_DRAM_324MHZ)
	header->dram_timing1	= DRAM_TIMING1_VAL_324MHZ;
	header->dram_timing2	= DRAM_TIMING2_VAL_324MHZ;
	header->dram_timing3	= DRAM_TIMING3_VAL_324MHZ;
	header->dram_mode	= DRAM_EXT_MODE0_VAL_324MHZ;
	header->dram_ext_mode	= DRAM_EXT_MODE1_VAL_324MHZ;
	header->dram_ext_mode2	= DRAM_EXT_MODE2_VAL_324MHZ;
	header->dram_ext_mode3	= DRAM_EXT_MODE3_VAL_324MHZ;
#else
#error	Unsupport DRAM frequency
#endif
	header->dll0		= RCT_DLL0_VAL;
	header->dll1		= RCT_DLL1_VAL;
	header->dram_control	= 0x0;
	header->dram_config 	= DRAM_CFG_VAL;
	header->por_delay	= 0x100;
	header->dram_self_ref	= 0x0;
	header->dram_zq_calib	= DRAM_ZQ_CALIB;
	header->dram_pad_term	= DRAM_PAD_TERM_VAL;
	header->dram_dqs_sync	= DRAM_DQS_SYNC_VAL;

	header->pll_ddr_ctrl	= RCT_DDR_PLL_CTRL_VAL;
	header->dram_dll_ctrl	= RCT_DLL_CTRL_SEL_VAL;
	header->pll_ddr_ctrl2	= PLL_DDR2_CTRL2_VAL;
	header->pll_ddr_ctrl3	= PLL_DDR2_CTRL3_VAL;
	header->delay2		= 0x100;

	header->rsv1[0] 	= 0x0;
	header->rsv1[1] 	= 0x0;
	header->rsv1[2] 	= 0x0;

	header->payload_len = 0x0;
	header->mem_target = AMBPROM_RAM_START;
	header->rsv2[0] = 0x0;
	header->rsv2[1] = 0x0;

	return 0;
}
/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile> <outfile> <dram_cfg_file>\n", argv[0]);
}
#else

/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <infile> <outfile>\n", argv[0]);
}
#endif

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
	FILE *fin = NULL;
	FILE *fout = NULL;
	char buf[1024];
	struct stat f_stat;
	size_t rsize, wsize;
	int rval;
	u32 ssi_freq, target_baud_rate;

	if (argc != 3 && argc != 4) {
		usage(argc, argv);
		return -1;
	}

	header.baud = get_ssi_baud(TARGET_APB_FREQ, TARGET_SSI_BUAD);

#if	(CHIP_REV == A5S || CHIP_REV == A7) || (CHIP_REV == I1)
	rval = load_dram_parameters(&header, argv[3]);
	if (rval < 0) {
		fprintf(stdout, "load_dram_parameters() failed: %s", argv[3]);
		return -1;
	}
#elif	(CHIP_REV == A2S)
	/* The baud rate can't be set in A2S chiprom. */
	/* Just use default value here. */
	/* The default baud rate is about 2 MHz if corefreq = 216 Mhz. */
	/* ssi_freq = 108000000 / 2 = 54000000 */
	/* baud_rate = 54000000 / 0x1c = 1928571 ~= 2 MHz */
	header.baud = 0x1c;
#elif	(CHIP_REV == A5L)
	rval = load_dram_parameters(&header);
#else
	/* Same as default value in chiprom. */
	header.baud = 0x1c;
#endif

	/* prom payload size */
	stat(argv[1], &f_stat);
	DEBUG_MSG(stdout, "file size = %d", f_stat.st_size);

	header.payload_len = f_stat.st_size;

	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		fprintf(stdout, "Can't open file: %s", argv[1]);
		return -1;
	}

	fout = fopen(argv[2], "wb");
	if (fout == NULL) {
		fprintf(stdout, "Can't open file: %s", argv[2]);
		return -1;
	}

	wsize = fwrite(&header, 1, sizeof(header), fout);
	if (wsize != sizeof(header)) {
		fprintf(stdout, "fwrite failed: %s", argv[2]);
		return -1;
	}

	while ((rsize = fread(buf, 1, sizeof(buf), fin)) > 0) {
		wsize = fwrite(buf, 1, rsize, fout);
		if (wsize != rsize) {
			fprintf(stdout, "fwrite failed: %s", argv[2]);
			return -1;
		}
	}

	rval = fclose(fin);
	if (rval < 0) {
		fprintf(stdout, "Can't close file: %s", argv[1]);
		return -1;
	}

	rval = fclose(fout);
	if (rval < 0) {
		fprintf(stdout, "Can't close file: %s", argv[2]);
		return -1;
	}

	return 0;
}
