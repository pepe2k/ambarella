/**
 * system/src/bld/dsp.c
 *
 * History:
 *    2007/11/06 - [E-John Lien] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <basedef.h>
#include <ambhw.h>
#include <bldfunc.h>
#include <dsp.h>
#include <vout.h>

#ifndef KB
#define KB (1024)
#endif

#ifndef MB
#define MB (1024*1024)
#endif

#if (CHIP_REV == A2)
#define A2_DSP_START         0xc0e00000
#define A2_DSP_END           0xc4000000
#define A2_VOUT_SET_PTR      0xc3800000
#define DRAM_A2_DSP_BASE_REG 0x6000402c
#endif

#if (CHIP_REV == A2S)
#define A2S_DSP_START         0xc1000000
#define A2S_DSP_END           0xc4000000
#define A2S_VOUT_SET_PTR      0xc3800000
#define DRAM_A2S_DSP_BASE_REG 0x6000402c
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
//#define A3_DSP_START    0xc1000000
#define A3_DSP_START      0xc3000000
#define A3_DSP_END        0xc6000000
// These address should not be used by anyone
#define A3_VOUT0_SET_PTR  (IDSP_RAM_START + IDSP_RAM_SIZE - 6 *MB + 1.5*MB + 256*KB + 1*KB + 12*KB + 12*KB)
#define A3_VOUT1_SET_PTR  (A3_VOUT0_SET_PTR + 64)
#endif

#if (CHIP_REV == A5S)
#define A5S_SPLASH_DBG        0
#define VOUT_AHB_DIRECT_ACESS 0

/* put Splashlogo data in BSB mem,  which will be overwritten by BSB after booting to Linux */
#define A5S_SPLASH_START      (BSB_RAM_START)
#define A5S_SPLASH_END        (BSB_RAM_START + 1 * MB)

#define DEFAULT_BACK_GROUND_Y    0x10
#define DEFAULT_BACK_GROUND_V    0x80
#define DEFAULT_BACK_GROUND_U    0x80
#define DEFAULT_HIGHLIGHT_Y      0xd2
#define DEFAULT_HIGHLIGHT_U      0x10
#define DEFAULT_HIGHLIGHT_V      0x92
#define DEFAULT_HIGHLIGHT_THRESH 0x00

#define VIDEO_WIN_OFFSET_X    0
#define VIDEO_WIN_OFFSET_Y    0

#define dsp_get_vout_a()    (VOUT_DISPLAY_A)

/* Unit in all sizes are u32 */
#define DSP_VOUT_A_CTLREG_BUF_SIZE   (372 << 2)  /* 0x0300 - 0x0470  (368) */
#define DSP_VOUT_B_CTLREG_BUF_SIZE   (372 << 2)  /* 0x0600 - 0x0770  (368) */
#define DSP_VOUT_DVE_CTLREG_BUF_SIZE (512 << 2)  /* 0x0000 - 0x01fc  (508) */
#define DSP_VOUT_OSD_CTRREG_BUF_SIZE (1024 << 2) /* 0x9000 - 0x93fc (1021) */
#define DSP_OSD_BUF_SIZE             (736 * 576)
#endif

#if (CHIP_REV == A7)
#define A7_SPLASH_DBG         0
#define VOUT_AHB_DIRECT_ACESS 0
#define A7_SPLASH_START       IDSP_RAM_START
#define A7_SPLASH_END         (IDSP_RAM_START + IDSP_RAM_SIZE - 6 * MB)
#define DSP_DRAM_MASK         0x1FFFFFFF

#define DEFAULT_BACK_GROUND_Y 0x10
#define DEFAULT_BACK_GROUND_V 0x80
#define DEFAULT_BACK_GROUND_U 0x80
#define DEFAULT_HIGHLIGHT_Y   0xd2
#define DEFAULT_HIGHLIGHT_U   0x10
#define DEFAULT_HIGHLIGHT_V   0x92
#define DEFAULT_HIGHLIGHT_THRESH  0x00

#define VIDEO_WIN_OFFSET_X    0
#define VIDEO_WIN_OFFSET_Y    0

#define dsp_get_vout_a()    (VOUT_DISPLAY_A)

/* Unit in all sizes are u32 */
#define DSP_VOUT_A_CTLREG_BUF_SIZE  (372 << 2)  /* 0x0300 - 0x0470  (368) */
#define DSP_VOUT_B_CTLREG_BUF_SIZE  (372 << 2)  /* 0x0600 - 0x0770  (368) */
#define DSP_VOUT_DVE_CTLREG_BUF_SIZE  (512 << 2)  /* 0x0000 - 0x01fc  (508) */
#define DSP_VOUT_OSD_CTRREG_BUF_SIZE  (1024 << 2) /* 0x9000 - 0x93fc (1021) */
#define DSP_OSD_BUF_SIZE    (360 * 240 * 2)
#endif

#if (CHIP_REV == I1)
#define I1_SPLASH_DBG			0
#define VOUT_AHB_DIRECT_ACESS		0
#define I1_SPLASH_START			((IDSP_RAM_START + MB - 1) & ~(MB - 1))
#define IDSP_RAM_SIZE			(DRAM_SIZE - (I1_SPLASH_START - DRAM_START_ADDR))
#define I1_SPLASH_END			(IDSP_RAM_START + IDSP_RAM_SIZE - 6 * MB)
#define DSP_DRAM_MASK			0xFFFFFFFF

#define DEFAULT_BACK_GROUND_Y		0x10
#define DEFAULT_BACK_GROUND_V		0x80
#define DEFAULT_BACK_GROUND_U		0x80
#define DEFAULT_HIGHLIGHT_Y		0xd2
#define DEFAULT_HIGHLIGHT_U		0x10
#define DEFAULT_HIGHLIGHT_V		0x92
#define DEFAULT_HIGHLIGHT_THRESH	0x00

#define VIDEO_WIN_OFFSET_X		0
#define VIDEO_WIN_OFFSET_Y		0

#define dsp_get_vout_a()		(VOUT_DISPLAY_A)

/* Unit in all sizes are u32 */
#define DSP_VOUT_A_CTLREG_BUF_SIZE	(372 << 2)  /* 0x0300 - 0x0470  (368) */
#define DSP_VOUT_B_CTLREG_BUF_SIZE	(372 << 2)  /* 0x0600 - 0x0770  (368) */
#define DSP_VOUT_DVE_CTLREG_BUF_SIZE	(512 << 2)  /* 0x0000 - 0x01fc  (508) */
#define DSP_VOUT_OSD_CTRREG_BUF_SIZE	(1024 << 2) /* 0x9000 - 0x93fc (1021) */
#define DSP_OSD_BUF_SIZE		(1024 * 768)
#endif

#if (CHIP_REV == S2)
#define S2_SPLASH_DBG			0
#define VOUT_AHB_DIRECT_ACESS		0
#define S2_SPLASH_START			((IDSP_RAM_START + MB - 1) & ~(MB - 1))
#define IDSP_RAM_SIZE			(DRAM_SIZE - (S2_SPLASH_START - DRAM_START_ADDR))
#define S2_SPLASH_END			(IDSP_RAM_START + IDSP_RAM_SIZE - 16 * MB)
#define DSP_DRAM_MASK			0x3FFFFFFF

#define DEFAULT_BACK_GROUND_Y		0x10
#define DEFAULT_BACK_GROUND_V		0x80
#define DEFAULT_BACK_GROUND_U		0x80
#define DEFAULT_HIGHLIGHT_Y		0xd2
#define DEFAULT_HIGHLIGHT_U		0x10
#define DEFAULT_HIGHLIGHT_V		0x92
#define DEFAULT_HIGHLIGHT_THRESH	0x00

#define VIDEO_WIN_OFFSET_X		0
#define VIDEO_WIN_OFFSET_Y		0

#define dsp_get_vout_a()		(VOUT_DISPLAY_A)

/* Unit in all sizes are u32 */
#define DSP_VOUT_A_CTLREG_BUF_SIZE	(372 << 2)  /* 0x0300 - 0x0470  (368) */
#define DSP_VOUT_B_CTLREG_BUF_SIZE	(372 << 2)  /* 0x0600 - 0x0770  (368) */
#define DSP_VOUT_DVE_CTLREG_BUF_SIZE	(512 << 2)  /* 0x0000 - 0x01fc  (508) */
#define DSP_VOUT_OSD_CTRREG_BUF_SIZE	(1024 << 2) /* 0x9000 - 0x93fc (1021) */
#define DSP_OSD_BUF_SIZE		(1024 * 768)
#endif


/**
 * Base address for a2.
 */
#if (CHIP_REV == A2)
struct ucode_base_addr_a2_s
{
  u32 dsp_start;
  u32 dsp_end;
  u32 code_addr;
  u32 memd_addr;
  u32 defbin_addr;
  u32 shadow_reg_addr;
  u32 defimg_y_addr;
  u32 defimg_uv_addr;
  u32 dsp_buffer_addr;
  u32 dsp_buffer_size;
} __attribute__ ((packed));
#endif

#if (CHIP_REV == A2S)
struct ucode_base_addr_a2s_s
{
  u32 dsp_start;
  u32 dsp_end;
  u32 code_addr;
  u32 memd_addr;
  u32 defbin_addr;
  u32 shadow_reg_addr;
  u32 defimg_y_addr;
  u32 defimg_uv_addr;
  u32 dsp_buffer_addr;
  u32 dsp_buffer_size;
} __attribute__ ((packed));
#endif

/**
 * Base address for a3.
 */
#if (CHIP_REV == A3) || (CHIP_REV == A5)
struct ucode_base_addr_a3_s
{
  u32 dsp_start;
  u32 dsp_end;
  u32 code_addr;
  u32 sub0_addr;
  u32 sub1_addr;
  u32 defbin_addr;
  u32 shadow_reg_addr;
  u32 defimg_y_addr;
  u32 defimg_uv_addr;
  u32 dsp_buffer_addr;
  u32 dsp_buffer_size;
} __attribute__ ((packed));
#endif

#if (CHIP_REV == A5S)
#define DSP_INIT_DATA_SIZE        128
#define DSP_INIT_DATA_ADDR        0xC00F0000

#define DSP_CMD_SIZE              128

#define  A5S_DSP_RESULT_SIZE      256
#define A5S_DSP_CMD_QUEUE_SIZE    4096
#define A5S_DSP_RESULT_QUEUE_SIZE 4096
#define A5S_DSP_DEFCFG_QUEUE_SIZE 4096
#define  A5S_NUM_MSG_FIELDS       (A5S_DSP_RESULT_SIZE / 4)
#define A5S_NUM_DSP_CMDS          ((A5S_DSP_CMD_QUEUE_SIZE - 8) / DSP_CMD_SIZE)
#define A5S_NUM_DSP_RESULTS       (A5S_DSP_RESULT_QUEUE_SIZE / A5S_DSP_RESULT_SIZE)
#define A5S_CMD_MAX_COUNT         A5S_NUM_DSP_CMDS

