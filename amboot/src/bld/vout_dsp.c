/**
 * system/src/bld/vout.c
 *
 * History:
 *    2008/02/24 - [E-John Lien] created file
 *    2009/10/20 - [E-John Lien] separate vout_ahb.c and vout_dsp.c
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#include <vout.h>

/**************************************************************************
 *  A5S		                                                          *
 **************************************************************************/
#if (VOUT_DIRECT_DSP_INTERFACE == 1)

#if (VOUT_SUPPORT_DIGITAL_CSC == 1)

#define VO_CSC_DIGITAL		0

/** Color Space Conversion (CSC) */
#define	VO_CSC_YUVSD2YUVHD	0	/* YUV601 -> YUV709 */
#define	VO_CSC_YUVSD2YUVSD	1	/* YUV601 -> YUV601 */
#define	VO_CSC_YUVSD2RGB	2	/* YUV601 -> RGB    */
#define	VO_CSC_YUVHD2YUVSD	3	/* YUV709 -> YUV601 */
#define	VO_CSC_YUVHD2YUVHD	4	/* YUV709 -> YUV709 */
#define	VO_CSC_YUVHD2RGB	5	/* YUV709 -> RGB    */

/** Data output range clamping */
#define	VO_DATARANGE_DIGITAL_HD_FULL	2
#define	VO_DATARANGE_DIGITAL_SD_FULL	3
#define	VO_DATARANGE_DIGITAL_HD_CLAMP	6
#define	VO_DATARANGE_DIGITAL_SD_CLAMP	7
#define	VO_DATARANGE_DIGITAL_RGB_FULL	8
#define	VO_DATARANGE_DIGITAL_RGB_CLAMP	9

#endif

typedef struct vd_ctrl_s {
	u32 fixed_format_select	:  5;	/* [4:0]   Fixed format select */
#define VD_FFS_DISABLED         0x0
#define	VD_480I60		0x1
#define	VD_480P60		0x2
#define	VD_576I50		0x3
#define	VD_576P50		0x4

       	u32 interlace		:  1;	/* [5:5] 0: progressive mode
					         1: interlace mode */
#define VD_PROGRESSIVE          0
#define VD_INTERLACE            1

	u32 reverse_mode        : 1;    /* [6:6] 0: normal
		                                   1: horizontal reverse mode */
#define VD_NORMAL          	0
#define VD_HORIZONTAL_REVERSE  	1

	u32 reserved            : 18;   /* [24:7] reserved */
        u32 vout_vout_sync	:  1; 	/* [25:25] 0: normal
		                                   1: vout-vout sync */
	u32 vin_vout_sync	:  1; 	/* [26:26] 0: normal
		                                   1: wait for vin sync */
#define SYNC_DISABLE         	0
#define SYNC_ENABLE         	1

        u32 digital_enable 	:  1;   /* [27:27] digital output enable */
        u32 analog_enable 	:  1;   /* [28:28] analog output enable */
        u32 hdmi_enable 	:  1;   /* [29:29] HDMI output enable */
#define OUTPUT_DISABLE  0
#define OUTPUT_ENABLE   1

	u32 dve_reset  		:  1;   /* [30:30] dve reset control */
	u32 sw_reset		:  1;	/* [31:31] display section reset*/
} vd_ctrl_t;

typedef union {
	vd_ctrl_t s;
	u32 w;
} vd_control_t;

typedef union {
	struct {
	u32 hdmi_field		:  1;	/* [0:0], 1 - odd, 0 - even */
	u32 analog_field	:  1;	/* [1:1] */
	u32 digital_field	:  1;	/* [2:2] */
	u32 reserved		: 24;	/* [26:3] */
	u32 hdmi_underflow	:  1;	/* [27:27] */
	u32 analog_underflow	:  1;	/* [28:28] */
	u32 digital_underflow	:  1;	/* [29:29] */
	u32 dve_reset_complete	:  1;	/* [30:30] */
	u32 sw_reset_complete	:  1;	/* [31:31] */
	} s;
	u32 w;
} vd_status_t;

