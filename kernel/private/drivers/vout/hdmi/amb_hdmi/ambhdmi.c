/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi.c
 *
 * History:
 *    2009/06/02 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include "vout_pri.h"
#include "ambhdmi_edid.h"

/* ========================================================================== */
struct ambhdmi_sink {
	struct device			*dev;
	unsigned char __iomem		*regbase;
	u32				irq;
	wait_queue_head_t		irq_waitqueue;
	u32				irq_pending;
	struct task_struct		*kthread;
	u32				killing_kthread;
	struct i2c_adapter		*ddc_adapter;
	amba_hdmi_raw_edid_t		raw_edid;
	amba_hdmi_edid_t		edid;
	struct proc_dir_entry		*proc_entry;
	struct __amba_vout_video_sink	video_sink;
	const enum amba_video_mode	*mode_list;
	struct notifier_block		audio_transition;
};

struct ambhdmi_instance_info {
	const char			name[AMBA_VOUT_NAME_LENGTH];
	const int			source_id;
	const u32			sink_type;
	const struct resource		io_mem;
	const struct resource		irq;
	const enum amba_video_mode	mode_list[AMBA_VIDEO_MODE_NUM];

	struct ambhdmi_sink		*phdmi_sink;
};

/* ========================================================================== */
#define AMBHDMI_EDID_PROC_NAME		"hdmi_edid"
static int ambhdmi_probe(struct device *dev);
static int ambhdmi_remove(struct device *dev);

struct amb_driver ambhdmi = {
	.probe = ambhdmi_probe,
	.remove = ambhdmi_remove,
	.driver = {
		.name = "ambhdmi",
		.owner = THIS_MODULE,
	}
};

#define AMBHDMI_PLUG_IN		(HDMI_STS_RX_SENSE | HDMI_STS_HPD_HI)
static int ambhdmi_hw_init(void);
static int ambhdmi_task(void *arg);

/* ========================================================================== */
#include "ambhdmi_edid.c"
#include "ambhdmi_hdcp.c"
#include "ambhdmi_docmd.c"
#include "arch/ambhdmi_arch.c"

/* ========================================================================== */
static int ambhdmi_add_video_sink(struct i2c_adapter *adapter, struct device *pdev)
{
	int				i, errorCode = 0;
	struct ambhdmi_sink		*phdmi_sink;
	struct __amba_vout_video_sink	*pvideo_sink;
	amba_hdmi_edid_t 		*pedid;

	/* Malloc HDMI INFO */
	phdmi_sink = kzalloc(sizeof(struct ambhdmi_sink), GFP_KERNEL);
	if (!phdmi_sink) {
		errorCode = -ENOMEM;
		goto ambhdmi_add_video_sink_abnormal_exit;
	}

	/* Fill HDMI INFO */
	phdmi_sink->dev = pdev;
	phdmi_sink->regbase = (unsigned char __iomem *)hdmi_instance.io_mem.start;
	phdmi_sink->irq = hdmi_instance.irq.start;
	init_waitqueue_head(&phdmi_sink->irq_waitqueue);
	phdmi_sink->irq_pending	= 0;
	phdmi_sink->killing_kthread = 0;
	pedid = &phdmi_sink->edid;
	pedid->interface = HDMI;
	pedid->color_space.support_ycbcr444 = 0;
	pedid->color_space.support_ycbcr422 = 0;
	pedid->cec.physical_address = 0xffff;
	pedid->cec.logical_address = CEC_DEV_UNREGISTERED;
	phdmi_sink->ddc_adapter = adapter;
	phdmi_sink->audio_transition.notifier_call =
		ambhdmi_hdmise_audio_transition;
	phdmi_sink->mode_list = hdmi_instance.mode_list;

	/* Add Sink and Register Audio Notifier */
	pvideo_sink = &phdmi_sink->video_sink;
	pvideo_sink->source_id = hdmi_instance.source_id;
	pvideo_sink->sink_type = hdmi_instance.sink_type;
	strlcpy(pvideo_sink->name, hdmi_instance.name, sizeof(pvideo_sink->name));
	pvideo_sink->state = AMBA_VOUT_SINK_STATE_IDLE;
	pvideo_sink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
	for (i = 0; i < sizeof(pvideo_sink->hdmi_modes) / sizeof (enum amba_video_mode); i++) {
		pvideo_sink->hdmi_modes[i] = AMBA_VIDEO_MODE_MAX;
	}
	pvideo_sink->hdmi_native_mode = AMBA_VIDEO_MODE_MAX;
	pvideo_sink->owner = THIS_MODULE;
	pvideo_sink->pinfo = phdmi_sink;
	pvideo_sink->docmd = ambhdmi_docmd;

	errorCode = amba_vout_add_video_sink(pvideo_sink);
	if (errorCode) {
		goto ambhdmi_add_video_sink_abnormal_exit;
	}
	ambarella_audio_register_notifier(&phdmi_sink->audio_transition);
	amba_vout_video_source_cmd(pvideo_sink->source_id,
			AMBA_VIDEO_SOURCE_REGISTER_AR_NOTIFIER,
			ambhdmi_hdmise_video_ar_transition);
	hdmi_instance.phdmi_sink = phdmi_sink;
	phdmi_sink->kthread = kthread_run(ambhdmi_task, NULL, "hdmid");
	vout_notice("%s:%d@%d probed!\n", pvideo_sink->name, pvideo_sink->id,
		pvideo_sink->source_id);

	goto ambhdmi_add_video_sink_normal_exit;

ambhdmi_add_video_sink_abnormal_exit:
	if (phdmi_sink)
		kfree(phdmi_sink);
	hdmi_instance.phdmi_sink = NULL;

ambhdmi_add_video_sink_normal_exit:
	return errorCode;
}

