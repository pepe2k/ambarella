/*
 * kernel/private/drivers/ambarella/fdet/fdet.c
 *
 * History:
 *    2012/06/28 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/timer.h>
#include <amba_common.h>
#include "fdet.h"
#include "utils.h"

static struct amba_fdet_info	*pinfo = NULL;

/* ========================================================================== */
static int fdet_enable(void)
{
	int		i, ret = -1;
	u32		val;

	amba_writel(FDET_ENABLE_REG, FDET_ENABLE);
	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);

	for (i = 0; i < 5; i++) {
		val = amba_readl(FDET_ENABLE_REG);
		if (val == FDET_ENABLE) {
			ret = 0;
			break;
		}
	}

	if (ret) {
		printk("%s: Fail to enable fdet!\n", __func__);
	} else {
		amba_writel(FDET_GO_REG, 0);
		amba_writel(FDET_CONFIG_DONE_REG, 0);
		amba_writel(FDET_RESET_REG, 0);
		amba_writel(FDET_ERROR_STATUS_REG, 0xffff);
	}

	return ret;
}

static int fdet_disable(void)
{
	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_RESET_REG, FDET_RESET);
	amba_writel(FDET_ENABLE_REG, FDET_DISABLE);

	return 0;
}

static void fdet_timer(unsigned long context)
{
	del_timer(&pinfo->timer);

	if (pinfo->vm_state == FDET_VM_STATE_IDLE) {
		pinfo->vm_state = FDET_VM_STATE_READY;
		amba_writel(FDET_CONFIG_DONE_REG, FDET_CONFIG_DONE);
	}
}

static irqreturn_t fdet_isr(int irq, void *dev_data)
{
	int			num = -1;
	unsigned long		tick;

	if (pinfo->config.input_mode == FDET_MODE_STILL) {
		if (pinfo->config.policy & FDET_POLICY_MEASURE_TIME) {
			tick			= jiffies;
			printk("Still used time: %lu ms\n", 1000 * (tick - pinfo->ts_tick) / HZ);
			pinfo->ts_tick	= tick;
		}
		num = fdet_get_result_still();
		FDET_DEBUG("Found %d Faces:\n", num);
	} else {
		switch (pinfo->vm_state) {
		case FDET_VM_STATE_IDLE:
			pinfo->timer.expires = jiffies + FDET_VM_DELAY * HZ / 1000;
			add_timer(&pinfo->timer);
			break;

		case FDET_VM_STATE_READY:
			num = fdet_get_result_video_fs();
			FDET_DEBUG("Found %d Faces:\n", num);
			break;

		case FDET_VM_STATE_RUNNING:
			num = fdet_get_result_video_fs();
			num = fdet_get_result_video_ts();

			if (pinfo->config.policy & FDET_POLICY_MEASURE_TIME) {
				tick			= jiffies;
				printk("Ts used time: %lu ms\n", 1000 * (tick - pinfo->ts_tick) / HZ);
				pinfo->ts_tick	= tick;
			}
			FDET_DEBUG("Found %d Faces:\n", num);
			break;

		default:
			break;
		}
	}

	if (num < 0) {
		goto amba_fdet_isr_exit;
	}

	if (num > 0) {
		memcpy(pinfo->latest_faces, pinfo->merged_faces, num * sizeof(struct fdet_merged_face));
		pinfo->latest_faces_num = num;
		fdet_print_faces();
	}

	complete(&pinfo->result_completion);

amba_fdet_isr_exit:
	amba_writel(FDET_ERROR_STATUS_REG, 0xffffffff);
	return IRQ_HANDLED;
}


/* ========================================================================== */
int fdet_get_sf(int width, int height)
{
	int			num_scales, num_sub_scales, num_total_scales;
	int			si, sr, srr;
	unsigned int 		swi, csw, csh, csf;
	unsigned int		crf;
	long long		csrr;
	unsigned int		sfMant;
	int			sfExpn;
	unsigned short		hwsf_mant;
	unsigned char		hwsf_expn;
	unsigned int		rsf_mant;
	unsigned int		rsf_expn;
	unsigned short		hwrsf_mant;
	unsigned char		hwrsf_expn;
	fdet_scale_factor_reg_t	*p_sf_reg;

	if (width > 320) {
		swi	= 320;
	} else {
		swi	= (width / FDET_TEMPLATE_SIZE) * FDET_TEMPLATE_SIZE;
	}

	sr	= 56988;
	srr	= 75366;

	csw	= swi;
	csh	= (csw * height + (width >> 2)) / width;
	csrr	= (long long)sr;
	for (si = 0; si < 32; si++) {
		csw	= RIGHT_SHIFT_16_ROUND(swi * (int)csrr);
		csh	= (csw * height + (width >> 2)) / width;
		if (csw < FDET_TEMPLATE_SIZE || csh < FDET_TEMPLATE_SIZE) {
			break;
		}
		csrr = RIGHT_SHIFT_16_ROUND(csrr * sr);
	}
	num_scales = si + 1;

	if (num_scales < 1) {
		return 0;
	}

	csrr	= (long long)srr;
	for (si = num_scales; si < 32; si++) {
		csw	= RIGHT_SHIFT_16_ROUND(swi * (int)csrr);
		csh	= (csw * height + (width >> 2)) / width;
		if (csw > width || csh > height) {
			break;
		}
		csrr	= RIGHT_SHIFT_16_ROUND(csrr * srr);
	}
	num_total_scales	= si;
	num_sub_scales		= num_total_scales - num_scales;

	csw	= swi;
	csh	= (csw * height + (width >> 2)) / width;
	csrr	= (long long)sr;
	for (si = num_sub_scales; si < num_total_scales; si++) {
		csf = ((csw << (16 + 1)) + width) / (2 * width);
		pinfo->scale_factor[si] = csf;

		sfMant	= csf;
		sfExpn	= 0;
		while (sfMant < 65536) {
			sfMant = sfMant << 1;
			sfExpn--;
		}
		hwsf_mant = sfMant >> (16 - 10);
		hwsf_expn = abs(sfExpn);

		crf = ((width << (16 + 1)) + csw) / (2 * csw);
		pinfo->recip_scale_factor[si] = crf;

		rsf_mant = crf;
		rsf_expn = 0;
		while (rsf_mant >= 2 * 65536) {
			rsf_mant = rsf_mant >> 1;
			rsf_expn++;
		}
		hwrsf_mant = rsf_mant >> (16 - 10);
		hwrsf_expn = rsf_expn;


		if (hwsf_mant == 1024 && hwsf_expn == 0) {
			pinfo->scale_factor[si]		= 65472;
			pinfo->recip_scale_factor[si]	= 65600;

			hwsf_mant				= 1023;
			hwsf_expn				= 0;
			hwrsf_mant				= 1025;
			hwrsf_expn				= 0;
		}

		p_sf_reg			= &pinfo->scale_factor_regs[si];
		p_sf_reg->s.exponent		= hwsf_expn;
		p_sf_reg->s.mantissa		= hwsf_mant;
		p_sf_reg->s.reciprocal_exponent	= hwrsf_expn;
		p_sf_reg->s.reciprocal_mantissa	= hwrsf_mant;

		csw	= RIGHT_SHIFT_16_ROUND(swi * (int)csrr);
		csh	= (csw * height + (width >> 2 )) / width;
		csrr	= RIGHT_SHIFT_16_ROUND(csrr * sr);
	}

	csrr	= (long long)srr;
	for (si = num_sub_scales - 1; si >= 0; si--) {
		csw	= RIGHT_SHIFT_16_ROUND(swi * (int)csrr);
		csh	= (csw * height + (width >> 2)) / width;
		csf	= ((csw << (16 + 1)) + width) / (2 * width);
		pinfo->scale_factor[si] = csf;

		sfMant = csf;
		sfExpn = 0;
		while (sfMant < 65536) {
			sfMant = sfMant << 1;
			sfExpn--;
		}
		hwsf_mant = sfMant >> (16 - 10);
		hwsf_expn = abs(sfExpn);

		crf = ((width << (16 + 1)) + csw) / (2 * csw);
		pinfo->recip_scale_factor[si] = crf;

		rsf_mant = crf;
		rsf_expn = 0;
		while (rsf_mant >= 2 * 65536) {
			rsf_mant = rsf_mant >> 1;
			rsf_expn++;
		}
		hwrsf_mant = rsf_mant >> (16 - 10);
		hwrsf_expn = rsf_expn;

		if (hwsf_mant == 1024 && hwsf_expn == 0) {
			pinfo->scale_factor[si]		= 65472;
			pinfo->recip_scale_factor[si]	= 65600;

			hwsf_mant				= 1023;
			hwsf_expn				= 0;
			hwrsf_mant				= 1025;
			hwrsf_expn				= 0;
		}

		p_sf_reg			= &pinfo->scale_factor_regs[si];
		p_sf_reg->s.exponent		= hwsf_expn;
		p_sf_reg->s.mantissa		= hwsf_mant;
		p_sf_reg->s.reciprocal_exponent	= hwrsf_expn;
		p_sf_reg->s.reciprocal_mantissa	= hwrsf_mant;

		csrr = RIGHT_SHIFT_16_ROUND(csrr * srr);
	}

	pinfo->num_scales		= num_scales;
	pinfo->num_sub_scales	= num_sub_scales;
	pinfo->num_total_scales	= num_total_scales;

	return num_total_scales;
}

