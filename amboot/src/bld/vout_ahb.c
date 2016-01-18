/**
 * system/src/bld/vout.c
 *
 * History:
 *    2008/02/24 - [E-John Lien] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <ambhw.h>
#include <bldfunc.h>
#include <vout.h>

#if (VOUT_INSTANCES == 1)
#define check_channel_status(id) { K_ASSERT(id < 1); }
#else
#define check_channel_status(id)
#endif


/**************************************************************************
 *  A1, A2, A3, A5                                                        *
 **************************************************************************/
#if (VOUT_DISPLAY_SECTIONS == 1)

#if (VOUT_INSTANCES == 2)

typedef struct vd_ctrl_s {
	u32 vid_format		:  4;	/* [3:0]   output resolution
						   ignored win mode=00 and 01)*/
	u32 disable_dclk_out    :  1;   /* [4:4] dclk out */
	u32 use_2_clks_hdmi     :  1;   /* [5:5] This bit must be set to 1
					   when outputting 480I60 or 576I50
					   resolutions from the HDMI module */
	u32 reserved1		:  5;	/* [10:6] reserved */
	u32 hdmi_mode_sel	:  2;	/* [12:11] HDMI mode selection */
	u32 reserved2		:  2;	/* [14:13] reserved */
	u32 vspol		:  1;	/* [15:15] 0: Vsync active low
					           1: Vsync active high */
	u32 hspol		:  1; 	/* [16:16] 0: Hsync active low
						   1: Hsync active high */
	u32 vvldpol		:  1;	/* [17:17] 0: VD_VVLD active high
						   1: VD_VVLD active low */
	u32 hvldpol		:  1;	/* [18:18] 0: VD_HVLD active high
						   1: VD_HVLD active low */
	u32 dpol		:  1;	/* [19:19] 0: data valid on rising edge
					           1: data valid on falling
						      edge */
	u32 sync_to_vin		:  1; 	/* [20:20] 0: normal
		                                   1: wait for vin sync */
	u32 interlace		:  1;	/* [21:21] 0: progressive mode
					           1: interlace mode */
	u32 sync_on_all		:  1;	/* [22:22] 0: HD sync on luma only
						   1: HD sync on all channels */
	u32 vo_clk		:  1;	/* [23:23] output non-divided clock if
					           divided clock is used */
	u32 input_select	:  1;	/* [24:24] read-only, this bit is
					   	   driven by ORC */
	u32 reserved3		:  1;	/* [25:25] reserved */
	u32 vid_mode		:  3;	/* [28:26] output mode selection */
	u32 dve_reset  		:  1;   /* [29:29] dve reset control */
	u32 state		:  2;	/* [31:30] 1x: reset; 01: idle;
						   00: run */
} vd_ctrl_t;

#endif

/**************************************************************************
 *  A2S, A2M  								  *
 **************************************************************************/

#if (VOUT_INSTANCES == 1) && (VOUT_SUPPORT_ONCHIP_HDMI == 1)

typedef struct vd_ctrl_s {
	u32 vid_format		:  4;	/* [3:0]   output resolution
						   ignored win mode=00 and 01)*/
	u32 disable_dclk_out    :  1;   /* [4:4] dclk out */
	u32 use_2_clks_hdmi     :  1;   /* [5:5] This bit must be set to 1
					   when outputting 480I60 or 576I50
					   resolutions from the HDMI module */
	u32 reserved1		:  5;	/* [10:6] reserved */
	u32 hdmi_mode_sel	:  2;	/* [12:11] HDMI mode selection */
	u32 reserved2		:  2;	/* [14:13] reserved */
	u32 vspol		:  1;	/* [15:15] 0: Vsync active low
					           1: Vsync active high */
	u32 hspol		:  1; 	/* [16:16] 0: Hsync active low
						   1: Hsync active high */
	u32 vvldpol		:  1;	/* [17:17] 0: VD_VVLD active high
						   1: VD_VVLD active low */
	u32 hvldpol		:  1;	/* [18:18] 0: VD_HVLD active high
						   1: VD_HVLD active low */
	u32 dpol		:  1;	/* [19:19] 0: data valid on rising edge
					           1: data valid on falling
						      edge */
	u32 sync_to_vin		:  1; 	/* [20:20] 0: normal
		                                   1: wait for vin sync */
	u32 interlace		:  1;	/* [21:21] 0: progressive mode
					           1: interlace mode */
	u32 sync_on_all		:  1;	/* [22:22] 0: HD sync on luma only
						   1: HD sync on all channels */
	u32 vo_clk		:  1;	/* [23:23] output non-divided clock if
					           divided clock is used */
	u32 input_select	:  1;	/* [24:24] read-only, this bit is
					   	   driven by ORC */
	u32 dout_8b_select	:  1;	/* [25:25] reserved */
	u32 vid_mode		:  3;	/* [28:26] output mode selection */
	u32 dve_reset  		:  1;   /* [29:29] dve reset control */
	u32 state		:  2;	/* [31:30] 1x: reset; 01: idle;
						   00: run */
} vd_ctrl_t;

#endif

/**************************************************************************
 *  A2  								  *
 **************************************************************************/
#if (VOUT_INSTANCES == 1) && (VOUT_SUPPORT_ONCHIP_HDMI == 0)

