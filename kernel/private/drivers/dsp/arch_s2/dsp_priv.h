/*
 * dsp_priv.h
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __DSP_PRIV_H
#define __DSP_PRIV_H

// start - physical
// base - virtual

#define DSP_UCODE_SIZE			(6 * MB)
#define DSP_BUFFER_SIZE			(get_ambarella_dspmem_size() - DSP_UCODE_SIZE)

#define DSP_CODE_MEMORY_OFFSET		(0)
#define DSP_BINARY_DATA_MEMORY_OFFSET	(3 * MB)
#define DSP_MEMD_MEMORY_OFFSET		(4 * MB)
#define DSP_MDXF_MEMORY_OFFSET		(5 * MB)

#define DSP_DRAM_START			(get_ambarella_dspmem_phys())
#define DSP_DRAM_BASE			(get_ambarella_dspmem_virt())

#define DSP_UCODE_START			(DSP_DRAM_START + DSP_BUFFER_SIZE)
#define DSP_UCODE_BASE			(DSP_DRAM_BASE + DSP_BUFFER_SIZE)

#define DSP_DRAM_CODE_START		(DSP_UCODE_START + DSP_CODE_MEMORY_OFFSET)
#define DSP_DRAM_CODE_BASE		(DSP_UCODE_BASE + DSP_CODE_MEMORY_OFFSET)

#define DSP_BINARY_DATA_START		(DSP_UCODE_START + DSP_BINARY_DATA_MEMORY_OFFSET)
#define DSP_BINARY_DATA_BASE		(DSP_UCODE_BASE + DSP_BINARY_DATA_MEMORY_OFFSET)

#define DSP_DRAM_MEMD_START		(DSP_UCODE_START + DSP_MEMD_MEMORY_OFFSET)
#define DSP_DRAM_MEMD_BASE		(DSP_UCODE_BASE + DSP_MEMD_MEMORY_OFFSET)

#define DSP_DRAM_MDXF_START		(DSP_UCODE_START + DSP_MDXF_MEMORY_OFFSET)
#define DSP_DRAM_MDXF_BASE		(DSP_UCODE_BASE + DSP_MDXF_MEMORY_OFFSET)

#define DSP_IDSP_BASE			(get_ambarella_apb_virt() + 0x11801C)
#define DSP_ORC_BASE			(get_ambarella_apb_virt() + 0x11FF00)

#if 0
#define DSP_RESULT_SIZE			128
#define DSP_MAX_RESULT_NUM		32

#define DSP_CMD_SIZE			128
#define DSP_MAX_CMD_NUM			31
#define DSP_MAX_DEFAULT_CMD_NUM		31
#endif


#define DSP_INIT_DATA_BASE		(get_ambarella_ppm_virt() + 0x000F0000)
#define DSP_LOG_AREA			(get_ambarella_ppm_virt() + 0x00080000)
#define DSP_LOG_AREA_PHYS		(get_ambarella_ppm_phys() + 0x00080000)
#define DSP_LOG_SIZE			(128*KB)

#define DSP_MAX_UDEC		16

typedef struct dsp_cmdq_header_s {
	uint32_t cmd_code;
	uint32_t cmd_seq_num;
	uint32_t num_cmds;
	uint32_t reserved[29];
} dsp_cmdq_header_t;

#define MAX_NUM_CMD		32
#define MAX_NUM_MSG		32
#define MAX_DEFAULT_CMD		32
#define MAX_BATCH_CMD_NUM_PER_STREAM	(4)
#define MAX_BATCH_CMD_CHUNK	(4)

enum {
	DSP_CMD_MODE_NONE,	// not booted
	DSP_CMD_MODE_BUSY,	// changing mode
	DSP_CMD_MODE_DEFAULT,	// default command
	DSP_CMD_MODE_NORMAL,	// normal command
	DSP_CMD_MODE_SYNCC,	// sync counter command mode
};

typedef enum {
	DSP_CMD_TYPE_NORMAL = 0,
	DSP_CMD_TYPE_BLOCK = 1,	// cache block cmds first, copy and send them in IRQ
	DSP_CMD_TYPE_BATCH = 2,	// cache batch cmds and send the cmd buffer in one cmd
} DSP_CMD_TYPE;

typedef struct dsp_cmd_port_s {
	DSP_CMD		cmd_buffer[MAX_NUM_CMD - 1];
	DSP_CMD		*cmd_queue;
	u32		prev_cmd_seq_num;
	u16		update_cmd_seq_num;
	u16		max_num_cmds;
	u16		num_cmds;
	u16		num_udec_cmds;
	int		cmd_sent;
	DSP_MSG		*msg_queue;
	u32		max_num_msgs;
	u32		msg_seq_num;
	DSP_CMD_TYPE	cmd_type;
	DSP_CMD	cmd_blk[MAX_NUM_CMD - 1];
	u8		num_blk_cmds;
	u8		index_batch_cmd;
	u8		update_batch_cmd;
	u8		num_batch_cmds;
	DSP_CMD	*batch_cmds;
	int		num_waiters;	// number of tasks waiting on the port
	struct semaphore sem;
	struct UDEC_DEC_BFIFO_CMDtag *last_udec_dec_bfifo_cmd[DSP_MAX_UDEC];
	u32		timestamp;
} dsp_cmd_port_t;

typedef struct dsp_obj_s {
	DSP_INIT_DATA	*dsp_init_data;
	DSP_INIT_DATA	*dsp_init_data_pm;

	int		resyncing;
	int		irq_handler_installed;
	void		*irq_dev;

	DSP_CMD		*default_cmd_queue;
	u32		max_num_default_cmds;
	u32		num_default_cmds;

	dsp_cmd_port_t	*curr_cmd_port;
	int		cmd_mode;

	unsigned long	max_isr_ticks;
	unsigned long	min_isr_interval;
	unsigned long	max_isr_interval;
	unsigned long	last_tick;

	int		num_msgseq_error;
	int		last_msgseq;

	unsigned long	vcap_counter;
	unsigned long	irq_counter;
	unsigned long	sync_counter;

	DSP_OP_MODE	dsp_op_mode;
//	unsigned char			is_dsp_booted;
//	unsigned char			not_first_mode_switch_from_init_mode;
//	unsigned char			reserved1, reserved2;

	int			idsp_reset_flag;

	int		dsp_buffer_space_id;
	u32		free_space_id_bitmap;
	dsp_vm_space_t	mode_vms;

	spinlock_t	lock;
	int		num_waiters;
	struct semaphore sem;

	int		num_syncc_waiters;
	struct semaphore syncc_sem;

//	remove SYNCC related
//	struct semaphore syncc_sem;

	u8		debug_module;
	u8		setup_debug_level : 1;
	u8		setup_debug_thread : 1;
	u8		reserved1 : 6;
	u8		reserved2;
	u8		debug_add_or_set_all;
	u32		debug_level_mask_all;
	u8		debug_add_or_set[DEBUG_MODULE_NUM];
	u32		debug_level_mask[DEBUG_MODULE_NUM];
	u32		debug_thread_mask;

	u32		pair_cmd_code;
	u32		pair_callback_id;
	int		pair_error_code;
	struct semaphore pair_sem;	// used by paired cmd/msg

	union {
		MEMM_CONFIG_FRM_BUF_POOL_MSG memm_config_frm_buf_pool_msg;
		MEMM_QUERY_DSP_SPACE_SIZE_MSG memm_query_dsp_space_size_msg;
		MEMM_SET_DSP_DRAM_SPACE_MSG memm_set_dsp_dram_space_msg;
		MEMM_RESET_DSP_DRAM_SPACE_MSG memm_reset_dsp_dram_space_msg;
		MEMM_CREATE_FRM_BUF_POOL_MSG memm_create_frm_buf_pool_msg;
		MEMM_GET_FRM_BUF_POOL_INFO_MSG memm_get_frm_buf_pool_info_msg;
		MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSG memm_update_frm_buf_pool_config_msg;
		MEMM_REQ_FRM_BUF_MSG memm_req_frm_buf_msg;
	};

	dsp_cmd_port_t *vcap_cmd_port;	//DSP_CMD_PORT_GEN or DSP_CMD_PORT_VCAP

	dsp_cmd_port_t	cmd_ports[NUM_DSP_CMD_PORT];

} dsp_obj_t;


//#define CONFIG_PRINT_DSP_CMD

#ifdef CONFIG_PRINT_DSP_CMD
#include "print_cmd.c"
#define PRINT_CMD(_cmd, _size, _default) \
	print_cmd(_cmd, _size, _default)
#else
#define PRINT_CMD(_cmd, _size, _default)
#endif


int dsp_printk_init(u8 *code_addr, u8 *memd_addr, u8 *print_buffer, u32 buffer_size);
int dsp_printk_work(void);

#endif