static int ambhdmi_del_video_sink(void)
{
	int				errorCode = 0;
	struct ambhdmi_sink		*phdmi_sink;

	if (hdmi_instance.phdmi_sink) {
		phdmi_sink = hdmi_instance.phdmi_sink;
		if (phdmi_sink->kthread) {
			phdmi_sink->killing_kthread = 1;
			wake_up(&phdmi_sink->irq_waitqueue);
			kthread_stop(phdmi_sink->kthread);
		}
		if (phdmi_sink->raw_edid.buf)
			vfree(phdmi_sink->raw_edid.buf);
		ambarella_audio_unregister_notifier(&phdmi_sink->audio_transition);
		errorCode = amba_vout_del_video_sink(&phdmi_sink->video_sink);
		vout_notice("%s module removed!\n", phdmi_sink->video_sink.name);
		kfree(phdmi_sink);
		hdmi_instance.phdmi_sink = NULL;
	}

	return errorCode;
}

#ifdef CONFIG_PROC_FS
static int ambarella_hdmi_edid_proc_show(struct seq_file *m, void *v)
{
	int errorCode = 0;
	struct ambhdmi_sink *phdmi_sink;
	int i, j;

	/* Check HDMI sink */
	if (hdmi_instance.phdmi_sink) {
		phdmi_sink = hdmi_instance.phdmi_sink;
	} else {
		seq_printf(m, "No HDMI sink!\n");
		errorCode = -EINVAL;
		goto ambhdmi_read_edid_proc_exit;
	}
	if (phdmi_sink->video_sink.hdmi_plug == AMBA_VOUT_SINK_REMOVED) {
		errorCode = seq_printf(m,
			"No HDMI Sink Device Plugged In!\n");
		goto ambhdmi_read_edid_proc_exit;
	}
	if (!phdmi_sink->raw_edid.buf) {
		errorCode = seq_printf(m,
			"No EDID Contained in Sink Device!\n");
		goto ambhdmi_read_edid_proc_exit;
	}

	/* Base EDID */
	seq_printf(m, "Basic EDID:\n");
	for (i = 0; i < EDID_PER_SEGMENT_SIZE; i++) {
		if ((i & 0xf) == 0) {
			seq_printf(m, "0x%02x: ", i);
		}
		seq_printf(m, "%02x ", phdmi_sink->raw_edid.buf[i]);
		if ((i & 0xf) == 0xf) {
			seq_printf(m, "\n");
		}
	}

	/* EDID Extensions */
	if (phdmi_sink->raw_edid.buf[0x7e]) {
		for (i = 0; i < phdmi_sink->raw_edid.buf[0x7e]; i++) {
			seq_printf(m, "EDID Extensions #%d:\n", i + 1);
			for (j = 0; j < EDID_PER_SEGMENT_SIZE; j++) {
				if ((j & 0xf) == 0) {
					seq_printf(m, "0x%02x: ", j);
				}
				seq_printf(m, "%02x ",
					phdmi_sink->raw_edid.buf[EDID_PER_SEGMENT_SIZE * (i + 1) + j]);
				if ((j & 0xf) == 0xf) {
					seq_printf(m, "\n");
				}
			}
		}
	}
	errorCode = 0;

ambhdmi_read_edid_proc_exit:
	return errorCode;
}