typedef struct vd_ctrl_s {

#if	(VOUT_SUPPORT_SMEM_PREVIEW == 1)

	u32 bt656mode		:  3;	/* [2:0] 000: disable */
					/*       others: 480I/P-1080I */
	u32 reserved1	        :  10;	/* [12:3] reserved */
#else
        u32 reserved1	        :  13;	/* [12:0] reserved */
#endif

	u32 hvldpol		:  1;	/* [13:13] 0: VD_HVLD active high
						   1: VD_HVLD active low */
	u32 vvldpol		:  1;	/* [14;14] 0: VD_VVLD active high
						   1: VD_VVLD active low */
	u32 dpol		:  1;	/* [15:15] 0: data valid on rising edge
					           1: data valid on falling
						      edge */
	u32 vspol		:  1;	/* [16:16] 0: Vsync active low
					           1: Vsync active high */
	u32 hspol		:  1; 	/* [17:17] 0: Hsync active low
						   1: Hsync active high */
	u32 adoutsel		:  1;	/* [18:18] 0: digital output
					           1: analog output */
	u32 reserved2		:  1;	/* [19:19] 0: reserved2 */
	u32 interlace		:  1;	/* [20:20] 0: progressive mode
					           1: interlace mode */
#if	(VOUT_SUPPORT_SMEM_PREVIEW == 1)
	u32 sync_to_vin		:  1; 	/* [21:21] 0: normal
		                                   1: wait for vin sync */
#else
        u32 reserved0		:  1; 	/* [21:21] resevered */
#endif

	u32 doutsel		:  1; 	/* [22:22] 0: digital RGB or
						      656 mode (A2)
					           1: digital CbY,CrY mode */
	u32 doutclamp		:  1; 	/* [23:23] 0: clamping disable
					           1: clamp digital YCbCr */
	u32 hdmodesel		:  2;	/* [25:24] 00: 480p; 01:576p;
					           10: 720p; 11: 1080i */
	u32 sdaout		:  1;	/* [26:26] 0: HD analog output
						      (use HD sync and VBI)
						   1: SD analog output
						      (use dve)
					   Ignore if adoutsel=0 */
	u32 dve_reset  		:  1;   /* [27:27] dve reset control */
	u32 sync_on_all		:  1;	/* [28:28] 0: HD sync on luma only
						   1: HD sync on all channels */
	u32 vo_clk		:  1;	/* [29:29] output non-divided clock if
					           divided clock is used */
	u32 state		:  2;	/* [31:30] 1x: reset; 01: idle;
						   00: run */
} vd_ctrl_t;

#endif

typedef union {
	vd_ctrl_t s;
	u32 w;
} vd_control_t;

typedef union {
	struct {
	u32 seqo		:  3;	/* [2:0] Output RGB color sequence for
					        odd line */

/** VOUT RGB sequences of odd and even fields */
#define VO_SEQ_R0_G1_B2		0   	/* 0 : R0 > G1 > B2 */
#define VO_SEQ_R0_B1_G2         1   	/* 1 : R0 > B1 > G2 */
#define VO_SEQ_G0_R1_B2         2   	/* 2 : G0 > R1 > B2 */
#define VO_SEQ_G0_B1_R2         3   	/* 3 : G0 > B1 > R2 */
#define VO_SEQ_B0_R1_G2         4   	/* 4 : B0 > R1 > G2 */
#define VO_SEQ_B0_G1_R2         5   	/* 5 : B0 > G1 > R2 */

	u32 reserved1		:  1;	/* [3:3] */
	u32 seqe		:  3;	/* [6:4] Output RGB color sequence for
					         even line */
	u32 reserved2		:  1;	/* [7:7] */
	u32 dummyclk		:  1;	/* [8:8] 0: no dummy dclk between pixel
					         1: insert one dummy dclk after
					           outputting one pixel	*/
#define LCD_NO_DUMMY_DCLK		0
#define LCD_ONE_DUMMY_DCLK_INSERT	1

	u32 mode		:  2;	/* [10:9] 0: single color per dot
					          1: 3 color per dot
					          2: (5:6:5) mode */
/** VOUT output color modes: 3 and 4 are reserved */
#define LCD_MODE_1COLOR_PER_DOT         0
#define	LCD_MODE_3COLORS_PER_DOT        1
#define	LCD_MODE_RGB565                 2
#define	LCD_MODE_3COLORS_DUMMY_PER_DOT  5  /* 3 color per dot + dummy */

	u32 reverse		:  1;	/* [11] reverse video output
						vertically and horizontally */
#define LCD_REVERSE_DIS         0
#define LCD_REVERSE_ENA         1

	u32 bitpatwidth0	:  7;	/* [18:12] num of left most bits in
					           LCD_DclkBitPat0 to use */
	u32 bitpatwidth1	:  7;	/* [25:19] num of left most bits in
					           LCD_DclkBitPat1 to use */
	u32 bitpatenb		:  1;	/* [26] 0: use dclksrc as dclk
					        1: divide dclksrc by bit
					           pattern to generate dclk */
#define LCD_BIT_PAT_DIS         0
#define LCD_BIT_PAT_ENA         1

	u32 bitpatset		:  1;	/* [27] 0: use LCD_DclkPattern[0-3]
					        1: use LCD_DclkPattern[4-7] */
#define LCD_BIT_PAT_SET0_3      0
#define LCD_BIT_PAT_SET4_7	1
	u32 reserved		:  4;	/* [31:28] */
	} s;
	u32 w;
} lcd_control_t;

typedef struct {
	int length;             /* Pattern length in bits */
	int enable;             /* Pattern enable/disable */
	int set;                /* Pattern set */
	u32 pattern3;           /* MSB */
	u32 pattern2;
	u32 pattern1;
	u32 pattern0;           /* LSB */
} lcd_clk_pattern_t;

#endif	/* #if (VOUT_DISPLAY_SECTIONS == 1)*/


typedef struct vout_obj_s {
	u32 base;       		/* Base address of the controller */
#if (VOUT_DISPLAY_SECTIONS == 1)
	/* Shadow buffers */
 	lcd_control_t	        lcd;	/* Cursor control info */
	vd_control_t	        ctrl;	/* VD control register */
#endif
	osd_t			osd;
} vout_obj_t;

static __ARMCC_ALIGN(4) vout_obj_t G_vout[2] __GNU_ALIGN(4);
//static __ARMCC_ALIGN(4) u8 G_osd_buf[480*1280] __GNU_ALIGN(4);


/**
 * Initial VOUT object
 */
