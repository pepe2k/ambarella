#ifndef	IMG_DSP_INTERFACE_ARCH_H
#define	IMG_DSP_INTERFACE_ARCH_H
typedef void* (*img_malloc)(u32 size);
typedef void (*img_free)(void *ptr);
typedef void* (*img_memset)(void *s, int c, u32 n);
typedef void* (*img_memcpy)(void *dest, const void *src, u32 n);
typedef int (*img_ioctl)(int handle, int request, ...);
typedef int (*img_print)(const char*format, ...);

typedef struct mem_op_s{
	img_malloc	_malloc;
	img_free		_free;
	img_memset	_memset;
	img_memcpy	_memcpy;
}mem_op_t;

typedef struct capture_region_s {
   unsigned int pitch;
   unsigned int startCol;
   unsigned int startRow;
   unsigned int endCol;
   unsigned int endRow;
}capture_region_t;

void img_dsp_register_mem(mem_op_t *op);
void img_dsp_register_print(img_print p_func);
void img_dsp_register_ioc(img_ioctl p_func);

int img_dsp_init(u32 enable_defblc, u32 enable_sharpen_b);
int img_dsp_deinit(void);

void img_dsp_get_lib_version(img_lib_version_t* p_img_dsp_info );//only can be called after img_lib_init()
int img_dsp_get_statistics(int fd_iav, img_aaa_stat_t *p_stat, aaa_tile_report_t *p_act_tile);
int img_dsp_get_statistics_raw(int fd_iav, rgb_aaa_stat_t *p_rgb_stat, cfa_aaa_stat_t *p_cfa_stat);
int img_dsp_get_statistics_hdr(int fd_iav, img_aaa_stat_t* p_stat, aaa_tile_report_t* p_act_tile);
int img_dsp_get_statistics_multi_vin(int fd_iav, img_aaa_stat_t * p_stat, aaa_tile_report_t * p_act_tile);
int img_dsp_config_statistics_info(int fd_iav, statistics_config_t* setup);
int img_dsp_set_af_statistics_ex(int fd_iav, af_statistics_ex_t *paf_ex, u8 enable);
int img_dsp_get_af_statistics_ex(af_statistics_ex_t* setup);
int img_dsp_get_config_statistics_info(statistics_config_t* setup);
int img_dsp_set_config_float_statistics_info(int fd_iav, float_statistics_config_t *aaa_float_stats_info);
int img_dsp_get_config_float_statistics_info(int fd_iav, float_statistics_config_t * setup);
int img_dsp_set_config_histogram_info(int fd_iav, histogram_config_t* aaa_hist_info);
int img_dsp_get_config_histogram_info(int fd_iav, histogram_config_t* setup);

int img_dsp_set_def_blc(int fd_iav, u8 enable, s16 def_amount[3]);
int img_dsp_get_def_blc(u8* enable, s16* def_amount);

int img_dsp_set_global_blc(int fd_iav, blc_level_t* blc, bayer_pattern pat);
int img_dsp_get_global_blc(blc_level_t* blc);

int img_dsp_set_static_bad_pixel_correction(int fd_iav, fpn_correction_t *fpn);
int img_dsp_set_fpn_col_gain(int fd_iav, fpn_col_gain_t *fpn_col_gain);
int img_dsp_set_dynamic_bad_pixel_correction(int fd_iav, dbp_correction_t *bad_corr);
int img_dsp_get_dynamic_bad_pixel_correction(dbp_correction_t *bad_corr);

int img_dsp_set_cfa_leakage_filter(int fd_iav, cfa_leakage_filter_t *cfa_leakage_filter);
int img_dsp_get_cfa_leakage_filter(cfa_leakage_filter_t *cfa_leakage_filter);

void img_dsp_set_cfa_gain_flag(u8 flag);
int img_dsp_set_cfa_noise_filter(int fd_iav, cfa_noise_filter_info_t* cfa_noise_filter);
int img_dsp_get_cfa_noise_filter(cfa_noise_filter_info_t* cfa_noise_filter);

int img_dsp_set_anti_aliasing(int fd_iav, u8 strength);
u8 img_dsp_get_anti_aliasing(void);

