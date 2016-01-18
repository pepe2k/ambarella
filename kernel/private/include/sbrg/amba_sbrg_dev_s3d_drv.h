/*
 * kernel/private/include/sbrg/amba_sbrg_dev_s3d_drv.h
 *
 * History:
 *    2011/06/17 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_SBRG_DEV_S3D_DRV_H
#define __AMBA_SBRG_DEV_S3D_DRV_H

typedef struct sbrg_reg_3d_cfg_s {

	u16	addr;
	u16	data;
} sbrg_reg_3d_cfg_t;

/**
 * Structure of the sbrg s3d rct configuration.
 */

#define NWORDS(x)			sizeof(x) / sizeof(u16)

/* define RCT VOUTF SSTL CTRL */
#define RCT_VOUTF_SSTL_6		0x661
/**
 * Structure of the sbrg RCT configuration.
 */
typedef struct sbrg_rct_3d_cfg_s {

	u8	clk_si_sel;		/**< Sensor clk referance */
#define	CLKSI_SORCE_CLK_REF	0
#define	CLKSI_SORCE_PLL_OUT	1

	u8	gclk_core_sel;		/**< S3D Voutf referance */
#define	CORE_SORCE_CLK_REF	0
#define	CORE_SORCE_PLL_OUT	1

	u32	input_freq_hz;		/**< S3D input clk */

	u32	output_freq_hz;		/**< S3D output clk */

} sbrg_rct_3d_cfg_t;

/**
 * Structure of the s3d mipi phy configuration.
 */
typedef struct sbrg_rct_3d_mipi_phy_cfg_s {

	u16	mipi_phy_ctrl_1;
#define LVCMOS_INPUT_PAD		0x03ff
#define LVDS_INPUT_PAD			0x0000
#define LVDS_INPUT_8_LANES_PAD		0x0000
#define LVDS_INPUT_4_LANES_PAD		0x02f0

	u16	mipi_phy_ctrl_2;

	u16	mipi_phy_ctrl_3;
#define RCT_MIPI_PHY_3_7_0_1_1		0x711
#define RCT_MIPI_PHY_3_POWER_DOWN	0x800

} sbrg_rct_3d_mipi_phy_cfg_t;


/**
 * Structure of the sbrg IDC configuration.
 */
typedef struct sbrg_idc_3d_cfg_s {

	u8 sbrg_wr_sel;
#define SBRG_S3D_IDC_WRITE_BOTH			0
#define SBRG_S3D_IDC_DISABLE_WRITE_SDA0		1
#define SBRG_S3D_IDC_DISABLE_WRITE_SDA1		2
#define SBRG_S3D_IDC_DISABLE_WRITE_BOTH		3

	u8 sbrg_rd_sel;
#define SBRG_S3D_IDC_READ_SDA0	0
#define SBRG_S3D_IDC_READ_SDA1	1

} sbrg_idc_3d_cfg_t;

/**
 * Structure of the sbrg SPI configuration.
 */
typedef struct sbrg_spi_3d_cfg_s {

	u16 sbrg_wr_sel;
#define	SBRG_S3D_SPI_L_ENABLE_HIGH_ACTIVE	0x1 /* 0x0001 */
#define	SBRG_S3D_SPI_L_ENABLE_LOW_ACTIVE	0x9 /* 0x1001 */
#define	SBRG_S3D_SPI_R_ENABLE_HIGH_ACTIVE	0x4 /* 0x0100 */
#define	SBRG_S3D_SPI_R_ENABLE_LOW_ACTIVE	0x6 /* 0x0110 */
#define	SBRG_S3D_SPI_BOTH_ENABLE_HIGH_ACTIVE	0x5 /* 0x0101 */
#define	SBRG_S3D_SPI_BOTH_ENABLE_LOW_ACTIVE	0xf /* 0x1111 */
#define	SBRG_S3D_SPI_BOTH_DISABLE_HIGH_ACTIVE	0x0 /* 0x0000 */
#define	SBRG_S3D_SPI_BOTH_DISABLE_LOW_ACTIVE	0xa /* 0x1010 */

	u8 sbrg_rd_sel;
#define SBRG_S3D_SPI_READ_SDA0		0
#define SBRG_S3D_SPI_READ_SDA1		1

	u8 sbrg_spi_en;
#define SBRG_S3D_IDC_ENABLE		0
#define SBRG_S3D_SPI_ENABLE		1

} sbrg_spi_3d_cfg_t;

/**
 * Structure of the sbrg vin 3d configuration.
 */