static vout_obj_t* vout_init(u8 ch)
{
#if (VOUT_DISPLAY_SECTIONS == 1)
	int chan = 0;
	/* Channel 0 */
	G_vout[chan].ctrl.w	= 0x40000000;
	G_vout[chan].base 	= VOUT_BASE;
//	G_vout[chan].osd.buf	= G_osd_buf;
	G_vout[chan].osd.buf	= (void*)0xc2a00000;
#if (VOUT_INSTANCES == 2)
        /* Channel 1 */
        chan = 1;
	G_vout[chan].ctrl.w	= 0x40000000;
	G_vout[chan].base 	= VOUT2_BASE;
//	G_vout[chan].osd.buf	= G_osd_buf;
	G_vout[chan].osd.buf	= (void*)0xc2a00000;
#endif

#endif
	if (ch == 0)
		return (&G_vout[0]);
	else
		return (&G_vout[1]);
}

/**
 * Reset VOUT to the initial configuration and idle state
 */
static int vout_reset(int chan)
{
        u32     polling_loop = 2000;  /* Check hardware existence by 2k times */
#if (VOUT_DISPLAY_SECTIONS == 1)
	u32     reg_val;
	u32 	addr;
#endif

	check_channel_status(chan);

#if (VOUT_DISPLAY_SECTIONS == 1)
	/* reset vout module and wait for idle start */
	addr 	= G_vout[chan].base + VOUT_CTL_CONTROL_OFFSET;

	writel(addr, 0x80000000);
	reg_val = (readl(addr) & 0x40000000);
	while (( reg_val != 0x40000000) && (polling_loop > 0)) {
		reg_val = (readl(addr) & 0x40000000);
		polling_loop --;
	}

	/* Disable VOUT[chan] clock and will enable it at freerun */
	writel(addr, (readl(addr) | 0x00000010));
#endif

	if (polling_loop == 0) {
		/* VOUT is not ready */
		putstr("VOUT ");
		puthex((u32) &chan);
		putstr("init failed");
		return -1;
	}

#if (VOUT_DISPLAY_SECTIONS == 1)
	G_vout[chan].ctrl.w     	= 0x40000000;
	G_vout[chan].lcd.w         	= 0;
#endif
	return 0;
}

/**
 * This function is to set VOUT module start running
 */
static void vout_set_freerun(int chan)
{
#if (VOUT_INSTANCES == 1)
	check_channel_status(chan);

        G_vout[chan].ctrl.s.state 	= 0;
	writel(G_vout[chan].base + VOUT_CTL_CONTROL_OFFSET,
	       G_vout[chan].ctrl.w);
#endif

#if (VOUT_DISPLAY_SECTIONS == 1) && (VOUT_INSTANCES == 2)
	G_vout[chan].ctrl.s.state 	= 0;
	writel(G_vout[chan].base + VOUT_CTL_CONTROL_OFFSET,
	       G_vout[chan].ctrl.w);
#endif
}

/**
 * This function is to config VOUT control register
 */
