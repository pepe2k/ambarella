/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_edid.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include "ambhdmi_cec.h"
#include "ambhdmi_cec.c"
#include "arch/ambhdmi_edid_arch.c"

#define EDID_PRINT(S...)			DRV_PRINT(KERN_DEBUG S)

typedef struct {
	enum amba_video_mode			vmode;
	u16					h_active;
	u16					v_active;
	u8					fps;
	u8					interlace;
} _vd_mode_t;

typedef struct {
	const char				*vendor;
	const u16				product_code;
} _hdmi_sink_id_t;

const static _vd_mode_t VD_MODE_PARAMS[] = {
	{AMBA_VIDEO_MODE_480I,      1440,  240, 60, 1},
	{AMBA_VIDEO_MODE_576I,      1440,  288, 50, 1},
	{AMBA_VIDEO_MODE_D1_NTSC,    720,  480, 60, 0},
	{AMBA_VIDEO_MODE_D1_PAL,     720,  576, 50, 0},
	{AMBA_VIDEO_MODE_720P,      1280,  720, 60, 0},
	{AMBA_VIDEO_MODE_720P_PAL,  1280,  720, 50, 0},
	{AMBA_VIDEO_MODE_1080I,     1920,  540, 60, 1},
	{AMBA_VIDEO_MODE_1080I_PAL, 1920,  540, 50, 1},
	{AMBA_VIDEO_MODE_1080P,     1920, 1080, 60, 0},
	{AMBA_VIDEO_MODE_1080P_PAL, 1920, 1080, 50, 0},
	{AMBA_VIDEO_MODE_1080P30,   1920, 1080, 30, 0},
	{AMBA_VIDEO_MODE_1080P25,   1920, 1080, 25, 0},
	{AMBA_VIDEO_MODE_1080P24,   1920, 1080, 24, 0},
	{AMBA_VIDEO_MODE_2160P30,   3840, 2160, 30, 0},
	{AMBA_VIDEO_MODE_2160P25,   3840, 2160, 25, 0},
	{AMBA_VIDEO_MODE_2160P24,   3840, 2160, 24, 0},
	{AMBA_VIDEO_MODE_2160P24_SE,3840, 2160, 24, 0},
};

const static _hdmi_sink_id_t FREE_OVERSCAN_DEVICES[] = {
	/* Sony */
	{"SNY", 0x00F6},
	{"SNY", 0x0209},

	/* Samsung */
	{"SAM", 0x9B06},

	/* Philips */
	{"PHL", 0x43C0},

	/* AOC */
	{"AOC", 0x0024},

	/* TCL */
	{"TCL", 0x0000},

	/* Others */
	{"GSM", 0x599C},
	{"HEC", 0x2900},
};

const static _hdmi_sink_id_t FORCE_NATIVE_TIMING_DEVICES[] = {
	/* Sony */
	{"SNY", 0x02F3},
};


/* ========================================================================== */
static int ambhdmi_edid_read_base(struct i2c_adapter *adapter, u8 *buf)
{
	int			i, errorCode = 0;
	u8			segment, offset;
	struct i2c_msg		msg[3];
	const u8		header[8] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
	u8			check_sum;

	if (!buf || !adapter) {
		errorCode = -EINVAL;
		goto ambhdmi_edid_read_base_exit;
	}

	/* Specify Segment Pointer */
	segment		= 0 >> 1;
	msg[0].addr	= EDID_SEGMENT_POINTER_ADDR;
	msg[0].buf	= &segment;
	msg[0].len	= 1;
	msg[0].flags	= I2C_M_IGNORE_NAK;

	/* Specify Sub-Address */
	offset		= (0 & 0x1) << 7;
	msg[1].addr	= EDID_DATA_ACCESS_ADDR;
	msg[1].buf	= &offset;
	msg[1].len	= 1;
	msg[1].flags	= I2C_M_IGNORE_NAK;

	/* Read EDID Extension */
	msg[2].addr	= EDID_DATA_ACCESS_ADDR;
	msg[2].buf	= buf;
	msg[2].len	= EDID_PER_SEGMENT_SIZE;
	msg[2].flags	= I2C_M_RD;

	errorCode = i2c_transfer(adapter, msg, 3);
	if (errorCode != 3) {
		errorCode = -EIO;
		vout_err("Can't read DDC base!\n");
		goto ambhdmi_edid_read_base_exit;
	}

	/* Check Sum */
	check_sum = 0;
	for (i = 0; i < EDID_PER_SEGMENT_SIZE; i++)
		check_sum += buf[i];
	if (check_sum != 0) {
		errorCode = -EIO;
		vout_err("Incorrect EDID checksum!\n");
		goto ambhdmi_edid_read_base_exit;
	}

	/* Check Header */
	errorCode = 0;
	for (i = 0; i < 0x8; i++) {
		if (buf[i] != header[i]) {
			errorCode = 1;
			break;
		}
	}
	if (errorCode) {
		errorCode = -EIO;
		vout_err("Incorrect EDID header!\n");
		goto ambhdmi_edid_read_base_exit;
	}

ambhdmi_edid_read_base_exit:
	return errorCode;
}