int img_dsp_set_rgb_gain(int fd_iav, wb_gain_t * wb_gain, u32 dgain);
int img_dsp_get_rgb_gain(wb_gain_t* wb_gain, u32* dgain);
int img_dsp_set_wb_gain(int fd_iav, wb_gain_t* wb_gain);
int img_dsp_set_dgain(int fd_iav,u32 dgain);
int img_dsp_set_global_dgain(u32 dgain);

int img_dsp_set_dgain_saturation_level(int fd_iav, digital_sat_level_t* dgain_saturation);
int img_dsp_get_dgain_saturation_level(digital_sat_level_t* dgain_saturation);

int img_dsp_set_local_exposure(int fd_iav, local_exposure_t *local_exposure);
int img_dsp_get_local_exposure(local_exposure_t *local_exposure);

int img_dsp_enable_color_correction(int fd_iav);
int img_dsp_set_color_correction_reg(color_correction_reg_t *color_corr_reg);
int img_dsp_set_color_correction(int fd_iav, color_correction_t *color_corr);
int img_dsp_set_tone_curve(int fd_iav, tone_curve_t *tone_curve);
int img_dsp_get_tone_curve(tone_curve_t *tone_curve);
int img_dsp_set_sec_cc_en(int fd_iav, u8 enable);

int img_dsp_set_rgb2yuv_matrix(int fd_iav, rgb_to_yuv_t* matrix);
int img_dsp_get_rgb2yuv_matrix(rgb_to_yuv_t* matrix);

int img_dsp_set_chroma_scale(int fd_iav, chroma_scale_filter_t* chroma_scale);
int img_dsp_get_chroma_scale(chroma_scale_filter_t* chroma_scale);

int img_dsp_set_chroma_median_filter(int fd_iav, chroma_median_filter_t *chroma_median_setup);
int img_dsp_get_chroma_median_filter(chroma_median_filter_t *chroma_median_setup);

int img_dsp_set_chroma_noise_filter(int fd_iav, u8 mode, chroma_filter_t *chroma_filter);
int img_dsp_get_chroma_noise_filter(chroma_filter_t *chroma_filter);
int img_dsp_set_chroma_noise_filter_max_radius(u16 radius);

int img_dsp_set_vignette_compensation(int fd_iav, vignette_info_t *vignette_info);

int img_dsp_set_luma_high_freq_noise_reduction(int fd_iav, luma_high_freq_nr_t *lhfnr);
int img_dsp_get_luma_high_freq_noise_reduction(luma_high_freq_nr_t *lhfnr);

int img_dsp_init_video_mctf(int fd_iav);
int img_dsp_set_video_mctf(int fd_iav, video_mctf_info_t *mctf_info);
int img_dsp_get_video_mctf(video_mctf_info_t *mctf_info);
int img_dsp_set_video_mctf_enable(u8 enable); // I think it will be removed
int img_dsp_set_video_mctf_compression_enable(u8 enable);// I think it will be removed
int img_dsp_set_video_mctf_ghost_prv(int fd_iav, video_mctf_ghost_prv_info_t *gp_info);
int img_dsp_set_video_mctf_zmv_enable(u8 enable);
int img_dsp_get_video_mctf_zmv_enable(void);
int img_dsp_set_global_motion_vector(int fd_iav, gmv_info_t *gmv_info);
int img_dsp_get_global_motion_vector(gmv_info_t *gmv_info);

int img_dsp_set_color_dependent_noise_reduction(int fd_iav, u8 vs_mode, cdnr_info_t *cdnr);
int img_dsp_get_color_dependent_noise_reduction(cdnr_info_t *cdnr);

int img_dsp_set_video_sharpen_a_or_spatial_filter(int fd_iav, u8 select);
u8 img_dsp_get_video_sharpen_a_or_spatial_filter(void);
int img_dsp_set_color_correction_desaturation(int fd_iav, cc_desat_t *cc_desat);

int img_dsp_set_advance_spatial_filter(int fd_iav, u8 mode, asf_info_t *asf);
int img_dsp_get_advance_spatial_filter(u8 mode, asf_info_t *asf);