typedef union {
	struct {
	u32 height		: 14;	/* [13:0]  frame height */
 	u32 reserved15		:  2;	/* [15:14] */
	u32 width		: 14;	/* [29:16] frame width */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_frame_size_field0_t;

typedef union {
	struct {
	u32 height		: 14;	/* [13:0]  frame height */
 	u32 reserved31		:  2;	/* [31:14] */
	} s;
	u32 w;
} vd_frame_size_field1_t;

typedef union {
	struct {
	u32 row			: 14;	/* [13:0]  region starts on it */
	u32 reserved15		:  2;	/* [15:14] */
	u32 col			: 14;   /* [29:16] region starts on it */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_active_region_start_t;

typedef union {
	struct {
	u32 row			: 14;	/* [13:0]  region ends on it */
	u32 reserved15		:  2;	/* [15:14] */
	u32 col			: 14;   /* [29:16] region ends on it */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_active_region_end_t;

typedef struct vd_modes_s {
	u32 dout_hd_pol 	;
	u32 dout_vd_pol 	;
	u32 dclk_output_divider	;
	u32 dclk_divider_enable ;
	u32 dclk_edge_select	;
	u32 dclk_disable    	;
	u32 bit_pattern_width	;

	u32 mipi_line_timing	;
	u32 mipi_line_count	;
	u32 mipi_frame_count	;
	u32 mipi_send_blanking	;
	u32 mipi_all_line	;
	u32 mipi_ecc_order	;

	u32 lcd_seqe		;
	u32 lcd_seqo		;
	u32 dout_mode		;
	u32 aout_hd_pol 	;
	u32 aout_vd_pol 	;
	u32 hdmi_hd_pol 	;
	u32 hdmi_vd_pol 	;
	u32 hdmi_mode		;
} vd_modes_t;

typedef union {
        vd_modes_t s;
	u32 w;
} vd_output_modes_t;

typedef struct vd_digital_modes_s {
	u32 hd_pol 		:  1;   /* [0:0] 0: asserted low
						 1: asserted high */
	u32 vd_pol 		:  1;   /* [1:1] 0: asserted low
						 1: asserted high */
#define VD_ACT_LOW              0
#define VD_ACT_HIGH             1

	u32 dclk_output_divider	:  1;   /* [2:2] 0: Output clock same as
						    clk_vo_b input,
						 1: Output clock same as
						    internally divided clock */
#define OUTPUT_CLK_VO_DISABLE    1
#define OUTPUT_CLK_VO_ENABLE     0

	u32 dclk_divider_enable :  1;   /* [3:3] digital clock divider enable.
						For display section is set as
						digital output alone */
#define VD_DCLK_DIV_ENABLE      1
#define VD_DCLK_DIV_DISABLE  	0

	u32 dclk_edge_select	:  1;	/* [4:4] 0: data valid on rising edge
					           1: data valid on falling edge */
#define DVALID_RISING           0
#define DVALID_FALLING          1

	u32 dclk_disable    	:  1;   /* [5:5] digital clock disable */
#define VD_DCLK_ENABLE		0
#define VD_DCLK_DISABLE		1

	u32 bit_pattern_width	:  7;	/* [12:6] number of valid bits in
						   clock pattern registers */
	u32 mipi_line_timing	:  1;	/* [13:13] enable sending start and end
						line packet at hsync*/
#define MIPI_EN_LINE_TIMING	1
#define MIPI_DIS_LINE_TIMING	0

	u32 mipi_line_count	:  1;	/* [14:14] set 1 to enable sending of
						line count*/
#define MIPI_EN_LINE_COUNT	1
#define MIPI_DIS_LINE_COUNT	0

	u32 mipi_frame_count	:  1;	/* [15:15] set 1 to enable sending of
						frame count*/
#define MIPI_EN_FRAME_COUNT	1
#define MIPI_DIS_FRAME_COUNT	0

	u32 mipi_send_blanking	:  1;	/* [16:16] set 1 to enable sending data
						for lines during VBLANK*/
#define MIPI_EN_SEND_BLANKING	1
#define MIPI_DIS_SEND_BLANKING	0

	u32 mipi_all_line	:  1;	/* [17:17] set 1 to enable sending LS/LE
						packets for all line during
						blanking*/
#define MIPI_EN_ALL_LINE	1
#define MIPI_DIS_ALL_LINE	0

	u32 mipi_ecc_order	:  1;	/* [18:18] set 1 to reverse byte order
						   of MIPI ECC computation
						   (DEBUG ONLY)*/
#define MIPI_EN_ECC_REVERSE	1
#define MIPI_DIS_ECC_REVERSE	0

	u32 hvld_pol		:  1;	/* [19:19] HVLD Polarity */
	u32 reserved19		:  1;	/* [20:20] Reserved */

	u32 lcd_seqe		:  3;	/* [23:21] Output RGB color sequence for
					        even lines */
	u32 lcd_seqo		:  3;	/* [26:24] Output RGB color sequence for
					        odd lines */
/** VOUT RGB sequences of odd and even fields in digital mode 0 , 1 , 2  */
#define VO_SEQ_R0_G1_B2		0   	/* 0 : R0 > G1 > B2 */
#define VO_SEQ_R0_B1_G2         1   	/* 1 : R0 > B1 > G2 */
#define VO_SEQ_G0_R1_B2         2   	/* 2 : G0 > R1 > B2 */
#define VO_SEQ_G0_B1_R2         3   	/* 3 : G0 > B1 > R2 */
#define VO_SEQ_B0_R1_G2         4   	/* 4 : B0 > R1 > G2 */
#define VO_SEQ_B0_G1_R2         5   	/* 5 : B0 > G1 > R2 */
/*in digital output mode 8 or 11*/
#define VO_SEQ_R0_G1_R2_G3	0   	/* 5 : R0 > G1 > R2 > G3 */
#define VO_SEQ_G0_R1_G2_R3	2   	/* 5 : G0 > R1 > G2 > R3 */
#define VO_SEQ_G0_B1_G2_B3	3   	/* 5 : G0 > B1 > G2 > B3 */
#define VO_SEQ_B0_G1_B2_R3	5   	/* 5 : B0 > G1 > B2 > R3 */

	u32 dout_mode		:  5;	/* [31:27] */
#define LCD_1COLOR_PER_DOT         	0
#define	LCD_3COLORS_PER_DOT        	1
#define	LCD_3COLORS_DUMMY_PER_DOT  	2  /* 3 color per dot  dummy */
#define	LCD_RGB565                      3
#define DOUT_656                        4
#define DOUT_601                        5
#define DOUT_601_24BITS			6
#define DOUT_601_8_CBYCRY		7
#define BAYER_PATTERN			8
#define MIPI_422_YUV			9
#define MIPI_565_RGB			10
#define MIPI_RAW_8			11
#define MIPI_888_RGB			12
} vd_digital_modes_t;

typedef union {
        vd_digital_modes_t s;
	u32 w;
} vd_digital_output_modes_t;

typedef struct vd_analog_modes_s {
	u32 hd_pol 	:  1;   /* [0:0] 0: asserted low
						 1: asserted high */
	u32 vd_pol 	:  1;   /* [1:1] 0: asserted low
						 1: asserted high */
#define VD_ACT_LOW              0
#define VD_ACT_HIGH             1
	u32 reserved3		: 30;	/* [2:31] Reserved */
} vd_analog_modes_t;

typedef union {
        vd_analog_modes_t s;
	u32 w;
} vd_analog_output_modes_t;


typedef struct vd_hdmi_modes_s {
	u32 hd_pol 	:  1;   /* [0:0] 0: asserted low
						 1: asserted high */
	u32 vd_pol 	:  1;   /* [1:1] 0: asserted low
						 1: asserted high */
#define VD_ACT_LOW              0
#define VD_ACT_HIGH             1
	u32 reserved3		: 27;	/* [28:2] Reserved */

	u32 hdmi_mode		:  3;	/* [31:29] */
#define	HDMI_MODE_YCBCR444	0x0
#define	HDMI_MODE_RGB444	0x1
#define	HDMI_MODE_YC422		0x2
} vd_hdmi_modes_t;

typedef union {
        vd_hdmi_modes_t s;
	u32 w;
} vd_hdmi_output_modes_t;


typedef union {
	struct {
	u32 end			: 14;	/* [13:0] */
	u32 reserved15		:  2;	/* [15:14] */
	u32 start		: 14;	/* [29:16] */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_hsync_control_t;

typedef union {
	struct {
	u32 row			: 14;	/* [13:0]  */
	u32 reserved15		:  2;	/* [15:14] */
	u32 col			: 14;	/* [29:16] */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_vsync_start_t;

typedef union {
	struct {
 	u32 row			: 14;	/* [13:0]  */
	u32 reserved15		:  2;	/* [15:14] */
	u32 col			: 14;	/* [29:16] */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_vsync_end_t;

typedef union {
	struct {
        u32 end_row		: 14;	/* [13:0]  */
	u32 reserved15		:  2;	/* [15:14] */
	u32 start_row		: 14;	/* [29:16] */
	u32 reserved31		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_digital_656_vbit_t;

typedef union {
	struct {
        u32 start		: 14;	/* [13:0]  */
	u32 reserved31		: 18;	/* [31:14] */
	} s;
	u32 w;
} vd_digital_656_sav_start_t;

typedef struct {
	int length;             /* Pattern length in bits */
	int enable;             /* Pattern enable/disable */
	u32 pattern3;           /* MSB */
	u32 pattern2;
	u32 pattern1;
	u32 pattern0;           /* LSB */
} vd_dclk_pattern_t;


typedef union {
	struct {
	u32 cr			:  8;	/* [7:0]  background Cr value */
	u32 cb			:  8;	/* [15:8]  background Cb value */
	u32 y			:  8;	/* [23:16] background Y value */
	u32 reserved		:  8;	/* [31:24] */
	} s;
	u32 w;
} vd_background_t;

typedef union{
        struct {
	u32 start_rwo		:  14;	/* [13:0] */
	u32 reserved14		:  2;	/* [15:14] */
	u32 field_select	:  1;	/* [16:16] background Y value */
	u32 reserved17		:  15;	/* [31:17] */
	} s;
	u32 w;
} vd_vout_vout_sync_t;


typedef union {
	struct {
	u32 display_in_enable	:  1;	/* [0:0] display A/B in enable */
#define VD_STREAM_ENABLE_SMEM	1
#define VD_STREAM_ENABLE_MIXER 	0

	u32 reserved31		:  31;	/* [31:1] */
	} s;
	u32 w;
} vd_stream_enables_t;

typedef union {
	struct {
        u32 display_frame_enable	: 1;	/* [0:0]  */
	u32 reserved31			: 31;	/* [31:30] */
	} s;
	u32 w;
} vd_enable_t;

typedef struct dev_s {
        u16 disp_mode;  /** device display mode */
        u16 disp_chan;  /** device display channel */
        u16 disp_rev;   /** device display mode */
#define VD_NORM_DISPLAY         0
#define VD_FLIP_DISPLAY         1

	u16 width;      /** device display width */
	u16 height;     /** device display height */
	u16 height_i;   /** device display width in one field if available */
	u8  ar;         /** aspect ratio */
	u8  framerate;  /** frame rate */
	u8  sfx;        /** scaling factor in horizontal edge */
#define VD_SFX_1X       	1
#define VD_SFX_2X       	2
#define VD_SFX_4X       	4
	char model[40]; /* model name */

#if (VOUT_DISPLAY_SECTIONS == 2)
	u8 paths;                /* VOUT paths */
#define	VO_DIGITAL_ENABLE        1

#endif
} vout_dev_t;


typedef struct vout_obj_s {
        int init;			/* Initialization status */
        int status;        		/* 0: idle state,  1: running state */
	u32 base;       		/* Base address of the controller */
	u32 clut_base;       		/* Base address of the osda/b CLUTs */

	/* Shadow buffers */
	vd_control_t			 ctrl;        	 /* Control */
	vd_status_t             	 d_status;       /* Status*/
	vd_frame_size_field0_t  	 frm_sz_fld0;    /* Frame size field0 */
	vd_frame_size_field1_t  	 frm_sz_fld1;    /* Frame size field1 */
	vd_active_region_start_t         act_start0;     /* Active regiion start */
	vd_active_region_end_t           act_end0;       /* Active regiion end */
	vd_active_region_start_t         act_start1;     /* Active regiion start */
	vd_active_region_end_t           act_end1;       /* Active regiion end */
	vd_output_modes_t                modes;          /* Output modes*/
#if (VOUT_SUPPORT_MIPI == 1) || (VOUT_SUPPORT_DIGITAL_DITHER == 1)
	vd_digital_output_modes_t        digital_modes;  /* Digital output modes*/
	vd_analog_output_modes_t         analog_modes;  /* Digital output modes*/
	vd_hdmi_output_modes_t           hdmi_modes;  /* Digital output modes*/
#endif
	vd_hsync_control_t               digital_hd_ctrl;	/* Hsync control */
	vd_vsync_start_t                 digital_vd_start[2];	/* Vsync start */
	vd_vsync_end_t                	 digital_vd_end[2];	/* Vsync end */
	vd_hsync_control_t               hdmi_hd_ctrl;	/* Hsync control */
	vd_vsync_start_t                 hdmi_vd_start[2];	/* Vsync start */
	vd_vsync_end_t                	 hdmi_vd_end[2];	/* Vsync end */
	vd_hsync_control_t               analog_hd_ctrl;	/* Hsync control */
	vd_vsync_start_t                 analog_vd_start[2];	/* Vsync start */
	vd_vsync_end_t                	 analog_vd_end[2];	/* Vsync end */
	vd_digital_656_vbit_t            d656_vbit;      /* Digital 656 vbit */
	vd_digital_656_sav_start_t       d656_sav_start; /* Digital 656 sav start */
	vd_dclk_pattern_t                dclk_pattern;   /* Digital clock pattern */
	vd_background_t                  bg;             /* Background */
	vd_stream_enables_t              stream_enable;  /* Stream in enable */
	vd_enable_t                      enable;
} vout_obj_t;

static __ARMCC_ALIGN(4) vout_obj_t G_vout[2] __GNU_ALIGN(4);

extern int dsp_set_vout_reg(int vout_id, u32 offset, u32 val);
extern int dsp_set_vout_osd_clut(int vout_id, u32 offset, u32 val);
extern int dsp_set_vout_dve(int vout_id, u32 offset, u32 val);

/**
 * Initial VOUT object
 */
static vout_obj_t* vout_init(u8 chan)
{
	extern u8 *dsp_get_oad_ptr(int chan);

	if (chan != VOUT_DISPLAY_A && chan != VOUT_DISPLAY_B)
		return NULL;

	memset(&G_vout[chan], 0, sizeof(vout_obj_t));
	G_vout[chan].init = 1;

//	G_vout[chan].osd.buf = dsp_get_osd_ptr(chan);

	return (&G_vout[chan]);
}

int vout_enable_path(int chan, int path, int enable)
{
	int rval = 0;
	u32 offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_CONTROL_OFFSET : VOUT_DA_CONTROL_OFFSET;

	switch (path) {
	case VO_DIGITAL:
	        G_vout[chan].ctrl.s.digital_enable 	= enable;
	        break;

	case VO_HDMI:
	        G_vout[chan].ctrl.s.hdmi_enable 	= enable;
	        break;

	case VO_CVBS:
	        G_vout[chan].ctrl.s.analog_enable 	= enable;
	        break;

	default:
		rval = -1;
		putstr("The path is not supported!");
		break;
	};

	dsp_set_vout_reg(chan, offset, G_vout[chan].ctrl.w);

	return rval;
}

/**
 * Reset VOUT to the initial configuration and idle state
 */
static int vout_reset(int chan)
{
	u32 offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_CONTROL_OFFSET : VOUT_DA_CONTROL_OFFSET;

	G_vout[chan].ctrl.s.sw_reset	= 1;
	G_vout[chan].init 		= 1;

	dsp_set_vout_reg(chan, offset, G_vout[chan].ctrl.w);

	return 0;
}

/**
 * This function is to set VOUT module start running
 */
static void vout_set_freerun(int chan)
{
 	u32     offset = 0;

 	offset = (chan == VOUT_DISPLAY_B) ?
		VOUT_DB_INPUT_STREAM_ENABLES_OFFSET:
		VOUT_DA_INPUT_STREAM_ENABLES_OFFSET;
        G_vout[chan].stream_enable.s.display_in_enable 	=
					VD_STREAM_ENABLE_MIXER;

	dsp_set_vout_reg(chan, offset, G_vout[chan].stream_enable.w);

	offset = (chan == VOUT_DISPLAY_B) ?
		VOUT_DB_CONTROL_OFFSET : VOUT_DA_CONTROL_OFFSET;
	G_vout[chan].ctrl.s.sw_reset	= 0;
	dsp_set_vout_reg(chan, offset, G_vout[chan].ctrl.w);

	G_vout[chan].enable.s.display_frame_enable      = 1;

	offset = (chan == VOUT_DISPLAY_B) ?
		VOUT_DB_FRAME_ENABLE_OFFSET :
		VOUT_DA_FRAME_ENABLE_OFFSET;

	dsp_set_vout_reg(chan, offset, G_vout[chan].enable.w);

	/* G_vout[chan].status set to 1 means freerun */
	G_vout[chan].status 			= 1;
}

/**
 * This function is to config VOUT control register
 */
void vout_config_info(int chan, lcd_dev_vout_cfg_t *cfg)
{
	u32 offset = 0;

        /* Control parameter settings */
	G_vout[chan].digital_modes.s.hd_pol 		= cfg->hs_polarity;
	G_vout[chan].digital_modes.s.vd_pol 		= cfg->vs_polarity;
	G_vout[chan].digital_modes.s.hvld_pol 		= cfg->de_polarity;
	G_vout[chan].digital_modes.s.dclk_edge_select 	= cfg->data_poliarity;
	G_vout[chan].hdmi_modes.s.hd_pol 		= cfg->hs_polarity;
	G_vout[chan].hdmi_modes.s.vd_pol 		= cfg->vs_polarity;
	G_vout[chan].hdmi_modes.s.hdmi_mode		= HDMI_MODE_RGB444;
	G_vout[chan].analog_modes.s.hd_pol 		= cfg->hs_polarity;
	G_vout[chan].analog_modes.s.vd_pol 		= cfg->vs_polarity;

	if (cfg->color_space == CS_RGB) {	/* RGB color space */
		if (cfg->lcd_sync == INPUT_FORMAT_601) {	/* 601 */
			/* Initial LCD */
			//G_vout[chan].digital_modes.s.dout_mode = LCD_RGB565; /* 0:RGB digital */
			if (cfg->lcd_display == LCD_PROG_DISPLAY) {
				/* 0:VD_PROGRESSIVE */
				G_vout[chan].ctrl.s.interlace = VD_PROGRESSIVE;
			} else {
				/* 1:VD_INTERLACE */
				G_vout[chan].ctrl.s.interlace = VD_INTERLACE;
			}
		} else {
			G_vout[chan].ctrl.s.interlace = VD_PROGRESSIVE;
			G_vout[chan].digital_modes.s.dout_mode = 7;
			putstr("Not supported");
		}
	} else {				/* YUV color space */
		G_vout[chan].ctrl.s.interlace = 1;
		if (cfg->lcd_sync == INPUT_FORMAT_656)
			/* 2:YCbCr656 digital */
			G_vout[chan].digital_modes.s.dout_mode = DOUT_656;
		else
			/* 1:YCbCr601 digital */
			G_vout[chan].digital_modes.s.dout_mode = DOUT_601;
		if (cfg->lcd_display == LCD_NTSC_DISPLAY) {
			/* Initial NTSC 480I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				/* 0x0:480I60 */
				G_vout[chan].ctrl.s.fixed_format_select = VD_480I60;
			else
				/* 0x0:480I60 */
				G_vout[chan].ctrl.s.fixed_format_select = VD_480I60;
		} else if (cfg->lcd_display == LCD_PAL_DISPLAY) {
			/* Initial NTSC 576I */
			if (cfg->lcd_sync == INPUT_FORMAT_656)
				/* 0x0:576I60 */
				G_vout[chan].ctrl.s.fixed_format_select = VD_576I50;
			else
				/* 0x0:576I60 */
				G_vout[chan].ctrl.s.fixed_format_select = VD_576I50;
		}
	}

	if (chan == VOUT_DISPLAY_B && !cfg->use_dve) {
		G_vout[chan].ctrl.s.interlace = VD_PROGRESSIVE;
		switch (cfg->lcd_display) {
		case LCD_NTSC_DISPLAY:
			G_vout[chan].ctrl.s.fixed_format_select = VD_480P60;
			break;

		case LCD_PAL_DISPLAY:
			G_vout[chan].ctrl.s.fixed_format_select = VD_576P50;
			break;

		default:
			break;
		}
	}

	offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_CONTROL_OFFSET : VOUT_DA_CONTROL_OFFSET;
	dsp_set_vout_reg(chan, offset, G_vout[chan].ctrl.w);

	offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
			VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;
	dsp_set_vout_reg(chan, offset, G_vout[chan].digital_modes.w);

	if (chan == VOUT_DISPLAY_B) {
		if (!cfg->use_dve) {
			offset = VOUT_DB_HDMI_OUTPUT_MODES_OFFSET;
			dsp_set_vout_reg(chan, offset, G_vout[chan].hdmi_modes.w);
		} else {
			offset = VOUT_DB_ANALOG_OUTPUT_MODES_OFFSET;
			dsp_set_vout_reg(chan, offset, G_vout[chan].analog_modes.w);
		}
	}
}

/**
 * This function is to set backgorund color
 */
void vout_set_monitor_bg_color(int chan, u8 y, u8 cb, u8 cr)
{
	/* Control parameter settings */
	u32 offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_BACKGROUND_OFFSET : VOUT_DA_BACKGROUND_OFFSET;

	G_vout[chan].bg.s.y 	= y;
	G_vout[chan].bg.s.cb 	= cb;
	G_vout[chan].bg.s.cr 	= cr;
	dsp_set_vout_reg(chan, offset, G_vout[chan].bg.w);
}

/**
 * This function is to set start and end position for active window
 */
static void vout_set_active_win(int chan, u16 start_x, u16 end_x,
			 u16 start_y, u16 end_y)
{
	u32 offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_ACTIVE_REGION_START_0_OFFSET :
			VOUT_DA_ACTIVE_REGION_START_0_OFFSET;
	G_vout[chan].act_start0.s.row     = start_y;
	G_vout[chan].act_start0.s.col     = start_x;
	dsp_set_vout_reg(chan, offset, G_vout[chan].act_start0.w);

	offset	 = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_ACTIVE_REGION_END_0_OFFSET :
			VOUT_DA_ACTIVE_REGION_END_0_OFFSET;
	G_vout[chan].act_end0.s.row	= end_y;
	G_vout[chan].act_end0.s.col	= end_x;
	dsp_set_vout_reg(chan, offset, G_vout[chan].act_end0.w);
}

/**
 * This function is to set start and end position for active window of field1
 */
void vout_set_active_win_field1(int chan, u16 start_x, u16 end_x,
					u16 start_y, u16 end_y)
{

	u32 offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_ACTIVE_REGION_START_1_OFFSET :
			VOUT_DA_ACTIVE_REGION_START_1_OFFSET;
	G_vout[chan].act_start1.s.row	= start_y;
	G_vout[chan].act_start1.s.col	= start_x;
	dsp_set_vout_reg(chan, offset, G_vout[chan].act_start1.w);

	offset	 = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_ACTIVE_REGION_END_1_OFFSET :
			VOUT_DA_ACTIVE_REGION_END_1_OFFSET;
	G_vout[chan].act_end1.s.row	= end_y;
	G_vout[chan].act_end1.s.col	= end_x;
	dsp_set_vout_reg(chan, offset, G_vout[chan].act_end1.w);
}

/**
 * This function is to set Vsync/Hsync polarity
 * 0 = active low & 1 = active high
 */
static void vout_set_sync_polarity(int chan, int path, u8 h_pol, u8 v_pol)
{
#if (VOUT_SUPPORT_MIPI == 0) && (VOUT_SUPPORT_DIGITAL_DITHER == 0)
	u32 offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_OUTPUT_MODES_OFFSET :
				VOUT_DA_OUTPUT_MODES_OFFSET;
	vd_rd_reg(G_vout[chan].base + offset, G_vout[chan].modes.w);

	switch (path) {
        case VO_DIGITAL:
                G_vout[chan].modes.s.dout_hd_pol 	= h_pol;
                G_vout[chan].modes.s.dout_vd_pol 	= v_pol;
                break;

	case VO_HDMI:
                G_vout[chan].modes.s.hdmi_hd_pol 	= h_pol;
                G_vout[chan].modes.s.hdmi_vd_pol 	= v_pol;
                break;

	case VO_CVBS:
                G_vout[chan].modes.s.analog_hd_pol 	= h_pol;
                G_vout[chan].modes.s.analog_vd_pol 	= v_pol;
                break;

	default:
	        printk("The selected path does not exist");
	}
	dsp_set_vout_reg(chan, offset, G_vout[chan].modes.w);
#else

	u32 offset ;
	switch (path) {
        case VO_DIGITAL:
		offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
			VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].digital_modes.w);

                G_vout[chan].digital_modes.s.hd_pol 	= h_pol;
                G_vout[chan].digital_modes.s.vd_pol 	= v_pol;
                G_vout[chan].modes.s.dout_hd_pol 	= h_pol;
                G_vout[chan].modes.s.dout_vd_pol 	= v_pol;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].digital_modes.w);
                break;

	case VO_HDMI:
		offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
			VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].digital_modes.w);

                G_vout[chan].hdmi_modes.s.hd_pol 	= h_pol;
                G_vout[chan].hdmi_modes.s.vd_pol 	= v_pol;
                G_vout[chan].modes.s.hdmi_hd_pol 	= h_pol;
                G_vout[chan].modes.s.hdmi_vd_pol 	= v_pol;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].digital_modes.w);
                break;

	case VO_CVBS:
		offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_ANALOG_OUTPUT_MODES_OFFSET :
			VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].analog_modes.w);

                G_vout[chan].analog_modes.s.hd_pol 	= h_pol;
                G_vout[chan].analog_modes.s.vd_pol 	= v_pol;
                G_vout[chan].modes.s.aout_hd_pol 	= h_pol;
                G_vout[chan].modes.s.aout_vd_pol 	= v_pol;

		dsp_set_vout_reg(chan, offset,
			  G_vout[chan].analog_modes.w);
                break;

	default:
	        putstr("The selected path does not exist");
	        break;
	}
#endif
}

static void vout_set_h_sync(int chan, int path, u32 start, u32 end)
{
	switch (path) {
	case VO_DIGITAL:
	        G_vout[chan].digital_hd_ctrl.s.start	= start;
	        G_vout[chan].digital_hd_ctrl.s.end 	= end;
	        if (chan == VOUT_DISPLAY_B) {
		dsp_set_vout_reg(chan,
			VOUT_DB_DIGITAL_HSYNC_CONTROL_OFFSET,
		       	G_vout[chan].digital_hd_ctrl.w);
	  	} else {
	                dsp_set_vout_reg(chan,
			VOUT_DA_DIGITAL_HSYNC_CONTROL_OFFSET,
		       	G_vout[chan].digital_hd_ctrl.w);
		}
		break;

	case VO_HDMI:
	        G_vout[chan].hdmi_hd_ctrl.s.start	= start;
	        G_vout[chan].hdmi_hd_ctrl.s.end 	= end;
	        if (chan == VOUT_DISPLAY_B) {
			dsp_set_vout_reg(chan,
				VOUT_DB_HDMI_HSYNC_CONTROL_OFFSET,
			       	G_vout[chan].hdmi_hd_ctrl.w);
	  	} else {
	                dsp_set_vout_reg(chan,
			VOUT_DA_DIGITAL_HSYNC_CONTROL_OFFSET,
		       	G_vout[chan].hdmi_hd_ctrl.w);
		}
		break;

	case VO_CVBS:
	        G_vout[chan].analog_hd_ctrl.s.start	= start;
	        G_vout[chan].analog_hd_ctrl.s.end 	= end;
	        if (chan == VOUT_DISPLAY_B) {
			dsp_set_vout_reg(chan,
				VOUT_DB_ANALOG_HSYNC_CONTROL_OFFSET,
			       	G_vout[chan].analog_hd_ctrl.w);
	  	} else {
	                dsp_set_vout_reg(chan,
			VOUT_DA_DIGITAL_HSYNC_CONTROL_OFFSET,
		       	G_vout[chan].analog_hd_ctrl.w);
		}
		break;

	default:
		putstr("Unknown path:");
		putdec(path);
		break;
	}
}

static void vout_set_v_sync(int chan, int path, int fld,
			    u32 start_col, u32 start_row,
			    u32 end_col, u32 end_row)
{
	u32 offset;

	switch (path) {
        case VO_DIGITAL:
                G_vout[chan].digital_vd_start[fld].s.row = start_row;
        	G_vout[chan].digital_vd_start[fld].s.col = start_col;
        	G_vout[chan].digital_vd_end[fld].s.row 	 = end_row;
        	G_vout[chan].digital_vd_end[fld].s.col 	 = end_col;

                if (fld == 0) {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_DIGITAL_VSYNC_START_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].digital_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_DIGITAL_VSYNC_END_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].digital_vd_end[fld].w);

		} else {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_DIGITAL_VSYNC_START_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].digital_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_DIGITAL_VSYNC_END_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].digital_vd_end[fld].w);
		}
                break;

	case VO_HDMI:
                G_vout[chan].hdmi_vd_start[fld].s.row = start_row;
        	G_vout[chan].hdmi_vd_start[fld].s.col = start_col;
        	G_vout[chan].hdmi_vd_end[fld].s.row 	 = end_row;
        	G_vout[chan].hdmi_vd_end[fld].s.col 	 = end_col;

                if (fld == 0) {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_HDMI_VSYNC_START_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].hdmi_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_HDMI_VSYNC_END_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].hdmi_vd_end[fld].w);

		} else {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_HDMI_VSYNC_START_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].hdmi_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_HDMI_VSYNC_END_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].hdmi_vd_end[fld].w);
		}
                break;

	case VO_CVBS:
                G_vout[chan].analog_vd_start[fld].s.row = start_row;
        	G_vout[chan].analog_vd_start[fld].s.col = start_col;
        	G_vout[chan].analog_vd_end[fld].s.row 	 = end_row;
        	G_vout[chan].analog_vd_end[fld].s.col 	 = end_col;

                if (fld == 0) {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_ANALOG_VSYNC_START_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].analog_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_ANALOG_VSYNC_END_0_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_0_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].analog_vd_end[fld].w);

		} else {
                        offset = (chan == VOUT_DISPLAY_B) ?
					VOUT_DB_ANALOG_VSYNC_START_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_START_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].analog_vd_start[fld].w);

                        offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_ANALOG_VSYNC_END_1_OFFSET :
				VOUT_DA_DIGITAL_VSYNC_END_1_OFFSET;
                	dsp_set_vout_reg(chan, offset,
		       	       G_vout[chan].analog_vd_end[fld].w);
		}
                break;

	default:
		putstr("Unknown path:");
		putdec(path);
		break;
	}
}