typedef struct sbrg_vin_3d_cfg_s {

	/* S_Control */
	u8	hsync_mask;
#define HSYNC_MASK_CLR		0
#define HSYNC_MASK_SET		1

	u8      ecc_enable;             /**< Enable 656 error correction */
	u8 	sony_field_mode;	/**< Select to use SONY sensor
					     specific field mode,
					     or normal field mode */
#define NORMAL_FIELD		0
#define SONY_SPECIFIC_FIELD	1

	u8	field0_pol;		/**< Select field 0 polarity under SONY
					     field mode */

	u8	vs_polarity;		/**< Select polarity of V-Sync */
#define VD_HIGH 		0
#define VD_LOW		   	1
	u8	hs_polarity;		/**< Select polarity of H-Sync */
#define HD_HIGH 		0
#define HD_LOW  		1

	u8      emb_sync_loc;           /**< Embedded sync code location */
#define EMB_SYNC_LOWER_PEL      0
#define EMB_SYNC_UPPER_PEL      1
#define EMB_SYNC_BOTH_PELS      2

	u8      emb_sync_mode;
#define EMB_SYNC_ITU_656        0
#define EMB_SYNC_ITU_656_FULL	1

	u8      emb_sync;
#define EMB_SYNC_OFF		0
#define EMB_SYNC_ON		1

	u8      sync_mode;
#define SYNC_MODE_UNKNOWN       0
#define SYNC_MODE_SLAVE         1
#define SYNC_MODE_MASTER        2
#define SYNC_MODE_UNDEFINED	3

	u8	data_edge;		/**< Select if video input data is
					     valid on rising edge or falling
				             edge of SPCLK */
#define PCLK_RISING_EDGE	0
#define PCLK_FALLING_EDGE	1

	/* S_InputConfig */
	u8	mipi_lanes;
#define MIPI_1LANE		0
#define MIPI_2LANE		1
#define MIPI_3LANE		2
#define MIPI_4LANE		3

	u8      src_data_width;         /**< Pixel values is MSB aligned */
#define SRC_DATA_8B           	0
#define SRC_DATA_10B           	1
#define SRC_DATA_12B           	2
#define SRC_DATA_14B            3

	u16      input_mode;
#define VIN_RGB_LVDS_1PEL_SDR_LVCMOS   			0x0000
#define VIN_RGB_LVDS_1PEL_SDR_LVDS    			0x0001
#define VIN_RGB_LVDS_1PEL_DDR_LVCMOS   			0x0002
#define VIN_RGB_LVDS_1PEL_DDR_LVDS			0x0003
#define VIN_RGB_LVDS_2PELS_SDR_LVCMOS  			0x0004
#define VIN_RGB_LVDS_2PELS_DDR_LVCMOS  			0x0006
	/* For SLVS/MLVS separate module only */
#define VIN_RGB_LVDS_1PEL_SDR_SLVS			0x0000
#define VIN_RGB_LVDS_1PEL_DDR_SLVS			0x0002
#define VIN_RGB_LVDS_2PELS_SDR_SLVS			0x0004
#define VIN_RGB_LVDS_2PELS_DDR_SLVS			0x0006
	/* For MIPI input mode only */
#define VIN_RGB_LVDS_2PELS_DDR_MIPI			0x0006
#define VIN_YUV_LVDS_2PELS_DDR_MIPI			0x0096

	u8	clk_select;
#define	CLK_SELECT_MIPICLK	0
#define	CLK_SELECT_GCLK_VIN	0
#define	CLK_SELECT_SPCLK	1

	u16	hsync_w;
	u16	vsync_w;

	u16	bottom_vsync_offset;
	u16	top_vsync_offset;

	u16	pel_clk_a_line;
	u16	line_num_a_field;

	u16	hs_min;
	u16	vs_min;

	u16	start_x;
	u16	start_y;
	u16	end_x;
	u16	end_y;

	u16	hb_length;

	u32	v_timeout_count;
	u32	h_timeout_count;

} sbrg_vin_3d_cfg_t;



/**
 * Structure of idsp command configuration.
 */
typedef struct sbrg_vin_3d_idspcmd_cfg_s {

	u8	ctl_vin_vsync_en;
#define	FRAME_END_TRIGGER_DISABLE	0
#define	FRAME_END_TRIGGER_ENABLE	1

	u8	ctl_vin_hsync_en;
#define	CTRL_TRIGGER_DISABLE		0
#define	CTRL_TRIGGER_ENABLE		1

	u8	mipi_enable3;
	u8	mipi_enable2;
	u8	mipi_enable1;
	u8	mipi_enable0;

	u8	lvdsspclk_pr;
	u8	gclksovin_pr;
	u8	vinmipi_clk_pr;
	u8	clkslvs_pol;
	u8	clkvin_pol;

	u8	mipi_clk;		/**< clk_slvs source selection: */
#define	SLVS_BLK_LVDS_IDSP_SPCLK	0
#define	SLVS_BLK_MIPI_CLK		1

	u8	input_clk;		/**< clk_vin source selection: */
#define	GCLK_SO_VIN			0
#define	LVDS_IDSP_SPCLK			1

} sbrg_vin_3d_idspcmd_cfg_t;

/**
 * Structure of vin_update configuration.
 */
typedef struct sbrg_vin_3d_update_cfg_s {

	u8	update_done_always;	/**< Generate update_done always high,
					     and user don't need to program
					     frame by frame. */
#define S3D_VIN_UPDATE_DONE_DISABLE	0
#define S3D_VIN_UPDATE_DONE_ALWAYS	1

	u8	update_done_once;	/**< Generate update_done signal
					     until update_enable goes high. */
#define S3D_VIN_UPDATE_DONE_ONCE	1

} sbrg_vin_3d_update_cfg_t;


/**
 * Structure of the sbrg vin 3d slvs configuration.
 */