int img_dsp_set_sharpen_a_max_change(int fd_iav, u8 mode, max_change_t *max_change);
int img_dsp_set_sharpen_b_max_change(int fd_iav, u8 mode, max_change_t *max_change);
int img_dsp_get_sharpen_a_max_change(u8 mode, max_change_t* max_change);
int img_dsp_get_sharpen_b_max_change(u8 mode, max_change_t* max_change);
int img_dsp_set_sharpen_a_coring(int fd_iav, u8 mode, coring_table_t *coring);
int img_dsp_set_sharpen_b_coring(int fd_iav, u8 mode, coring_table_t *coring);
int img_dsp_get_sharpen_a_coring(u8 mode, coring_table_t *coring);
int img_dsp_get_sharpen_b_coring(u8 mode, coring_table_t *coring);
int img_dsp_set_sharpen_a_fir(int fd_iav, u8 mode, fir_t *fir);
int img_dsp_set_sharpen_b_fir(int fd_iav, u8 mode, fir_t *fir);
int img_dsp_get_sharpen_a_fir(u8 mode, fir_t *fir);
int img_dsp_get_sharpen_b_fir(u8 mode, fir_t *fir);
int img_dsp_set_sharpen_a_signal_retain_level(int fd_iav, u8 mode, u8 level);
int img_dsp_set_sharpen_b_signal_retain_level(int fd_iav, u8 mode, u8 level);
u8 img_dsp_get_sharpen_a_signal_retain_level(u8 mode);
u8 img_dsp_get_sharpen_b_signal_retain_level(u8 mode);
int img_dsp_set_sharpen_a_level_minimum(int fd_iav, u8 mode, sharpen_level_t *level);
int img_dsp_set_sharpen_b_level_minimum(int fd_iav, u8 mode, sharpen_level_t *level);
int img_dsp_get_sharpen_a_level_minimum(u8 mode, sharpen_level_t *level);
int img_dsp_get_sharpen_b_level_minimum(u8 mode, sharpen_level_t *level);
int img_dsp_set_sharpen_a_level_overall(int fd_iav, u8 mode, sharpen_level_t *level);
int img_dsp_set_sharpen_b_level_overall(int fd_iav, u8 mode, sharpen_level_t *level);
int img_dsp_get_sharpen_a_level_overall(u8 mode, sharpen_level_t* level);
int img_dsp_get_sharpen_b_level_overall(u8 mode, sharpen_level_t* level);
int img_dsp_set_sharpen_b_linearization(int fd_iav, u8 mode, u16 strength);
u16 img_dsp_get_sharpen_b_linearization(u8 mode);

int img_dsp_sensor_config(int fd_iav, sensor_info_t *psensor_info);

int img_dsp_set_art_effect(int fd_iav, u8 mode, u8 strength);

int img_dsp_set_yuv_contrast(int fd_iav, yuv_contrast_config_t *p_hdr_yuv_contrast);

// HDR related
int img_dsp_set_hdr_batch(int fd_iav,hdr_proc_data_t * p_hdr_proc);
int img_dsp_get_hdr_batch(hdr_proc_data_t *p_hdr_proc);
int img_dsp_set_hdr_video_proc(int fd_iav,video_proc_config_t * p_video_proc);
int img_dsp_set_hdr_row_offset(int fd_iav,raw_offset_config_t * p_raw_offset);

//int img_dsp_set_dzoom_factor(int fd_iav, zoom_factor_info_t* dzoom_factor);//removed from A7
//int img_dsp_get_dzoom_factor(zoom_factor_info_t* dzoom_factor);//removed from A7
//int img_dsp_set_global_motion_vector(int fd_iav, gmv_info_t *gmv_info);//removed from A7
//int img_dsp_dump(int fd_iav,int id_sec,u8* dsp_dump_info,int buffer_size);//removed from A7

int img_dsp_set_warp_config(int fd_iav, int mode, warp_correction_t *pwarp_info_input);

void img_dsp_get_default_bin_map(idsp_def_bin_t* out);
int img_dsp_load_default_bin(u32 addr, idsp_one_def_bin_t type);
int img_dsp_set_still_capture_info(still_cap_info_t *still_cap_info);
int img_dsp_dec_raw(unsigned int *raw_ptr, capture_region_t *region, unsigned short *dest);

int img_dsp_set_video_freeze(int fd_iav, int freeze);

#endif