void vout_config_info(int chan, lcd_dev_vout_cfg_t *cfg)
{
	vd_ctrl_t *vout_info;
	vout_info = (vd_ctrl_t *) &G_vout[chan].ctrl.s;

        /* Control parameter settings */
#if     (VOUT_DISPLAY_SECTIONS == 1) && (VOUT_INSTANCES == 2)	/* A3/A5 */
	G_vout[chan].ctrl.s.hspol = cfg->hs_polarity;
	G_vout[chan].ctrl.s.vspol = cfg->vs_polarity;
	G_vout[chan].ctrl.s.dpol = cfg->data_poliarity;
	G_vout[chan].ctrl.s.hvldpol = 1;
	G_vout[chan].ctrl.s.vvldpol = 1;

	if (cfg->color_space == CS_RGB) {	/* RGB color space */
		if (cfg->lcd_sync == INPUT_FORMAT_601) {	/* 601 */
			/* Initial LCD */
			G_vout[chan].ctrl.s.vid_mode = 0; /* 0:RGB digital */
			if (cfg->lcd_display == LCD_PROG_DISPLAY) {
				/* 0:VD_PROGRESSIVE */
				G_vout[chan].ctrl.s.interlace = 0;
			} else {
				/* 1:VD_INTERLACE */
				G_vout[chan].ctrl.s.interlace = 1;
			}
		} else {
			putstr("Not supported");
		}
	} else {				/* YUV color space */
		G_vout[chan].ctrl.s.interlace = 1;
		if (cfg->lcd_sync == INPUT_FORMAT_656)
			/* 2:YCbCr656 digital */
			G_vout[chan].ctrl.s.vid_mode = 0x02;
		else
			/* 1:YCbCr601 digital */
			G_vout[chan].ctrl.s.vid_mode = 0x01;
		if (cfg->lcd_display == LCD_NTSC_DISPLAY) {
			/* Initial NTSC 480I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				/* 0x0:480I60 */
				G_vout[chan].ctrl.s.vid_format = 0x00;
			else
				/* 0x0:480I60 */
				G_vout[chan].ctrl.s.vid_format = 0x00;
		} else if (cfg->lcd_display == LCD_PAL_DISPLAY) {
			/* Initial NTSC 576I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				/* 0x0:576I60 */
				G_vout[chan].ctrl.s.vid_format = 0x02;
			else
				/* 0x0:576I60 */
				G_vout[chan].ctrl.s.vid_format = 0x02;
		}
	}
#endif

#if     (VOUT_DISPLAY_SECTIONS == 1) && (VOUT_INSTANCES == 1)
	check_channel_status(chan);

#if (VOUT_SUPPORT_ONCHIP_HDMI == 0) 	/* A1, A2*/
	G_vout[chan].ctrl.s.doutclamp	= 1;
	G_vout[chan].ctrl.s.hspol	= cfg->hs_polarity;
	G_vout[chan].ctrl.s.vspol	= cfg->vs_polarity;
	G_vout[chan].ctrl.s.dpol	= cfg->data_poliarity;
	G_vout[chan].ctrl.s.hvldpol	= 0;
	G_vout[chan].ctrl.s.vvldpol	= 0;

	if (cfg->color_space == CS_RGB) {	/* RGB color space */
		if (cfg->lcd_sync == INPUT_FORMAT_601) {	/* 601 */
			/* Initial LCD */
			G_vout[chan].ctrl.s.doutsel = 0; /* 0:DIGITAL_RGB */
			if (cfg->lcd_display == LCD_PROG_DISPLAY) {
			        G_vout[chan].ctrl.s.interlace = 0x00; /* 0:VD_PROGRESSIVE */
				G_vout[chan].ctrl.s.hdmodesel = 0x00; /* 0x0:HDMODE480P */

			} else {
			        G_vout[chan].ctrl.s.interlace = 0x01; /* 0:VD_PROGRESSIVE */
				G_vout[chan].ctrl.s.hdmodesel = 0x01; /* 0x0:HDMODE480P */
			}
		} else {
			putstr("Not supported"); 
		}
	} else {				/* YUV color space */
	        G_vout[chan].ctrl.s.interlace = 0x01;
		if (cfg->lcd_sync == INPUT_FORMAT_656)
			G_vout[chan].ctrl.s.doutsel = 0;
		else
			G_vout[chan].ctrl.s.doutsel = 1; /* 1:DIGITAL_CBY_CRY */

		if (cfg->lcd_display == LCD_NTSC_DISPLAY) {
			/* Initial NTSC 480I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				G_vout[chan].ctrl.s.bt656mode = 0x1; /* 0x1:656MODE_480I*/
			else
				G_vout[chan].ctrl.s.bt656mode = 0x0; /* 0x0:656MODE_DIS*/
		} else if (cfg->lcd_display == LCD_PAL_DISPLAY) {
			/* Initial NTSC 576I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				G_vout[chan].ctrl.s.bt656mode = 0x03; /* 0x03:656MODE_576I*/
			else
				G_vout[chan].ctrl.s.bt656mode = 0x0; /* 0x0:656MODE_DIS*/
		}
	}
#endif

#if (VOUT_SUPPORT_ONCHIP_HDMI == 1)	/* A2S, A2M */
	G_vout[chan].ctrl.s.hspol   = cfg->hs_polarity;
	G_vout[chan].ctrl.s.vspol   = cfg->vs_polarity;
	G_vout[chan].ctrl.s.dpol    = cfg->data_poliarity;
	G_vout[chan].ctrl.s.hvldpol = 0;
	G_vout[chan].ctrl.s.vvldpol = 0;

	if (cfg->color_space == CS_RGB) {	/* RGB color space */
		if (cfg->lcd_sync == INPUT_FORMAT_601) {	/* 601 */
			/* Initial LCD */
			G_vout[chan].ctrl.s.vid_mode = 0; /* 0:RGB digital */
			if (cfg->lcd_display == LCD_PROG_DISPLAY) {
				G_vout[chan].ctrl.s.interlace = 0; /* 0:VD_PROGRESSIVE */
			} else {
				G_vout[chan].ctrl.s.interlace = 1; /* 1:VD_INTERLACE */
			}
		} else {
			putstr("Not supported"); 
		}
	} else {				/* YUV color space */
		vout_info->interlace   	= 1;
		if (cfg->lcd_sync == INPUT_FORMAT_656)
			G_vout[chan].ctrl.s.vid_mode = 0x02; /* 2:YCbCr656 digital */
		else
			G_vout[chan].ctrl.s.vid_mode = 0x01; /* 1:YCbCr601 digital */
		if (cfg->lcd_display == LCD_NTSC_DISPLAY) {
			/* Initial NTSC 480I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				G_vout[chan].ctrl.s.vid_format = 0x0; /* 0x0:480I60 */
			else
				G_vout[chan].ctrl.s.vid_format = 0; /* 0x0:480I60 */
		} else if (cfg->lcd_display == LCD_PAL_DISPLAY) {
			/* Initial NTSC 576I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				G_vout[chan].ctrl.s.vid_format = 0x02; /* 0x0:576I60 */
			else
				G_vout[chan].ctrl.s.vid_format = 0x02; /* 0x0:576I60 */
		}
	}
#endif

#endif

}

/**
 * This function is to set backgorund color
 */
void vout_set_monitor_bg_color(int chan, u8 y, u8 cb, u8 cr)
{
/*
			y     cb    cr
	VOUT_BG_RED 	0x51, 0x5a, 0xf0
	VOUT_BG_MAGENTA 0x6a, 0xca, 0xde
	VOUT_BG_YELLOW 	0xd2, 0x10, 0x92
	VOUT_BG_GREEN 	0x91, 0x36, 0x22
	VOUT_BG_BLUE 	0x29, 0xf0, 0x6e
	VOUT_BG_CYAN 	0xaa, 0xa6, 0x10
	VOUT_BG_WHITE 	0xeb, 0x80, 0x80
*/
#if     (VOUT_DISPLAY_SECTIONS == 1)
        check_channel_status(chan);

	u32 bg = 0;

	bg = ((y << 16) | (cb << 8) | (cr));
	writel(G_vout[chan].base + VOUT_CTL_BG_OFFSET, bg);
#endif
}

/**
 * This function is to set start and end position for active window
 */
static void vout_set_active_win(int chan, u16 start_x, u16 end_x,
			 u16 start_y, u16 end_y)
{
#if     (VOUT_DISPLAY_SECTIONS == 1)
        check_channel_status(chan);

	u32 act_start = 0, act_end = 0;

	act_start = ((start_x << 16) | (start_y));
	act_end	  = ((end_x << 16) | (end_y));

	writel(G_vout[chan].base + VOUT_CTL_ACTSTART_OFFSET, act_start);
	writel(G_vout[chan].base + VOUT_CTL_ACTEND_OFFSET, act_end);
#endif
}