typedef struct sbrg_vin_3d_slvs_cfg_s {

	/* offset 0x20 */
	u8	serial_mode;
#define SERIAL_VIN_MODE_DISABLE		0
#define SERIAL_VIN_MODE_ENABLE		1

	u8	sony_slvs_mode;
#define SONY_SLVS_MODE_DISABLE		0
#define SONY_SLVS_MODE_ENABLE		1

	u8	slvs_sync_repeat_all;
#define SYNC_REPEAT_ALL_DISABLE		0
#define SYNC_REPEAT_ALL_ENABLE		1

	u8	slvs_sync_repeat_2lanes;
#define SYNC_REPEAT_2LANES_DISABLE	0
#define SYNC_REPEAT_2LANES_ENABLE	1

	u8	slvs_sync_ecc_en;
#define SYNC_ECC_DISABLE		0
#define SYNC_ECC_ENABLE			1

	u8	slvs_n_jitter_en;
#define	N_JITTER_DISABLE		0
#define	N_JITTER_ENABLE			1

	u8	slvs_vin_stall_en;
#define VIN_STALL_DISABLE		0
#define VIN_STALL_ENABLE		1

	u8	slvs_param_at_posedge_en;
#define SLVS_PARAM_AT_POSEDGE_DIS	0
#define SLVS_PARAM_AT_POSEDGE_EN	1


	u8	slvs_act_lanes;		/**< Number of active SLVS lanes */
#define S3D_SLVS_1LANE			0
#define S3D_SLVS_2LANES			1
#define S3D_SLVS_3LANES			2
#define S3D_SLVS_4LANES			3

	u8	slvs_vsync_max_en;
#define SLVS_VSYNC_MAX_DISABLE		0
#define SLVS_VSYNC_MAX_ENABLE		1

	u8	slvs_variable_hblank_en;
#define SLVS_VARIABLE_HB_DISABLE	0
#define SLVS_VARIABLE_HB_ENABLE		1


	u8	slvs_code_check;
#define	ZERO_CODE_CHECK			0
#define	ONE_CODE_CHECK			1
#define	TWO_CODE_CHECK			2
#define	THREE_CODE_CHECK		3

	/* offset 0x21 */
	u16	slvs_sav2sav_dist;	/**< SAV to SAV distance */

	/* offset 0x22 */
	u16	slvs_eav_col;		/**< EAV column of SLVS mode */

	/* offset 0x23 */
	u16	vsync_max;

	/* offset 0x29 */
	u16	hblank;

} sbrg_vin_3d_slvs_cfg_t;

/**
 * Structure of the sbrg vin 3d mipi configuration.
 */
typedef struct sbrg_vin_3d_mipi_cfg_s {

	/* offset = 26 */
	u8	mipi_virtualch_select;
	u8	mipi_virtualch_mask;
	u8	mipi_enable;		/**< MIPI Enable */
#define MIPI_VIN_MODE_DISABLE	0
#define MIPI_VIN_MODE_ENABLE	1

	u8	mipi_s_hssettlectl;	/**< HS-RX settle time control register
					     bit[4] controls digital or analog
					     HS-RX settle time,
					     bit[3:0] are counter values for
					     bypass mode.(digital) */
#define MIPI_HSSETTLECTL	22

	u8	mipi_s_dpdn_swap_clk;
#define MIPI_DPDN_SWAP_CLK_OFF	0
#define MIPI_DPDN_SWAP_CLK_ON	1

	u8	mipi_s_dpdn_swap_data;
#define MIPI_DPDN_SWAP_DATA_OFF	0
#define MIPI_DPDN_SWAP_DATA_ON	1

	u8	mipi_s_clksettlectl;
	u8	mipi_decompression_mode;
#define SIMPLE_PREDICTOR_10810		0
#define ADVANCE_PREDICTOR_10810		1
#define SIMPLE_PREDICTOR_12812		2
#define ADVANCE_PREDICTOR_12812		3

	/* offset = 27 */
	u8	mipi_data_type_sel;	/**< Select Data type */
#define YUV422_8BIT		0x1e
#define RAW_8BIT		0x2a
#define RAW_10BIT		0x2b
#define RAW_12BIT		0x2c
#define RAW_14BIT		0x2d
#define GENERIC_8BIT_NULL	0x10
#define GENERIC_8BIT_BLANKING	0x11
#define GENERIC_8BIT_EMBEDDED	0x12

	u8	mipi_data_type_mask;	/**< Defines which long packets are
					     expected */
	u8	mipi_pixel_byte_swap;	/**< byte 0 will be swapped with byte 1,
					     byte 2 will be swapped with byte 3,
					     this bit is only set for YUV422 */
#define PIXEL_BYTE_SWAP_ON	1
#define PIXEL_BYTE_SWAP_OFF	0

	u8	mipi_decompression_enable; /**< Decompression mode enable,
					        default 0 (off) */
#define DECOMPRESSION_ON	1
#define DECOMPRESSION_OFF	0

	/* offset = 28 */
	u16	mipi_b_dphyctl;		/**< D-phy Master Analog block
					     control */
#define MIPI_B_DPHYCTL_DEFAULT	0x93e3

	/* offset = 29 */
	u16	mipi_s_dphyctl;
#define MIPI_S_DPHYCTL_DEFAULT	0x161f

	/* offset = 30 */
	u16 	mipi_error_status;
	/* offset = 31 */
	u16	mipi_counts_frames;

} sbrg_vin_3d_mipi_cfg_t;

/**
 * Structure of the sbrg 3d trigger_pin configuration.
 */
typedef struct trigger_pin_info_s {
	u8	enabled;		/**< trigger enable/disable */
	u8	polarity;		/**< set trigger pin polarity at the
					     startline'th line period */
#define TRIG_ACT_LOW            0
#define TRIG_ACT_HIGH           1

	u16	start_line;		/**< Assert trigger pin at the H-sync
					     of startline'th line after
					     V-sync */
	u16	last_line;		/**< De-assert trigger pin at the
					     H-sync of this line after V-sync */
} trigger_pin_info_t;

