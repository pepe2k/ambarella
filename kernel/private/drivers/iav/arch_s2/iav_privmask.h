/*
 * iav_privmask.h
 *
 * History:
 *	2012/02/20 - [Jian Tang] created file
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_PRIVMASK_H__
#define __IAV_PRIVMASK_H__

int reset_privacy_mask(void);
u32 pm_user_to_phy_addr(iav_context_t* context, u8* addr);

int get_MB_num(int pixel);
int get_buffer_pitch_in_MB(int MB_count_in_row);
int check_privacy_mask_in_MB(iav_context_t* context, iav_privacy_mask_setup_ex_t * mask_setup);
int check_hdr_pm_width(void);
void set_digital_zoom_privacy_mask(iav_context_t* context, iav_digital_zoom_privacy_mask_ex_t* dptz_pm);

void cmd_set_privacy_mask(iav_context_t* context);
void cmd_set_local_exposure(void);

int sync_le_with_pm(void);
int get_privacy_mask_param(iav_privacy_mask_setup_ex_t* pm_setup);
int cfg_privacy_mask_param(iav_context_t* context, iav_privacy_mask_setup_ex_t* pm_setup);
int get_stream_privacy_mask_param(iav_stream_privacy_mask_t* param);
int cfg_stream_privacy_mask_param(iav_context_t* context, iav_stream_privacy_mask_t* param);

//external function for driver
int iav_set_privacy_mask_ex(iav_context_t *context, struct iav_privacy_mask_setup_ex_s __user *arg);
int iav_get_privacy_mask_ex(iav_context_t *context, struct iav_privacy_mask_setup_ex_s __user *arg);
int iav_get_privacy_mask_info_ex(iav_context_t *context, struct iav_privacy_mask_info_ex_s __user *arg);
int iav_get_local_exposure_ex(iav_context_t *context, struct iav_local_exposure_ex_s __user *arg);
int iav_set_local_exposure_ex(iav_context_t *context, struct iav_local_exposure_ex_s __user *arg);

#endif	// __IAV_PRIVMASK_H__