#if (VOUT_DISPLAY_SECTIONS == 1)
/**
 * This function is to set Vsync/Hsync polarity
 * 0 = active low & 1 = active high
 */
static void vout_set_sync_polarity(int chan, u8 polarity)
{
        check_channel_status(chan);

	G_vout[chan].ctrl.s.hspol = polarity;
	G_vout[chan].ctrl.s.vspol = polarity;
	writel(G_vout[chan].base + VOUT_CTL_CONTROL_OFFSET,
	       G_vout[chan].ctrl.w);

}
#endif

#if (VOUT_DISPLAY_SECTIONS == 1)
/**
 * This function is to set Hsync and Vsync position
 */
static void vout_set_hv_sync(int chan, int mode, u32 sync_start, u32 sync_end)
{
	/*
	sync_start = sync start position
	sync_end = sync end position
	mode 0 = Hsync
	mode 1 = Vsync top
	mode 2 = Vsync bottom
	*/
        check_channel_status(chan);
	u32 hv_sync = 0;

	switch (mode) {
		case VO_MODE_HSYNC:
			hv_sync = (sync_end << 16) | sync_start;
			writel(G_vout[chan].base + VOUT_CTL_HSYNC_OFFSET,
			       hv_sync);
			break;
		case VO_MODE_VSYNC_TOP:
			hv_sync = (sync_end << 16) | sync_start;
			writel(G_vout[chan].base + VOUT_CTL_VSYNC0_OFFSET,
			       hv_sync);
			break;
		case VO_MODE_VSYNC_BOT:
			hv_sync = (sync_end << 16) | sync_start;
			writel(G_vout[chan].base + VOUT_CTL_VSYNC1_OFFSET,
			       hv_sync);
			break;
		default:
			putstr("Unknown mode:");
			puthex((u32) &mode);
			break;
	}
}
#endif

/**
 * This function is to set sync cycles for display format.
 * h  = # of cycles per horizontal line
 * v = # of hsync in one frame
 */
static void vout_set_hv(int chan, u16 h, u16 v, u16 v1)
{
#if (VOUT_DISPLAY_SECTIONS == 1)
        check_channel_status(chan);
	u32 hv;

	hv = (((h - 1) << 16) | (v - 1));
	writel(G_vout[chan].base + VOUT_CTL_HV_OFFSET, hv);
	writel(G_vout[chan].base + VOUT_CTL_V1_OFFSET, (v1 - 1));
#endif
}

/**
 * This function is to configure and enable/disable the VOUT data pattern to
 * LCD panel
 */
#if (VOUT_DISPLAY_SECTIONS == 1)
static int vout_config_dclk_pattern(int chan, lcd_clk_pattern_t *pattern)
{
	int offset;
	lcd_clk_pattern_t *bp = (lcd_clk_pattern_t *) pattern;

	check_channel_status(chan);

	if (bp->enable == LCD_BIT_PAT_ENA) {

	        if (bp->set == LCD_BIT_PAT_SET0_3) {
	                /* how many bits to be used to use pattern #0 */
	                G_vout[chan].lcd.s.bitpatwidth0    = bp->length - 1;
	                offset = 0;
		} else if (bp->set == LCD_BIT_PAT_SET4_7) {
   			/* how many bits to be used to use pattern #1 */
		        G_vout[chan].lcd.s.bitpatwidth1    = bp->length - 1;
		        offset = VOUT_LCD_CLKPATN4_OFFSET -
				 VOUT_LCD_CLKPATN0_OFFSET;
	        } else {
		        putstr("The selected pattern does not exist");
			return -1;
		}

                G_vout[chan].lcd.s.bitpatset 	= bp->set;
		writel(G_vout[chan].base + VOUT_LCD_CLKPATN0_OFFSET + offset,
							bp->pattern3);
		writel(G_vout[chan].base + VOUT_LCD_CLKPATN1_OFFSET + offset,
							bp->pattern2);
		writel(G_vout[chan].base + VOUT_LCD_CLKPATN2_OFFSET + offset,
							bp->pattern1);
		writel(G_vout[chan].base + VOUT_LCD_CLKPATN3_OFFSET + offset,
							bp->pattern0);
	        /* enable divider */
  		G_vout[chan].lcd.s.bitpatenb	= LCD_BIT_PAT_ENA;

	} else if (bp->enable == LCD_BIT_PAT_DIS) {
	        G_vout[chan].lcd.s.bitpatenb	= LCD_BIT_PAT_DIS;
	}

	writel(G_vout[chan].base + VOUT_LCD_CTRL_OFFSET, G_vout[chan].lcd.w);

	return 0;
}
#endif

/**
 * This function is to divide 27 MHz to 27/(2 ^ clk_div)MHz
 */
static void vout_set_fractional_clock(int chan, u8 clk_div)
{
#if (VOUT_DISPLAY_SECTIONS == 1)
        lcd_clk_pattern_t bp;

        check_channel_status(chan);

	memzero(&bp, sizeof(lcd_clk_pattern_t));

	if (clk_div == 1) { /* 13.5MHz */
	        bp.length	= 2;
		bp.enable	= LCD_BIT_PAT_ENA;      /* enable divider */
		bp.set		= LCD_BIT_PAT_SET0_3;
		bp.pattern3	= 0x00000000;           /* MSB */
		bp.pattern2	= 0x00000000;
		bp.pattern1	= 0x00000000;
		bp.pattern0	= 0x00000001;           /* LSB */

	} else if (clk_div >= 2) { /* 6.75MHz */
	        bp.length	= 4;
		bp.enable	= LCD_BIT_PAT_ENA;      /* enable divider */
		bp.set		= LCD_BIT_PAT_SET0_3;
		bp.pattern3	= 0x00000000;           /* MSB */
		bp.pattern2	= 0x00000000;
		bp.pattern1	= 0x00000000;
		bp.pattern0	= 0x00000003;           /* LSB */

	} else { /* 27MHz */
		bp.enable	= LCD_BIT_PAT_DIS;
	}
#endif
        vout_config_dclk_pattern(chan, &bp);
}