/**
 * Structure of the sbrg 3d prescaler configuration.
 */

#define INITIAL_PHASE_ROUNDING		256

/**
 * Data structure containing service functions that need to be provided to the
 * Sbridge device manager by the device drivers.
 */
typedef struct sbrg_prescaler_obj_s {
	int h_in_width;                /**< Horizontal input width
                                            from the sensor */
	int h_out_width;               /**< Horizontal output width */
	int h_sensor_readout_mode;     /**< Horizontal sensor readout mode */
#define S3D_PRESCALE_DEFAULT	0
#define S3D_PRESCALE_BIN2	1
#define S3D_PRESCALE_SKIP2	2

	int v_in_width;                /**< Vertical input width
                                            from the sensor */
	int v_out_width;               /**< Vertical output width */
	int v_sensor_readout_mode;     /**< Vertical sensor readout mode */
} sbrg_prescaler_obj_t;


typedef struct sbrg_prescaler_3d_cfg_s {

	u8	h_prescaler;
#define H_PRESCALER_DISABLE	0
#define H_PRESCALER_ENABLE	1

	u8	coefficient_shift_en; /**< set this bit
					   when the prescaler is on */
#define COEF_SHIFT_DISABLE	0
#define COEF_SHIFT_ENABLE	1

	u16	hout_width;
	u16	hphase_incr;
	u16	init_c0_int;
	u16	init_c0_frac;
	u16	init_c1_int;
	u16	init_c1_frac;

	u8	prescaler_update_done;
#define PRESCALER_DISABLE	0
#define PRESCALER_UPDATE_DONE	4

} sbrg_prescaler_3d_cfg_t;

/**
 * Structure of the sbrg voutf 3d configuration.
 */
typedef struct sbrg_voutf_3d_cfg_s {

	u8	voutf_pg_en;	/**< Embedded pattern generation enable bit. */
#define VOUTF_PG_DISABLE		0
#define VOUTF_PG_ENABLE			1

	u8	voutf_en;	/**< VOUTF_3D enable bit. */
#define VOUTF_DISABLE			0
#define VOUTF_ENABLE			1

	u8	voutf_mode;
#define VOUTF_3D_MODE_CONCATENATING	0
#define VOUTF_3D_MODE_INTERLEAVING	1
#define VOUTF_2D_MODE_L_CHANNEL_EN	2
#define VOUTF_2D_MODE_R_CHANNEL_EN	3
#define VOUTF_2D_MODE_PIXEL_SHUFFLING	4

	u16	voutf_act_pixel;
	u16	voutf_blank_pixel;
	u16	voutf_act_line;
	u16	voutf_rd_th;
	u16	voutf_black_border;

	u16	voutf_vd_clk_pol;
#define SAME_PHASE_OF_SOURCE_CLK	0
#define OPPOSITE_PHASE_OF_SOURCE_CLK	1

	u16	voutf_pip_sel;
#define LEFT_MOM_RIGHT_CHILD		0
#define RIGHT_MOM_LEFT_CHILD		1

	u16	voutf_pip_start_line;
	u16	voutf_pip_end_line;
	u16	voutf_pip_start_pixel;
	u16	voutf_pip_end_pixel;
	u16	voutf_pg_l_ini;
	u16	voutf_pg_r_ini;
	u16	voutf_pg_l_inc;
	u16	voutf_pg_r_inc;

	u16	voutf_l_ch_fst_r_ch_th;
#define VOUTF_2_PIXELS			0
#define VOUTF_4_PIXELS			1
#define VOUTF_8_PIXELS			2
#define VOUTF_32_PIXELS			3

} sbrg_voutf_3d_cfg_t;

/*
 * Structure of the sbrg lcd 3d vout configuration.
 */
typedef struct sbrg_lcd_3d_vout_cfg_s {
	u8	div3_wait_lock;
	u8	rgb888_mode;
	u8	din_wid_is_8b;
	u8	m8068_cdx_sel;		/**< Indicate polarity of cdx
					     while transferring pixel data. */

	u8	m8068_burst_en;		/**< Allow I80/M68 interface outputs
					     pixel data without deasserting
					     mcu_csx pin. */

	u8	cyc_per_pxl;		/**< Cycle count per input pixel. */
#define LCD_3D_1_CYCLE_1_PIXEL		0
#define LCD_3D_2_CYCLE_1_PIXEL		1
#define LCD_3D_3_CYCLE_1_PIXEL		2
#define LCD_3D_4_CYCLE_1_PIXEL		3

	u8	interleave_seq;		/**< Interleave sequence selection. */
#define LCD_3D_NO_INTERLEAVING		0
#define LCD_3D_3D_FRAMES		1
#define LCD_3D_DROP_R_FRAMES		2
#define LCD_3D_DROP_L_FRAMES		3

	u8	demux_sel;		/**< Select output interface to
					     be enabled. */
#define LCD_3D_OUTPUT_INTERFACE_BT601	0
#define LCD_3D_OUTPUT_INTERFACE_I80	2
#define LCD_3D_OUTPUT_INTERFACE_M68	3

	u8	bt601_den_sel;		/**< Select active state of
					     bt601_den_out. */
#define LCD_3D_BT601_ACTIVE_LOW		0
#define LCD_3D_BT601_ACTIVE_HIGH	1

	u8	dclk_phase_sel;		/**< Select the phase of dclk_out.
					     1: inverse of dclk_in
					     0: the same as dclk_in */

	u8	in_phase_sel;		/**< Select the phase of input latch
					     0: use negative edge of dclk_in
						to latch BT.601 input
					     1: use positive edge of dclk_in
						to latch BT.601 input */

	u8	lcd_3d_brg_en;
	u16	num_vsync_low;
	u16	num_vsync_back;
	u16	num_vsync_active;
	u16	num_vsync_front;
	u16	num_hsync_low;
	u16	num_hsync_back;
	u16	num_hsync_active;
	u16	num_hsync_front;
	u8	hif2mcu_rw;		/**< 0 : send a write-command
						 through I80/M68 interface
					     1 : send a read-command
						 through I80/M68 interface */

	u8	hif2mcu_dcx;
	u16	hif2mcu_data;
	u8	num_cssu;
	u8	num_cshd;
	u8	num_wrsu;
	u8	num_wrhd;
	u8	num_wren;
	u8	num_rden;
	u16	patgen_en;

} sbrg_lcd_3d_vout_cfg_t;