static int ambarella_hdmi_edid_proc_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	return single_open(file, ambarella_hdmi_edid_proc_show, PDE(inode)->data);
#else
	return single_open(file, ambarella_hdmi_edid_proc_show, PDE_DATA(inode));
#endif
}

static const struct file_operations ambarella_hdmi_edid_fops = {
	.open = ambarella_hdmi_edid_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
};
#endif

static int ambhdmi_create_edid_proc(void)
{
	int				errorCode = 0;
	struct ambhdmi_sink		*phdmi_sink;

	if (hdmi_instance.phdmi_sink) {
		phdmi_sink = hdmi_instance.phdmi_sink;
	} else {
		errorCode = -EINVAL;
		goto ambhdmi_create_edid_proc_exit;
	}

	phdmi_sink->proc_entry = proc_create_data(AMBHDMI_EDID_PROC_NAME,
		S_IRUGO, get_ambarella_proc_dir(),
		&ambarella_hdmi_edid_fops, phdmi_sink->raw_edid.buf);
	if (phdmi_sink->proc_entry == NULL) {
		errorCode = -ENOMEM;
	}

ambhdmi_create_edid_proc_exit:
	return errorCode;
}

static int ambhdmi_remove_edid_proc(void)
{
	int				errorCode = 0;
	struct ambhdmi_sink		*phdmi_sink;

	if (hdmi_instance.phdmi_sink) {
		phdmi_sink = hdmi_instance.phdmi_sink;
	} else {
		errorCode = -EINVAL;
		goto ambhdmi_remove_edid_proc_exit;
	}
	remove_proc_entry(AMBHDMI_EDID_PROC_NAME, get_ambarella_proc_dir());
	phdmi_sink->proc_entry = NULL;

ambhdmi_remove_edid_proc_exit:
	return errorCode;
}

static irqreturn_t ambhdmi_isr(int irq, void *dev_id)
{
	struct ambhdmi_sink		*phdmi_sink;

	phdmi_sink = hdmi_instance.phdmi_sink;
	disable_irq_nosync(irq);
	if (phdmi_sink && !phdmi_sink->irq_pending) {
		phdmi_sink->irq_pending = 1;
		wake_up(&phdmi_sink->irq_waitqueue);
	}

	return IRQ_HANDLED;
}

