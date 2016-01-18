/*
 * kernel/private/include/arch_a7/amba_arch_vin.h
 *
 * History:
 *    2010/06/21 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_ARCH_VIN_H
#define __AMBA_ARCH_VIN_H


    /* amba_vin_adap_config related defines */
#define VIN_DONT_CARE 	0	/**< use this to indicate, some bits is useless in related vin mode */

	//u8 hsync_mask;		/**< Toggle Hsync during vertical blankding */
#define HSYNC_MASK_CLR		0
#define HSYNC_MASK_SET		1

	//u8 sony_field_mode;		/**< Select to use SONY sensor specific field mode, or normal field mode */
#define AMBA_VIDEO_ADAP_NORMAL_FIELD		(0)
#define AMBA_VIDEO_ADAP_SONY_SPECIFIC_FIELD	(1)

	//u8 field0_pol;			/**< Select field 0 polarity under SONY field mode */

	//u8 vs_polarity;			/**< Select polarity of V-Sync */
	//u8 hs_polarity;			/**< Select polarity of H-Sync */
#define AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH		(0)/* high means rising edge, low means falling edge */
#define AMBA_VIDEO_ADAP_VD_HIGH_HD_LOW		(1)
#define AMBA_VIDEO_ADAP_VD_LOW_HD_HIGH		(2)
#define AMBA_VIDEO_ADAP_VD_LOW_HD_LOW		(3)

	//u8 emb_sync_loc;		/**< Embedded sync code location */
#define EMB_SYNC_LOWER_PEL      0
#define EMB_SYNC_UPPER_PEL      1
#define EMB_SYNC_BOTH_PELS      2

	//u8 emb_sync_mode;
#define EMB_SYNC_ITU_656        0
#define EMB_SYNC_ALL_ZEROS      1

	//u8 emb_sync;
#define EMB_SYNC_OFF		    0
#define EMB_SYNC_ON		        1

	//u8 sync_mode;
#define SYNC_MODE_UNKNOWN       0
#define SYNC_MODE_SLAVE         1
#define SYNC_MODE_MASTER        2
#define SYNC_MODE_SONY_SPECIFIC 3

	//u8 data_edge;			/**< Select if video input data is valid on rising edge or falling edge of SPCLK */
#define PCLK_RISING_EDGE	0
#define PCLK_FALLING_EDGE	1

	/* S_InputConfig */
	//u8 src_data_width;		/**< Pixel values is MSB aligned */
#define SRC_DATA_8B           	0
#define SRC_DATA_10B           	1
#define SRC_DATA_12B           	2
#define SRC_DATA_14B            3

	//u16 input_mode;
