
#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"

#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "utils.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_pts.h"
#include "iav_mem.h"

#define FCC(a, b, c, d)	((a << 24) | (b << 16) | (c << 8) | d)

u32 G_iav_debug_flag = IAV_DEBUG_PRINT_ENABLE;


int iav_test(iav_context_t *context, u32 arg)
{
	switch (arg) {
	case FCC('c', 'l', 'r', 's'): {
			VOUT_OSD_SETUP_CMD dsp_cmd;
			memset(&dsp_cmd, 0, sizeof(dsp_cmd));
			dsp_cmd.cmd_code = CMD_VOUT_OSD_SETUP;
			dsp_cmd.vout_id = 1;
			dsp_cmd.en = 0;
			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
			DRV_PRINT("test: clear screen\n");
		}
		break;
	}
	return 0;
}

int iav_test_add_print_in_isr(u32 args0, u32 args1, u32 args2, u32 args3)
{
	const int MAX_TIME_SECOND = 10;
	static int init_done = 0;
	static struct timeval ts, start;
	static int stream_mask = 0xFF;
	u32 time_passed = 0;
	u32 stream, audio_pts;
	dsp_bits_info_t * frame = NULL;
	iav_pts_info_t * pts = NULL;
	iav_hwtimer_pts_info_t * hw_pts = NULL;

	stream = args0;
	frame = (dsp_bits_info_t *)args1;
	pts = (iav_pts_info_t *)args2;
	audio_pts = args3;
	hw_pts = &pts->hw_pts_info;

	/* Print time diff every MAX_TIME_SECOND second */
	if (!init_done) {
		do_gettimeofday(&ts);
		init_done = 1;
	}
	do_gettimeofday(&start);
	time_passed = start.tv_sec - ts.tv_sec;
	if (time_passed > MAX_TIME_SECOND) {
		stream_mask = 0xFF;
		ts = start;
	}
	if (stream_mask & (1 << stream)) {
		stream_mask &= ~(1 << stream);
		printk(KERN_DEBUG "frm [0x%x] stream %d audio pts %u audio tick %u "
			"diff %ld (audio freq is %u).\n",
			(u32)frame, stream, audio_pts, hw_pts->audio_tick,
			abs(hw_pts->audio_tick - audio_pts), hw_pts->audio_freq);
		udelay(100);
	}

	/* If KERN_DEBUG is enabled this will spam console and errors will appear quickly. */
	printk(KERN_DEBUG "frame [0x%x] stream %d audio pts %u audio tick %u "
		"diff %ld (audio freq is %u).\n",
		(u32)frame, stream, audio_pts, hw_pts->audio_tick,
		abs(hw_pts->audio_tick - audio_pts), hw_pts->audio_freq);

	return 0;
}

int iav_log_setup(iav_context_t *context,
	struct iav_dsp_setup_s __user *dsp_debug_setup)
{
	iav_dsp_setup_t dsp_setup;

	if (copy_from_user(&dsp_setup, dsp_debug_setup, sizeof(dsp_setup)))
		return -EFAULT;

	switch (dsp_setup.cmd) {
	case 1:
		dsp_set_debug_level(dsp_setup.args[0],
			dsp_setup.args[1], dsp_setup.args[2]);
		break;
	case 2:
		dsp_set_debug_thread(dsp_setup.args[0]);
		break;
	default:
		iav_printk("No such command : %d.\n", dsp_setup.cmd);
		return -ENOIOCTLCMD;
	}
	return 0;
}

int iav_debug_setup(iav_context_t * context,
	struct iav_debug_setup_s __user *debug_info)
{
	iav_debug_setup_t debug;

	if (copy_from_user(&debug, debug_info, sizeof(debug)))
		return -EFAULT;

	if (debug.flag) {
		if (debug.enable) {
			G_iav_debug_flag |= debug.flag;
		} else {
			G_iav_debug_flag &= ~debug.flag;
		}
	}

	return 0;
}

/*  FPS statistics */
static void init_hw_timer_for_fps_stat(void)
{
	//sequential timer at high resolution
	iav_printk("Use HW TIMER1 for calculation...\n");
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);	//disable
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_OF1);	//no intr on overflow
	amba_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL1);	//use APB clock
	amba_writel(TIMER1_STATUS_REG, 0xFFFFFFFF);		//initial
	amba_writel(TIMER1_RELOAD_REG, 0xFFFFFFFF);		//reload
	amba_writel(TIMER1_MATCH1_REG, 0x0);
	amba_writel(TIMER1_MATCH2_REG, 0x0);
	amba_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN1);
}

int iav_start_vin_fps_stat(iav_context_t *context, u8 arg)
{
	iav_printk("start vin fps stat \n");
	init_hw_timer_for_fps_stat();
	amba_vin_vsync_calc_fps_reset();
	iav_printk("now wait for the stat to finish \n");
	amba_vin_vsync_calc_fps_wait();
	return 0;
}

int iav_get_vin_fps_stat(iav_context_t *context, struct amba_fps_report_s * fps)
{
	struct amba_fps_report_s fps_report;
	int ret;

	iav_printk("calculate vin fps \n");
	ret = amba_vin_vsync_calc_fps(&fps_report);

	if (ret < 0)
		return -1;

	return copy_to_user(fps, &fps_report, sizeof(fps_report)) ? -EFAULT : 0;
}