static int ambhdmi_edid_read_extension(struct i2c_adapter *adapter,
	u8 *buf, u8 _segment)
{
	int			i, errorCode = 0;
	u8			segment, offset;
	struct i2c_msg		msg[3];
	u8			check_sum;

	if (!buf || !_segment || !adapter) {
		errorCode = -EINVAL;
		goto ambhdmi_edid_read_extension_exit;
	}

	/* Specify Segment Pointer */
	segment		= _segment >> 1;
	msg[0].addr	= EDID_SEGMENT_POINTER_ADDR;
	msg[0].buf	= &segment;
	msg[0].len	= 1;
	msg[0].flags	= I2C_M_IGNORE_NAK;

	/* Specify Sub-Address */
	offset		= (_segment & 0x1) << 7;
	msg[1].addr	= EDID_DATA_ACCESS_ADDR;
	msg[1].buf	= &offset;
	msg[1].len	= 1;
	msg[1].flags	= I2C_M_IGNORE_NAK;

	/* Read EDID Extension */
	msg[2].addr	= EDID_DATA_ACCESS_ADDR;
	msg[2].buf	= buf;
	msg[2].len	= EDID_PER_SEGMENT_SIZE;
	msg[2].flags	= I2C_M_RD;

	errorCode = i2c_transfer(adapter, msg, 3);
	if (errorCode != 3) {
		errorCode = -EIO;
		vout_err("Can't read DDC extension %d!\n", _segment);
		goto ambhdmi_edid_read_extension_exit;
	}

	/* Check Sum */
	check_sum = 0;
	for (i = 0; i < EDID_PER_SEGMENT_SIZE; i++)
		check_sum += buf[i];
	if (check_sum != 0) {
		errorCode = -EIO;
		vout_err("Incorrect EDID checksum!\n");
		goto ambhdmi_edid_read_extension_exit;
	}

ambhdmi_edid_read_extension_exit:
	return 0;
}

static void ambhdmi_get_native_mode(amba_hdmi_video_timing_t *pvtiming, u32 fps)
{
	u32					i;

	for (i = 0; i < ARRAY_SIZE(VD_MODE_PARAMS); i++) {
		if (pvtiming->h_active != VD_MODE_PARAMS[i].h_active)
			continue;

		if (pvtiming->v_active != VD_MODE_PARAMS[i].v_active)
			continue;

		if (fps != VD_MODE_PARAMS[i].fps)
			continue;

		if (pvtiming->interlace != VD_MODE_PARAMS[i].interlace)
			continue;

		break;
	}

	if (i < ARRAY_SIZE(VD_MODE_PARAMS)) {
		pvtiming->vmode = VD_MODE_PARAMS[i].vmode;
	} else {
		pvtiming->vmode = AMBA_VIDEO_MODE_MAX;
	}
}

