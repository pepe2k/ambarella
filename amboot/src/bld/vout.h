/**
 * tcldsp/bld/vout.h
 *
 * History:
 *    2008/06/03 - [E-John Lien] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __VOUT_H__
#define __VOUT_H__

#define	CLIP(a, max, min) ((a) > (max)) ? (max) : (((a) < (min)) ? (min) : (a))

#if (VOUT_DISPLAY_SECTIONS == 1)
#define VOUT_CH0	0
#define VOUT_CH1	1
#endif

#if (VOUT_DISPLAY_SECTIONS == 2)
#define VOUT_DISPLAY_A          0
#define VOUT_DISPLAY_B        	1

/* Vout path */
#define	VO_DIGITAL 	        0
#define	VO_HDMI			1
#define	VO_CVBS			2

#define VOUT_DA_REG_BASE_OFFSET VOUT_DA_CONTROL_OFFSET
#define VOUT_DB_REG_BASE_OFFSET VOUT_DB_CONTROL_OFFSET
#endif	// #if (VOUT_DISPLAY_SECTIONS == 2)


/* LCD display modes */
#define VO_480I                 0x5
#define VO_576I                 0x6

#define VO_RGB_MODE(x)          (x + 0x20)
#define VO_RGB_640_480          VO_RGB_MODE(0)
#define VO_RGB_720_240          VO_RGB_MODE(1)
#define VO_RGB_960_240          VO_RGB_MODE(2)
#define VO_RGB_480_240          VO_RGB_MODE(3)
#define VO_RGB_360_240          VO_RGB_MODE(4)
#define VO_RGB_360_288          VO_RGB_MODE(5)
#define VO_RGB_320_240          VO_RGB_MODE(6)
#define VO_RGB_320_288          VO_RGB_MODE(7)
#define VO_RGB_240_320          VO_RGB_MODE(8)
#define VO_RGB_960_288          VO_RGB_MODE(9)
#define VO_RGB_800_480          VO_RGB_MODE(10)
#define VO_RGB_480_640          VO_RGB_MODE(11)
#define VO_RGB_320_480          VO_RGB_MODE(12)
#define VO_RGB_480_800          VO_RGB_MODE(13)
#define VO_RGB_1024_768        	VO_RGB_MODE(14)
#define VO_RGB_480P        	VO_RGB_MODE(15)
#define VO_RGB_576P        	VO_RGB_MODE(16)
#define VO_RGB_480I        	VO_RGB_MODE(17)
#define VO_RGB_576I        	VO_RGB_MODE(18)
#define VO_RGB_240_432        	VO_RGB_MODE(19)

/** Display aspect ratios */
#define	VO_DISPAR_4x3	        1
#define	VO_DISPAR_16x9	        2
#define	VO_DISPAR_1x1	        4

/** OSD modes */
#define OSD_480I		0
#define OSD_576I		1

#define	VO_OSD0		        2
#define	VO_OSD1		        4

/** VOUT clock rates */
#define DCLK_FREQ_1_TO_1		0
#define DCLK_FREQ_2_TO_1		1
#define DCLK_FREQ_4_TO_1		2

/* Input video formats */
#define INPUT_FORMAT_601	0
#define INPUT_FORMAT_656	1

/** HV sync */
#define VO_MODE_HSYNC         	0      /* mode 0 = Hsync */
#define VO_MODE_VSYNC_TOP       1      /* mode 1 = Vsync top */
#define VO_MODE_VSYNC_BOT       2      /* mode 2 = Vsync bottom */

/* Vout background colors */
#define VOUT_BG_RED		0x515af0
#define VOUT_BG_MAGENTA		0x6acade
#define VOUT_BG_YELLOW		0xd21092
#define VOUT_BG_GREEN		0x913622
#define VOUT_BG_BLUE		0x29f06e
#define VOUT_BG_CYAN		0xaaa610
#define VOUT_BG_WHITE		0xeb8080
#define VOUT_BG_BLACK		0x108080

#if (VOUT_SUPPORT_DIGITAL_CSC == 1)

/** Color Space Conversion (CSC) */
#define	VO_CSC_YUVSD2YUVHD	0	/* YUV601 -> YUV709 */
#define	VO_CSC_YUVSD2YUVSD	1	/* YUV601 -> YUV601 */
#define	VO_CSC_YUVSD2RGB	2	/* YUV601 -> RGB    */
#define	VO_CSC_YUVHD2YUVSD	3	/* YUV709 -> YUV601 */
#define	VO_CSC_YUVHD2YUVHD	4	/* YUV709 -> YUV709 */
#define	VO_CSC_YUVHD2RGB	5	/* YUV709 -> RGB    */
#define	VO_CSC_CVBS		6