/**
 * Set LCD information
 */
static void vout_set_lcd_mode(int chan, u8 clock, u8 out_color_mode,
			 u8 rgbtop, u8 rgbbot, u8 reverse)
{
        check_channel_status(chan);

	/*
	clock = 0 : 27.0M
	clock = 1 : 13.5M
	clock = 2 :  6.75M*/

	vout_set_fractional_clock(chan, clock);

	/*
	out_color_mode = 0 : signle color per dot
	out_color_mode = 1 : 3 color per dot
	out_color_mode = 2 : rgb565
	out_color_mode = 3/4 : reserved
	out_color_mode = 5 : 3 color per dot + dummy
	rgbodd = rgbeven >>
		0 : R0 > G1 > B2
		1 : R0 > B1 > G2
		2 : G0 > R1 > B2
		3 : G0 > B1 > R2
		4 : B0 > R1 > G2
		5 : B0 > G1 > R2
	*/
#if (VOUT_DISPLAY_SECTIONS == 1)

	G_vout[chan].lcd.s.mode     	= (out_color_mode & 0x3);
	G_vout[chan].lcd.s.dummyclk 	= (out_color_mode >> 2) & 0x1;

	G_vout[chan].lcd.s.seqe     	= rgbtop;
	G_vout[chan].lcd.s.seqo     	= rgbbot;
	G_vout[chan].lcd.s.reverse    	= reverse;
	writel(G_vout[chan].base + VOUT_LCD_CTRL_OFFSET, G_vout[chan].lcd.w);
#endif
}

/**
 * Calculate the entry of CLUT / palette
 */
static u32 cal_clut_entry(u8 *ptr, u8 blend_factor, u8 yuv)
{
        u32 table;
	u8  tab[3];

        if (!yuv){
                tab[0] = 0.299 * ptr[0] + 0.587 * ptr[1] + 0.114 * ptr[2];

		tab[1] = (ptr[2] - tab[0]) * 0.565 + 128;
		tab[1] = CLIP(tab[1], 240, 16);

		tab[2] = (ptr[0] - tab[0]) * 0.713 + 128;
		tab[2] = CLIP(tab[2], 240, 16);

		tab[0] = CLIP(tab[0], 235, 16);

		table  = ((blend_factor & 0xf) << 28) +
		         ((tab[0] & 0xff) << 16) +
			 ((tab[1] & 0xff) << 8) +
			  (tab[2] & 0xff);
        } else {
		table = ((blend_factor & 0xf) << 28) +
		        ((ptr[0] & 0xff) << 16) +
			((ptr[1] & 0xff) << 8) +
			 (ptr[2] & 0xff);
	}

        return (table);
}

/**
* This function is to set osd color table
*/
static void set_osd_clut(u8 chan, u8 *rgbtable, u8 *blend_table, u8 yuv)
{
	u32 addr, table;
	int j;
	u32 reg;
	u8 *ptr = NULL;

	ptr 	= &rgbtable[0];

	addr = G_vout[chan].base + VOUT_CTL_CLUT_OFFSET;

	for (j = 0; j < 256 ; j++) {
                table 	 = cal_clut_entry(ptr, blend_table[j], yuv);
#if 1
                /* Wait for CLUT ready */
                do {
			reg = readl(G_vout[chan].base + VOUT_CTL_STATUS_OFFSET);
		} while ((reg & 0x20) == 0);
#endif
		writel(addr, table);
		ptr	+= 3;
		addr 	+= 0x4;
	}
}

/**
 * Initial OSD
 */