static int ambhdmi_edid_parse_base(amba_hdmi_edid_t *pedid, const u8 *buf)
{
	u8					offset, i;
	u32					tmp[4];
	amba_hdmi_product_t			*pproduct;
	amba_hdmi_edid_version_t		*pversion;
	amba_hdmi_display_feature_t		*pfeature;
	amba_hdmi_color_characteristics_t	*pcolor;
	amba_hdmi_native_timing_t		*pntiming;
	amba_hdmi_video_timing_t		*pvtiming;

	/* Vendor/Product ID */
	pproduct = &pedid->product;
	pproduct->vendor[0] = 'A' + ((buf[0x8] & 0x7c) >> 2) - 1;
	pproduct->vendor[1] = 'A' + (((buf[0x8] & 0x3) << 3) +
		((buf[0x9] & 0xe0) >> 5)) - 1;
	pproduct->vendor[2] = 'A' + (buf[0x9] & 0x1f) - 1;
	pproduct->vendor[3] = '\0';
	pproduct->product_code = (buf[0xa] << 8) | buf[0xb];
	pproduct->product_serial_number = (buf[0xc] << 24) | (buf[0xd] << 16) |
		(buf[0xe] << 8) | buf[0xf];
	pproduct->manufacture_week = buf[0x10];
	pproduct->manufacture_year = 1990 + buf[0x11];

	/* EDID Version/Revision */
	pversion = &pedid->edid_version;
	pversion->version = buf[0x12];
	pversion->revision = buf[0x13];

	/* Basic Display Parameters/Features */
	pfeature = &pedid->display_feature;
	pfeature->video_input_definition = buf[0x14];
	pfeature->max_horizontal_image_size = buf[0x15];
	pfeature->max_vertical_image_size = buf[0x16];
	pfeature->gama = 100 + buf[0x17];
	switch ((buf[0x18] & 0x18) >> 3) {
	case 0:
		pfeature->display_type = MONOCHROME;
		break;

	case 1:
		pfeature->display_type = RGB;
		break;

	case 2:
		pfeature->display_type = NONRGB;
		break;

	case 3:
		pfeature->display_type = UNDEFINED;
		break;

	default:
		break;
	}

	/* Color Characteristics */
	pcolor = &pedid->color_characteristics;
	pcolor->red_x   = (buf[0x19] << 2) | ((buf[0x1a] & 0xc0) >> 6);
	pcolor->red_y   = (buf[0x19] << 2) | ((buf[0x1a] & 0x30) >> 4);
	pcolor->green_x = (buf[0x1d] << 2) | ((buf[0x19] & 0x0c) >> 2);
	pcolor->green_y = (buf[0x14] << 2) | ((buf[0x19] & 0x03) >> 0);
	pcolor->blue_x  = (buf[0x1f] << 2) | ((buf[0x1a] & 0xc0) >> 6);
	pcolor->blue_y  = (buf[0x20] << 2) | ((buf[0x1a] & 0x30) >> 4);
	pcolor->white_x = (buf[0x21] << 2) | ((buf[0x1a] & 0x0c) >> 2);
	pcolor->white_y = (buf[0x22] << 2) | ((buf[0x1a] & 0x03) >> 0);

	/* Detailed Timing Description */
	offset = 0x36;
	pntiming = &pedid->native_timings;
	for (i = 1; i <= 4; i++, offset += 0x12) {
		if (!buf[offset] && !buf[offset + 1]) {
			continue;
		}

		pvtiming =
			&pntiming->supported_native_timings[pntiming->number];
		pvtiming->pixel_clock =
			10 * ((buf[offset + 1] << 8) | buf[offset]);
		pvtiming->h_active =
			((buf[offset + 4] & 0xf0) << 4) | buf[offset + 2];
		pvtiming->v_active =
			((buf[offset + 7] & 0xf0) << 4) | buf[offset + 5];
		pvtiming->h_blanking =
			((buf[offset + 4] & 0x0f) << 8) | buf[offset + 3];
		pvtiming->v_blanking =
			((buf[offset + 7] & 0x0f) << 8) | buf[offset + 6];
		pvtiming->hsync_offset =
			((buf[offset + 11] & 0xc0) << 2) | buf[offset + 8];
		pvtiming->hsync_width =
			((buf[offset + 11] & 0x30) << 4) | buf[offset + 9];
		pvtiming->vsync_offset = ((buf[offset + 11] & 0x0c) << 2) |
			((buf[offset + 10] & 0xf0) >> 4);
		pvtiming->vsync_width = ((buf[offset + 11] & 0x03) << 4) |
			(buf[offset + 10] & 0x0f);
		if (buf[offset + 17] & 0x80) {
			pvtiming->interlace = 1;
		} else {
			pvtiming->interlace = 0;
		}
		if (buf[offset + 17] & 0x02) {
			pvtiming->hsync_polarity = 1;
		} else {
			pvtiming->hsync_polarity = 0;
		}
		if (buf[offset + 17] & 0x04) {
			pvtiming->vsync_polarity = 1;
		} else {
			pvtiming->vsync_polarity = 1;
		}
		tmp[0] =  1000 * pvtiming->pixel_clock;
		tmp[1] =  (pvtiming->h_active + pvtiming->h_blanking);
		tmp[1] *= (pvtiming->v_active + pvtiming->v_blanking);
		if (tmp[1] == 0) {
			continue;
		}
		tmp[2] =  tmp[0] / tmp[1];
		tmp[3] =  tmp[0] % tmp[1];

#ifdef CONFIG_HDMI_VIN_VOUT_SYNC_WORKAROUND_ENABLE
		if (tmp[2] == 60 || tmp[2] == 30) {
			pvtiming->pixel_clock = (5994 * pvtiming->pixel_clock + 3000) / 6000;
		}
#endif

		tmp[3] =  (tmp[3] << 1) / tmp[1];
		if (tmp[3]) tmp[2]++;

		if (pvtiming->interlace) {
			sprintf(pvtiming->name, "%dx%di%d", pvtiming->h_active,
					pvtiming->v_active << 1, tmp[2]);
		} else {
			sprintf(pvtiming->name, "%dx%dp%d", pvtiming->h_active,
					pvtiming->v_active, tmp[2]);
		}
		ambhdmi_get_native_mode(pvtiming, tmp[2]);
		pntiming->number++;
	}

	return 0;
}