int fdet_get_eval_id(void)
{
	unsigned short			*ptr;
	unsigned char			*ptr2;
	int				num_fs_cls, num_ts_cls, sz_fs, sz_ts;
	int				i;
	unsigned short			stages;
	unsigned int			word;
	int				left_ptr, top_ptr;
	int				addr;
	fdet_classifier_binary_t	*p_binary;
	fdet_classifier_t		*p_cls;

	ptr		= (unsigned short *)pinfo->classifier_binary;
	p_binary	= &pinfo->binary_info;

	p_binary->base	= ptr[17];

	num_fs_cls	= ptr[18];
	sz_fs		= num_fs_cls * sizeof(fdet_classifier_t);
	memcpy(p_binary->fs_cls, &ptr[19], sz_fs);

	num_ts_cls	= ptr[19 + sz_fs / 2];
	sz_ts		= num_ts_cls * sizeof(fdet_classifier_t);
	memcpy(p_binary->ts_cls, &ptr[20 + sz_fs / 2], sz_ts);

	p_cls		= p_binary->fs_cls;
	addr		= 0;
	for (i = 0; i < num_fs_cls; i++) {
		ptr2		= pinfo->classifier_binary + p_binary->base + (p_cls[i].offset << 3);
		left_ptr	= addr + p_cls[i].left_offset;
		top_ptr		= addr + p_cls[i].top_offset;

		word		= (left_ptr << 19) | (top_ptr << 6) | (ptr2[5] & 0x3f);
		ptr2[2]		= (word >> 24) & 0xff;
		ptr2[3]		= (word >> 16) & 0xff;
		ptr2[4]		= (word >>  8) & 0xff;
		ptr2[5]		= (word >>  0) & 0xff;

		stages		= (ptr2[0] << 8) | ptr2[1];
		pinfo->eval_id[i] = addr;
		if (pinfo->config.input_mode == FDET_MODE_STILL) {
			pinfo->eval_num[i] = stages << 16;
		} else {
			pinfo->eval_num[i] = stages;
		}

		addr += p_cls[i].sz;
	}

	p_binary->num_fs_cls	= num_fs_cls;
	p_binary->num_ts_cls	= num_ts_cls;

	return 0;
}

void fdet_get_cmds_video_fs(void)
{
	int				si;
	int				width, height;
	int				row, col, rows, cols;
	unsigned int			center_x, center_y;
	unsigned int			*pfs_cmd_buf = (unsigned int *)pinfo->fs_cmd_buf[pinfo->current_fs_buf_id];
	int				words = 0;
	int				cmd_id = 0, x, y;
	int				rx, ry;
	int				cls_len;
	unsigned int			cls_dram_addr;
	fdet_classifier_t		*p_cls;
	fdet_classifier_load_cmd_t	*p_cls_load_cmd;
	fdet_search_cmd_t		*p_search_cmd;
	fdet_om_t			*pom;

	p_cls		= pinfo->binary_info.fs_cls;
	pom		= &pinfo->om;


	/* Classifier Load */
	p_cls_load_cmd			= (fdet_classifier_load_cmd_t *)pfs_cmd_buf;
	cls_len				= (p_cls[0].sz + p_cls[1].sz + p_cls[2].sz) >> 1;
	cls_dram_addr			= (unsigned int)(pinfo->classifier_binary) + pinfo->binary_info.base + (p_cls[0].offset << 3);

	p_cls_load_cmd->op_code		= OPCODE_CLASSIFIER_LOAD;
	p_cls_load_cmd->classifier_addr	= 0;
	p_cls_load_cmd->length		= cls_len - 1;
	p_cls_load_cmd->dram_addr	= ambarella_virt_to_phys(cls_dram_addr);


	/* Search Command */
	pfs_cmd_buf	+= 2;
	words		+= 2;
	for (si = pinfo->num_sub_scales; si < pinfo->num_total_scales; si++) {
		width	= RIGHT_SHIFT_16_ROUND(pinfo->config.input_width  * pinfo->scale_factor[si]);
		height	= RIGHT_SHIFT_16_ROUND(pinfo->config.input_height * pinfo->scale_factor[si]);
		rows	= (height - FDET_TEMPLATE_SIZE / 2 + FDET_SCALED_PARTITION_SIZE - 1) / FDET_SCALED_PARTITION_SIZE;
		cols	= (width  - FDET_TEMPLATE_SIZE / 2 + FDET_SCALED_PARTITION_SIZE - 1) / FDET_SCALED_PARTITION_SIZE;

		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				center_x			= col * FDET_SCALED_PARTITION_SIZE + FDET_SCALED_PARTITION_SIZE / 2;
				center_y			= row * FDET_SCALED_PARTITION_SIZE + FDET_SCALED_PARTITION_SIZE / 2;
				x				= RIGHT_SHIFT_16_ROUND(center_x * pinfo->recip_scale_factor[si]);
				y				= RIGHT_SHIFT_16_ROUND(center_y * pinfo->recip_scale_factor[si]);
				rx				= (FDET_SCALED_PARTITION_SIZE + FDET_TEMPLATE_SIZE) / 2;
				ry				= (FDET_SCALED_PARTITION_SIZE + FDET_TEMPLATE_SIZE) / 2;

				p_search_cmd			= (fdet_search_cmd_t *)pfs_cmd_buf;

				p_search_cmd->op_code		= OPCODE_SEARCH;
				p_search_cmd->cmd_id		= ++cmd_id;
				p_search_cmd->center_x		= x;
				p_search_cmd->center_y		= y;

				p_search_cmd->num_eval		= pom->num_evals - 1;
				p_search_cmd->merge		= 0;
				p_search_cmd->radius_format	= 1;
				p_search_cmd->si_start		= si;
				p_search_cmd->num_si		= 0;
				p_search_cmd->radius_x		= rx;
				p_search_cmd->radius_y		= ry;

				if (pom->num_evals > 0) {
					p_search_cmd->bit_mask0	= pom->bm[0];
					p_search_cmd->eval_id0	= pom->id[0];
				}

				if (pom->num_evals > 1) {
					p_search_cmd->bit_mask1	= pom->bm[1];
					p_search_cmd->eval_id1	= pom->id[1];
				}

				if (pom->num_evals > 2) {
					p_search_cmd->bit_mask2	= pom->bm[2];
					p_search_cmd->eval_id2	= pom->id[2];
				}

				pfs_cmd_buf	+= (pom->num_evals + 5) / 2;
				words		+= (pom->num_evals + 5) / 2;
			}
		}
	}

	pinfo->current_fs_buf_sz	= words;
	pinfo->last_fs_cmd_id	= cmd_id;
}