static int ambhdmi_task(void *arg)
{
	int				i, j, errorCode;
	u32				regbase, reg, val;
	u32				irq, status;
	struct ambhdmi_sink		*phdmi_sink;
	amba_hdmi_edid_t 		*pedid;
	char *				envp[2];
	enum amba_vout_sink_plug	p_plug;

	regbase = hdmi_instance.io_mem.start;
	irq = hdmi_instance.irq.start;
	phdmi_sink = hdmi_instance.phdmi_sink;
	pedid = &phdmi_sink->edid;

	while (1) {
		wait_event_interruptible(phdmi_sink->irq_waitqueue,
			(phdmi_sink->irq_pending || phdmi_sink->killing_kthread));
		if (phdmi_sink->killing_kthread)
			break;
		phdmi_sink->irq_pending = 0;

		msleep(200);
		status = amba_readl(regbase + HDMI_STS_OFFSET);

		/* Rx Sense Remove or HPD Low */
		if ((status & AMBHDMI_PLUG_IN) != AMBHDMI_PLUG_IN) {
			if ((status & HDMI_STS_RX_SENSE) == 0 && phdmi_sink->video_sink.hdmi_plug == AMBA_VOUT_SINK_PLUGGED) {
				/* HDMI Device Removed */
				phdmi_sink->video_sink.hdmi_plug =
								AMBA_VOUT_SINK_REMOVED;
				for (i = 0; i < sizeof(phdmi_sink->video_sink.hdmi_modes) / sizeof(enum amba_video_mode); i++) {
					phdmi_sink->video_sink.hdmi_modes[i] = AMBA_VIDEO_MODE_MAX;
				}
				phdmi_sink->video_sink.hdmi_native_mode =
								AMBA_VIDEO_MODE_MAX;
				amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);
				envp[0] = "HOTPLUG=1";
				envp[1] = NULL;
				amb_event_report_uevent(&phdmi_sink->dev->kobj,
					KOBJ_CHANGE, envp);
				amba_vout_video_source_cmd(
					phdmi_sink->video_sink.source_id,
					AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,
					(void *)AMB_EV_VOUT_HDMI_REMOVE);
				vout_notice("HDMI Device Removed!\n");
			}
		}

		/* Rx Sense & HPD High */
		if ((status & AMBHDMI_PLUG_IN) == AMBHDMI_PLUG_IN && phdmi_sink->video_sink.hdmi_plug != AMBA_VOUT_SINK_PLUGGED) {
			/* Enable CEC Clock */
			reg = regbase + HDMI_CLOCK_GATED_OFFSET;
			val = amba_readl(reg);
			val |= (HDMI_CLOCK_GATED_CEC_CLOCK_EN | HDMI_CLOCK_GATED_HDCP_CLOCK_EN);
			amba_writel(reg, val);

			/* Read and Parse EDID */
#ifdef CONFIG_BSP_BOARD_S2ATB
			errorCode = 0;
#else
			errorCode = ambhdmi_edid_read_and_parse(phdmi_sink);
#endif

			if (!errorCode) {
				p_plug = phdmi_sink->video_sink.hdmi_plug;
				if (p_plug != AMBA_VOUT_SINK_PLUGGED) {
					vout_notice("HDMI Device Connected!\n");
				}

				/* hdmi_modes[] */
				phdmi_sink->video_sink.hdmi_plug = AMBA_VOUT_SINK_PLUGGED;
				for (i = 0, j = 0; i < pedid->native_timings.number; i++) {
#ifdef CONFIG_HDMI_NOT_SUPPORT_1080P
				if (pedid->native_timings.supported_native_timings[i].vmode == AMBA_VIDEO_MODE_1080P ||
					pedid->native_timings.supported_native_timings[i].vmode == AMBA_VIDEO_MODE_1080P_PAL) {
					continue;
				}
#endif
					phdmi_sink->video_sink.hdmi_modes[j++] = pedid->native_timings.supported_native_timings[i].vmode;
				}
				for (i = 0; i < pedid->cea_timings.number; i++) {
					if (j >= sizeof(phdmi_sink->video_sink.hdmi_modes) / sizeof(enum amba_video_mode)) {
						break;
					}
#ifdef CONFIG_HDMI_NOT_SUPPORT_1080P
				if (CEA_Timings[pedid->cea_timings.supported_cea_timings[i]].vmode == AMBA_VIDEO_MODE_1080P ||
					CEA_Timings[pedid->cea_timings.supported_cea_timings[i]].vmode == AMBA_VIDEO_MODE_1080P_PAL) {
					continue;
				}
#endif
					phdmi_sink->video_sink.hdmi_modes[j++] = CEA_Timings[pedid->cea_timings.supported_cea_timings[i]].vmode;
				}

				/* hdmi_native_mode */
#ifdef CONFIG_HDMI_1080P_CHECK_ENABLE
				i = 0;
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
				if (get_core_bus_freq_hz() < 240 * 1000 * 1000) {
#else
				if (clk_get_rate(clk_get(NULL, "gclk_core")) < 240 * 1000 * 1000) {
#endif
					i = pedid->native_timings.number;
					for (j = 0; j < pedid->native_timings.number; j++) {
						if (pedid->native_timings.supported_native_timings[j].vmode != AMBA_VIDEO_MODE_1080P
							&& pedid->native_timings.supported_native_timings[j].vmode != AMBA_VIDEO_MODE_1080P_PAL) {
							i = j;
							break;
						}
					}
				}

				if (pedid->native_timings.number && i < pedid->native_timings.number) {
					pedid->native_timings.supported_native_timings[i].vmode = AMBA_VIDEO_MODE_HDMI_NATIVE;
				}

				if (i >= pedid->native_timings.number) {
					phdmi_sink->video_sink.hdmi_native_mode = pedid->native_timings.supported_native_timings[0].vmode;
					phdmi_sink->video_sink.hdmi_native_width = pedid->native_timings.supported_native_timings[0].h_active;
					phdmi_sink->video_sink.hdmi_native_height = pedid->native_timings.supported_native_timings[0].v_active;
				} else {
					phdmi_sink->video_sink.hdmi_native_mode = pedid->native_timings.supported_native_timings[i].vmode;
					phdmi_sink->video_sink.hdmi_native_width = pedid->native_timings.supported_native_timings[i].h_active;
					phdmi_sink->video_sink.hdmi_native_height = pedid->native_timings.supported_native_timings[i].v_active;
				}
#else
#ifdef CONFIG_HDMI_NOT_SUPPORT_1080P
				i = pedid->native_timings.number;
				for (j = 0; j < pedid->native_timings.number; j++) {
					if (pedid->native_timings.supported_native_timings[j].vmode != AMBA_VIDEO_MODE_1080P
						&& pedid->native_timings.supported_native_timings[j].vmode != AMBA_VIDEO_MODE_1080P_PAL) {
						i = j;
						break;
					}
				}

				if (pedid->native_timings.number && i < pedid->native_timings.number) {
					pedid->native_timings.supported_native_timings[i].vmode = AMBA_VIDEO_MODE_HDMI_NATIVE;
				}

				if (i >= pedid->native_timings.number) {
					phdmi_sink->video_sink.hdmi_native_mode = pedid->native_timings.supported_native_timings[0].vmode;
					phdmi_sink->video_sink.hdmi_native_width = pedid->native_timings.supported_native_timings[0].h_active;
					phdmi_sink->video_sink.hdmi_native_height = pedid->native_timings.supported_native_timings[0].v_active;
				} else {
					phdmi_sink->video_sink.hdmi_native_mode = pedid->native_timings.supported_native_timings[i].vmode;
					phdmi_sink->video_sink.hdmi_native_width = pedid->native_timings.supported_native_timings[i].h_active;
					phdmi_sink->video_sink.hdmi_native_height = pedid->native_timings.supported_native_timings[i].v_active;
				}
#else
				if (pedid->native_timings.number) {
					pedid->native_timings.supported_native_timings[0].vmode = AMBA_VIDEO_MODE_HDMI_NATIVE;
				}
				phdmi_sink->video_sink.hdmi_native_mode = pedid->native_timings.supported_native_timings[0].vmode;
				phdmi_sink->video_sink.hdmi_native_width = pedid->native_timings.supported_native_timings[0].h_active;
				phdmi_sink->video_sink.hdmi_native_height = pedid->native_timings.supported_native_timings[0].v_active;
#endif
#endif

				if (p_plug != AMBA_VOUT_SINK_PLUGGED) {
					envp[0] = "HOTPLUG=0";
					envp[1] = NULL;
					amb_event_report_uevent(&phdmi_sink->dev->kobj,
						KOBJ_CHANGE, envp);
					amba_vout_video_source_cmd(
						phdmi_sink->video_sink.source_id,
						AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,
						(void *)AMB_EV_VOUT_HDMI_PLUG);
					ambhdmi_edid_print(pedid);
				}

				/* Enable CEC Rx Interrupt */
				reg = regbase + HDMI_INT_ENABLE_OFFSET;
				val = amba_readl(reg);
				val |= HDMI_INT_ENABLE_CEC_RX_INT_EN;
				amba_writel(reg, val);

				/* If working previously, reenable hdmise clock */
				reg = regbase + HDMI_OP_MODE_OFFSET;
				val = amba_readl(reg);
				if (val & HDMI_OP_MODE_OP_EN) {
					reg = regbase + HDMI_CLOCK_GATED_OFFSET;
					val = amba_readl(reg);
					val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
					amba_writel(reg, val);
				}

				/* Switch between HDMI and DVI Mode */
				reg = regbase + HDMI_OP_MODE_OFFSET;
				val = amba_readl(reg);
				if (HDMI_OP_MODE_OP_MODE(val) == HDMI_OP_MODE_OP_MODE_HDMI && phdmi_sink->edid.interface == DVI) {
					val &= 0xfffffffe;
					val |= HDMI_OP_MODE_OP_MODE_DVI;
					amba_writel(reg, val);
				}
				if (HDMI_OP_MODE_OP_MODE(val) == HDMI_OP_MODE_OP_MODE_DVI && phdmi_sink->edid.interface == HDMI) {
					val &= 0xfffffffe;
					val |= HDMI_OP_MODE_OP_MODE_HDMI;
					amba_writel(reg, val);
				}
			} else {
				amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);
			}
		};

		/* CEC Rx */
		if (status & HDMI_INT_STS_CEC_RX_INTERRUPT) {
			reg = regbase + HDMI_INT_ENABLE_OFFSET;
			val = amba_readl(reg);
			if (val & HDMI_INT_ENABLE_CEC_RX_INT_EN) {
				ambhdmi_cec_receive_message(phdmi_sink);
			}
		}

		amba_writel(regbase + HDMI_INT_STS_OFFSET, 0);
		enable_irq(irq);
	}

	return 0;
}