#define VIN_RGB_LVDS_1PEL_SDR_LVCMOS   		0x0000
#define VIN_RGB_LVDS_1PEL_SDR_LVDS    		0x0001
#define VIN_RGB_LVDS_1PEL_DDR_LVCMOS   		0x0002
#define VIN_RGB_LVDS_1PEL_DDR_LVDS		    0x0003
#define VIN_RGB_LVDS_2PELS_SDR_LVCMOS  		0x0004
#define VIN_RGB_LVDS_2PELS_DDR_LVCMOS  		0x0006
#define VIN_YUV_LVDS_1PEL_SDR_CR_Y0_CB_Y1_LVCMOS	0x0010
#define VIN_YUV_LVDS_1PEL_SDR_CB_Y0_CR_Y1_LVCMOS	0x0090
#define VIN_YUV_LVDS_1PEL_SDR_Y0_CR_Y1_CB_LVCMOS	0x0110
#define VIN_YUV_LVDS_1PEL_SDR_Y0_CB_Y1_CR_LVCMOS	0x0190
#define VIN_YUV_LVDS_1PEL_SDR_CR_Y0_CB_Y1_LVDS		0x0011
#define VIN_YUV_LVDS_1PEL_SDR_CB_Y0_CR_Y1_LVDS		0x0091
#define VIN_YUV_LVDS_1PEL_SDR_Y0_CR_Y1_CB_LVDS		0x0111
#define VIN_YUV_LVDS_1PEL_SDR_Y0_CB_Y1_CR_LVDS		0x0191
#define VIN_YUV_LVDS_1PEL_DDR_CR_Y0_CB_Y1_LVCMOS 	0x0012
#define VIN_YUV_LVDS_1PEL_DDR_CB_Y0_CR_Y1_LVCMOS 	0x0092
#define VIN_YUV_LVDS_1PEL_DDR_Y0_CR_Y1_CB_LVCMOS 	0x0112
#define VIN_YUV_LVDS_1PEL_DDR_Y0_CB_Y1_CR_LVCMOS 	0x0192
#define VIN_YUV_LVDS_1PEL_DDR_CR_Y0_CB_Y1_LVDS		0x0013
#define VIN_YUV_LVDS_1PEL_DDR_CB_Y0_CR_Y1_LVDS		0x0093
#define VIN_YUV_LVDS_1PEL_DDR_Y0_CR_Y1_CB_LVDS		0x0113
#define VIN_YUV_LVDS_1PEL_DDR_Y0_CB_Y1_CR_LVDS		0x0193
#define VIN_YUV_LVDS_2PELS_SDR_CRY0_CBY1_LVCMOS 0x0014
#define VIN_YUV_LVDS_2PELS_SDR_CBY0_CRY1_LVCMOS 0x0094
#define VIN_YUV_LVDS_2PELS_SDR_Y0CR_Y1CB_LVCMOS 0x0114
#define VIN_YUV_LVDS_2PELS_SDR_Y0CB_Y1CR_LVCMOS 0x0194
#define VIN_YUV_LVDS_2PELS_DDR_CRY0_CBY1_LVCMOS 0x0016
#define VIN_YUV_LVDS_2PELS_DDR_CBY0_CRY1_LVCMOS 0x0096
#define VIN_YUV_LVDS_2PELS_DDR_Y0CR_Y1CB_LVCMOS 0x0116
#define VIN_YUV_LVDS_2PELS_DDR_Y0CB_Y1CR_LVCMOS 0x0196
#define VIN_1PEL_CR_Y0_CB_Y1_IOPAD    		0x0018
#define VIN_1PEL_CB_Y0_CR_Y1_IOPAD    		0x0098
#define VIN_1PEL_Y0_CR_Y1_CB_IOPAD   		0x0118
#define VIN_1PEL_Y0_CB_Y1_CR_IOPAD    		0x0198
#define VIN_2PELS_CRY0_CBY1_IOPAD    		0x001c
#define VIN_2PELS_CBY0_CRY1_IOPAD    		0x009c
#define VIN_2PELS_Y0CR_Y1CB_IOPAD   		0x011c
#define VIN_2PELS_Y0CB_Y1CR_IOPAD   		0x019c
    /* For SLVS/MLVS only */
#define VIN_RGB_LVDS_2PELS_DDR_SLVS  			0x0006
    /* For MIPI input mode only */
#define VIN_RGB_LVDS_2PELS_DDR_MIPI			0x0006
#define VIN_YUV_LVDS_2PELS_DDR_MIPI			0x0096
    //u8    mipi_act_lanes;         /**< Number of active SLVS lanes */
#define MIPI_1LANE		0
#define MIPI_2LANE		1
#define MIPI_3LANE		2
#define MIPI_4LANE		3
    //u8    serial_mode;
#define SERIAL_VIN_MODE_DISABLE	0
#define SERIAL_VIN_MODE_ENABLE	1
    //u8    sony_slvs_mode;
#define SONY_SLVS_MODE_DISABLE	0
#define SONY_SLVS_MODE_ENABLE	1
    //u8    clk_select_slvs;
#define	CLK_SELECT_MIPICLK	0
#define	CLK_SELECT_SPCLK	1
    //u16   slvs_eav_col;           /**  EAV column of SLVS mode */
    //u16   slvs_sav2sav_dist;      /**  SAV to SAV distance */
/* A5S MIPI support */
    /* offset = 26 */
    //u8    mipi_virtualch_select;
    //u8    mipi_virtualch_mask;
    //u8    mipi_enable;            /** MIPI Enable */
#define MIPI_VIN_MODE_DISABLE	0
#define MIPI_VIN_MODE_ENABLE	1
    /*u8    mipi_s_hssettlectl;      HS-RX settle time control register
       bit[4] controls digital or analog
       HS-RX settle time,
       bit[3:0] are counter values for
       bypass mode.(digital) */
#define MIPI_HSSETTLECTL	22
    //u8    mipi_s_dpdn_swap_clk;
#define MIPI_DPDN_SWAP_CLK_OFF	0
#define MIPI_DPDN_SWAP_CLK_ON	1
    //u8    mipi_s_dpdn_swap_data;
