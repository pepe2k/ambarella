#ifndef __IMG_CUSTOMER_INTERFACE_H__
#define __IMG_CUSTOMER_INTERFACE_H__

int img_idsp_filters_init(int fd_iav);

// adj
int adj_video_init(adj_param_t *p_adj_video_param, pipeline_control_t *p_adj_video_pipe);
int adj_video_aeawb_control(u16 table_index, adj_awb_ae_t * p_adj_param, adj_aeawb_control_t * ouput_aeawb_param);
int adj_video_black_level_control(u16 table_index, wb_gain_t wb_gain,adj_param_t *p_adj_video_param, pipeline_control_t *p_adj_video_pipe);
int adj_video_ev_img(u16 table_index, wb_gain_t wb_gain, adj_param_t * p_adj_video_param, pipeline_control_t * p_adj_video_pipe, chroma_scale_filter_t * p_adj_chroma_scale_table);
int adj_video_noise_control(u16 table_index, wb_gain_t wb_gain, u16 dzoom_step, adj_param_t * p_adj_video_param, pipeline_control_t * p_adj_video_pipe);
void adj_set_color_conversion(rgb_to_yuv_t* rgb_to_yuv_matrix);
int adj_video_color_contrast(pipeline_control_t *p_adj_video_pipe, rgb_histogram_stat_t* p_rgb_hist);

int adj_set_hue(int hue);
int adj_set_brightness(int bright);
int adj_set_contrast(int contrast);
int adj_set_saturation(int saturation);
void adj_set_mctf(int level);
void adj_set_bw_mode(u8 en);
void adj_set_local_exp(int level);
int adj_lowluma_contrast(adj_param_t *p_adj_video_param,pipeline_control_t *p_adj_video_pipe, u8 ae_stable, local_exposure_t* manual_LE);
void adj_set_sharpness_level(int level);
void adj_set_spacial_denoise_level(int level);
void adj_set_cnf_strength(int level);
void adj_set_cnf_max_radius(int max_radius);

//awb
void awb_set_wb_ratio(adj_aeawb_control_t *p_aeawb_ctl);
void awb_control(u16 tile_count, awb_data_t *p_tile_info, awb_gain_t *p_awb_gain);
int awb_control_init(wb_gain_t *menu_gain, awb_lut_t *lut_table, awb_lut_idx_t *lut_table_idx);
int awb_set_wb_mode(awb_control_mode_t mode);
awb_control_mode_t awb_get_wb_mode(void);
int awb_set_wb_method(awb_work_method_t method);
awb_work_method_t awb_get_wb_method(void);
void awb_set_cali_thr(u16 r_thr, u16 b_thr);
void awb_set_wb_shift(wb_gain_t* org_gain, wb_gain_t* ref_gain);
void awb_gain_comp(wb_gain_t * wb_gain,wb_cali_comp_t direction);
void awb_set_speed(u8 speed);
void awb_get_cal(wb_gain_t* get_org_gain);
void awb_set_custom_gain(wb_gain_t* cus_gain);
void awb_set_environment(awb_environment_t env);
awb_environment_t awb_get_environment(void);
void awb_set_failure_remedy(awb_failure_remedy_t remedy);

//main
void dynamic_tone_curve_enable(u8 enable);
int dynamic_tone_curve_set_alpha(float alpha);// defog strength,0.0-1.0,default 0.4

#endif // __IMG_CUSTOMER_INTERFACE_H__