/**
 * This function is to set sync cycles for display format.
 * h  = # of cycles per horizontal line
 * v = # of hsync in one frame
 */
static void vout_set_hv(int chan, u16 h, u16 v, u16 v1)
{

	u32 offset = 0;
	G_vout[chan].frm_sz_fld0.s.width 	= h - 1;
	G_vout[chan].frm_sz_fld0.s.height 	= v - 1;
	G_vout[chan].frm_sz_fld1.s.height 	= v1 - 1;

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_FRAME_SIZE_OFFSET :
				VOUT_DA_FRAME_SIZE_OFFSET;
	dsp_set_vout_reg(chan, offset, G_vout[chan].frm_sz_fld0.w);

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_FRAME_HEIGHT_FIELD_1_OFFSET :
				VOUT_DA_FRAME_HEIGHT_FIELD_1_OFFSET;
	dsp_set_vout_reg(chan, offset, G_vout[chan].frm_sz_fld1.w);
}

void vout_set_dclk_out(int chan, int enabled)
{
#if (VOUT_SUPPORT_DCLK_ENA == 1)

#if (VOUT_SUPPORT_MIPI == 0) && (VOUT_SUPPORT_DIGITAL_DITHER == 0)
	u32 offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_OUTPUT_MODES_OFFSET :
				VOUT_DA_OUTPUT_MODES_OFFSET;
	G_vout[chan].modes.s.dclk_disable  = !enabled;
	dsp_set_vout_reg(chan, offset, G_vout[chan].modes.w);
#else
	u32 offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
				VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;
	G_vout[chan].digital_modes.s.dclk_disable  = !enabled;
	G_vout[chan].modes.s.dclk_disable  = !enabled;
	dsp_set_vout_reg(chan, offset, G_vout[chan].digital_modes.w);
#endif

#endif
}