u32 osd_set(u8 chan, int resolution, lcd_dev_vout_cfg_t *cfg)
{
	u32 ctrl_val;
	u32 start_val;
	u32 end_val;

#if 0
	switch (resolution) {
		case VO_RGB_360_240:
		case VO_480I:
			ctrl_val = 0x00001f00;
			start_val = 0x00ee0012;	/* 0xee  =  238, 0x12  =  18 */
			end_val = 0x068d0101;	/* 0x68d = 1677, 0x101 = 257 */
			break;
		case VO_576I:
		case VO_RGB_360_288:
			ctrl_val = 0x00001f00;
			start_val = 0x01080016;
			end_val	= 0x06a70135;
			break;
		case VO_RGB_640_480:
		case VO_RGB_720_240:
		case VO_RGB_960_240:
		case VO_RGB_480_240:
		case VO_RGB_320_240:
		case VO_RGB_320_288:
			putstr("Not implemented!"; 
			return -1;
			break;
		default:
			ctrl_val = 0x00001f00;
			start_val = 0x00ee0012;
			end_val = 0x068d0101;
			break;
	}
#else
	ctrl_val  = 0x00001f00;
	/* start_col | start_row */
	start_val = (((cfg->act_start_col_top & 0x0fff) << 16) |
		     (cfg->act_start_row_top & 0x07ff));
	/* end_col | end_row */
	end_val   = (((cfg->act_end_col_top & 0x0fff) << 16) |
		     (cfg->act_end_row_top & 0x07ff));
#endif

	/* 0x6000845c */
	writel((G_vout[chan].base + VOUT_CTL_OSD0_CTRL_OFFSET), ctrl_val);
	/* 0x60008460 */
	writel((G_vout[chan].base + VOUT_CTL_OSD0_START_OFFSET),start_val);
	/* 0x60008464 */
	writel((G_vout[chan].base + VOUT_CTL_OSD0_END_OFFSET), end_val);

	return 0;
}

/**
 * OSD open
 */
int osd_init(u32 width, u32 height, u16 resolution, osd_t *osdobj)
{
	osdobj->width = width;
	osdobj->height = height;
	osdobj->resolution = resolution;

	if (osdobj->height == 720) {
		osdobj->pitch = 1280;	/* for 720p */
	} else {
		osdobj->pitch = 1024;	/* for 480i, 480p, 1080i */
	}
	return 0;
}

/**
 * Congfigure VOUT CSC (Color Space Conversion)
 *
 */
void vout_set_csc(int chan, u8 csc_mode, u8 csc_clamp)
{
#if (VOUT_SUPPORT_DIGITAL_CSC == 1)
	int i;
	u32 offset;

        // !!! this function is to guide the next programmer
	//     how to program the CSC. And this is only applied
	//     to 8 bit mode (not include analog HD and hdmi mode).
	//     The next programer should be able to complete it.]
	//     ps. the following matrix coef is designed by Wilson K.
	u32 color_tbl[6][6] = {
        	/* VO_CSC_YUVSD2YUVHD: YUV601 -> YUV709 */
                { 0x005103a8, 0x1f950008, 0x1fbb04b0,
			0x00220009, 0x7ff603d5, 0x00017ff4 },
        	/* VO_CSC_YUVSD2YUVSD: YUV601 -> YUV601 */
        	{ 0x00000400, 0x00000000, 0x00000400,
			0x00000000, 0x00000400, 0x00000000 },
		/* VO_CSC_YUVSD2RGB: YUV601 -> RGB */
		{ 0x1e6f04a8, 0x04a81cbf, 0x1fff0812,
			0x1ffe04a8, 0x00880662, 0x7f217eeb },
                /* VO_CSC_YUVHD2YUVSD: YUV709 -> YUV601 */
                { 0x1fb60458, 0x00631ff2, 0x003c0362,
			0x1fe31ff2, 0x000a042b,	0x7fff000b },
                /* VO_CSC_YUVHD2YUVHD: YUV709 -> YUV709 */
                { 0x00000400, 0x00000000, 0x00000400,
			0x00000000, 0x00000400, 0x00000000 },
                /* VO_CSC_YUVHD2RGB: YUV709 -> RGB */
                { 0x1f2604a8, 0x04a81ddd, 0x00010874,
			0x1fff04a8, 0x004d072b, 0x7f087edf } };

	u32 clamp_tbl[8][3] = {
		/* VO_DATARANGE_ANALOG_HD_FULL  */
	 	{ 0x03ff0000, 0x03ff0000, 0x03ff0000 },
		/* VO_DATARANGE_ANALOG_SD_FULL   */
  		{ 0x03ac0040, 0x03c00040, 0x03c00040 },
		/* VO_DATARANGE_DIGITAL_HD_FULL  */
	 	{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },
		/* VO_DATARANGE_DIGITAL_SD_FULL  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },
		/* VO_DATARANGE_ANALOG_HD_CLAMP  */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },
		/* VO_DATARANGE_ANALOG_SD_CLAMP  */
		{ 0x00eb0010, 0x00f00010, 0x00f00010 },
		/* VO_DATARANGE_DIGITAL_HD_CLAMP */
		{ 0x00ff0000, 0x00ff0000, 0x00ff0000 },
		/* VO_DATARANGE_DIGITAL_SD_CLAMP */
        	{ 0x00eb0010, 0x00f00010, 0x00f00010 } };

	//K_ASSERT(csc_mode <= VO_CSC_YUVHD2RGB);

#if (VOUT_DISPLAY_SECTIONS == 1)
	offset = VOUT_CTL_PRI_CSC_COEF0_OFFSET;
#endif

	for (i = 0; i < 6; i++) {
                writel(G_vout[chan].base + offset + (4 * i),
		       color_tbl[csc_mode][i]);
	}

	//K_ASSERT(csc_clamp <= VO_DATARANGE_DIGITAL_SD_CLAMP);
#if (VOUT_DISPLAY_SECTIONS == 1)
	offset  = VOUT_CTL_PRI_CSC_CLIP0_OFFSET;
#endif
	for (i = 0; i < 3; i++) {
                writel(G_vout[chan].base + offset + (4 * i),
		       clamp_tbl[csc_clamp][i]);
	}

#endif
}

/**
 * Copy BMP to OSD memory
 */
static int bmp2osd_mem(bmp_t *bmp, osd_t *osdobj, int x, int y, int top_botm_revs)
{
	int point_idx = 0;
	int i = 0, j = 0;

	if ((osdobj->buf == NULL) ||
	    (osdobj->width < 0) ||
	    (osdobj->height < 0))
		return -1;

	/* set the first line to move */
	if (top_botm_revs == 0)
		point_idx = y * osdobj->pitch + x;
	else
		point_idx = (y + bmp->height) * osdobj->pitch + x;

#if 1
	for (i = 0; i < bmp->height; i++) {
		for (j = 0; j < bmp->width; j++) {
			if ((i == 0) || (i == (osdobj->height - 1)) ||
			    (j == 0) || (j == (osdobj->width - 1))) {
				osdobj->buf[point_idx + j] =
					bmp->buf[i * bmp->width + j];
			} else {
				osdobj->buf[point_idx + j] =
					bmp->buf[i * bmp->width + j];
			}
		}
		/* Goto next line */
		if (top_botm_revs == 0)
			point_idx += osdobj->pitch;
		else
			point_idx -= osdobj->pitch;
	}
#else
	/* speed up version */
	for (i = 0; i < bmp->height; i++) {
		memcpy(&osdobj->buf[point_idx + j],
		       &bmp->buf[i * bmp->width + j], bmp->width);
		/* Goto next line */
		if (top_botm_revs == 0)
			point_idx += osdobj->pitch;
		else
			point_idx -= osdobj->pitch;
	}
#endif
	return 0;
}

/**
 * Copy Splash BMP to OSD buffer for display
 */
