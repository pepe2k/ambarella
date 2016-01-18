/**
 * system/src/bld/splash.c
 *
 * History:
 *    2008/01/29 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#include <vout.h>

#if defined(SHOW_AMBOOT_SPLASH)

extern u32 __heap_start;
extern u32 __heap_end;
extern const char *__gzip_logo_1;
extern const int __gzip_logo_1_len;
extern const char *__gzip_logo_2;
extern const int __gzip_logo_2_len;
extern const char *__gzip_logo_3;
extern const int __gzip_logo_3_len;
extern const char *__gzip_logo_4;
extern const int __gzip_logo_4_len;
extern const char *__gzip_logo_5;
extern const int __gzip_logo_5_len;
u8 *logo_addr;
bmp_t G_splash_bmp;

extern u8 bld_clut[256 * 3];
extern u8 bld_blending[256];

static void update_lookup_table_from_bmp_file(u8 *argb, u32 num)
{
	int	i, offset1, offset2;

	for (i = 0, offset1 = 0, offset2 = 0; i < num; i++, offset1 += 3, offset2 += 4) {
		bld_clut[offset1 + 0]	= argb[offset2 + 2];
		bld_clut[offset1 + 1]	= argb[offset2 + 1];
		bld_clut[offset1 + 2]	= argb[offset2 + 0];
	}

	memset(bld_blending, 255, 256);
}

static void mem2leword(u8 *mem, u32 *w)
{
	u8 data[4];

	data[0] = *(mem++);
	data[1] = *(mem++);
	data[2] = *(mem++);
	data[3] = *(mem++);

	*w = (data[3] << 24) |
	     (data[2] << 16) |
	     (data[1] <<  8) |
	     (data[0]);
}

static void mem2lehfword(u8 *mem, u16 *hfw)
{
	u8 data[2];

	data[0] = *(mem++);
	data[1] = *(mem++);

	*hfw = (data[1] <<  8) |
	       (data[0]);
}

static int bld_parse_bmp(u8 *bmp_addr, bmp_t *bmp)
{
	u32 w;
	u16 hfw;

	mem2lehfword(bmp_addr, &hfw);
	bmp_addr += 2;
	bmp->header.fheader.id = hfw;
	if (bmp->header.fheader.id != 0x4d42) {
		putstr("The file is not bmp file!");
		return -1;
	}

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.fheader.file_size = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.fheader.reserved = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.fheader.offset = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.bmp_header_size = w;
	if (bmp->header.iheader.bmp_header_size != 0x28) {
		if (bmp->header.iheader.bmp_header_size == 0x0c)
			putstr("It's OS/2 1.x info header size!");
		else if (bmp->header.iheader.bmp_header_size == 0xf0)
			putstr("It's OS/2 2.x info header size!");
		else
			putstr("Unknown bmp info header size!");

		putstr("It's not Microsoft bmp info header size!");
		return -1;
	}

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.width = w;
	if (bmp->header.iheader.width < 0) {
		putstr("BMP width < 0!");
		return -1;
	}

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.height = w;
	if (bmp->header.iheader.height < 0) {
		putstr("BMP height < 0!");
		return -1;
	}

	mem2lehfword(bmp_addr, &hfw);
	bmp_addr += 2;
	bmp->header.iheader.planes = hfw;

	mem2lehfword(bmp_addr, &hfw);
	bmp_addr += 2;
	bmp->header.iheader.bits_per_pixel = hfw;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.compression = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.bmp_data_size = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.h_resolution = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.v_resolution = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.used_colors = w;

	mem2leword(bmp_addr, &w);
	bmp_addr += 4;
	bmp->header.iheader.important_colors = w;

	if (!bmp->header.iheader.used_colors) {
		update_lookup_table_from_bmp_file(bmp_addr, 256);
	} else {
		update_lookup_table_from_bmp_file(bmp_addr, bmp->header.iheader.used_colors);
	}

	return 0;
}

/**
 * Get the start address of raw BMP data without header.
 */
bmp_t* bld_get_splash_bmp(void)
{
	return(&G_splash_bmp);
}

void splash_logo(u32 splash_id)
{
	bmp_t *bmp;
	int rval = 0;
	u32 out_ptr = 0;
	u32 gzip_logo;
	int gzip_logo_len;

	/* Select the free memory for store the decomprssed data */
	if (logo_addr == NULL)
#if (CHIP_REV == I1)
		logo_addr = (u8 *)BSB_RAM_START;
#else
		logo_addr = (u8 *)0xc2000000;
#endif

	if (splash_id == 0) {
		gzip_logo = (u32)&__gzip_logo_1;
		gzip_logo_len = __gzip_logo_1_len;
	} else if (splash_id == 1) {
		gzip_logo = (u32)&__gzip_logo_2;
		gzip_logo_len = __gzip_logo_2_len;
	} else if (splash_id == 2) {
		gzip_logo = (u32)&__gzip_logo_3;
		gzip_logo_len = __gzip_logo_3_len;
	} else if (splash_id == 3) {
		gzip_logo = (u32)&__gzip_logo_4;
		gzip_logo_len = __gzip_logo_4_len;
	} else if (splash_id == 4) {
		gzip_logo = (u32)&__gzip_logo_5;
		gzip_logo_len = __gzip_logo_5_len;
	} else {
		putstr("Splash_Err : splash_id should be between 1 and 5!");
		putstr("\r\n");
		/* If splash id is not valid, VOUT_BG_BLUE */
		//vout_set_monitor_bg_color(chan, 0x29, 0xf0, 0x6e);
		return;
	}

	if (gzip_logo_len == 0) {
		putstr("Splash_Err : splash logo length = 0!");
		return;
	}

	out_ptr = decompress("splash",
		   (u32) gzip_logo,
		   ((u32) gzip_logo) + gzip_logo_len,
		   (u32) logo_addr,
		   (u32) &__heap_start,
		   (u32) &__heap_end);

	/** Use the golbal variable to store the BMP info for VOUT OSD used.
	 */
	bmp = bld_get_splash_bmp();

	/* Parse the BMP file header */
	rval = bld_parse_bmp(logo_addr, bmp);

	if (rval == 0) {
		/* Assign the the BMP raw data offset to the bmp buffer */
		bmp->buf = (logo_addr + bmp->header.fheader.offset);
		/* Assign the the BMP width, height, pitch from the info header */
		bmp->width = bmp->header.iheader.width;
		/* BMP pitch need be the mutilple of 4 */
		if ((bmp->width % 4) != 0)
			bmp->width = ((bmp->header.iheader.width / 4) + 1) * 4;

		bmp->height = bmp->header.iheader.height;
		bmp->pitch  = bmp->width;
		bmp->size   = bmp->header.iheader.bmp_data_size;
	} else {
		putstr("Splash_Err : Parsing BMP header!");
		putstr("\r\n");
		return;
	}
}

#endif