/**
 * This function is to configure and enable/disable the VOUT data pattern to
 * LCD panel
 */
static int vout_config_dclk_pattern(int chan, vd_dclk_pattern_t *pattern)
{
#if (VOUT_SUPPORT_MIPI == 0) && (VOUT_SUPPORT_DIGITAL_DITHER == 0)
 	u32     offset = (chan == VOUT_DISPLAY_B) ?
	 		VOUT_DB_DIGITAL_CLOCK_PATTERN_0_OFFSET :
		        VOUT_DA_DIGITAL_CLOCK_PATTERN_0_OFFSET;

	vd_dclk_pattern_t *bp = (vd_dclk_pattern_t *) pattern;

	if (bp->enable == VD_DCLK_DIV_ENABLE) {
	        vd_wr_reg((G_vout[chan].base + offset + (3 << 2)), bp->pattern3);
  		vd_wr_reg((G_vout[chan].base + offset + (2 << 2)), bp->pattern2);
  		vd_wr_reg((G_vout[chan].base + offset + (1 << 2)), bp->pattern1);
    		vd_wr_reg((G_vout[chan].base + offset), bp->pattern0);
	        G_vout[chan].modes.s.bit_pattern_width = bp->length - 1;
	}

	G_vout[chan].modes.s.dclk_divider_enable = bp->enable;

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_OUTPUT_MODES_OFFSET :
				VOUT_DA_OUTPUT_MODES_OFFSET;
	vd_wr_reg(G_vout[chan].base + offset, G_vout[chan].modes.w);
#else
 	u32     offset = (chan == VOUT_DISPLAY_B) ?
	 		VOUT_DB_DIGITAL_CLOCK_PATTERN_0_OFFSET :
		        VOUT_DA_DIGITAL_CLOCK_PATTERN_0_OFFSET;

	vd_dclk_pattern_t *bp = (vd_dclk_pattern_t *) pattern;

	if (bp->enable == VD_DCLK_DIV_ENABLE) {
	        dsp_set_vout_reg(chan, offset + (3 << 2), bp->pattern3);
  		dsp_set_vout_reg(chan, offset + (2 << 2), bp->pattern2);
  		dsp_set_vout_reg(chan, offset + (1 << 2), bp->pattern1);
    		dsp_set_vout_reg(chan, offset, bp->pattern0);
	        G_vout[chan].modes.s.bit_pattern_width = bp->length - 1;
	        G_vout[chan].digital_modes.s.bit_pattern_width = bp->length - 1;
	}

	G_vout[chan].modes.s.dclk_divider_enable = bp->enable;
	G_vout[chan].digital_modes.s.dclk_divider_enable = bp->enable;

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
				VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;
	dsp_set_vout_reg(chan, offset, G_vout[chan].digital_modes.w);
#endif

	return 0;
}