/**
 * Callback type for s3d vic event handler
 */
typedef void (*s3d_event_handler)(void);

/*
 * Structure of the sbrg vic configuration.
 */
typedef struct sbrg_vic_3d_cfg_s {
	u16	idsp_vic_enable;
#define IDSP_PROGRAMMABLE_INT_L		0x0001
#define IDSP_MASTER_FRAME_END_L		0x0002
#define IDSP_SLAVE_FRAME_END_L		0x0004
#define IDSP_CAPTURE_WIN_END_L		0x0008
#define IDSP_CAPTURE_WIN_START_L	0x0010
#define VIN_PROGRAMMABLE_INT_1_L	0x0020
#define VIN_PROGRAMMABLE_INT_2_L	0x0040
#define IDSP_CONTROLLABLE_TRIGGER_L	0x0080
#define IDSP_PROGRAMMABLE_INT_R		0x0100
#define IDSP_MASTER_FRAME_END_R		0x0200
#define IDSP_SLAVE_FRAME_END_R		0x0400
#define IDSP_CAPTURE_WIN_END_R		0x0800
#define IDSP_CAPTURE_WIN_START_R	0x1000
#define VIN_PROGRAMMABLE_INT_1_R	0x2000
#define VIN_PROGRAMMABLE_INT_2_R	0x4000
#define IDSP_CONTROLLABLE_TRIGGER_R	0x8000
	u8	s3d_irq;
	s3d_event_handler hdl;
} sbrg_vic_3d_cfg_t;

/* S3D RCT OFFSET */
#define S3D_PLL_CTRL1_OFFSET		(0x0)
#define S3D_PLL_CTRL2_OFFSET		(0x1)
#define S3D_ED_DIV_CLK_SENSOR_OFFSET	(0x4)
#define S3D_ED_DIV_CLK_PLL_REF_OFFSET	(0x5)


/* IDCS CONFIGURATION OFFSET */
#define S3D_IDCS_T_HD_DAT_OFFSET	(0x6)
#define S3D_IDCS_BRG_OFFSET		(0x7)
#define S3D_GCLK_CORE_SEL_OFFSET	(0x8)


/* IOPAD CONFIGURATION OFFSET */
#define S3D_IOPAD_CONOF_CTRL1_OFFSET		(0x9)
#define S3D_IOPAD_CONOF_CTRL2_OFFSET		(0xa)
#define S3D_IOPAD_CONOF_CTRL3_OFFSET		(0xb)
#define S3D_IOPAD_CONOF_CTRL4_OFFSET		(0xc)
#define S3D_IOPAD_CONOF_CTRL5_OFFSET		(0xd)
#define S3D_SPI_PROGRAM_CTRL_OFFSET		(0xe)
#define S3D_IOPAD_SONOF_CTRL2_OFFSET		(0xf)
#define S3D_IOPAD_SONOF_CTRL3_OFFSET		(0x10)
#define S3D_IOPAD_SONOF_CTRL4_OFFSET		(0x11)
#define S3D_IOPAD_SONOF_CTRL5_OFFSET		(0x12)
#define S3D_IOPAD_PU_CTRL1_OFFSET		(0x13)
#define S3D_IOPAD_PU_CTRL2_OFFSET		(0x14)
#define S3D_IOPAD_PU_CTRL3_OFFSET		(0x15)
#define S3D_IOPAD_PU_CTRL4_OFFSET		(0x16)
#define S3D_IOPAD_PU_CTRL5_OFFSET		(0x17)
#define S3D_IOPAD_PD_CTRL1_OFFSET		(0x18)
#define S3D_IOPAD_PD_CTRL2_OFFSET		(0x19)
#define S3D_IOPAD_PD_CTRL3_OFFSET		(0x1a)
#define S3D_IOPAD_PD_CTRL4_OFFSET		(0x1b)
#define S3D_IOPAD_PD_CTRL5_OFFSET		(0x1c)