/** Data output range clamping */
#define	VO_DATARANGE_DIGITAL_HD_FULL	2
#define	VO_DATARANGE_DIGITAL_SD_FULL	3
#define	VO_DATARANGE_DIGITAL_HD_CLAMP	6
#define	VO_DATARANGE_DIGITAL_SD_CLAMP	7

#define	VO_DATARANGE_ANALOG_SD_NTSC	0
#define	VO_DATARANGE_ANALOG_SD_PAL	1

#endif

/* OSD object for store the BMP raw data */
typedef struct osd_s {
	u8 *buf;
	u32 width;
	u32 height;
	u32 pitch;
	u32 size;
	u16 resolution;
	u8  fg_color;
	u8  bg_color;
} osd_t;

/* BMP file header structure */
typedef struct bmp_file_header_s {
	u16 id;			/** the magic number used to identify
				 *  'BM' - Windows 3.1x, 95, NT, ...
				 *  'BA' - OS/2 Bitmap Array
				 *  'CI' - OS/2 Color Icon
				 *  'CP' - OS/2 Color Pointer
				 *  'IC' - OS/2 Icon
				 *  'PT' - OS/2 Pointer
				 *  the BMP file: 0x42 0x4D
				 *  (ASCII code points for B and M)
				 *  Need 0x424d
				 */
	u32 file_size;		/* the size of the BMP file in bytes */
	u32 reserved;		/* reserved */
	u32 offset;		/** the offset, i.e. starting address,
				 *  of the byte where the bitmap data
				 *  can be found.
				 */
} bmp_file_header_t;

/* BMP info header structure */
typedef struct bmp_info_header_s {
	u32 bmp_header_size;	/** 0x28 : Windows V3, all Windows versions
				 *	   since Windows 3.0
				 *  0x0c : OS/2 V1
				 *  0xf0 : OS/2 V2
				 *  0x6c : Windows V4, all Windows versions
				 *	   since Windows 95/NT4
				 *  0x7c : Windows V5, Windows 98/2000 and newer
				 *  Supports 0x28 now.
				 */
	u32 width;		/** the bitmap width in pixels (signed integer)
				 *  Need less or equal to 360.
				 */
	u32 height;		/** the bitmap height in pixels (signed integer)
				 *  Need less or equal to 240.
				 */
	u16 planes;		/** the number of color planes being used.
				 *  Must be set to 1.
				 */
	u16 bits_per_pixel;	/** the number of bits per pixel,
				 *  which is the color depth of the image.
				 *  Typical values are 1, 4, 8, 16, 24 and 32.
				 *  Need to 8 now.
				 */
	u32 compression;	/** the compression method being used.
				 *  Support 0, none compression now.
				 */
	u32 bmp_data_size;	/** the image size.
				 *  This is the size of the raw bitmap data.
				 */
	u32 h_resolution;	/** the horizontal resolution of the image.
				 *  (pixel per meter, signed integer)
				 */
	u32 v_resolution;	/** the vertical resolution of the image.
				 *  (pixel per meter, signed integer)
				 */
	u32 used_colors;	/** the number of colors in the color palette,
				 *  or 0 to default to 2n.
				 */
	u32 important_colors;	/** the number of important colors used,
				 *  or 0 when every color is important;
				 *  generally ignored.
				 */
} bmp_info_header_t;

/* BMP header structure */
typedef struct bmp_header_s {
	bmp_file_header_t fheader;
	bmp_info_header_t iheader;
} bmp_header_t;

/* BMP data structure */
typedef struct bmp_s {
	bmp_header_t header;
	u8 *buf;		/* Point to the addr that stored BMP raw data */
	u32 width;		/* The bmp width */
	u32 height;		/* The bmp height */
	u32 pitch;		/* The bmp pitch */
	u32 size;		/* The bmp size */
	u8  color_depth;	/* Color depth of CLUT */
} bmp_t;

/******************************** DVE *****************************************/
typedef union {
	struct {
	u32 t_reset_fsc    :  1; /* [0]  */
#define RST_PHASE_ACCUMULATOR_DIS       0
#define RST_PHASE_ACCUMULATOR_ENA       1

	u32 t_offset_phase :  1; /* [1] */
#define PHASE_OFFSET_DIS                0
#define PHASE_OFFSET_ENA                1

	u32 v_invert       :  1; /* [2] */
	u32 u_invert       :  1; /* [3] */
#define INVERT_COMP_DIS                 0
#define INVERT_COMP_ENA	                1

	u32 reserved       :  4; /* [7:6] */
	u32 unused         : 24; /*[31:8] */
	} s;
	u32 w;
} dve40_t;