#define MIPI_DPDN_SWAP_DATA_OFF	0
#define MIPI_DPDN_SWAP_DATA_ON	1
    //u8    mipi_s_clksettlectl;
    //u8    mipi_decompression_mode;
#define SIMPLE_PREDICTOR_10810		0
#define ADVANCE_PREDICTOR_10810		1
#define SIMPLE_PREDICTOR_12812		2
#define ADVANCE_PREDICTOR_12812		3
    /* offset = 27 */
    //u8    mipi_data_type_sel;     /**<Select Data type */
#define YUV422_8BIT		0x1e
#define RAW_8BIT		0x2a
#define RAW_10BIT		0x2b
#define RAW_12BIT		0x2c
#define RAW_14BIT		0x2d
#define GENERIC_8BIT_NULL	0x10
#define GENERIC_8BIT_BLANKING	0x11
#define GENERIC_8BIT_EMBEDDED	0x12
    //u8    mipi_data_type_mask;    /**<Defines which long packets are expected */
    //u8    mipi_pixel_byte_swap;   /**<byte 0 will be swapped with byte 1, byte 2 will be swapped with byte 3, this bit is only set for YUV422 */
#define PIXEL_BYTE_SWAP_ON	1
#define PIXEL_BYTE_SWAP_OFF	0
    //u8    mipi_decompression_enable; /**<Decompression mode enable, default 0 (off)*/
#define DECOMPRESSION_ON	1
#define DECOMPRESSION_OFF	0

struct amba_vin_adap_config {
	/* -------------------S_Control register related----------------*/
	u8 hsync_mask;		/**< Toggle Hsync during vertical blankding */
//#define HSYNC_MASK_CLR		0
//#define HSYNC_MASK_SET		1
	u8 ecc_enable;			/**< Enable 656 error correction */
	u8 sony_field_mode;		/**< Select to use SONY sensor specific field mode, or normal field mode */
//#define AMBA_VIDEO_ADAP_NORMAL_FIELD		(0)
//#define AMBA_VIDEO_ADAP_SONY_SPECIFIC_FIELD	(1)

	u8 field0_pol;			/**< Select field 0 polarity under SONY field mode */

	u8 vs_polarity;			/**< Select polarity of V-Sync */
	u8 hs_polarity;			/**< Select polarity of H-Sync */
//#define AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH		(0)
//#define AMBA_VIDEO_ADAP_VD_HIGH_HD_LOW		(1)
//#define AMBA_VIDEO_ADAP_VD_LOW_HD_HIGH		(2)
//#define AMBA_VIDEO_ADAP_VD_LOW_HD_LOW		(3)

	u8 emb_sync_loc;		/**< Embedded sync code location */
//#define EMB_SYNC_LOWER_PEL      0
//#define EMB_SYNC_UPPER_PEL      1
//#define EMB_SYNC_BOTH_PELS      2

	u8 emb_sync_mode;
//#define EMB_SYNC_ITU_656        0
//#define EMB_SYNC_ALL_ZEROS      1

	u8 emb_sync;
//#define EMB_SYNC_OFF		    	0
//#define EMB_SYNC_ON		        1

	u8 sync_mode;
//#define SYNC_MODE_UNKNOWN       0
//#define SYNC_MODE_SLAVE         1
//#define SYNC_MODE_MASTER        2
//#define SYNC_MODE_SONY_SPECIFIC 3

	u8 data_edge;			/**< Select if video input data is valid on rising edge or falling edge of SPCLK */
//#define PCLK_RISING_EDGE	0
//#define PCLK_FALLING_EDGE	1

	/* -------------------S_InputConfig register related---------------*/
	u8 src_data_width;		/**< Pixel values is MSB aligned */
//#define SRC_DATA_8B           	0
//#define SRC_DATA_10B           	1
//#define SRC_DATA_12B           	2
//#define SRC_DATA_14B            	3