static int ambhdmi_hw_init(void)
{
	int				errorCode = 0;
	struct ambarella_gpio_io_info	hdmi_extpower;
	u32				regbase;

	regbase = hdmi_instance.io_mem.start;

	/* Power on HDMI 5V */
	hdmi_extpower = ambarella_board_generic.hdmi_extpower;
	if (hdmi_extpower.gpio_id >= 0) {
		ambarella_gpio_config(hdmi_extpower.gpio_id, GPIO_FUNC_SW_OUTPUT);
		ambarella_gpio_set(hdmi_extpower.gpio_id, hdmi_extpower.active_level);
		msleep(hdmi_extpower.active_delay);
	}

	/* Soft Reset HDMISE */
	amba_writel(regbase + HDMI_HDMISE_SOFT_RESET_OFFSET,
						HDMI_HDMISE_SOFT_RESET);
	amba_writel(regbase + HDMI_HDMISE_SOFT_RESET_OFFSET,
						~HDMI_HDMISE_SOFT_RESET);

	/* Reset CEC */
#if (VOUT_HDMI_CEC_REGS_OFFSET_GROUP == 1)
	amba_writel(regbase + CEC_CTRL_OFFSET, 0x1 << 31);
#else
	amba_writel(regbase + CEC_CTRL1_OFFSET, 0x1 << 31);
#endif

	/* Clock Gating */
	amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);

	/* Enable Hotplug detect and loss interrupt */
	amba_writel(regbase + HDMI_INT_ENABLE_OFFSET,
				HDMI_INT_ENABLE_PHY_RX_SENSE_REMOVE_EN |
				HDMI_INT_ENABLE_PHY_RX_SENSE_EN |
				HDMI_INT_ENABLE_HOT_PLUG_LOSS_EN |
				HDMI_INT_ENABLE_HOT_PLUG_DETECT_EN
				);

	return errorCode;
}