static int ambhdmi_edid_parse_extension(amba_hdmi_edid_t *pedid, const u8 *buf)
{
	u8					offset, i, j, k, l, m, focus;
	u8					vsdb_present;
	u32					tmp[4];
	amba_hdmi_native_timing_t		*pntiming;
	amba_hdmi_cea_timing_t			*pctiming;
	amba_hdmi_video_timing_t		*pvtiming;
	amba_hdmi_ddd_support_t			*pstructure;

	if (buf[0x0] != 0x02) {
		vout_notice("Extension tag = %d(not 0x2), "
				"not supported yet!\n", buf[0x0]);
		goto ambhdmi_edid_parse_extension_exit;
	}

	pedid->color_space.support_ycbcr444 = buf[0x03] & 0x20;
	pedid->color_space.support_ycbcr422 = buf[0x03] & 0x10;

	offset = buf[0x02];
	if (offset < 4) {
		goto ambhdmi_edid_parse_extension_exit;
	}

	i = 0x4;
	vsdb_present = 0;
	while (i < offset) {
		tmp[0] = (buf[i] & 0xe0) >> 5;
		tmp[1] = buf[i] & 0x1f;

		/* SVD, CEA timings */
		if (tmp[0] == 2) {
			pctiming = &pedid->cea_timings;
			for (j = 1; j <= tmp[1]; j++) {
				pctiming->supported_cea_timings[pctiming->number]
					= buf[i + j] & 0x7f;
				pctiming->number++;
			}
		}

		/* VSDB, Vendor Specific Data */
		if (tmp[0] == 3) {
			if (buf[i + 1] == 0x03 && buf[i + 2] == 0x0c && buf[i + 3] == 0x00) {
				vsdb_present = 1;
			}

			/* CEC Physical Address */
			if (tmp[1] >= 5) {
				pedid->cec.physical_address =
					((buf[i + 4] << 8) | buf[i + 5]);
			}

			/* 3D Formats */
			if ( (tmp[1] >= 8) && (buf[i + 8] & 0x20) ) {
				u8		ddd_present;
				u8		ddd_multi_present;
				u8		hdmi_vic_len;
				//u8		hdmi_3d_len;
				u8		vic;

				focus = i + 13;	/* 3D_present, 3D_Multi_present */
				ddd_present = (buf[focus] & 0x80) >> 7;
				ddd_multi_present = (buf[focus] & 0x60) >> 5;
				hdmi_vic_len = (buf[focus + 1] & 0xe0) >> 5;
				//hdmi_3d_len = (buf[focus + 1] & 0x1f) >> 0;

				/* Extended CEA Timings */
				focus = i + 15; /* (if HDMI_VIC_LEN > 0) */
				pctiming = &pedid->cea_timings;
				for (j = 0; j < hdmi_vic_len; j++) {
					vic = buf[focus + j];
					if (vic > 0 && vic < MAX_EXTENDED_TIMINGS) {
						pctiming->supported_cea_timings[pctiming->number] = MAX_CEA_TIMINGS + vic;
						pctiming->number++;
					}
				}

				/* All Supported 3D Structures */
				if (ddd_multi_present == 0b01 || ddd_multi_present == 0b10) {
					u16		ddd_structure_all;

					focus = i + 15 + hdmi_vic_len; /* (if 3D_Multi_present = 01 or 10) */
					pstructure = &pedid->ddd_structure;
					ddd_structure_all = (buf[focus] << 8) | buf[focus + 1];
					for (j = 0; j < 16; j++) {
						if (ddd_structure_all & 0x01) {
							pstructure->ddd_structures[pstructure->ddd_num] = j;
							pstructure->ddd_num++;
						}
						ddd_structure_all >>= 1;
					}
				}

				/* Mask 3D Structures */
				if (ddd_multi_present == 0b10) {
					u16		ddd_structure_mask;

					focus = i + 15 + hdmi_vic_len + 2; /* (if 3D_Multi_present = 10) */
					ddd_structure_mask = (buf[focus] << 8) | buf[focus + 1];
					pctiming = &pedid->cea_timings;
					pstructure = &pedid->ddd_structure;

					for (j = 0; j < 16; j++) {
						if (ddd_structure_mask & 0x01) {
							vic = buf[i + 15 + j];
							if (vic > 0 && vic < MAX_EXTENDED_TIMINGS) {
								vic += MAX_CEA_TIMINGS;
								for (k = 0; k < pctiming->number; k++) {
									if (pctiming->supported_cea_timings[k] == vic) {
										memcpy(pctiming->supported_ddd_structures, pstructure, sizeof(*pstructure));
										break;
									}
								}
							}
						}
						ddd_structure_mask >>= 1;
					}
				}

				/* 2D_VIC_order_X, 3D_Structure_X, 3D_Detail_X */
				switch (ddd_multi_present) {
				case 0b01:
					focus = i + 15 + hdmi_vic_len + 2;
					ddd_multi_present -= 2;
					break;

				case 0b10:
					focus = i + 15 + hdmi_vic_len + 4;
					ddd_multi_present -= 4;
					break;

				default:
					focus = i + 15 + hdmi_vic_len;
					break;
				}

				if (ddd_multi_present) {
					u8	dd_vic_order_x;
					u8	ddd_structure_x;

					pctiming = &pedid->cea_timings;
					for (l = 0; l < ddd_multi_present; l++) {
						dd_vic_order_x = (buf[focus + l] & 0xf0) >> 4;
						ddd_structure_x = (buf[focus + l] & 0x0f) >> 0;

						if (dd_vic_order_x > 0 && dd_vic_order_x < MAX_EXTENDED_TIMINGS) {
							dd_vic_order_x += MAX_CEA_TIMINGS;
							for (k = 0; k < pctiming->number; k++) {
								if (pctiming->supported_cea_timings[k] == dd_vic_order_x) {
									pstructure = &pctiming->supported_ddd_structures[k];
									for (m = 0; m < pstructure->ddd_num; m++) {
										if (pstructure->ddd_structures[m] == ddd_structure_x) {
											break;
										}
									}
									if (m == pstructure->ddd_num) {
										pstructure->ddd_structures[m] = ddd_structure_x;
										pstructure->ddd_num++;
									}
									break;
								}
							}
						}

						/* We will ignore 3D_Detail_X */
						if (ddd_structure_x & 0x80) {
							l++;
						}
					}
				}

				/* 3D_present */
				if (ddd_present) {
					struct __mandatory_mode {
						enum amba_video_mode	mode;
						ddd_structure_t		ddd_structure;
					} mandatory_modes[] =
					{
						{AMBA_VIDEO_MODE_720P,		DDD_FRAME_PACKING	},
						{AMBA_VIDEO_MODE_720P,		DDD_TOP_AND_BOTTOM	},

						{AMBA_VIDEO_MODE_720P_PAL,	DDD_FRAME_PACKING	},
						{AMBA_VIDEO_MODE_720P_PAL,	DDD_TOP_AND_BOTTOM	},

						{AMBA_VIDEO_MODE_1080I,		DDD_SIDE_BY_SIDE_HALF	},
						{AMBA_VIDEO_MODE_1080I_PAL,	DDD_SIDE_BY_SIDE_HALF	},

						{AMBA_VIDEO_MODE_1080P24,	DDD_FRAME_PACKING	},
						{AMBA_VIDEO_MODE_1080P24,	DDD_TOP_AND_BOTTOM	},
					};

					/* CEA Timing */
					pctiming = &pedid->cea_timings;
					for (j = 0; j < pctiming->number; j++) {
						for (k = 0; k < sizeof(mandatory_modes) / sizeof(struct __mandatory_mode); k++) {
							vic = pctiming->supported_cea_timings[j];
							if (CEA_Timings[vic].vmode == mandatory_modes[k].mode) {
								pstructure = &pctiming->supported_ddd_structures[j];
								for (l = 0; l < pstructure->ddd_num; l++) {
									if (pstructure->ddd_structures[l] == mandatory_modes[k].ddd_structure) {
										break;
									}
								}
								if (l == pstructure->ddd_num) {
									pstructure->ddd_structures[l] = mandatory_modes[k].ddd_structure;
									pstructure->ddd_num++;
								}
							}
						}
					}

					/* Native Timing */
					pntiming = &pedid->native_timings;
					for (j = 0; j < pntiming->number; j++) {
						for (k = 0; k < sizeof(mandatory_modes) / sizeof(struct __mandatory_mode); k++) {
							if (pntiming->supported_native_timings[j].vmode == mandatory_modes[k].mode) {
								pstructure = &pntiming->supported_ddd_structures[j];
								for (l = 0; l < pstructure->ddd_num; l++) {
									if (pstructure->ddd_structures[l] == mandatory_modes[k].ddd_structure) {
										break;
									}
								}
								if (l == pstructure->ddd_num) {
									pstructure->ddd_structures[l] = mandatory_modes[k].ddd_structure;
									pstructure->ddd_num++;
								}
							}
						}
					}
				}
			}
		}

		i += (tmp[1] + 1);
	}

	if (!vsdb_present) {
		pedid->interface = DVI;
		vout_notice("HDMI VSDB not found in EDID extension!\n");
	}

	/* Native timings */
	pntiming = &pedid->native_timings;
	for (i = 0; i < (buf[0x03] & 0x0f); i++, offset += 0x12) {
		pvtiming = &pntiming->supported_native_timings[pntiming->number];

		pvtiming->pixel_clock =
			10 * ((buf[offset + 1] << 8) | buf[offset]);
		pvtiming->h_active =
			((buf[offset + 4] & 0xf0) << 4) | buf[offset + 2];
		pvtiming->v_active =
			((buf[offset + 7] & 0xf0) << 4) | buf[offset + 5];
		pvtiming->h_blanking =
			((buf[offset + 4] & 0x0f) << 8) | buf[offset + 3];
		pvtiming->v_blanking =
			((buf[offset + 7] & 0x0f) << 8) | buf[offset + 6];
		pvtiming->hsync_offset =
			((buf[offset + 11] & 0xc0) << 2) | buf[offset + 8];
		pvtiming->hsync_width =
			((buf[offset + 11] & 0x30) << 4) | buf[offset + 9];
		pvtiming->vsync_offset = ((buf[offset + 11] & 0x0c) << 2) |
			((buf[offset + 10] & 0xf0) >> 4);
		pvtiming->vsync_width = ((buf[offset + 11] & 0x03) << 4) |
			(buf[offset + 10] & 0x0f);
		if (buf[offset + 17] & 0x80) {
			pvtiming->interlace = 1;
		} else {
			pvtiming->interlace = 0;
		}
		if (buf[offset + 17] & 0x02) {
			pvtiming->hsync_polarity = 1;
		} else {
			pvtiming->hsync_polarity = 0;
		}
		if (buf[offset + 17] & 0x04) {
			pvtiming->vsync_polarity = 1;
		} else {
			pvtiming->vsync_polarity = 1;
		}
		tmp[0] =  1000 * pvtiming->pixel_clock;
		tmp[1] =  (pvtiming->h_active + pvtiming->h_blanking);
		tmp[1] *= (pvtiming->v_active + pvtiming->v_blanking);
		if (tmp[1] == 0) {
			continue;
		}
		tmp[2] =  tmp[0] / tmp[1];
		tmp[3] =  tmp[0] % tmp[1];
		tmp[3] =  (tmp[3] << 1) / tmp[1];
		if (tmp[3]) tmp[2]++;

		if (pvtiming->interlace) {
			sprintf(pvtiming->name, "%dx%di%d", pvtiming->h_active,
					pvtiming->v_active << 1, tmp[2]);
		} else {
			sprintf(pvtiming->name, "%dx%dp%d", pvtiming->h_active,
					pvtiming->v_active, tmp[2]);
		}

		ambhdmi_get_native_mode(pvtiming, tmp[2]);
		pntiming->number++;
	}

ambhdmi_edid_parse_extension_exit:
	return 0;
}

