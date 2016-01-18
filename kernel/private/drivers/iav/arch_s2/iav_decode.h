
#ifndef __IAV_DECODE_H__
#define __IAV_DECODE_H__

int iav_decode_init(void *dev);
void iav_decode_deinit(void);
int __iav_decode_ioctl(iav_context_t *context, unsigned int cmd, unsigned long arg);

void iav_decode_release(iav_context_t *context);

int iav_enter_decode_mode(iav_context_t *context);
int iav_leave_decode_mode(iav_context_t *context);
int iav_decode_stop(iav_context_t *context);

#if 0
int iav_decode_jpeg(iav_context_t *context, iav_jpeg_info_t __user *arg);
int iav_decode_h264(iav_context_t *context, iav_h264_decode_t __user *arg);
int iav_decode_multi(iav_context_t *context, iav_multi_info_t __user *arg);

int iav_decode_pause(iav_context_t *context);
int iav_decode_resume(iav_context_t *context);
int iav_decode_step(iav_context_t *context);
int iav_decode_trick_play(iav_context_t *context, iav_trick_play_t __user *arg);

int iav_get_decode_info(iav_context_t *context, iav_decode_info_t __user *arg);
int iav_decode_wait_bsb_emptiness(iav_context_t *context, iav_bsb_emptiness_t __user *arg);
int iav_decode_wait_eos(iav_context_t *context, iav_wait_eos_t __user *arg);

int iav_config_decoder(iav_context_t *context, iav_config_decoder_t __user *arg);
int iav_config_display(iav_context_t *context, iav_config_display_t __user *arg);
#endif

int iav_enter_udec_mode(iav_context_t *context, iav_udec_mode_config_t __user *arg);

int iav_request_frame(iav_context_t *context, iav_frame_buffer_t __user *arg);
int iav_get_decoded_frame(iav_context_t *context, iav_frame_buffer_t __user *arg);
int iav_render_frame(iav_context_t *context, iav_frame_buffer_t __user * arg, int release_only);
int iav_postp_frame(iav_context_t *context, iav_frame_buffer_t __user *arg);

int iav_decode_wait(iav_context_t *context, iav_wait_decoder_t __user *arg);
int iav_decode_flush(iav_context_t *context);

int iav_init_udec(iav_context_t *context, iav_udec_info_ex_t __user *arg);
int iav_release_udec(iav_context_t *context, u32 arg);

int iav_udec_decode(iav_context_t *context, iav_udec_decode_t __user *arg);
int iav_stop_udec(iav_context_t *context, u32 udec_id);

int iav_wait_udec_status(iav_context_t *context, iav_udec_status_t __user *arg);

int iav_udec_map(iav_context_t *context, iav_udec_map_t __user *arg);
int iav_udec_unmap(iav_context_t *context);

int iav_config_fb_pool(iav_context_t *context, iav_fbp_config_t __user *arg);

int iav_decode_dbg(iav_context_t *context, u32 arg);

#endif