void fdet_get_cmds_video_ts(void)
{
	int					i, cmd_id = 0;
	int					x, y ,radius_x, radius_y;
	int					rf, si_start, num_scales;
	int					num_evals;
	int					eval_num;
	int					words = 0;
	int					best_oi;
	int					merge = 0;
	int					size, face_size;
	unsigned int				*pts_cmd_buf;
	struct fdet_adjacent_update_info	*padj_info;

	pts_cmd_buf	= (unsigned int *)pinfo->ts_cmd_buf[pinfo->current_ts_buf_id];

	if (pinfo->config.policy & FDET_POLICY_DISABLE_TS) {
		pinfo->num_faces = 0;
	}

	for (i = 0; i < pinfo->num_faces; i++) {
		x		= pinfo->merged_faces[i].x;
		y		= pinfo->merged_faces[i].y;
		size		= pinfo->merged_faces[i].sz;
		cmd_id		+= 1;
		radius_x	= 10 + search_radius[pinfo->num_faces - 1];
		radius_y	= 10 + search_radius[pinfo->num_faces - 1];
		rf		= 1;

		for (si_start = pinfo->num_sub_scales; ; si_start++) {
			face_size = pinfo->scale_factor[si_start] * size >> 16;
			if (face_size < FDET_TEMPLATE_SIZE) {
				break;
			}
		}
		si_start	-= 2;
		num_scales	= 4;
		if (si_start < 0) {
			si_start = 0;
		}
		if (si_start >= pinfo->num_total_scales) {
			si_start = pinfo->num_total_scales - 1;
		}
		if (num_scales > pinfo->num_total_scales - si_start) {
			num_scales = pinfo->num_total_scales - si_start;
		}

		best_oi	= pinfo->merged_faces[i].best_oi;
		num_evals		= FDET_NUM_EVALS;
		eval_num		= 0;
		padj_info		= fdet_orientation_table[best_oi].adjacent_update_info;

		pts_cmd_buf[words++]	= (0x1 << 0) | (cmd_id << 2) |  ((x & 0x3ff) << 12) | ((y & 0x3ff) << 22);
		pts_cmd_buf[words++]	= ((num_evals - 1) << 0 ) | (merge << 2) | (rf << 3) | (si_start << 4) | (((num_scales - 1) & 0x1f) << 9) | ((radius_x & 0x1ff) << 14) | ((radius_y & 0x1ff) << 23);

		switch (num_evals) {
		case 1:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8);
			break;

		case 2:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			break;

		case 3:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			pts_cmd_buf[words++] = (padj_info[eval_num + 2].orientation_bitmask << 0) | ((padj_info[eval_num + 2].eval_id) << 8);
			break;

		default:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			pts_cmd_buf[words++] = (padj_info[eval_num + 2].orientation_bitmask << 0) | ((padj_info[eval_num + 2].eval_id) << 8) | (padj_info[eval_num + 3].orientation_bitmask << 16) | ((padj_info[eval_num + 3].eval_id) << 24);
			break;
		}

	}

	pinfo->current_ts_buf_sz = words;
}

void fdet_get_cmds_still(void)
{
	unsigned int				*pts_cmd_buf = NULL;
	int					x, y;
	int					radius_x, radius_y;
	int					si_start, num_scales;
	int					cls_len;
	unsigned int				cls_dram_addr;
	fdet_classifier_t			*p_cls;
	fdet_classifier_load_cmd_t		*p_cls_load_cmd;
	fdet_search_cmd_t			*p_search_cmd;
	fdet_om_t				*pom;


	p_cls			= pinfo->binary_info.fs_cls;
	pts_cmd_buf		= (unsigned int *)pinfo->ts_cmd_buf[pinfo->current_ts_buf_id];
	p_cls_load_cmd		= (fdet_classifier_load_cmd_t *)pts_cmd_buf;
	p_search_cmd		= (fdet_search_cmd_t *)&pts_cmd_buf[2];

	/* Classifier Load */
	cls_len			= (p_cls[0].sz + p_cls[1].sz + p_cls[2].sz) >> 1;
	cls_dram_addr		= (unsigned int)(pinfo->classifier_binary) + pinfo->binary_info.base + (p_cls[0].offset << 3);

	p_cls_load_cmd->op_code		= OPCODE_CLASSIFIER_LOAD;
	p_cls_load_cmd->classifier_addr	= 0;
	p_cls_load_cmd->length		= cls_len - 1;
	p_cls_load_cmd->dram_addr	= ambarella_virt_to_phys(cls_dram_addr);

	/* Search Command */
	x		= pinfo->config.input_width / 2;
	y		= pinfo->config.input_height / 2;
	radius_x	= x - 1;
	radius_y	= y - 1;
	si_start	= pinfo->num_sub_scales;
	num_scales	= pinfo->num_scales;

	pom		= &pinfo->om;

	p_search_cmd->op_code		= OPCODE_SEARCH;
	p_search_cmd->cmd_id		= 1;
	p_search_cmd->center_x		= x;
	p_search_cmd->center_y		= y;

	p_search_cmd->num_eval		= pom->num_evals - 1;
	p_search_cmd->merge		= 0;
	p_search_cmd->radius_format	= 0;
	p_search_cmd->si_start		= si_start;
	p_search_cmd->num_si		= num_scales - 1;
	p_search_cmd->radius_x		= radius_x;
	p_search_cmd->radius_y		= radius_y;

	p_search_cmd->bit_mask0		= pom->bm[0];
	p_search_cmd->eval_id0		= pom->id[0];

	p_search_cmd->bit_mask1		= pom->bm[1];
	p_search_cmd->eval_id1		= pom->id[1];

	p_search_cmd->bit_mask2		= pom->bm[2];
	p_search_cmd->eval_id2		= pom->id[2];

	pinfo->current_ts_buf_sz	= 4 + (pom->num_evals + 1) / 2;
}

int fdet_is_the_same_face(int i, int j, int merged)
{
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face	*pm;
	int				distance;

	pu		= pinfo->unmerged_faces;
	pm		= pinfo->merged_faces;

	if (!merged) {
		distance	= pu[i].sz >> 2;

		if (	(pu[j].x  < pu[i].x + distance) &&
			(pu[j].x  > pu[i].x - distance) &&
			(pu[j].y  < pu[i].y + distance) &&
			(pu[j].y  > pu[i].y - distance) &&
			(pu[j].sz < (pu[i].sz * 5) >> 2) &&
			(pu[i].sz < (pu[j].sz * 5) >> 2) ) {
			return 1;
		} else {
			return 0;
		}
	} else {
		distance	= pm[i].sz >> 2;

		if (	(pm[j].x  < pm[i].x + distance) &&
			(pm[j].x  > pm[i].x - distance) &&
			(pm[j].y  < pm[i].y + distance) &&
			(pm[j].y  > pm[i].y - distance) &&
			(pm[j].sz < (pm[i].sz * 5) >> 2) &&
			(pm[i].sz < (pm[j].sz * 5) >> 2) ) {
			return 1;
		} else {
			return 0;
		}
	}
}