/**
 * This function is to divide 27 MHz to 27/(2 ^ clk_div)MHz
 */
static void vout_set_fractional_clock(int chan, u8 clk_div)
{
 	vd_dclk_pattern_t bp;

	memset(&bp, 0x0, sizeof(vd_dclk_pattern_t));

	if (clk_div == 1) { /* 13.5MHz */
	        bp.length	= 2;
		bp.enable	= VD_DCLK_DIV_ENABLE;      /* enable divider */
		bp.pattern3	= 0x00000000;           /* MSB */
		bp.pattern2	= 0x00000000;
		bp.pattern1	= 0x00000000;
		bp.pattern0	= 0x00000001;           /* LSB */
	} else if (clk_div >= 2) { /* 6.75MHz */
	        bp.length	= 4;
		bp.enable	= VD_DCLK_DIV_ENABLE;      /* enable divider */
		bp.pattern3	= 0x00000000;           /* MSB */
		bp.pattern2	= 0x00000000;
		bp.pattern1	= 0x00000000;
		bp.pattern0	= 0x00000003;           /* LSB */
	} else { /* 27MHz */
		bp.enable	= VD_DCLK_DIV_DISABLE;
	}
	vout_config_dclk_pattern(chan, &bp);
}

/**
 * Set LCD information
 */