static int ambhdmi_edid_read_and_parse(struct ambhdmi_sink *phdmi_sink)
{
	int				errorCode = 0;
	u8				i, extensions, buf[EDID_PER_SEGMENT_SIZE] = {0};
	struct i2c_adapter		*adapter = phdmi_sink->ddc_adapter;
	amba_hdmi_edid_t 		*pedid;
	int				j, overscan;

	if (!phdmi_sink) {
		errorCode = -EINVAL;
		goto ambhdmi_edid_read_and_parse_exit;
	} else {
		pedid = &phdmi_sink->edid;
	}

	if (phdmi_sink->raw_edid.buf) {
		vfree(phdmi_sink->raw_edid.buf);
		phdmi_sink->raw_edid.buf = NULL;
		phdmi_sink->raw_edid.len = 0;
	}

	memset(pedid, 0, sizeof(*pedid));
	pedid->interface = HDMI;
	pedid->color_space.support_ycbcr444 = 0;
	pedid->color_space.support_ycbcr422 = 0;
	pedid->cec.physical_address = 0xffff;
	pedid->cec.logical_address = CEC_DEV_UNREGISTERED;

	/* Read EDID Base */
	errorCode = ambhdmi_edid_read_base(adapter, buf);
	if (errorCode) {
		goto ambhdmi_edid_read_and_parse_exit;
	} else {
		extensions = buf[0x7e];
		phdmi_sink->raw_edid.buf =
			vmalloc(EDID_PER_SEGMENT_SIZE * (1 + extensions));
		if (!phdmi_sink->raw_edid.buf) {
			errorCode = -ENOMEM;
			goto ambhdmi_edid_read_and_parse_exit;
		} else {
			phdmi_sink->raw_edid.len =
				EDID_PER_SEGMENT_SIZE * (1 + extensions);
		}
		memcpy(phdmi_sink->raw_edid.buf, buf, EDID_PER_SEGMENT_SIZE);

	}

	/* Parse EDID Base */
	errorCode = ambhdmi_edid_parse_base(pedid, buf);
	if (errorCode) {
		goto ambhdmi_edid_read_and_parse_exit;
	}

	/* Device Force Overscan */
	for (j = 0, overscan = 1; j < ARRAY_SIZE(FREE_OVERSCAN_DEVICES); j++) {
		if (!strcmp(pedid->product.vendor, FREE_OVERSCAN_DEVICES[j].vendor)
			&& pedid->product.product_code == FREE_OVERSCAN_DEVICES[j].product_code) {
			overscan = 0;
			break;
		}
	}
	if (overscan) {
		phdmi_sink->video_sink.hdmi_overscan
				= AMBA_VOUT_HDMI_FORCE_OVERSCAN;
	} else {
		phdmi_sink->video_sink.hdmi_overscan
				= AMBA_VOUT_HDMI_NON_FORCE_OVERSCAN;
	}

	/* Forced to use native timing */
	phdmi_sink->video_sink.hdmi_force_native_timing = 0;
	for (j = 0; j < ARRAY_SIZE(FORCE_NATIVE_TIMING_DEVICES); j++) {
		if (!strcmp(pedid->product.vendor, FORCE_NATIVE_TIMING_DEVICES[j].vendor)
			&& pedid->product.product_code == FORCE_NATIVE_TIMING_DEVICES[j].product_code) {
			phdmi_sink->video_sink.hdmi_force_native_timing = 1;
			break;
		}
	}

	if (!extensions) {
		pedid->interface = DVI;
		vout_notice("No EDID extensions found!\n");
		goto ambhdmi_edid_read_and_parse_exit;
	}

	if (extensions > 1) {
		vout_notice("EDID Extensions = %d > 1, "
					"not verified yet!\n", extensions);
	}

	for (i = 1; i <= extensions; i++) {
		/* Read EDID Extension */
		errorCode = ambhdmi_edid_read_extension(adapter, buf, i);
		if (errorCode) {
			goto ambhdmi_edid_read_and_parse_exit;
		} else {
			u32 offset;
			u8  *pbuf;

			offset = EDID_PER_SEGMENT_SIZE * i;
			pbuf = &phdmi_sink->raw_edid.buf[offset];
			memcpy(pbuf, buf, EDID_PER_SEGMENT_SIZE);
		}

		/* Parse EDID Extension */
		errorCode = ambhdmi_edid_parse_extension(pedid, buf);
		if (errorCode) {
			goto ambhdmi_edid_read_and_parse_exit;
		}
	}

	/* Allocate CEC Logical Address */
	if (pedid->cec.physical_address) {
		pedid->cec.logical_address =
			ambhdmi_cec_allocate_logical_address(phdmi_sink);

		if (pedid->cec.logical_address == CEC_DEV_UNREGISTERED)
			goto ambhdmi_edid_read_and_parse_exit;

		if (ambhdmi_cec_report_physical_address(phdmi_sink))
			vout_notice("Reporting Physical Address Failed!\n");
	}

ambhdmi_edid_read_and_parse_exit:
	return errorCode;
}