	u16 input_mode;
//#define VIN_RGB_LVDS_1PEL_SDR_LVCMOS   		0x0000
//#define VIN_RGB_LVDS_1PEL_SDR_LVDS    		0x0001
//#define VIN_RGB_LVDS_1PEL_DDR_LVCMOS   		0x0002
//#define VIN_RGB_LVDS_1PEL_DDR_LVDS		    0x0003
//#define VIN_RGB_LVDS_2PELS_SDR_LVCMOS  		0x0004
//#define VIN_RGB_LVDS_2PELS_DDR_LVCMOS  		0x0006
//#define VIN_YUV_LVDS_1PEL_SDR_CR_Y0_CB_Y1_LVCMOS      0x0010
//#define VIN_YUV_LVDS_1PEL_SDR_CB_Y0_CR_Y1_LVCMOS      0x0090
//#define VIN_YUV_LVDS_1PEL_SDR_Y0_CR_Y1_CB_LVCMOS      0x0110
//#define VIN_YUV_LVDS_1PEL_SDR_Y0_CB_Y1_CR_LVCMOS      0x0190
//#define VIN_YUV_LVDS_1PEL_SDR_CR_Y0_CB_Y1_LVDS                0x0011
//#define VIN_YUV_LVDS_1PEL_SDR_CB_Y0_CR_Y1_LVDS                0x0091
//#define VIN_YUV_LVDS_1PEL_SDR_Y0_CR_Y1_CB_LVDS                0x0111
//#define VIN_YUV_LVDS_1PEL_SDR_Y0_CB_Y1_CR_LVDS                0x0191
//#define VIN_YUV_LVDS_1PEL_DDR_CR_Y0_CB_Y1_LVCMOS      0x0012
//#define VIN_YUV_LVDS_1PEL_DDR_CB_Y0_CR_Y1_LVCMOS      0x0092
//#define VIN_YUV_LVDS_1PEL_DDR_Y0_CR_Y1_CB_LVCMOS      0x0112
//#define VIN_YUV_LVDS_1PEL_DDR_Y0_CB_Y1_CR_LVCMOS      0x0192
//#define VIN_YUV_LVDS_1PEL_DDR_CR_Y0_CB_Y1_LVDS                0x0013
//#define VIN_YUV_LVDS_1PEL_DDR_CB_Y0_CR_Y1_LVDS                0x0093
//#define VIN_YUV_LVDS_1PEL_DDR_Y0_CR_Y1_CB_LVDS                0x0113
//#define VIN_YUV_LVDS_1PEL_DDR_Y0_CB_Y1_CR_LVDS                0x0193
//#define VIN_YUV_LVDS_2PELS_SDR_CRY0_CBY1_LVCMOS       0x0014
//#define VIN_YUV_LVDS_2PELS_SDR_CBY0_CRY1_LVCMOS       0x0094
//#define VIN_YUV_LVDS_2PELS_SDR_Y0CR_Y1CB_LVCMOS       0x0114
//#define VIN_YUV_LVDS_2PELS_SDR_Y0CB_Y1CR_LVCMOS 0x0194
//#define VIN_YUV_LVDS_2PELS_DDR_CRY0_CBY1_LVCMOS 0x0016
//#define VIN_YUV_LVDS_2PELS_DDR_CBY0_CRY1_LVCMOS 0x0096
//#define VIN_YUV_LVDS_2PELS_DDR_Y0CR_Y1CB_LVCMOS 0x0116
//#define VIN_YUV_LVDS_2PELS_DDR_Y0CB_Y1CR_LVCMOS 0x0196
//#define VIN_1PEL_CR_Y0_CB_Y1_IOPAD    		0x0018
//#define VIN_1PEL_CB_Y0_CR_Y1_IOPAD    		0x0098
//#define VIN_1PEL_Y0_CR_Y1_CB_IOPAD   		0x0118
//#define VIN_1PEL_Y0_CB_Y1_CR_IOPAD    		0x0198
//#define VIN_2PELS_CRY0_CBY1_IOPAD    		0x001c
//#define VIN_2PELS_CBY0_CRY1_IOPAD    		0x009c
//#define VIN_2PELS_Y0CR_Y1CB_IOPAD   		0x011c
//#define VIN_2PELS_Y0CB_Y1CR_IOPAD   		0x019c
	/* For SLVS/MLVS only */
//#define VIN_RGB_LVDS_2PELS_DDR_SLVS                   0x0006

	/* For MIPI input mode only */
//#define VIN_RGB_LVDS_2PELS_DDR_MIPI                   0x0006
//#define VIN_YUV_LVDS_2PELS_DDR_MIPI                   0x0096

	u8 mipi_act_lanes;		/**< Number of active SLVS lanes */
//#define MIPI_1LANE            0
//#define MIPI_2LANE            1
//#define MIPI_3LANE            2
//#define MIPI_4LANE            3

