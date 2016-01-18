/*
 * dsp_api.c
 *
 * History:
 *	2011/07/25 - [Louis Sun] ported from iOne for S2 IPCAM
 *	2012/08/30 - [Jian Tang] modified for S2
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#include <amba_common.h>

#include "dsp_cmd.h"
#include "iav_common.h"
#include "dsp_api.h"
#include "dsp_priv.h"
#include "iav_drv.h"
#include "utils.h"

//#define REMOVE_SHARE_MEM

#define MAX_DSP_CMD_LOG_SIZE	(256*KB)
#define MAX_BATCH_CMD_NUM	(IAV_STREAM_MAX_NUM_IMPL * MAX_BATCH_CMD_NUM_PER_STREAM)

//#define CONFIG_DSP_ENABLE_L2_ON_OFF
//#define CONFIG_DSP_ENABLE_HIGH_IRQ_LEVEL
//#define CONFIG_DSP_ENABLE_TIMER
//#define CONFIG_DSP_USE_VIRTUAL_MEMORY

//debug only
#if 0
extern unsigned int g_decode_cmd_cnt[16];//ugly here, fix me
extern unsigned int g_decode_frame_cnt[16];
#endif

//#define DEBUG_PRINT_FRAME_NUMBER
#ifdef DEBUG_PRINT_FRAME_NUMBER
static int g_has_decode_cmd = 0;
#endif

static const ucode_load_info_t dsp_ucode_info = {
	.map_size	= DSP_UCODE_SIZE,
	.nr_item	= 4,
	.items 		= {
		[0] = {
			.addr_offset	= DSP_CODE_MEMORY_OFFSET,
			.filename	= "orccode.bin",
		},
		[1] = {
			.addr_offset	= DSP_MEMD_MEMORY_OFFSET,
			.filename	= "orcme.bin",
		},
		[2] = {
			.addr_offset	= DSP_MDXF_MEMORY_OFFSET,
			.filename	= "orcmdxf.bin",
		},
		[3] = {
			.addr_offset	= DSP_BINARY_DATA_MEMORY_OFFSET,
			.filename	= "default_binary.bin",
		},
	}
};

static dsp_cat_msg_handler g_cat_msg_handler[NUM_MSG_CAT];
static void *g_cat_context[NUM_MSG_CAT];

static dsp_enc_handler g_enc_handler;
static void *g_enc_context = NULL;

static dsp_obj_t *G_dsp_obj = NULL;

u8 *G_dramc_base;
int G_use_syncc = 1;
int G_enable_vm = 0;
EXPORT_SYMBOL(G_enable_vm);

static void wait_dsp_mode(DSP_OP_MODE mode, const char *msg);
static irqreturn_t vdsp_irq(int irqno, void *dev_id);
static irqreturn_t vcap_irq(int irqno, void *dev_id);
static irqreturn_t venc_irq(int irqno, void *dev_id);
//static irqreturn_t sync_irq(int irqno, void *dev_id);

#include "dsp_mem.c"

static void *dsp_zalloc(size_t size)
{
	void *ptr = kzalloc(size, GFP_KERNEL);
	if (ptr == NULL) {
		DRV_PRINT("Out of memory in dsp_zalloc !\n");
		return NULL;
	}
	return ptr;
}

static inline void clear_udec_dec_info(dsp_cmd_port_t *port)
{
	int i;
	for (i = 0; i < DSP_MAX_UDEC; i++) {
		port->last_udec_dec_bfifo_cmd[i] = NULL;
	}
}

static int init_cmd_port(u32 port_num)
{
	DSP_HEADER_CMD *header_cmd;
	DSP_STATUS_MSG *status_msg;
	dsp_cmd_port_t *port;
	size_t size;

	port = G_dsp_obj->cmd_ports + port_num;
	// cmd queue
	port->max_num_cmds = MAX_NUM_CMD;
	size = MAX_NUM_CMD * DSP_CMD_SIZE;
	if ((port->cmd_queue = dsp_zalloc(size)) == NULL) {
		goto EXIT_INIT_CMD_PORT;
	}

	port->prev_cmd_seq_num = 0;
	port->update_cmd_seq_num = 1;
	port->num_cmds = 0;
	port->num_udec_cmds = 0;
	clear_udec_dec_info(port);

	// msg queue
	port->max_num_msgs = MAX_NUM_MSG;
	size = MAX_NUM_MSG * DSP_MSG_SIZE;
	if ((port->msg_queue = dsp_zalloc(size)) == NULL) {
		goto EXIT_INIT_CMD_PORT;
	}

	// block cmd queue
	port->cmd_type = DSP_CMD_TYPE_NORMAL;
	port->num_blk_cmds = 0;
	port->num_waiters = 0;
	sema_init(&port->sem, 0);

	// batch encode cmd queue for GEN port only
	if (DSP_CMD_PORT_GEN == port_num) {
		size = MAX_BATCH_CMD_CHUNK * MAX_BATCH_CMD_NUM * DSP_CMD_SIZE;
		if ((port->batch_cmds = dsp_zalloc(size)) == NULL) {
			goto EXIT_INIT_CMD_PORT;
		}
		port->update_batch_cmd = 0;
		port->index_batch_cmd = 0;
	}

	// header cmd
	header_cmd = (void*)port->cmd_queue;
	header_cmd->cmd_code = CMD_DSP_HEADER;
	//header_cmd->cmd_seq_num = 1;
	header_cmd->num_cmds = 0;
	clean_d_cache(header_cmd, DSP_CMD_SIZE);

	// status msg
	status_msg = (void*)port->msg_queue;
	status_msg->dsp_mode = DSP_OP_MODE_INIT;
	clean_d_cache(status_msg, DSP_MSG_SIZE);

	return 0;

EXIT_INIT_CMD_PORT:
	if (port->batch_cmds) {
		kfree(port->batch_cmds);
		port->batch_cmds = NULL;
	}
	if (port->msg_queue) {
		kfree(port->msg_queue);
		port->msg_queue = NULL;
	}
	if (port->cmd_queue) {
		kfree(port->cmd_queue);
		port->cmd_queue = NULL;
	}

	return -ENOMEM;
}

unsigned long __dsp_lock(void)
{
	unsigned long flags;
	spin_lock_irqsave(&G_dsp_obj->lock, flags);
	return flags;
}
EXPORT_SYMBOL(__dsp_lock);

void dsp_unlock(unsigned long flags)
{
	spin_unlock_irqrestore(&G_dsp_obj->lock, flags);
}
EXPORT_SYMBOL(dsp_unlock);

int dsp_check_status(void)
{
	if (amba_readl(DSP_DRAM_MAIN_REG) &&
		amba_readl(DSP_DRAM_SUB0_REG) &&
		amba_readl(DSP_DRAM_SUB1_REG))
		return 1;

	return 0;
}
EXPORT_SYMBOL(dsp_check_status);

int dsp_set_cat_msg_handler(dsp_cat_msg_handler handler, unsigned cat, void *context)
{
	dsp_printk("dsp_set_cat_msg_handler, cat [%d].\n", cat);

	if (cat >= ARRAY_SIZE(g_cat_msg_handler))
		return -EINVAL;
	g_cat_msg_handler[cat] = handler;
	g_cat_context[cat] = context;
	return 0;
}
EXPORT_SYMBOL(dsp_set_cat_msg_handler);

int dsp_set_enc_handler(dsp_enc_handler handler, void *context)
{
	dsp_printk("dsp_set_enc_handler\n");
	g_enc_handler = handler;
	g_enc_context = context;
	return 0;
}
EXPORT_SYMBOL(dsp_set_enc_handler);

#if 0
#if defined(CONFIG_DSP_ENABLE_HIGH_IRQ_LEVEL) && defined(CONFIG_ARM_GIC)
void ambarella_set_irq_level(u8 level, u32 irq)
{
	u32 address = AMBARELLA_VA_GIC_DIST_BASE + 0x400 + irq;

	__raw_writeb(level, __io(address));
}
#endif


void dsp_init_high_irq(void)
{
#if defined(CONFIG_DSP_ENABLE_HIGH_IRQ_LEVEL) && defined(CONFIG_ARM_GIC)
	ambarella_set_irq_level(0x00, CODING_ORC0_IRQ);
	ambarella_set_irq_level(0x00, CODING_ORC1_IRQ);
	ambarella_set_irq_level(0x00, CODING_ORC2_IRQ);
	ambarella_set_irq_level(0x00, CODING_ORC3_IRQ);
	ambarella_set_irq_level(0x10, VOUT_IRQ);
	ambarella_set_irq_level(0x10, ORC_VOUT0_IRQ);
#endif
}
#endif

static struct {
	int irqno;
	irqreturn_t (*handler)(int irqno, void *dev_id);
	const char *desc;
} dsp_irqs[] = {
	{CODE_VDSP_0_IRQ, vdsp_irq, "vdsp"},
	{CODE_VDSP_1_IRQ, vcap_irq, "vcap"},
	{CODE_VDSP_2_IRQ, venc_irq, "venc"},
	//{CODING_ORC2_IRQ, sync_irq, "syncc"},
};


int dsp_init_irq(void *dev)
{
	int rval;
	int i;

	if (G_dsp_obj->irq_handler_installed)
		return 0;

	//dsp_init_high_irq();

	for (i = 0; i < ARRAY_SIZE(dsp_irqs); i++) {
		//A7 old IAV is using FALLING Edge for vDSP IRQ
		rval = request_irq(dsp_irqs[i].irqno, dsp_irqs[i].handler,
			IRQF_TRIGGER_RISING | IRQF_DISABLED, dsp_irqs[i].desc, dev);
		if (rval < 0) {
			DRV_PRINT("Cannot request %s irq\n", dsp_irqs[i].desc);
			return rval;
		}
	}

	G_dsp_obj->irq_dev = dev;
	G_dsp_obj->irq_handler_installed = 1;

	return 0;
}
EXPORT_SYMBOL(dsp_init_irq);

int dsp_release_irq(void)
{
	int i;

	if (G_dsp_obj->irq_handler_installed) {
		for (i = 0; i < ARRAY_SIZE(dsp_irqs); i++) {
			free_irq(dsp_irqs[i].irqno, G_dsp_obj->irq_dev);
		}
		G_dsp_obj->irq_handler_installed = 0;
	}

	return 0;
}
EXPORT_SYMBOL(dsp_release_irq);

int dsp_enable_irq(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dsp_irqs); i++) {
		enable_irq(dsp_irqs[i].irqno);
	}

	return 0;
}
EXPORT_SYMBOL(dsp_enable_irq);

int dsp_disable_irq(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dsp_irqs); i++) {
		disable_irq(dsp_irqs[i].irqno);
	}

	return 0;
}
EXPORT_SYMBOL(dsp_disable_irq);


#if 0
#define write_dramc_reg(_offset, _value) \
	do { \
		volatile u32 *__addr = (u32*)(G_dramc_base + (_offset)); \
		*__addr = _value; \
	} while (0)
#endif

int dsp_init(void)
{
	DSP_INIT_DATA *dsp_init_data;
	dsp_cmd_port_t *port;
	vdsp_info_t *vdsp_info;
	u32 * chip_id;
//	u8 *dsp_log;
//	u8 *dsp_log_phys;
	int rval;

	// the DSP object
	if ((G_dsp_obj = dsp_zalloc(sizeof(*G_dsp_obj))) == NULL)
		return -ENOMEM;

	// DSP_INIT_DATA
	dsp_init_data = (DSP_INIT_DATA*)DSP_INIT_DATA_BASE;
	G_dsp_obj->dsp_init_data = dsp_init_data;

	G_dsp_obj->dsp_init_data_pm = (DSP_INIT_DATA*)kzalloc(
		sizeof(DSP_INIT_DATA), GFP_KERNEL);
	if (G_dsp_obj->dsp_init_data_pm == NULL)
		return -ENOMEM;

	// cmd/msg ports
	if ((rval = init_cmd_port(DSP_CMD_PORT_GEN)) < 0)
		return rval;
	if ((rval = init_cmd_port(DSP_CMD_PORT_VCAP)) < 0)
		return rval;
	if ((rval = init_cmd_port(DSP_CMD_PORT_SYNCC)) < 0)
		return rval;
	if ((rval = init_cmd_port(DSP_CMD_PORT_AUX)) < 0)
		return rval;

	memset(dsp_init_data, 0, sizeof(*dsp_init_data));

	// default binary data
	dsp_init_data->default_binary_data_addr = (u32)PHYS_TO_DSP(DSP_BINARY_DATA_START);
	dsp_init_data->default_binary_data_size = 512*KB;

	/* initialize GEN port and VCAP port */
	// General purpose cmd/msg port for DSP-ARM
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;
	dsp_init_data->cmd_data_gen_daddr = (u32)VIRT_TO_DSP(port->cmd_queue);
	dsp_init_data->cmd_data_gen_size = port->max_num_cmds * DSP_CMD_SIZE;
	dsp_init_data->msg_queue_gen_daddr = (u32)VIRT_TO_DSP(port->msg_queue);
	dsp_init_data->msg_queue_gen_size = port->max_num_msgs * DSP_MSG_SIZE;

	// Secondary cmd/msg port for VCAP-ARM
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_VCAP;
	dsp_init_data->cmd_data_vcap_daddr = (u32)VIRT_TO_DSP(port->cmd_queue);
	dsp_init_data->cmd_data_vcap_size = port->max_num_cmds * DSP_CMD_SIZE;
	dsp_init_data->msg_queue_vcap_daddr = (u32)VIRT_TO_DSP(port->msg_queue);
	dsp_init_data->msg_queue_vcap_size = port->max_num_msgs * DSP_MSG_SIZE;

	// sync counter cmd/msg port
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_SYNCC;
	dsp_init_data->cmd_data_3rd_daddr = (u32)VIRT_TO_DSP(port->cmd_queue);
	dsp_init_data->cmd_data_3rd_size = port->max_num_cmds * DSP_CMD_SIZE;
	dsp_init_data->msg_queue_3rd_daddr = (u32)VIRT_TO_DSP(port->msg_queue);
	dsp_init_data->msg_queue_3rd_size = port->max_num_msgs * DSP_MSG_SIZE;

	// the fourth cmd/msg port
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_AUX;
	dsp_init_data->cmd_data_4th_daddr = (u32)VIRT_TO_DSP(port->cmd_queue);
	dsp_init_data->cmd_data_4th_size = port->max_num_cmds * DSP_CMD_SIZE;
	dsp_init_data->msg_queue_4th_daddr = (u32)VIRT_TO_DSP(port->msg_queue);
	dsp_init_data->msg_queue_4th_size = port->max_num_msgs * DSP_MSG_SIZE;

	// default cmd queue
	if ((G_dsp_obj->default_cmd_queue = dsp_zalloc(MAX_DEFAULT_CMD * DSP_CMD_SIZE)) == NULL)
		return -ENOMEM;
	G_dsp_obj->max_num_default_cmds = MAX_DEFAULT_CMD;
	G_dsp_obj->num_default_cmds = 0;

	dsp_init_data->default_config_daddr = (u32)VIRT_TO_DSP(G_dsp_obj->default_cmd_queue);
	dsp_init_data->default_config_size = MAX_DEFAULT_CMD * DSP_CMD_SIZE;

	// dsp buffer
	dsp_init_data->dsp_buffer_daddr = (u32)PHYS_TO_DSP(DSP_DRAM_START);
	dsp_init_data->dsp_buffer_size = DSP_BUFFER_SIZE;

	// dsp log