typedef union {
	struct {
	u32 t_ygain_val   :  1; /* [0] */
	u32 t_sel_ylpf    :  1; /* [1] Luma LPF select */
	u32 t_ydel_adj    :  3; /* [4:2] Adjust Y delay */
	u32 y_colorbar_en :  1; /* [5] Enable internal color bar gen */
	u32 y_interp      :  2; /* [7:6] choose luma interpolatino mode */
	u32 unused        : 24; /* [31:8] */
	} s;
	u32 w;
} dve46_t;

typedef union {
	struct {
	u32 pwr_dwn_c_dac  :  1; /* [0] */
	u32 pwr_dwn_y_dac  :  1; /* [1] */
	u32 pwr_dwn_cv_dac :  1; /* [2] */
	u32 sel_yuv        :  1; /* [3] */
	u32 reserved       :  4; /* [7:4] */
	u32 unused         : 24; /* [31:8] */
	} s;
	u32 w;
} dve47_t;

typedef union {
	struct {
	u32 sel_c_gain     :  1; /* [0] */
	u32 pal_c_lpf      :  1; /* [1] */
	u32 reserved       :  4; /* [7:4] */
	u32 unused         : 24; /* [31:8] */
	} s;
	u32 w;
} dve52_t;

typedef union {
	struct {
	u32 y_tencd_mode  :  3; /* [2:0] select TV standard */
	u32 y_tsyn_mode   :  3; /* [5:3] master/slave/timecode ysnc
			         	 mode and input format select */
	u32 t_vsync_phs   :  1; /* [6] select phase of Vsync in/out */
	u32 t_hsync_phs   :  1; /* [7] select phase of hsync in/out */
	u32 unused        : 24; /* [31:8] */
	} s;
	u32 w;
} dve56_t;

typedef union {
	struct {
	u32 vso          :  2; /* [1:0] vertical sync offset MSBs */
	u32 unused       :  2; /* [3:2] */
	u32 t_psync_phs  :  1; /* [4] select phase of field sync in/out */
	u32 t_psync_enb  :  1; /* [5] enable ext_vsync_in as
			      	      field sync input */
	u32 clk_phs      :  2; /* [7:6] 6.75/13.5 MHz phase adjust */
	u32 unused2      : 24; /* [31:8] */
	} s;
	u32 w;
} dve57_t;

typedef union {
	struct {
	u32 cs_ln       :  2; /* [1:0] */
	u32 cs_num      :  3; /* [4:2] */
	u32 cs_sp       :  3; /* [7:5] */
	u32 unused      : 24; /* [31:8] */
	} s;
	u32 w;
} dve77_t;

typedef union {
	struct {
	u32 bst_zone_sw2 :  4; /* [3:0] */
	u32 bst_zone_sw1 :  4; /* [7:4] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve88_t;

typedef union {
	struct {
	u32 bz_invert_en :  3; /* [2:0] */
	u32 adv_bs_en    :  1; /* [3] */
	u32 bz3_end      :  4; /* [7:4] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve89_t;

typedef union {
	struct {
	u32 fsc_tst_en   :  1; /* [0] */
	u32 sel_sin      :  1; /* [1] */
	u32 reserved     :  6; /* [7:2] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve96_t;

typedef union {
	struct {
	u32 dig_out_en  :  2; /* [1:0] */
	u32 sel_dac_tst :  1; /* [2] */
	u32 sin_cos_en  :  1; /* [3] */
	u32 ygain_off   :  1; /* [4] */
	u32 sel_y_lpf   :  1; /* [5] */
	u32 byp_y_ups   :  1; /* [6] */
	u32 reserved    :  1; /* [7] */
	u32 unused      : 24; /* [31:8] */
	} s;
	u32 w;
} dve97_t;