int fdet_merge_faces(int unmerged_faces)
{
	int				i, j;
	int				cluster, clusters;
	int				hs, ths;
	int				max_hitcount;
	int				si, max_scales;
	unsigned short			best_oi;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		*pm;

	pu = pinfo->unmerged_faces;
	pm = pinfo->merged_faces;

	clusters = 0;
	for (i = 0; i < unmerged_faces; i++) {

		if (pu[i].cluster < 0) {
			pu[i].cluster	= clusters;

			pm[clusters].x			 = pu[i].x;
			pm[clusters].y			 = pu[i].y;
			pm[clusters].num_si[pu[i].si]	+= 1;
			pm[clusters].hit_count[pu[i].oi]+= 1;
			pm[clusters].hit_sum		 = 1;

			if (++clusters >= FDET_MAX_MERGED_FACES) {
				break;
			}
		}

		for (j = i + 1; j < unmerged_faces; j++) {
			if (pu[j].cluster >= 0) {
				continue;
			}

			if (!fdet_is_the_same_face(j, i, 0)) {
				continue;
			}

			cluster				= pu[i].cluster;
			pu[j].cluster			= cluster;

			pm[cluster].x			+= pu[j].x;
			pm[cluster].y			+= pu[j].y;
			pm[cluster].num_si[pu[j].si]	+= 1;
			pm[cluster].hit_count[pu[j].oi]	+= 1;
			pm[cluster].hit_sum		+= 1;
		}
	}

	for (i = 0; i < clusters; i++) {
		hs		= pm[i].hit_sum;
		ths		= hs << 1;
		pm[i].x		= (2 * pm[i].x  + hs) / ths;
		pm[i].y		= (2 * pm[i].y  + hs) / ths;

		si		= 0;
		max_scales	= 0;
		for (j = pinfo->num_sub_scales; j < pinfo->num_total_scales; j++) {
			if (pm[i].num_si[j] > max_scales) {
				max_scales	= pm[i].num_si[j];
				si		= j;
			}
		}
		pm[i].sz	= RIGHT_SHIFT_16_ROUND(FDET_TEMPLATE_SIZE * pinfo->recip_scale_factor[si]);
		pm[i].x		-= pinfo->recip_scale_factor[si] >> 18;
		pm[i].y		+= pinfo->recip_scale_factor[si] >> 18;

		max_hitcount	= 0;
		best_oi		= 0;
		for (j = 0; j < 8; j++) {
			if (pm[i].hit_count[j] > max_hitcount) {
				max_hitcount	= pm[i].hit_count[j];
				best_oi		= j;
			}
		}
		pm[i].best_oi		= best_oi;
		pm[i].best_hitcount	= max_hitcount;
		pm[i].type		= FDET_RESULT_TYPE_FS;
	}

	return clusters;
}

struct fdet_merged_face fdet_merge_ts_faces(int unmerged_faces)
{
	int				i, j, k;
	int				oi;
	int				x1, x2, y1, y2;
	int				xl, xr, yl, yh;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		merged_faces;

	pu = pinfo->ts_unmerged_faces;
	memset(&merged_faces, 0, sizeof(merged_faces));

	if (unmerged_faces <= 0) {
		return merged_faces;
	}

	xl = -10000;
	xr = +10000;
	yl = -10000;
	yh = +10000;

	for (i = 0, j = 0, k = 0; i < unmerged_faces; i++) {
		if (!k) {
			x1 = pu[i].x - pu[i].sz / 2;
			x2 = pu[i].x + pu[i].sz / 2;
			y1 = pu[i].y - pu[i].sz / 2;
			y2 = pu[i].y + pu[i].sz / 2;

			x1 = max(xl, x1);
			x2 = min(xr, x2);
			y1 = max(yl, y1);
			y2 = min(yh, y2);

			if (x2 >= x1 && y2 >= y1) {
				xl = x1;
				xr = x2;
				yl = y1;
				yh = y2;
				j++;
			} else {
				k = 1;
			}
		}

		merged_faces.sz	+= pu[i].sz;
		oi		=  pu[i].oi;
		merged_faces.hit_count[oi]++;
		merged_faces.hit_sum++;
	}


	merged_faces.x	= (xl + xr + 1) / 2;
	merged_faces.y	= (yl + yh + 1) / 2;
	merged_faces.sz	= (2 * merged_faces.sz + unmerged_faces) / (unmerged_faces << 1);

	for (i = 0; i < 8; i++) {
		if (merged_faces.hit_count[i] > merged_faces.best_hitcount) {
			merged_faces.best_hitcount = merged_faces.hit_count[i];
			merged_faces.best_oi = i;
		}
	}

	return merged_faces;
}

int fdet_is_valid_hitcount(unsigned int *hit_count)
{
	int				o;

	for (o = 0; o < 8; o++) {
		if (hit_count[o] >=  pinfo->config.min_hitcounts[o]) {
			return 1;
		}
	}

	return 0;
}

int fdet_filter_merged_faces(int merged_faces)
{
	int				i, faces, valid;
	struct fdet_merged_face		*pm;

	pm	= pinfo->merged_faces;
	faces	= 0;

	for (i = 0; i < merged_faces; i++) {
		valid = fdet_is_valid_hitcount(pm[i].hit_count);
		if (!valid) {
			continue;
		}

		pm[faces] = pm[i];
		faces++;
	}

	return faces;
}

int fdet_eliminate_overlap_faces(int merged_faces)
{
	struct fdet_merged_face		*pm;
	int				dx, dy;
	int				distance, threshold;
	int				i, j;

	pm	= pinfo->merged_faces;

	for (i = 0; i < merged_faces; i++) {
		if (!pm[i].sz) {
			continue;
		}

		for (j = i + 1; j < merged_faces; j++) {
			if (!pm[j].sz) {
				continue;
			}

			dx		= abs((int)pm[i].x - (int)pm[j].x);
			dy		= abs((int)pm[i].y - (int)pm[j].y);
			threshold	= max(pm[i].sz, pm[j].sz) >> 1;

			if (dx >= dy) {
				if (dx > (dy << 2)) {
					distance = (dx * 66047) >> 16;
				} else if (dx > (dy << 1)) {
					distance = (dx * 69992) >> 16;
				} else if (3 * dx > (dy << 2)) {
					distance = (dx * 77280) >> 16;
				} else {
					distance = (dx * 87084) >> 16;
				}
			} else {
				if (dy > (dx << 2)) {
					distance = (dy * 66047) >> 16;
				} else if (dy > (dx << 1)) {
					distance = (dy * 69992) >> 16;
				} else if (3 *dy > (dx << 2)) {
					distance = (dy * 77280) >> 16;
				} else {
					distance = (dy * 87084) >> 16;
				}
			}

			if (distance < threshold) {
				if (pm[j].best_hitcount <= pm[i].best_hitcount) {
					pm[j].sz = 0;
				} else {
					pm[i].sz = 0;
				}
			}
		}
	}

	for (i = 0, j = 0; i < merged_faces; i++) {
		if (pm[i].sz) {
			pm[j++] = pm[i];
		}
	}

	return j;
}

void fdet_print_faces(void)
{
	struct fdet_merged_face		*pm;
	int				i;

	pm = pinfo->merged_faces;

	for (i = 0; i < pinfo->num_faces; i++) {
		FDET_DEBUG("\tFace %2d: (%3d, %3d), Size: %3d\n", i, pm[i].x, pm[i].y, pm[i].sz);
	}
}