//	dsp_printk_init((u8*)DSP_DRAM_CODE_BASE, (u8*)DSP_DRAM_MEMD_BASE,
//		(u8*)DSP_LOG_AREA, DSP_LOG_SIZE);

	//dsp info
	if ((vdsp_info = dsp_zalloc(sizeof(*vdsp_info))) == NULL)
		return -ENOMEM;
	dsp_init_data->vdsp_info_ptr = (u32 *)vdsp_info;

	if ((chip_id = dsp_zalloc(sizeof(u32))) == NULL)
		return -ENOMEM;

	//chip id
	dsp_init_data->chip_id_ptr = (u32 *)VIRT_TO_DSP(chip_id);

	// dsp obj
	G_dsp_obj->curr_cmd_port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;
	G_dsp_obj->vcap_cmd_port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;
	G_dsp_obj->cmd_mode = DSP_CMD_MODE_DEFAULT;
	spin_lock_init(&G_dsp_obj->lock);

	G_dsp_obj->dsp_op_mode = DSP_OP_MODE_INIT;

	G_dsp_obj->num_waiters = 0;
	sema_init(&G_dsp_obj->sem, 0);

	G_dsp_obj->num_syncc_waiters = 0;
	sema_init(&G_dsp_obj->syncc_sem, 0);
	sema_init(&G_dsp_obj->pair_sem, 0);

	dsp_set_cat_msg_handler(handle_memm_msg, CAT_MEMM, NULL);

	return 0;
}
EXPORT_SYMBOL(dsp_init);