static void ambhdmi_edid_print(const amba_hdmi_edid_t *pedid)
{
	int					i, j;
	char					buf[256];
	const amba_hdmi_product_t		*pproduct;
	const amba_hdmi_edid_version_t		*pversion;
	const amba_hdmi_display_feature_t	*pfeature;
	const amba_hdmi_color_characteristics_t	*pcolor;
	const amba_hdmi_ddd_support_t		*pstructure;
	const amba_hdmi_native_timing_t		*pntiming;
	const amba_hdmi_cea_timing_t		*pctiming;
	const amba_hdmi_video_timing_t		*pvtiming;
	const amba_hdmi_cec_t			*cec;

	/* Vendor/Product ID */
	pproduct = &pedid->product;
	pversion = &pedid->edid_version;
	EDID_PRINT("Vendor: %s\n", pproduct->vendor);
	EDID_PRINT("Product Code: %04X\n", pproduct->product_code);
	EDID_PRINT("Serial Number: %d\n", pproduct->product_serial_number);
	EDID_PRINT("Manufacture Date: Week %d, Year %d\n", pproduct->manufacture_week, pproduct->manufacture_year);
	EDID_PRINT("EDID Version: %d.%d\n", pversion->version, pversion->revision);

	/* Basic Display Parameters/Features */
	pfeature = &pedid->display_feature;
	sprintf(buf, "\nVideo Input: ");
	if (pfeature->video_input_definition & 0x80) {
		sprintf(buf, "%sDigital, DFP 1.x ", buf);
		if (pfeature->video_input_definition & 0x01)
			sprintf(buf, "%scompatible\n", buf);
		else
			sprintf(buf, "%snot compatible\n", buf);
	} else {
		sprintf(buf, "%sAnalog, Video Level = \n", buf);
		switch ((pfeature->video_input_definition & 0x60) >> 5) {
		case 0:
			sprintf(buf, "%s0.7, 0.3\n", buf);
			break;
		case 1:
			sprintf(buf, "%s0.714, 0.286\n", buf);
			break;
		case 2:
			sprintf(buf, "%s1, 0.4\n", buf);
			break;
		case 3:
			sprintf(buf, "%s0.7, 0\n", buf);
			break;
		default:
			break;
		}
	}
	EDID_PRINT("%s", buf);

	sprintf(buf, "Display Type: ");
	switch (pfeature->display_type) {
	case MONOCHROME:
		sprintf(buf, "%sMonochrome\n", buf);
		break;
	case RGB:
		sprintf(buf, "%sRGB\n", buf);
		break;
	case NONRGB:
		sprintf(buf, "%sNon-RGB\n", buf);
		break;
	case UNDEFINED:
		sprintf(buf, "%sUndefined\n", buf);
		break;
	default:
		break;
	}
	EDID_PRINT("%s", buf);

	EDID_PRINT("Max Image Size: %dcm x %dcm\n", pfeature->max_horizontal_image_size, pfeature->max_vertical_image_size);
	EDID_PRINT("Gamma: %d.%02d\n\n", pfeature->gama / 100, pfeature->gama % 100);

	/* Color Characteristics */
	pcolor = &pedid->color_characteristics;
	EDID_PRINT("Color Characteristics:\n");
	EDID_PRINT("\tRed-x:   0.%04d\n", (10000 * pcolor->red_x) >> 10);
	EDID_PRINT("\tRed-y:   0.%04d\n", (10000 * pcolor->red_y) >> 10);
	EDID_PRINT("\tGreen-x: 0.%04d\n", (10000 * pcolor->green_x) >> 10);
	EDID_PRINT("\tGreen-y: 0.%04d\n", (10000 * pcolor->green_y) >> 10);
	EDID_PRINT("\tBlue-x:  0.%04d\n", (10000 * pcolor->blue_x) >> 10);
	EDID_PRINT("\tBlue-y:  0.%04d\n", (10000 * pcolor->blue_y) >> 10);
	EDID_PRINT("\tWhite-x: 0.%04d\n", (10000 * pcolor->white_x) >> 10);
	EDID_PRINT("\tWhite-y: 0.%04d\n\n", (10000 * pcolor->white_y) >> 10);

	/* Interface type and supported input color space */
	if (pedid->interface == HDMI) {
		sprintf(buf, "Interface Type: HDMI\n");
		sprintf(buf, "%sSupported Input Color Space: RGB", buf);
		if (pedid->color_space.support_ycbcr444)
			sprintf(buf, "%s, YCBCR 4:4:4", buf);
		if (pedid->color_space.support_ycbcr422)
			sprintf(buf, "%s, YCBCR 4:2:2", buf);
		sprintf(buf, "%s\n\n", buf);
		EDID_PRINT("%s", buf);
	} else {
		EDID_PRINT("Interface Type: DVI\n");
		EDID_PRINT("Supported Input Color Space: RGB\n\n");
	}

	/* 3D Support */
	pstructure = &pedid->ddd_structure;
	if (pstructure->ddd_num == 0) {
		EDID_PRINT("Supported 3D Structures: None\n\n");
	} else {
		for (i = 0; i < pstructure->ddd_num; i++) {
			EDID_PRINT("\t%s\n", AMBA_3D_STRUCTURE_NAMES[pstructure->ddd_structures[i]]);
		}
		EDID_PRINT("\n");
	}

	/* Native timings */
	pntiming = &pedid->native_timings;
	for (i = 0; i < pntiming->number; i++) {
		pvtiming = &pntiming->supported_native_timings[i];
		EDID_PRINT("Native timing #%2d: %s\n", i + 1, pvtiming->name);
		/*EDID_PRINT("Pixel Clock: %d.%03dMHz\n", pvtiming->pixel_clock / 1000, pvtiming->pixel_clock % 1000);
		EDID_PRINT("H avtive: %d pixels, V active %d lines\n", pvtiming->h_active, pvtiming->v_active);
		EDID_PRINT("H blanking: %d pixels, V blanking %d lines\n", pvtiming->h_blanking, pvtiming->v_blanking);
		EDID_PRINT("H sync offset: %d pixels, width %d pixels\n", pvtiming->hsync_offset, pvtiming->hsync_width);
		EDID_PRINT("V sync offset: %d lines, width %d lines\n", pvtiming->vsync_offset, pvtiming->vsync_width);
		if (pvtiming->interlace)
			EDID_PRINT("Interlace\n");
		else
			EDID_PRINT("Progressive\n");
		if (pvtiming->hsync_polarity)
			EDID_PRINT("H sync Positive\n");
		else
			EDID_PRINT("H sync Negative\n");
		if (pvtiming->vsync_polarity)
			EDID_PRINT("V sync Positive\n");
		else
			EDID_PRINT("V sync Negative\n");
		EDID_PRINT("\n");*/
	}

	/* CEA timings */
	pctiming = &pedid->cea_timings;
	EDID_PRINT("\n");
	for (i = 0; i < pctiming->number; i++) {
		j = pctiming->supported_cea_timings[i];
		pvtiming = &CEA_Timings[j];
		EDID_PRINT("CEA    timing #%2d: %s\n", j, pvtiming->name);
	}
	EDID_PRINT("\n");

	/* CEC */
	cec = &pedid->cec;
	EDID_PRINT("CEC Physical Address = %X.%X.%X.%X\n", (cec->physical_address & 0xf000) >> 12,
		(cec->physical_address & 0x0f00) >> 8, (cec->physical_address & 0x00f0) >> 4,
		(cec->physical_address & 0x000f) >> 0);
	EDID_PRINT("CEC Logical  Address = %d\n\n", cec->logical_address);
}