	u8 serial_mode;
//#define SERIAL_VIN_MODE_DISABLE       0
//#define SERIAL_VIN_MODE_ENABLE        1

	u8 sony_slvs_mode;
//#define SONY_SLVS_MODE_DISABLE        0
//#define SONY_SLVS_MODE_ENABLE 1

	u8 clk_select_slvs;
//#define       CLK_SELECT_MIPICLK      0
//#define       CLK_SELECT_SPCLK        1

	u16 slvs_eav_col;		/**  EAV column of SLVS mode */
	u16 slvs_sav2sav_dist;		/**  SAV to SAV distance */

/* A5S MIPI support */

	/* offset = 26 */
	u8 mipi_virtualch_select;
	u8 mipi_virtualch_mask;
	u8 mipi_enable;			/** MIPI Enable */
//#define MIPI_VIN_MODE_DISABLE 0
//#define MIPI_VIN_MODE_ENABLE  1
	u8 mipi_s_hssettlectl;		/** HS-RX settle time control register
					    bit[4] controls digital or analog
					    HS-RX settle time,
					    bit[3:0] are counter values for
					    bypass mode.(digital)*/
//#define MIPI_HSSETTLECTL      22
	u8 mipi_s_dpdn_swap_clk;
//#define MIPI_DPDN_SWAP_CLK_OFF        0
//#define MIPI_DPDN_SWAP_CLK_ON 1
	u8 mipi_s_dpdn_swap_data;
//#define MIPI_DPDN_SWAP_DATA_OFF       0
//#define MIPI_DPDN_SWAP_DATA_ON        1
	u8 mipi_s_clksettlectl;
	u8 mipi_decompression_mode;
//#define SIMPLE_PREDICTOR_10810                0
//#define ADVANCE_PREDICTOR_10810               1
//#define SIMPLE_PREDICTOR_12812                2
//#define ADVANCE_PREDICTOR_12812               3

	/* offset = 27 */
	u8 mipi_data_type_sel;		/**<Select Data type */
//#define YUV422_8BIT           0x1e
//#define RAW_8BIT              0x2a
//#define RAW_10BIT             0x2b
//#define RAW_12BIT             0x2c
//#define RAW_14BIT             0x2d
//#define GENERIC_8BIT_NULL     0x10
//#define GENERIC_8BIT_BLANKING 0x11
//#define GENERIC_8BIT_EMBEDDED 0x12
	u8 mipi_data_type_mask;		/**<Defines which long packets are expected */
	u8 mipi_pixel_byte_swap;	/**<byte 0 will be swapped with byte 1,
								    byte 2 will be swapped with byte 3,
								    this bit is only set for YUV422 */
//#define PIXEL_BYTE_SWAP_ON    1
//#define PIXEL_BYTE_SWAP_OFF   0

	u8 mipi_decompression_enable; /**<Decompression mode enable, default 0 (off)*/
//#define DECOMPRESSION_ON      1
//#define DECOMPRESSION_OFF     0
	/* offset = 28 */
	u16 mipi_b_dphyctl;	/*D-phy Master Analog block control */
	/* offset = 29 */
	u16 mipi_s_dphyctl;
	/* offset = 30 */
	u16 mipi_error_status;
	/* offset = 31 */
	u16 mipi_counts_frames;
	/* offset = 37 */
	u16 mipi_frame_number;
	/* offset = 38 */
	u16 mipi_phy_status;

	//for SLVDS control
	u16 slvs_control;
	u16 slvs_frame_line_w;
	u16 slvs_act_frame_line_w;
	u16 slvs_vsync_max;
	u16 slvs_lane_mux_select[4];
	u16 slvs_status;
	u16 slvs_line_sync_timeout;
	u16 slvs_debug;
	//for sync code remapping
	u8 enhance_mode;
	u8 syncmap_mode;
	u8 s_slvs_ctrl_1;
	u16 s_slvs_sav_vzero_map;
	u16 s_slvs_sav_vone_map;
	u16 s_slvs_eav_vzero_map;
	u16 s_slvs_eav_vone_map;
};

