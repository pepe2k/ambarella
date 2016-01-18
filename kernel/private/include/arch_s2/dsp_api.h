/*
 * dsp_api.h
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
 *	2012/05/16 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef _DSP_API_H
#define _DSP_API_H

#define DSP_MIN_SPACE_ID	4
#define DSP_MAX_SPACE_ID	15
#define DSP_SPACE_ID_SHIFT	28

#define SPACE_ID_VALID(_space_id)	((_space_id) >= 4 || (_space_id) <= 15)

#define DSP_SPACE_ID(_addr)		((u32)(_addr) >> DSP_SPACE_ID_SHIFT)
#define IS_PHYS_ADDR(_addr)		(DSP_SPACE_ID(_addr) < DSP_MIN_SPACE_ID)
#define DSP_SPACE_OFFSET(_addr)		((u32)(_addr) & ((1 << DSP_SPACE_ID_SHIFT) - 1))

/*
 * Bits info message memory layout :
 *
 *  |---------------------------------------------------|
 *  |        Bits Info Message FIFO (N * 64 bytes)
 *  |---------------------------------------------------|
 *
 */

typedef struct dsp_bits_info_s
{
	uint32_t	channel_id : 8;
	uint32_t	stream_type : 8;
	uint32_t	reserved0 : 16;

	uint32_t	frmNo;		//Frame Number 32 bits
	uint64_t	pts_64;		//PTS counter 64 bits
	uint32_t	start_addr;	//Start Address	32 bits

	uint32_t	frameTy : 3;	//frame type, use FRM_TYPE. It is always 0 for Audio bitstream. 0-don't care
	uint32_t	levelIDC : 3;	//level in HierB. ex, for M=4, I0/P4 is L0, B2 is L1, B1/B3 is L2.
	uint32_t	refIDC : 1;	//the picture is used as reference or not.
	uint32_t	picStruct : 1;	//0: frame, 1: field
	uint32_t	length : 24;	//Picture Size  24 bits

	uint32_t	top_field_first : 1;
	uint32_t	repeat_first_field : 1;
	uint32_t	progressive_sequence : 1;
	uint32_t	pts_minus_dts_in_pic : 5;	 //(pts-dts) in frame or field unit. (if progressive_sequence=0, in field unit. If progressive_sequence=1, in frame unit.)
	uint32_t	jpeg_qlevel : 8;
	uint32_t	addr_offset_sliceheader : 16;	//slice header address offset in

	int32_t		cpb_fullness;	//cpb fullness with considering real cabac/cavlc bits. use sign int, such that negative value indicate cpb underflow.
	uint32_t	reserved2[8];	//reserved to form 64 Bytes
} dsp_bits_info_t;   //64 Bytes

/* extended bits info, ARM generate to compensate for DSP */
typedef struct dsp_extend_bits_info_s
{
	u64	dsp_pts;
	u64	monotonic_pts;
	u16	new_stream;
	u16	old_stream;
	u8	new_bits_type;
	u8	old_bits_type;
	u16	state;
	u32	new_frame_size;
	u32	old_frame_size;
	struct list_head valid;		// list in the valid frame queue
	struct list_head type;		// list in the type queue (H264 queue and MJPEG queue)
	struct list_head frame;	// list in the stream queue
} dsp_extend_bits_info_t;

typedef struct dsp_vm_space_s {
	unsigned long size;	// in
	int space_id;		// out - space id
	int nr_pages;		// out - number of pages
	unsigned long *pfn;	// out - page frame numbers
} dsp_vm_space_t;

extern int G_use_syncc;
extern int G_enable_vm;

/*
 * Bits partition message memory layout :
 *
 * The first 4 bytes are reserved for a bits patition counter, which is a 32
 * bit counter that increased by 1 each time a new element is written to the
 * bits partition FIFO. The counter wraps around when it reaches (2^32 - 1)
 * so it is ARM's responsibility to take that into consideration. The remaining
 * part of the buffer is used for storing the bits partition message.
 *
 *  |--------------------------------------------------------------------|
 *  |Bits Partition Counter (4 bytes) |     Bits partition Message FIFO (N * 32 bytes)
 *  |--------------------------------------------------------------------|
 *
 */