u32 dsp_get_chip_id(void)
{
	u32 * chip_id = (u32 *)DSP_TO_VIRT(G_dsp_obj->dsp_init_data->chip_id_ptr);
	invalidate_d_cache(chip_id, sizeof(u32));
	return *chip_id;
}
EXPORT_SYMBOL(dsp_get_chip_id);

#if 0
int dsp_hot_init(void)
{
	DSP_INIT_DATA *dsp_init_data;
	dsp_cmd_port_t *port;
	u8 *code_addr;
	u8 *memd_addr;
	u8 *dsp_log;

	dsp_init_data = (DSP_INIT_DATA*)DSP_INIT_DATA_BASE;
	invalidate_d_cache((void*)dsp_init_data, sizeof(DSP_INIT_DATA));

	// General purpose cmd/msg port for DSP-ARM
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;
	port->cmd_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->cmd_data_gen_daddr);
	port->max_num_cmds = dsp_init_data->cmd_data_gen_size / DSP_CMD_SIZE;
	port->msg_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->msg_queue_gen_daddr);
	port->max_num_msgs = dsp_init_data->msg_queue_gen_size / DSP_MSG_SIZE;

	// Secondary cmd/msg port for VCAP-ARM
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_VCAP;
	port->cmd_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->cmd_data_vcap_daddr);
	port->max_num_cmds = dsp_init_data->cmd_data_vcap_size / DSP_CMD_SIZE;
	port->msg_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->msg_queue_vcap_daddr);
	port->max_num_msgs = dsp_init_data->msg_queue_vcap_size / DSP_MSG_SIZE;

	// sync counter cmd/msg port
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_SYNCC;
	port->cmd_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->cmd_data_3rd_daddr);
	port->max_num_cmds = dsp_init_data->cmd_data_3rd_size / DSP_CMD_SIZE;
	port->msg_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->msg_queue_3rd_daddr);
	port->max_num_msgs = dsp_init_data->msg_queue_3rd_size / DSP_MSG_SIZE;

	// the fourth cmd/msg port
	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_AUX;
	port->cmd_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->cmd_data_4th_daddr);
	port->max_num_cmds = dsp_init_data->cmd_data_4th_size / DSP_CMD_SIZE;
	port->msg_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->msg_queue_4th_daddr);
	port->max_num_msgs = dsp_init_data->msg_queue_4th_size / DSP_MSG_SIZE;

	// default cmd queue
	G_dsp_obj->default_cmd_queue = (void*)DSP_TO_AMBVIRT(dsp_init_data->default_config_daddr);
	G_dsp_obj->max_num_default_cmds = dsp_init_data->default_config_size / DSP_CMD_SIZE;
	G_dsp_obj->num_default_cmds = 0;

	// log buffer
	code_addr = (void*)amba_readl(DSP_DRAM_MAIN_REG);
	code_addr = (void*)DSP_TO_AMBVIRT(code_addr);
	memd_addr = (void*)amba_readl(DSP_DRAM_SUB0_REG);
	memd_addr = (void*)DSP_TO_AMBVIRT(memd_addr);
	dsp_log = (void*)DSP_TO_AMBVIRT(dsp_init_data->dsp_debug_daddr);
	dsp_printk_init(code_addr, memd_addr, dsp_log, dsp_init_data->dsp_debug_size);

	G_dsp_obj->resyncing = 1;

	return 0;
}
#else


int dsp_hot_init(void)
{
	dsp_printk("dsp hot init \n");

	return -1;
}
EXPORT_SYMBOL(dsp_hot_init);
#endif

int dsp_resync(void)
{
#if 0
	dsp_cmd_port_t *port;
	unsigned long flags;

	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;
	dsp_lock(flags);
	port->num_waiters++;
	dsp_unlock(flags);
	down(&port->sem);

	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_VCAP;
	port->msg_seq_num = ((DSP_STATUS_MSG*)port->msg_queue)->msg_seq_num;

	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_SYNCC;
	port->msg_seq_num = ((DSP_STATUS_MSG*)port->msg_queue)->msg_seq_num;

	G_dsp_obj->resyncing = 0;
#else
	dsp_printk("dsp resync \n");
#endif

	return 0;
}


u32 dsp_get_obj_addr(void)
{
	return (u32)G_dsp_obj;
}
EXPORT_SYMBOL(dsp_get_obj_addr);

static inline void flush_init_data(void)
{
	clean_d_cache(G_dsp_obj->dsp_init_data, sizeof(DSP_INIT_DATA));
}

static void flush_default_cmdq(void)
{
	DSP_HEADER_CMD *header = (void*)G_dsp_obj->default_cmd_queue;
	header->cmd_code = CMD_DSP_HEADER;
	header->cmd_seq_num = 1;
	header->num_cmds = G_dsp_obj->num_default_cmds;
	clean_d_cache(G_dsp_obj->default_cmd_queue,
		(G_dsp_obj->num_default_cmds + 1) * DSP_CMD_SIZE);
}


