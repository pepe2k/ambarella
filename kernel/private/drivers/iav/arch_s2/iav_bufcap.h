/*
* iav_bufcap.h
*
* History:
*	2012/10/10 - [Jian Tang] Created file
*
* Copyright (C) 2012-2016, Ambarella, Inc.
*
* All rights reserved. No Part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Ambarella, Inc.
*
*/

#ifndef __IAV_BUFCAP_H__
#define __IAV_BUFCAP_H__

int iav_bufcap_init(void);
void iav_bufcap_reset(void);
void save_dsp_buffers(VCAP_STRM_REPORT *msg, VCAP_STRM_REPORT_EXT *msg_ext);

int iav_read_raw_info_ex(iav_context_t *context, iav_raw_info_t __user *arg);
int iav_read_yuv_buffer_info_ex(iav_context_t * context, struct iav_yuv_buffer_info_ex_s __user *arg);
int iav_read_me1_buffer_info(iav_context_t * context, iav_me1_buffer_info_ex_t __user * arg);

int iav_bufcap_read(iav_context_t * context, iav_buf_cap_t __user *arg);
void prepare_vout_b_letter_box(void);

#endif	// __IAV_BUFCAP_H__