/* MIPI PHY CONFIGURATION OFFSET */
#define S3D_MIPI_PHY_CTRL_L_1_OFFSET		(0x1d)
#define S3D_MIPI_PHY_CTRL_L_2_OFFSET		(0x1e)
#define S3D_MIPI_PHY_CTRL_L_3_OFFSET		(0x1f)
#define S3D_MIPI_PHY_CTRL_R_1_OFFSET		(0x20)
#define S3D_MIPI_PHY_CTRL_R_2_OFFSET		(0x21)
#define S3D_MIPI_PHY_CTRL_R_3_OFFSET		(0x22)

#define S3D_PRESCALER_CLOCK_CTRL_OFFSET		(0x23)
#define S3D_REG_SOFT_RESET_OFFSET		(0x24)
#define S3D_VOUTF_SSTL_CTRL_OFFSET		(0x25)
#define S3D_SPI_CTRL_OFFSET			(0x26)
#define S3D_VERSION_OFFSET			(0x27)


/* VIN CONFIGURATION OFFSET */
#define S3D_VIN_S_CTRL_OFFSET			(0x00)
#define S3D_VIN_S_INCFG_OFFSET			(0x01)
#define S3D_VIN_S_STATUS_OFFSET			(0x02)
#define S3D_VIN_S_V_WIDTH_OFFSET		(0x03)
#define S3D_VIN_S_H_WIDTH_OFFSET		(0x04)
#define S3D_VIN_S_HOF_TOP_OFFSET		(0x05)
#define S3D_VIN_S_HOF_BTM_OFFSET		(0x06)
#define S3D_VIN_S_V_OFFSET			(0x07)
#define S3D_VIN_S_H_OFFSET			(0x08)
#define S3D_VIN_S_MIN_V_OFFSET			(0x09)
#define S3D_VIN_S_MIN_H_OFFSET			(0x0a)
#define S3D_VIN_S_TRG_0_ST_OFFSET		(0x0b)
#define S3D_VIN_S_TRG_0_ED_OFFSET		(0x0c)
#define S3D_VIN_S_TRG_1_ST_OFFSET		(0x0d)
#define S3D_VIN_S_TRG_1_ED_OFFSET		(0x0e)
#define S3D_VIN_S_IDSPCMD_OFFSET		(0x0f)

#define S3D_VIN_S_CAPSTARTV_OFFSET		(0x11)
#define S3D_VIN_S_CAPSTARTH_OFFSET		(0x12)
#define S3D_VIN_S_CAPENDV_OFFSET		(0x13)
#define S3D_VIN_S_CAPENDH_OFFSET		(0x14)
#define S3D_VIN_S_BLANKLENGTH_H_OFFSET		(0x15)
#define S3D_VIN_S_TOUT_V_LOW_OFFSET		(0x16)
#define S3D_VIN_S_TOUT_V_HIGH_OFFSET		(0x17)
#define S3D_VIN_S_TOUT_H_LOW_OFFSET		(0x18)
#define S3D_VIN_S_TOUT_H_HIGH_OFFSET		(0x19)
#define S3D_VIN_MIPI_CTRL_0_OFFSET		(0x1a)
#define S3D_VIN_MIPI_CTRL_1_OFFSET		(0x1b)
#define S3D_VIN_MIPI_B_DPHYCTL_OFFSET		(0x1c)
#define S3D_VIN_MIPI_S_DPHYCTL_OFFSET		(0x1d)
#define S3D_VIN_MIPI_ERR_ST_OFFSET		(0x1e)
#define S3D_VIN_FRAME_WITH_ERRCNT_OFFSET	(0x1f)
#define S3D_VIN_SLVS_CTRL_OFFSET		(0x20)
#define S3D_VIN_SLVS_VH_WIDTH_OFFSET		(0x21)
#define S3D_VIN_SLVS_ACTIVE_VH_WIDTH_OFFSET	(0x22)
#define S3D_VIN_SLVS_VSYNC_MAX_OFFSET		(0x23)
#define S3D_VIN_LANE_MUX_SELECT_REG_0_OFFSET	(0x24)
#define S3D_VIN_LANE_MUX_SELECT_REG_1_OFFSET	(0x25)
#define S3D_VIN_LANE_MUX_SELECT_REG_2_OFFSET	(0x26)
#define S3D_VIN_LANE_MUX_SELECT_REG_3_OFFSET	(0x27)
#define S3D_VIN_SLVS_STATUS_OFFSET		(0x28)
#define S3D_VIN_SLVS_LINE_SYNC_TOUT_OFFSET	(0x29)
#define S3D_VIN_SLVS_DEBUG_OFFSET		(0x2a)
#define S3D_VIN_S_ACT_MIN_V_OFFSET		(0x2b)
#define S3D_VIN_S_ACT_MIN_H_OFFSET		(0x2c)
#define S3D_VIN_S_CTRLTRIGSTARTLOW_OFFSET	(0x2d)
#define S3D_VIN_S_CTRLTRIGSTARTHIGH_OFFSET	(0x2e)
#define S3D_VIN_S_INTRSTARTLOW_OFFSET		(0x2f)
#define S3D_VIN_S_INTRSTARTHIGH_OFFSET		(0x30)
#define S3D_VIN_S_SYNCFIFOCNT_OFFSET		(0x31)
#define S3D_VIN_S_OUTCFA0_OFFSET		(0x32)
#define S3D_VIN_S_OUTCFA1_OFFSET		(0x33)
#define S3D_VIN_MIPI_RESET_OFFSET		(0x34)
#define S3D_VIN_S_OUTVALID_OFFSET		(0x35)
#define S3D_VIN_MIPI_FRAME_NUMBER_OFFSET	(0x36)
#define S3D_VIN_MIPI_STATE_0_OFFSET		(0x37)
#define S3D_VIN_MIPI_STATE_1_OFFSET		(0x38)
#define S3D_VIN_S_TIMINGSNAPACTV_OFFSET		(0x39)
#define S3D_VIN_S_TIMINGSNAPACTH_OFFSET		(0x3a)
#define S3D_VIN_S_TIMESTAMPACTVLOW_OFFSET	(0x3b)
#define S3D_VIN_S_TIMESTAMPACTVHIGH_OFFSET	(0x3c)
#define S3D_VIN_S_TIMESTAMPBLANKVLOW_OFFSET	(0x3d)
#define S3D_VIN_S_TIMESTAMPBLANKVHIGH_OFFSET	(0x3e)
#define S3D_VIN_UPDATE_DONE_OFFSET		(0x3f)