int fdet_get_result_still(void)
{
	unsigned int			*result_buf, words;
	unsigned int			i, unmerged_faces, merged_faces;
	struct fdet_unmerged_face	*pu;
	fdet_result_num_reg_t		result_num;
	fdet_error_status_reg_t		error_status;
	fdet_result_t			*p_result;

	result_buf	= (unsigned int *)pinfo->ts_result_buf[pinfo->current_ts_result_id ^ 1];

	result_num.w	= amba_readl(FDET_RESULT_STATUS_REG);
	error_status.w	= amba_readl(FDET_ERROR_STATUS_REG);
	words		= result_num.s.ts_num;

	if (error_status.s.ts_result_overflow || words > FDET_TS_RESULT_BUF_WORDS) {
		printk("%s: Ts Result Overflow!\n", __func__);
		words = FDET_TS_RESULT_BUF_WORDS;
	}

	memset(pinfo->unmerged_faces, 0, sizeof(pinfo->unmerged_faces));
	memset(pinfo->merged_faces,   0, sizeof(pinfo->merged_faces));
	unmerged_faces		= 0;
	pinfo->num_faces	= 0;

	/* Read Unmerged Faces */
	invalidate_d_cache(result_buf, FDET_TS_RESULT_BUF_SIZE);
	for (i = 0; i < words; i += 2) {
		p_result	= (fdet_result_t *)&result_buf[i];
		if (p_result->type != RESULT_TYPE_UNMERGED_FACES) {
			continue;
		}
		if (p_result->cmd_id == 0) {
			continue;
		}

		if (unmerged_faces >= FDET_MAX_UNMERGED_FACES) {
			unmerged_faces = FDET_MAX_UNMERGED_FACES - 1;
		}

		pu		= &pinfo->unmerged_faces[unmerged_faces];
		pu->x		= p_result->x;
		pu->y		= p_result->y;
		pu->sz		= RIGHT_SHIFT_16_ROUND(FDET_TEMPLATE_SIZE * pinfo->recip_scale_factor[p_result->si]);
		pu->si		= p_result->si;
		pu->oi		= p_result->oi;
		pu->cluster	= -1;

		unmerged_faces++;
	}
	FDET_DEBUG("Unmerged Faces: %d\n", unmerged_faces);

	/* Merge Faces */
	merged_faces		= fdet_merge_faces(unmerged_faces);
	pinfo->num_faces	= merged_faces;
	FDET_DEBUG("Merged Faces before Hitcount Check: %d\n", merged_faces);

	/* Filter Faces */
	merged_faces		= fdet_filter_merged_faces(merged_faces);
	pinfo->num_faces	= merged_faces;
	FDET_DEBUG("Merged Faces after  Hitcount Check: %d\n", merged_faces);

	/* Eliminate Overlapping Faces */
	merged_faces		= fdet_eliminate_overlap_faces(merged_faces);
	pinfo->num_faces	= merged_faces;
	FDET_DEBUG("Merged Faces: %d\n", merged_faces);

	return merged_faces;
}

int fdet_get_result_video_fs(void)
{
	unsigned int			*result_buf, words;
	unsigned int			i, unmerged_faces, merged_faces = 0;
	struct fdet_unmerged_face	*pu;
	int				end = -1;
	static unsigned int		offset = 0;
	unsigned long			tick;
	fdet_result_num_reg_t		result_num;
	fdet_error_status_reg_t		error_status;
	fdet_result_t			*p_result;

	result_num.w	= amba_readl(FDET_RESULT_STATUS_REG);
	error_status.w	= amba_readl(FDET_ERROR_STATUS_REG);
	words		= result_num.s.fs_num;

	if (error_status.s.fs_result_overflow || words > FDET_FS_RESULT_BUF_WORDS) {
		printk("%s: Fs Result Overflow!\n", __func__);
		words = FDET_FS_RESULT_BUF_WORDS;
	}

	unmerged_faces		= offset;
	pinfo->num_faces	= 0;
	memset(pinfo->merged_faces, 0, sizeof(pinfo->merged_faces));

	result_buf	= (unsigned int *)pinfo->fs_result_buf[pinfo->current_fs_result_id];
	invalidate_d_cache(result_buf, FDET_FS_RESULT_BUF_SIZE);
	for (i = 0; i < FDET_FS_RESULT_BUF_WORDS; i += 2) {
		p_result	= (fdet_result_t *)&result_buf[i];

		if (p_result->type == RESULT_TYPE_UNMERGED_FACES && p_result->cmd_id) {
			pu		= &pinfo->unmerged_faces[unmerged_faces];

			pu->x		= p_result->x;
			pu->y		= p_result->y;
			pu->sz		= RIGHT_SHIFT_16_ROUND(FDET_TEMPLATE_SIZE * pinfo->recip_scale_factor[p_result->si]);
			pu->si		= p_result->si;
			pu->oi		= p_result->oi;
			pu->cluster	= -1;

			unmerged_faces++;
		}

		if (p_result->type == RESULT_TYPE_COMMAND_STATUS) {
			if (p_result->cmd_id == pinfo->last_fs_cmd_id) {
				end = unmerged_faces;
			}
		}
	}
	memset((void *)result_buf, 0, FDET_FS_RESULT_BUF_SIZE);
	clean_d_cache(result_buf, FDET_FS_RESULT_BUF_SIZE);

	result_buf	= (unsigned int *)pinfo->fs_result_buf[pinfo->current_fs_result_id ^ 1];
	invalidate_d_cache(result_buf, FDET_FS_RESULT_BUF_SIZE);
	for (i = 0; i < words; i += 2) {
		p_result	= (fdet_result_t *)&result_buf[i];

		if (p_result->type == RESULT_TYPE_UNMERGED_FACES && p_result->cmd_id) {
			pu		= &pinfo->unmerged_faces[unmerged_faces];

			pu->x		= p_result->x;
			pu->y		= p_result->y;
			pu->sz		= RIGHT_SHIFT_16_ROUND(FDET_TEMPLATE_SIZE * pinfo->recip_scale_factor[p_result->si]);
			pu->si		= p_result->si;
			pu->oi		= p_result->oi;
			pu->cluster	= -1;

			unmerged_faces++;
		}

		if (p_result->type == RESULT_TYPE_COMMAND_STATUS) {
			if (p_result->cmd_id == pinfo->last_fs_cmd_id) {
				end = unmerged_faces;
			}
		}
	}
	memset((void *)result_buf, 0, words << 2);
	clean_d_cache((void *)result_buf, words << 2);

	if (end >= 0) {
		pinfo->fs_found_faces[0] = pinfo->fs_found_faces[1];
		pinfo->fs_found_faces[1] = 0;
	}

	if (end >= 0 && pinfo->config.policy & FDET_POLICY_MEASURE_TIME) {
		tick			= jiffies;
		printk("Fs used time: %lu ms\n", 1000 * (tick - pinfo->fs_tick) / HZ);
		pinfo->fs_tick	= tick;
	}

	FDET_DEBUG("Unmerged Faces: %d\n", unmerged_faces);

	if ((pinfo->config.policy & FDET_POLICY_WAIT_FS_COMPLETE) == 0) {
		end = unmerged_faces;
	}

	if (end < 0) {
		offset = unmerged_faces;
		goto fdet_get_result_video_fs_exit;
	}

	merged_faces = fdet_merge_faces(end);
	pinfo->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces before Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_filter_merged_faces(merged_faces);
	pinfo->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces after  Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_eliminate_overlap_faces(merged_faces);
	pinfo->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces: %d\n", merged_faces);

	if (merged_faces) {
		pinfo->fs_found_faces[1]++;
	}

	offset	= unmerged_faces - end;
	pu	= pinfo->unmerged_faces;
	memcpy(pu, &pu[end], offset * sizeof(struct fdet_unmerged_face));

fdet_get_result_video_fs_exit:
	return merged_faces;
}