/* ======================= Debug ============================ */
static void set_debug_level_and_thread(void)
{
	u8 module;
	if (G_dsp_obj->setup_debug_level) {
		DSP_SET_DEBUG_LEVEL_CMD dsp_cmd;
		memset(&dsp_cmd, 0, sizeof(dsp_cmd));
		module = G_dsp_obj->debug_module;
		dsp_cmd.cmd_code = CMD_DSP_SET_DEBUG_LEVEL;
		dsp_cmd.module = module;
		if (module < DEBUG_MODULE_LAST) {
			dsp_cmd.add_or_set = G_dsp_obj->debug_add_or_set[module];
			dsp_cmd.debug_mask = G_dsp_obj->debug_level_mask[module];
		} else {
			dsp_cmd.add_or_set = G_dsp_obj->debug_add_or_set_all;
			dsp_cmd.debug_mask = G_dsp_obj->debug_level_mask_all;
		}
		G_dsp_obj->setup_debug_level = 0;

              dsp_printk("DEBUG: SET_DEBUG_LEVEL\n");
              dsp_printk("DEBUG: module =%d ,  add_or_set=%d, debug_mask =0x%x\n",
			  dsp_cmd.module, dsp_cmd.add_or_set, dsp_cmd.debug_mask);

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	if (G_dsp_obj->setup_debug_thread) {
		DSP_SET_DEBUG_THREAD_CMD dsp_cmd;
		memset(&dsp_cmd, 0, sizeof(dsp_cmd));
		dsp_cmd.cmd_code = CMD_DSP_SET_DEBUG_THREAD;
		dsp_cmd.thread_mask = G_dsp_obj->debug_thread_mask;
		G_dsp_obj->setup_debug_thread = 0;
              dsp_printk("DEBUG: SET_DEBUG_THREAD\n");
              dsp_printk("DEBUG: thread_mask = 0x%x \n", dsp_cmd.thread_mask);
		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

}

void dsp_set_debug_level(u32 module, u32 add_or_set, u32 mask)
{
	G_dsp_obj->setup_debug_level = 1;
	G_dsp_obj->debug_module = module;
	if (module < DEBUG_MODULE_LAST) {
		G_dsp_obj->debug_add_or_set[module] = add_or_set;
		G_dsp_obj->debug_level_mask[module] = mask;
	} else {
		G_dsp_obj->debug_add_or_set_all = add_or_set;
		G_dsp_obj->debug_level_mask_all = mask;
	}
	dsp_printk("dsp_set_debug_level: dsp op mode = %d \n",
		G_dsp_obj->dsp_op_mode);
	if (G_dsp_obj->dsp_op_mode != DSP_OP_MODE_INIT) {
		// Booted - otherwise the command will be sent when boot
		set_debug_level_and_thread();
	}
}
EXPORT_SYMBOL(dsp_set_debug_level);

void dsp_set_debug_thread(u32 thread_mask)
{
	G_dsp_obj->setup_debug_thread = 1;
	G_dsp_obj->debug_thread_mask = thread_mask;
	if (G_dsp_obj->dsp_op_mode != DSP_OP_MODE_INIT) {
		set_debug_level_and_thread();
	}
}
EXPORT_SYMBOL(dsp_set_debug_thread);

void dsp_reset(void)
{
	dsp_printk("DSP reset IDSP and ORC.\n");
	amba_writel(DSP_ORC_BASE, 0xFF);
	if (G_dsp_obj->idsp_reset_flag == 0) {
		G_dsp_obj->idsp_reset_flag = 1;
		amba_writel(DSP_IDSP_BASE, 0xFF);
	}
}
EXPORT_SYMBOL(dsp_reset);


void dsp_reset_idsp(void)
{
	dsp_printk("DSP reset idsp.\n");
	amba_writel(DSP_IDSP_BASE, 0xFF);
}
EXPORT_SYMBOL(dsp_reset_idsp);

int dsp_boot(DSP_OP_MODE op_mode)
{
	// dsp_cmdq_header_t *cmdq_header;
	BUG_ON((DSP_DRAM_CODE_START & 0x000FFFFF) != 0);
	BUG_ON((DSP_DRAM_MEMD_START & 0x000FFFFF) != 0);
	BUG_ON((DSP_DRAM_MDXF_START & 0x000FFFFF) != 0);

	// disable DSP debug
	{
		u8 i;
		DSP_SET_DEBUG_LEVEL_CMD dsp_cmd;
		DSP_SET_DEBUG_THREAD_CMD dsp_cmd2;

		memset(&dsp_cmd, 0, sizeof(dsp_cmd));
		dsp_cmd.cmd_code = CMD_DSP_SET_DEBUG_LEVEL;
		for (i = DEBUG_MODULE_FIRST; i < DEBUG_PERFORM_MODULE; ++i) {
			dsp_cmd.module = i;
			if (G_dsp_obj->debug_module == DEBUG_ALL_MODULE) {
				dsp_cmd.add_or_set = G_dsp_obj->debug_add_or_set_all;
				dsp_cmd.debug_mask = G_dsp_obj->debug_level_mask_all;
			} else {
				dsp_cmd.add_or_set = G_dsp_obj->debug_add_or_set[i];
				dsp_cmd.debug_mask = G_dsp_obj->debug_level_mask[i];
			}
			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
		}

		memset(&dsp_cmd2, 0, sizeof(dsp_cmd2));
		dsp_cmd2.cmd_code = CMD_DSP_SET_DEBUG_THREAD;
		dsp_cmd2.thread_mask = 0; //enable all debug threads
		dsp_cmd2.reserved1 = 0;
		dsp_cmd2.reserved2 = 0;
		dsp_issue_cmd(&dsp_cmd2, sizeof(dsp_cmd2));
	}

	flush_init_data();
	flush_default_cmdq();

	if ((op_mode == DSP_OP_MODE_CAMERA_RECORD) ||
		(op_mode == DSP_OP_MODE_IP_CAMERA_RECORD)) {
		G_dsp_obj->idsp_reset_flag = 1;
		amba_writel(DSP_IDSP_BASE, 0xFF);
		dsp_printk("Reset IDSP in DSP boot.\n");
	}

	// set dsp code/memd/mdxf address
	amba_writel(DSP_DRAM_MAIN_REG, PHYS_TO_DSP(DSP_DRAM_CODE_START));
	amba_writel(DSP_DRAM_SUB0_REG, PHYS_TO_DSP(DSP_DRAM_MEMD_START));
	amba_writel(DSP_DRAM_SUB1_REG, PHYS_TO_DSP(DSP_DRAM_MDXF_START));

	// reset dsp
	amba_writel(DSP_CONFIG_MAIN_REG, 0xFF);
	amba_writel(DSP_CONFIG_SUB0_REG, 0xF);
	amba_writel(DSP_CONFIG_SUB1_REG, 0xF);

	G_dsp_obj->cmd_mode = DSP_CMD_MODE_NORMAL;
	dsp_printk("DSP_BOOT in dsp_set_mode to mode %d \n", op_mode);

        //set debug level and trhread during boot
        G_dsp_obj->setup_debug_level = 1;
        G_dsp_obj->setup_debug_thread = 1;
        set_debug_level_and_thread();

	return 0;
}
EXPORT_SYMBOL(dsp_boot);

static inline void dsp_copy_cmd(u8 *ptr, void *cmd, u32 size)
{
	memcpy(ptr, cmd, size);
	memset(ptr + size, 0, DSP_CMD_SIZE - size);
}

#ifdef CONFIG_LOG_DSP_CMDMSG
void dsp_clear_cmd_msg_log(void)
{
	//G_dsp_obj->dsp_cmd_addr = G_dsp_obj->dsp_cmd_base;
	//G_dsp_obj->dsp_cmd_bytes_left = G_dsp_obj->dsp_cmd_bytes_total;
}
void dsp_save_cmd_msg(void *addr, size_t size, int type, int port)
{
#if 0
	int total = size + sizeof(size)*4;
	unsigned long flags = 0;

	if ((type & 0xFF) == 1)
		dsp_lock(flags);

	if (G_dsp_obj->dsp_cmd_bytes_left < total)
		dsp_clear_cmd_msg_log();

	if (G_dsp_obj->dsp_cmd_bytes_left >= total) {
		u32 *ptr = (u32*)G_dsp_obj->dsp_cmd_addr;

		*ptr++ = size | (type << 16);
		*ptr++ = G_dsp_obj->irq_counter;
		*ptr++ = G_dsp_obj->cmd_ports[port].timestamp;
		*ptr++ = G_dsp_obj->cmd_ports[port].msg_seq_num;

		memcpy(ptr, addr, size);

		G_dsp_obj->dsp_cmd_addr += total;
		G_dsp_obj->dsp_cmd_bytes_left -= total;
	}

	if ((type & 0xFF) == 1)
		dsp_unlock(flags);
#endif
}
#endif

static inline int dsp_is_vcap_cmd(void *cmd)
{
	u8 * pcmd = (u8 *)cmd;
	return ((pcmd[3] == CAT_VCAP)||(pcmd[3] == CAT_IDSP));
}

static void issue_default_cmd(void *cmd, u32 size)
{
	u8 * ptr = NULL;
//	dsp_printk("num_default_cmds %d, default_cmd_queue 0x%x \n",
//		G_dsp_obj->num_default_cmds, (u32)G_dsp_obj->default_cmd_queue);

	BUG_ON(G_dsp_obj->num_default_cmds >= G_dsp_obj->max_num_default_cmds - 1);
	G_dsp_obj->num_default_cmds++;
	ptr = (u8*)(G_dsp_obj->default_cmd_queue + G_dsp_obj->num_default_cmds);
	dsp_copy_cmd(ptr, cmd, size);
}

static void issue_normal_cmd(void * cmd, u32 size)
{
	unsigned long flags;
	u8 * ptr = NULL, group_cmds_num = 0;
	dsp_cmd_port_t *port;
	DSP_HEADER_CMD * header = NULL;

	if (dsp_is_vcap_cmd(cmd)) {
		port = G_dsp_obj->vcap_cmd_port;
	} else {
		port = G_dsp_obj->curr_cmd_port;
	}

	/* Wait until cmd Q is ready */
	if (DSP_CMD_TYPE_BLOCK == port->cmd_type) {
		/* Block commands share same buffer Q with normal commands, while
		 * encoder batch commands use another buffer Q per each encoder. */
		group_cmds_num = port->num_blk_cmds;
	}
	while (1) {
		dsp_lock(flags);
		if (likely((port->num_cmds + group_cmds_num +
			port->num_udec_cmds) < ARRAY_SIZE(port->cmd_buffer))) {
			break;
		}
		port->num_waiters++;
		dsp_unlock(flags);
		down(&port->sem);
	}

	if (G_dsp_obj->dsp_op_mode == DSP_OP_MODE_UDEC) {
		/* UDEC mode has one "merge" command need to be cached for
		 * one frame. Save DSP command into the cached memory.
		 */
		ptr = (u8*)(port->cmd_buffer + port->num_udec_cmds);
		dsp_copy_cmd(ptr, cmd, size);
		port->num_udec_cmds++;
	} else {
		/* ENCODE mode */
		switch (port->cmd_type) {
		case DSP_CMD_TYPE_NORMAL:
			/* No need to cache DSP commands in ENC mode.
			 * Save DSP command into the shared memory.
			 */
			ptr = (u8*)(port->cmd_queue + 1 + port->num_cmds);
			dsp_copy_cmd(ptr, cmd, size);
			clean_d_cache(ptr, DSP_CMD_SIZE);
			port->num_cmds++;
			header = (DSP_HEADER_CMD *)port->cmd_queue;
			if (port->update_cmd_seq_num == 1) {
				header->cmd_seq_num++;
				port->update_cmd_seq_num = 0;
			}
			header->num_cmds = port->num_cmds;
			clean_d_cache(header, DSP_CMD_SIZE);
			break;

		case DSP_CMD_TYPE_BLOCK:
			/* Save block command into cached memory and flush them in ISR.
			 */
			ptr = (u8*)(port->cmd_blk + port->num_blk_cmds);
			dsp_copy_cmd(ptr, cmd, size);
			port->num_blk_cmds++;
			break;

		case DSP_CMD_TYPE_BATCH:
			/* Save batch command into cached memory */
			ptr = (u8*)(port->batch_cmds + port->index_batch_cmd *
				MAX_BATCH_CMD_NUM + port->num_batch_cmds);
			dsp_copy_cmd(ptr, cmd, size);
			port->num_batch_cmds++;
			break;

		default:
			break;
		}
	}

	dsp_unlock(flags);
}

static void issue_syncc_cmd(void *cmd, u32 size)
{
	unsigned long flags;
	u8 * ptr = NULL;
	dsp_cmd_port_t *port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_SYNCC;
	dsp_lock(flags);
	BUG_ON(port->num_cmds >= port->max_num_cmds - 1);
	ptr = (u8*)(port->cmd_buffer + port->num_cmds);
	dsp_copy_cmd(ptr, cmd, size);
	port->num_cmds++;
	dsp_unlock(flags);
}

static void issue_enc_batch_cmd(void)
{
	DSP_CMD * addr = NULL;
	int cmd_index;
	ENCODER_BATCH_CMD dsp_cmd;
	dsp_cmd_port_t * port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_GEN;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_H264ENC_BATCH_CMD;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = STRM_TP_ENC_0;
	dsp_cmd.cmd_buf_num = port->num_batch_cmds;

	cmd_index = port->index_batch_cmd;
	addr = &port->batch_cmds[cmd_index * MAX_BATCH_CMD_NUM];
	dsp_cmd.cmd_buf_addr = VIRT_TO_DSP(addr);
	clean_d_cache(addr, dsp_cmd.cmd_buf_num * DSP_CMD_SIZE);

	dsp_printk("%s : cmd_code = 0x%x", __func__, dsp_cmd.cmd_code);
	dsp_printk("%s : stream_type = 0x%x", __func__, dsp_cmd.stream_type);
	dsp_printk("%s : cmd_buf_num = %d", __func__, dsp_cmd.cmd_buf_num);
	dsp_printk("%s : cmd_buf_addr = 0x%x", __func__, dsp_cmd.cmd_buf_addr);

	port->update_batch_cmd = 1;
	issue_normal_cmd(&dsp_cmd, sizeof(dsp_cmd));

	/* reset batch cmd param for next loop */
	port->index_batch_cmd = (cmd_index + 1) % MAX_BATCH_CMD_CHUNK;
	port->num_batch_cmds = 0;
}

static void dsp_wait_cmdq_empty(u32 p, u32 t)
{
	u32 out = 0;
	unsigned long flags;
	dsp_cmd_port_t *port = &G_dsp_obj->cmd_ports[p];

	BUG_ON(t == DSP_CMD_TYPE_NORMAL);

	while (1) {
		dsp_lock(flags);
		switch (t) {
		case DSP_CMD_TYPE_BLOCK:
			if (likely(port->num_blk_cmds == 0)) {
				out = 1;
			}
			break;
		case DSP_CMD_TYPE_BATCH:
			if (likely(port->update_batch_cmd == 0)) {
				out = 1;
			}
			break;
		default:
			break;
		}
		if (out) {
			dsp_unlock(flags);
			break;
		}
		port->num_waiters++;
		dsp_unlock(flags);
//		dsp_printk("dsp_wait_cmdq_empty \n");
		down(&port->sem);
	}
}

// the only entry API to send cmd to DSP
void dsp_issue_cmd(void *cmd, u32 size)
{
	PRINT_CMD(cmd, size, G_dsp_obj->cmd_mode == DSP_CMD_MODE_DEFAULT);

	if (G_dsp_obj->num_msgseq_error) {
		dsp_printk("msg lost %d: last_msgseq = %d\n",
			G_dsp_obj->num_msgseq_error, G_dsp_obj->last_msgseq);
	}

	switch (G_dsp_obj->cmd_mode) {
	default:						/* fall through */
	case DSP_CMD_MODE_NONE:		/* fall through */
	case DSP_CMD_MODE_BUSY:		/* fall through */
		dsp_error("dsp_issue_cmd at bad state %d\n", G_dsp_obj->cmd_mode);
		BUG();
		break;

	case DSP_CMD_MODE_DEFAULT:
		issue_default_cmd(cmd, size);
		break;

	case DSP_CMD_MODE_NORMAL:
		issue_normal_cmd(cmd, size);
		break;

	case DSP_CMD_MODE_SYNCC:
		issue_syncc_cmd(cmd, size);
		break;
	}
}
EXPORT_SYMBOL(dsp_issue_cmd);

int dsp_is_cmdq_full(u32 additional_cmds)
{
	dsp_cmd_port_t *port = NULL;

	port = G_dsp_obj->vcap_cmd_port;
	if (port->num_cmds + additional_cmds > ARRAY_SIZE(port->cmd_buffer))
		return 1;

	port = G_dsp_obj->curr_cmd_port;
	if (port->num_cmds + additional_cmds > ARRAY_SIZE(port->cmd_buffer))
		return 1;

	return 0;
}
EXPORT_SYMBOL(dsp_is_cmdq_full);

static void transfer_cmd(int index)
{
	unsigned long flags;

	u32 num_cmds;
	u32 i, prev_num_cmds, rest_cmds, prev_seq_num;
	u8 * cmd_addr = NULL;
	dsp_cmd_port_t * port = NULL;
	DSP_HEADER_CMD * header = NULL;
	DSP_STATUS_MSG * status = NULL;

	dsp_lock(flags);

	port = G_dsp_obj->cmd_ports + index;
	header = (void *)port->cmd_queue;
	status = (void *)port->msg_queue;
	invalidate_d_cache(status, sizeof(DSP_STATUS_MSG));

	prev_seq_num = status->prev_cmd_seq;
	prev_num_cmds = (prev_seq_num != port->prev_cmd_seq_num) ?
		status->prev_num_cmds : 0;
	BUG_ON(prev_num_cmds > port->num_cmds);
	port->prev_cmd_seq_num = prev_seq_num;

	/* Re-align the rest commands issued after DSP fetched.
	 */
	rest_cmds = port->num_cmds - prev_num_cmds;
	if ((rest_cmds > 0) && (prev_num_cmds > 0)) {
		cmd_addr = (u8*)header + DSP_CMD_SIZE;
		for (i = 0; i < rest_cmds; ++i, cmd_addr += DSP_CMD_SIZE) {
			memcpy(cmd_addr, cmd_addr + prev_num_cmds * DSP_CMD_SIZE,
				DSP_CMD_SIZE);
		}
	}
	port->num_cmds = rest_cmds;

	num_cmds = (port->cmd_type != DSP_CMD_TYPE_NORMAL) ? port->num_cmds :
		(port->num_cmds + port->num_blk_cmds + port->num_udec_cmds);

	/* If "num_cmds > 0", copy block cmds and udec cmds into cmd Q.
	 * If "prev_num_cmds > 0", update the seq num and advance flag of cmd header.
	 */
	if (num_cmds || prev_num_cmds) {

		BUG_ON(num_cmds >= port->max_num_cmds);

		/* Copy BLOCK commands when the block is finished. */
		num_cmds = port->num_blk_cmds;
		if ((port->cmd_type == DSP_CMD_TYPE_NORMAL) && (num_cmds > 0) &&
				(num_cmds + port->num_cmds < port->max_num_cmds)) {
			cmd_addr = (u8*)header + (port->num_cmds + 1) * DSP_CMD_SIZE;
			memcpy(cmd_addr, port->cmd_blk, num_cmds * DSP_CMD_SIZE);

			port->num_cmds += num_cmds;
			port->num_blk_cmds = 0;
		}

		/* Copy UDEC commands from cached buffer to shared memory. */
		num_cmds = port->num_udec_cmds;
		if ((G_dsp_obj->dsp_op_mode == DSP_OP_MODE_UDEC) && (num_cmds > 0) &&
				(num_cmds + port->num_cmds < port->max_num_cmds)) {
			cmd_addr = (u8*)header + (port->num_cmds + 1) * DSP_CMD_SIZE;
			memcpy(cmd_addr, port->cmd_buffer, num_cmds * DSP_CMD_SIZE);

			port->num_cmds += num_cmds;
			port->num_udec_cmds = 0;
		}

		/* Flush out all command first. */
		if (port->num_cmds) {
			clean_d_cache((u8*)header + DSP_CMD_SIZE,
				port->num_cmds * DSP_CMD_SIZE);
		}

		/* Update the seq num and advance flag of cmd header. */
		header->cmd_code = CMD_DSP_HEADER;
		header->num_cmds = port->num_cmds;
		if (port->num_cmds == 0) {
			port->update_cmd_seq_num = 1;
		} else {
			if (prev_seq_num == header->cmd_seq_num) {
				++header->cmd_seq_num;
				port->update_cmd_seq_num = 0;
			}
		}
		clean_d_cache(header, DSP_CMD_SIZE);

		port->update_batch_cmd = 0;
	} else if (header->num_cmds != 0) {
		header->num_cmds = 0;
		clean_d_cache(header, DSP_CMD_SIZE);
	}

	if (G_dsp_obj->dsp_op_mode == DSP_OP_MODE_UDEC) {
		clear_udec_dec_info(port);
	}

	dsp_unlock(flags);
}

DSP_OP_MODE dsp_get_op_mode(void)
{
	return G_dsp_obj->dsp_op_mode;
}
EXPORT_SYMBOL(dsp_get_op_mode);


static void process_msg(int index)
{
#ifdef CONFIG_CALC_IRQ_TIME
	const int print_count_max = 300;
	struct timeval start, stop;
	static u64 irq_time[NUM_DSP_CMD_PORT] = {0, 0, 0, 0};
	static int count[NUM_DSP_CMD_PORT] = {0, 0, 0, 0};
#endif

	unsigned long flags;
	dsp_cmd_port_t *port = G_dsp_obj->cmd_ports + index;
	DSP_STATUS_MSG *status = (void*)port->msg_queue;

#ifdef CONFIG_DSP_ENABLE_TIMER
	volatile u32 *sts_addr = (u32*)(APB_BASE + TIMER_OFFSET + 0x10);
	port->timestamp = *sts_addr;
#endif


#ifdef CONFIG_CALC_IRQ_TIME
	do_gettimeofday(&start);
#endif

	invalidate_d_cache(status, sizeof(*status));

	dsp_lock(flags);

	// Get DSP OP MODE from DSP_STATUS_MSG
	G_dsp_obj->dsp_op_mode = status->dsp_mode;

	dsp_unlock(flags);

	if (status->num_msgs > 0) {
		DSP_MSG *msg = (DSP_MSG*)status + 1;
		int j;

		invalidate_d_cache(msg, DSP_MSG_SIZE * status->num_msgs);

		for (j = status->num_msgs; j > 0; j--, msg++) {
			unsigned cat = GET_DSP_CMD_CAT(msg->msg_code);

			BUG_ON(cat >= NUM_MSG_CAT);

			if (cat < NUM_MSG_CAT) {
				if (g_cat_msg_handler[cat]) {
					g_cat_msg_handler[cat](g_cat_context[cat], cat, msg, index);
				} else {
					int i;
					DRV_PRINT("no msg handler, msg_code=0x%x\n", msg->msg_code);
					for (i = 0; i < 16*4; i += 4)
						DRV_PRINT("0x%08x\n", *(u32*)((u8*)msg + i));
				}
			}
		}
	}

#ifdef CONFIG_CALC_IRQ_TIME
	do_gettimeofday(&stop);
	irq_time[index] += ((u64) (stop.tv_sec - start.tv_sec) * 1000000L +
		(stop.tv_usec - start.tv_usec));
	if ((++count[index] % print_count_max) == 0) {
		dsp_printk("[DSP PORT: %d] Total time in IRQ of [%8d] times [%lld] us.\n",
			index, count[index], irq_time[index]);
	}
#endif
}

static inline void __dsp_notify_port_waiters(int index)
{
	dsp_cmd_port_t *port = G_dsp_obj->cmd_ports + index;
	while (port->num_waiters > 0) {
		port->num_waiters--;
		up(&port->sem);
	}
}

static inline void __dsp_notify_waiters(void)
{
	while (G_dsp_obj->num_waiters > 0) {
		G_dsp_obj->num_waiters--;
		up(&G_dsp_obj->sem);
	}
}

void dsp_notify_waiters(void)
{
	unsigned long flags;
	dsp_lock(flags);
	__dsp_notify_waiters();
	dsp_unlock(flags);
}
EXPORT_SYMBOL(dsp_notify_waiters);


int dsp_wait_irq_interruptible(void)
{
	unsigned long flags;

	dsp_lock(flags);
	G_dsp_obj->num_waiters++;
	dsp_unlock(flags);

	if (down_interruptible(&G_dsp_obj->sem)) {
		dsp_lock(flags);
		if (G_dsp_obj->num_waiters > 0) {
			G_dsp_obj->num_waiters--;
		} else {
			down(&G_dsp_obj->sem);
		}
		dsp_unlock(flags);
		return -EINTR;
	}

	return 0;
}
EXPORT_SYMBOL(dsp_wait_irq_interruptible);

static irqreturn_t vdsp_irq(int irqno, void *dev_id)
{
	unsigned long flags;

	G_dsp_obj->irq_counter++;
	transfer_cmd(DSP_CMD_PORT_GEN);
	process_msg(DSP_CMD_PORT_GEN);

	dsp_lock(flags);
	__dsp_notify_port_waiters(DSP_CMD_PORT_GEN);
	__dsp_notify_waiters();
//	dsp_printk_work();
	dsp_unlock(flags);

	return IRQ_HANDLED;
}

static irqreturn_t vcap_irq(int irqno, void *dev_id)
{
	unsigned long flags;

	G_dsp_obj->vcap_counter++;
	transfer_cmd(DSP_CMD_PORT_VCAP);
	process_msg(DSP_CMD_PORT_VCAP);

	dsp_lock(flags);
	__dsp_notify_port_waiters(DSP_CMD_PORT_VCAP);
	__dsp_notify_waiters();
//	dsp_printk_work();
	dsp_unlock(flags);

	return IRQ_HANDLED;
}

static irqreturn_t venc_irq(int irqno, void *dev_id)
{
	if (g_enc_handler) {
		g_enc_handler(g_enc_context);
	}
	return IRQ_HANDLED;
}

void dsp_issue_udec_dec_cmd(UDEC_DEC_FIFO_CMD *dsp_cmd, int size)
{
	int done = 0;

	if (1) {
		unsigned long flags;

		//dsp_save_cmd(dsp_cmd, size, DSP_CMD_PORT_GEN);	// todo

		dsp_lock(flags);

		if (G_dsp_obj->cmd_mode == DSP_CMD_MODE_NORMAL) {
			dsp_cmd_port_t *port = G_dsp_obj->curr_cmd_port;
			int decoder_id = dsp_cmd->base_cmd.decoder_id;
			UDEC_DEC_FIFO_CMD *last_dsp_cmd = port->last_udec_dec_bfifo_cmd[decoder_id];
			if (last_dsp_cmd != NULL &&
//					last_dsp_cmd->base_cmd.decoder_id == dsp_cmd->base_cmd.decoder_id &&
					last_dsp_cmd->base_cmd.decoder_type == dsp_cmd->base_cmd.decoder_type) // &&
//					last_dsp_cmd->bits_fifo_end + 1 == dsp_cmd->bits_fifo_start)
			{
				last_dsp_cmd->base_cmd.num_of_pics += dsp_cmd->base_cmd.num_of_pics;
				last_dsp_cmd->bits_fifo_end = dsp_cmd->bits_fifo_end;
#ifdef DEBUG_PRINT_FRAME_NUMBER
				g_decode_frame_cnt[decoder_id] += dsp_cmd->base_cmd.num_of_pics;
				g_has_decode_cmd = 1;
#endif
				done = 2;
			} else if (port->num_cmds < ARRAY_SIZE(port->cmd_buffer)) {

				u8 *ptr = (u8*)(port->cmd_buffer + port->num_udec_cmds);
				dsp_copy_cmd(ptr, dsp_cmd, sizeof(*dsp_cmd));
				port->num_udec_cmds++;

				port->last_udec_dec_bfifo_cmd[decoder_id] = (void*)ptr;
				done = 1;

				//debug code
				//g_decode_cmd_cnt[decoder_id] ++;
#ifdef DEBUG_PRINT_FRAME_NUMBER
				g_decode_frame_cnt[decoder_id] = dsp_cmd->base_cmd.num_of_pics;
				g_has_decode_cmd = 1;
#endif
			} else {
				DRV_PRINT("too many decode cmd to send, send it after next vdsp interrupt.\n");
				//g_decode_cmd_cnt[decoder_id] ++;
			}
		}

		dsp_unlock(flags);
	}

	if (!done) {
		dsp_issue_cmd(dsp_cmd, size);
		//dsp_issue_cmd_sync(dsp_cmd, size);
	}
}

EXPORT_SYMBOL(dsp_issue_udec_dec_cmd);

#if 0
static irqreturn_t sync_irq(int irqno, void *dev_id)
{
	unsigned long flags;

	dsp_lock(flags);

	G_dsp_obj->irq_counter++;
	G_dsp_obj->sync_counter++;

	process_msg(DSP_CMD_PORT_SYNCC);

	dsp_printk_work();

	while (G_dsp_obj->num_syncc_waiters > 0) {
		G_dsp_obj->num_syncc_waiters--;
		up(&G_dsp_obj->syncc_sem);
	}

	dsp_unlock(flags);

	dsp_printk("unexpected sync irq generated \n");
	return IRQ_HANDLED;
}
#endif


void dsp_start_default_cmd(void)
{
	G_dsp_obj->cmd_mode = DSP_CMD_MODE_DEFAULT;
	G_dsp_obj->num_default_cmds = 0;
}
EXPORT_SYMBOL(dsp_start_default_cmd);


void dsp_end_default_cmd(void)
{
	flush_default_cmdq();
	G_dsp_obj->cmd_mode = DSP_CMD_MODE_NORMAL;
}
EXPORT_SYMBOL(dsp_end_default_cmd);


void dsp_enable_vcap_cmd_port(int enable)
{
	G_dsp_obj->vcap_cmd_port = G_dsp_obj->cmd_ports + (enable ?
		DSP_CMD_PORT_VCAP : DSP_CMD_PORT_GEN);
}
EXPORT_SYMBOL(dsp_enable_vcap_cmd_port);


void dsp_start_cmdblk(u32 port)
{
	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_NORMAL);
	dsp_wait_cmdq_empty(port, DSP_CMD_TYPE_BLOCK);
	G_dsp_obj->cmd_ports[port].cmd_type = DSP_CMD_TYPE_BLOCK;
}
EXPORT_SYMBOL(dsp_start_cmdblk);


void dsp_end_cmdblk(u32 port)
{
	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_NORMAL);
	G_dsp_obj->cmd_ports[port].cmd_type = DSP_CMD_TYPE_NORMAL;
}
EXPORT_SYMBOL(dsp_end_cmdblk);