/* PRESCALER OFFSET */
#define S3D_PRESCALER_CTRL_OFFSET		(0x00)
#define S3D_PRESCALER_HOUT_OFFSET		(0x01)
#define S3D_PRESCALER_HPHASE_INCR_OFFSET	(0x02)
#define S3D_PRESCALER_INIT_C0_INT_OFFSET	(0x03)
#define S3D_PRESCALER_INIT_C0_FRAC_OFFSET	(0x04)
#define S3D_PRESCALER_INIT_C1_INT_OFFSET	(0x05)
#define S3D_PRESCALER_INIT_C1_FRAC_OFFSET	(0x06)
#define S3D_PRESCALER_UPDATE_DONE_OFFSET	(0x09)


/* VOUTF 3D CONFIGURATION OFFSET */
#define S3D_VOUTF_3D_EN_OFFSET			(0x00)
#define S3D_VOUTF_3D_MODE_OFFSET		(0x01)
#define S3D_VOUTF_3D_ACT_PIXEL_OFFSET		(0x02)
#define S3D_VOUTF_3D_BLANK_PIXEL_OFFSET		(0x03)
#define S3D_VOUTF_3D_ACT_LINE_OFFSET		(0x04)
#define S3D_VOUTF_3D_RD_TH_OFFSET		(0x05)
#define S3D_VOUTF_3D_BLACK_BORDER_OFFSET	(0x18)
#define S3D_VOUTF_3D_VD_CLK_POL_OFFSET		(0x19)
#define S3D_VOUTF_3D_PIP_SEL_OFFSET		(0x1a)
#define S3D_VOUTF_3D_PIP_START_LINE_OFFSET	(0x1b)
#define S3D_VOUTF_3D_PIP_END_LINE_OFFSET	(0x1c)
#define S3D_VOUTF_3D_PIP_START_PIXEL_OFFSET	(0x1d)
#define S3D_VOUTF_3D_PIP_END_PIXEL_OFFSET	(0x1e)
#define S3D_VOUTF_3D_PG_L_INI_OFFSET		(0x1f)
#define S3D_VOUTF_3D_PG_R_INI_OFFSET		(0x20)
#define S3D_VOUTF_3D_PG_L_INC_OFFSET		(0x21)
#define S3D_VOUTF_3D_PG_R_INC_OFFSET		(0x22)
#define S3D_VOUTF_3D_STS_OFFSET			(0x23)
#define S3D_VOUTF_3D_L_CH_FST_R_CH_TH_OFFSET	(0x24)
#define S3D_VOUTF_3D_SM_OFFSET			(0x25)


/* LCD 3D VOUT CONFIGURATION OFFSET */
#define S3D_LCD_3D_VOUT_CTRL_OFFSET		(0x00)
#define S3D_LCD_3D_VOUT_V_LOW_OFFSET		(0x01)
#define S3D_LCD_3D_VOUT_V_BACK_OFFSET		(0x02)
#define S3D_LCD_3D_VOUT_V_ACTIVE_OFFSET		(0x03)
#define S3D_LCD_3D_VOUT_V_FRONT_OFFSET		(0x04)
#define S3D_LCD_3D_VOUT_H_LOW_OFFSET		(0x05)
#define S3D_LCD_3D_VOUT_H_BACK_OFFSET		(0x06)
#define S3D_LCD_3D_VOUT_H_ACTIVE_OFFSET		(0x07)
#define S3D_LCD_3D_VOUT_H_FRONT_OFFSET		(0x08)
#define S3D_LCD_3D_VOUT_HIF2MCU_CTRL_OFFSET	(0x09)
#define S3D_LCD_3D_VOUT_HIF2MCU_DATA_OFFSET	(0x0a)
#define S3D_LCD_3D_VOUT_CSB_PIN_OFFSET		(0x0b)
#define S3D_LCD_3D_VOUT_E_PIN_OFFSET		(0x0c)
#define S3D_LCD_3D_VOUT_WR_EN_CYCLE_OFFSET	(0x0d)
#define S3D_LCD_3D_VOUT_PATTERN_GEN_EN_OFFSET	(0x0e)

/* VIC */
#define S3D_VIC_ENABLE_OFFSET			(0x00)
#define S3D_VIC_STAT1_OFFSET			(0x01)
#define S3D_VIC_STAT2_OFFSET			(0x02)



/* MOULDE ADDR */
#define S3D_REG_ADDR(id, offset)	(((id) & 0xff00) + (offset))