static void vout_set_lcd_mode(int chan, u8 clock, u8 out_color_mode,
			      u8 rgbtop, u8 rgbbot, u8 reverse)
{
	u32 offset = 0;
	/*
	clock = 0 : 27.0M
	clock = 1 : 13.5M
	clock = 2 :  6.75M*/
	vout_set_fractional_clock(chan, clock);
	/*
		rgbodd = rgbeven >>
		0 : R0 > G1 > B2
		1 : R0 > B1 > G2
		2 : G0 > R1 > B2
		3 : G0 > B1 > R2
		4 : B0 > R1 > G2
		5 : B0 > G1 > R2
	*/

#if (VOUT_SUPPORT_MIPI == 0) && (VOUT_SUPPORT_DIGITAL_DITHER == 0)
       	/*
	out_color_mode = 0 : signle color per dot
	out_color_mode = 1 : 3 color per dot
	out_color_mode = 2 : rgb565
	out_color_mode = 3/4 : reserved
	out_color_mode = 5 : 3 color per dot + dummy
	*/

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_OUTPUT_MODES_OFFSET :
				VOUT_DA_OUTPUT_MODES_OFFSET;
	G_vout[chan].modes.s.dout_mode 		= out_color_mode;
	G_vout[chan].modes.s.lcd_seqe  		= rgbtop;
	G_vout[chan].modes.s.lcd_seqo  		= rgbbot;
 	vd_wr_reg(G_vout[chan].base + offset, G_vout[chan].modes.w);
#else
        /*
	out_color_mode = 0 : signle color per dot
	out_color_mode = 1 : 3 color per dot
	out_color_mode = 2 : 3 color per dot + dummy
	out_color_mode = 3 : rgb565
	out_color_mode = 4 : dvout 656
	out_color_mode = 5 : dvout 601
	*/

	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_DIGITAL_OUTPUT_MODES_OFFSET :
				VOUT_DA_DIGITAL_OUTPUT_MODES_OFFSET;

	G_vout[chan].digital_modes.s.dout_mode 		= out_color_mode;
	G_vout[chan].digital_modes.s.lcd_seqe  		= rgbtop;
	G_vout[chan].digital_modes.s.lcd_seqo  		= rgbbot;
	G_vout[chan].modes.s.dout_mode 			= out_color_mode;
	G_vout[chan].modes.s.lcd_seqe  			= rgbtop;
	G_vout[chan].modes.s.lcd_seqo  			= rgbbot;

	//In ide state
	dsp_set_vout_reg(chan, offset, G_vout[chan].digital_modes.w);

#endif
	offset = (chan == VOUT_DISPLAY_B) ?
				VOUT_DB_CONTROL_OFFSET :
				VOUT_DA_CONTROL_OFFSET;
	G_vout[chan].ctrl.s.reverse_mode 	= reverse;
	dsp_set_vout_reg(chan, offset, G_vout[chan].ctrl.w);
}