typedef struct dsp_bits_partition_info_s
{
	uint32_t	channel_id : 8;
	uint32_t	stream_type : 8;
	uint32_t	reserved0 : 16;

	uint32_t	frmNo;		// Frame Number 32 bits
	uint64_t	pts_64;		// PTS counter 64 bits
	uint32_t	start_addr;	// Start Address 32 bits

	uint32_t	frameTy : 3;	// Frame type, it is always 0 for audio bitstream, 0-don't care
	uint32_t	levelIDC : 3;	// level in HierB, ex, for M=4, I0/P4 is L0, B2 is L1, B1/B3 is L2.
	uint32_t	refIDC : 1;	// the picture is used as reference or not
	uint32_t	picStruct : 1;	// 0: frame, 1: field
	uint32_t	length : 24;	// Picture Size 24 bits

	// It indicates the total number of MB rows in the picture
	uint32_t	partition_count : 16;
	// It indicates the number of MB rows encoded
	uint32_t	partition_index : 16;

	uint32_t	reserved;	// Used to make it 32 Bytes aligned
} dsp_bits_partition_info_t; // 32 Bytes

typedef struct dsp_enc_stat_info_s
{
	uint32_t	channel_id  : 8;
	uint32_t	stream_type : 8;
	uint32_t	reserved_0  :16;
	uint32_t	frmNo;			// counter of the encode statistics
	uint64_t	pts_64;			// PTS counter 64 bits
	uint32_t	mv_start_addr;	// ref0 16x16 motion vectors start address in DRAM
	uint32_t	qp_hist_daddr;
	uint32_t	reserved_1[10];	// used to make it 64 bytes aligned
} dsp_enc_stat_info_t;	// 64 bytes

typedef struct dsp_extend_enc_stat_info_s
{
	u64	mono_pts;
	u8	state;
	u8	reserved[3];
	u16	new_stream;
	u16	old_stream;
	u32	new_frame_num;
	u32	old_frame_num;
	struct list_head valid;		// list in the valid queue
	struct list_head frame;	// list in the stream queue
} dsp_extend_enc_stat_info_t;

#define NUM_MSG_CAT	16
// log of dsp cmd/msg
#ifdef CONFIG_LOG_DSP_CMDMSG

void dsp_clear_cmd_msg_log(void);
void dsp_save_cmd_msg(void *addr, size_t size, int type, int port);

#define dsp_save_cmd(_addr, _size, _port)		dsp_save_cmd_msg(_addr, _size, 0x001, _port)
#define dsp_save_cmd_sync(_addr, _size, _port)		dsp_save_cmd_msg(_addr, _size, 0x101, _port)

#define dsp_save_msg(_addr, _size, _port)		dsp_save_cmd_msg(_addr, _size, _port == 2 ? 0x102 : 0x002, _port)

#else

#define dsp_clear_cmd_msg_log()
#define dsp_save_cmd_msg(_addr, _size, _type, _port)

#define dsp_save_cmd(_addr, _size, _port)
#define dsp_save_cmd_sync(_addr, _size, _port)

#define dsp_save_msg(_addr, _size, _port)

#endif
// dsp_api.c

unsigned long __dsp_lock(void);

#define dsp_lock(_flags) \
	do { _flags = __dsp_lock(); } while (0)

void dsp_unlock(unsigned long flags);

typedef void (*dsp_cat_msg_handler)(void *context, unsigned int cat, DSP_MSG *msg, int port);
typedef void (*dsp_enc_handler)(void *context);

int dsp_set_cat_msg_handler(dsp_cat_msg_handler handler, unsigned cat, void *context);
int dsp_set_enc_handler(dsp_enc_handler handler, void * context);

