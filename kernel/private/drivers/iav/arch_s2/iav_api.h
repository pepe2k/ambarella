/*
 * iav_api.h
 *
 * History:
 *	2012/04/13 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_API_H__
#define __IAV_API_H__

int iav_init(struct iav_global_info *g_info, void *dev);
int iav_obj_init(void);
int iav_debug_init(void);

int iav_halt_vout(iav_context_t *context, int vout_id);
int iav_select_output_dev(iav_context_t *context, int dev);
int iav_configure_sink(iav_context_t *context, struct amba_video_sink_mode __user *pcfg);
int iav_enable_vout_csc(iav_context_t *context, struct iav_vout_enable_csc_s * arg);
int iav_enable_vout_video(iav_context_t *context, struct iav_vout_enable_video_s * arg);
int iav_flip_vout_video(iav_context_t *context, struct iav_vout_flip_video_s * arg);
int iav_rotate_vout_video(iav_context_t *context, struct iav_vout_rotate_video_s * arg);
int iav_change_vout_video_size(iav_context_t *context, struct iav_vout_change_video_size_s *arg);
int iav_change_vout_video_offset(iav_context_t *context, struct iav_vout_change_video_offset_s *arg);
int iav_select_vout_fb(iav_context_t *context, struct iav_vout_fb_sel_s * arg);
int iav_enable_vout_osd_rescaler(iav_context_t *context, struct iav_vout_enable_osd_rescaler_s * arg);
int iav_change_vout_osd_offset(iav_context_t *context, struct iav_vout_change_osd_offset_s *arg);

int iav_set_encode_format(iav_context_t *context, iav_encode_format_t __user *format);
int iav_get_encode_format (iav_context_t *context, iav_encode_format_t __user *format);

int iav_get_preview_format(iav_context_t *context, iav_preview_format_t __user *format);


int iav_enable_preview(iav_context_t *context);
int iav_disable_preview(iav_context_t *context);

int iav_get_state_info(iav_context_t *context, iav_state_info_t __user *arg);

int iav_enter_decode_mode(iav_context_t *context);
int iav_leave_decode_mode(iav_context_t *context);
int iav_decode_stop(iav_context_t *context);

int iav_overlay_insert(iav_context_t *context, overlay_insert_t __user *arg);

int iav_init_still_capture(iav_context_t * context, struct iav_still_init_info __user * arg);
int iav_start_still_capture(iav_context_t *context, struct iav_still_cap_info __user *arg);
int iav_stop_still_capture(iav_context_t *context);

//move to iav_decode.h
// iav_decode.c
int iav_get_frame_buffer(iav_context_t *context, iav_frame_buffer_t __user *arg);
int iav_decode_get_frame(iav_context_t *context, iav_decoded_frame_t __user *arg);
int iav_render_frame(iav_context_t *context, iav_decoded_frame_t __user * arg, int release_only);
int iav_decode_wait(iav_context_t *context, iav_wait_decoder_t __user *arg);
int iav_decode_flush(iav_context_t *context);

int __iav_is_sw_decoder(iav_context_t *context);
void __iav_get_swdec_buffer(iav_context_t *context, u32 *addr, u32 *size);

// iav_drv.c
int iav_update_vin(iav_context_t *context, int rval);
void iav_lock(void);
void iav_unlock(void);

int wait_vcap(int mode, const char *desc);
void wait_vsync_loss_msg(void);


// EX APIs for S2 compatibility

// iav_api.c

void iav_config_vin(void);
int iav_get_source_buffer_info_ex(iav_context_t * context, struct iav_source_buffer_info_ex_s __user * arg);

int iav_set_source_buffer_type_all_ex(iav_context_t * context, struct iav_source_buffer_type_all_ex_s __user * arg);
int iav_get_source_buffer_type_all_ex(iav_context_t * context, struct iav_source_buffer_type_all_ex_s __user * arg);

int iav_set_source_buffer_format_ex(iav_context_t * context, struct iav_source_buffer_format_ex_s __user * arg);
int iav_get_source_buffer_format_ex(iav_context_t * context, struct iav_source_buffer_format_ex_s __user * arg);

int iav_set_source_buffer_format_all_ex(iav_context_t *context, struct iav_source_buffer_format_all_ex_s __user* arg);
int iav_get_source_buffer_format_all_ex(iav_context_t *context, struct iav_source_buffer_format_all_ex_s __user* arg);

int iav_set_preview_buffer_format_all_ex(iav_context_t * context, struct iav_preview_buffer_format_all_ex_s __user * arg);
int iav_get_preview_buffer_format_all_ex(iav_context_t * context, struct iav_preview_buffer_format_all_ex_s __user * arg);

int iav_set_source_buffer_setup_ex(iav_context_t * context, struct iav_source_buffer_setup_ex_s __user * arg);
int iav_get_source_buffer_setup_ex(iav_context_t * context, struct iav_source_buffer_setup_ex_s __user * arg);

int iav_set_preview_A_framerate_divisor_ex(iav_context_t * context, u8 __user arg);

int iav_set_digital_zoom_ex(iav_context_t *context, struct iav_digital_zoom_ex_s __user *arg);
int iav_get_digital_zoom_ex(iav_context_t *context, struct iav_digital_zoom_ex_s __user *arg);
int iav_set_2nd_digital_zoom_ex(iav_context_t * context, struct iav_digital_zoom_ex_s __user * arg);
int iav_get_2nd_digital_zoom_ex(iav_context_t * context, struct iav_digital_zoom_ex_s __user * arg);

int iav_set_vin_capture_win(iav_context_t * context, struct iav_rect_ex_s __user * arg);
int iav_get_vin_capture_win(iav_context_t * context, struct iav_rect_ex_s __user *arg);

int iav_set_digital_zoom_privacy_mask_ex(iav_context_t * context, struct iav_digital_zoom_privacy_mask_ex_s __user * arg);

int iav_set_system_setup_info(iav_context_t * context, iav_system_setup_info_ex_t * arg);
int iav_get_system_setup_info(iav_context_t * context, iav_system_setup_info_ex_t * arg);
int iav_set_system_resource_limit_ex(iav_context_t *context, struct iav_system_resource_setup_ex_s __user * arg);
int iav_get_system_resource_limit_ex(iav_context_t *context, struct iav_system_resource_setup_ex_s __user * arg);
int iav_set_sharpen_filter_config_ex(iav_context_t *context, struct iav_sharpen_filter_cfg_s __user * arg);
int iav_get_sharpen_filter_config_ex(iav_context_t *context, struct iav_sharpen_filter_cfg_s __user * arg);

int iav_get_chip_id_ex(iav_context_t* context, int __user *arg);

int iav_enc_dram_request_frame(iav_context_t * context, struct iav_enc_dram_buf_frame_ex_s __user *arg);
int iav_enc_dram_release_frame(iav_context_t * context, struct iav_enc_dram_buf_frame_ex_s __user *arg);
int iav_enc_dram_buf_update_frame(iav_context_t * context, struct iav_enc_dram_buf_update_ex_s __user *arg);

int iav_query_encmode_cap_ex(iav_context_t * context, struct iav_encmode_cap_ex_s __user *arg);
int iav_query_encbuf_cap_ex(iav_context_t * context, struct iav_encbuf_cap_ex_s __user * arg);

int iav_get_video_proc(iav_context_t * context, void __user * arg);
int iav_cfg_video_proc(iav_context_t * context, void __user * arg);
int iav_apply_video_proc(iav_context_t * context, void __user * arg);

int iav_get_vcap_proc(iav_context_t * context, void __user * arg);
int iav_cfg_vcap_proc(iav_context_t * context, void __user * arg);
int iav_apply_vcap_proc(iav_context_t * context, void __user * arg);
int iav_get_frm_buf_pool_info(u32 *id_map);

int iav_vout_cross_check(iav_context_t *context, struct amba_video_sink_mode *cfg);
int iav_vout_update_sink_cfg(iav_context_t *context, struct amba_video_sink_mode *cfg);

// iav_overlay.c
int iav_overlay_insert_ex(iav_context_t *context, struct overlay_insert_ex_s __user *arg);
int iav_get_overlay_insert_ex(iav_context_t * context, struct overlay_insert_ex_s __user * arg);

//iav_test.c
int iav_test(iav_context_t *context, u32 arg);
int iav_test_add_print_in_isr(u32 args0, u32 args1, u32 args2, u32 args3);

int iav_log_setup(iav_context_t *context, struct iav_dsp_setup_s __user * arg);
int iav_debug_setup(iav_context_t * context, struct iav_debug_setup_s __user * arg);
int iav_start_vin_fps_stat(iav_context_t *context, u8 arg);
int iav_get_vin_fps_stat(iav_context_t *context, struct amba_fps_report_s * fps);

#endif