int fdet_get_result_video_ts(void)
{
	int				words, i, hit;
	unsigned int			wi;
	int				faces = pinfo->num_faces;
	unsigned int			*result_buf;
	int				unmerged_faces = 0;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		*pm, *pl;
	fdet_result_num_reg_t		result_num;
	fdet_error_status_reg_t		error_status;
	fdet_result_t			*p_result;

	result_num.w	= amba_readl(FDET_RESULT_STATUS_REG);
	error_status.w	= amba_readl(FDET_ERROR_STATUS_REG);
	words		= result_num.s.ts_num;

	pm		= pinfo->merged_faces;
	pl		= pinfo->latest_faces;

	FDET_DEBUG("%s: result status: 0x%08x, error status: 0x%08x\n", __func__, result_num.w, error_status.w);

	if (error_status.s.ts_too_long || error_status.s.ts_merge_overflow) {
		goto fdet_get_result_video_ts_exit;
	}

	if (error_status.s.ts_result_overflow || words > FDET_TS_RESULT_BUF_WORDS) {
		words = FDET_TS_RESULT_BUF_WORDS;
	}

	result_buf = (unsigned int *)pinfo->ts_result_buf[pinfo->current_ts_result_id ^ 1];
	invalidate_d_cache(result_buf, FDET_TS_RESULT_BUF_SIZE);
	for (wi = 0; wi < words; wi += 2) {
		p_result = (fdet_result_t *)&result_buf[wi];

		if (!p_result->cmd_id) {
			continue;
		}

		if (p_result->type == RESULT_TYPE_UNMERGED_FACES) {

			pu		= &pinfo->ts_unmerged_faces[unmerged_faces];

			pu->x		= p_result->x;
			pu->y		= p_result->y;
			pu->sz		= RIGHT_SHIFT_16_ROUND(FDET_TEMPLATE_SIZE * pinfo->recip_scale_factor[p_result->si]);
			pu->si		= p_result->si;
			pu->oi		= p_result->oi;
			pu->cluster	= -1;

			unmerged_faces++;
		}

		if (p_result->type == RESULT_TYPE_COMMAND_STATUS) {
			struct fdet_merged_face		merged_faces;

			merged_faces = fdet_merge_ts_faces(unmerged_faces);

			if (merged_faces.hit_sum >= 15 && pinfo->fs_found_faces[0]) {
				if (abs(merged_faces.x - pl[p_result->cmd_id - 1].x) < merged_faces.sz / FDET_TEMPLATE_SIZE &&
					abs(merged_faces.y - pl[p_result->cmd_id - 1].y) < merged_faces.sz / FDET_TEMPLATE_SIZE &&
					5 * merged_faces.sz > 4 * pl[p_result->cmd_id - 1].sz &&
					4 * merged_faces.sz < 5 * pl[p_result->cmd_id - 1].sz) {
					pm[faces].x		= pl[p_result->cmd_id - 1].x;
					pm[faces].y		= pl[p_result->cmd_id - 1].y;
					pm[faces].sz		= pl[p_result->cmd_id - 1].sz;
					pm[faces].best_oi	= merged_faces.best_oi;
				} else {
					pm[faces]		= merged_faces;
				}

				hit = 0;
				for (i = 0; i < pinfo->num_faces; i++) {
					if (fdet_is_the_same_face(i, faces, 1)) {
						hit = 1;
						break;
					}
				}

				if (!hit) {
					pm[faces].type		= FDET_RESULT_TYPE_TS;
					faces++;
				}
			}

			unmerged_faces = 0;
			pu		= pinfo->ts_unmerged_faces;
			memset(pu, 0, sizeof(pinfo->unmerged_faces));
		}
	}

	if (faces) {
		faces = fdet_eliminate_overlap_faces(faces);
	}

	pinfo->num_faces = faces;

fdet_get_result_video_ts_exit:
	return faces;
}

static int fdet_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int fdet_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int fdet_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int			ret;

	switch (pinfo->mmap_type) {
	case FDET_MMAP_TYPE_ORIG_BUFFER:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pinfo->orig_target_buf) >> PAGE_SHIFT;
		pinfo->orig_len = vma->vm_end - vma->vm_start;
		FDET_DEBUG("%s: Orig Buffer Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;

	case FDET_MMAP_TYPE_TMP_BUFFER:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pinfo->tmp_target_buf) >> PAGE_SHIFT;
		FDET_DEBUG("%s: Tmp Buffer Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;

	default:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pinfo->classifier_binary) >> PAGE_SHIFT;
		pinfo->cls_bin_len = vma->vm_end - vma->vm_start;
		FDET_DEBUG("%s: Classifier Binary Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;
	}

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);

	return ret;
}

void fdet_config(void)
{
	char			*pfs_result_buf;
	char			*pts_cmd_buf, *pts_result_buf;
	char			*ptmp_target_buf;
	unsigned int		*p2;
	u32			p[6];
	int			i;

	FDET_DEBUG("%s: \n", __func__);

	pfs_result_buf	= pinfo->fs_result_buf[pinfo->current_fs_result_id];
	pts_cmd_buf	= pinfo->ts_cmd_buf[pinfo->current_ts_buf_id];
	pts_result_buf	= pinfo->ts_result_buf[pinfo->current_ts_result_id];
	ptmp_target_buf	= pinfo->tmp_target_buf + (FDET_TMP_TARGET_BUF_SIZE / 2) * pinfo->current_tmp_target_id;
	p2		= (unsigned int *)pts_cmd_buf;

	p[1]		= ambarella_virt_to_phys((u32)pfs_result_buf);
	p[2]		= ambarella_virt_to_phys((u32)pts_cmd_buf);
	p[3]		= ambarella_virt_to_phys((u32)pts_result_buf);
	p[4]		= ambarella_virt_to_phys((u32)pinfo->orig_target_buf);
	p[5]		= ambarella_virt_to_phys((u32)ptmp_target_buf);

	FDET_DEBUG("\tTracking Search Command Buffer Size: %d\n", pinfo->current_ts_buf_sz);
	for (i = 0; i < pinfo->current_ts_buf_sz; i++) {
		FDET_DEBUG("\t\tTs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	clean_d_cache(pts_cmd_buf, pinfo->current_ts_buf_sz << 2);

	amba_writel(FDET_FS_RESULT_BUF_PTR_REG, p[1]);
	amba_writel(FDET_FS_RESULT_BUF_SIZE_REG, (FDET_FS_RESULT_BUF_SIZE >> 2) - 1);
	FDET_DEBUG("\t fs result buffer pointer:   0x%08x\n", p[1]);

	amba_writel(FDET_TS_CMD_LIST_PTR_REG, p[2]);
	if (pinfo->current_ts_buf_sz) {
		amba_writel(FDET_TS_CMD_LIST_SIZE_REG, pinfo->current_ts_buf_sz - 1);
	} else {
		amba_writel(FDET_TS_CMD_LIST_SIZE_REG, 0);
	}
	FDET_DEBUG("\t ts cmd list pointer:        0x%08x\n", p[2]);

	memset(pts_result_buf, 0, FDET_TS_RESULT_BUF_SIZE);
	clean_d_cache(pts_result_buf, FDET_TS_RESULT_BUF_SIZE);
	amba_writel(FDET_TS_RESULT_BUF_PTR_REG, p[3]);
	amba_writel(FDET_TS_RESULT_BUF_SIZE_REG, (FDET_TS_RESULT_BUF_SIZE >> 2) - 1);
	FDET_DEBUG("\t ts result buffer pointer:   0x%08x\n", p[3]);

	if (pinfo->config.input_source) {
		amba_writel(FDET_ORIG_TARGET_PTR_REG, get_ambarella_dspmem_phys() + pinfo->config.input_offset);
		amba_writel(FDET_ORIG_TARGET_PITCH_REG, pinfo->config.input_pitch);
		FDET_DEBUG("\t orig buffer pointer:        0x%08x\n", get_ambarella_dspmem_phys() + pinfo->config.input_offset);
	} else {
		amba_writel(FDET_ORIG_TARGET_PTR_REG, p[4]);
		amba_writel(FDET_ORIG_TARGET_PITCH_REG, pinfo->config.input_pitch);
		FDET_DEBUG("\t orig buffer pointer:        0x%08x\n", p[4]);
		clean_d_cache(pinfo->orig_target_buf, pinfo->orig_len);
	}

	memset(ptmp_target_buf, 0, FDET_TMP_TARGET_BUF_SIZE / 2);
	clean_d_cache(ptmp_target_buf, FDET_TMP_TARGET_BUF_SIZE / 2);
	amba_writel(FDET_TMP_TARGET_PTR_REG, p[5]);
	FDET_DEBUG("\t tmp buffer pointer:         0x%08x\n", p[5]);

	pinfo->current_fs_result_id	^= 1;
	pinfo->current_ts_buf_id		^= 1;
	pinfo->current_ts_result_id	^= 1;
	pinfo->current_tmp_target_id	^= 1;
	pinfo->current_ts_buf_sz		 = 0;

	amba_writel(FDET_CONFIG_DONE_REG, FDET_CONFIG_DONE);
}

static int fdet_start_video(unsigned long arg)
{
	char			*pfs_cmd_buf;
	unsigned int		*p2;
	u32			p;
	int			i, ret;

	FDET_DEBUG("%s: \n", __func__);

	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_INPUT_WIDTH_REG, pinfo->config.input_width - 1);
	amba_writel(FDET_INPUT_HEIGHT_REG, pinfo->config.input_height - 1);
	FDET_DEBUG("\t input width: %d, input height: %d\n",
		pinfo->config.input_width, pinfo->config.input_height);
	amba_writel(FDET_DEADLINE_REG, 1);
	amba_writel(FDET_SKIP_FIRST_INT_REG, FDET_SKIP_FIRST_INTERRUPT);

	/* Scale Factors */
	ret = fdet_get_sf(pinfo->config.input_width, pinfo->config.input_height);
	if (ret <= 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Scales: %d, Sub Face Scales: %d\n", pinfo->num_scales, pinfo->num_sub_scales);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_SCALE_FACTOR_REG(i), pinfo->scale_factor_regs[i].w);
		FDET_DEBUG("\t\tScale Factor[%02d]: 0x%08x\n", i, pinfo->scale_factor_regs[i].w);
	}

	/* Evaluation IDs */
	ret = fdet_get_eval_id();
	if (ret < 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Fs Classifiers: %d, Face Ts Classifiers: %d\n",
		pinfo->binary_info.num_fs_cls,
		pinfo->binary_info.num_ts_cls);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_ID_REG(i), pinfo->eval_id[i]);
		FDET_DEBUG("\t\tEvaluation  ID[%02d]: 0x%08x\n", i, pinfo->eval_id[i]);
	}