/**
 * Calculate the entry of CLUT / palette
 */
static u32 cal_clut_entry(u8 *ptr, u8 blend_factor, u8 yuv)
{
	u32 table = 0;
	u8  tab[3] = {0, 0, 0};

        if (!yuv){
		/* tab[0] = 0.299 * ptr[0] + 0.587 * ptr[1] + 0.114 * ptr[2]; */
		tab[0] = (5016387 * ptr[0] + 9848226 * ptr[1] +
			1912603 * ptr[2]) >> 24;

		/* tab[1] = (ptr[2] - tab[0]) * 0.565; */
		if (tab[0] >= ptr[2])
			tab[1] = ((128 << 24) - (tab[0] - ptr[2]) * 9479127)
				>> 24 ;
		else
			tab[1] = ((ptr[2] - tab[0]) * 9479127 +
				(128 << 24)) >> 24;

		/* tab[2] = (ptr[0] - tab[0]) * 0.713; */
		if (tab[0] >= ptr[0])
			tab[2] = ((128 << 24) - (tab[0] - ptr[0]) * 11962155)
				>> 24 ;
		else
			tab[2] = ((ptr[0] - tab[0]) * 11962155 +
				(128 << 24)) >> 24;

#if 1   /* 16 ~ 235 */
		tab[0] = CLIP(tab[0], 235, 16);
		tab[1] = CLIP(tab[1], 240, 16);
		tab[2] = CLIP(tab[2], 240, 16);
#else   /* 0 ~ 255 */
		tab[0] = CLIP(tab[0], 255, 0);
		tab[1] = CLIP(tab[1], 255, 0);
		tab[2] = CLIP(tab[2], 255, 0);
#endif
		table  = ((blend_factor & 0xff) << 24) +
		         ((tab[0] & 0xff) << 16) +
			 ((tab[1] & 0xff) << 8) +
			  (tab[2] & 0xff);
        } else {
		table = ((blend_factor & 0xff) << 24) +
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
	u32 offset = 0, table = 0;
	int j = 0;
	u8 *ptr = NULL;

	ptr 	= &rgbtable[0];
	offset 	= 0;
	for (j = 0; j < 256; j++) {
	        table 	 = cal_clut_entry(ptr, blend_table[j], yuv);
		dsp_set_vout_osd_clut(chan, offset, table);
		ptr    += 3;
		offset += 4;
	}
}

/**
* This function is to set dve config
*/
static void set_dve(u8 chan, dram_dve_t *dve)
{
	int	i;
	u32	*tmp = (u32 *)dve;

	for (i = 0; i < sizeof(*dve) / sizeof(u32); i++) {
		dsp_set_vout_dve(chan, 4 * i, tmp[i]);
	}
}

/**
 * Congfigure VOUT CSC (Color Space Conversion)
 *
 */
void vout_set_csc(int chan, int path, u8 csc_mode, u8 csc_clamp)
{
#if (VOUT_SUPPORT_DIGITAL_CSC == 1)

	int i = 0;
	u32 offset = 0;

	// !!! this function is to guide the next programmer
	//     how to program the CSC. And this is only applied
	//     to 8 bit mode (not include analog HD and hdmi mode).
	//     The next programer should be able to complete it.]
	//     ps. the following matrix coef is designed by Wilson K.
	u32 color_tbl[7][6] = {
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
			0x1fff04a8, 0x004d072b, 0x7f087edf },

		{ 0x04000400, 0x00000400, 0x00000000,
		  	0x03ff0000, 0x03ff0000, 0x03ff0000 }};

#ifdef CONFIG_CVBS_MODE_NONE
	u32 clamp_tbl[10][3] = {
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
#else
	u32 clamp_tbl[2][6] = {
		/* VO_DATARANGE_ANALOG_SD_NTSC */
		{ 0x031b031b, 0x000004c0, 0x00000000,
		  0x03ac0040, 0x03ff0000, 0x03c00040 },

		/* VO_DATARANGE_ANALOG_SD_PAL */
		{ 0x03300330, 0x000004c0, 0x00000000,
		  0x03ac0040, 0x03ff0000, 0x03c00040 } };
#endif

	switch (path) {
	case VO_DIGITAL:
		offset = (chan == VOUT_DISPLAY_B) ?
			VOUT_DB_DIGITAL_CSC_PARAM_0_OFFSET :
		  	VOUT_DA_DIGITAL_CSC_PARAM_0_OFFSET;
		break;

	case VO_HDMI:
		offset = VOUT_DB_HDMI_CSC_PARAM_0_OFFSET;
		break;

	case VO_CVBS:
		offset = VOUT_DB_ANALOG_CSC_PARAM_0_OFFSET;
		break;

	default:
		break;
	}
	for (i = 0; i < 6; i++) {
        dsp_set_vout_reg(chan, offset + (4 * i),
	       color_tbl[csc_mode][i]);
	}
	offset += 24;
#ifdef CONFIG_CVBS_MODE_NONE
	for (i = 0; i < 3; i++) {
		dsp_set_vout_reg(chan, offset + (4 * i),
				 clamp_tbl[csc_clamp][i]);
	}
#else
	for (i = 0; i < 6; i++) {
		dsp_set_vout_reg(chan, offset + (4 * i),
				 clamp_tbl[csc_clamp][i]);
	}
#endif
#endif
}

/* The parameters are setting to VOUT control for specific lcd */
int vout_config(int chan, lcd_dev_vout_cfg_t *cfg)
{
	int		rval = -1;
	u8		csc_mode = VO_CSC_YUVSD2RGB;
	u8		csc_clamp = VO_DATARANGE_DIGITAL_SD_FULL;
	int		path = VO_DIGITAL;
	vout_obj_t	*pvout;

	if (chan) {
		if (cfg->use_dve) {
			path = VO_CVBS;
		} else {
			path = VO_HDMI;
		}
	}

	pvout = vout_init(chan);

	if (chan != VOUT_DISPLAY_A && chan != VOUT_DISPLAY_B) {
		return rval;
	}

	vout_reset(chan);

        vout_enable_path(chan, path, 1);

	if (path == VO_DIGITAL) {
		vout_set_dclk_out(chan, 1);
	}

	set_osd_clut(chan, bld_clut, bld_blending, 0);

	if (chan && path == VO_CVBS) {
		set_dve(chan, &cfg->dve);
	}

	/* Hsycn polarity = Vsync polarity */
	vout_set_sync_polarity(chan, path, cfg->hs_polarity, cfg->vs_polarity);

	/* Total H pclk, total V lines */
	vout_set_hv(chan, cfg->ftime_hs, cfg->ftime_vs_top, cfg->ftime_vs_bot);

	/* 0x328 */
	vout_set_h_sync(chan, path, cfg->hs_start, cfg->hs_end);

	vout_set_v_sync(chan, path, 0,
			cfg->vs_start_col_top, cfg->vs_start_row_top,
			cfg->vs_end_col_top, cfg->vs_end_row_top);

	if (cfg->lcd_display != LCD_PROG_DISPLAY) {
	        /* VD_vsync1 */
		vout_set_v_sync(chan, path, 1,
				cfg->vs_start_col_bot, cfg->vs_start_row_bot,
				cfg->vs_end_col_bot, cfg->vs_end_row_bot);
	}

	if (path == VO_DIGITAL && cfg->color_space == CS_RGB) {
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

	if (cfg->lcd_display != LCD_PROG_DISPLAY) {
		vout_set_active_win_field1(chan,
					   cfg->act_start_col_bot,
					   cfg->act_end_col_bot,
					   cfg->act_start_row_bot,
					   cfg->act_end_row_bot);
	}

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
	switch (cfg->color_space) {
	case CS_RGB:
		csc_mode	= VO_CSC_YUVSD2RGB;
		csc_clamp	= VO_DATARANGE_DIGITAL_SD_FULL;
		break;

	case CS_YUV:
		csc_mode	= VO_CSC_CVBS;
		csc_clamp	= VO_DATARANGE_ANALOG_SD_NTSC;
		if (cfg->lcd_display == LCD_PAL_DISPLAY) {
			csc_clamp	= VO_DATARANGE_ANALOG_SD_PAL;
		}
		break;

	default:
		break;

	}

	vout_set_csc(chan, path, csc_mode, csc_clamp);
#endif

	vout_set_freerun(chan);

	return 0;
}
#endif