static int splash_bmp2osd_buf(int chan, osd_t* posd, int bot_top_res)
{
	extern bmp_t* bld_get_splash_bmp(void);

	bmp_t* splash_bmp;
	int x = 0, y = 0;
	int rval = -1;

	if (posd == NULL) {
		putstr("Splash_Err: NULL pointer of OSD buffer!");
		return -1;
	}

	/* Get the spalsh bmp address and osd buffer address */
	splash_bmp = bld_get_splash_bmp();
	if (splash_bmp == NULL) {
		putstr("Splash_Err: NULL pointer of splash bmp!");
		return -1;
	}

	/* Calculate the splash center pointer */
	x = (posd->width - splash_bmp->width) / 2;
	y = (posd->height - splash_bmp->height) / 2;

	/* Copy the BMP to the OSD buffer */
	rval = bmp2osd_mem(splash_bmp, posd, x, y, bot_top_res);
	if (rval != 0) {
		putstr("Splash_Err: copy Splash BMP to OSD error!");
		return -1;
	}

	return 0;
}

/**
 * Get the osd object pointer
 */
osd_t* get_osd_obj(int chan)
{
	return(&G_vout[chan].osd);
}

/* The parameters are setting to VOUT control for specific lcd */
int vout_config(int chan, lcd_dev_vout_cfg_t *cfg)
{
	osd_t* posd;
	int bot_top_res = 1;
	int rval = -1;
	u8 csc_mode = 0;
	u8 csc_clamp = 0;

#if (VOUT_INSTANCES == 1)
	/* One VOUT instance, use chan = 0 */
	chan = 0;
#endif
	vout_obj_t *pvout;

	pvout = vout_init(chan);

	vout_reset(chan);

	set_osd_clut(chan, bld_clut, bld_blending, 0);

	/* Hsycn polarity = Vsync polarity */
	vout_set_sync_polarity(chan, cfg->hs_polarity);

	/* Total H pclk, total V lines */
	vout_set_hv(chan, cfg->ftime_hs, cfg->ftime_vs_top, cfg->ftime_vs_bot);

	/* HD_hsync */
        vout_set_hv_sync(chan, VO_MODE_HSYNC, cfg->hs_start, cfg->hs_end);

        /* VD_vsync0 */
        vout_set_hv_sync(chan,
			 VO_MODE_VSYNC_TOP,
			 ((cfg->ftime_hs * cfg->vs_start_row_top) +
			 			cfg->vs_start_col_top),
			 ((cfg->ftime_hs * cfg->vs_end_row_top) +
			 			cfg->vs_end_col_top));

	if (cfg->lcd_display != LCD_PROG_DISPLAY) {
	        /* VD_vsync1 */
		vout_set_hv_sync(chan,
			 VO_MODE_VSYNC_BOT,
			 ((cfg->ftime_hs * cfg->vs_start_row_bot) +
			 			cfg->vs_start_col_bot),
			 ((cfg->ftime_hs * cfg->vs_end_row_bot) +
			 			cfg->vs_end_col_bot));
	}

	if (cfg->color_space == CS_RGB) {
        /* lcd control register */
		vout_set_lcd_mode(chan,
				  cfg->lcd_data_clk,
	                          cfg->lcd_color_mode,
	                          cfg->lcd_rgb_seq_top,
	                          cfg->lcd_rgb_seq_bot,
				  cfg->lcd_hscan);
	}

	/* Set activ window */
	vout_set_active_win(chan,
			    cfg->act_start_col_top,
	                    cfg->act_end_col_top,
	                    cfg->act_start_row_top,
	                    cfg->act_end_row_top);

	switch (cfg->bg_color) {
	case VOUT_BG_RED:
		vout_set_monitor_bg_color (chan, 0x51, 0x5a, 0xf0);
		break;
	case VOUT_BG_MAGENTA:
		vout_set_monitor_bg_color (chan, 0x6a, 0xca, 0xde);
		break;
	case VOUT_BG_YELLOW:
		vout_set_monitor_bg_color (chan, 0xd2, 0x10, 0x92);
		break;
	case VOUT_BG_GREEN:
		vout_set_monitor_bg_color (chan, 0x91, 0x36, 0x22);
		break;
	case VOUT_BG_BLUE:
		vout_set_monitor_bg_color (chan, 0x29, 0xf0, 0x6e);
		break;
	case VOUT_BG_CYAN:
		vout_set_monitor_bg_color (chan, 0xaa, 0xa6, 0x10);
		break;
	case VOUT_BG_WHITE:
		vout_set_monitor_bg_color (chan, 0xeb, 0x80, 0x80);
		break;
	default: /* VOUT_BG_BLACK */
		vout_set_monitor_bg_color (chan, 0x10, 0x80, 0x80);
		break;
	}

	vout_config_info(chan, cfg);

#if (VOUT_SUPPORT_DIGITAL_CSC == 1)
	if (cfg->color_space == CS_RGB) {
		csc_mode = VO_CSC_YUVSD2RGB;
		csc_clamp = VO_DATARANGE_DIGITAL_SD_FULL;
	} else if ((cfg->color_space == CS_YCBCR) ||
		   (cfg->color_space == CS_YUV)) {
		csc_mode = VO_CSC_YUVSD2YUVSD;
		csc_clamp = VO_DATARANGE_DIGITAL_SD_CLAMP;
	}
	vout_set_csc(chan, csc_mode, csc_clamp);
#endif

	osd_init(cfg->osd_width,
		 cfg->osd_height,
		 cfg->osd_resolution,
		 &pvout->osd);

	osd_set(chan, cfg->osd_resolution, cfg);

	posd = get_osd_obj(chan);
	/* Copy splash BMP to OSD buffer */
	rval = splash_bmp2osd_buf(chan, posd, bot_top_res);
	if (rval != 0) {
		putstr("Splash_Err: Copy splahs to OSD fail!");
		return -1;
	}

	vout_set_freerun(chan);

	return 0;
}