static int ambhdmi_probe(struct device *dev)
{
	int				errorCode    = 0;
	struct i2c_adapter		*adapter;


	/* Add video sink */
	adapter = i2c_get_adapter(1);
	if (!adapter) {
		errorCode = -ENODEV;
		vout_err("%s, %d\n", __func__, __LINE__);
		goto ambhdmi_probe_exit;
	}

	errorCode = ambhdmi_add_video_sink(adapter, dev);
	if (errorCode) {
		vout_err("%s, %d\n", __func__, __LINE__);
		goto ambhdmi_probe_exit;
	}

	/* Create proc directory */
	errorCode = ambhdmi_create_edid_proc();
	if (errorCode) {
		vout_err("%s, %d\n", __func__, __LINE__);
		goto create_edid_proc_error;
	}

	/* Init hardware */
	errorCode = ambhdmi_hw_init();
	if (errorCode) {
		vout_err("%s, %d\n", __func__, __LINE__);
		goto hw_init_error;
	}

	errorCode = request_irq(hdmi_instance.irq.start, ambhdmi_isr,
					IRQF_TRIGGER_HIGH, "HDMI", NULL);

	goto ambhdmi_probe_exit;

hw_init_error:
	ambhdmi_remove_edid_proc();

create_edid_proc_error:
	ambhdmi_del_video_sink();

ambhdmi_probe_exit:
	return errorCode;
}

static int ambhdmi_remove(struct device *dev)
{
	ambhdmi_remove_edid_proc();
	ambhdmi_del_video_sink();
	free_irq(hdmi_instance.irq.start, NULL);

	return 0;
}

static int __init ambhdmi_init(void)
{
	return amb_register_driver(&ambhdmi);
}

static void __exit ambhdmi_exit(void)
{
	amb_unregister_driver(&ambhdmi);
}

module_init(ambhdmi_init);
module_exit(ambhdmi_exit);

MODULE_DESCRIPTION("Ambarella Built-in HDMI Controller");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