void dsp_start_enc_batch_cmd(void)
{
	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_NORMAL);
	dsp_wait_cmdq_empty(DSP_CMD_PORT_GEN, DSP_CMD_TYPE_BATCH);
	G_dsp_obj->cmd_ports[DSP_CMD_PORT_GEN].cmd_type = DSP_CMD_TYPE_BATCH;
}
EXPORT_SYMBOL(dsp_start_enc_batch_cmd);


void dsp_end_enc_batch_cmd(void)
{
	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_NORMAL);
	G_dsp_obj->cmd_ports[DSP_CMD_PORT_GEN].cmd_type = DSP_CMD_TYPE_NORMAL;

	if (likely(G_dsp_obj->cmd_ports[DSP_CMD_PORT_GEN].num_batch_cmds)) {
		issue_enc_batch_cmd();
	}
}
EXPORT_SYMBOL(dsp_end_enc_batch_cmd);


void dsp_start_sync_cmd(void)
{
#if 0
	if (!G_use_syncc)
		return;

	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_NORMAL);
	G_dsp_obj->cmd_mode = DSP_CMD_MODE_SYNCC;
#endif
}
EXPORT_SYMBOL(dsp_start_sync_cmd);


void dsp_end_sync_cmd(void)
{
#if 0
	dsp_cmd_port_t *port;
	u32 num_cmds;

	if (!G_use_syncc)
		return;

	BUG_ON(G_dsp_obj->cmd_mode != DSP_CMD_MODE_SYNCC);
	G_dsp_obj->cmd_mode = DSP_CMD_MODE_NORMAL;

	port = G_dsp_obj->cmd_ports + DSP_CMD_PORT_SYNCC;
	num_cmds = port->num_cmds;

	if (num_cmds > 0) {
		DSP_HEADER_CMD *header;
		unsigned long flags;

		while (1) {
			u32 read_val;

			dsp_lock(flags);

			read_val = *(volatile u32*)SYNCC_READ_ADDR;
			read_val = (read_val >> 12) & 0xFFF;
			if (read_val == 0) {
				dsp_unlock(flags);
				break;
			}

			G_dsp_obj->num_syncc_waiters++;
			dsp_unlock(flags);

			down(&G_dsp_obj->syncc_sem);
		}

		header = (void*)port->cmd_queue;

		header->cmd_code = CMD_DSP_HEADER;
//		header->cmd_seq_num += port->cmd_sent;
		header->cmd_seq_num++;
		header->num_cmds = num_cmds;

		memcpy((u8*)header + DSP_CMD_SIZE, port->cmd_buffer, num_cmds * DSP_CMD_SIZE);
		clean_d_cache(header, (num_cmds + 1) * DSP_CMD_SIZE);

		*(volatile u32*)SYNCC_WRITE_ADDR = 11;

		port->num_cmds = 0;
//		port->cmd_sent = 1;
	}
#endif
}
EXPORT_SYMBOL(dsp_end_sync_cmd);