__ARMCC_PACK__ struct dsp_cmd_s
{
  u32     code;
  u8      param[DSP_CMD_SIZE - 4];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_cmd_data_a5s_s
{
  u32     seqno;
  u32     ncmd;
  struct dsp_cmd_s cmd[A5S_NUM_DSP_CMDS];
} __ATTRIB_PACK__;

struct dsp_cmd_data_a5s_s cmd_data_a5s;
struct dsp_cmd_data_a5s_s def_cmd_data_a5s;

__ARMCC_PACK__ struct result_msg_a5s_s
{
  u32 field[A5S_NUM_MSG_FIELDS];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_resqueue_a5s_s
{
  struct result_msg_a5s_s result[A5S_NUM_DSP_RESULTS];
} __ATTRIB_PACK__ result_queue_a5s;

__ARMCC_PACK__ struct dsp_init_data_a5s_s
{
  u32  *default_binary_data;
  struct  dsp_cmd_data_a5s_s *cmd_data_ptr;
  u32  cmd_data_size;
  struct  dsp_resqueue_a5s_s *res_queue_ptr;
  u32  res_queue_size;
  u32  rsv0[3];
  u32  op_mode;

  struct  dsp_cmd_data_a5s_s *def_config_ptr;
  u32  def_config_size;
  u32  *dsp_buf_ptr;
  u32  dsp_buf_size;
  u32  vdsp_info_ptr;
  u32  rsv[18];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct stage_cmd_data_a5s_s {
  struct dsp_cmd_data_a5s_s data;
  int bits;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_vout_reg_a5s_s
{
  u32  *vo_ctl_a_base;
  u32  *vo_ctl_b_base;
  u32  *vo_ctl_dve_base;
  u32  *vo_ctl_osd_clut_a_base;
  u32  *vo_ctl_osd_clut_b_base;
  u32  vo_ctl_a_size;
  u32  vo_ctl_b_size;
  u32  vo_ctl_dve_size;
  u32  vo_ctl_osd_clut_a_size;
  u32  vo_ctl_osd_clut_b_size;
} __ATTRIB_PACK__;

#endif /* if (CHIP_REV == A5S) */

#if (CHIP_REV == A7)
#define DSP_INIT_DATA_SIZE    256
#define DSP_INIT_DATA_ADDR    0xC00F0000

#define DSP_CMD_SIZE          128

#define A7_DSP_RESULT_SIZE       256
#define A7_DSP_CMD_QUEUE_SIZE    4096
#define A7_DSP_RESULT_QUEUE_SIZE 4096
#define A7_DSP_DEFCFG_QUEUE_SIZE 4096
#define A7_NUM_MSG_FIELDS        (A7_DSP_RESULT_SIZE / 4)
#define A7_NUM_DSP_CMDS          ((A7_DSP_CMD_QUEUE_SIZE - 8) / DSP_CMD_SIZE)
#define A7_NUM_DSP_RESULTS       (A7_DSP_RESULT_QUEUE_SIZE / A7_DSP_RESULT_SIZE)
#define A7_CMD_MAX_COUNT         A7_NUM_DSP_CMDS

#define CMD_HEADER_CODE      0x000000ab
#define MSG_HEADER_CODE      0x81000001
#define MSG_ENC_STATUS       0x82000001
#define MSG_JPG_ENC_STATUS   0x82000002

__ARMCC_PACK__ struct dsp_cmd_s
{
  u32     code;
  u8      param[DSP_CMD_SIZE - 4];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_header_cmd_s
{
  u32  code;
  u32  seqno;
  u32  ncmd;
  u8  param[DSP_CMD_SIZE - 12];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_cmd_data_a7_s
{
  struct dsp_header_cmd_s header_cmd;
  struct dsp_cmd_s cmd[A7_NUM_DSP_CMDS];
} __ATTRIB_PACK__;

struct dsp_cmd_data_a7_s cmd_data_a7;
struct dsp_cmd_data_a7_s def_cmd_data_a7;

__ARMCC_PACK__ struct result_msg_header_a7_s
{
  u32 msg_code;
  u32 dsp_mode;
  u32 time_code;
  u32 prev_cmd_seq;
  u32 prev_cmd_echo;
  u32 prev_num_cmds;
  u32 num_msgs;
  u32 rsv[A7_NUM_MSG_FIELDS - 7];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct result_msg_a7_s
{
  u32 field[A7_NUM_MSG_FIELDS];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_resqueue_a7_s
{
  struct result_msg_header_a7_s header_msg;
  struct result_msg_a7_s result[A7_NUM_DSP_RESULTS - 1];
} __ATTRIB_PACK__ result_queue_a7;

__ARMCC_PACK__ struct dsp_init_data_a7_s
{
  u32 default_binary_data_addr;
  u32 default_binary_data_size;

  /// General purpose cmd/msg channel for DSP-ARM
  u32 cmd_data_gen_daddr;
  u32 cmd_data_gen_size;
  u32 msg_queue_gen_daddr;
  u32 msg_queue_gen_size;

  /// Secondary cmd/msg channel for VCAP-ARM
  u32 cmd_data_vcap_daddr;
  u32 cmd_data_vcap_size;
  u32 msg_queue_vcap_daddr;
  u32 msg_queue_vcap_size;

  // Third
  u32 cmd_data_3rd_daddr;
  u32 cmd_data_3rd_size;
  u32 msg_queue_3rd_daddr;
  u32 msg_queue_3rd_size;

  // Fourth
  u32 cmd_data_4th_daddr;
  u32 cmd_data_4th_size;
  u32 msg_queue_4th_daddr;
  u32 msg_queue_4th_size;

  u32 default_config_daddr;
  u32 default_config_size;

  u32 dsp_buffer_daddr;
  u32 dsp_buffer_size;
  u32 rsv[42];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_vout_reg_a7_s
{
  u32  *vo_ctl_a_base;
  u32  *vo_ctl_b_base;
  u32  *vo_ctl_dve_base;
  u32  *vo_ctl_osd_clut_a_base;
  u32  *vo_ctl_osd_clut_b_base;
  u32  vo_ctl_a_size;
  u32  vo_ctl_b_size;
  u32  vo_ctl_dve_size;
  u32  vo_ctl_osd_clut_a_size;
  u32  vo_ctl_osd_clut_b_size;
} __ATTRIB_PACK__;

#endif /* if (CHIP_REV == A7) */

#if (CHIP_REV == I1)
#define DSP_INIT_DATA_SIZE		256
#define DSP_INIT_DATA_ADDR		0x000F0000

#define DSP_CMD_SIZE			128

#define	I1_DSP_RESULT_SIZE		256
#define I1_DSP_CMD_QUEUE_SIZE		4096
#define I1_DSP_RESULT_QUEUE_SIZE	4096
#define I1_DSP_DEFCFG_QUEUE_SIZE	4096
#define	I1_NUM_MSG_FIELDS		(I1_DSP_RESULT_SIZE / 4)
#define I1_NUM_DSP_CMDS        		((I1_DSP_CMD_QUEUE_SIZE - 8) / DSP_CMD_SIZE)
#define I1_NUM_DSP_RESULTS		(I1_DSP_RESULT_QUEUE_SIZE / I1_DSP_RESULT_SIZE)
#define I1_CMD_MAX_COUNT		I1_NUM_DSP_CMDS

#define CMD_HEADER_CODE			0x000000ab
#define MSG_HEADER_CODE			0x81000001
#define MSG_ENC_STATUS			0x82000001
#define MSG_JPG_ENC_STATUS		0x82000002

__ARMCC_PACK__ struct dsp_cmd_s
{
	u32     code;
	u8      param[DSP_CMD_SIZE - 4];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_header_cmd_s
{
	u32	code;
	u32	seqno;
	u32	ncmd;
	u8	param[DSP_CMD_SIZE - 12];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_cmd_data_i1_s
{
	struct dsp_header_cmd_s header_cmd;
	struct dsp_cmd_s cmd[I1_NUM_DSP_CMDS];
} __ATTRIB_PACK__;

struct dsp_cmd_data_i1_s cmd_data_i1;
struct dsp_cmd_data_i1_s def_cmd_data_i1;

__ARMCC_PACK__ struct result_msg_header_i1_s
{
	u32 msg_code;
	u32 dsp_mode;
	u32 time_code;
	u32 prev_cmd_seq;
	u32 prev_cmd_echo;
	u32 prev_num_cmds;
	u32 num_msgs;
	u32 rsv[I1_NUM_MSG_FIELDS - 7];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct result_msg_i1_s
{
	u32 field[I1_NUM_MSG_FIELDS];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_resqueue_i1_s
{
	struct result_msg_header_i1_s header_msg;
	struct result_msg_i1_s result[I1_NUM_DSP_RESULTS - 1];
} __ATTRIB_PACK__ result_queue_i1;

__ARMCC_PACK__ struct dsp_init_data_i1_s
{
	u32 default_binary_data_addr;
	u32 default_binary_data_size;

	/// General purpose cmd/msg channel for DSP-ARM
	u32 cmd_data_gen_daddr;
	u32 cmd_data_gen_size;
	u32 msg_queue_gen_daddr;
	u32 msg_queue_gen_size;

	/// Secondary cmd/msg channel for VCAP-ARM
	u32 cmd_data_vcap_daddr;
	u32 cmd_data_vcap_size;
	u32 msg_queue_vcap_daddr;
	u32 msg_queue_vcap_size;

	// Third
	u32 cmd_data_3rd_daddr;
	u32 cmd_data_3rd_size;
	u32 msg_queue_3rd_daddr;
	u32 msg_queue_3rd_size;

	// Fourth
	u32 cmd_data_4th_daddr;
	u32 cmd_data_4th_size;
	u32 msg_queue_4th_daddr;
	u32 msg_queue_4th_size;

	u32 default_config_daddr;
	u32 default_config_size;

	u32 dsp_buffer_daddr;
	u32 dsp_buffer_size;
	u32 dsp_buffer_pagetbl_daddr; // the tables of all physical pages corresponding to dsp_buffer_daddr
	u32 dsp_buffer_pagetbl_size;

	u32 dsp_att_cpn_start_offset; // the portion of att assigned to DSP(SMEM)
	u32 dsp_att_cpn_size;

	u32 dsp_smem_base;
	u32 dsp_smem_size;

	u32 dsp_debug_daddr;
	u32 dsp_debug_size;

	u8 vouta_frm_buf_num;
	u8 vpipa_frm_buf_num;
	u8 voutb_frm_buf_num;
	u8 vpipb_frm_buf_num;

	u16 vouta_frm_buf_pitch;
	u16 vouta_frm_buf_height;

	u16 vpipa_frm_buf_pitch;
	u16 vpipa_frm_buf_height;

	u16 voutb_frm_buf_pitch;
	u16 voutb_frm_buf_height;

	u16 vpipb_frm_buf_pitch;
	u16 vpipb_frm_buf_height;
	u32 rsv[29];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_vout_reg_i1_s
{
	u32	*vo_ctl_a_base;
	u32	*vo_ctl_b_base;
	u32	*vo_ctl_dve_base;
	u32	*vo_ctl_osd_clut_a_base;
	u32	*vo_ctl_osd_clut_b_base;
	u32	vo_ctl_a_size;
	u32	vo_ctl_b_size;
	u32	vo_ctl_dve_size;
	u32	vo_ctl_osd_clut_a_size;
	u32	vo_ctl_osd_clut_b_size;
} __ATTRIB_PACK__;

#endif /* if (CHIP_REV == I1) */

#if (CHIP_REV == S2)
#define DSP_INIT_DATA_SIZE		256
#define DSP_INIT_DATA_ADDR		0xC00F0000

#define DSP_CMD_SIZE			128

#define	S2_DSP_RESULT_SIZE		256
#define S2_DSP_CMD_QUEUE_SIZE		4096
#define S2_DSP_RESULT_QUEUE_SIZE	4096
#define S2_DSP_DEFCFG_QUEUE_SIZE	4096
#define	S2_NUM_MSG_FIELDS		(S2_DSP_RESULT_SIZE / 4)
#define S2_NUM_DSP_CMDS        		((S2_DSP_CMD_QUEUE_SIZE - 8) / DSP_CMD_SIZE)
#define S2_NUM_DSP_RESULTS		(S2_DSP_RESULT_QUEUE_SIZE / S2_DSP_RESULT_SIZE)
#define S2_CMD_MAX_COUNT		S2_NUM_DSP_CMDS

#define CMD_HEADER_CODE			0x000000ab
#define MSG_HEADER_CODE			0x81000001
#define MSG_ENC_STATUS			0x82000001
#define MSG_JPG_ENC_STATUS		0x82000002

__ARMCC_PACK__ struct dsp_cmd_s
{
	u32     code;
	u8      param[DSP_CMD_SIZE - 4];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_header_cmd_s
{
	u32	code;
	u32	seqno;
	u32	ncmd;
	u8	param[DSP_CMD_SIZE - 12];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_cmd_data_s2_s
{
	struct dsp_header_cmd_s header_cmd;
	struct dsp_cmd_s cmd[S2_NUM_DSP_CMDS];
} __ATTRIB_PACK__;

struct dsp_cmd_data_s2_s cmd_data_s2;
struct dsp_cmd_data_s2_s def_cmd_data_s2;

__ARMCC_PACK__ struct result_msg_header_s2_s
{
	u32 msg_code;
	u32 dsp_mode;
	u32 time_code;
	u32 prev_cmd_seq;
	u32 prev_cmd_echo;
	u32 prev_num_cmds;
	u32 num_msgs;
	u32 rsv[S2_NUM_MSG_FIELDS - 7];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct result_msg_s2_s
{
	u32 field[S2_NUM_MSG_FIELDS];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_resqueue_s2_s
{
	struct result_msg_header_s2_s header_msg;
	struct result_msg_s2_s result[S2_NUM_DSP_RESULTS - 1];
} __ATTRIB_PACK__ result_queue_s2;

__ARMCC_PACK__ struct dsp_init_data_s2_s
{
	u32 default_binary_data_addr;
	u32 default_binary_data_size;

	/// General purpose cmd/msg channel for DSP-ARM
	u32 cmd_data_gen_daddr;
	u32 cmd_data_gen_size;
	u32 msg_queue_gen_daddr;
	u32 msg_queue_gen_size;

	/// Secondary cmd/msg channel for VCAP-ARM
	u32 cmd_data_vcap_daddr;
	u32 cmd_data_vcap_size;
	u32 msg_queue_vcap_daddr;
	u32 msg_queue_vcap_size;

	// Third
	u32 cmd_data_3rd_daddr;
	u32 cmd_data_3rd_size;
	u32 msg_queue_3rd_daddr;
	u32 msg_queue_3rd_size;

	// Fourth
	u32 cmd_data_4th_daddr;
	u32 cmd_data_4th_size;
	u32 msg_queue_4th_daddr;
	u32 msg_queue_4th_size;

	u32 default_config_daddr;
	u32 default_config_size;

	u32 dsp_buffer_daddr;
	u32 dsp_buffer_size;
	u32 dsp_buffer_pagetbl_daddr; // the tables of all physical pages corresponding to dsp_buffer_daddr
	u32 dsp_buffer_pagetbl_size;

	u32 dsp_att_cpn_start_offset; // the portion of att assigned to DSP(SMEM)
	u32 dsp_att_cpn_size;

	u32 dsp_smem_base;
	u32 dsp_smem_size;

	u32 dsp_debug_daddr;
	u32 dsp_debug_size;

	u8 vouta_frm_buf_num;
	u8 vpipa_frm_buf_num;
	u8 voutb_frm_buf_num;
	u8 vpipb_frm_buf_num;

	u16 vouta_frm_buf_pitch;
	u16 vouta_frm_buf_height;

	u16 vpipa_frm_buf_pitch;
	u16 vpipa_frm_buf_height;

	u16 voutb_frm_buf_pitch;
	u16 voutb_frm_buf_height;

	u16 vpipb_frm_buf_pitch;
	u16 vpipb_frm_buf_height;
	u32 rsv[29];
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct dsp_vout_reg_s2_s
{
	u32	*vo_ctl_a_base;
	u32	*vo_ctl_b_base;
	u32	*vo_ctl_dve_base;
	u32	*vo_ctl_osd_clut_a_base;
	u32	*vo_ctl_osd_clut_b_base;
	u32	vo_ctl_a_size;
	u32	vo_ctl_b_size;
	u32	vo_ctl_dve_size;
	u32	vo_ctl_osd_clut_a_size;
	u32	vo_ctl_osd_clut_b_size;
} __ATTRIB_PACK__;

#endif /* if (CHIP_REV == S2) */


/**
 * Base address for a7.
 */
#if (CHIP_REV == A7)

__ARMCC_PACK__ struct dsp_ucode_ver_s {
  u16 year;
  u8  month;
  u8  day;
  u16 edition_num;
  u16 edition_ver;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct stage_cmd_data_a7_s {
  struct dsp_cmd_data_a7_s data;
  int bits;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct ucode_base_addr_a7_s
{
  u32 dsp_start;
  u32 dsp_end;
  u32 code_addr;
  u32 sub0_addr;
  u32 sub1_addr;
  u32 defbin_addr;
  u32 shadow_reg_addr;
  u32 defimg_y_addr;
  u32 defimg_uv_addr;
  u32 dsp_buffer_addr;
  u32 dsp_buffer_size;
  u32 dsp_cmd_max_count;
  u32 *osd_a_buf_ptr;
  u32 *osd_b_buf_ptr;
  struct dsp_init_data_a7_s dsp_init_data;
  struct dsp_vout_reg_a7_s dsp_vout_reg;
  osd_t  osd;
} __ATTRIB_PACK__;

#endif

/**
 * Base address for i1.
 */
#if (CHIP_REV == I1)

__ARMCC_PACK__ struct dsp_ucode_ver_s {
	u16 year;
	u8  month;
	u8  day;
	u16 edition_num;
	u16 edition_ver;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct stage_cmd_data_i1_s {
	struct dsp_cmd_data_i1_s data;
	int bits;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct ucode_base_addr_i1_s
{
	u32 dsp_start;
	u32 dsp_end;
	u32 code_addr;
	u32 sub0_addr;
	u32 sub1_addr;
	u32 defbin_addr;
	u32 shadow_reg_addr;
	u32 defimg_y_addr;
	u32 defimg_uv_addr;
	u32 dsp_buffer_addr;
	u32 dsp_buffer_size;
	u32 dsp_cmd_max_count;
	u32 *osd_a_buf_ptr;
	u32 *osd_b_buf_ptr;
	struct dsp_init_data_i1_s dsp_init_data;
	struct dsp_vout_reg_i1_s dsp_vout_reg;
	osd_t  osd;
} __ATTRIB_PACK__;

#endif

/**
 * Base address for s2.
 */
#if (CHIP_REV == S2)

__ARMCC_PACK__ struct dsp_ucode_ver_s {
	u16 year;
	u8  month;
	u8  day;
	u16 edition_num;
	u16 edition_ver;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct stage_cmd_data_s2_s {
	struct dsp_cmd_data_s2_s data;
	int bits;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct ucode_base_addr_s2_s
{
	u32 dsp_start;
	u32 dsp_end;
	u32 code_addr;
	u32 sub0_addr;
	u32 sub1_addr;
	u32 defbin_addr;
	u32 shadow_reg_addr;
	u32 defimg_y_addr;
	u32 defimg_uv_addr;
	u32 dsp_buffer_addr;
	u32 dsp_buffer_size;
	u32 dsp_cmd_max_count;
	u32 *osd_a_buf_ptr;
	u32 *osd_b_buf_ptr;
	struct dsp_init_data_s2_s dsp_init_data;
	struct dsp_vout_reg_s2_s dsp_vout_reg;
	osd_t  osd;
} __ATTRIB_PACK__;

#endif


/**
 * Base address for a5s.
 */
#if (CHIP_REV == A5S)

__ARMCC_PACK__ struct dsp_ucode_ver_s {
  u16 year;
  u8  month;
  u8  day;
  u16 edition_num;
  u16 edition_ver;
} __ATTRIB_PACK__;

__ARMCC_PACK__ struct ucode_base_addr_a5s_s
{
  u32 dsp_start;
  u32 dsp_end;
  u32 code_addr;
  u32 sub0_addr;
  u32 defbin_addr;
  u32 shadow_reg_addr;
  u32 defimg_y_addr;
  u32 defimg_uv_addr;
  u32 dsp_buffer_addr;
  u32 dsp_buffer_size;
  u32 dsp_cmd_max_count;
  u32 *osd_a_buf_ptr;
  u32 *osd_b_buf_ptr;
  struct dsp_init_data_a5s_s dsp_init_data;
  struct dsp_vout_reg_a5s_s dsp_vout_reg;
  osd_t  osd;
} __ATTRIB_PACK__;

#endif

/**
 * For video_output_setup_ptr for a1/a2.
 */
#if (CHIP_REV == A2)
struct vout_setup_a2_s
{
  u8  format;
  u8  frame_rate;
  u8  au_samp_freq;
  u8  au_clk_freq;
  u16  width;
  u16  height;
  u32  osd1_addr;
  u32  osd2_addr;
  u16  osd1_width;
  u16  osd1_height;
  u16  osd1_pitch;
  u16  osd2_width;
  u16  osd2_height;
  u16  osd2_pitch;
  u32  defimg_y_addr;
  u32  defimg_uv_addr;
  u16  defimg_pitch;
  u16  defimg_height;
  u8  polarity;
  u8  flip_ctrl;
  u8  reset;
  u8  osd_prgsv;
} __attribute__ ((packed));
#endif

#if (CHIP_REV == A2S)
struct vout_setup_a2s_s
{
  u8  format;
  u8  frame_rate;
  u8  au_samp_freq;
  u8  au_clk_freq;
  u16  width;
  u16  height;
  u32  osd1_addr;
  u32  osd2_addr;
  u16  osd1_width;
  u16  osd1_height;
  u16  osd1_pitch;
  u16  osd2_width;
  u16  osd2_height;
  u16  osd2_pitch;
  u32  defimg_y_addr;
  u32  defimg_uv_addr;
  u16  defimg_pitch;
  u16  defimg_height;
  u8  polarity;
  u8  flip_ctrl;
  u8  reset;
  u8  osd_prgsv;
} __attribute__ ((packed));
#endif

/**
 * For video_output_setup_ptr for a3.
 */
#if (CHIP_REV == A3) || (CHIP_REV == A5)
struct vout_setup_a3_s
{
  u8  format;
  u8  frame_rate;
  u8  au_samp_freq;
  u8  au_clk_freq;
  u16  act_width;
  u16  act_height;
  u16  vid_width;
  u16  vid_height;
  u32  osd0_addr;
  u32  osd1_addr;
  u16  osd0_width;
  u16  osd1_width;
  u16  osd0_height;
  u16  osd1_height;
  u16  osd0_pitch;
  u16  osd1_pitch;
  u32  defimg_y_addr;
  u32  defimg_uv_addr;
  u16  defimg_pitch;
  u16  defimg_height;
  u8  polarity;
  u8  flip_ctrl;
  u8  reset;
  u8  osd_prgsv;
  u32  reserved[4];
} __attribute__ ((packed));
#endif

struct embbin_s {
  const char              *name;
  const unsigned char     *data;
  const int               len;
};

#if (CHIP_REV == A2)
struct ucode_base_addr_a2_s G_a2_ucode;
#endif

#if (CHIP_REV == A2S)
struct ucode_base_addr_a2s_s G_a2s_ucode;
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
struct ucode_base_addr_a3_s G_a3_ucode;
#endif

#if (CHIP_REV == A5S)
struct ucode_base_addr_a5s_s G_a5s_ucode;
#endif

#if (CHIP_REV == A7)
struct ucode_base_addr_a7_s G_a7_ucode;
#endif

#if (CHIP_REV == I1)
struct ucode_base_addr_i1_s G_i1_ucode;
#endif

#if (CHIP_REV == S2)
struct ucode_base_addr_s2_s G_s2_ucode;
#endif


#if 0
static void dsp_delay(int ms)
{
  u32 delay;

  /* Set timer for delay use*/
  timer_reset_count(TIMER2_ID);
  delay = timer_get_count(TIMER2_ID);
  timer_enable(TIMER2_ID);

  /* delay time */
  while(1) {
    delay = timer_get_count(TIMER2_ID);
    if (delay > ms)
      break;
  }

  timer_disable(TIMER2_ID);
}
#endif

/**
 * Get embedded binary data.
 */
int embbin_get(const char *name, const u8 **bin)
{
#if  !defined(SHOW_AMBOOT_SPLASH)
  *bin = NULL;
  return 0;
#else
  extern const struct embbin_s G_bld_embbin[];
  int i;

  *bin = NULL;

  for (i = 0; G_bld_embbin[i].len >= 0; i++) {
    if (strcmp(name, G_bld_embbin[i].name) == 0) {
      *bin = G_bld_embbin[i].data;
      return G_bld_embbin[i].len;
    }
  }

  return 0;
#endif
}


#if (CHIP_REV == A2)
static void a2_ucode_addr_init(u32 dsp_start,
  u32 dsp_end,
  struct ucode_base_addr_a2_s *ucode)
{
  ucode->dsp_start       = dsp_start;
  ucode->dsp_end         = dsp_end;

  ucode->code_addr       = ucode->dsp_end - 0x200000;
  ucode->memd_addr       = ucode->dsp_end - 0x100000;
  ucode->defbin_addr     = ucode->code_addr + 743 * 1024;
  ucode->shadow_reg_addr = ucode->defbin_addr + 256 *1024;
  ucode->defimg_y_addr   = ucode->shadow_reg_addr + 1 *1024;
  ucode->defimg_uv_addr  = ucode->defimg_y_addr + 12 * 1024;
  ucode->dsp_buffer_addr = ucode->dsp_start;
  ucode->dsp_buffer_size = ucode->dsp_end - ucode->dsp_start - 0x200000;
}

static int a2_dsp_boot(struct ucode_base_addr_a2_s *ucode)
{
  /* Set up the base address */
  writel(DRAM_A2_DSP_BASE_REG,
    ((ucode->code_addr & 0x0ff00000) >> 4 |
     (ucode->memd_addr & 0x0ff00000) >> 20));
  /* Enable the engines */
  writel(MEMD_BASE, 0xf);
  writel(CODE_BASE, 0xf);

  return 0;
}

static int a2_dsp_vout_set(int resolution,
  u32 defimg_y_addr,
  u32 defimg_uv_addr,
  u32 osd0_addr,
  u32 osd1_addr)
{
  u32 vout_set_addr;
  struct vout_setup_a2_s vout;
  int rval            = 0;

  vout_set_addr       = A2_VOUT_SET_PTR;

  vout.au_samp_freq   = 0;
  vout.au_clk_freq    = 0;
  vout.polarity       = 0;
  vout.flip_ctrl      = 0;
  vout.reset          = 0;
  vout.osd1_addr      = osd0_addr;
  vout.osd2_addr      = osd1_addr;
  vout.defimg_y_addr  = defimg_y_addr;
  vout.defimg_uv_addr = defimg_uv_addr;

  switch (resolution) {
  case VO_RGB_320_240:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.width         = 320;
    vout.height        = 240;
    vout.osd1_width    = 320;
    vout.osd1_height   = 480;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_240:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.width         = 360;
    vout.height        = 480;
    vout.osd1_width    = 360;
    vout.osd1_height   = 480;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_288:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.width         = 360;
    vout.height        = 576;
    vout.osd1_width    = 360;
    vout.osd1_height   = 576;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_480I:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.width         = 720;
    vout.height        = 480;
    vout.osd1_width    = 720;
    vout.osd1_height   = 480;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  case VO_576I:
    vout.format        = 1;
    vout.frame_rate    = 25;
    vout.width         = 720;
    vout.height        = 576;
    vout.osd1_width    = 720;
    vout.osd1_height   = 576;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  default:
    putstr("Unknow resolution");
    rval = -1;
    break;
  }

  memcpy((void *)vout_set_addr,
    (void *)&vout,
    sizeof(struct vout_setup_a2_s));

  return rval;
}
#endif

#if (CHIP_REV == A2S)
static void a2s_ucode_addr_init(u32 dsp_start,
  u32 dsp_end,
  struct ucode_base_addr_a2s_s *ucode)
{
  ucode->dsp_start       = dsp_start;
  ucode->dsp_end         = dsp_end;

  ucode->code_addr       = ucode->dsp_end - 0x200000;
  ucode->memd_addr       = ucode->dsp_end - 0x100000;
  ucode->defbin_addr     = ucode->code_addr + 743 * 1024;
  ucode->shadow_reg_addr = ucode->defbin_addr + 256 *1024;
  ucode->defimg_y_addr   = ucode->shadow_reg_addr + 1 *1024;
  ucode->defimg_uv_addr  = ucode->defimg_y_addr + 12 * 1024;
  ucode->dsp_buffer_addr = ucode->dsp_start;
  ucode->dsp_buffer_size = ucode->dsp_end - ucode->dsp_start - 0x200000;
#if 0
  putstr("\r\n");
  putstr("start :");
  puthex(ucode->dsp_start);
  putstr("\r\n");

  putstr("end :");
  puthex(ucode->dsp_end);
  putstr("\r\n");

  putstr("code_addr :");
  puthex(ucode->code_addr);
  putstr("\r\n");

  putstr("memd_addr :");
  puthex(ucode->memd_addr);
  putstr("\r\n");

  putstr("defbin_addr :");
  puthex(ucode->defbin_addr);
  putstr("\r\n");

  putstr("shadow_reg_addr :");
  puthex(ucode->shadow_reg_addr);
  putstr("\r\n");

  putstr("defimg_y_addr :");
  puthex(ucode->defimg_y_addr);
  putstr("\r\n");

  putstr("defimg_uv_addr :");
  puthex(ucode->defimg_uv_addr);
  putstr("\r\n");

  putstr("dsp_buffer_addr :");
  puthex(ucode->dsp_buffer_addr);
  putstr("\r\n");

  putstr("dsp_buffer_size :");
  puthex(ucode->dsp_buffer_size);
  putstr("\r\n");
#endif
}

static int a2s_dsp_boot(struct ucode_base_addr_a2s_s *ucode)
{
  /* Set up the base address */
  writel(DRAM_A2S_DSP_BASE_REG,
    ((ucode->code_addr & 0x0ff00000) >> 4 |
     (ucode->memd_addr & 0x0ff00000) >> 20));
  /* Enable the engines */
  writel(MEMD_BASE, 0xf);
  writel(CODE_BASE, 0xf);

  return 0;
}

static int a2s_dsp_vout_set(int resolution,
  u32 defimg_y_addr,
  u32 defimg_uv_addr,
  u32 osd0_addr,
  u32 osd1_addr)
{
  u32 vout_set_addr;
  struct vout_setup_a2s_s vout;
  int rval            = 0;

  vout_set_addr       = A2S_VOUT_SET_PTR;

  vout.au_samp_freq   = 0;
  vout.au_clk_freq    = 0;
  vout.polarity       = 0;
  vout.flip_ctrl      = 0;
  vout.reset          = 0;
  vout.osd1_addr      = osd0_addr;
  vout.osd2_addr      = osd1_addr;
  vout.defimg_y_addr  = defimg_y_addr;
  vout.defimg_uv_addr = defimg_uv_addr;

  switch (resolution) {
  case VO_RGB_320_240:
    vout.format        = 0;  /* 0: progressive, 1: interlaced */
    vout.frame_rate    = 0;
    vout.width         = 320;
    vout.height        = 240;
    vout.osd1_width    = 320;
    vout.osd1_height   = 240;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_240:
    vout.format        = 0;
    vout.frame_rate    = 0;
    vout.width         = 360;
    vout.height        = 240;
    vout.osd1_width    = 360;
    vout.osd1_height   = 240;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_288:
    vout.format        = 0;
    vout.frame_rate    = 0;
    vout.width         = 360;
    vout.height        = 288;
    vout.osd1_width    = 360;
    vout.osd1_height   = 288;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_480I:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.width         = 720;
    vout.height        = 480;
    vout.osd1_width    = 720;
    vout.osd1_height   = 480;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  case VO_576I:
    vout.format        = 1;
    vout.frame_rate    = 25;
    vout.width         = 720;
    vout.height        = 576;
    vout.osd1_width    = 720;
    vout.osd1_height   = 576;
    vout.osd1_pitch    = 1024;
    vout.osd2_width    = 0;
    vout.osd2_height   = 0;
    vout.osd2_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  default:
    putstr("Unknow resolution");
    rval = -1;
    break;
  }

  memcpy((void *)vout_set_addr,
    (void *)&vout,
    sizeof(struct vout_setup_a2s_s));

  return rval;
}
#endif


#if (CHIP_REV == A3) || (CHIP_REV == A5)
static void a3_ucode_addr_init(u32 dsp_start,
  u32 dsp_end,
  struct ucode_base_addr_a3_s *ucode)
{
  ucode->dsp_start       = dsp_start;
  ucode->dsp_end         = dsp_end;

  ucode->code_addr       = ucode->dsp_end - 0x500000;
  ucode->sub0_addr       = ucode->dsp_end - 0x400000;
  ucode->sub1_addr       = ucode->dsp_end - 0x300000;
  ucode->defbin_addr     = ucode->code_addr + 743 * 1024;
  ucode->shadow_reg_addr = ucode->defbin_addr + 256 *1024;
  ucode->defimg_y_addr   = ucode->shadow_reg_addr + 1 *1024;
  ucode->defimg_uv_addr  = ucode->defimg_y_addr + 12 * 1024;
  ucode->dsp_buffer_addr = ucode->dsp_start;
  ucode->dsp_buffer_size = ucode->dsp_end - ucode->dsp_start - 0x500000;

#if 0
  putstr("\r\n");
  putstr("start :");
  puthex(ucode->dsp_start);
  putstr("\r\n");

  putstr("end :");
  puthex(ucode->dsp_end);
  putstr("\r\n");

  putstr("code_addr :");
  puthex(ucode->code_addr);
  putstr("\r\n");

  putstr("sub0_addr :");
  puthex(ucode->sub0_addr);
  putstr("\r\n");

  putstr("sub1_addr :");
  puthex(ucode->sub1_addr);
  putstr("\r\n");

  putstr("defbin_addr :");
  puthex(ucode->defbin_addr);
  putstr("\r\n");

  putstr("shadow_reg_addr :");
  puthex(ucode->shadow_reg_addr);
  putstr("\r\n");

  putstr("defimg_y_addr :");
  puthex(ucode->defimg_y_addr);
  putstr("\r\n");

  putstr("defimg_uv_addr :");
  puthex(ucode->defimg_uv_addr);
  putstr("\r\n");

  putstr("dsp_buffer_addr :");
  puthex(ucode->dsp_buffer_addr);
  putstr("\r\n");

  putstr("dsp_buffer_size :");
  puthex(ucode->dsp_buffer_size);
  putstr("\r\n");
#endif

}

static int a3_dsp_boot(struct ucode_base_addr_a3_s *ucode)
{
  /* Set up the base address */
  writel(DSP_DRAM_MAIN_REG, ucode->code_addr);
  writel(DSP_DRAM_SUB0_REG, ucode->sub0_addr);
  writel(DSP_DRAM_SUB1_REG, ucode->sub1_addr);
  /* Enable the engines */
  writel(DSP_CONFIG_MAIN_REG, 0xf);
  writel(DSP_CONFIG_SUB0_REG, 0xf);
  writel(DSP_CONFIG_SUB1_REG, 0xf);

  return 0;
}

static int a3_dsp_vout_set(int id,
  int resolution,
  u32 defimg_y_addr,
  u32 defimg_uv_addr,
  u32 osd0_addr,
  u32 osd1_addr)
{
  u32 vout_set_addr = 0;
  struct vout_setup_a3_s vout;
  int rval = 0;

  if (id != 0 && id != 1)
    return -1;

  if (id == 0)
    vout_set_addr = A3_VOUT0_SET_PTR;
  else if (id == 1)
    vout_set_addr = A3_VOUT1_SET_PTR;

  memset(&vout, 0x0, sizeof(struct vout_setup_a3_s));

  vout.au_samp_freq   = 0;
  vout.au_clk_freq    = 0;
  vout.polarity       = 0;
  vout.flip_ctrl      = 0;
  vout.reset          = 0;
  vout.osd0_addr      = osd0_addr;
  vout.osd1_addr      = osd1_addr;
  vout.defimg_y_addr  = defimg_y_addr;
  vout.defimg_uv_addr = defimg_uv_addr;

  switch (resolution) {
  case VO_RGB_320_240:
    vout.format        = 0;
    vout.frame_rate    = 0;
    vout.act_width     = 320;
    vout.act_height    = 240;
    vout.vid_width     = 320;
    vout.vid_height    = 240;
    vout.osd0_width    = 320;
    vout.osd0_height   = 480;
    vout.osd0_pitch    = 1024;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_240:
    vout.format        = 0;
    vout.frame_rate    = 0;
    vout.act_width     = 360;
    vout.act_height    = 240;
    vout.vid_width     = 360;
    vout.vid_height    = 240;
    vout.osd0_width    = 360;
    vout.osd0_height   = 240;
    vout.osd0_pitch    = 1024;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_RGB_360_288:
    vout.format        = 0;
    vout.frame_rate    = 0;
    vout.act_width     = 360;
    vout.act_height    = 576;
    vout.vid_width     = 360;
    vout.vid_height    = 576;
    vout.osd0_width    = 360;
    vout.osd0_height   = 576;
    vout.osd0_pitch    = 1024;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 384;
    vout.defimg_height = 6;
    vout.osd_prgsv     = 1;
    break;
  case VO_480I:
    vout.format        = 1;
    vout.frame_rate    = 0;
    vout.act_width     = 720;
    vout.act_height    = 480;
    vout.vid_width     = 720;
    vout.vid_height    = 480;
    vout.osd0_width    = 720;
    vout.osd0_height   = 480;
    vout.osd0_pitch    = 1024;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  case VO_576I:
    vout.format        = 1;
    vout.frame_rate    = 25;
    vout.act_width     = 720;
    vout.act_height    = 576;
    vout.vid_width     = 720;
    vout.vid_height    = 576;
    vout.osd0_width    = 720;
    vout.osd0_height   = 576;
    vout.osd0_pitch    = 1024;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 736;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 1;
    break;
  default:
    vout.format        = 6;
    vout.frame_rate    = 0;
    vout.act_width     = 0;
    vout.act_height    = 0;
    vout.vid_width     = 0;
    vout.vid_height    = 0;
    vout.osd0_width    = 0;
    vout.osd0_height   = 0;
    vout.osd0_pitch    = 0;
    vout.osd1_width    = 0;
    vout.osd1_height   = 0;
    vout.osd1_pitch    = 0;
    vout.defimg_pitch  = 0;
    vout.defimg_height = 6;
    // Display OSD buffer in progressive mode even if video is in
    // interlaced mode. Setting this flag in interlaced mode has the
    // effect of vertical 2X scale. Bit0 - OSD_1. Bit 1 - OSD_2.
    vout.osd_prgsv = 0;

    putstr("Disable the VOUT");
    putdec(id);
    rval = -1;
    break;
  }

  /* Use indirect memory way to get the vout_setup data
     The 0xc1800000 will store the address of VOUT0 vout_setup data
     The 0xc1900000 will store the address of VOUT1 vout_setup data
     */
  if (id == 0) {
    memcpy((void *)vout_set_addr,
      (void *)&vout,
      sizeof(struct vout_setup_a3_s));
    writel(0xc1800000, vout_set_addr);
  } else {
    memcpy((void *)vout_set_addr,
      (void *)&vout,
      sizeof(struct vout_setup_a3_s));
    writel(0xc1900000, vout_set_addr);
  }

  return rval;
}
#endif

#if (CHIP_REV == A5S)

static struct ucode_base_addr_a5s_s* dsp_get_dsp_ptr(void)
{
  return &G_a5s_ucode;
}

/**
 * Get aligned hunge buffer.
 *
 * @param buf     - pointer to the aligned buffer pointer
 * @param raw_buf - the original buf address
 * @param size    - size of aligned buffer
 * @param align   - align byte
 * @returns       - 0 if successful, < 0 if failed
 */
static int dsp_get_align_buf(u8 **buf, u32 raw_buf, u32 size, u32 align)
{
  u32 misalign;

  size += align << 1;

  misalign = (u32)(raw_buf) & (align - 1);

  *buf = (u8 *)((u32)(raw_buf) + (align - misalign));

  return 0;
}

int dsp_set_vout_reg(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a5s_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr = VOUT_BASE;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
  addr += offset;
#else
  if (vout_id == vout_a) {
    offset = offset - VOUT_DA_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
  } else {
    offset = offset - VOUT_DB_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
  }
#endif
  writel(addr, val);

#if 0
  dsp_delay(1);
  dsp_print_var_value("addr",  addr,  "hex");
  dsp_print_var_value("offset",  offset,  "hex");
  dsp_print_var_value("value",  val,  "hex");
#endif
  return 0;
}

u32 dsp_get_vout_reg(int vout_id, u32 offset)
{
  struct ucode_base_addr_a5s_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr = VOUT_BASE;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
  addr += offset;
#else
  if (vout_id == vout_a) {
    offset = offset - VOUT_DA_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
  } else {
    offset = offset - VOUT_DB_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
  }
#endif
  return readl(addr);
}

int dsp_set_vout_osd_clut(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a5s_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();

  if (vout_id == vout_a) {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base + offset;
  } else {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base + offset;
  }

  writel(addr, val);
#if 0
  dsp_print_var_value("Addr", addr, "hex");
  dsp_print_var_value("value", val, "hex");
#endif
  return 0;
}

int dsp_set_vout_dve(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a5s_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();

  if (vout_id == vout_a) {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
  } else {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
  }

  writel(addr, val);
#if 0
  dsp_print_var_value("Addr", addr, "hex");
  dsp_print_var_value("value", val, "hex");
#endif
  return 0;
}

#if (A5S_SPLASH_DBG == 1)
static int dsp_print_var_value(const char *var_str, u32 value, char *format)
{
  if (strcmp(format, "hex") == 0) {
    putstr("0x");
    puthex((u32)value);
  } else if (strcmp(format, "dec") == 0) {
    putdec((u32)value);
  } else {
    putstr("Warning : Unknown data format, should be ""hex"" or ""dec""! ");
    return -1;
  }
  putstr(" : ");
  putstr(var_str);
  putstr("\r\n");

  return 0;
}

void dsp_memory_allocation_print(struct ucode_base_addr_a5s_s *ucode)
{
  struct dsp_vout_reg_a5s_s  *vout;

  vout = &ucode->dsp_vout_reg;

  dsp_print_var_value("ucode->dsp_start",
    (u32)ucode->dsp_start, "hex");

  dsp_print_var_value("ucode->dsp_end",
    (u32)ucode->dsp_end, "hex");

  dsp_print_var_value("ucode->code_addr",
    (u32)ucode->code_addr, "hex");

  dsp_print_var_value("ucode->sub0_addr",
    (u32)ucode->sub0_addr, "hex");

  dsp_print_var_value("ucode->defbin_addr",
    (u32)ucode->defbin_addr, "hex");

  dsp_print_var_value("ucode->shadow_reg_addr",
    (u32)ucode->shadow_reg_addr, "hex");

  dsp_print_var_value("ucode->defimg_y_addr",
    (u32)ucode->defimg_y_addr, "hex");

  dsp_print_var_value("ucode->defimg_uv_addr",
    (u32)ucode->defimg_uv_addr, "hex");

  dsp_print_var_value("ucode->dsp_buffer_addr",
    (u32)ucode->defimg_y_addr, "hex");

  dsp_print_var_value("ucode->dsp_buffer_size",
    (u32)ucode->defimg_uv_addr, "hex");

  dsp_print_var_value("dsp_init_data.cmd_data_ptr",
    (u32)ucode->dsp_init_data.cmd_data_ptr, "hex");

  dsp_print_var_value("dsp_init_data.res_queue_ptr",
    (u32)ucode->dsp_init_data.res_queue_ptr, "hex");

  dsp_print_var_value("dsp_init_data.def_config_ptr",
    (u32)ucode->dsp_init_data.def_config_ptr, "hex");

  dsp_print_var_value("vout->vo_ctl_a_base",
    (u32)vout->vo_ctl_a_base, "hex");

  dsp_print_var_value("vout->vo_ctl_b_base",
    (u32)vout->vo_ctl_b_base, "hex");

  dsp_print_var_value("vout->vo_ctl_dve_base",
    (u32)vout->vo_ctl_dve_base, "hex");

  dsp_print_var_value("vout->vo_ctl_osd_clut_a_base",
    (u32)vout->vo_ctl_osd_clut_a_base, "hex");

  dsp_print_var_value("vout->vo_ctl_osd_clut_b_base",
    (u32)vout->vo_ctl_osd_clut_b_base, "hex");

  dsp_print_var_value("ucode->osd_a_buf_ptr",
    (u32)ucode->osd_a_buf_ptr, "hex");

  dsp_print_var_value("ucode->osd_b_buf_ptr",
    (u32)ucode->osd_b_buf_ptr, "hex");
}

static void dsp_get_ucode_version(void)
{
  u32 addr, tmp;
  struct ucode_base_addr_a5s_s* ucode;
  struct dsp_ucode_ver_s version;

  ucode = dsp_get_dsp_ptr();
  addr = ucode->code_addr + 36;
  tmp = *((u32 *) addr);

  version.year  = (tmp & 0xFFFF0000) >> 16;
  version.month = (tmp & 0x0000FF00) >> 8;
  version.day   = (tmp & 0x000000FF);

  addr = ucode->code_addr + 32;
  tmp = *(u32 *)addr;

  version.edition_num = (tmp & 0xFFFF0000) >> 16;
  version.edition_ver = (tmp & 0x0000FFFF);

  dsp_print_var_value("year",      version.year,   "dec");
  dsp_print_var_value("month",      version.month,   "dec");
  dsp_print_var_value("day",      version.day,   "dec");
  dsp_print_var_value("edition_num", version.edition_num, "dec");
  dsp_print_var_value("edition_ver", version.edition_ver, "dec");
}
#endif

int osd_init(u16 resolution, osd_t *osdobj)
{
  int rval = 0;

  osdobj->resolution = resolution;

  switch (resolution) {
  case VO_RGB_800_480:
    osdobj->width  = 800;
    osdobj->height = 480;
    osdobj->pitch  = 800;
    break;
  case VO_RGB_480_800:
    osdobj->width  = 480;
    osdobj->height = 800;
    osdobj->pitch  = 480;
    break;
  case VO_RGB_360_240:
    osdobj->width  = 360;
    osdobj->height = 240;
    osdobj->pitch  = 384;
    break;
  case VO_RGB_360_288:
    osdobj->width  = 360;
    osdobj->height = 288;
    osdobj->pitch  = 384;
    break;
  case VO_RGB_320_240:
    osdobj->width  = 320;
    osdobj->height = 240;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_320_288:
    osdobj->width  = 320;
    osdobj->height = 288;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_320_480:
    osdobj->width  = 320;
    osdobj->height = 480;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_960_240:
    osdobj->width  = 960;
    osdobj->height = 240;
    osdobj->pitch  = 960;
    break;
  case VO_RGB_480_240:
    osdobj->width  = 480;
    osdobj->height = 240;
    osdobj->pitch  = 480;
    break;
  case VO_RGB_480P:
    osdobj->width  = 720;
    osdobj->height = 480;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_576P:
    osdobj->width  = 720;
    osdobj->height = 576;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_480I:
    osdobj->width  = 720;
    osdobj->height = 480;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_576I:
    osdobj->width  = 720;
    osdobj->height = 576;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_240_432:
    osdobj->width  = 240;
    osdobj->height = 432;
    osdobj->pitch  = 256;
    break;
  default :
    osdobj->width  = 360;
    osdobj->height = 240;
    osdobj->pitch  = 0;
    rval = -1;
    break;
  }

  return rval;
}

/**
 * Copy BMP to OSD memory
 */
static int bmp2osd_mem(bmp_t *bmp, osd_t *osdobj, int x, int y, int top_botm_revs)
{
  int point_idx = 0;
  int i = 0, j = 0;

  if ((osdobj->buf == NULL) ||
    (osdobj->width < 0) || (osdobj->height < 0))
    return -1;

  /* set the first line to move */
  if (top_botm_revs == 0)
    point_idx = y * osdobj->pitch + x;
  else
    point_idx = (y + (bmp->height - 1)) * osdobj->pitch + x;
#if 0
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

  bmp_t* splash_bmp = NULL;
  int x = 0, y = 0;
  int rval = -1;

  if (posd == NULL) {
    putstr("Splash_Err: NULL pointer of OSD buffer!");
    return -1;
  }

  /* Get the spalsh bmp address and osd buffer address */
#if defined(SHOW_AMBOOT_SPLASH)
  splash_bmp = bld_get_splash_bmp();
#endif
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

void dsp_set_splash_bmp2osd_buf(int chan, int res, int bot_top_res)
{
  struct ucode_base_addr_a5s_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  int rval   = -1;

  vout_a     = dsp_get_vout_a();
  ucode      = dsp_get_dsp_ptr();

  osd_init(res, &ucode->osd);

  if (chan == vout_a)
    ucode->osd.buf = (u8 *)ucode->osd_a_buf_ptr;
  else
    ucode->osd.buf = (u8 *)ucode->osd_b_buf_ptr;

  /* Clear OSD Buffer */
  memset(ucode->osd.buf, 0, DSP_OSD_BUF_SIZE);

  /* Copy splash BMP to OSD buffer */
  rval = splash_bmp2osd_buf(chan, &ucode->osd, bot_top_res);
  if (rval != 0) {
    putstr("Splash_Err: Copy splahs to OSD fail!");
  }
}


static void a5s_ucode_addr_init(u32 dsp_start, u32 dsp_end,
  struct ucode_base_addr_a5s_s *ucode)
{
  /** In normal ucode, the DSP buffer is used fo encode/decode data used,
   *  here in the splash, use the available for VOUT memory mapping
   *  registers, CLUT memory mapping registers, OSD data.
   */
  //#  +-----------------------------------------+ DSP_INIT_DATA_ADDR
  //#  |                                         | 0xC00F0000
  //#  |                                         |
  //#  +-----------------------------------------+ dsp_start
  //#  |                                         | A5S_SPLASH_START
  //#  |                                         | cmd_data_ptr
  //#  |                     4K                  |
  //#  +-----------------------------------------+ res_queue_ptr
  //#  |                     4K                  |
  //#  +-----------------------------------------+ def_config_ptr
  //#  |                     4K                  |
  //#  +-----------------------------------------+ vo_ctl_a_base
  //#  |                   372 * 4               |
  //#  +-----------------------------------------+ vo_ctl_b_base
  //#  |                   372 * 4               |
  //#  +-----------------------------------------+ vo_ctl_dve_base
  //#  |                   512 * 4               |
  //#  +-----------------------------------------+ vo_ctl_osd_clut_a_base
  //#  |                  1024 * 4               |
  //#  +-----------------------------------------+ vo_ctl_osd_clut_b_base
  //#  |                  1024 * 4               |
  //#  +-----------------------------------------+ osd0_buf_addr
  //#  |                360 * 240 * 2            |
  //#  +-----------------------------------------+ osd1_buf_addr
  //#  |                360 * 240 * 2            |
  //#  +-----------------------------------------+ dsp_end - 0x8000
  //#  |               orccode.bin:31KB          |
  //#  +-----------------------------------------+ dsp_end - 0xA0
  //#  |                orcme.bin:140B           |
  //#  +-----------------------------------------+ dsp_end
  //#                A5S_SPLASH_END
  //#
  struct dsp_init_data_a5s_s *dsp;
  struct dsp_vout_reg_a5s_s  *vout;

  memset(ucode, 0x0, sizeof(struct ucode_base_addr_a5s_s));

  ucode->dsp_start       = dsp_start; // A5S_SPLASH_START
  ucode->dsp_end         = dsp_end;   // A5S_SPLASH_END

  ucode->code_addr       = ucode->dsp_end - 0x00008000;
  ucode->sub0_addr       = ucode->dsp_end - 0x000000A0; // Must be 32 bytes aligned
  ucode->defbin_addr     = 0;
  ucode->shadow_reg_addr = 0;
  ucode->defimg_y_addr   = 0;
  ucode->defimg_uv_addr  = 0;
  ucode->dsp_buffer_addr = IDSP_RAM_START;
  ucode->dsp_buffer_size = 0x008000;

  ucode->dsp_init_data.default_binary_data = (u32 *)ucode->defbin_addr;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_ptr,
    ucode->dsp_start, A5S_DSP_CMD_QUEUE_SIZE, 32);
  ucode->dsp_init_data.cmd_data_size = A5S_DSP_CMD_QUEUE_SIZE;
  memset(ucode->dsp_init_data.cmd_data_ptr, 0x00, A5S_DSP_CMD_QUEUE_SIZE);

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.res_queue_ptr,
    ((u32)ucode->dsp_init_data.cmd_data_ptr +
     A5S_DSP_CMD_QUEUE_SIZE),
    A5S_DSP_RESULT_QUEUE_SIZE, 32);

  ucode->dsp_init_data.res_queue_size = A5S_DSP_RESULT_QUEUE_SIZE;
  memset(ucode->dsp_init_data.res_queue_ptr,
    0x00, A5S_DSP_RESULT_QUEUE_SIZE);

  ucode->dsp_init_data.op_mode = DECODE_MODE;

  /* default config dsp commands pointer */
  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.def_config_ptr,
    ((u32)ucode->dsp_init_data.res_queue_ptr +
     A5S_DSP_RESULT_QUEUE_SIZE),
    A5S_DSP_DEFCFG_QUEUE_SIZE, 32);
  ucode->dsp_init_data.def_config_size = A5S_DSP_DEFCFG_QUEUE_SIZE;
  memset(ucode->dsp_init_data.def_config_ptr,
    0x00, A5S_DSP_DEFCFG_QUEUE_SIZE);

  ucode->dsp_init_data.dsp_buf_ptr         = (u32 *)ucode->dsp_buffer_addr;

  ucode->dsp_init_data.dsp_buf_size        = ucode->dsp_buffer_size;

  ucode->dsp_cmd_max_count                 = A5S_CMD_MAX_COUNT;
  ucode->dsp_init_data.cmd_data_ptr->seqno = 0;
  ucode->dsp_init_data.cmd_data_ptr->ncmd  = 0;

  vout                                     = &ucode->dsp_vout_reg;
  vout->vo_ctl_a_size                      = DSP_VOUT_A_CTLREG_BUF_SIZE;
  vout->vo_ctl_b_size                      = DSP_VOUT_B_CTLREG_BUF_SIZE;
  vout->vo_ctl_dve_size                    = DSP_VOUT_DVE_CTLREG_BUF_SIZE;
  vout->vo_ctl_osd_clut_a_size             = DSP_VOUT_OSD_CTRREG_BUF_SIZE;
  vout->vo_ctl_osd_clut_b_size             = DSP_VOUT_OSD_CTRREG_BUF_SIZE;

  dsp_get_align_buf((u8 **)&vout->vo_ctl_a_base,
    ((u32)ucode->dsp_init_data.def_config_ptr +
     A5S_DSP_DEFCFG_QUEUE_SIZE),
    DSP_VOUT_A_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_b_base,
    ((u32)vout->vo_ctl_a_base +
     DSP_VOUT_A_CTLREG_BUF_SIZE),
    DSP_VOUT_B_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_dve_base,
    ((u32)vout->vo_ctl_b_base +
     DSP_VOUT_B_CTLREG_BUF_SIZE),
    DSP_VOUT_DVE_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_a_base,
    ((u32)vout->vo_ctl_dve_base +
     DSP_VOUT_DVE_CTLREG_BUF_SIZE),
    DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_b_base,
    ((u32)vout->vo_ctl_osd_clut_a_base +
     DSP_VOUT_OSD_CTRREG_BUF_SIZE),
    DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);

  dsp_get_align_buf((u8 **)&ucode->osd_a_buf_ptr,
    ((u32)vout->vo_ctl_osd_clut_b_base +
     DSP_VOUT_OSD_CTRREG_BUF_SIZE),
    DSP_OSD_BUF_SIZE, 32);

  dsp_get_align_buf((u8 **)&ucode->osd_b_buf_ptr,
    ((u32)ucode->osd_a_buf_ptr +
     DSP_OSD_BUF_SIZE),
    DSP_OSD_BUF_SIZE, 32);

  memset(vout->vo_ctl_a_base, 0x00, DSP_VOUT_A_CTLREG_BUF_SIZE);
  memset(vout->vo_ctl_b_base, 0x00, DSP_VOUT_B_CTLREG_BUF_SIZE);

  /* Put the dsp_init_data at this address */
  dsp = (struct dsp_init_data_a5s_s *)DSP_INIT_DATA_ADDR;
  memset((u8 *)dsp, 0, sizeof(struct dsp_init_data_a5s_s));
  memcpy((u8 *)dsp, &ucode->dsp_init_data, sizeof(struct dsp_init_data_a5s_s));
  clean_d_cache((u8 *)DSP_INIT_DATA_ADDR, sizeof(struct dsp_init_data_a5s_s));

#if (A5S_SPLASH_DBG == 1)
  dsp_memory_allocation_print(ucode);
#endif
}

#if 0
u8 *dsp_get_osd_ptr(u16 vout_id)
{
  int vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a5s_s* ucode;

  ucode  = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();

  if (vout_id == vout_a)
    return (u8 *)(ucode->osd_a_buf_ptr);
  else
    return (u8 *)(ucode->osd_b_buf_ptr);
}
#endif

static int dsp_write_to_command_queue(struct ucode_base_addr_a5s_s *ucode,
  u8* cmd, u32 align_cmd_len)
{
  struct dsp_cmd_data_a5s_s *cmd_queue_ptr;
  u32 cmd_max_count = 0;
  int i = 0;

  cmd_queue_ptr   = ucode->dsp_init_data.cmd_data_ptr;
  cmd_max_count   = ucode->dsp_cmd_max_count;

  cmd_queue_ptr->seqno++;
  i = cmd_queue_ptr->ncmd;

  if (cmd_queue_ptr->ncmd < cmd_max_count) {
    /* clean buffer to 0 */
    memset((cmd_queue_ptr->cmd + i), 0, sizeof(struct dsp_cmd_s));
    memcpy((cmd_queue_ptr->cmd + i), cmd, align_cmd_len);
#if 0
{
	u32	*cmd2 = (u32 *)(cmd_queue_ptr->cmd + i);
	int	j;

	putstr("cmd ");
	putdec(i);
	putstr(":\r\n");
	for (j = 0; j < 128 / 4; j++) {
		puthex(j);
		putstr(": ");
		puthex(cmd2[j]);
		putstr("\r\n");
	}
	putstr("\r\n");
}
#endif
    cmd_queue_ptr->ncmd = cmd_queue_ptr->ncmd + 1;
  } else {
    return -1;
  }

  //  dsp_delay(10);

  return 0;
}

int dsp_get_win_params(int resolution, u16 *width, u16 *height)
{
  int rval = 0;

  switch (resolution) {
  case VO_RGB_800_480:
    *width  = 800;
    *height  = 480;
    break;
  case VO_RGB_360_240:
    *width  = 360;
    *height  = 240;
    break;
  case VO_RGB_360_288:
    *width  = 360;
    *height  = 288;
    break;
  case VO_RGB_320_240:
    *width  = 320;
    *height  = 240;
    break;
  case VO_RGB_320_288:
    *width  = 320;
    *height  = 288;
    break;
  case VO_RGB_320_480:
    *width  = 320;
    *height = 480;
    break;
  case VO_RGB_960_240:
    *width  = 960;
    *height  = 240;
    break;
  case VO_RGB_480_240:
    *width  = 480;
    *height  = 240;
    break;
  case VO_RGB_480_800:
    *width  = 480;
    *height = 800;
    break;
  case VO_RGB_480P:
    *width  = 720;
    *height = 480;
    break;
  case VO_RGB_576P:
    *width  = 720;
    *height = 576;
    break;
  case VO_RGB_480I:
    *width  = 720;
    *height = 480 / 2;
    break;
  case VO_RGB_576I:
    *width  = 720;
    *height = 576 / 2;
    break;
  case VO_RGB_240_432:
    *width  = 240;
    *height = 432;
    break;
  default :
    *width  = 0;
    *height = 0;
    rval    = -1;
    break;
  }

  return rval;
}

int dsp_vout_mixer_setup(u16 vout_id, int resolution)
{
  struct ucode_base_addr_a5s_s* ucode;
  vout_mixer_setup_t  dsp_cmd;
  u8  interlaced               = 0;
  u8  frame_rate               = 0;
  u16 active_win_width         = 0;
  u16 active_win_height        = 0;
  u8  back_ground_v            = DEFAULT_BACK_GROUND_V;
  u8  back_ground_u            = DEFAULT_BACK_GROUND_U;
  u8  back_ground_y            = DEFAULT_BACK_GROUND_Y;
  u8  reserved                 = 0;
  u8  highlight_v              = DEFAULT_HIGHLIGHT_V;
  u8  highlight_u              = DEFAULT_HIGHLIGHT_U;
  u8  highlight_y              = DEFAULT_HIGHLIGHT_Y;
  u8  highlight_thresh         = DEFAULT_HIGHLIGHT_THRESH;
  int rval                     = -1;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &active_win_width, &active_win_height);

  if (rval != 0)
    return -1;

  switch (resolution) {
  case VO_RGB_800_480:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_360_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_288:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_480:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_360_288:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_960_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480_800:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480P:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_576P:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480I:
    interlaced    = 1;
    frame_rate    = 1;
    break;
  case VO_RGB_576I:
    interlaced    = 1;
    frame_rate    = 1;
    break;
  case VO_RGB_240_432:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  default:
    break;
  }

  dsp_cmd.cmd_code         = VOUT_MIXER_SETUP;
  dsp_cmd.vout_id          = vout_id;
  dsp_cmd.interlaced       = interlaced;
  dsp_cmd.frm_rate         = frame_rate;
  dsp_cmd.act_win_width    = active_win_width;
  dsp_cmd.act_win_height   = active_win_height;
  dsp_cmd.back_ground_v    = back_ground_v;
  dsp_cmd.back_ground_u    = back_ground_u;
  dsp_cmd.back_ground_y    = back_ground_y;
  dsp_cmd.reserved         = reserved;
  dsp_cmd.highlight_v      = highlight_v;
  dsp_cmd.highlight_u      = highlight_u;
  dsp_cmd.highlight_y      = highlight_y;
  dsp_cmd.highlight_thresh = highlight_thresh;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_video_setup(u16 vout_id, int video_en, int video_src, int resolution)
{
  struct ucode_base_addr_a5s_s* ucode;
  vout_video_setup_t  dsp_cmd;
  u8  flip = 0;
  u8  rotate = 0;
  u16 reserved = 0;
  u16 win_offset_x = VIDEO_WIN_OFFSET_X;
  u16 win_offset_y = VIDEO_WIN_OFFSET_Y;
  u16 win_width  = 0;
  u16 win_height = 0;
  u16 default_img_pitch = 0;
  u8  default_img_repeat_field = 0;
  u8  reserved2 = 0;
  int rval = -1;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &win_width, &win_height);
  if (rval != 0)
    return -1;

  default_img_pitch                = (((int)(win_width + 31) / 32) * 32);

  dsp_cmd.cmd_code                 = VOUT_VIDEO_SETUP;
  dsp_cmd.vout_id                  = vout_id;
  dsp_cmd.en                       = video_en;
  dsp_cmd.src                      = video_src;
  dsp_cmd.flip                     = flip;
  dsp_cmd.rotate                   = rotate;
  dsp_cmd.reserved                 = reserved;
  dsp_cmd.win_offset_x             = win_offset_x;
  dsp_cmd.win_offset_y             = win_offset_y;
  dsp_cmd.win_width                = win_width;
  dsp_cmd.win_height               = win_height;

  dsp_cmd.default_img_y_addr       = ucode->defimg_y_addr;
  dsp_cmd.default_img_uv_addr      = ucode->defimg_uv_addr;
  dsp_cmd.default_img_pitch        = default_img_pitch;
  dsp_cmd.default_img_repeat_field = default_img_repeat_field;
  dsp_cmd.reserved2                = reserved2;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_setup(u16 vout_id, u8 flip, u8 osd_en, u8 osd_src, int resolution)
{
  struct ucode_base_addr_a5s_s* ucode;
  vout_osd_setup_t  dsp_cmd;
  u16 vout_a                   = VOUT_DISPLAY_A;
  u8  rescaler_en              = 0;
  u8  premultiplied            = 0;
  u8  global_blend             = 0xff;
  u16  win_offset_x            = 0;
  u16  win_offset_y            = 0;
  u16  win_width               = 0;
  u16  win_height              = 0;
  u16  rescaler_input_width    = 0;
  u16  rescaler_input_height   = 0;
  u16  osd_buf_pitch           = 0;
  u8  osd_buf_repeat           = 0;
  u8  osd_direct_mode          = 0;
  u16  osd_transparent_color   = 0;
  u8  osd_transparent_color_en = 0;
  int  rval                    = -1;

  ucode = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &win_width, &win_height);
  if (rval != 0)
    return -1;

  osd_buf_pitch   = (((int)(win_width + 31) / 32) * 32);

  rescaler_input_width          = win_width;
  rescaler_input_height         = win_height;

  dsp_cmd.cmd_code              = VOUT_OSD_SETUP;
  dsp_cmd.vout_id               = vout_id;
  dsp_cmd.en                    = osd_en;
  dsp_cmd.src                   = osd_src;
  dsp_cmd.flip                  = flip;
  dsp_cmd.rescaler_en           = rescaler_en;
  dsp_cmd.premultiplied         = premultiplied;
  dsp_cmd.global_blend          = global_blend;
  dsp_cmd.win_offset_x          = win_offset_x;
  dsp_cmd.win_offset_y          = win_offset_y;
  dsp_cmd.win_width             = win_width;
  dsp_cmd.win_height            = win_height;
  dsp_cmd.rescaler_input_width  = rescaler_input_width;
  dsp_cmd.rescaler_input_height = rescaler_input_height;
  if (vout_id == vout_a)
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
  else
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

  dsp_cmd.osd_buf_pitch            = osd_buf_pitch;
  dsp_cmd.osd_buf_repeat_field     = osd_buf_repeat;
  dsp_cmd.osd_direct_mode          = osd_direct_mode;
  dsp_cmd.osd_transparent_color    = osd_transparent_color;
  dsp_cmd.osd_transparent_color_en = osd_transparent_color_en;
  dsp_cmd.osd_buf_info_dram_addr   = 0x0;  //For A7M, not used in A5S

  /* FIXME: hardcode for test Funai 960x240 splash bmp file */
  //  dsp_cmd.rescaler_en = 1;
  //  dsp_cmd.rescaler_input_width = 360;
  //  dsp_cmd.win_width = 960;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_buf_setup(u16 vout_id, int resolution)
{
  struct ucode_base_addr_a5s_s* ucode;
  vout_osd_buf_setup_t  dsp_cmd;
  u16   vout_a = VOUT_DISPLAY_A;
  u16  osd_buf_pitch = 0;
  u8  osd_buf_repeat = 0;

  ucode  = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_OSD_BUFFER_SETUP;
  dsp_cmd.vout_id = vout_id;

  if (vout_id == vout_a)
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
  else
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

  switch (resolution) {
  case VO_RGB_800_480:
    osd_buf_pitch  = 800;
    break;
  case VO_RGB_360_240:
    osd_buf_pitch  = 384;
    break;
  case VO_RGB_360_288:
    osd_buf_pitch  = 384;
    break;
  case VO_RGB_320_240:
    osd_buf_pitch  = 320;
    break;
  case VO_RGB_320_288:
    osd_buf_pitch  = 320;
    break;
  case VO_RGB_320_480:
    osd_buf_pitch = 320;
    break;
  case VO_RGB_960_240:
    osd_buf_pitch  = 960;
    break;
  case VO_RGB_480_240:
    osd_buf_pitch  = 480;
    break;
  case VO_RGB_480_800:
    osd_buf_pitch  = 480;
    break;
  case VO_RGB_480P:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_576P:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_480I:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_576I:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_240_432:
    osd_buf_pitch  = 256;
    break;
  default :
    break;
  }

  dsp_cmd.osd_buf_pitch     = osd_buf_pitch;
  dsp_cmd.osd_buf_repeat_field   = osd_buf_repeat;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_clut_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a5s_s* ucode;
  vout_osd_clut_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code  = VOUT_OSD_CLUT_SETUP;
  dsp_cmd.vout_id    = vout_id;

  if (vout_id == vout_a)
    dsp_cmd.clut_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base;
  else
    dsp_cmd.clut_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_display_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a5s_s* ucode;
  vout_display_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_DISPLAY_SETUP;
  dsp_cmd.vout_id  = vout_id;
  if (vout_id == vout_a)
    dsp_cmd.disp_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_a_base;
  else
    dsp_cmd.disp_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_b_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_dve_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a5s_s* ucode;
  vout_dve_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_DVE_SETUP;
  dsp_cmd.vout_id  = vout_id;
  if (vout_id == vout_a)
    dsp_cmd.dve_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_dve_base;
  else
    dsp_cmd.dve_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_dve_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_reset(u16 vout_id, u8 reset_mixer, u8  reset_disp)
{
  struct ucode_base_addr_a5s_s* ucode;
  vout_reset_t  dsp_cmd;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code    = VOUT_RESET;
  dsp_cmd.vout_id     = vout_id;
  dsp_cmd.reset_mixer = reset_mixer;
  dsp_cmd.reset_disp  = reset_disp;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

static int a5s_dsp_boot(struct ucode_base_addr_a5s_s *ucode)
{
  /* Set up the base address */
  writel(DSP_DRAM_MAIN_REG, ucode->code_addr);
  writel(DSP_DRAM_SUB0_REG, ucode->sub0_addr);

  /* Enable the engines */
  writel(DSP_CONFIG_MAIN_REG, 0xf);
  writel(DSP_CONFIG_SUB0_REG, 0xf);

  return 0;
}

int dsp_dram_clean_cache(void)
{
  struct ucode_base_addr_a5s_s* ucode;

  ucode = dsp_get_dsp_ptr();
  if (ucode == NULL)
    return -1;

  /* clean d cache in these area because
     all bld memory all set to cachable!!! */
  clean_d_cache((void *)ucode->dsp_init_data.cmd_data_ptr,
    ucode->dsp_init_data.cmd_data_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_a_base,
    ucode->dsp_vout_reg.vo_ctl_a_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_b_base,
    ucode->dsp_vout_reg.vo_ctl_b_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base,
    ucode->dsp_vout_reg.vo_ctl_osd_clut_a_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base,
    ucode->dsp_vout_reg.vo_ctl_osd_clut_b_size);
  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_dve_base,
    ucode->dsp_vout_reg.vo_ctl_dve_size);
  return 0;
}
#endif


#if (CHIP_REV == A7)

static struct ucode_base_addr_a7_s* dsp_get_dsp_ptr(void)
{
  return &G_a7_ucode;
}

/**
 * Get aligned hunge buffer.
 *
 * @param buf     - pointer to the aligned buffer pointer
 * @param raw_buf - the original buf address
 * @param size    - size of aligned buffer
 * @param align   - align byte
 * @returns       - 0 if successful, < 0 if failed
 */
static int dsp_get_align_buf(u8 **buf, u32 raw_buf, u32 size, u32 align)
{
  u32 misalign;

  size += align << 1;

  misalign = (u32)(raw_buf) & (align - 1);

  *buf = (u8 *)((u32)(raw_buf) + (align - misalign));

  return 0;
}

int dsp_set_vout_reg(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a7_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr = VOUT_BASE;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
  addr += offset;
#else
  if (vout_id == vout_a) {
    offset = offset - VOUT_DA_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
  } else {
    offset = offset - VOUT_DB_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
  }
#endif
  writel(addr, val);

#if 0
  dsp_delay(1);
  dsp_print_var_value("addr",  addr,  "hex");
  dsp_print_var_value("offset",  offset,  "hex");
  dsp_print_var_value("value",  val,  "hex");
#endif
  return 0;
}

u32 dsp_get_vout_reg(int vout_id, u32 offset)
{
  struct ucode_base_addr_a7_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr = VOUT_BASE;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
  addr += offset;
#else
  if (vout_id == vout_a) {
    offset = offset - VOUT_DA_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
  } else {
    offset = offset - VOUT_DB_CONTROL_OFFSET;
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
  }
#endif
  return readl(addr);
}

int dsp_set_vout_osd_clut(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a7_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();

  if (vout_id == vout_a) {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base + offset;
  } else {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base + offset;
  }

  writel(addr, val);
#if 0
  dsp_print_var_value("Addr", addr, "hex");
  dsp_print_var_value("value", val, "hex");
#endif
  return 0;
}

int dsp_set_vout_dve(int vout_id, u32 offset, u32 val)
{
  struct ucode_base_addr_a7_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  u32 addr;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();

  if (vout_id == vout_a) {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
  } else {
    addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
  }

  writel(addr, val);
#if 0
  dsp_print_var_value("Addr", addr, "hex");
  dsp_print_var_value("value", val, "hex");
#endif
  return 0;
}

#if (A7_SPLASH_DBG == 1)
static int dsp_print_var_value(const char *var_str, u32 value, char *format)
{
  if (strcmp(format, "hex") == 0) {
    putstr("0x");
    puthex((u32)value);
  } else if (strcmp(format, "dec") == 0) {
    putdec((u32)value);
  } else {
    putstr("Warning : Unknown data format, should be ""hex"" or ""dec""! ");
    return -1;
  }
  putstr(" : ");
  putstr(var_str);
  putstr("\r\n");

  return 0;
}

void dsp_memory_allocation_print(struct ucode_base_addr_a7_s *ucode)
{
  struct dsp_vout_reg_a7_s  *vout;

  vout = &ucode->dsp_vout_reg;

  dsp_print_var_value("ucode->dsp_start",
    (u32)ucode->dsp_start, "hex");

  dsp_print_var_value("ucode->dsp_end",
    (u32)ucode->dsp_end, "hex");

  dsp_print_var_value("ucode->code_addr",
    (u32)ucode->code_addr, "hex");

  dsp_print_var_value("ucode->sub0_addr",
    (u32)ucode->sub0_addr, "hex");

  dsp_print_var_value("ucode->defbin_addr",
    (u32)ucode->defbin_addr, "hex");

  dsp_print_var_value("ucode->shadow_reg_addr",
    (u32)ucode->shadow_reg_addr, "hex");

  dsp_print_var_value("ucode->defimg_y_addr",
    (u32)ucode->defimg_y_addr, "hex");

  dsp_print_var_value("ucode->defimg_uv_addr",
    (u32)ucode->defimg_uv_addr, "hex");

  dsp_print_var_value("ucode->dsp_buffer_addr",
    (u32)ucode->dsp_buffer_addr, "hex");

  dsp_print_var_value("ucode->dsp_buffer_size",
    (u32)ucode->dsp_buffer_size, "hex");

  dsp_print_var_value("dsp_init_data.cmd_data_gen_ptr",
    (u32)ucode->dsp_init_data.cmd_data_gen_daddr, "hex");

  dsp_print_var_value("dsp_init_data.msg_queue_gen_daddr",
    (u32)ucode->dsp_init_data.msg_queue_gen_daddr, "hex");

  dsp_print_var_value("dsp_init_data.default_config_daddr",
    (u32)ucode->dsp_init_data.default_config_daddr, "hex");

  dsp_print_var_value("dsp_init_data.cmd_data_vcap_daddr",
    (u32)ucode->dsp_init_data.cmd_data_vcap_daddr, "hex");

  dsp_print_var_value("dsp_init_data.msg_queue_vcap_daddr",
    (u32)ucode->dsp_init_data.msg_queue_vcap_daddr, "hex");

  dsp_print_var_value("dsp_init_data.cmd_data_3rd_daddr",
    (u32)ucode->dsp_init_data.cmd_data_3rd_daddr, "hex");

  dsp_print_var_value("dsp_init_data.msg_queue_3rd_daddr",
    (u32)ucode->dsp_init_data.msg_queue_3rd_daddr, "hex");

  dsp_print_var_value("dsp_init_data.cmd_data_4th_daddr",
    (u32)ucode->dsp_init_data.cmd_data_4th_daddr, "hex");

  dsp_print_var_value("dsp_init_data.msg_queue_4th_daddr",
    (u32)ucode->dsp_init_data.msg_queue_4th_daddr, "hex");

  dsp_print_var_value("dsp_init_data.dsp_buffer_daddr",
    (u32)ucode->dsp_init_data.dsp_buffer_daddr, "hex");

  dsp_print_var_value("vout->vo_ctl_a_base",
    (u32)vout->vo_ctl_a_base, "hex");

  dsp_print_var_value("vout->vo_ctl_b_base",
    (u32)vout->vo_ctl_b_base, "hex");

  dsp_print_var_value("vout->vo_ctl_dve_base",
    (u32)vout->vo_ctl_dve_base, "hex");

  dsp_print_var_value("vout->vo_ctl_osd_clut_a_base",
    (u32)vout->vo_ctl_osd_clut_a_base, "hex");

  dsp_print_var_value("vout->vo_ctl_osd_clut_b_base",
    (u32)vout->vo_ctl_osd_clut_b_base, "hex");

  dsp_print_var_value("ucode->osd_a_buf_ptr",
    (u32)ucode->osd_a_buf_ptr, "hex");

  dsp_print_var_value("ucode->osd_b_buf_ptr",
    (u32)ucode->osd_b_buf_ptr, "hex");
}

static void dsp_get_ucode_version(void)
{
  u32 addr, tmp;
  struct ucode_base_addr_a7_s* ucode;
  struct dsp_ucode_ver_s version;

  ucode = dsp_get_dsp_ptr();
  addr = ucode->code_addr + 92;
  tmp = *((u32 *) addr);

  version.year  = (tmp & 0xFFFF0000) >> 16;
  version.month = (tmp & 0x0000FF00) >> 8;
  version.day   = (tmp & 0x000000FF);

  addr = ucode->code_addr + 88;
  tmp = *(u32 *)addr;

  version.edition_num = (tmp & 0xFFFF0000) >> 16;
  version.edition_ver = (tmp & 0x0000FFFF);

  addr = ucode->code_addr + 96;
  tmp = *(u32 *)addr;

  dsp_print_var_value("year",      version.year,   "dec");
  dsp_print_var_value("month",      version.month,   "dec");
  dsp_print_var_value("day",      version.day,   "dec");
  dsp_print_var_value("edition_num", version.edition_num, "dec");
  dsp_print_var_value("edition_ver", version.edition_ver, "dec");
}
#endif

int osd_init(u16 resolution, osd_t *osdobj)
{
  int rval = 0;

  osdobj->resolution = resolution;

  switch (resolution) {
  case VO_RGB_800_480:
    osdobj->width  = 800;
    osdobj->height = 480;
    osdobj->pitch  = 800;
    break;
  case VO_RGB_480_800:
    osdobj->width  = 480;
    osdobj->height = 800;
    osdobj->pitch  = 480;
    break;
  case VO_RGB_360_240:
    osdobj->width  = 360;
    osdobj->height = 240;
    osdobj->pitch  = 384;
    break;
  case VO_RGB_360_288:
    osdobj->width  = 360;
    osdobj->height = 288;
    osdobj->pitch  = 384;
    break;
  case VO_RGB_320_240:
    osdobj->width  = 320;
    osdobj->height = 240;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_320_288:
    osdobj->width  = 320;
    osdobj->height = 288;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_320_480:
    osdobj->width  = 320;
    osdobj->height = 480;
    osdobj->pitch  = 320;
    break;
  case VO_RGB_480P:
    osdobj->width  = 720;
    osdobj->height = 480;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_576P:
    osdobj->width  = 720;
    osdobj->height = 576;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_480I:
    osdobj->width  = 720;
    osdobj->height = 480;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_576I:
    osdobj->width  = 720;
    osdobj->height = 576;
    osdobj->pitch  = 736;
    break;
  case VO_RGB_480P:
    osdobj->width  = 240;
    osdobj->height = 432;
    osdobj->pitch  = 256;
    break;
  default :
    osdobj->width  = 360;
    osdobj->height = 240;
    osdobj->pitch  = 0;
    rval = -1;
    break;
  }

  return rval;
}

/**
 * Copy BMP to OSD memory
 */
static int bmp2osd_mem(bmp_t *bmp, osd_t *osdobj, int x, int y, int top_botm_revs)
{
  int point_idx = 0;
  int i = 0, j = 0;

  if ((osdobj->buf == NULL) ||
    (osdobj->width < 0) || (osdobj->height < 0))
    return -1;

  /* set the first line to move */
  if (top_botm_revs == 0)
    point_idx = y * osdobj->pitch + x;
  else
    point_idx = (y + bmp->height) * osdobj->pitch + x;

#if 0
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

  bmp_t* splash_bmp = NULL;
  int x = 0, y = 0;
  int rval = -1;

  if (posd == NULL) {
    putstr("Splash_Err: NULL pointer of OSD buffer!");
    return -1;
  }

  /* Get the spalsh bmp address and osd buffer address */
#if defined(SHOW_AMBOOT_SPLASH)
  splash_bmp = bld_get_splash_bmp();
#endif
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

void dsp_set_splash_bmp2osd_buf(int chan, int res, int bot_top_res)
{
  struct ucode_base_addr_a7_s* ucode;
  int vout_a = VOUT_DISPLAY_A;
  int rval = -1;

  vout_a = dsp_get_vout_a();
  ucode = dsp_get_dsp_ptr();

  osd_init(res, &ucode->osd);

  if (chan == vout_a)
    ucode->osd.buf = (u8 *)ucode->osd_a_buf_ptr;
  else
    ucode->osd.buf = (u8 *)ucode->osd_b_buf_ptr;

  memset(ucode->osd.buf, 0, DSP_OSD_BUF_SIZE);

  /* Copy splash BMP to OSD buffer */
  rval = splash_bmp2osd_buf(chan, &ucode->osd, bot_top_res);
  if (rval != 0) {
    putstr("Splash_Err: Copy splahs to OSD fail!");
  }
}


static void a7_ucode_addr_init(u32 dsp_start, u32 dsp_end,
  struct ucode_base_addr_a7_s *ucode)
{
  /** In normal ucode, the DSP buffer is used fo encode/decode data used,
   *  here in the splash, use the available for VOUT memory mapping
   *  registers, CLUT memory mapping registers, OSD data.
   */
  //#  +-----------------------------------------+ DSP_INIT_DATA_ADDR
  //#  |                                         | 0xC00F0000
  //#  |                                         |
  //#  +-----------------------------------------+ dsp_start
  //#  |                                         | A7_SPLASH_START
  //#  |                     4K                  | default_config_daddr
  //#  |                                         |
  //#  +-----------------------------------------+ res_queue_ptr
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ def_config_ptr
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ vo_ctl_a_base
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ vo_ctl_b_base
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ vo_ctl_dve_base
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ vo_ctl_osd_clut_a_base
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ vo_ctl_osd_clut_b_base
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ osd0_buf_addr
  //#  |                                         |
  //#  |                                         |
  //#  +-----------------------------------------+ osd1_buf_addr
  //#  |                                         |
  //#  +-----------------------------------------+ dsp_end - 0xe000
  //#  |              orccode.bin:52kB           |
  //#  +-----------------------------------------+ dsp_end - 0x1000
  //#  |                orcme.bin:64B            |
  //#  +-----------------------------------------+ dsp_end
  //#                A7_SPLASH_END
  //#
  struct dsp_init_data_a7_s *dsp;
  struct dsp_vout_reg_a7_s  *vout;
  struct dsp_cmd_data_a7_s *gen, *vcap, *cfg;

  memset(ucode, 0x0, sizeof(struct ucode_base_addr_a7_s));

  ucode->dsp_start       = dsp_start; // A7_SPLASH_START
  ucode->dsp_end         = dsp_end;   // A7_SPLASH_END
  ucode->code_addr       = ucode->dsp_end - 0xa00000;
  ucode->sub0_addr       = ucode->code_addr + 2048 * 1024;
  ucode->sub1_addr       = ucode->sub0_addr + 1024 * 1024;
  ucode->defbin_addr     = ucode->sub1_addr + 1024 * 1024;
  ucode->shadow_reg_addr = ucode->defbin_addr + 512 * 1024;
  ucode->defimg_y_addr   = ucode->shadow_reg_addr + 1 * 1024;
  ucode->defimg_uv_addr  = ucode->defimg_y_addr + 1920 * 1080;
  ucode->dsp_buffer_addr = ucode->dsp_start;
  ucode->dsp_buffer_size = ucode->dsp_end - ucode->dsp_start - 0xa00000;
#if 0
  ucode->code_addr  = ucode->dsp_end - 0xe000;
  ucode->sub0_addr  = ucode->dsp_end - 0x1000;
  ucode->sub1_addr  = ucode->dsp_end - 0x500;
  ucode->defbin_addr  = 0;
  ucode->shadow_reg_addr  = 0;
  ucode->defimg_y_addr  = 0;
  ucode->defimg_uv_addr  = 0;
  ucode->dsp_buffer_addr  = IDSP_RAM_START;
  ucode->dsp_buffer_size  = 0x008000;
#endif
  ucode->dsp_init_data.default_binary_data_addr = ucode->defbin_addr;
  ucode->dsp_init_data.default_binary_data_size = (u32)(512 * 1024);

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_gen_daddr,
    ucode->dsp_start, A7_DSP_CMD_QUEUE_SIZE, 32);
  ucode->dsp_init_data.cmd_data_gen_size = A7_DSP_CMD_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_gen_daddr,
    ((u32)ucode->dsp_init_data.cmd_data_gen_daddr +
     A7_DSP_CMD_QUEUE_SIZE),
    A7_DSP_RESULT_QUEUE_SIZE, 32);

  ucode->dsp_init_data.msg_queue_gen_size = A7_DSP_RESULT_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_vcap_daddr,
    ((u32)ucode->dsp_init_data.msg_queue_gen_daddr +
     A7_DSP_RESULT_QUEUE_SIZE),
    A7_DSP_CMD_QUEUE_SIZE, 32);
  ucode->dsp_init_data.cmd_data_vcap_size = A7_DSP_CMD_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_vcap_daddr,
    ((u32)ucode->dsp_init_data.cmd_data_vcap_daddr +
     A7_DSP_CMD_QUEUE_SIZE),
    A7_DSP_RESULT_QUEUE_SIZE, 32);

  ucode->dsp_init_data.msg_queue_vcap_size = A7_DSP_RESULT_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_3rd_daddr,
    ((u32)ucode->dsp_init_data.msg_queue_vcap_daddr +
     A7_DSP_RESULT_QUEUE_SIZE),
    A7_DSP_CMD_QUEUE_SIZE, 32);
  ucode->dsp_init_data.cmd_data_3rd_size = A7_DSP_CMD_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_3rd_daddr,
    ((u32)ucode->dsp_init_data.cmd_data_3rd_daddr +
     A7_DSP_CMD_QUEUE_SIZE),
    A7_DSP_RESULT_QUEUE_SIZE, 32);

  ucode->dsp_init_data.msg_queue_3rd_size = A7_DSP_RESULT_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_4th_daddr,
    ((u32)ucode->dsp_init_data.msg_queue_3rd_daddr +
     A7_DSP_RESULT_QUEUE_SIZE),
    A7_DSP_CMD_QUEUE_SIZE, 32);
  ucode->dsp_init_data.cmd_data_4th_size = A7_DSP_CMD_QUEUE_SIZE;

  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_4th_daddr,
    ((u32)ucode->dsp_init_data.cmd_data_4th_daddr +
     A7_DSP_CMD_QUEUE_SIZE),
    A7_DSP_RESULT_QUEUE_SIZE, 32);

  ucode->dsp_init_data.msg_queue_4th_size = A7_DSP_RESULT_QUEUE_SIZE;

  /* default config dsp commands pointer */
  dsp_get_align_buf((u8 **)&ucode->dsp_init_data.default_config_daddr,
    ((u32)ucode->dsp_init_data.msg_queue_4th_daddr +
     A7_DSP_RESULT_QUEUE_SIZE),
    A7_DSP_DEFCFG_QUEUE_SIZE, 32);
  ucode->dsp_init_data.default_config_size = A7_DSP_DEFCFG_QUEUE_SIZE;

  memset((void *)ucode->dsp_init_data.cmd_data_gen_daddr,
    0x00, A7_DSP_CMD_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.msg_queue_gen_daddr,
    0x00, A7_DSP_RESULT_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.cmd_data_vcap_daddr,
    0x00, A7_DSP_CMD_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.msg_queue_vcap_daddr,
    0x00, A7_DSP_RESULT_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.cmd_data_3rd_daddr,
    0x00, A7_DSP_CMD_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.msg_queue_3rd_daddr,
    0x00, A7_DSP_RESULT_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.cmd_data_4th_daddr,
    0x00, A7_DSP_CMD_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.msg_queue_4th_daddr,
    0x00, A7_DSP_RESULT_QUEUE_SIZE);
  memset((void *)ucode->dsp_init_data.default_config_daddr,
    0x00, A7_DSP_DEFCFG_QUEUE_SIZE);

  ucode->dsp_init_data.dsp_buffer_size = ucode->dsp_buffer_size;
  ucode->dsp_init_data.dsp_buffer_daddr = ucode->dsp_buffer_addr;
  ucode->dsp_cmd_max_count = A7_CMD_MAX_COUNT;


  gen = (struct dsp_cmd_data_a7_s*)
    (ucode->dsp_init_data.cmd_data_gen_daddr);

  gen->header_cmd.code = CMD_HEADER_CODE;
  gen->header_cmd.ncmd = 0;

  vcap = (struct dsp_cmd_data_a7_s*)
    (ucode->dsp_init_data.cmd_data_vcap_daddr);

  vcap->header_cmd.code = CMD_HEADER_CODE;
  vcap->header_cmd.ncmd = 0;

  cfg = (struct dsp_cmd_data_a7_s*)
    (ucode->dsp_init_data.default_config_daddr);

  cfg->header_cmd.code = CMD_HEADER_CODE;
  cfg->header_cmd.ncmd = 0;

  vout                         = &ucode->dsp_vout_reg;
  vout->vo_ctl_a_size          = DSP_VOUT_A_CTLREG_BUF_SIZE;
  vout->vo_ctl_b_size          = DSP_VOUT_B_CTLREG_BUF_SIZE;
  vout->vo_ctl_dve_size        = DSP_VOUT_DVE_CTLREG_BUF_SIZE;
  vout->vo_ctl_osd_clut_a_size = DSP_VOUT_OSD_CTRREG_BUF_SIZE;
  vout->vo_ctl_osd_clut_b_size = DSP_VOUT_OSD_CTRREG_BUF_SIZE;

  dsp_get_align_buf((u8 **)&vout->vo_ctl_a_base,
    ((u32)ucode->dsp_init_data.default_config_daddr +
     A7_DSP_DEFCFG_QUEUE_SIZE),
    DSP_VOUT_A_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_b_base,
    ((u32)vout->vo_ctl_a_base +
     DSP_VOUT_A_CTLREG_BUF_SIZE),
    DSP_VOUT_B_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_dve_base,
    ((u32)vout->vo_ctl_b_base +
     DSP_VOUT_B_CTLREG_BUF_SIZE),
    DSP_VOUT_DVE_CTLREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_a_base,
    ((u32)vout->vo_ctl_dve_base +
     DSP_VOUT_DVE_CTLREG_BUF_SIZE),
    DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);
  dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_b_base,
    ((u32)vout->vo_ctl_osd_clut_a_base +
     DSP_VOUT_OSD_CTRREG_BUF_SIZE),
    DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);

  dsp_get_align_buf((u8 **)&ucode->osd_a_buf_ptr,
    ((u32)vout->vo_ctl_osd_clut_b_base +
     DSP_VOUT_OSD_CTRREG_BUF_SIZE),
    DSP_OSD_BUF_SIZE, 32);

  dsp_get_align_buf((u8 **)&ucode->osd_b_buf_ptr,
    ((u32)ucode->osd_a_buf_ptr +
     DSP_OSD_BUF_SIZE),
    DSP_OSD_BUF_SIZE, 32);

  memset(vout->vo_ctl_a_base, 0x00, DSP_VOUT_A_CTLREG_BUF_SIZE);
  memset(vout->vo_ctl_b_base, 0x00, DSP_VOUT_B_CTLREG_BUF_SIZE);

  /* Put the dsp_init_data at this address */
  dsp = (struct dsp_init_data_a7_s *)DSP_INIT_DATA_ADDR;
  memset((u8 *)dsp, 0, sizeof(struct dsp_init_data_a7_s));
  memcpy((u8 *)dsp, &ucode->dsp_init_data, sizeof(struct dsp_init_data_a7_s));
  clean_d_cache((u8 *)DSP_INIT_DATA_ADDR, sizeof(struct dsp_init_data_a7_s));

#if (A7_SPLASH_DBG == 1)
  dsp_memory_allocation_print(ucode);
#endif
}

#if 0
u8 *dsp_get_osd_ptr(u16 vout_id)
{
  int vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a7_s* ucode;

  ucode  = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();

  if (vout_id == vout_a)
    return (u8 *)(ucode->osd_a_buf_ptr);
  else
    return (u8 *)(ucode->osd_b_buf_ptr);
}
#endif

static int dsp_write_to_command_queue(struct ucode_base_addr_a7_s *ucode,
  u8* cmd, u32 align_cmd_len)
{
  struct dsp_cmd_data_a7_s *cmd_queue_ptr;
  u32 cmd_max_count = 0;
  int i = 0;

  cmd_queue_ptr   = (struct dsp_cmd_data_a7_s *)
    ((ucode->dsp_init_data.default_config_daddr | DRAM_START_ADDR));
  cmd_max_count   = ucode->dsp_cmd_max_count;

  i = cmd_queue_ptr->header_cmd.ncmd;
  if (cmd_queue_ptr->header_cmd.ncmd < cmd_max_count) {
    /* clean buffer to 0 */
    memset((cmd_queue_ptr->cmd + i), 0, sizeof(struct dsp_cmd_s));
    memcpy((cmd_queue_ptr->cmd + i), cmd, align_cmd_len);

    cmd_queue_ptr->header_cmd.ncmd = cmd_queue_ptr->header_cmd.ncmd + 1;
  } else {
    return -1;
  }

  return 0;
}

int dsp_get_win_params(int resolution, u16 *width, u16 *height)
{
  int rval = 0;

  switch (resolution) {
  case VO_RGB_800_480:
    *width  = 800;
    *height  = 480;
    break;
  case VO_RGB_480_800:
    *width  = 480;
    *height = 800;
    break;
  case VO_RGB_360_240:
    *width  = 360;
    *height  = 240;
    break;
  case VO_RGB_360_288:
    *width  = 360;
    *height  = 288;
    break;
  case VO_RGB_320_240:
    *width  = 320;
    *height  = 240;
    break;
  case VO_RGB_320_288:
    *width  = 320;
    *height  = 288;
    break;
  case VO_RGB_320_480:
    *width  = 320;
    *height = 480;
    break;
  case VO_RGB_480P:
    *width  = 720;
    *height = 480;
    break;
  case VO_RGB_576P:
    *width  = 720;
    *height = 576;
    break;
  case VO_RGB_480I:
    *width  = 720;
    *height = 480 / 2;
    break;
  case VO_RGB_576I:
    *width  = 720;
    *height = 576 / 2;
    break;
  case VO_RGB_240_432:
    *width  = 240;
    *height = 432;
    break;
  default :
    *width  = 0;
    *height = 0;
    rval    = -1;
    break;
  }

  return rval;
}

int dsp_vout_mixer_setup(u16 vout_id, int resolution)
{
  struct ucode_base_addr_a7_s* ucode;
  vout_mixer_setup_t  dsp_cmd;
  u8  interlaced     = 0;
  u8  frame_rate     = 0;
  u16 active_win_width   = 0;
  u16 active_win_height   = 0;
  u8  back_ground_v   = DEFAULT_BACK_GROUND_V;
  u8  back_ground_u  = DEFAULT_BACK_GROUND_U;
  u8  back_ground_y   = DEFAULT_BACK_GROUND_Y;
  u8  mixer_444     = 0;
  u8  highlight_v    = DEFAULT_HIGHLIGHT_V;
  u8  highlight_u    = DEFAULT_HIGHLIGHT_U;
  u8  highlight_y    = DEFAULT_HIGHLIGHT_Y;
  u8  highlight_thresh  = DEFAULT_HIGHLIGHT_THRESH;
  u8  reverse_en    = 0;
  u8  csc_en    = 0;
  u8  reserved    = 0;
  //  u32 csc_parms[9];

  int rval = -1;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &active_win_width, &active_win_height);

  if (rval != 0)
    return -1;

  switch (resolution) {
  case VO_RGB_800_480:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480_800:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_360_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_240:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_288:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_320_480:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_360_288:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480P:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_576P:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  case VO_RGB_480I:
    interlaced    = 1;
    frame_rate    = 1;
    break;
  case VO_RGB_576I:
    interlaced    = 1;
    frame_rate    = 1;
    break;
  case VO_RGB_240_432:
    interlaced    = 0;
    frame_rate    = 1;
    break;
  default:
    break;
  }

  dsp_cmd.cmd_code  = VOUT_MIXER_SETUP;
  dsp_cmd.vout_id    = vout_id;
  dsp_cmd.interlaced   = interlaced;
  dsp_cmd.frm_rate   = frame_rate;
  dsp_cmd.act_win_width   = active_win_width;
  dsp_cmd.act_win_height   = active_win_height;
  dsp_cmd.back_ground_v   = back_ground_v;
  dsp_cmd.back_ground_u   = back_ground_u;
  dsp_cmd.back_ground_y   = back_ground_y;
  dsp_cmd.mixer_444  = mixer_444;
  dsp_cmd.highlight_v   = highlight_v;
  dsp_cmd.highlight_u   = highlight_u;
  dsp_cmd.highlight_y   = highlight_y;
  dsp_cmd.highlight_thresh = highlight_thresh;
  dsp_cmd.reverse_en  = reverse_en;
  dsp_cmd.csc_en    = csc_en;
  dsp_cmd.reserved[0]  = reserved;
  dsp_cmd.reserved[1]  = reserved;
  //  dsp_cmd.csc_parms[9]; /* */

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_video_setup(u16 vout_id, int video_en, int video_src, int resolution)
{
  struct ucode_base_addr_a7_s* ucode;
  vout_video_setup_t  dsp_cmd;
  u8  flip = 0;
  u8  rotate = 0;
  u16 reserved = 0;
  u16 win_offset_x = VIDEO_WIN_OFFSET_X;
  u16 win_offset_y = VIDEO_WIN_OFFSET_Y;
  u16 win_width  = 0;
  u16 win_height = 0;
  u16 default_img_pitch = 0;
  u8  default_img_repeat_field = 0;
  u8  reserved2 = 0;
  int rval = -1;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &win_width, &win_height);
  if (rval != 0)
    return -1;

  default_img_pitch   = (((int)(win_width + 31) / 32) * 32);

  dsp_cmd.cmd_code   = VOUT_VIDEO_SETUP;
  dsp_cmd.vout_id   = vout_id;
  dsp_cmd.en     = video_en;
  dsp_cmd.src     = video_src;
  dsp_cmd.flip     = flip;
  dsp_cmd.rotate     = rotate;
  dsp_cmd.reserved  = reserved;
  dsp_cmd.win_offset_x   = win_offset_x;
  dsp_cmd.win_offset_y   = win_offset_y;
  dsp_cmd.win_width   = win_width;
  dsp_cmd.win_height   = win_height;

  dsp_cmd.default_img_y_addr    = ucode->defimg_y_addr;
  dsp_cmd.default_img_uv_addr    = ucode->defimg_uv_addr;
  dsp_cmd.default_img_pitch    = default_img_pitch;
  dsp_cmd.default_img_repeat_field = default_img_repeat_field;
  dsp_cmd.reserved2     = reserved2;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_setup(u16 vout_id, u8 flip, u8 osd_en, u8 osd_src, int resolution)
{
  struct ucode_base_addr_a7_s* ucode;
  vout_osd_setup_t  dsp_cmd;
  u16 vout_a = VOUT_DISPLAY_A;
  u8  rescaler_en = 0;
  u8  premultiplied = 0;
  u8  global_blend = 0xff;
  u16  win_offset_x = 0;
  u16  win_offset_y = 0;
  u16  win_width = 0;
  u16  win_height= 0;
  u16  rescaler_input_width = 0;
  u16  rescaler_input_height = 0;
  u16  osd_buf_pitch = 0;
  u8  osd_buf_repeat = 0;
  u8  osd_direct_mode = 0;
  u16  osd_transparent_color = 0;
  u8  osd_transparent_color_en = 0;
  u8  osd_swap_bytes = 0;
  int  rval = -1;

  ucode = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));

  rval = dsp_get_win_params(resolution, &win_width, &win_height);
  if (rval != 0)
    return -1;

  osd_buf_pitch   = (((int)(win_width + 31) / 32) * 32);

  rescaler_input_width  = win_width;
  rescaler_input_height  = win_height;

  dsp_cmd.cmd_code = VOUT_OSD_SETUP;
  dsp_cmd.vout_id = vout_id;
  dsp_cmd.en = osd_en;
  dsp_cmd.src = osd_src;
  dsp_cmd.flip = flip;
  dsp_cmd.rescaler_en = rescaler_en;
  dsp_cmd.premultiplied = premultiplied;
  dsp_cmd.global_blend = global_blend;
  dsp_cmd.win_offset_x = win_offset_x;
  dsp_cmd.win_offset_y = win_offset_y;
  dsp_cmd.win_width = win_width;
  dsp_cmd.win_height = win_height;
  dsp_cmd.rescaler_input_width = rescaler_input_width;
  dsp_cmd.rescaler_input_height = rescaler_input_height;
  if (vout_id == vout_a)
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
  else
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

  dsp_cmd.osd_buf_pitch = osd_buf_pitch;
  dsp_cmd.osd_buf_repeat_field = osd_buf_repeat;
  dsp_cmd.osd_direct_mode = osd_direct_mode;
  dsp_cmd.osd_transparent_color = osd_transparent_color;
  dsp_cmd.osd_transparent_color_en = osd_transparent_color_en;
  dsp_cmd.osd_swap_bytes = osd_swap_bytes;
  dsp_cmd.osd_buf_info_dram_addr = 0x0;  //For A7M, not used in A7

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_buf_setup(u16 vout_id, int resolution)
{
  struct ucode_base_addr_a7_s* ucode;
  vout_osd_buf_setup_t  dsp_cmd;
  u16   vout_a = VOUT_DISPLAY_A;
  u16  osd_buf_pitch = 0;
  u8  osd_buf_repeat = 0;

  ucode  = dsp_get_dsp_ptr();
  vout_a = dsp_get_vout_a();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_OSD_BUFFER_SETUP;
  dsp_cmd.vout_id = vout_id;

  if (vout_id == vout_a)
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
  else
    dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

  switch (resolution) {
  case VO_RGB_800_480:
    osd_buf_pitch  = 800;
    break;
  case VO_RGB_480_800:
    osd_buf_pitch  = 480;
    break;
  case VO_RGB_360_240:
    osd_buf_pitch  = 384;
    break;
  case VO_RGB_360_288:
    osd_buf_pitch  = 384;
    break;
  case VO_RGB_320_240:
    osd_buf_pitch  = 320;
    break;
  case VO_RGB_320_288:
    osd_buf_pitch  = 320;
    break;
  case VO_RGB_320_480:
    osd_buf_pitch  = 320;
    break;
  case VO_RGB_480P:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_576P:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_480I:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_576I:
    osd_buf_pitch  = 736;
    break;
  case VO_RGB_240_432:
    osd_buf_pitch  = 256;
    break;
  default :
    break;
  }

  dsp_cmd.osd_buf_pitch     = osd_buf_pitch;
  dsp_cmd.osd_buf_repeat_field   = osd_buf_repeat;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_osd_clut_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a7_s* ucode;
  vout_osd_clut_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code  = VOUT_OSD_CLUT_SETUP;
  dsp_cmd.vout_id    = vout_id;

  if (vout_id == vout_a)
    dsp_cmd.clut_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base;
  else
    dsp_cmd.clut_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_display_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a7_s* ucode;
  vout_display_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_DISPLAY_SETUP;
  dsp_cmd.vout_id  = vout_id;
  if (vout_id == vout_a)
    dsp_cmd.disp_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_a_base;
  else
    dsp_cmd.disp_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_b_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

nt dsp_vout_dve_setup(u16 vout_id)
{
  u16 vout_a = VOUT_DISPLAY_A;
  struct ucode_base_addr_a7_s* ucode;
  vout_dve_setup_t  dsp_cmd;

  vout_a = dsp_get_vout_a();
  ucode  = dsp_get_dsp_ptr();

  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code = VOUT_DVE_SETUP;
  dsp_cmd.vout_id  = vout_id;
  if (vout_id == vout_a)
    dsp_cmd.dve_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_dve_base;
  else
    dsp_cmd.dve_config_dram_addr =
      (u32)ucode->dsp_vout_reg.vo_ctl_dve_base;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

int dsp_vout_reset(u16 vout_id, u8 reset_mixer, u8  reset_disp)
{
  struct ucode_base_addr_a7_s* ucode;
  vout_reset_t  dsp_cmd;

  ucode = dsp_get_dsp_ptr();
  memset(&dsp_cmd, 0, sizeof(dsp_cmd));
  dsp_cmd.cmd_code   = VOUT_RESET;
  dsp_cmd.vout_id   = vout_id;
  dsp_cmd.reset_mixer   = reset_mixer;
  dsp_cmd.reset_disp   = reset_disp;

  dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

  return 0;
}

static int a7_dsp_boot(struct ucode_base_addr_a7_s *ucode)
{
  /* Set up the base address */
  writel(DSP_DRAM_MAIN_REG, ucode->code_addr);
  writel(DSP_DRAM_SUB0_REG, ucode->sub0_addr);
  writel(DSP_DRAM_SUB1_REG, ucode->sub1_addr);

  /* Enable the engines */
  writel(DSP_CONFIG_MAIN_REG, 0xff);
  writel(DSP_CONFIG_SUB0_REG, 0xf);
  writel(DSP_CONFIG_SUB1_REG, 0xf);

  return 0;
}

int dsp_dram_clean_cache(void)
{
  struct ucode_base_addr_a7_s* ucode;

  ucode = dsp_get_dsp_ptr();
  if (ucode == NULL)
    return -1;

  /* clean d cache in these area because
     all bld memory all set to cachable!!! */
  clean_d_cache((void *)(ucode->dsp_init_data.default_config_daddr),
    ucode->dsp_init_data.default_config_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_a_base,
    ucode->dsp_vout_reg.vo_ctl_a_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_b_base,
    ucode->dsp_vout_reg.vo_ctl_b_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base,
    ucode->dsp_vout_reg.vo_ctl_osd_clut_a_size);

  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base,
    ucode->dsp_vout_reg.vo_ctl_osd_clut_b_size);
  clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_dve_base,
    ucode->dsp_vout_reg.vo_ctl_dve_size);
  return 0;
}
#endif

#if (CHIP_REV == I1)

static struct ucode_base_addr_i1_s* dsp_get_dsp_ptr(void)
{
	return &G_i1_ucode;
}

/**
 * Get aligned hunge buffer.
 *
 * @param buf     - pointer to the aligned buffer pointer
 * @param raw_buf - the original buf address
 * @param size    - size of aligned buffer
 * @param align   - align byte
 * @returns       - 0 if successful, < 0 if failed
 */
static int dsp_get_align_buf(u8 **buf, u32 raw_buf, u32 size, u32 align)
{
	u32 misalign;

	size += align << 1;

	misalign = (u32)(raw_buf) & (align - 1);

	*buf = (u8 *)((u32)(raw_buf) + (align - misalign));

	return 0;
}

int dsp_set_vout_reg(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_i1_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr = VOUT_BASE;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
	addr += offset;
#else
	if (vout_id == vout_a) {
		offset = offset - VOUT_DA_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
	} else {
		offset = offset - VOUT_DB_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
	}
#endif
	writel(addr, val);

#if 0
	dsp_delay(1);
	dsp_print_var_value("addr",	addr,	"hex");
	dsp_print_var_value("offset",	offset,	"hex");
	dsp_print_var_value("value",	val,	"hex");
#endif
	return 0;
}

u32 dsp_get_vout_reg(int vout_id, u32 offset)
{
	struct ucode_base_addr_i1_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr = VOUT_BASE;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
	addr += offset;
#else
	if (vout_id == vout_a) {
		offset = offset - VOUT_DA_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
	} else {
		offset = offset - VOUT_DB_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
	}
#endif
	return readl(addr);
}

int dsp_set_vout_osd_clut(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_i1_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	if (vout_id == vout_a) {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base + offset;
	} else {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base + offset;
	}

	writel(addr, val);
#if 0
	dsp_print_var_value("Addr", addr, "hex");
	dsp_print_var_value("value", val, "hex");
#endif
	return 0;
}

int dsp_set_vout_dve(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_i1_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	if (vout_id == vout_a) {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
	} else {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
	}

	writel(addr, val);
#if 0
	dsp_print_var_value("Addr", addr, "hex");
	dsp_print_var_value("value", val, "hex");
#endif
	return 0;
}

#if (I1_SPLASH_DBG == 1)
static int dsp_print_var_value(const char *var_str, u32 value, char *format)
{
	if (strcmp(format, "hex") == 0) {
		putstr("0x");
		puthex((u32)value);
	} else if (strcmp(format, "dec") == 0) {
		putdec((u32)value);
	} else {
		putstr("Warning : Unknown data format, should be ""hex"" or ""dec""! ");
		return -1;
	}
	putstr(" : ");
	putstr(var_str);
	putstr("\r\n");

	return 0;
}

void dsp_memory_allocation_print(struct ucode_base_addr_i1_s *ucode)
{
	struct dsp_vout_reg_i1_s  *vout;

	vout = &ucode->dsp_vout_reg;

	dsp_print_var_value("ucode->dsp_start",
			    (u32)ucode->dsp_start, "hex");

	dsp_print_var_value("ucode->dsp_end",
			    (u32)ucode->dsp_end, "hex");

	dsp_print_var_value("ucode->code_addr",
			    (u32)ucode->code_addr, "hex");

	dsp_print_var_value("ucode->sub0_addr",
			    (u32)ucode->sub0_addr, "hex");

  	dsp_print_var_value("ucode->sub1_addr",
    			    (u32)ucode->sub1_addr, "hex");

	dsp_print_var_value("ucode->defbin_addr",
			    (u32)ucode->defbin_addr, "hex");

	dsp_print_var_value("ucode->shadow_reg_addr",
			    (u32)ucode->shadow_reg_addr, "hex");

	dsp_print_var_value("ucode->defimg_y_addr",
			    (u32)ucode->defimg_y_addr, "hex");

	dsp_print_var_value("ucode->defimg_uv_addr",
			    (u32)ucode->defimg_uv_addr, "hex");

	dsp_print_var_value("ucode->dsp_buffer_addr",
			    (u32)ucode->dsp_buffer_addr, "hex");

	dsp_print_var_value("ucode->dsp_buffer_size",
			    (u32)ucode->dsp_buffer_size, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_gen_ptr",
			    (u32)ucode->dsp_init_data.cmd_data_gen_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_gen_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_gen_daddr, "hex");

	dsp_print_var_value("dsp_init_data.default_config_daddr",
			    (u32)ucode->dsp_init_data.default_config_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_vcap_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_vcap_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_vcap_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_vcap_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_3rd_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_3rd_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_3rd_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_3rd_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_4th_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_4th_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_4th_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_4th_daddr, "hex");

	dsp_print_var_value("dsp_init_data.dsp_buffer_daddr",
			    (u32)ucode->dsp_init_data.dsp_buffer_daddr, "hex");

	dsp_print_var_value("vout->vo_ctl_a_base",
			    (u32)vout->vo_ctl_a_base, "hex");

	dsp_print_var_value("vout->vo_ctl_b_base",
			    (u32)vout->vo_ctl_b_base, "hex");

	dsp_print_var_value("vout->vo_ctl_dve_base",
			    (u32)vout->vo_ctl_dve_base, "hex");

	dsp_print_var_value("vout->vo_ctl_osd_clut_a_base",
			    (u32)vout->vo_ctl_osd_clut_a_base, "hex");

	dsp_print_var_value("vout->vo_ctl_osd_clut_b_base",
			    (u32)vout->vo_ctl_osd_clut_b_base, "hex");

	dsp_print_var_value("ucode->osd_a_buf_ptr",
			    (u32)ucode->osd_a_buf_ptr, "hex");

	dsp_print_var_value("ucode->osd_b_buf_ptr",
			    (u32)ucode->osd_b_buf_ptr, "hex");
}

static void dsp_get_ucode_version(void)
{
	u32 addr, tmp;
	struct ucode_base_addr_i1_s* ucode;
	struct dsp_ucode_ver_s version;

	ucode = dsp_get_dsp_ptr();
	addr = ucode->code_addr + 92;
	tmp = *((u32 *) addr);

	version.year  = (tmp & 0xFFFF0000) >> 16;
	version.month = (tmp & 0x0000FF00) >> 8;
	version.day   = (tmp & 0x000000FF);

	addr = ucode->code_addr + 88;
	tmp = *(u32 *)addr;

	version.edition_num = (tmp & 0xFFFF0000) >> 16;
	version.edition_ver = (tmp & 0x0000FFFF);

	addr = ucode->code_addr + 96;
	tmp = *(u32 *)addr;

	dsp_print_var_value("year", 	   version.year, 	"dec");
	dsp_print_var_value("month", 	   version.month, 	"dec");
	dsp_print_var_value("day", 	   version.day, 	"dec");
	dsp_print_var_value("edition_num", version.edition_num, "dec");
	dsp_print_var_value("edition_ver", version.edition_ver, "dec");
}
#endif

int osd_init(u16 resolution, osd_t *osdobj)
{
	int rval = 0;

	osdobj->resolution = resolution;

	switch (resolution) {
		case VO_RGB_800_480:
			osdobj->width  = 800;
			osdobj->height = 480;
			osdobj->pitch  = 800;
			break;

		case VO_RGB_480_800:
			osdobj->width  = 480;
			osdobj->height = 800;
			osdobj->pitch  = 480;
			break;

		case VO_RGB_1024_768:
			osdobj->width  = 1024;
			osdobj->height = 768;
			osdobj->pitch  = 1024;
			break;

		case VO_RGB_480P:
			osdobj->width  = 720;
			osdobj->height = 480;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_576P:
			osdobj->width  = 720;
			osdobj->height = 576;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_480I:
			osdobj->width  = 720;
			osdobj->height = 480;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_576I:
			osdobj->width  = 720;
			osdobj->height = 576;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_240_432:
			osdobj->width  = 240;
			osdobj->height = 432;
			osdobj->pitch  = 256;
			break;

		case VO_RGB_360_240:
			osdobj->width  = 360;
			osdobj->height = 240;
			osdobj->pitch  = 384;
			break;

		default :
			osdobj->width  = 800;
			osdobj->height = 480;
			osdobj->pitch  = 800;
			rval = -1;
			break;
	}

	return rval;
}

/**
 * Copy BMP to OSD memory
 */
static int bmp2osd_mem(bmp_t *bmp, osd_t *osdobj, int x, int y, int top_botm_revs)
{
	int point_idx = 0;
	int i = 0, j = 0;

	if ((osdobj->buf == NULL) ||
	    (osdobj->width < 0) || (osdobj->height < 0))
		return -1;

	/* set the first line to move */
	if (top_botm_revs == 0)
		point_idx = y * osdobj->pitch + x;
	else
		point_idx = (y + bmp->height) * osdobj->pitch + x;

#if 0
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

	bmp_t* splash_bmp = NULL;
	int x = 0, y = 0;
	int rval = -1;

	if (posd == NULL) {
		putstr("Splash_Err: NULL pointer of OSD buffer!");
		return -1;
	}

	/* Get the spalsh bmp address and osd buffer address */
#if defined(SHOW_AMBOOT_SPLASH)
	splash_bmp = bld_get_splash_bmp();
#endif
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

void dsp_set_splash_bmp2osd_buf(int chan, int res, int bot_top_res)
{
	struct ucode_base_addr_i1_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	int rval = -1;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	osd_init(res, &ucode->osd);

	if (chan == vout_a)
		ucode->osd.buf = (u8 *)ucode->osd_a_buf_ptr;
	else
		ucode->osd.buf = (u8 *)ucode->osd_b_buf_ptr;

	memset(ucode->osd.buf, 0, DSP_OSD_BUF_SIZE);

	/* Copy splash BMP to OSD buffer */
	rval = splash_bmp2osd_buf(chan, &ucode->osd, bot_top_res);
	if (rval != 0) {
		putstr("Splash_Err: Copy splahs to OSD fail!");
	}
}


static void i1_ucode_addr_init(u32 dsp_start, u32 dsp_end,
				struct ucode_base_addr_i1_s *ucode)
{
	/** In normal ucode, the DSP buffer is used fo encode/decode data used,
	 *  here in the splash, use the available for VOUT memory mapping
	 *  registers, CLUT memory mapping registers, OSD data.
	 */
	//#	+-----------------------------------------+ DSP_INIT_DATA_ADDR
	//#	|					  | 0x000F0000
	//#	|					  |
	//#	+-----------------------------------------+ dsp_start
	//#	|					  | I1_SPLASH_START
	//#	|		4K			  | default_config_daddr
	//#	|					  |
	//#	+-----------------------------------------+ res_queue_ptr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ def_config_ptr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_a_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_b_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_dve_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_osd_clut_a_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_osd_clut_b_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ osd0_buf_addr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ osd1_buf_addr
	//#	|					  |
	//#	+-----------------------------------------+ dsp_end - 0xe000
	//#	| 		orccode.bin:52kB	  |
	//#	+-----------------------------------------+ dsp_end - 0x1000
	//#	| 		orcme.bin:64B		  |
	//#	+-----------------------------------------+ dsp_end
	//#						    I1_SPLASH_END
	//#
	struct dsp_init_data_i1_s *dsp;
	struct dsp_vout_reg_i1_s  *vout;
	struct dsp_cmd_data_i1_s *gen, *vcap, *cfg;

	memset(ucode, 0x0, sizeof(struct ucode_base_addr_i1_s));

	ucode->dsp_start	= dsp_start; // I1_SPLASH_START
	ucode->dsp_end		= dsp_end;   // I1_SPLASH_END
	ucode->code_addr	= ucode->dsp_end - 6 * MB;
	ucode->sub0_addr	= ucode->code_addr + 512 * KB;
	ucode->sub1_addr	= ucode->sub0_addr + 512 * KB;
	ucode->defbin_addr	= ucode->sub1_addr + 512 * KB;
	ucode->shadow_reg_addr	= ucode->defbin_addr + 512 * KB;
	ucode->defimg_y_addr	= ucode->shadow_reg_addr + 1 * MB;
	ucode->defimg_uv_addr	= ucode->defimg_y_addr + 2 * MB;
	ucode->dsp_buffer_addr	= ucode->dsp_start;
	ucode->dsp_buffer_size	= ucode->dsp_end - ucode->dsp_start - 6 * MB;
#if 0
	ucode->code_addr	= ucode->dsp_end - 0xe000;
	ucode->sub0_addr	= ucode->dsp_end - 0x1000;
	ucode->sub1_addr	= ucode->dsp_end - 0x500;
	ucode->defbin_addr	= 0;
	ucode->shadow_reg_addr	= 0;
	ucode->defimg_y_addr	= 0;
	ucode->defimg_uv_addr	= 0;
	ucode->dsp_buffer_addr	= IDSP_RAM_START;
	ucode->dsp_buffer_size	= 0x008000;
#endif
	ucode->dsp_init_data.default_binary_data_addr = ucode->defbin_addr;
	ucode->dsp_init_data.default_binary_data_size = (u32)(512 * 1024);

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_gen_daddr,
			  ucode->dsp_start, I1_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_gen_size = I1_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_gen_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_gen_daddr +
			  I1_DSP_CMD_QUEUE_SIZE),
			  I1_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_gen_size = I1_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_vcap_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_gen_daddr +
			  I1_DSP_RESULT_QUEUE_SIZE),
			  I1_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_vcap_size = I1_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_vcap_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_vcap_daddr +
			  I1_DSP_CMD_QUEUE_SIZE),
			  I1_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_vcap_size = I1_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_3rd_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_vcap_daddr +
			  I1_DSP_RESULT_QUEUE_SIZE),
			  I1_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_3rd_size = I1_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_3rd_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_3rd_daddr +
			  I1_DSP_CMD_QUEUE_SIZE),
			  I1_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_3rd_size = I1_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_4th_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_3rd_daddr +
			  I1_DSP_RESULT_QUEUE_SIZE),
			  I1_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_4th_size = I1_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_4th_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_4th_daddr +
			  I1_DSP_CMD_QUEUE_SIZE),
			  I1_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_4th_size = I1_DSP_RESULT_QUEUE_SIZE;

	/* default config dsp commands pointer */
	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.default_config_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_4th_daddr +
			  I1_DSP_RESULT_QUEUE_SIZE),
			  I1_DSP_DEFCFG_QUEUE_SIZE, 32);
	ucode->dsp_init_data.default_config_size = I1_DSP_DEFCFG_QUEUE_SIZE;

	memset((void *)ucode->dsp_init_data.cmd_data_gen_daddr,
	       0x00, I1_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_gen_daddr,
	       0x00, I1_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_vcap_daddr,
	       0x00, I1_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_vcap_daddr,
	       0x00, I1_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_3rd_daddr,
	       0x00, I1_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_3rd_daddr,
	       0x00, I1_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_4th_daddr,
	       0x00, I1_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_4th_daddr,
	       0x00, I1_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.default_config_daddr,
	       0x00, I1_DSP_DEFCFG_QUEUE_SIZE);

	ucode->dsp_init_data.dsp_buffer_size = ucode->dsp_buffer_size;
	ucode->dsp_init_data.dsp_buffer_daddr = ucode->dsp_buffer_addr;
	ucode->dsp_init_data.dsp_buffer_pagetbl_daddr = 0;
	ucode->dsp_init_data.dsp_buffer_pagetbl_size = ucode->dsp_buffer_size >> 10;
	ucode->dsp_init_data.dsp_att_cpn_start_offset = 0;
	ucode->dsp_init_data.dsp_att_cpn_size = 0;
	ucode->dsp_init_data.dsp_smem_base = 0;
	ucode->dsp_init_data.dsp_smem_size = 1024 * KB;
	ucode->dsp_init_data.dsp_debug_daddr = 0x1ff80000;
	ucode->dsp_init_data.dsp_debug_size = 0x00020000;
	ucode->dsp_cmd_max_count = I1_CMD_MAX_COUNT;


	gen = (struct dsp_cmd_data_i1_s*)
	      (ucode->dsp_init_data.cmd_data_gen_daddr);

	gen->header_cmd.code = CMD_HEADER_CODE;
	gen->header_cmd.ncmd = 0;

	vcap = (struct dsp_cmd_data_i1_s*)
	      (ucode->dsp_init_data.cmd_data_vcap_daddr);

	vcap->header_cmd.code = CMD_HEADER_CODE;
	vcap->header_cmd.ncmd = 0;

	cfg = (struct dsp_cmd_data_i1_s*)
	      (ucode->dsp_init_data.default_config_daddr);

	cfg->header_cmd.code = CMD_HEADER_CODE;
	cfg->header_cmd.ncmd = 0;

	vout 				= &ucode->dsp_vout_reg;
	vout->vo_ctl_a_size		= DSP_VOUT_A_CTLREG_BUF_SIZE;
	vout->vo_ctl_b_size		= DSP_VOUT_B_CTLREG_BUF_SIZE;
	vout->vo_ctl_dve_size		= DSP_VOUT_DVE_CTLREG_BUF_SIZE;
	vout->vo_ctl_osd_clut_a_size	= DSP_VOUT_OSD_CTRREG_BUF_SIZE;
	vout->vo_ctl_osd_clut_b_size	= DSP_VOUT_OSD_CTRREG_BUF_SIZE;

	dsp_get_align_buf((u8 **)&vout->vo_ctl_a_base,
			 ((u32)ucode->dsp_init_data.default_config_daddr +
			 I1_DSP_DEFCFG_QUEUE_SIZE),
			 DSP_VOUT_A_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_b_base,
			 ((u32)vout->vo_ctl_a_base +
			 DSP_VOUT_A_CTLREG_BUF_SIZE),
			 DSP_VOUT_B_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_dve_base,
			 ((u32)vout->vo_ctl_b_base +
			 DSP_VOUT_B_CTLREG_BUF_SIZE),
			 DSP_VOUT_DVE_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_a_base,
			 ((u32)vout->vo_ctl_dve_base +
			 DSP_VOUT_DVE_CTLREG_BUF_SIZE),
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_b_base,
			 ((u32)vout->vo_ctl_osd_clut_a_base +
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE),
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);

	dsp_get_align_buf((u8 **)&ucode->osd_a_buf_ptr,
			 ((u32)vout->vo_ctl_osd_clut_b_base +
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE),
			 DSP_OSD_BUF_SIZE, 32);

	dsp_get_align_buf((u8 **)&ucode->osd_b_buf_ptr,
			 ((u32)ucode->osd_a_buf_ptr +
			 DSP_OSD_BUF_SIZE),
			 DSP_OSD_BUF_SIZE, 32);

	memset(vout->vo_ctl_a_base, 0x00, DSP_VOUT_A_CTLREG_BUF_SIZE);
	memset(vout->vo_ctl_b_base, 0x00, DSP_VOUT_B_CTLREG_BUF_SIZE);
	memset(ucode->osd_a_buf_ptr, 0x00, DSP_OSD_BUF_SIZE);
	memset(ucode->osd_b_buf_ptr, 0x00, DSP_OSD_BUF_SIZE);

	/* Put the dsp_init_data at this address */
	dsp = (struct dsp_init_data_i1_s *)DSP_INIT_DATA_ADDR;
	memset((u8 *)dsp, 0, sizeof(struct dsp_init_data_i1_s));
	memcpy((u8 *)dsp, &ucode->dsp_init_data, sizeof(struct dsp_init_data_i1_s));
	clean_d_cache((u8 *)DSP_INIT_DATA_ADDR, sizeof(struct dsp_init_data_i1_s));

#if (I1_SPLASH_DBG == 1)
	dsp_memory_allocation_print(ucode);
#endif
}

#if 0
u8 *dsp_get_osd_ptr(u16 vout_id)
{
	int vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_i1_s* ucode;

	ucode  = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();

	if (vout_id == vout_a)
		return (u8 *)(ucode->osd_a_buf_ptr);
	else
		return (u8 *)(ucode->osd_b_buf_ptr);
}
#endif

static int dsp_write_to_command_queue(struct ucode_base_addr_i1_s *ucode,
				      u8* cmd, u32 align_cmd_len)
{
	struct dsp_cmd_data_i1_s *cmd_queue_ptr;
	u32 cmd_max_count = 0;
	int i = 0;

	cmd_queue_ptr 	= (struct dsp_cmd_data_i1_s *)
			  ((ucode->dsp_init_data.default_config_daddr | DRAM_START_ADDR));
	cmd_max_count 	= ucode->dsp_cmd_max_count;

	i = cmd_queue_ptr->header_cmd.ncmd;
	if (cmd_queue_ptr->header_cmd.ncmd < cmd_max_count) {
		/* clean buffer to 0 */
		memset((cmd_queue_ptr->cmd + i), 0, sizeof(struct dsp_cmd_s));
		memcpy((cmd_queue_ptr->cmd + i), cmd, align_cmd_len);

		cmd_queue_ptr->header_cmd.ncmd = cmd_queue_ptr->header_cmd.ncmd + 1;
	} else {
		return -1;
	}

	return 0;
}

int dsp_get_win_params(int resolution, u16 *width, u16 *height)
{
	int rval = 0;

	switch (resolution) {
		case VO_RGB_1024_768:
			*width	= 1024;
			*height	= 768;
			break;
		case VO_RGB_800_480:
			*width	= 800;
			*height	= 480;
			break;
		case VO_RGB_480_800:
			*width	= 480;
			*height	= 800;
			break;
		case VO_RGB_360_240:
			*width	= 360;
			*height	= 240;
			break;
		case VO_RGB_360_288:
			*width	= 360;
			*height	= 288;
			break;
		case VO_RGB_320_240:
			*width	= 320;
			*height	= 240;
			break;
		case VO_RGB_320_288:
			*width	= 320;
			*height	= 288;
			break;
		case VO_RGB_480P:
			*width	= 720;
			*height	= 480;
			break;
		case VO_RGB_576P:
			*width	= 720;
			*height	= 576;
			break;
		case VO_RGB_480I:
			*width	= 720;
			*height	= 480 / 2;
			break;
		case VO_RGB_576I:
			*width	= 720;
			*height	= 576 / 2;
			break;
		case VO_RGB_240_432:
			*width	= 240;
			*height	= 432;
			break;
		default :
			*width  = 0;
			*height = 0;
			rval    = -1;
			break;
	}

	return rval;
}

int dsp_vout_mixer_setup(u16 vout_id, int resolution)
{
	struct ucode_base_addr_i1_s* ucode;
	vout_mixer_setup_t	dsp_cmd;
	u8  interlaced 		= 0;
	u8  frame_rate 		= 0;
	u16 active_win_width 	= 0;
	u16 active_win_height 	= 0;
	u8  back_ground_v 	= DEFAULT_BACK_GROUND_V;
	u8  back_ground_u	= DEFAULT_BACK_GROUND_U;
	u8  back_ground_y 	= DEFAULT_BACK_GROUND_Y;
	u8  mixer_444 		= 0;
	u8  highlight_v		= DEFAULT_HIGHLIGHT_V;
	u8  highlight_u		= DEFAULT_HIGHLIGHT_U;
	u8  highlight_y		= DEFAULT_HIGHLIGHT_Y;
	u8  highlight_thresh	= DEFAULT_HIGHLIGHT_THRESH;
	u8  reverse_en		= 0;
	u8  csc_en		= 0;
	u8  reserved		= 0;
//	u32 csc_parms[9];

	int rval = -1;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &active_win_width, &active_win_height);

	if (rval != 0)
		return -1;

	switch (resolution) {
	case VO_RGB_1024_768:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_800_480:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480_800:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_360_240:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_320_240:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_320_288:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_360_288:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480P:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_576P:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480I:
		interlaced		= 1;
		frame_rate		= 1;
		break;
	case VO_RGB_576I:
		interlaced		= 1;
		frame_rate		= 1;
		break;
	case VO_RGB_240_432:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	default:
		break;
	}

	dsp_cmd.cmd_code	= VOUT_MIXER_SETUP;
	dsp_cmd.vout_id		= vout_id;
	dsp_cmd.interlaced 	= interlaced;
	dsp_cmd.frm_rate 	= frame_rate;
	dsp_cmd.act_win_width 	= active_win_width;
	dsp_cmd.act_win_height 	= active_win_height;
	dsp_cmd.back_ground_v 	= back_ground_v;
	dsp_cmd.back_ground_u 	= back_ground_u;
	dsp_cmd.back_ground_y 	= back_ground_y;
	dsp_cmd.mixer_444	= mixer_444;
	dsp_cmd.highlight_v 	= highlight_v;
	dsp_cmd.highlight_u 	= highlight_u;
	dsp_cmd.highlight_y 	= highlight_y;
	dsp_cmd.highlight_thresh = highlight_thresh;
	dsp_cmd.reverse_en	= reverse_en;
	dsp_cmd.csc_en		= csc_en;
	dsp_cmd.reserved[0]	= reserved;
	dsp_cmd.reserved[1]	= reserved;
//	dsp_cmd.csc_parms[9]; /* */

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_video_setup(u16 vout_id, int video_en, int video_src, int resolution)
{
	struct ucode_base_addr_i1_s* ucode;
	vout_video_setup_t	dsp_cmd;
	u8  flip = 0;
	u8  rotate = 0;
	u16 reserved = 0;
	u16 win_offset_x = VIDEO_WIN_OFFSET_X;
	u16 win_offset_y = VIDEO_WIN_OFFSET_Y;
	u16 win_width  = 0;
	u16 win_height = 0;
	u16 default_img_pitch = 0;
	u8  default_img_repeat_field = 0;
	u8  reserved2 = 0;
	int rval = -1;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &win_width, &win_height);
	if (rval != 0)
		return -1;

	default_img_pitch 	= (((int)(win_width + 31) / 32) * 32);

	dsp_cmd.cmd_code 	= VOUT_VIDEO_SETUP;
	dsp_cmd.vout_id 	= vout_id;
	dsp_cmd.en 		= video_en;
	dsp_cmd.src 		= video_src;
	dsp_cmd.flip 		= flip;
	dsp_cmd.rotate 		= rotate;
	dsp_cmd.reserved	= reserved;
	dsp_cmd.win_offset_x 	= win_offset_x;
	dsp_cmd.win_offset_y 	= win_offset_y;
	dsp_cmd.win_width 	= win_width;
	dsp_cmd.win_height 	= win_height;

	dsp_cmd.default_img_y_addr 	 = ucode->defimg_y_addr;
	dsp_cmd.default_img_uv_addr 	 = ucode->defimg_uv_addr;
	dsp_cmd.default_img_pitch 	 = default_img_pitch;
	dsp_cmd.default_img_repeat_field = default_img_repeat_field;
	dsp_cmd.reserved2		 = reserved2;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_setup(u16 vout_id, u8 flip, u8 osd_en, u8 osd_src, int resolution)
{
	struct ucode_base_addr_i1_s* ucode;
	vout_osd_setup_t	dsp_cmd;
	u16 vout_a = VOUT_DISPLAY_A;
	u8	rescaler_en = 0;
	u8	premultiplied = 0;
	u8	global_blend = 0xff;
	u16	win_offset_x = 0;
	u16	win_offset_y = 0;
	u16	win_width = 0;
	u16	win_height= 0;
	u16	rescaler_input_width = 0;
	u16	rescaler_input_height = 0;
	u16	osd_buf_pitch = 0;
	u8	osd_buf_repeat = 0;
	u8	osd_direct_mode = 0;
	u16	osd_transparent_color = 0;
	u8	osd_transparent_color_en = 0;
	u8	osd_swap_bytes = 0;
	int	rval = -1;

	ucode = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &win_width, &win_height);
	if (rval != 0)
		return -1;

	osd_buf_pitch 	= (((int)(win_width + 31) / 32) * 32);

	rescaler_input_width	= win_width;
	rescaler_input_height	= win_height;

	dsp_cmd.cmd_code = VOUT_OSD_SETUP;
	dsp_cmd.vout_id = vout_id;
	dsp_cmd.en = osd_en;
	dsp_cmd.src = osd_src;
	dsp_cmd.flip = flip;
	dsp_cmd.rescaler_en = rescaler_en;
	dsp_cmd.premultiplied = premultiplied;
	dsp_cmd.global_blend = global_blend;
	dsp_cmd.win_offset_x = win_offset_x;
	dsp_cmd.win_offset_y = win_offset_y;
	dsp_cmd.win_width = win_width;
	dsp_cmd.win_height = win_height;
	dsp_cmd.rescaler_input_width = rescaler_input_width;
	dsp_cmd.rescaler_input_height = rescaler_input_height;
	if (vout_id == vout_a)
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
	else
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

	dsp_cmd.osd_buf_pitch = osd_buf_pitch;
	dsp_cmd.osd_buf_repeat_field = osd_buf_repeat;
	dsp_cmd.osd_direct_mode = osd_direct_mode;
	dsp_cmd.osd_transparent_color = osd_transparent_color;
	dsp_cmd.osd_transparent_color_en = osd_transparent_color_en;
	dsp_cmd.osd_swap_bytes = osd_swap_bytes;
	dsp_cmd.osd_buf_info_dram_addr = 0x0;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_buf_setup(u16 vout_id, int resolution)
{
	struct ucode_base_addr_i1_s* ucode;
	vout_osd_buf_setup_t	dsp_cmd;
	u16 	vout_a = VOUT_DISPLAY_A;
	u16	osd_buf_pitch = 0;
	u8	osd_buf_repeat = 0;

	ucode  = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_OSD_BUFFER_SETUP;
	dsp_cmd.vout_id = vout_id;

	if (vout_id == vout_a)
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
	else
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

	switch (resolution) {
		case VO_RGB_1024_768:
			osd_buf_pitch	= 1024;
			break;
		case VO_RGB_800_480:
			osd_buf_pitch	= 800;
			break;
		case VO_RGB_480_800:
			osd_buf_pitch	= 480;
			break;
		case VO_RGB_360_240:
			osd_buf_pitch	= 384;
			break;
		case VO_RGB_360_288:
			osd_buf_pitch	= 384;
			break;
		case VO_RGB_320_240:
			osd_buf_pitch	= 320;
			break;
		case VO_RGB_320_288:
			osd_buf_pitch	= 320;
			break;
		case VO_RGB_480P:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_576P:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_480I:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_576I:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_240_432:
			osd_buf_pitch	= 256;
			break;
		default :
			break;
	}

	dsp_cmd.osd_buf_pitch 		= osd_buf_pitch;
	dsp_cmd.osd_buf_repeat_field 	= osd_buf_repeat;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_clut_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_i1_s* ucode;
	vout_osd_clut_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code	= VOUT_OSD_CLUT_SETUP;
	dsp_cmd.vout_id		= vout_id;

	if (vout_id == vout_a)
		dsp_cmd.clut_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base;
	else
		dsp_cmd.clut_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_display_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_i1_s* ucode;
	vout_display_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_DISPLAY_SETUP;
	dsp_cmd.vout_id  = vout_id;
	if (vout_id == vout_a)
		dsp_cmd.disp_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_a_base;
	else
		dsp_cmd.disp_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_b_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_dve_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_i1_s* ucode;
	vout_dve_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_DVE_SETUP;
	dsp_cmd.vout_id  = vout_id;
	if (vout_id == vout_a)
		dsp_cmd.dve_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_dve_base;
	else
		dsp_cmd.dve_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_dve_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_reset(u16 vout_id, u8 reset_mixer, u8  reset_disp)
{
	struct ucode_base_addr_i1_s* ucode;
	vout_reset_t	dsp_cmd;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code 	= VOUT_RESET;
	dsp_cmd.vout_id 	= vout_id;
	dsp_cmd.reset_mixer 	= reset_mixer;
	dsp_cmd.reset_disp 	= reset_disp;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int i1_dsp_boot(struct ucode_base_addr_i1_s *ucode)
{
	/* Set up the base address */
	writel(DSP_DRAM_MAIN_REG, ucode->code_addr);
	writel(DSP_DRAM_SUB0_REG, ucode->sub0_addr);
	writel(DSP_DRAM_SUB1_REG, ucode->sub1_addr);

	/* Enable the engines */
	writel(DSP_CONFIG_MAIN_REG, 0xff);
	writel(DSP_CONFIG_SUB0_REG, 0xf);
	writel(DSP_CONFIG_SUB1_REG, 0xf);

	return 0;
}

int dsp_dram_clean_cache(void)
{
	struct ucode_base_addr_i1_s* ucode;

	ucode = dsp_get_dsp_ptr();
	if (ucode == NULL)
		return -1;

	/* clean d cache in these area because
	   all bld memory all set to cachable!!! */
	clean_d_cache((void *)(ucode->dsp_init_data.default_config_daddr),
			ucode->dsp_init_data.default_config_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_a_base,
			ucode->dsp_vout_reg.vo_ctl_a_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_b_base,
			ucode->dsp_vout_reg.vo_ctl_b_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base,
			ucode->dsp_vout_reg.vo_ctl_osd_clut_a_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base,
			ucode->dsp_vout_reg.vo_ctl_osd_clut_b_size);
	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_dve_base,
    			ucode->dsp_vout_reg.vo_ctl_dve_size);
	return 0;
}
#endif

#if (CHIP_REV == S2)

static struct ucode_base_addr_s2_s* dsp_get_dsp_ptr(void)
{
	return &G_s2_ucode;
}

/**
 * Get aligned hunge buffer.
 *
 * @param buf     - pointer to the aligned buffer pointer
 * @param raw_buf - the original buf address
 * @param size    - size of aligned buffer
 * @param align   - align byte
 * @returns       - 0 if successful, < 0 if failed
 */
static int dsp_get_align_buf(u8 **buf, u32 raw_buf, u32 size, u32 align)
{
	u32 misalign;

	size += align << 1;

	misalign = (u32)(raw_buf) & (align - 1);

	*buf = (u8 *)((u32)(raw_buf) + (align - misalign));

	return 0;
}

int dsp_set_vout_reg(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_s2_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr = VOUT_BASE;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
	addr += offset;
#else
	if (vout_id == vout_a) {
		offset = offset - VOUT_DA_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
	} else {
		offset = offset - VOUT_DB_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
	}
#endif
	writel(addr, val);

#if 0
	dsp_delay(1);
	dsp_print_var_value("addr",	addr,	"hex");
	dsp_print_var_value("offset",	offset,	"hex");
	dsp_print_var_value("value",	val,	"hex");
#endif
	return 0;
}

u32 dsp_get_vout_reg(int vout_id, u32 offset)
{
	struct ucode_base_addr_s2_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr = VOUT_BASE;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();
#if (VOUT_AHB_DIRECT_ACESS == 1)
	addr += offset;
#else
	if (vout_id == vout_a) {
		offset = offset - VOUT_DA_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_a_base + offset;
	} else {
		offset = offset - VOUT_DB_CONTROL_OFFSET;
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_b_base + offset;
	}
#endif
	return readl(addr);
}

int dsp_set_vout_osd_clut(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_s2_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	if (vout_id == vout_a) {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base + offset;
	} else {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base + offset;
	}

	writel(addr, val);
#if 0
	dsp_print_var_value("Addr", addr, "hex");
	dsp_print_var_value("value", val, "hex");
#endif
	return 0;
}

int dsp_set_vout_dve(int vout_id, u32 offset, u32 val)
{
	struct ucode_base_addr_s2_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	u32 addr;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	if (vout_id == vout_a) {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
	} else {
		addr = (u32)ucode->dsp_vout_reg.vo_ctl_dve_base + offset;
	}

	writel(addr, val);
#if 0
	dsp_print_var_value("Addr", addr, "hex");
	dsp_print_var_value("value", val, "hex");
#endif
	return 0;
}

#if (S2_SPLASH_DBG == 1)
static int dsp_print_var_value(const char *var_str, u32 value, char *format)
{
	if (strcmp(format, "hex") == 0) {
		putstr("0x");
		puthex((u32)value);
	} else if (strcmp(format, "dec") == 0) {
		putdec((u32)value);
	} else {
		putstr("Warning : Unknown data format, should be ""hex"" or ""dec""! ");
		return -1;
	}
	putstr(" : ");
	putstr(var_str);
	putstr("\r\n");

	return 0;
}

void dsp_memory_allocation_print(struct ucode_base_addr_s2_s *ucode)
{
	struct dsp_vout_reg_s2_s  *vout;

	vout = &ucode->dsp_vout_reg;

	dsp_print_var_value("ucode->dsp_start",
			    (u32)ucode->dsp_start, "hex");

	dsp_print_var_value("ucode->dsp_end",
			    (u32)ucode->dsp_end, "hex");

	dsp_print_var_value("ucode->code_addr",
			    (u32)ucode->code_addr, "hex");

	dsp_print_var_value("ucode->sub0_addr",
			    (u32)ucode->sub0_addr, "hex");

  	dsp_print_var_value("ucode->sub1_addr",
    			    (u32)ucode->sub1_addr, "hex");

	dsp_print_var_value("ucode->defbin_addr",
			    (u32)ucode->defbin_addr, "hex");

	dsp_print_var_value("ucode->shadow_reg_addr",
			    (u32)ucode->shadow_reg_addr, "hex");

	dsp_print_var_value("ucode->defimg_y_addr",
			    (u32)ucode->defimg_y_addr, "hex");

	dsp_print_var_value("ucode->defimg_uv_addr",
			    (u32)ucode->defimg_uv_addr, "hex");

	dsp_print_var_value("ucode->dsp_buffer_addr",
			    (u32)ucode->dsp_buffer_addr, "hex");

	dsp_print_var_value("ucode->dsp_buffer_size",
			    (u32)ucode->dsp_buffer_size, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_gen_ptr",
			    (u32)ucode->dsp_init_data.cmd_data_gen_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_gen_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_gen_daddr, "hex");

	dsp_print_var_value("dsp_init_data.default_config_daddr",
			    (u32)ucode->dsp_init_data.default_config_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_vcap_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_vcap_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_vcap_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_vcap_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_3rd_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_3rd_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_3rd_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_3rd_daddr, "hex");

	dsp_print_var_value("dsp_init_data.cmd_data_4th_daddr",
			    (u32)ucode->dsp_init_data.cmd_data_4th_daddr, "hex");

	dsp_print_var_value("dsp_init_data.msg_queue_4th_daddr",
			    (u32)ucode->dsp_init_data.msg_queue_4th_daddr, "hex");

	dsp_print_var_value("dsp_init_data.dsp_buffer_daddr",
			    (u32)ucode->dsp_init_data.dsp_buffer_daddr, "hex");

	dsp_print_var_value("vout->vo_ctl_a_base",
			    (u32)vout->vo_ctl_a_base, "hex");

	dsp_print_var_value("vout->vo_ctl_b_base",
			    (u32)vout->vo_ctl_b_base, "hex");

	dsp_print_var_value("vout->vo_ctl_dve_base",
			    (u32)vout->vo_ctl_dve_base, "hex");

	dsp_print_var_value("vout->vo_ctl_osd_clut_a_base",
			    (u32)vout->vo_ctl_osd_clut_a_base, "hex");

	dsp_print_var_value("vout->vo_ctl_osd_clut_b_base",
			    (u32)vout->vo_ctl_osd_clut_b_base, "hex");

	dsp_print_var_value("ucode->osd_a_buf_ptr",
			    (u32)ucode->osd_a_buf_ptr, "hex");

	dsp_print_var_value("ucode->osd_b_buf_ptr",
			    (u32)ucode->osd_b_buf_ptr, "hex");
}

static void dsp_get_ucode_version(void)
{
	u32 addr, tmp;
	struct ucode_base_addr_s2_s* ucode;
	struct dsp_ucode_ver_s version;

	ucode = dsp_get_dsp_ptr();
	addr = ucode->code_addr + 92;
	tmp = *((u32 *) addr);

	version.year  = (tmp & 0xFFFF0000) >> 16;
	version.month = (tmp & 0x0000FF00) >> 8;
	version.day   = (tmp & 0x000000FF);

	addr = ucode->code_addr + 88;
	tmp = *(u32 *)addr;

	version.edition_num = (tmp & 0xFFFF0000) >> 16;
	version.edition_ver = (tmp & 0x0000FFFF);

	addr = ucode->code_addr + 96;
	tmp = *(u32 *)addr;

	dsp_print_var_value("year", 	   version.year, 	"dec");
	dsp_print_var_value("month", 	   version.month, 	"dec");
	dsp_print_var_value("day", 	   version.day, 	"dec");
	dsp_print_var_value("edition_num", version.edition_num, "dec");
	dsp_print_var_value("edition_ver", version.edition_ver, "dec");
}
#endif

int osd_init(u16 resolution, osd_t *osdobj)
{
	int rval = 0;

	osdobj->resolution = resolution;

	switch (resolution) {
		case VO_RGB_800_480:
			osdobj->width  = 800;
			osdobj->height = 480;
			osdobj->pitch  = 800;
			break;

		case VO_RGB_480_800:
			osdobj->width  = 480;
			osdobj->height = 800;
			osdobj->pitch  = 480;
			break;

		case VO_RGB_1024_768:
			osdobj->width  = 1024;
			osdobj->height = 768;
			osdobj->pitch  = 1024;
			break;

		case VO_RGB_480P:
			osdobj->width  = 720;
			osdobj->height = 480;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_576P:
			osdobj->width  = 720;
			osdobj->height = 576;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_480I:
			osdobj->width  = 720;
			osdobj->height = 480;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_576I:
			osdobj->width  = 720;
			osdobj->height = 576;
			osdobj->pitch  = 736;
			break;

		case VO_RGB_240_432:
			osdobj->width  = 240;
			osdobj->height = 432;
			osdobj->pitch  = 256;
			break;

		case VO_RGB_360_240:
			osdobj->width  = 360;
			osdobj->height = 240;
			osdobj->pitch  = 384;
			break;

		default :
			osdobj->width  = 800;
			osdobj->height = 480;
			osdobj->pitch  = 800;
			rval = -1;
			break;
	}

	return rval;
}

/**
 * Copy BMP to OSD memory
 */
static int bmp2osd_mem(bmp_t *bmp, osd_t *osdobj, int x, int y, int top_botm_revs)
{
	int point_idx = 0;
	int i = 0, j = 0;

	if ((osdobj->buf == NULL) ||
	    (osdobj->width < 0) || (osdobj->height < 0))
		return -1;

	/* set the first line to move */
	if (top_botm_revs == 0)
		point_idx = y * osdobj->pitch + x;
	else
		point_idx = (y + bmp->height) * osdobj->pitch + x;

#if 0
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

	bmp_t* splash_bmp = NULL;
	int x = 0, y = 0;
	int rval = -1;

	if (posd == NULL) {
		putstr("Splash_Err: NULL pointer of OSD buffer!");
		return -1;
	}

	/* Get the spalsh bmp address and osd buffer address */
#if defined(SHOW_AMBOOT_SPLASH)
	splash_bmp = bld_get_splash_bmp();
#endif
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

void dsp_set_splash_bmp2osd_buf(int chan, int res, int bot_top_res)
{
	struct ucode_base_addr_s2_s* ucode;
	int vout_a = VOUT_DISPLAY_A;
	int rval = -1;

	vout_a = dsp_get_vout_a();
	ucode = dsp_get_dsp_ptr();

	osd_init(res, &ucode->osd);

	if (chan == vout_a)
		ucode->osd.buf = (u8 *)ucode->osd_a_buf_ptr;
	else
		ucode->osd.buf = (u8 *)ucode->osd_b_buf_ptr;

	memset(ucode->osd.buf, 0, DSP_OSD_BUF_SIZE);

	/* Copy splash BMP to OSD buffer */
	rval = splash_bmp2osd_buf(chan, &ucode->osd, bot_top_res);
	if (rval != 0) {
		putstr("Splash_Err: Copy splahs to OSD fail!");
	}
}


static void s2_ucode_addr_init(u32 dsp_start, u32 dsp_end,
				struct ucode_base_addr_s2_s *ucode)
{
	/** In normal ucode, the DSP buffer is used fo encode/decode data used,
	 *  here in the splash, use the available for VOUT memory mapping
	 *  registers, CLUT memory mapping registers, OSD data.
	 */
	//#	+-----------------------------------------+ DSP_INIT_DATA_ADDR
	//#	|					  | 0x000F0000
	//#	|					  |
	//#	+-----------------------------------------+ dsp_start
	//#	|					  | S2_SPLASH_START
	//#	|		4K			  | default_config_daddr
	//#	|					  |
	//#	+-----------------------------------------+ res_queue_ptr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ def_config_ptr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_a_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_b_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_dve_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_osd_clut_a_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ vo_ctl_osd_clut_b_base
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ osd0_buf_addr
	//#	|					  |
	//#	|					  |
	//#	+-----------------------------------------+ osd1_buf_addr
	//#	|					  |
	//#	+-----------------------------------------+ dsp_end - 0xe000
	//#	| 		orccode.bin:52kB	  |
	//#	+-----------------------------------------+ dsp_end - 0x1000
	//#	| 		orcme.bin:64B		  |
	//#	+-----------------------------------------+ dsp_end
	//#						    S2_SPLASH_END
	//#
	struct dsp_init_data_s2_s *dsp;
	struct dsp_vout_reg_s2_s  *vout;
	struct dsp_cmd_data_s2_s *gen, *vcap, *cfg;

	memset(ucode, 0x0, sizeof(struct ucode_base_addr_s2_s));

	ucode->dsp_start	= dsp_start; // S2_SPLASH_START
	ucode->dsp_end		= dsp_end;   // S2_SPLASH_END
	ucode->code_addr	= ucode->dsp_end - 6 * MB;
	ucode->sub0_addr	= ucode->code_addr + 512 * KB;
	ucode->sub1_addr	= ucode->sub0_addr + 512 * KB;
	ucode->defbin_addr	= ucode->sub1_addr + 512 * KB;
	ucode->shadow_reg_addr	= ucode->defbin_addr + 512 * KB;
	ucode->defimg_y_addr	= ucode->shadow_reg_addr + 1 * MB;
	ucode->defimg_uv_addr	= ucode->defimg_y_addr + 2 * MB;
	ucode->dsp_buffer_addr	= ucode->dsp_start;
	ucode->dsp_buffer_size	= ucode->dsp_end - ucode->dsp_start - 6 * MB;
#if 0
	ucode->code_addr	= ucode->dsp_end - 0xe000;
	ucode->sub0_addr	= ucode->dsp_end - 0x1000;
	ucode->sub1_addr	= ucode->dsp_end - 0x500;
	ucode->defbin_addr	= 0;
	ucode->shadow_reg_addr	= 0;
	ucode->defimg_y_addr	= 0;
	ucode->defimg_uv_addr	= 0;
	ucode->dsp_buffer_addr	= IDSP_RAM_START;
	ucode->dsp_buffer_size	= 0x008000;
#endif
	ucode->dsp_init_data.default_binary_data_addr = ucode->defbin_addr;
	ucode->dsp_init_data.default_binary_data_size = (u32)(512 * 1024);

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_gen_daddr,
			  ucode->dsp_start, S2_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_gen_size = S2_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_gen_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_gen_daddr +
			  S2_DSP_CMD_QUEUE_SIZE),
			  S2_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_gen_size = S2_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_vcap_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_gen_daddr +
			  S2_DSP_RESULT_QUEUE_SIZE),
			  S2_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_vcap_size = S2_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_vcap_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_vcap_daddr +
			  S2_DSP_CMD_QUEUE_SIZE),
			  S2_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_vcap_size = S2_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_3rd_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_vcap_daddr +
			  S2_DSP_RESULT_QUEUE_SIZE),
			  S2_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_3rd_size = S2_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_3rd_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_3rd_daddr +
			  S2_DSP_CMD_QUEUE_SIZE),
			  S2_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_3rd_size = S2_DSP_RESULT_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.cmd_data_4th_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_3rd_daddr +
			  S2_DSP_RESULT_QUEUE_SIZE),
			  S2_DSP_CMD_QUEUE_SIZE, 32);
	ucode->dsp_init_data.cmd_data_4th_size = S2_DSP_CMD_QUEUE_SIZE;

	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.msg_queue_4th_daddr,
			  ((u32)ucode->dsp_init_data.cmd_data_4th_daddr +
			  S2_DSP_CMD_QUEUE_SIZE),
			  S2_DSP_RESULT_QUEUE_SIZE, 32);

	ucode->dsp_init_data.msg_queue_4th_size = S2_DSP_RESULT_QUEUE_SIZE;

	/* default config dsp commands pointer */
	dsp_get_align_buf((u8 **)&ucode->dsp_init_data.default_config_daddr,
			  ((u32)ucode->dsp_init_data.msg_queue_4th_daddr +
			  S2_DSP_RESULT_QUEUE_SIZE),
			  S2_DSP_DEFCFG_QUEUE_SIZE, 32);
	ucode->dsp_init_data.default_config_size = S2_DSP_DEFCFG_QUEUE_SIZE;

	memset((void *)ucode->dsp_init_data.cmd_data_gen_daddr,
	       0x00, S2_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_gen_daddr,
	       0x00, S2_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_vcap_daddr,
	       0x00, S2_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_vcap_daddr,
	       0x00, S2_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_3rd_daddr,
	       0x00, S2_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_3rd_daddr,
	       0x00, S2_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.cmd_data_4th_daddr,
	       0x00, S2_DSP_CMD_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.msg_queue_4th_daddr,
	       0x00, S2_DSP_RESULT_QUEUE_SIZE);
	memset((void *)ucode->dsp_init_data.default_config_daddr,
	       0x00, S2_DSP_DEFCFG_QUEUE_SIZE);

	ucode->dsp_init_data.dsp_buffer_size = ucode->dsp_buffer_size;
	ucode->dsp_init_data.dsp_buffer_daddr = ucode->dsp_buffer_addr;
	ucode->dsp_init_data.dsp_buffer_pagetbl_daddr = 0;
	ucode->dsp_init_data.dsp_buffer_pagetbl_size = ucode->dsp_buffer_size >> 10;
	ucode->dsp_init_data.dsp_att_cpn_start_offset = 0;
	ucode->dsp_init_data.dsp_att_cpn_size = 0;
	ucode->dsp_init_data.dsp_smem_base = 0;
	ucode->dsp_init_data.dsp_smem_size = 1024 * KB;
	ucode->dsp_init_data.dsp_debug_daddr = 0x1ff80000;
	ucode->dsp_init_data.dsp_debug_size = 0x00020000;
	ucode->dsp_cmd_max_count = S2_CMD_MAX_COUNT;


	gen = (struct dsp_cmd_data_s2_s*)
	      (ucode->dsp_init_data.cmd_data_gen_daddr);

	gen->header_cmd.code = CMD_HEADER_CODE;
	gen->header_cmd.ncmd = 0;

	vcap = (struct dsp_cmd_data_s2_s*)
	      (ucode->dsp_init_data.cmd_data_vcap_daddr);

	vcap->header_cmd.code = CMD_HEADER_CODE;
	vcap->header_cmd.ncmd = 0;

	cfg = (struct dsp_cmd_data_s2_s*)
	      (ucode->dsp_init_data.default_config_daddr);

	cfg->header_cmd.code = CMD_HEADER_CODE;
	cfg->header_cmd.ncmd = 0;

	vout 				= &ucode->dsp_vout_reg;
	vout->vo_ctl_a_size		= DSP_VOUT_A_CTLREG_BUF_SIZE;
	vout->vo_ctl_b_size		= DSP_VOUT_B_CTLREG_BUF_SIZE;
	vout->vo_ctl_dve_size		= DSP_VOUT_DVE_CTLREG_BUF_SIZE;
	vout->vo_ctl_osd_clut_a_size	= DSP_VOUT_OSD_CTRREG_BUF_SIZE;
	vout->vo_ctl_osd_clut_b_size	= DSP_VOUT_OSD_CTRREG_BUF_SIZE;

	dsp_get_align_buf((u8 **)&vout->vo_ctl_a_base,
			 ((u32)ucode->dsp_init_data.default_config_daddr +
			 S2_DSP_DEFCFG_QUEUE_SIZE),
			 DSP_VOUT_A_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_b_base,
			 ((u32)vout->vo_ctl_a_base +
			 DSP_VOUT_A_CTLREG_BUF_SIZE),
			 DSP_VOUT_B_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_dve_base,
			 ((u32)vout->vo_ctl_b_base +
			 DSP_VOUT_B_CTLREG_BUF_SIZE),
			 DSP_VOUT_DVE_CTLREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_a_base,
			 ((u32)vout->vo_ctl_dve_base +
			 DSP_VOUT_DVE_CTLREG_BUF_SIZE),
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);
	dsp_get_align_buf((u8 **)&vout->vo_ctl_osd_clut_b_base,
			 ((u32)vout->vo_ctl_osd_clut_a_base +
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE),
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE, 32);

	dsp_get_align_buf((u8 **)&ucode->osd_a_buf_ptr,
			 ((u32)vout->vo_ctl_osd_clut_b_base +
			 DSP_VOUT_OSD_CTRREG_BUF_SIZE),
			 DSP_OSD_BUF_SIZE, 32);

	dsp_get_align_buf((u8 **)&ucode->osd_b_buf_ptr,
			 ((u32)ucode->osd_a_buf_ptr +
			 DSP_OSD_BUF_SIZE),
			 DSP_OSD_BUF_SIZE, 32);

	memset(vout->vo_ctl_a_base, 0x00, DSP_VOUT_A_CTLREG_BUF_SIZE);
	memset(vout->vo_ctl_b_base, 0x00, DSP_VOUT_B_CTLREG_BUF_SIZE);
	memset(ucode->osd_a_buf_ptr, 0x00, DSP_OSD_BUF_SIZE);
	memset(ucode->osd_b_buf_ptr, 0x00, DSP_OSD_BUF_SIZE);

	/* Put the dsp_init_data at this address */
	dsp = (struct dsp_init_data_s2_s *)DSP_INIT_DATA_ADDR;
	memset((u8 *)dsp, 0, sizeof(struct dsp_init_data_s2_s));
	memcpy((u8 *)dsp, &ucode->dsp_init_data, sizeof(struct dsp_init_data_s2_s));
	clean_d_cache((u8 *)DSP_INIT_DATA_ADDR, sizeof(struct dsp_init_data_s2_s));

#if (S2_SPLASH_DBG == 1)
	dsp_memory_allocation_print(ucode);
#endif
}

#if 0
u8 *dsp_get_osd_ptr(u16 vout_id)
{
	int vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_s2_s* ucode;

	ucode  = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();

	if (vout_id == vout_a)
		return (u8 *)(ucode->osd_a_buf_ptr);
	else
		return (u8 *)(ucode->osd_b_buf_ptr);
}
#endif

static int dsp_write_to_command_queue(struct ucode_base_addr_s2_s *ucode,
				      u8* cmd, u32 align_cmd_len)
{
	struct dsp_cmd_data_s2_s *cmd_queue_ptr;
	u32 cmd_max_count = 0;
	int i = 0;

	cmd_queue_ptr 	= (struct dsp_cmd_data_s2_s *)
			  ((ucode->dsp_init_data.default_config_daddr | DRAM_START_ADDR));
	cmd_max_count 	= ucode->dsp_cmd_max_count;

	i = cmd_queue_ptr->header_cmd.ncmd;
	if (cmd_queue_ptr->header_cmd.ncmd < cmd_max_count) {
		/* clean buffer to 0 */
		memset((cmd_queue_ptr->cmd + i), 0, sizeof(struct dsp_cmd_s));
		memcpy((cmd_queue_ptr->cmd + i), cmd, align_cmd_len);

		cmd_queue_ptr->header_cmd.ncmd = cmd_queue_ptr->header_cmd.ncmd + 1;
	} else {
		return -1;
	}

	return 0;
}

int dsp_get_win_params(int resolution, u16 *width, u16 *height)
{
	int rval = 0;

	switch (resolution) {
		case VO_RGB_1024_768:
			*width	= 1024;
			*height	= 768;
			break;
		case VO_RGB_800_480:
			*width	= 800;
			*height	= 480;
			break;
		case VO_RGB_480_800:
			*width	= 480;
			*height	= 800;
			break;
		case VO_RGB_360_240:
			*width	= 360;
			*height	= 240;
			break;
		case VO_RGB_360_288:
			*width	= 360;
			*height	= 288;
			break;
		case VO_RGB_320_240:
			*width	= 320;
			*height	= 240;
			break;
		case VO_RGB_320_288:
			*width	= 320;
			*height	= 288;
			break;
		case VO_RGB_480P:
			*width	= 720;
			*height	= 480;
			break;
		case VO_RGB_576P:
			*width	= 720;
			*height	= 576;
			break;
		case VO_RGB_480I:
			*width	= 720;
			*height	= 480 / 2;
			break;
		case VO_RGB_576I:
			*width	= 720;
			*height	= 576 / 2;
			break;
		case VO_RGB_240_432:
			*width	= 240;
			*height	= 432;
			break;
		default :
			*width  = 0;
			*height = 0;
			rval    = -1;
			break;
	}

	return rval;
}

int dsp_vout_mixer_setup(u16 vout_id, int resolution)
{
	struct ucode_base_addr_s2_s* ucode;
	vout_mixer_setup_t	dsp_cmd;
	u8  interlaced 		= 0;
	u8  frame_rate 		= 0;
	u16 active_win_width 	= 0;
	u16 active_win_height 	= 0;
	u8  back_ground_v 	= DEFAULT_BACK_GROUND_V;
	u8  back_ground_u	= DEFAULT_BACK_GROUND_U;
	u8  back_ground_y 	= DEFAULT_BACK_GROUND_Y;
	u8  mixer_444 		= 0;
	u8  highlight_v		= DEFAULT_HIGHLIGHT_V;
	u8  highlight_u		= DEFAULT_HIGHLIGHT_U;
	u8  highlight_y		= DEFAULT_HIGHLIGHT_Y;
	u8  highlight_thresh	= DEFAULT_HIGHLIGHT_THRESH;
	u8  reverse_en		= 0;
	u8  csc_en		= 0;
	u8  reserved		= 0;
//	u32 csc_parms[9];

	int rval = -1;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &active_win_width, &active_win_height);

	if (rval != 0)
		return -1;

	switch (resolution) {
	case VO_RGB_1024_768:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_800_480:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480_800:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_360_240:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_320_240:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_320_288:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_360_288:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480P:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_576P:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	case VO_RGB_480I:
		interlaced		= 1;
		frame_rate		= 1;
		break;
	case VO_RGB_576I:
		interlaced		= 1;
		frame_rate		= 1;
		break;
	case VO_RGB_240_432:
		interlaced		= 0;
		frame_rate		= 1;
		break;
	default:
		break;
	}

	dsp_cmd.cmd_code	= VOUT_MIXER_SETUP;
	dsp_cmd.vout_id		= vout_id;
	dsp_cmd.interlaced 	= interlaced;
	dsp_cmd.frm_rate 	= frame_rate;
	dsp_cmd.act_win_width 	= active_win_width;
	dsp_cmd.act_win_height 	= active_win_height;
	dsp_cmd.back_ground_v 	= back_ground_v;
	dsp_cmd.back_ground_u 	= back_ground_u;
	dsp_cmd.back_ground_y 	= back_ground_y;
	dsp_cmd.mixer_444	= mixer_444;
	dsp_cmd.highlight_v 	= highlight_v;
	dsp_cmd.highlight_u 	= highlight_u;
	dsp_cmd.highlight_y 	= highlight_y;
	dsp_cmd.highlight_thresh = highlight_thresh;
	dsp_cmd.reverse_en	= reverse_en;
	dsp_cmd.csc_en		= csc_en;
	dsp_cmd.reserved[0]	= reserved;
	dsp_cmd.reserved[1]	= reserved;
//	dsp_cmd.csc_parms[9]; /* */

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_video_setup(u16 vout_id, int video_en, int video_src, int resolution)
{
	struct ucode_base_addr_s2_s* ucode;
	vout_video_setup_t	dsp_cmd;
	u8  flip = 0;
	u8  rotate = 0;
	u16 reserved = 0;
	u16 win_offset_x = VIDEO_WIN_OFFSET_X;
	u16 win_offset_y = VIDEO_WIN_OFFSET_Y;
	u16 win_width  = 0;
	u16 win_height = 0;
	u16 default_img_pitch = 0;
	u8  default_img_repeat_field = 0;
	u8  reserved2 = 0;
	int rval = -1;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &win_width, &win_height);
	if (rval != 0)
		return -1;

	default_img_pitch 	= (((int)(win_width + 31) / 32) * 32);

	dsp_cmd.cmd_code 	= VOUT_VIDEO_SETUP;
	dsp_cmd.vout_id 	= vout_id;
	dsp_cmd.en 		= video_en;
	dsp_cmd.src 		= video_src;
	dsp_cmd.flip 		= flip;
	dsp_cmd.rotate 		= rotate;
	dsp_cmd.reserved	= reserved;
	dsp_cmd.win_offset_x 	= win_offset_x;
	dsp_cmd.win_offset_y 	= win_offset_y;
	dsp_cmd.win_width 	= win_width;
	dsp_cmd.win_height 	= win_height;

	dsp_cmd.default_img_y_addr 	 = ucode->defimg_y_addr;
	dsp_cmd.default_img_uv_addr 	 = ucode->defimg_uv_addr;
	dsp_cmd.default_img_pitch 	 = default_img_pitch;
	dsp_cmd.default_img_repeat_field = default_img_repeat_field;
	dsp_cmd.reserved2		 = reserved2;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_setup(u16 vout_id, u8 flip, u8 osd_en, u8 osd_src, int resolution)
{
	struct ucode_base_addr_s2_s* ucode;
	vout_osd_setup_t	dsp_cmd;
	u16 vout_a = VOUT_DISPLAY_A;
	u8	rescaler_en = 0;
	u8	premultiplied = 0;
	u8	global_blend = 0xff;
	u16	win_offset_x = 0;
	u16	win_offset_y = 0;
	u16	win_width = 0;
	u16	win_height= 0;
	u16	rescaler_input_width = 0;
	u16	rescaler_input_height = 0;
	u16	osd_buf_pitch = 0;
	u8	osd_buf_repeat = 0;
	u8	osd_direct_mode = 0;
	u16	osd_transparent_color = 0;
	u8	osd_transparent_color_en = 0;
	u8	osd_swap_bytes = 0;
	int	rval = -1;

	ucode = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	rval = dsp_get_win_params(resolution, &win_width, &win_height);
	if (rval != 0)
		return -1;

	osd_buf_pitch 	= (((int)(win_width + 31) / 32) * 32);

	rescaler_input_width	= win_width;
	rescaler_input_height	= win_height;

	dsp_cmd.cmd_code = VOUT_OSD_SETUP;
	dsp_cmd.vout_id = vout_id;
	dsp_cmd.en = osd_en;
	dsp_cmd.src = osd_src;
	dsp_cmd.flip = flip;
	dsp_cmd.rescaler_en = rescaler_en;
	dsp_cmd.premultiplied = premultiplied;
	dsp_cmd.global_blend = global_blend;
	dsp_cmd.win_offset_x = win_offset_x;
	dsp_cmd.win_offset_y = win_offset_y;
	dsp_cmd.win_width = win_width;
	dsp_cmd.win_height = win_height;
	dsp_cmd.rescaler_input_width = rescaler_input_width;
	dsp_cmd.rescaler_input_height = rescaler_input_height;
	if (vout_id == vout_a)
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
	else
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

	dsp_cmd.osd_buf_pitch = osd_buf_pitch;
	dsp_cmd.osd_buf_repeat_field = osd_buf_repeat;
	dsp_cmd.osd_direct_mode = osd_direct_mode;
	dsp_cmd.osd_transparent_color = osd_transparent_color;
	dsp_cmd.osd_transparent_color_en = osd_transparent_color_en;
	dsp_cmd.osd_swap_bytes = osd_swap_bytes;
	dsp_cmd.osd_buf_info_dram_addr = 0x0;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_buf_setup(u16 vout_id, int resolution)
{
	struct ucode_base_addr_s2_s* ucode;
	vout_osd_buf_setup_t	dsp_cmd;
	u16 	vout_a = VOUT_DISPLAY_A;
	u16	osd_buf_pitch = 0;
	u8	osd_buf_repeat = 0;

	ucode  = dsp_get_dsp_ptr();
	vout_a = dsp_get_vout_a();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_OSD_BUFFER_SETUP;
	dsp_cmd.vout_id = vout_id;

	if (vout_id == vout_a)
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_a_buf_ptr;
	else
		dsp_cmd.osd_buf_dram_addr = (u32)ucode->osd_b_buf_ptr;

	switch (resolution) {
		case VO_RGB_1024_768:
			osd_buf_pitch	= 1024;
			break;
		case VO_RGB_800_480:
			osd_buf_pitch	= 800;
			break;
		case VO_RGB_480_800:
			osd_buf_pitch	= 480;
			break;
		case VO_RGB_360_240:
			osd_buf_pitch	= 384;
			break;
		case VO_RGB_360_288:
			osd_buf_pitch	= 384;
			break;
		case VO_RGB_320_240:
			osd_buf_pitch	= 320;
			break;
		case VO_RGB_320_288:
			osd_buf_pitch	= 320;
			break;
		case VO_RGB_480P:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_576P:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_480I:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_576I:
			osd_buf_pitch	= 736;
			break;
		case VO_RGB_240_432:
			osd_buf_pitch	= 256;
			break;
		default :
			break;
	}

	dsp_cmd.osd_buf_pitch 		= osd_buf_pitch;
	dsp_cmd.osd_buf_repeat_field 	= osd_buf_repeat;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_osd_clut_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_s2_s* ucode;
	vout_osd_clut_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code	= VOUT_OSD_CLUT_SETUP;
	dsp_cmd.vout_id		= vout_id;

	if (vout_id == vout_a)
		dsp_cmd.clut_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base;
	else
		dsp_cmd.clut_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_display_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_s2_s* ucode;
	vout_display_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_DISPLAY_SETUP;
	dsp_cmd.vout_id  = vout_id;
	if (vout_id == vout_a)
		dsp_cmd.disp_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_a_base;
	else
		dsp_cmd.disp_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_b_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_dve_setup(u16 vout_id)
{
	u16 vout_a = VOUT_DISPLAY_A;
	struct ucode_base_addr_s2_s* ucode;
	vout_dve_setup_t	dsp_cmd;

	vout_a = dsp_get_vout_a();
	ucode  = dsp_get_dsp_ptr();

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = VOUT_DVE_SETUP;
	dsp_cmd.vout_id  = vout_id;
	if (vout_id == vout_a)
		dsp_cmd.dve_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_dve_base;
	else
		dsp_cmd.dve_config_dram_addr =
			(u32)ucode->dsp_vout_reg.vo_ctl_dve_base;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

int dsp_vout_reset(u16 vout_id, u8 reset_mixer, u8  reset_disp)
{
	struct ucode_base_addr_s2_s* ucode;
	vout_reset_t	dsp_cmd;

	ucode = dsp_get_dsp_ptr();
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code 	= VOUT_RESET;
	dsp_cmd.vout_id 	= vout_id;
	dsp_cmd.reset_mixer 	= reset_mixer;
	dsp_cmd.reset_disp 	= reset_disp;

	dsp_write_to_command_queue(ucode, (u8 *)&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int s2_dsp_boot(struct ucode_base_addr_s2_s *ucode)
{
	/* Set up the base address */
	writel(DSP_DRAM_MAIN_REG, ucode->code_addr);
	writel(DSP_DRAM_SUB0_REG, ucode->sub0_addr);
	writel(DSP_DRAM_SUB1_REG, ucode->sub1_addr);

	/* Enable the engines */
	writel(DSP_CONFIG_MAIN_REG, 0xff);
	writel(DSP_CONFIG_SUB0_REG, 0xf);
	writel(DSP_CONFIG_SUB1_REG, 0xf);

	return 0;
}

int dsp_dram_clean_cache(void)
{
	struct ucode_base_addr_s2_s* ucode;

	ucode = dsp_get_dsp_ptr();
	if (ucode == NULL)
		return -1;

	/* clean d cache in these area because
	   all bld memory all set to cachable!!! */
	clean_d_cache((void *)(ucode->dsp_init_data.default_config_daddr),
			ucode->dsp_init_data.default_config_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_a_base,
			ucode->dsp_vout_reg.vo_ctl_a_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_b_base,
			ucode->dsp_vout_reg.vo_ctl_b_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_a_base,
			ucode->dsp_vout_reg.vo_ctl_osd_clut_a_size);

	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_osd_clut_b_base,
			ucode->dsp_vout_reg.vo_ctl_osd_clut_b_size);
	clean_d_cache((void *)ucode->dsp_vout_reg.vo_ctl_dve_base,
    			ucode->dsp_vout_reg.vo_ctl_dve_size);
	return 0;
}
#endif


#if ((CHIP_REV == A2) || (CHIP_REV == A2S) || \
  (CHIP_REV == A3) || (CHIP_REV == A5))
static void dsp_init(void)
{
#if (CHIP_REV == A2)
  /* ucode addr init */
  a2_ucode_addr_init(A2_DSP_START, A2_DSP_END, &G_a2_ucode);
#endif

#if (CHIP_REV == A2S)
  /* ucode addr init */
  a2s_ucode_addr_init(A2S_DSP_START, A2S_DSP_END, &G_a2s_ucode);
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
  /* ucode addr init */
  a3_ucode_addr_init(A3_DSP_START, A3_DSP_END, &G_a3_ucode);
#endif
}

static void dsp_vout_ptr_set(int chan, int res, u32 osd_addr)
{
#if (CHIP_REV == A2)
  a2_dsp_vout_set(res,
    G_a2_ucode.defimg_y_addr,
    G_a2_ucode.defimg_uv_addr,
    osd_addr,
    osd_addr);
#endif

#if (CHIP_REV == A2S)
  a2s_dsp_vout_set(res,
    G_a2s_ucode.defimg_y_addr,
    G_a2s_ucode.defimg_uv_addr,
    osd_addr,
    osd_addr);
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
  a3_dsp_vout_set(chan,
    res,
    G_a3_ucode.defimg_y_addr,
    G_a3_ucode.defimg_uv_addr,
    osd_addr,
    osd_addr);
#endif
}
#endif

static void dsp_load_code(void)
{
#if (CHIP_REV == A2)
  const u8 *main_addr, *memd_addr;
  int main_len, memd_len;

  /* get embedded binary code address and length */
  main_len = embbin_get("code.bin", &main_addr);
  memd_len = embbin_get("memd.bin", &memd_addr);

  memcpy((void *)G_a2_ucode.code_addr, main_addr, main_len);
  memcpy((void *)G_a2_ucode.memd_addr, memd_addr, memd_len);
#endif

#if (CHIP_REV == A2S)
  const u8 *main_addr, *memd_addr;
  int main_len, memd_len;

  /* get embedded binary code address and length */
  main_len = embbin_get("code.bin", &main_addr);
  memd_len = embbin_get("memd.bin", &memd_addr);

  memcpy((void *)G_a2s_ucode.code_addr, main_addr, main_len);
  memcpy((void *)G_a2s_ucode.memd_addr, memd_addr, memd_len);
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
  const u8 *main_addr, *sub0_addr, *sub1_addr;
  int main_len, sub0_len, sub1_len;

  /* get embedded binary code address and length */
  main_len = embbin_get("orccode.bin", &main_addr);
  sub0_len = embbin_get("orcme.bin", &sub0_addr);
  sub1_len = embbin_get("orcmdxf.bin", &sub1_addr);

  memcpy((void *)G_a3_ucode.code_addr, main_addr, main_len);
  memcpy((void *)G_a3_ucode.sub0_addr, sub0_addr, sub0_len);
  memcpy((void *)G_a3_ucode.sub1_addr, sub1_addr, sub1_len);
#endif

#if (CHIP_REV == A5S)
  const u8 *main_addr, *sub0_addr;
  int main_len, sub0_len;

  /* get embedded binary code address and length */
  main_len = embbin_get("orccode.bin", &main_addr);
  sub0_len = embbin_get("orcme.bin", &sub0_addr);

  memcpy((void *)G_a5s_ucode.code_addr, main_addr, main_len);
  memcpy((void *)G_a5s_ucode.sub0_addr, sub0_addr, sub0_len);

#if (A5S_SPLASH_DBG == 1)
  /* dump ucode address */
  dsp_print_var_value("orccode.bin", G_a5s_ucode.code_addr, "hex");
  dsp_print_var_value("orcme.bin",   G_a5s_ucode.sub0_addr, "hex");
  dsp_get_ucode_version();
#endif
#endif

#if (CHIP_REV == A7)
  const u8 *main_addr, *sub0_addr;
  int main_len, sub0_len;

  /* get embedded binary code address and length */
  main_len = embbin_get("orccode.bin", &main_addr);
  sub0_len = embbin_get("orcme.bin", &sub0_addr);

  memcpy((void *)G_a7_ucode.code_addr, main_addr, main_len);
  memcpy((void *)G_a7_ucode.sub0_addr, sub0_addr, sub0_len);

#if (A7_SPLASH_DBG == 1)
  /* dump ucode address */
  dsp_print_var_value("orccode.bin", G_a7_ucode.code_addr, "hex");
  dsp_print_var_value("main_len", main_len, "hex");
  dsp_print_var_value("orcme.bin", G_a7_ucode.sub0_addr, "hex");
  dsp_print_var_value("sub0_len", sub0_len, "hex");
  dsp_get_ucode_version();
#endif
#endif

#if (CHIP_REV == I1)
	const u8 *main_addr, *sub0_addr, *sub1_addr;
	int main_len, sub0_len, sub1_len;

	/* get embedded binary code address and length */
	main_len = embbin_get("orccode.bin", &main_addr);
	sub0_len = embbin_get("orcme.bin", &sub0_addr);
	sub1_len = embbin_get("orcmdxf.bin", &sub1_addr);

	memcpy((void *)G_i1_ucode.code_addr, main_addr, main_len);
	memcpy((void *)G_i1_ucode.sub0_addr, sub0_addr, sub0_len);
	memcpy((void *)G_i1_ucode.sub1_addr, sub1_addr, sub1_len);
	clean_d_cache((void *)(G_i1_ucode.code_addr), main_len);
	clean_d_cache((void *)(G_i1_ucode.sub0_addr), sub0_len);
	clean_d_cache((void *)(G_i1_ucode.sub1_addr), sub1_len);

#if (I1_SPLASH_DBG == 1)
	/* dump ucode address */
	dsp_print_var_value("orccode.bin", G_i1_ucode.code_addr, "hex");
	dsp_print_var_value("main_len", main_len, "hex");
	dsp_print_var_value("orcme.bin", G_i1_ucode.sub0_addr, "hex");
	dsp_print_var_value("sub0_len", sub0_len, "hex");
	dsp_print_var_value("orcmdxf.bin", G_i1_ucode.sub1_addr, "hex");
	dsp_print_var_value("sub1_len", sub1_len, "hex");
	dsp_get_ucode_version();
#endif
#endif

#if (CHIP_REV == S2)
	const u8 *main_addr, *sub0_addr, *sub1_addr;
	int main_len, sub0_len, sub1_len;

	/* get embedded binary code address and length */
	main_len = embbin_get("orccode.bin", &main_addr);
	sub0_len = embbin_get("orcme.bin", &sub0_addr);
	sub1_len = embbin_get("orcmdxf.bin", &sub1_addr);

	memcpy((void *)G_s2_ucode.code_addr, main_addr, main_len);
	memcpy((void *)G_s2_ucode.sub0_addr, sub0_addr, sub0_len);
	memcpy((void *)G_s2_ucode.sub1_addr, sub1_addr, sub1_len);
	clean_d_cache((void *)(G_s2_ucode.code_addr), main_len);
	clean_d_cache((void *)(G_s2_ucode.sub0_addr), sub0_len);
	clean_d_cache((void *)(G_s2_ucode.sub1_addr), sub1_len);

#if (S2_SPLASH_DBG == 1)
	/* dump ucode address */
	dsp_print_var_value("orccode.bin", G_s2_ucode.code_addr, "hex");
	dsp_print_var_value("main_len", main_len, "hex");
	dsp_print_var_value("orcme.bin", G_s2_ucode.sub0_addr, "hex");
	dsp_print_var_value("sub0_len", sub0_len, "hex");
	dsp_print_var_value("orcmdxf.bin", G_s2_ucode.sub1_addr, "hex");
	dsp_print_var_value("sub1_len", sub1_len, "hex");
	dsp_get_ucode_version();
#endif
#endif
}

void dsp_boot(void)
{
#if (CHIP_REV == A2)
  a2_dsp_boot(&G_a2_ucode);
#endif

#if (CHIP_REV == A2S)
  a2s_dsp_boot(&G_a2s_ucode);
#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
  a3_dsp_boot(&G_a3_ucode);
#endif

#if (CHIP_REV == A5S)
  a5s_dsp_boot(&G_a5s_ucode);
#endif
#if (CHIP_REV == A7)
  a7_dsp_boot(&G_a7_ucode);
#endif
#if (CHIP_REV == I1)
	i1_dsp_boot(&G_i1_ucode);
#endif
#if (CHIP_REV == S2)
	s2_dsp_boot(&G_s2_ucode);
#endif
}

int dsp_vout_init(void)
{
#if (CHIP_REV == A5S)
  extern void get_stepping_info(int *chip, int *major, int *minor);
  struct ucode_base_addr_a5s_s *ucode;
  int chip_id, major, minor;

  get_stepping_info(&chip_id, &major, &minor);

  if (chip_id == 5 && major == 1 && minor == 0) {
    ucode = dsp_get_dsp_ptr();
    /* ucode addr init, memory allocation */
    a5s_ucode_addr_init(A5S_SPLASH_START, A5S_SPLASH_END, ucode);
    /* load dsp code into */
    dsp_load_code();
  } else {
    putstr("Chip id");
    putdec(chip_id);
    putstr("load code not implemented");
  }
#elif (CHIP_REV == A7)
  extern void get_stepping_info(int *chip, int *major, int *minor);
  struct ucode_base_addr_a7_s *ucode;
  int chip_id, major, minor;
  int retry = 3, status = 0;

  get_stepping_info(&chip_id, &major, &minor);

  if (chip_id == 7 && major == 0 && minor == 0) {
    ucode = dsp_get_dsp_ptr();
    /* ucode addr init, memory allocation */
    a7_ucode_addr_init(A7_SPLASH_START, A7_SPLASH_END, ucode);

    /*reset IDSP */
    writel(0x7011801c, 0xff);

    /*ensure the reset is done*/
    writel(0x70118000, 0x1000);

    status = readl(0x70110008);

    while(status > 0 && retry > 0) {
      writel(0x7011801c, 0xff);
      writel(0x70118000, 0x1000);
      status = readl(0x70110008);
      retry --;
    }

    /* load dsp code into */
    dsp_load_code();
  } else {
    putstr("Chip id ");
    putdec(chip_id);
    putstr(" load code not implemented");
  }
#elif (CHIP_REV == I1)
	extern void get_stepping_info(int *chip, int *major, int *minor);
	struct ucode_base_addr_i1_s *ucode;
	int chip_id, major, minor;
	int retry = 3, status = 0;

	get_stepping_info(&chip_id, &major, &minor);

	//chip_id == 7 &&
	if (1) {
		ucode = dsp_get_dsp_ptr();
		/* ucode addr init, memory allocation */
		i1_ucode_addr_init(I1_SPLASH_START, I1_SPLASH_END, ucode);

		/*reset IDSP */
		writel(APB_BASE + 0x118030, 0x201);
		writel(0xe811801c, 0xff);

		/*ensure the reset is done*/
		writel(0xe8118000, 0x1000);

		status = readl(0xe8110008);

		while(status > 0 && retry > 0) {
			writel(0xe811801c, 0xff);
			writel(0xe8118000, 0x1000);
			status = readl(0xe8110008);
			retry --;
		}

		/* load dsp code into */
		dsp_load_code();
	} else {
		putstr("Chip id ");
		putdec(chip_id);
		putstr(" load code not implemented");
	}
#elif (CHIP_REV == S2)
	extern void get_stepping_info(int *chip, int *major, int *minor);
	struct ucode_base_addr_s2_s *ucode;
	int chip_id, major, minor;
	int retry = 3, status = 0;

	get_stepping_info(&chip_id, &major, &minor);

	//chip_id == 7 &&
	if (1) {
		ucode = dsp_get_dsp_ptr();
		/* ucode addr init, memory allocation */
		s2_ucode_addr_init(S2_SPLASH_START, S2_SPLASH_END, ucode);

		/*reset IDSP */
		writel(APB_BASE + 0x118030, 0x201);
		writel(APB_BASE + 0x11801c, 0xff);

		/*ensure the reset is done*/
		writel(APB_BASE + 0x118000, 0x1000);

		status = readl(0xe8110008);

		while(status > 0 && retry > 0) {
			writel(APB_BASE + 0x11801c, 0xff);
			writel(APB_BASE + 0x118000, 0x1000);
			status = readl(APB_BASE + 0x110008);
			retry --;
		}

		/* load dsp code into */
		dsp_load_code();
	} else {
		putstr("Chip id ");
		putdec(chip_id);
		putstr(" load code not implemented");
	}
#endif

  return 0;
}

int dsp_vout_set(int chan, int res)
{
#if defined(SHOW_AMBOOT_SPLASH)
  /* DSP setting */
#if ((CHIP_REV == A2) || (CHIP_REV == A2S))
  osd_t* posd;

  posd = get_osd_obj(chan);

  dsp_init();

  dsp_vout_ptr_set(chan, res, (u32)posd->buf);

  dsp_load_code();

  dsp_boot();

#endif

#if (CHIP_REV == A3) || (CHIP_REV == A5)
  osd_t* posd;

  posd = get_osd_obj(chan);

  dsp_init();

  if (chan == 0) {
    dsp_vout_ptr_set(1, -1, (u32)posd->buf);
    dsp_vout_ptr_set(chan, res, (u32)posd->buf);
  } else if (chan == 1) {
    dsp_vout_ptr_set(0, -1, (u32)posd->buf);
    dsp_vout_ptr_set(chan, res, (u32)posd->buf);
  } else {
    putstr("Wrong chan, should be 0 or 1");
    return -1;
  }

  dsp_load_code();

  dsp_boot();

#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7)
  u8 osd_en       = 1;
  u8 osd_src      = 0;
  u8 video_en     = 1;
  u8 video_src    = 1; /* 0:default image, 1:background color, 2:enc, 3:dec */
  int bot_top_res = 1;
  u8 flip         = 0;

  /* dspcmd vout_display_setup */
  if ((chan == VOUT_DISPLAY_A) || (chan == VOUT_DISPLAY_B)) {
    /* reset mixer and display modules */
    dsp_vout_reset(chan, 1, 1);
    /* copy splash bmp data to osd buf*/
    dsp_set_splash_bmp2osd_buf(chan, res, bot_top_res);
    /* display registers memory mapping address setup */
    dsp_vout_display_setup(chan);
#ifndef CONFIG_CVBS_MODE_NONE
	dsp_vout_dve_setup(chan);
#endif

    /* clut registers bus memory mapping address */
    dsp_vout_osd_clut_setup(chan);
    /* dspcmd osd_buf_setup */
    dsp_vout_osd_buf_setup(chan, res);
    /* dspcmd osd_setup */
    dsp_vout_osd_setup(chan, flip, osd_en, osd_src, res);
    /* dspcmd vout_mixer_setup */
    dsp_vout_mixer_setup (chan, res);
    /* dspcmd vout_video_setup */
    dsp_vout_video_setup (chan, video_en, video_src, res);

    dsp_dram_clean_cache();
    /* dsp boot */
    dsp_boot();
  } else {
    return -1;
  }

#endif

#if (CHIP_REV == I1)
  u8 osd_en       = 1;
  u8 osd_src      = 0;
  u8 video_en     = 0;
  u8 video_src    = 1; /* 0:default image, 1:background color, 2:enc, 3:dec */
  int bot_top_res = 1;
  u8 flip         = 0;

  /* dspcmd vout_display_setup */
  if ((chan == VOUT_DISPLAY_A) || (chan == VOUT_DISPLAY_B)) {
    /* reset mixer and display modules */
    dsp_vout_reset(chan, 1, 1);
    /* copy splash bmp data to osd buf*/
    dsp_set_splash_bmp2osd_buf(chan, res, bot_top_res);
    /* display registers memory mapping address setup */
    dsp_vout_display_setup(chan);
#ifndef CONFIG_CVBS_MODE_NONE
	dsp_vout_dve_setup(chan);
#endif
    /* clut registers bus memory mapping address */
    dsp_vout_osd_clut_setup(chan);
    /* dspcmd osd_buf_setup */
    dsp_vout_osd_buf_setup(chan, res);
    /* dspcmd osd_setup */
    dsp_vout_osd_setup(chan, flip, osd_en, osd_src, res);
    /* dspcmd vout_mixer_setup */
    dsp_vout_mixer_setup (chan, res);
    /* dspcmd vout_video_setup */
    dsp_vout_video_setup (chan, video_en, video_src, res);

    dsp_dram_clean_cache();
    /* dsp boot */
    dsp_boot();
  } else {
    return -1;
  }

#endif

#if (CHIP_REV == S2)
  u8 osd_en       = 1;
  u8 osd_src      = 0;
  u8 video_en     = 0;
  u8 video_src    = 1; /* 0:default image, 1:background color, 2:enc, 3:dec */
  int bot_top_res = 1;
  u8 flip         = 0;

  /* dspcmd vout_display_setup */
  if ((chan == VOUT_DISPLAY_A) || (chan == VOUT_DISPLAY_B)) {
    /* reset mixer and display modules */
    dsp_vout_reset(chan, 1, 1);
    /* copy splash bmp data to osd buf*/
    dsp_set_splash_bmp2osd_buf(chan, res, bot_top_res);
    /* display registers memory mapping address setup */
    dsp_vout_display_setup(chan);
#ifndef CONFIG_CVBS_MODE_NONE
	dsp_vout_dve_setup(chan);
#endif
    /* clut registers bus memory mapping address */
    dsp_vout_osd_clut_setup(chan);
    /* dspcmd osd_buf_setup */
    dsp_vout_osd_buf_setup(chan, res);
    /* dspcmd osd_setup */
    dsp_vout_osd_setup(chan, flip, osd_en, osd_src, res);
    /* dspcmd vout_mixer_setup */
    dsp_vout_mixer_setup (chan, res);
    /* dspcmd vout_video_setup */
    dsp_vout_video_setup (chan, video_en, video_src, res);

    dsp_dram_clean_cache();
    /* dsp boot */
    dsp_boot();
  } else {
    return -1;
  }

#endif
#endif
  return 0;
}