static const amba_hdmi_video_timing_t *ambhdmi_edid_find_video_mode(
		struct ambhdmi_sink *phdmi_sink, enum amba_video_mode vmode)
{
	int					i, j;
	const amba_hdmi_edid_t			*pedid = &phdmi_sink->edid;
	const amba_hdmi_native_timing_t		*pntiming;
	const amba_hdmi_cea_timing_t		*pctiming;
	const amba_hdmi_video_timing_t		*vt = NULL;

	if (!pedid) {
		goto ambhdmi_edid_find_video_mode_exit;
	}

	if (!phdmi_sink->video_sink.hdmi_force_native_timing) {
		/* Try to find it in EDID CEA Timings */
		pctiming = &pedid->cea_timings;
		for (i = 0; i < pctiming->number; i++) {
			j = pctiming->supported_cea_timings[i];
			if (CEA_Timings[j].vmode == vmode) {
				vt = &CEA_Timings[j];
				goto ambhdmi_edid_find_video_mode_exit;
			}
		}

		/* Try to find it in EDID Native Timings */
		pntiming = &pedid->native_timings;
		for (i = 0; i < pntiming->number; i++) {
			if (pntiming->supported_native_timings[i].vmode == vmode) {
				vt = &pntiming->supported_native_timings[i];
				goto ambhdmi_edid_find_video_mode_exit;
			}
		}
	} else {
		/* Try to find it in EDID Native Timings */
		pntiming = &pedid->native_timings;
		for (i = 0; i < pntiming->number; i++) {
			if (pntiming->supported_native_timings[i].vmode == vmode) {
				vt = &pntiming->supported_native_timings[i];
				goto ambhdmi_edid_find_video_mode_exit;
			}
		}

		/* Try to find it in EDID CEA Timings */
		pctiming = &pedid->cea_timings;
		for (i = 0; i < pctiming->number; i++) {
			j = pctiming->supported_cea_timings[i];
			if (CEA_Timings[j].vmode == vmode) {
				vt = &CEA_Timings[j];
				goto ambhdmi_edid_find_video_mode_exit;
			}
		}
	}

	/* Use Default CEA Timings */
	for (i = 0; i < MAX_TOTAL_CEA_TIMINGS; i++) {
		if (CEA_Timings[i].vmode == vmode) {
			vt = &CEA_Timings[i];
			goto ambhdmi_edid_find_video_mode_exit;
		}
	}

ambhdmi_edid_find_video_mode_exit:
	return vt;
}