void dsp_issue_cmd_sync(void *cmd, u32 size)
{
	dsp_start_sync_cmd();
	dsp_issue_cmd(cmd, size);
	dsp_end_sync_cmd();
}
EXPORT_SYMBOL(dsp_issue_cmd_sync);

void dsp_issue_cmd_sync_ex(void *cmd, u32 size)
{
	if (G_dsp_obj->cmd_mode == DSP_CMD_MODE_SYNCC)
		dsp_issue_cmd(cmd, size);
	else
		dsp_issue_cmd_sync(cmd, size);
}
EXPORT_SYMBOL(dsp_issue_cmd_sync_ex);

void __dsp_issue_cmd_pair_msg(void *cmd, u32 size, int sync_port)
{
	if (sync_port) {
		dsp_issue_cmd_sync(cmd, size);
		down(&G_dsp_obj->pair_sem);
		BUG_ON(G_dsp_obj->pair_callback_id != *(u32*)cmd);
	} else {
		dsp_issue_cmd(cmd, size);
		down(&G_dsp_obj->pair_sem);
	}
}

int dsp_system_event(unsigned long val)
{
	int					errorCode = 0;

	dsp_printk("dsp_system_event %ld \n", val);
	switch (val) {
	case AMBA_EVENT_PRE_PM:
		memcpy(G_dsp_obj->dsp_init_data_pm, G_dsp_obj->dsp_init_data,
			sizeof(DSP_INIT_DATA));
		break;

	case AMBA_EVENT_POST_PM:
		memcpy(G_dsp_obj->dsp_init_data, G_dsp_obj->dsp_init_data_pm,
			sizeof(DSP_INIT_DATA));
		clean_d_cache((void *)G_dsp_obj->dsp_init_data,
			sizeof(DSP_INIT_DATA));
		break;

	default:
		DRV_PRINT("%s: unknown event %ld\n", __func__, val);
		break;
	}

	return errorCode;
}
EXPORT_SYMBOL(dsp_system_event);