#define S3D_BASE_ADDR(x)		(0x0 + (x))
#define S3D_RCT_ADDR(x)			(0x0 + (x))

#define S3D_VIN_L_ADDR(x)		(0x100 + (x))
#define S3D_VIN_R_ADDR(x)		(0x200 + (x))
#define S3D_VOUTF_ADDR(x)		(0x500 + (x))
#define S3D_LCD_3D_VOUT_ADDR(x)		(0x600 + (x))
#define S3D_VIN_BOTH_ADDR(x)		(0x700 + (x))
#define S3D_PRESCALER_ADDR(x)		(0x800 + (x))
#define S3D_VIC_ADDR(x)			(0x900 + (x))

/* ------------------ FOR API ----------------*/

/* MODULE NAME FOR API */
#define S3D_RESET_ALL				(0)
#define S3D_RESET_L_VIN				(1)
#define S3D_RESET_R_VIN				(2)
#define S3D_RESET_BOTH_VIN			(3)
#define S3D_RESET_L_IDSP			(4)
#define S3D_RESET_R_IDSP			(5)
#define S3D_RESET_BOTH_IDSP			(6)

#define S3D_RESET_VOUT				(7)
#define S3D_RESET_PLL				(8)
#define S3D_RESET_L_PRESCALER			(9)
#define S3D_RESET_R_PRESCALER			(10)
#define S3D_RESET_BOTH_PRESCALER		(11)

#define S3D_RESET_L_MIPI			(12)
#define S3D_RESET_R_MIPI			(13)
#define S3D_RESET_BOTH_MIPI			(14)

#define S3D_CONFIG_RCT_MODULE			(0x000)
#define S3D_CONFIG_IDCS_MODULE			(0x007)
#define S3D_VOUTF_SSTL_CTRL_MUDULE		(0x025)
#define S3D_CONFIG_SPI_MODULE			(0x026)
#define S3D_MIPI_PHY_CTRL_L_MODULE		(0x01d)
#define S3D_MIPI_PHY_CTRL_R_MODULE		(0x020)
#define S3D_MIPI_PHY_CTRL_BOTH_MODULE		(0x03d)

#define S3D_CONFIG_VIN_L_MODULE			(0x100)
#define S3D_CONFIG_IDSP_L_MODULE		(0x10f)
#define S3D_CONFIG_VIN_TRIGGER0_L_MODULE	(0x10b)
#define S3D_CONFIG_VIN_TRIGGER1_L_MODULE	(0x10d)
#define S3D_CONFIG_VIN_MIPI_L_MODULE		(0x11a)
#define S3D_CONFIG_VIN_SLVS_L_MODULE		(0x120)
#define S3D_CONFIG_SLVS_LANEMUX_L_MODULE	(0x124)
#define S3D_CONFIG_UPDATE_L_MODULE		(0x13f)

#define S3D_CONFIG_VIN_R_MODULE			(0x200)
#define S3D_CONFIG_IDSP_R_MODULE		(0x20f)
#define S3D_CONFIG_VIN_TRIGGER0_R_MODULE	(0x20b)
#define S3D_CONFIG_VIN_TRIGGER1_R_MODULE	(0x20d)
#define S3D_CONFIG_VIN_MIPI_R_MODULE		(0x21a)
#define S3D_CONFIG_VIN_SLVS_R_MODULE		(0x220)
#define S3D_CONFIG_SLVS_LANEMUX_R_MODULE	(0x224)
#define S3D_CONFIG_UPDATE_R_MODULE		(0x23f)

#define S3D_CONFIG_VIN_BOTH_MODULE		(0x700)
#define S3D_CONFIG_IDSP_BOTH_MODULE		(0x70f)
#define S3D_CONFIG_VIN_TRIGGER0_BOTH_MODULE	(0x70b)
#define S3D_CONFIG_VIN_TRIGGER1_BOTH_MODULE	(0x70d)
#define S3D_CONFIG_VIN_MIPI_BOTH_MODULE		(0x71a)
#define S3D_CONFIG_VIN_SLVS_BOTH_MODULE		(0x720)
#define S3D_CONFIG_SLVS_LANEMUX_BOTH_MODULE	(0x724)
#define S3D_CONFIG_UPDATE_BOTH_MODULE		(0x73f)

#define S3D_CONFIG_PRESCALER_L_MODULE		(0x300)
#define S3D_CONFIG_PRESCALER_R_MODULE		(0x400)
#define S3D_CONFIG_VOUTF_3D_MODULE		(0x500)
#define S3D_CONFIG_LCD_3D_VOUT_MODULE		(0x600)

#define S3D_CONFIG_PRESCALER			(0x8FF)	//fixme main prescale command
#define S3D_CONFIG_PRESCALER_MODULE		(0x800)

#define S3D_CONFIG_L_PRESC_COEF_MODULE		(0x340)
#define S3D_CONFIG_R_PRESC_COEF_MODULE		(0x440)
#define S3D_CONFIG_BOTH_PRESC_COEF_MODULE	(0x840)

#define S3D_CONFIG_VIC_MODULE			(0x900)

#define S3D_SET_REG_DATA			(0xFFFB)
#define S3D_GET_REG_DATA			(0xFFFC)
#define S3D_CONFIG_ENABLE			(0xFFFD)
#define S3D_CONFIG_DISABLE			(0xFFFE)
#define S3D_CONFIG_RESET			(0xFFFF)


#endif