#ifdef FDET_HAVE_EVAL_NUM_REGS
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_NUM_REG(i), pinfo->eval_num[i]);
		FDET_DEBUG("\t\tEvaluation NUM[%02d]: 0x%08x\n", i, pinfo->eval_num[i]);
	}
#endif

	/* Full Search Command Buffer */
	pfs_cmd_buf	= pinfo->fs_cmd_buf[pinfo->current_fs_buf_id];
	p2		= (unsigned int *)pfs_cmd_buf;
	p		= ambarella_virt_to_phys((u32)pfs_cmd_buf);
	fdet_get_cmds_video_fs();
	pinfo->current_fs_buf_id	^= 1;
	FDET_DEBUG("\tFull Search Command Buffer Size: %d\n", pinfo->current_fs_buf_sz);
	for (i = 0; i < 32; i++) {
		FDET_DEBUG("\t\tFs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	for (i = pinfo->current_fs_buf_sz - 32; i < pinfo->current_fs_buf_sz; i++) {
		FDET_DEBUG("\t\tFs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	clean_d_cache(pfs_cmd_buf, pinfo->current_fs_buf_sz << 2);
	clean_d_cache(pinfo->classifier_binary, pinfo->cls_bin_len);

	amba_writel(FDET_FS_CMD_LIST_PTR_REG, p);
	amba_writel(FDET_FS_CMD_LIST_SIZE_REG, pinfo->current_fs_buf_sz - 1);
	FDET_DEBUG("\t fs cmd list pointer:        0x%08x\n", p);

	pinfo->current_ts_buf_sz = 0;
	fdet_config();

	pinfo->vm_state = FDET_VM_STATE_IDLE;
	init_completion(&pinfo->result_completion);
	pinfo->fs_tick = jiffies;

	amba_writel(FDET_GO_REG, FDET_START);

	return 0;
}

static int fdet_start_still(unsigned long arg)
{
	char			*pfs_cmd_buf;
	u32			p;
	int			i, ret;

	FDET_DEBUG("%s: \n", __func__);

	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_INPUT_WIDTH_REG, pinfo->config.input_width - 1);
	amba_writel(FDET_INPUT_HEIGHT_REG, pinfo->config.input_height - 1);
	FDET_DEBUG("\t input width: %d, input height: %d\n",
		pinfo->config.input_width, pinfo->config.input_height);
	amba_writel(FDET_DEADLINE_REG, 0);
	amba_writel(FDET_SKIP_FIRST_INT_REG, FDET_SKIP_FIRST_INTERRUPT);

	/* Scale Factors */
	ret = fdet_get_sf(pinfo->config.input_width, pinfo->config.input_height);
	if (ret <= 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Scales: %d, Sub Face Scales: %d\n", pinfo->num_scales, pinfo->num_sub_scales);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_SCALE_FACTOR_REG(i), pinfo->scale_factor_regs[i].w);
		FDET_DEBUG("\t\tScale Factor[%02d]: 0x%08x\n", i, pinfo->scale_factor_regs[i].w);
	}

	/* Evaluation IDs */
	ret = fdet_get_eval_id();
	if (ret < 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Fs Classifiers: %d, Face Ts Classifiers: %d\n",
		pinfo->binary_info.num_fs_cls,
		pinfo->binary_info.num_ts_cls);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_ID_REG(i), pinfo->eval_id[i]);
		FDET_DEBUG("\t\tEvaluation  ID[%02d]: 0x%08x\n", i, pinfo->eval_id[i]);
	}
#ifdef FDET_HAVE_EVAL_NUM_REGS
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_NUM_REG(i), pinfo->eval_num[i]);
		FDET_DEBUG("\t\tEvaluation NUM[%02d]: 0x%08x\n", i, pinfo->eval_num[i]);
	}
#endif

	/* Tracking Search Command Buffer */
	fdet_get_cmds_still();
	clean_d_cache(pinfo->classifier_binary, pinfo->cls_bin_len);

	pfs_cmd_buf	= pinfo->fs_cmd_buf[pinfo->current_fs_buf_id];
	p		= ambarella_virt_to_phys((u32)pfs_cmd_buf);
	pinfo->current_fs_buf_id	^= 1;
	amba_writel(FDET_FS_CMD_LIST_PTR_REG, p);
	amba_writel(FDET_FS_CMD_LIST_SIZE_REG, 0);
	FDET_DEBUG("\t fs cmd list pointer:        0x%08x\n", p);

	init_completion(&pinfo->result_completion);
	fdet_config();
	pinfo->ts_tick = jiffies;
	amba_writel(FDET_GO_REG, FDET_START);

	return 0;
}

static int fdet_stop(unsigned long arg)
{
	amba_writel(FDET_GO_REG, FDET_STOP);
	complete_all(&pinfo->result_completion);

	return 0;
}

static int fdet_get_result(unsigned long arg)
{
	struct fdet_merged_face		*pm;
	int				i, ret;
	struct fdet_face		faces[FDET_MAX_FACES];

	wait_for_completion_interruptible(&pinfo->result_completion);

	pm = pinfo->merged_faces;

	for (i = 0; i < pinfo->num_faces; i++) {
		faces[i].x	= pm[i].x;
		faces[i].y	= pm[i].y;
		faces[i].size	= pm[i].sz;
		faces[i].type	= pm[i].type;
		faces[i].name[0]= '\0';
	}

	ret = copy_to_user((void *)arg, faces, i * sizeof(struct fdet_face));
	if (ret < 0) {
		i = -EINVAL;
	}

	return i;
}