typedef union {
	struct {
	u32 cgain_off    :  1; /* [0] */
	u32 byp_c_lpf    :  1; /* [1] */
	u32 byp_c_ups    :  1; /* [2] */
	u32 reserved     :  5; /* [7:3] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve99_t;

//read only
typedef union {
	struct {
	u32 ed_stat_full :  1; /* [0] */
	u32 cc_stat_full :  1; /* [1] */
	u32 reserved     :  6; /* [7:2] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve128_t;

typedef struct {
	u32		unsued1[32];
	u32		phi_7_0;
	u32		phi_15_8;
	u32		phi_16_23;
	u32		phi_24_31;
	u32		sctoh_7_0;
	u32		sctoh_15_8;
	u32		sctoh_23_16;
	u32		sctoh_31_24;
	dve40_t		dve_40;
	u32		unused2;
	u32		black_lvl;
	u32		blank_lvl;
	u32		clamp_lvl;
	u32		sync_lvl;
	dve46_t		dve_46;
	dve47_t		dve_47;
	u32		unused3[2];
	u32		nba;
	u32		pba;
	dve52_t		dve_52;
	u32		unused4[3];
	dve56_t		dve_56;
	dve57_t		dve_57;
	u32		vso_7_0;
	u32		hso_10_8;
	u32		hso_7_0;
	u32		hcl_9_8;
	u32		hcl_7_0;
	u32		unused5[2];
	u32		ccd_odd_15_8;
	u32		ccd_odd_7_0;
	u32		ccd_even_15_8;
	u32		ccd_even_7_0;
	u32		cc_enbl;
	u32		unused6[2];
	u32		mvfcr;
	u32		mvcsl1_5_0;
	u32		mvcls1_5_0;
	u32		mvcsl2_5_0;
	u32		mvcls2_5_0;
	dve77_t		dve_77;
	u32		mvpsd_5_0;
	u32		mvpsl_5_0;
	u32		mvpss_5_0;
	u32		mvpsls_14_8;
	u32		mvpsls_7_0;
	u32		mvpsfs_14_8;
	u32		mvpsfs;
	u32		mvpsagca;
	u32		mvpsagcb;
	u32		mveofbpc;
	dve88_t		dve_88;
	dve89_t		dve_89;
	u32		mvpcslimd_7_0;
	u32		mvpcslimd_9_8;
	u32		unused7[4];
	dve96_t		dve_96;
	dve97_t		dve_97;
	u32		unused8;
	dve99_t		dve_99;
	u32		mvtms_3_0;
	u32		unused9[8];
	u32		hlr_9_8;
	u32		hlr_7_0;
	u32		vsmr_4_0;
	u32		unused10[4];
	u32		dve116;
	u32		dve117;
	u32		dve118;
	u32		dve119;
	u32		dve120;
	u32		dve121;
	u32		dve122;
	u32		dve123;
	u32		dve124;
	u32		unused11[3];
	dve128_t	dve_r128;
} dram_dve_t;

/**
 * The structure is set by the LCD driver in the BSP,
 * then pass to Ambarella VOUT controller for setting these values.
 */
typedef struct lcd_dev_vout_cfg_s {
	int lcd_sync;		/**> 0 - 601, 1 - 656 */
/* Input video formats */
#define INPUT_FORMAT_601	0
#define INPUT_FORMAT_656	1
#define INPUT_DOUT_601_24BITS	6

	int color_space;	/* color space */
#define CS_RGB			0
#define CS_YCBCR		1
#define CS_YUV			1
	u8 hs_polarity;
	u8 vs_polarity;
	u8 de_polarity;
#define ACTIVE_LO		0
#define ACTIVE_HI		1
	u8 data_poliarity;
#define RISING			0
#define FALLING			1

	int ftime_hs;		/**> Number of VD_CLK periods per H-Sync */
	int ftime_vs_top;	/**> Number of H-Sync per video field0 */
	int ftime_vs_bot;	/**> Number of H-Sync per video field1 */

	u32 hs_start;		/**> Hsync pulse start */
	u32 hs_end;		/**> Hsync pulse end */
	u32 vs_start_col_top;	/**> Vsync pulse start column */
	u32 vs_start_col_bot;	/**> Vsync pulse start column */
	u32 vs_end_col_top;	/**> Hsync pulse end column */
	u32 vs_end_col_bot;	/**> Hsync pulse end column */
	u32 vs_start_row_top;	/**> Vsync pulse start row */
	u32 vs_start_row_bot;	/**> Vsync pulse start row */
	u32 vs_end_row_top;	/**> Vsync pulse end row */
	u32 vs_end_row_bot;	/**> Vsync pulse end row */

	/**> Note that only A6 silicon can set the active windows
	     for the top field and bottom field separately.
	     On silicons expcet A6, the settings for top field are
	     applied to the ctive windows of both fields. */
	u16 act_start_col_top;	/**> Vsync active start col */
	u16 act_start_col_bot;	/**> Vsync active start col */
	u16 act_end_col_top;	/**> Vsync active end col */
	u16 act_end_col_bot;	/**> Vsync active end col */
	u16 act_start_row_top;	/**> Vsync active start row */
	u16 act_start_row_bot;	/**> Vsync active start row */
	u16 act_end_row_top;	/**> Vsync active end row */
	u16 act_end_row_bot;	/**> Vsync active end row */

	int bg_color;		/**> Background color */
/* Vout background colors */
#define BG_RED		VOUT_BG_RED
#define BG_MAGENTA	VOUT_BG_MAGENTA
#define BG_YELLOW	VOUT_BG_YELLOW
#define BG_GREEN	VOUT_BG_GREEN
#define BG_BLUE		VOUT_BG_BLUE
#define BG_CYAN		VOUT_BG_CYAN
#define BG_WHITE	VOUT_BG_WHITE
#define BG_BLACK	VOUT_BG_BLACK

	/* The following fields are valid for RGB color space */
	int lcd_display;	/**> 0 - progressive, 1 - interlaced */
/* LCD display types */
#define LCD_NTSC_DISPLAY	0
#define LCD_PAL_DISPLAY		1
#define LCD_PROG_DISPLAY	2       /* Progressive */

	int lcd_frame_rate;	/**> frame rate */

 	int lcd_data_clk;	/**> Data clock to device */
/** VOUT clock rates */
#define CLK_DCLK_FREQ_1_TO_1		DCLK_FREQ_1_TO_1
#define CLK_DCLK_FREQ_2_TO_1		DCLK_FREQ_2_TO_1
#define CLK_DCLK_FREQ_4_TO_1		DCLK_FREQ_2_TO_1

	int lcd_color_mode;	/**> RGB color mode*/
	int lcd_rgb_seq_top;	/**> RGB color sequence */
	int lcd_rgb_seq_bot;	/**> RGB color sequence */
	int lcd_hscan;		/**> horizontal scan type */

	int osd_width;		/**> OSD display width */
	int osd_height;		/**> OSD display height */
	int osd_resolution;	/**> OSD display resolution */

	int clock_hz;

	int use_dve;
	dram_dve_t dve;	/* Digital Video Encoder */
} lcd_dev_vout_cfg_t;

/**
 * This function is to set backgorund color
 */
extern void vout_set_monitor_bg_color(int chan, u8 y, u8 cb, u8 cr);

/**
 * The parameters are setting to VOUT control for specific lcd
 */
extern int vout_config(int chan, lcd_dev_vout_cfg_t *cfg);

/**
 * Get the osd object pointer
 */
extern osd_t* get_osd_obj(int chan);

/**
 * This function is to set color space conversion
 *
 * @ param chan - channel ID
 * @ param csc_mode - CSC mode
 * 	VO_CSC_YUVSD2YUVHD	YUV601 -> YUV709
 * 	VO_CSC_YUVSD2YUVSD	YUV601 -> YUV601
 * 	VO_CSC_YUVSD2RGB	YUV601 -> RGB
 * 	VO_CSC_YUVHD2YUVSD	YUV709 -> YUV601
 * 	VO_CSC_YUVHD2YUVHD	YUV709 -> YUV709
 * 	VO_CSC_YUVHD2RGB	YUV709 -> RGB
 * @ param csc_clamp - CSC data output clamping
 *	VO_DATARANGE_DIGITAL_HD_FULL	YUV = [0~ 255]  ( 8 bit)
 *	VO_DATARANGE_DIGITAL_SD_FULL	YUV = [0~ 255]  ( 8 bit)
 *	VO_DATARANGE_DIGITAL_HD_CLAMP	Y = [16~235] UV = [16~240] ( 8 bit)
 *	VO_DATARANGE_DIGITAL_SD_CLAMP	Y = [16~235] UV = [16~240] ( 8 bit)
 **/
#if (VOUT_DIRECT_DSP_INTERFACE == 1)
extern void vout_set_csc(int chan, int path, u8 csc_mode, u8 csc_clamp);
#else
extern void vout_set_csc(int chan, u8 csc_mode, u8 csc_clamp);
#endif

#ifdef CONFIG_HDMI_MODE_480P
extern int hdmi_set_vout_ctrl_480p(lcd_dev_vout_cfg_t *vout_cfg);
extern void hdmi_config_480p(void);
#endif

#ifdef CONFIG_HDMI_MODE_576P
extern int hdmi_set_vout_ctrl_576p(lcd_dev_vout_cfg_t *vout_cfg);
extern void hdmi_config_576p(void);
#endif

#ifdef CONFIG_CVBS_MODE_480I
extern int cvbs_set_vout_ctrl_480i(lcd_dev_vout_cfg_t *vout_cfg);
extern void cvbs_config_480i(dram_dve_t *dve);
#endif

#ifdef CONFIG_CVBS_MODE_576I
extern int cvbs_set_vout_ctrl_576i(lcd_dev_vout_cfg_t *vout_cfg);
extern void cvbs_config_576i(dram_dve_t *dve);
#endif

#endif	/* __VOUT_H__ */