struct amba_vin_cap_win_info {
	u16 s_ctrl_reg;
	u16 s_inp_cfg_reg;
	u16 prog_opt            :4;
	u16 num_extra_vin       :4;
	u16 reserved            :8;
	u16 s_v_width_reg;
	u16 s_h_width_reg;
	u16 s_h_offset_top_reg;
	u16 s_h_offset_bot_reg;
	u16 s_v_reg;
	u16 s_h_reg;
	u16 s_min_v_reg;
	u16 s_min_h_reg;
	u16 s_trigger_0_start_reg;
	u16 s_trigger_0_end_reg;
	u16 s_trigger_1_start_reg;
	u16 s_trigger_1_end_reg;
	u16 s_vout_start_0_reg;
	u16 s_vout_start_1_reg;
	u16 s_cap_start_v_reg;
	u16 s_cap_start_h_reg;
	u16 s_cap_end_v_reg;
	u16 s_cap_end_h_reg;
	u16 s_blank_leng_h_reg;
	u16 s_timeout_v_low_reg;
	u16 s_timeout_v_high_reg;
	u16 s_timeout_h_low_reg;
	u16 s_timeout_h_high_reg;
	u16 mipi_cfg1_reg;
	u16 mipi_cfg2_reg;
	u16 mipi_bdphyctl_reg;
	u16 mipi_sdphyctl_reg;
	u32 extra_vin_setup_addr[4];
	u32 rawH;
	u8 is_turbo_ena;
	u16 slvs_control;
	u16 slvs_frame_line_w;
	u16 slvs_act_frame_line_w;
	u16 slvs_vsync_max;
	u16 slvs_lane_mux_select[4];
	u16 slvs_status;
	u16 slvs_line_sync_timeout;
	u16 slvs_debug;
	u8 enhance_mode;
	u8 syncmap_mode;
	u8 s_slvs_ctrl_1;
	u16 s_slvs_sav_vzero_map;
	u16 s_slvs_sav_vone_map;
	u16 s_slvs_eav_vzero_map;
	u16 s_slvs_eav_vone_map;
	u16 s_slvs_vsync_horizontal_start;
	u16 s_slvs_vsync_horizontal_end;
};

struct amba_vin_src_capability {
	u8 input_type;		/**< Ref AMBA_VIDEO_TYPE */
	u8 dev_type;		/**< Ref AMBA_VIN_SRC_DEV_TYPE */
	u8 reserved[2];
	u32 frame_rate;		/**< Ref AMBA_VIDEO_FPS */
	u8 video_format;	/**< Ref AMBA_VIDEO_FORMAT */
	u8 bit_resolution;	/**< Ref AMBA_VIDEO_BITS */
	u8 aspect_ratio;	/**< Ref AMBA_VIDEO_RATIO */
	u8 video_system;	/**< Ref AMBA_VIDEO_SYSTEM */
	u8 ext_fps[AMBA_VIN_MAX_FPS_TABLE_SIZE];/**< Ref AMBA_VIDEO_FPS */

	u16 max_width;		/**< The max width in pixels of input source */
	u16 max_height;		/**< The max height in lines of input source */
	u16 def_cap_w;		/**< Default video input capture width */
	u16 def_cap_h;		/**< Default video input capture height */
	u16 cap_start_x;	/**< Default video input capture start x */
	u16 cap_start_y;	/**< Default video input capture start y */
	u16 cap_cap_w;		/**< Default video input capture width */
	u16 cap_cap_h;		/**< Default video input capture height */
	u16 act_start_x;		/**< Active video input capture start x */
	u16 act_start_y;		/**< Active video input capture start y */
	u16 act_width;		/**< Active video input capture width */
	u16 act_height;		/**< Active video input capture height */

	/* Valid when dev_type == AMBA_VIN_SRC_DEV_TYPE_CMOS or AMBA_VIN_SRC_DEV_TYPE_CCD
	   Ref DSP CMD(0x00002001 & 0x00003001) */
	u8 sensor_id;
	u8 bayer_pattern;
	u8 field_format;
	u8 active_capinfo_num;
	u8 bin_max;
	u8 skip_max;
	u8 input_format;
	u8 column_bin;
	u8 row_bin;
	u8 column_skip;
	u8 row_skip;
	u8 hdr_data_read_protocol;
	u32 sensor_readout_mode;

	u32 mode_type;
	u32 slave_mode;
	enum amba_video_mode current_vin_mode;
	u32 current_shutter_time;
	s32 current_gain_db;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32 current_fps;

	struct amba_vin_cap_win_info vin_cap_info;
};

#endif				//__AMBA_ARCH_VIN_H