void dsp_start_change_mode(DSP_OP_MODE mode)
{
	dsp_printk("dsp_start_change_mode \n");
}
EXPORT_SYMBOL(dsp_start_change_mode);


void dsp_end_change_mode(void)
{
	dsp_printk("dsp_end_change_mode \n");
}
EXPORT_SYMBOL(dsp_end_change_mode);


const struct ucode_load_info_s *dsp_get_ucode_info(void)
{
	return &dsp_ucode_info;
}
EXPORT_SYMBOL(dsp_get_ucode_info);


unsigned long dsp_get_ucode_start(void)
{
	return DSP_UCODE_START;
}
EXPORT_SYMBOL(dsp_get_ucode_start);


unsigned long dsp_get_ucode_size(void)
{
	return DSP_UCODE_SIZE;
}
EXPORT_SYMBOL(dsp_get_ucode_size);

const struct ucode_version_s *dsp_get_ucode_version(void)
{
	#define TIME_STRING_OFFSET		(92)
	#define EDITION_OFFSET			(88)
	#define VERSION_OFFSET			(96)

	static ucode_version_t version;
	u8 *addr;
	u32 tmp;
	static u8 *dsp_dram_code_base = NULL;

	// Fix Me! It's better to move MAP into dsp_init.
	if (!dsp_dram_code_base) {
		dsp_dram_code_base = ioremap_nocache(DSP_DRAM_CODE_START, VERSION_OFFSET + sizeof(u32));
	}

	addr = dsp_dram_code_base + TIME_STRING_OFFSET;
	tmp = *(u32*)addr;
	version.year = tmp >> 16;
	version.month = (tmp >> 8) & 0xFF;
	version.day = tmp & 0xFF;

	addr = dsp_dram_code_base + EDITION_OFFSET;
	version.edition_num = *(u32*)addr;

	addr = dsp_dram_code_base + VERSION_OFFSET;
	version.edition_ver = *(u32*)addr;

	return &version;
}
EXPORT_SYMBOL(dsp_get_ucode_version);