void dsp_issue_udec_dec_cmd(UDEC_DEC_FIFO_CMD *dsp_cmd, int size);

int dsp_init_irq(void *dev);
int dsp_release_irq(void);

int dsp_enable_irq(void);
int dsp_disable_irq(void);

int dsp_wait_irq_interruptible(void);
void dsp_notify_waiters(void);
int dsp_can_switch(void);

int dsp_init(void);
u32 dsp_get_chip_id(void);
int dsp_hot_init(void);
int dsp_resync(void);

int dsp_suspend(void);
int dsp_system_event(unsigned long val);

u32 dsp_get_obj_addr(void);

int __dsp_set_mode(DSP_OP_MODE mode, const char *desc, void (*init_mode)(void*), void *context);
#define dsp_set_mode(_mode, _init_mode, _context) \
	__dsp_set_mode(_mode, #_mode, _init_mode, _context)

void dsp_activate_mode(void);

void dsp_start_change_mode(DSP_OP_MODE mode);
void dsp_end_change_mode(void);

void dsp_set_debug_level(u32 module, u32 add_or_set, u32 level_mask);
void dsp_set_debug_thread(u32 thread_mask);

const struct ucode_load_info_s *dsp_get_ucode_info(void);
unsigned long dsp_get_ucode_start(void);
unsigned long dsp_get_ucode_size(void);
const struct ucode_version_s *dsp_get_ucode_version(void);

void dsp_issue_cmd(void *cmd, u32 size);
int dsp_is_cmdq_full(u32 additional_cmds);

void dsp_start_default_cmd(void);
void dsp_end_default_cmd(void);

void dsp_start_cmdblk(u32 port);
void dsp_end_cmdblk(u32 port);

void dsp_start_enc_batch_cmd(void);
void dsp_end_enc_batch_cmd(void);

void dsp_start_sync_cmd(void);
void dsp_end_sync_cmd(void);
void dsp_issue_cmd_sync(void *cmd, u32 size);

void dsp_issue_cmd_sync_ex(void *cmd, u32 size);

void dsp_enable_vcap_cmd_port(int enable);

// make sure cmd_code == callback_id
void __dsp_issue_cmd_pair_msg(void *cmd, u32 size, int sync_port);
void print_cmd(void *cmd, u32 size, int default_cmd);


int dsp_check_status(void);

void *dsp_vcap_irq(void);
int dsp_boot(DSP_OP_MODE op_mode);

void dsp_reset(void);

DSP_OP_MODE dsp_get_op_mode(void);

void dsp_reset_idsp(void);

//dsp_mem.c
void dsp_mm_config_frm_buf_pool(MEMM_CONFIG_FRM_BUF_POOL_CMD *dsp_cmd, MEMM_CONFIG_FRM_BUF_POOL_MSG *dsp_msg);
void dsp_mm_update_frm_buf_pool_config(MEMM_UPDATE_FRM_BUF_POOL_CONFG_CMD *dsp_cmd, MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSG *dsp_msg);
void dsp_mm_request_fb(MEMM_REQ_FRM_BUF_CMD *dsp_cmd, MEMM_REQ_FRM_BUF_MSG *dsp_msg);
void dsp_mm_register_fb(u16 fb_id);
void dsp_mm_release_fb(u16 fb_id);

int dsp_alloc_vm_space(dsp_vm_space_t *vms);
void dsp_free_vm_space(dsp_vm_space_t *vms);

int dsp_alloc_mode_vms(unsigned long size);
dsp_vm_space_t *dsp_get_mode_vms(void);
int dsp_get_mode_space_id(void);

int dsp_mm_set_dram_space(dsp_vm_space_t *vms);
int dsp_mm_reset_dram_space(dsp_vm_space_t *vms);

int __dsp_map_vms(struct vm_area_struct *vma, dsp_vm_space_t *vms, unsigned long addr, unsigned long dsp_virt_addr, unsigned long size, pgprot_t prot);

#endif	// _DSP_API_H