static int fdet_set_mmap_type(unsigned long arg)
{
	switch (arg) {
	case FDET_MMAP_TYPE_ORIG_BUFFER:
	case FDET_MMAP_TYPE_TMP_BUFFER:
	case FDET_MMAP_TYPE_CLASSIFIER_BINARY:
		pinfo->mmap_type = arg;
		return 0;

	default:
		printk("%s: Invalid mmap type!\n", __func__);
		return -EINVAL;
	}
}

static int fdet_set_configuration(unsigned long arg)
{
	int				i, ret = 0;
	struct fdet_configuration	cfg;
	fdet_om_t			*pom;

	ret = copy_from_user(&cfg, (void *)arg, sizeof(cfg));
	if (ret < 0) {
		printk("%s: Error occurred when copying argument from user space!\n", __func__);
		goto fdet_set_configuration_exit;
	}

	if (cfg.input_width > FDET_MAX_INPUT_WIDTH || cfg.input_height > FDET_MAX_INPUT_HEIGHT) {
		printk("%s: Invalid input width or height!\n", __func__);
		ret = -EINVAL;
		goto fdet_set_configuration_exit;
	}

	if (cfg.input_mode != FDET_MODE_VIDEO && cfg.input_mode != FDET_MODE_STILL) {
		printk("%s: Invalid input mode!\n", __func__);
		ret = -EINVAL;
		goto fdet_set_configuration_exit;
	}

	memcpy(&pinfo->config, &cfg, sizeof(cfg));
	pom		= &pinfo->om;
	pom->num_evals	= 0;
	for (i = 0; i < 3; i++) {
		if (!cfg.om[i]) {
			continue;
		}

		pom->bm[pom->num_evals]	= cfg.om[i];
		pom->id[pom->num_evals]	= i;
		pom->num_evals++;
	}
	if (!pom->num_evals) {
		pom->num_evals	= 1;
		pom->bm[0]	= 0x01;
		pom->id[0]	= 0;
	}

	for (i = 0; i < 8; i++) {
		if (!pinfo->config.min_hitcounts[i]) {
			pinfo->config.min_hitcounts[i] = 7;
		}
	}

fdet_set_configuration_exit:
	return ret;
}

static int fdet_track_face(unsigned long arg)
{
	if (pinfo->vm_state == FDET_VM_STATE_IDLE) {
		return -EINVAL;
	} else {
		pinfo->vm_state = FDET_VM_STATE_RUNNING;
	}

	pinfo->config.input_offset = arg;
	fdet_get_cmds_video_ts();
	pinfo->ts_tick = jiffies;
	fdet_config();

	return 0;
}

static long fdet_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int		ret = -ENOIOCTLCMD;

	switch (cmd) {
	case FDET_IOCTL_START:
		if (pinfo->config.input_mode == FDET_MODE_VIDEO) {
			ret = fdet_start_video(arg);
		} else {
			ret = fdet_start_still(arg);
		}
		break;

	case FDET_IOCTL_STOP:
		ret = fdet_stop(arg);
		break;

	case FDET_IOCTL_GET_RESULT:
		ret = fdet_get_result(arg);
		break;

	case FDET_IOCTL_SET_MMAP_TYPE:
		ret = fdet_set_mmap_type(arg);
		break;

	case FDET_IOCTL_SET_CONFIGURATION:
		ret = fdet_set_configuration(arg);
		break;

	case FDET_IOCTL_TRACK_FACE:
		ret = fdet_track_face(arg);
		break;

	default:
		break;
	}

	return ret;
}

static struct file_operations fdet_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl		= fdet_ioctl,
	.mmap			= fdet_mmap,
	.open			= fdet_open,
	.release		= fdet_release
};

/* ========================================================================== */
static int __init fdet_init(void)
{
	int		ret = 0;
	dev_t		dev_id;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		printk("%s: Fail to allocate fdet info!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	} else {
		pinfo->mmap_type = FDET_MMAP_TYPE_CLASSIFIER_BINARY;
	}

	pinfo->orig_target_buf = kzalloc(FDET_ORIG_TARGET_BUF_SIZE, GFP_KERNEL);
	if (!pinfo->orig_target_buf) {
		printk("%s: Fail to allocate orig buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}

	pinfo->tmp_target_buf = kzalloc(FDET_TMP_TARGET_BUF_SIZE, GFP_KERNEL);
	if (!pinfo->tmp_target_buf) {
		printk("%s: Fail to allocate tmp buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}

	pinfo->classifier_binary = kzalloc(FDET_CLASSIFIER_BINARY_SIZE, GFP_KERNEL);
	if (!pinfo->classifier_binary) {
		printk("%s: Fail to allocate classifier binary buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}


	pinfo->irq = FDET_IRQ;
	ret = request_irq(pinfo->irq, fdet_isr, FDET_IRQF, "fdet", pinfo);

	if (ret) {
		printk("%s: Fail to request fdet irq!\n", __func__);
		pinfo->irq = 0;
		goto amba_fdet_init_exit;
	}

	printk("%s: Fdet Freq: %d MHz.\n", __func__, amb_get_fdet_clock_frequency(HAL_BASE_VP) / 1000000);

	ret = fdet_enable();
	if (ret) {
		goto amba_fdet_init_exit;
	}

	dev_id = MKDEV(FDET_MAJOR, FDET_MINOR);
	ret = register_chrdev_region(dev_id, 1, FDET_NAME);
	if (ret) {
		goto amba_fdet_init_exit;
	}

	cdev_init(&pinfo->char_dev, &fdet_fops);
	pinfo->char_dev.owner = THIS_MODULE;
	ret = cdev_add(&pinfo->char_dev, dev_id, 1);
	if (ret) {
		unregister_chrdev_region(dev_id, 1);
		goto amba_fdet_init_exit;
	}

	pinfo->timer.function	= fdet_timer;
	pinfo->timer.data		= 0;
	init_timer(&pinfo->timer);

	init_completion(&pinfo->result_completion);

amba_fdet_init_exit:
 	if (ret && pinfo) {
		if (pinfo->irq) {
			free_irq(pinfo->irq, pinfo);
		}
		if (pinfo->classifier_binary) {
			kfree(pinfo->classifier_binary);
			pinfo->classifier_binary = NULL;
		}
		if (pinfo->tmp_target_buf) {
			kfree(pinfo->tmp_target_buf);
			pinfo->tmp_target_buf = NULL;
		}
		if (pinfo->orig_target_buf) {
			kfree(pinfo->orig_target_buf);
			pinfo->orig_target_buf = NULL;
		}
		kfree(pinfo);
		pinfo = NULL;
	}
	return ret;
}

static void __exit amba_fdet_exit(void)
{
	dev_t		dev_id;

	fdet_disable();
	if (pinfo) {
		if (pinfo->irq) {
			free_irq(pinfo->irq, pinfo);
		}

		cdev_del(&pinfo->char_dev);

		dev_id = MKDEV(FDET_MAJOR, FDET_MINOR);
		unregister_chrdev_region(dev_id, 1);

		if (pinfo->classifier_binary) {
			kfree(pinfo->classifier_binary);
			pinfo->classifier_binary = NULL;
		}
		if (pinfo->tmp_target_buf) {
			kfree(pinfo->tmp_target_buf);
			pinfo->tmp_target_buf = NULL;
		}
		if (pinfo->orig_target_buf) {
			kfree(pinfo->orig_target_buf);
			pinfo->orig_target_buf = NULL;
		}

		kfree(pinfo);
		pinfo = NULL;
	}
}

module_init(fdet_init);
module_exit(amba_fdet_exit);

MODULE_DESCRIPTION("Ambarella A7 / S2 Fdet driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