static void wait_dsp_mode(DSP_OP_MODE next_mode, const char *desc)
{
	dsp_printk("Entering DSP mode [%s]..., target mode %d, ori mode %d\n",
		desc, next_mode, G_dsp_obj->dsp_op_mode);

	while (1) {
		unsigned long flags;
		dsp_lock(flags);
		if (G_dsp_obj->dsp_op_mode == next_mode) {
			dsp_unlock(flags);
			break;
		}
		G_dsp_obj->num_waiters++;
		dsp_unlock(flags);
		down(&G_dsp_obj->sem);
	}

	dsp_printk("[Done] Entered mode [%s], target mode %d, ori mode %d\n",
		desc, next_mode, G_dsp_obj->dsp_op_mode);
}

void dsp_activate_mode(void)
{
	DSP_ACTIVATE_OP_MODE_CMD dsp_cmd;
	dsp_cmd.cmd_code = CMD_DSP_ACTIVATE_OPERATION_MODE;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
}
EXPORT_SYMBOL(dsp_activate_mode);


static int set_mode_vm_space(void)
{
#if 0
	dsp_vm_space_t *vms;

	vms = dsp_mode_vms();
	if (vms->space_id <= 0)
		return -ENOMEM;

	return dsp_mm_set_dram_space(vms);
#else
	dsp_printk("set mode vm space \n");
	return 0;
#endif
}

// from other mode to IDLE mode
static inline void enter_idle_mode(void)
{
	if (G_dsp_obj->dsp_op_mode != DSP_OP_MODE_IDLE) {
		DSP_SUSPEND_OP_MODE_CMD dsp_cmd;
		memset(&dsp_cmd, 0, sizeof(dsp_cmd));
		dsp_cmd.cmd_code = CMD_DSP_SUSPEND_OPERATION_MODE;

#ifdef CONFIG_DSP_ENABLE_L2_ON_OFF
		set_smem_size(512 * KB);
#endif

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
		wait_dsp_mode(DSP_OP_MODE_IDLE, "idle");

#ifdef CONFIG_DSP_ENABLE_L2_ON_OFF
		set_l2_on();
#endif

	//	dsp_free_vm_space(dsp_mode_vms());
	}
}

// change DSP operation mode
int __dsp_set_mode(DSP_OP_MODE next_mode, const char *desc,
	void (*init_mode)(void*), void *context)
{
	dsp_printk("DSP switch mode %d -> %d ", G_dsp_obj->dsp_op_mode, next_mode);

	if (G_dsp_obj->dsp_op_mode == next_mode) {
		dsp_printk("DSP already in mode %d, do nothing \n", next_mode);
		return 0;
	}

	if (G_dsp_obj->dsp_op_mode == DSP_OP_MODE_INIT) {

		// DSP not booted
#ifdef CONFIG_DSP_ENABLE_L2_ON_OFF
		set_l2_off();
		if (next_mode == DSP_OP_MODE_IDLE)
			set_smem_size(512 * KB);
		else
			set_smem_size(1 * MB);
#endif

		if (next_mode != DSP_OP_MODE_IDLE) {
			set_mode_vm_space();
		}

		if (init_mode) {
			init_mode(context);
		}

		if (next_mode != DSP_OP_MODE_IDLE) {
			dsp_printk("Activate mode %d \n", next_mode);
			dsp_activate_mode();
		}

		dsp_boot(next_mode);
		wait_dsp_mode(next_mode, desc);

#ifdef CONFIG_DSP_ENABLE_L2_ON_OFF
		if (next_mode == DSP_OP_MODE_IDLE)
			set_l2_on();
#endif

	} else {

		// DSP already booted. Enter IDLE mode first
		enter_idle_mode();

		if (G_dsp_obj->dsp_op_mode != next_mode) {

#ifdef CONFIG_DSP_ENABLE_L2_ON_OFF
			set_smem_size(1 * MB);
			set_l2_off();
#endif

			set_mode_vm_space();
			if (init_mode) {
				init_mode(context);
			}

			dsp_printk("Activate mode %d \n", next_mode);
			dsp_activate_mode();

			wait_dsp_mode(next_mode, desc);
		}
	}

	return 0;
}
EXPORT_SYMBOL(__dsp_set_mode);



int dsp_init_logbuf(u8 **code_addr, u8 **memd_addr, u8 **print_buffer, u32 * buffer_size)
{
        *code_addr = (u8*)DSP_DRAM_CODE_BASE;
        *memd_addr= (u8*)DSP_DRAM_MEMD_BASE;
        *print_buffer=(u8*)DSP_LOG_AREA;
         *buffer_size = DSP_LOG_SIZE;
        return 0;
}
EXPORT_SYMBOL(dsp_init_logbuf);


