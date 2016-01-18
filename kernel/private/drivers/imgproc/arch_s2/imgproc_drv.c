#include <amba_common.h>
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "utils.h"

#include "dsp_cmd.h"
#include "dsp_api.h"
#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"
#include "iav_common.h"
#include "iav_drv.h"
#include "imgproc_drv.h"
#include "imgproc_pri.h"

typedef struct hdr_exposure_ctrl_s {
	u8	enable;
	u8	updated;
	u8	cmd_num;
	u8	reserved;
} hdr_exposure_ctrl_t;


//#define DSP_LOG
#define PRINT_IMGPROC	(0)
#define PIXEL_MAP_MAX_SIZE		(4096*3008/8)	//width & height round of 16
#define	INPUT_LOOK_UP_TABLE_SIZE	(192*3*4)
#define	MATRIX_TABLE_SIZE		(16*16*16*4)
#define	OUTPUT_LOOK_UP_TABLE_SIZE	(256*4)
#define	JPEG_QT_SIZE	(128)
#define	SEC_CC_SIZE (20608) //A7 secondary CC file size

#define	HDR_IDSP_CMD_SIZE		(128)
#define	HDR_IDSP_CMD_NUM		(12)
#define	HDR_IDSP_EXPO_BUF_SIZE	(HDR_IDSP_CMD_SIZE * HDR_IDSP_CMD_NUM)
#define	HDR_PROC_BUFFRE_SIZE		(HDR_IDSP_EXPO_BUF_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_ALPHA_MAP_SIZE		(HDR_IDSP_CMD_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_CC_INPUT_TABLE_SIZE	(INPUT_LOOK_UP_TABLE_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_CC_OUTPUT_TABLE_SIZE	(OUTPUT_LOOK_UP_TABLE_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_CC_MATRIX_SIZE		(MATRIX_TABLE_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_BPC_TABLE_SIZE		(DYN_BPC_THD_TABLE_SIZE * MAX_EXPOSURE_NUM)
#define	HDR_YUV_CNTL_BUFF_SIZE	(HDR_IDSP_CMD_SIZE)

#define	HDR_BUF_NUM		(2)

#define	FLOAT_TILE_CONFIG_SIZE	(320)

#define HISO_CFG_DATA_SIZE		(1024)
#define IMG_IK_MEM_SIZE			(4<<20)
#define IMG_IMGPROC_MEM_SIZE		(64<<20)

static void 	*rgb_aaa_ptr = NULL;
static u32	rgb_aaa_phys = 0;
static u32	cfa_aaa_phys = 0;
static void 	*cfa_aaa_ptr = NULL;
static u32	rgb_aaa_size = 0;
static u32	cfa_aaa_size = 0;
static void	*hist_aaa_ptr = NULL;
static u32	hist_pitch_size = 0;
static void	*cfa_start_ptr = NULL;
static void	*cfa_end_ptr = NULL;
static void	*rgb_start_ptr = NULL;
static void	*rgb_end_ptr = NULL;
static void	*vignette_r_gain = NULL;
static void	*vignette_ge_gain = NULL;
static void	*vignette_go_gain = NULL;
static void	*vignette_b_gain = NULL;
static void	*exposure_gain_curve = NULL;
static void	*input_lookup_table = NULL;
static void	*matrix_dram_address = NULL;
static void	*output_lookup_table = NULL;
static void	*chroma_gain_curve = NULL;
static void	*k0123_table = NULL;
static void	*luma_sharpening_alpha_table = NULL;
static void	*coeff_fir1_addr = NULL;
static void	*coeff_fir2_addr = NULL;
static void	*coring_table = NULL;
static void	*hdr_coeff_fir1_addr = NULL;
static void	*hdr_coring_table = NULL;
static void	*hot_pixel_thd_table = NULL;
static void	*dark_pixel_thd_table = NULL;
static void	*mctf_cfg_addr = NULL;
static void	*cc_cfg_addr = NULL;
static void 	*cmpr_cfg_addr = NULL;
static void	*pixel_map_addr = NULL;
static void	*fpn_reg_addr = NULL;
static u32*	dsp_raw_info = NULL;
static void	*jpeg_quant_matrix = NULL;
static void	*column_acc;
static void	*column_offset;
static void 	*lnl_tone_curve;
static void	*luma_3d_table;

static void	*hdr_proc_buf = NULL;
static void	*hdr_alpha_map_buf = NULL;
static void	*hdr_yuv_cntl_buf = NULL;
static void	*float_tile_config_addr = NULL;

static void	*hdr_alpha_map_input_lookup_table = NULL;
static void	*hdr_alpha_map_matrix_dram_address = NULL;
static void	*hdr_alpha_map_output_lookup_table = NULL;

static void	*hdr_cc_input_lookup_table = NULL;
static void	*hdr_cc_matrix_dram_address = NULL;
static void	*hdr_cc_output_lookup_table = NULL;

static void	*hdr_bpc_hot_thd_table = NULL;
static void	*hdr_bpc_dark_thd_table = NULL;

static u8	g_dsp_mode;
static DECLARE_COMPLETION(g_statis_comp);
static DECLARE_COMPLETION(g_raw2yuv_comp);
static DECLARE_WAIT_QUEUE_HEAD(g_dsp_wq);

static hdr_exposure_ctrl_t g_hdr_expo[MAX_EXPOSURE_NUM];
static u8 g_curr_hdr_buf_idx = 0;
static u8 g_curr_expo_idx = 0;

static struct video_hdr_yuv_cntl_param g_yuv_ce_info;

static u32 phys_start = 0;
static u32 user_start = 0;

static int need_remap = 1;


void clean_cache_aligned(u8 *kernel_start, unsigned long size)
{
	unsigned long offset = (unsigned long)kernel_start & (CACHE_LINE_SIZE - 1);
	kernel_start -= offset;
	size += offset;
	clean_d_cache(kernel_start, size);
}

static u32 _get_phys_addr_raw2enc(u8* usr_addr)
{
	if ((u32)usr_addr < user_start || (u32)usr_addr >= (user_start+IMG_IMGPROC_MEM_SIZE)) {
		img_printk("HISO addr [%p] not in range [%p, %p].\n", usr_addr,
			(void*)user_start, (void*)(user_start+IMG_IMGPROC_MEM_SIZE));
		return (u32)usr_addr;
	}

	return (u32)(usr_addr - user_start + phys_start);
}

#if 0
static u32 _get_phys_addr_ik(u8* usr_addr)
{
	if ((u32)usr_addr < user_start || (u32)usr_addr >= (user_start+IMG_IK_MEM_SIZE)) {
		img_printk("HISO addr [%p] not in range [%p, %p].\n", usr_addr,
			(void*)user_start, (void*)(user_start+IMG_IK_MEM_SIZE));
		return (u32)usr_addr;
	}

	return (u32)(usr_addr - user_start + phys_start);
}
#endif

static int img_init(void)
{
	if ((input_lookup_table = kzalloc(INPUT_LOOK_UP_TABLE_SIZE, GFP_KERNEL))==NULL) {
		img_printk("input_lookup_table malloc error\n");
		goto OUT;
	}
	if ((matrix_dram_address = kzalloc(MATRIX_TABLE_SIZE, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}
	if ((output_lookup_table = kzalloc(OUTPUT_LOOK_UP_TABLE_SIZE, GFP_KERNEL))==NULL) {
		img_printk("output_lookup_table malloc error\n");
		goto OUT;
	}
	if ((chroma_gain_curve = kzalloc(NUM_CHROMA_GAIN_CURVE*2, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}

	if ((hot_pixel_thd_table = kzalloc(DYN_BPC_THD_TABLE_SIZE, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}

	if ((dark_pixel_thd_table = kzalloc(DYN_BPC_THD_TABLE_SIZE, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}

	if ((mctf_cfg_addr = kzalloc(MCTF_CFG_SIZE, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}

	if ((cc_cfg_addr = kzalloc(SEC_CC_SIZE, GFP_KERNEL))==NULL) {
		img_printk("secondary cc_cfg_addr malloc error\n");
		goto OUT;
	}
	if ((cmpr_cfg_addr = kzalloc(544, GFP_KERNEL))==NULL) {
		img_printk("secondary cmpr_cfg_addr malloc error\n");
		goto OUT;
	}

	if ((k0123_table = kzalloc(K0123_ARRAY_SIZE*sizeof(u16), GFP_KERNEL))==NULL) {
		img_printk("k0123_table malloc error\n");
		goto OUT;
	}

	if ((exposure_gain_curve = kzalloc(NUM_EXPOSURE_CURVE*sizeof(u16)*MAX_EXPOSURE_NUM, GFP_KERNEL))==NULL) {
		img_printk("exposure_gain_curve malloc error\n");
		goto OUT;
	}

	if ((luma_sharpening_alpha_table = kzalloc(NUM_ALPHA_TABLE, GFP_KERNEL))==NULL) {
		img_printk("luma_sharpening_alpha_table malloc error\n");
		goto OUT;
	}
	if ((coeff_fir1_addr = kzalloc(256, GFP_KERNEL))==NULL) {
		img_printk("coeff_fir1_addr malloc error\n");
		goto OUT;
	}
	if ((coeff_fir2_addr = kzalloc(256, GFP_KERNEL))==NULL) {
		img_printk("coeff_fir2_addr malloc error\n");
		goto OUT;
	}
	if ((coring_table = kzalloc(256, GFP_KERNEL))==NULL) {
		img_printk("coring_table malloc error\n");
		goto OUT;
	}
	if ((hdr_coeff_fir1_addr = kzalloc(256, GFP_KERNEL))==NULL) {
		img_printk("hdr_coeff_fir1_addr malloc error\n");
		goto OUT;
	}
	if ((hdr_coring_table = kzalloc(256, GFP_KERNEL))==NULL) {
		img_printk("hdr_coring_table malloc error\n");
		goto OUT;
	}

	if ((vignette_r_gain = kzalloc(MAX_VIGNETTE_NUM*4, GFP_KERNEL))==NULL) {
		img_printk("vignette_r_gain malloc error\n");
		goto OUT;
	}
	vignette_go_gain = (void*)((u32)vignette_r_gain + MAX_VIGNETTE_NUM);
	vignette_ge_gain = (void*)((u32)vignette_go_gain + MAX_VIGNETTE_NUM);
	vignette_b_gain = (void*)((u32)vignette_ge_gain + MAX_VIGNETTE_NUM);
	if((pixel_map_addr = kzalloc(PIXEL_MAP_MAX_SIZE, GFP_KERNEL))==NULL) {
		img_printk("bad pixel map malloc error\n");
		goto OUT;
	}
	if((fpn_reg_addr = kzalloc(1024, GFP_KERNEL)) ==NULL) {
		img_printk("bad pixel map malloc error\n");
		goto OUT;
	}
	memset(fpn_reg_addr, 0, 1024);

	if((dsp_raw_info = kzalloc(64, GFP_KERNEL)) ==NULL) {
		img_printk("dsp raw info malloc error\n");
		goto OUT;
	}
	if((jpeg_quant_matrix = kzalloc(JPEG_QT_SIZE, GFP_KERNEL)) ==NULL) {
		img_printk("jpeg_quant_matrix malloc error\n");
		goto OUT;
	}
	if((column_acc = kzalloc(8192, GFP_KERNEL)) == NULL){
		img_printk("can not malloc memory for column_acc!\n");
		goto OUT;
	}
	if((column_offset = kzalloc(4096, GFP_KERNEL)) == NULL){
		img_printk("can not malloc memory for column_offset!\n");
		goto OUT;
	}
	if((lnl_tone_curve = kzalloc(256*sizeof(u16), GFP_KERNEL)) == NULL){
		img_printk("can not malloc memory for lnl_tone_curve!\n");
		goto OUT;
	}
	if ((hdr_proc_buf = kzalloc(HDR_PROC_BUFFRE_SIZE * HDR_BUF_NUM, GFP_KERNEL)) == NULL) {
		img_printk("can not malloc memory for HDR!\n");
		goto OUT;
	}
	if ((hdr_alpha_map_buf = kzalloc(HDR_ALPHA_MAP_SIZE * HDR_BUF_NUM, GFP_KERNEL)) == NULL) {
		img_printk("can not malloc memory for HDR!\n");
		goto OUT;
	}
	if ((hdr_yuv_cntl_buf = kzalloc(HDR_YUV_CNTL_BUFF_SIZE * HDR_BUF_NUM, GFP_KERNEL)) == NULL) {
		img_printk("can not malloc memory for HDR!\n");
		goto OUT;
	}
	if ((hdr_alpha_map_input_lookup_table = kzalloc(HDR_CC_INPUT_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("input_lookup_table malloc error\n");
		goto OUT;
	}
	if ((hdr_alpha_map_matrix_dram_address = kzalloc(HDR_CC_MATRIX_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}
	if ((hdr_alpha_map_output_lookup_table = kzalloc(HDR_CC_OUTPUT_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("output_lookup_table malloc error\n");
		goto OUT;
	}
	if ((hdr_cc_input_lookup_table = kzalloc(HDR_CC_INPUT_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("input_lookup_table malloc error\n");
		goto OUT;
	}
	if ((hdr_cc_matrix_dram_address = kzalloc(HDR_CC_MATRIX_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}
	if ((hdr_cc_output_lookup_table = kzalloc(HDR_CC_OUTPUT_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("output_lookup_table malloc error\n");
		goto OUT;
	}
	if ((hdr_bpc_hot_thd_table = kzalloc(HDR_BPC_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}
	if ((hdr_bpc_dark_thd_table = kzalloc(HDR_BPC_TABLE_SIZE * HDR_BUF_NUM, GFP_KERNEL))==NULL) {
		img_printk("matrix_dram_address malloc error\n");
		goto OUT;
	}
	if((float_tile_config_addr = kzalloc(FLOAT_TILE_CONFIG_SIZE, GFP_KERNEL)) == NULL){
		img_printk("can not malloc memory for FLOAT TILE!\n");
		goto OUT;
	}
	if((luma_3d_table = kzalloc(LS_THREE_D_TABLE_SIZE>>1, GFP_KERNEL)) == NULL){
		img_printk("can not malloc memory for FLOAT TILE!\n");
		goto OUT;
	}

	return 0;

OUT:
	kfree(chroma_gain_curve);
	kfree(input_lookup_table);
	kfree(matrix_dram_address);
	kfree(hot_pixel_thd_table);
	kfree(dark_pixel_thd_table);
	kfree(mctf_cfg_addr);
	kfree(cc_cfg_addr);
	kfree(cmpr_cfg_addr);
	kfree(output_lookup_table);
	kfree(k0123_table);
	kfree(exposure_gain_curve);
	kfree(luma_sharpening_alpha_table);
	kfree(coeff_fir1_addr);
	kfree(coeff_fir2_addr);
	kfree(coring_table);
	kfree(hdr_coeff_fir1_addr);
	kfree(hdr_coring_table);
	kfree(vignette_r_gain);
	kfree(pixel_map_addr);
	kfree(fpn_reg_addr);
	kfree(dsp_raw_info);
//	kfree(hiLow_iso_param);
	kfree(jpeg_quant_matrix);
	kfree(column_acc);
	kfree(column_offset);
	kfree(lnl_tone_curve);
	kfree(hdr_proc_buf);
	kfree(hdr_yuv_cntl_buf);
	kfree(hdr_alpha_map_buf);
	kfree(hdr_alpha_map_input_lookup_table);
	kfree(hdr_alpha_map_matrix_dram_address);
	kfree(hdr_alpha_map_output_lookup_table);
	kfree(hdr_cc_input_lookup_table);
	kfree(hdr_cc_matrix_dram_address);
	kfree(hdr_cc_output_lookup_table);
	kfree(hdr_bpc_hot_thd_table);
	kfree(hdr_bpc_dark_thd_table);
	kfree(float_tile_config_addr);
	kfree(luma_3d_table);
	return -ENOMEM;
}

static void img_exit(void)
{
	kfree(chroma_gain_curve);
	kfree(input_lookup_table);
	kfree(matrix_dram_address);
	kfree(hot_pixel_thd_table);
	kfree(dark_pixel_thd_table);
	kfree(mctf_cfg_addr);
	kfree(cc_cfg_addr);
	kfree(cmpr_cfg_addr);
	kfree(output_lookup_table);
	kfree(k0123_table);
	kfree(exposure_gain_curve);
	kfree(luma_sharpening_alpha_table);
	kfree(coeff_fir1_addr);
	kfree(coeff_fir2_addr);
	kfree(coring_table);
	kfree(hdr_coeff_fir1_addr);
	kfree(hdr_coring_table);
	kfree(vignette_r_gain);
	kfree(pixel_map_addr);
	kfree(fpn_reg_addr);
	kfree(dsp_raw_info);
	kfree(jpeg_quant_matrix);
	kfree(column_acc);
	kfree(column_offset);
	kfree(lnl_tone_curve);
	kfree(hdr_proc_buf);
	kfree(hdr_yuv_cntl_buf);
	kfree(hdr_alpha_map_buf);
	kfree(hdr_alpha_map_input_lookup_table);
	kfree(hdr_alpha_map_matrix_dram_address);
	kfree(hdr_alpha_map_output_lookup_table);
	kfree(hdr_cc_input_lookup_table);
	kfree(hdr_cc_matrix_dram_address);
	kfree(hdr_cc_output_lookup_table);
	kfree(hdr_bpc_hot_thd_table);
	kfree(hdr_bpc_dark_thd_table);
	kfree(float_tile_config_addr);
	kfree(luma_3d_table);
}

#if PRINT_IMGPROC
static void statis_CFA_print(void* p_cfa, const u8 slice)
{
	struct cfa_aaa_stat *p_aaa;
	struct cfa_ae_stat *p_ae;
	struct af_stat *p_af;
	struct float_cfa_awb_stat* p_float_awb;
	struct float_cfa_ae_stat* p_float_ae;
	struct float_cfa_af_stat* p_float_af;
	const u8 float_tile_num = 32;
	int awb_tile_num_col, awb_tile_num_row;
	int ae_tile_num_col, ae_tile_num_row;
	int af_tile_num_col, af_tile_num_row;

	p_aaa = (struct cfa_aaa_stat *)p_cfa;
	awb_tile_num_col = p_aaa->aaa_tile_info.awb_tile_num_col;
	awb_tile_num_row = p_aaa->aaa_tile_info.awb_tile_num_row;
	ae_tile_num_col = p_aaa->aaa_tile_info.ae_tile_num_col;
	ae_tile_num_row = p_aaa->aaa_tile_info.ae_tile_num_row;
	af_tile_num_col = p_aaa->aaa_tile_info.af_tile_num_col;
	af_tile_num_row = p_aaa->aaa_tile_info.af_tile_num_row;

	p_ae = (struct cfa_ae_stat *)(p_aaa->awb_stat + awb_tile_num_col*awb_tile_num_row);
	p_af = (struct af_stat *)(p_ae + ae_tile_num_col*ae_tile_num_row);
	printk("\nCFA statistics\n");
	printk("AWB %d %d %d %d %d\n", awb_tile_num_col, awb_tile_num_row,
		p_aaa->aaa_tile_info.awb_tile_width, p_aaa->aaa_tile_info.awb_tile_height, p_aaa->aaa_tile_info.awb_rgb_shift);
	printk("AE %d %d %d %d %d\n", ae_tile_num_col, ae_tile_num_row,
		p_aaa->aaa_tile_info.ae_tile_width, p_aaa->aaa_tile_info.ae_tile_height, p_aaa->aaa_tile_info.ae_linear_y_shift);
	printk("AF %d %d %d %d %d %d\n",af_tile_num_col, af_tile_num_row, p_aaa->aaa_tile_info.af_tile_active_width,
		p_aaa->aaa_tile_info.af_tile_active_height, p_aaa->aaa_tile_info.af_cfa_y_shift,p_aaa->aaa_tile_info.af_y_shift);
	printk("SLICE 0x%p %d %d %d %d %d %d %d %d\n", p_aaa, p_aaa->aaa_tile_info.total_slices_x, p_aaa->aaa_tile_info.total_slices_y,
		p_aaa->aaa_tile_info.slice_index_x, p_aaa->aaa_tile_info.slice_index_y,p_aaa->aaa_tile_info.slice_width, p_aaa->aaa_tile_info.slice_height,
		p_aaa->aaa_tile_info.slice_start_x, p_aaa->aaa_tile_info.slice_start_y);

	printk("%d slice %d %d %d %d\n", p_aaa->aaa_tile_info.total_slices_x, p_aaa->aaa_tile_info.ae_tile_num_col, p_aaa->aaa_tile_info.ae_tile_num_row,
		p_aaa->aaa_tile_info.awb_tile_num_row, p_aaa->aaa_tile_info.awb_tile_num_col);
	printk("awb %d %d %d ~ %d %d %d\n", p_aaa->awb_stat[0].sum_r, p_aaa->awb_stat[0].sum_g, p_aaa->awb_stat[0].sum_b,
		p_aaa->awb_stat[awb_tile_num_col*awb_tile_num_row-1].sum_r,
		p_aaa->awb_stat[awb_tile_num_col*awb_tile_num_row-1].sum_g,
		p_aaa->awb_stat[awb_tile_num_col*awb_tile_num_row-1].sum_b);
	printk("ae %d ~ %d\n", p_ae[0].lin_y,p_ae[ae_tile_num_col*ae_tile_num_row-1].lin_y);
	printk("af %d %d %d ~ %d %d %d\n\n",p_af[0].sum_fy, p_af[0].sum_fv1, p_af[0].sum_fv2,
		p_af[af_tile_num_col*af_tile_num_row-1].sum_fy,
		p_af[af_tile_num_col*af_tile_num_row-1].sum_fv1,
		p_af[af_tile_num_col*af_tile_num_row-1].sum_fv2);
	p_af = p_af + af_tile_num_col*af_tile_num_row;
	p_float_awb = (struct float_cfa_awb_stat*)((u8*)p_af + sizeof(struct cfa_histogram_stat));
	p_float_ae = (struct float_cfa_ae_stat*)(p_float_awb + float_tile_num);
	p_float_af = (struct float_cfa_af_stat*)(p_float_ae + float_tile_num);
	printk("float awb %d %d %d ~ %d %d %d\n", p_float_awb[0].sum_r, p_float_awb[0].sum_g,
		p_float_awb[0].sum_b, p_float_awb[float_tile_num-1].sum_r,p_float_awb[float_tile_num-1].sum_g,
		p_float_awb[float_tile_num-1].sum_b);
	printk("float ae %d %d %d ~ %d %d %d\n", p_float_ae[0].lin_y,p_float_ae[0].count_min,p_float_ae[0].count_max,
		p_float_ae[float_tile_num-1].lin_y, p_float_ae[float_tile_num-1].count_min, p_float_ae[float_tile_num-1].count_max);
	printk("float af %d %d ~ %d %d\n\n", p_float_af[0].sum_fv1, p_float_af[0].sum_fv2,
		p_float_af[float_tile_num-1].sum_fv1, p_float_af[float_tile_num-1].sum_fv2);
}

static void statis_RGB_print(void* p_rgb, const u8 slice)
{
	struct rgb_aaa_stat *p_aaa_rgb;
	u16 *p_ae_rgb;
	struct af_stat *p_af_rgb;
	u32* p_float_ae_rgb;
	struct float_rgb_af_stat* p_float_af_rgb;
	const u8 float_tile_num_rgb = 32;
	int awb_tile_num_col_rgb, awb_tile_num_row_rgb;
	int ae_tile_num_col_rgb, ae_tile_num_row_rgb;
	int af_tile_num_col_rgb, af_tile_num_row_rgb;

	p_aaa_rgb = (struct rgb_aaa_stat *)p_rgb;
	awb_tile_num_col_rgb = p_aaa_rgb->aaa_tile_info.awb_tile_num_col;
	awb_tile_num_row_rgb = p_aaa_rgb->aaa_tile_info.awb_tile_num_row;
	ae_tile_num_col_rgb = p_aaa_rgb->aaa_tile_info.ae_tile_num_col;
	ae_tile_num_row_rgb = p_aaa_rgb->aaa_tile_info.ae_tile_num_row;
	af_tile_num_col_rgb = p_aaa_rgb->aaa_tile_info.af_tile_num_col;
	af_tile_num_row_rgb = p_aaa_rgb->aaa_tile_info.af_tile_num_row;

	p_af_rgb = (struct af_stat *)(p_aaa_rgb->af_stat);
	p_ae_rgb = (u16 *)(p_af_rgb + af_tile_num_col_rgb*af_tile_num_row_rgb);
	printk("\nRGB statistics\n");
	printk("AWB %d %d %d %d %d\n", awb_tile_num_col_rgb, awb_tile_num_row_rgb,
		p_aaa_rgb->aaa_tile_info.awb_tile_width, p_aaa_rgb->aaa_tile_info.awb_tile_height, p_aaa_rgb->aaa_tile_info.awb_rgb_shift);
	printk("AE %d %d %d %d %d\n", ae_tile_num_col_rgb, ae_tile_num_row_rgb,
		p_aaa_rgb->aaa_tile_info.ae_tile_width, p_aaa_rgb->aaa_tile_info.ae_tile_height, p_aaa_rgb->aaa_tile_info.ae_linear_y_shift);
	printk("AF %d %d %d %d %d %d\n",af_tile_num_col_rgb, af_tile_num_row_rgb, p_aaa_rgb->aaa_tile_info.af_tile_active_width,
		p_aaa_rgb->aaa_tile_info.af_tile_active_height, p_aaa_rgb->aaa_tile_info.af_cfa_y_shift,p_aaa_rgb->aaa_tile_info.af_y_shift);
	printk("SLICE 0x%p %d %d %d %d %d %d %d %d\n", p_aaa_rgb, p_aaa_rgb->aaa_tile_info.total_slices_x, p_aaa_rgb->aaa_tile_info.total_slices_y,
		p_aaa_rgb->aaa_tile_info.slice_index_x, p_aaa_rgb->aaa_tile_info.slice_index_y,p_aaa_rgb->aaa_tile_info.slice_width, p_aaa_rgb->aaa_tile_info.slice_height,
		p_aaa_rgb->aaa_tile_info.slice_start_x, p_aaa_rgb->aaa_tile_info.slice_start_y);

	printk("%d slice %d %d %d %d\n", p_aaa_rgb->aaa_tile_info.total_slices_x, p_aaa_rgb->aaa_tile_info.ae_tile_num_col, p_aaa_rgb->aaa_tile_info.ae_tile_num_row,
		p_aaa_rgb->aaa_tile_info.awb_tile_num_row, p_aaa_rgb->aaa_tile_info.awb_tile_num_col);
	printk("ae %d ~ %d\n", p_ae_rgb[0],p_ae_rgb[ae_tile_num_col_rgb*ae_tile_num_row_rgb-1]);
	printk("af %d %d %d ~ %d %d %d\n\n",p_af_rgb[0].sum_fy, p_af_rgb[0].sum_fv1, p_af_rgb[0].sum_fv2,
		p_af_rgb[af_tile_num_col_rgb*af_tile_num_row_rgb-1].sum_fy,
		p_af_rgb[af_tile_num_col_rgb*af_tile_num_row_rgb-1].sum_fv1,
		p_af_rgb[af_tile_num_col_rgb*af_tile_num_row_rgb-1].sum_fv2);
	p_ae_rgb = p_ae_rgb + ae_tile_num_col_rgb*ae_tile_num_row_rgb;
	p_float_af_rgb = (struct float_rgb_af_stat*)((u8*)p_ae_rgb + sizeof(struct rgb_histogram_stat));
	p_float_ae_rgb = (u32*)(p_float_af_rgb + float_tile_num_rgb);
	printk("float ae %d  ~ %d\n", p_float_ae_rgb[0], p_float_ae_rgb[float_tile_num_rgb -1]);
	printk("float af %d %d %d %d ~ %d %d %d %d\n\n", p_float_af_rgb[0].sum_fv1_h,
		p_float_af_rgb[0].sum_fv1_v, p_float_af_rgb[0].sum_fv2_h, p_float_af_rgb[0].sum_fv2_v,
		p_float_af_rgb[float_tile_num_rgb-1].sum_fv1_h, p_float_af_rgb[float_tile_num_rgb-1].sum_fv1_v,
		p_float_af_rgb[float_tile_num_rgb-1].sum_fv2_h, p_float_af_rgb[float_tile_num_rgb-1].sum_fv2_v);
}
#endif

static int img_statistics_ready(void)
{
	complete(&g_statis_comp);
	return 0;
}

static int img_set_hist_ptr(void *hist_fifo_ptr, u32 pitch_size)
{
	if(hist_fifo_ptr==NULL) {
		hist_aaa_ptr = NULL;
		return -1;
	}

	hist_pitch_size = pitch_size;
	//hist_aaa_ptr = (void *)DSP_TO_AMBVIRT(hist_fifo_ptr);
	hist_aaa_ptr = (void *)(DSP_TO_PHYS(hist_fifo_ptr) - (u32)rgb_aaa_phys + rgb_start_ptr);

	return 0;
}

static int img_set_rgb_ptr(void *rgb_fifo_ptr)
{
	if (rgb_fifo_ptr==NULL) {
		rgb_aaa_ptr = NULL;
		return -1;
	}

	//rgb_aaa_ptr = (void *)DSP_TO_AMBVIRT(rgb_fifo_ptr);
	rgb_aaa_ptr = (void *)(DSP_TO_PHYS(rgb_fifo_ptr) - (u32)rgb_aaa_phys + rgb_start_ptr);

	return 0;
}

static int img_set_cfa_ptr(void *cfa_fifo_ptr)
{
	if (cfa_fifo_ptr==NULL) {
		cfa_aaa_ptr = NULL;
		return -1;
	}

	//cfa_aaa_ptr = (void *)DSP_TO_AMBVIRT(cfa_fifo_ptr);
	cfa_aaa_ptr = (void *)(DSP_TO_PHYS(cfa_fifo_ptr) - (u32)cfa_aaa_phys + cfa_start_ptr);

	return 0;
}

#if 0
static int img_set_cfa_start_ptr(void * cfa_fifo_start_ptr)
{
	if(cfa_fifo_start_ptr == NULL){
		cfa_start_ptr = NULL;
		return -1;
	}
	cfa_start_ptr = (void*)DSP_TO_AMBVIRT(cfa_fifo_start_ptr);
	return 0;
}

static int img_set_cfa_end_ptr(void * cfa_fifo_end_ptr)
{
	if(cfa_fifo_end_ptr == NULL){
		cfa_end_ptr = NULL;
		return -1;
	}
	cfa_end_ptr = (void*)DSP_TO_AMBVIRT(cfa_fifo_end_ptr);
	return 0;
}

static int img_set_rgb_start_ptr(void * rgb_fifo_start_ptr)
{
	if(rgb_fifo_start_ptr == NULL){
		rgb_start_ptr = NULL;
		return -1;
	}
	rgb_start_ptr = (void*)DSP_TO_AMBVIRT(rgb_fifo_start_ptr);
	return 0;
}

static int img_set_rgb_end_ptr(void * rgb_fifo_end_ptr)
{
	if(rgb_fifo_end_ptr == NULL){
		rgb_end_ptr = NULL;
		return -1;
	}
	rgb_end_ptr = (void*)DSP_TO_AMBVIRT(rgb_fifo_end_ptr);
	return 0;
}
#endif

static int check_cfa_aaa_stat(struct cfa_aaa_stat*  cfa_stat)
{
	if (cfa_stat == NULL) {
		iav_error("%s cfa_stat is null \n", __func__);
		return -1;
	}

    //more data integrity checks to CFA AAA STAT can be added here
    return 0;
}

static inline int wait_img_msg_count(struct mutex *mutex, int count)
{
	int i;

	INIT_COMPLETION(g_statis_comp);
	for (i = 0; i < count; i++) {
		mutex_unlock(mutex);
		if (wait_for_completion_interruptible(&g_statis_comp)) {
			mutex_lock(mutex);
			return -EINTR;
		}

		mutex_lock(mutex);
	}

	return 0;
}

static inline void img_remap_dsp(void)
{
	u32 phys_addr_page_aligned = 0;
	u32 offset = 0, size_page_aligned = 0;

	if (need_remap) {
		if (cfa_aaa_phys) {
			if (cfa_start_ptr) {
				iounmap((void *)((u32)cfa_start_ptr & PAGE_MASK));
			}
			offset = cfa_aaa_phys & ~PAGE_MASK;
			phys_addr_page_aligned = cfa_aaa_phys & PAGE_MASK;
			size_page_aligned = round_up(offset + cfa_aaa_size, PAGE_SIZE);
			cfa_start_ptr = ioremap_nocache(phys_addr_page_aligned,
				size_page_aligned) + offset;
			cfa_end_ptr = cfa_start_ptr + cfa_aaa_size;
		}

		if (rgb_aaa_phys) {
			if (rgb_start_ptr) {
				iounmap((void *)((u32)rgb_start_ptr & PAGE_MASK));
			}
			offset = rgb_aaa_phys & ~PAGE_MASK;
			phys_addr_page_aligned = rgb_aaa_phys & PAGE_MASK;
			size_page_aligned = round_up(offset + rgb_aaa_size, PAGE_SIZE);
			rgb_start_ptr = ioremap_nocache(phys_addr_page_aligned,
				size_page_aligned) + offset;
			rgb_end_ptr = rgb_start_ptr + rgb_aaa_size;
		}
		need_remap = 0;
	}
}

static int img_get_statistics(iav_context_t *context,
	struct img_statistics __user *arg)
{
	struct iav_global_info	*g_info = context->g_info;

	struct img_statistics mw_cmd;
	struct cfa_aaa_stat *pCfa = NULL;
	struct rgb_aaa_stat *pRgb = NULL;
	int single_pitch = 0;
	int stat_blk_num = 0;
	int cnt = 0;
	int row = 0, col = 0;
	const int total_hist_row = 4;
	u8* p_block = NULL;
	u8* p_usr = NULL;

	if (copy_from_user(&mw_cmd, arg, sizeof(mw_cmd)))
		return -EFAULT;

	if(mw_cmd.rgb_statis == NULL ||mw_cmd.rgb_data_valid == NULL ||
		mw_cmd.cfa_statis ==NULL || mw_cmd.cfa_data_valid == NULL){
		return -EFAULT;
	}

	img_remap_dsp();

	if (wait_img_msg_count(context->mutex, 1) < 0) {
		iav_error("Failed to wait for completion!\n");
		return -EINTR;
	}

	if (need_remap) {
		return -EAGAIN;
	}

	put_user(0, mw_cmd.rgb_data_valid);
	if (rgb_aaa_ptr != NULL) {
		//invalidate_d_cache(rgb_aaa_ptr, RGB_AAA_DATA_BLOCK);
		pRgb = (struct rgb_aaa_stat*)rgb_aaa_ptr;

		/* read out the rest RGB parameters */
		if (g_info->high_mega_pixel_enable) {
			stat_blk_num = pRgb->aaa_tile_info.total_slices_x;
		} else if (g_info->hdr_mode) {
			if(pRgb->aaa_tile_info.exposure_index == pRgb->aaa_tile_info.total_exposure){
				stat_blk_num = pRgb->aaa_tile_info.total_exposure + 1;
			}else{
				stat_blk_num = pRgb->aaa_tile_info.total_exposure;
			}
		} else if (g_info->vin_num > 1) {
			// currently each vin have one slice
			stat_blk_num = pRgb->aaa_tile_info.total_channel_num;
		} else {
			stat_blk_num = 1;
		}

		for (cnt = stat_blk_num; cnt > 0; --cnt) {
			p_block = (u8*)rgb_aaa_ptr - RGB_AAA_DATA_BLOCK*cnt;
			if ((u32)p_block < (u32)rgb_start_ptr) {
				p_block = p_block + ((u32)rgb_end_ptr - (u32)rgb_start_ptr);
			}
			//invalidate_d_cache(p_block, RGB_AAA_DATA_BLOCK);
			if (copy_to_user(mw_cmd.rgb_statis + (stat_blk_num - cnt) * RGB_AAA_DATA_BLOCK,
					p_block, RGB_AAA_DATA_BLOCK))
				return -EFAULT;
		}
		put_user(stat_blk_num, mw_cmd.rgb_data_valid);

#if PRINT_IMGPROC
		statis_RGB_print(rgb_aaa_ptr, 0);
#endif
	}

	put_user(0, mw_cmd.cfa_data_valid);
	if (cfa_aaa_ptr != NULL) {
		//invalidate_d_cache(cfa_aaa_ptr, CFA_AAA_DATA_BLOCK);
		pCfa = (struct cfa_aaa_stat*)cfa_aaa_ptr;

		//do some data check before copy CFA AAA stat to user space
		if (check_cfa_aaa_stat(pCfa) < 0) {
			printk(KERN_DEBUG "CFA AAA stat check error \n");
			return -EFAULT;
		}

		/* read out the rest CFA parameters */
		if (g_info->high_mega_pixel_enable) {
			stat_blk_num = pCfa->aaa_tile_info.total_slices_x;
		} else if (g_info->hdr_mode) {
			stat_blk_num = pCfa->aaa_tile_info.total_exposure;
		} else if (g_info->vin_num > 1) {
			// currently each vin have one slice
			stat_blk_num = pCfa->aaa_tile_info.total_channel_num;
		} else {
			stat_blk_num = 1;
		}

		for (cnt = stat_blk_num; cnt > 0; --cnt) {
			p_block = (u8*)cfa_aaa_ptr - CFA_AAA_DATA_BLOCK * cnt;
			if ((u32)p_block < (u32)cfa_start_ptr){
				p_block = p_block + ((u32)cfa_end_ptr - (u32)cfa_start_ptr);
			}
			//invalidate_d_cache(p_block, CFA_AAA_DATA_BLOCK);
			if(copy_to_user(mw_cmd.cfa_statis + (stat_blk_num - cnt) * CFA_AAA_DATA_BLOCK,
					p_block, CFA_AAA_DATA_BLOCK))
				return -EFAULT;
		}
		put_user(stat_blk_num, mw_cmd.cfa_data_valid);

#if PRINT_IMGPROC
		statis_CFA_print(cfa_aaa_ptr, 0);
#endif
	}

	if(mw_cmd.hist_statis != NULL && mw_cmd.hist_data_valid != NULL){
		put_user(0, mw_cmd.hist_data_valid);
		if ((hist_aaa_ptr != NULL) &&
			(context->g_info->pvininfo->capability.sensor_id == 0x2A)){
			stat_blk_num = g_info->vin_num;

			if (stat_blk_num == 1) {
				//invalidate_d_cache(hist_aaa_ptr, hist_pitch_size*total_hist_row);
				put_user(hist_pitch_size, (u32 *)mw_cmd.hist_statis);

				if (copy_to_user(mw_cmd.hist_statis + sizeof(hist_pitch_size),
					hist_aaa_ptr, total_hist_row * hist_pitch_size)) {
					iav_error("Failed to copy data to user of hist data 2!\n");
					return -EFAULT;
				}
			} else if (stat_blk_num > 1) {
				//invalidate_d_cache(hist_aaa_ptr,
				//	g_info->pvininfo->capability.cap_cap_h * hist_pitch_size);
				single_pitch = hist_pitch_size/stat_blk_num;

				for (col = 0; col < stat_blk_num; ++col) {
					//the top 2 lines
					for (row = 0; row < 2; ++row) {
						p_block = (u8 *)hist_aaa_ptr + row * hist_pitch_size
							+ col * single_pitch;
						p_usr = (u8 *)mw_cmd.hist_statis +
							col * SENSOR_HIST_DATA_BLOCK + row * single_pitch;
						if (row == 0) {
							put_user(single_pitch, (int *)p_usr);
						}

						if (copy_to_user(p_usr + sizeof(single_pitch), p_block,
							single_pitch)) {
							iav_error("Failed to copy data to user of hist data 2!\n");
							return -EFAULT;
						}
					}
					//the bottom 2 lines
					for(row = 0; row < 2; ++row){
						p_block = (u8 *)hist_aaa_ptr + hist_pitch_size *
							(g_info->pvininfo->capability.cap_cap_h - 2 + row) +
							col * single_pitch;
						p_usr = (u8 *)mw_cmd.hist_statis +
							col * SENSOR_HIST_DATA_BLOCK + (row + 2) * single_pitch;

						if (copy_to_user(p_usr + sizeof(single_pitch), p_block,
							single_pitch)) {
							iav_error("Failed to copy data to user of hist data 2!\n");
							return -EFAULT;
						}
					}
				}
			}
			put_user(stat_blk_num, mw_cmd.hist_data_valid);
		}
	}

	return 0;
}

static void save_hdr_idsp_cmd(void * cmd, u32 cmd_size)
{
	u8 * addr = (u8 *)hdr_proc_buf +
		g_curr_hdr_buf_idx * HDR_PROC_BUFFRE_SIZE +
		g_curr_expo_idx * HDR_IDSP_EXPO_BUF_SIZE +
		g_hdr_expo[g_curr_expo_idx].cmd_num * HDR_IDSP_CMD_SIZE;
	memcpy(addr, cmd, cmd_size);
	memset(addr + cmd_size, 0, HDR_IDSP_CMD_SIZE - cmd_size);
	g_hdr_expo[g_curr_expo_idx].cmd_num++;
	img_printk("Copy hdr idsp cmd [0x%x] from addr [0x%x], size [%d] to addr [0x%x].\n",
		*(u32 *)cmd, (u32)cmd, cmd_size, (u32)addr);
}

static void save_hdr_alpha_cmd(void * cmd, u32 cmd_size)
{
	u8 * addr = (u8 *)hdr_alpha_map_buf +
		g_curr_hdr_buf_idx * HDR_ALPHA_MAP_SIZE +
		g_curr_expo_idx * HDR_IDSP_CMD_SIZE;
	memcpy(addr, cmd, cmd_size);
	memset(addr + cmd_size, 0, HDR_IDSP_CMD_SIZE - cmd_size);
	img_printk("Copy alpha map cmd [0x%x] from addr [0x%x], size [%d] to addr [0x%x].\n",
		*(u32 *)cmd, (u32)cmd, cmd_size, (u32)addr);
}

static void save_hdr_yuv_cntl_cmd(void *cmd, u32 cmd_size)
{
	u8 *addr = (u8*)hdr_yuv_cntl_buf + g_curr_hdr_buf_idx * HDR_YUV_CNTL_BUFF_SIZE;
	memcpy(addr, cmd, cmd_size);
	memset(addr + cmd_size, 0, HDR_IDSP_CMD_SIZE - cmd_size);
	img_printk("Copy yuv cntl cmd [0x%x] from addr [0x%x], size [%d] to addr [0x%x].\n",
		*(u32 *)cmd, (u32)cmd, cmd_size, (u32)addr);
}

static inline int is_in_3a_work_state(iav_context_t * context)
{
	struct iav_global_info * g_info = context->g_info;
	return ((g_info->state == IAV_STATE_PREVIEW) ||
		(g_info->state == IAV_STATE_ENCODING));
}

static int dsp_aaa_floating_statistics_setup (iav_context_t *context,
	struct aaa_floating_tile_config_info __user * param)
{
	aaa_floating_tile_config_t dsp_cmd;
	struct aaa_floating_tile_config_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = AAA_FLOATING_TILE_CONFIG;
	dsp_cmd.frame_sync_id = mw_cmd.frame_sync_id;
	dsp_cmd.number_of_tiles = mw_cmd.number_of_tiles;
	dsp_cmd.floating_tile_config_addr = VIRT_TO_DSP(float_tile_config_addr);

	if (copy_from_user(float_tile_config_addr,
		(void*)mw_cmd.floating_tile_config_addr, FLOAT_TILE_CONFIG_SIZE))
		return -EFAULT;

	clean_cache_aligned(float_tile_config_addr, FLOAT_TILE_CONFIG_SIZE);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_aaa_statistics_setup (iav_context_t *context,
	struct aaa_statistics_config __user * param)
{
	aaa_statistics_setup_t	dsp_cmd;
	struct aaa_statistics_config	mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
	return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = AAA_STATISTICS_SETUP;
	dsp_cmd.on = mw_cmd.enable;
	dsp_cmd.auto_shift = mw_cmd.auto_shift;

	dsp_cmd.data_fifo_base = 0;
	dsp_cmd.data_fifo_limit = 0;

	dsp_cmd.data_fifo2_base = 0;
	dsp_cmd.data_fifo2_limit = 0;

	dsp_cmd.awb_tile_num_col = mw_cmd.awb_tile_num_col;
	dsp_cmd.awb_tile_num_row = mw_cmd.awb_tile_num_row;
	dsp_cmd.awb_tile_col_start = mw_cmd.awb_tile_col_start;
	dsp_cmd.awb_tile_row_start = mw_cmd.awb_tile_row_start;
	dsp_cmd.awb_tile_width = mw_cmd.awb_tile_width;
	dsp_cmd.awb_tile_height = mw_cmd.awb_tile_height;
	dsp_cmd.awb_tile_active_width = mw_cmd.awb_tile_active_width;
	dsp_cmd.awb_tile_active_height = mw_cmd.awb_tile_active_height;
	dsp_cmd.awb_pix_min_value = mw_cmd.awb_pix_min_value;
	dsp_cmd.awb_pix_max_value = mw_cmd.awb_pix_max_value;

	dsp_cmd.ae_tile_num_col = mw_cmd.ae_tile_num_col;
	dsp_cmd.ae_tile_num_row = mw_cmd.ae_tile_num_row;
	dsp_cmd.ae_tile_col_start = mw_cmd.ae_tile_col_start;
	dsp_cmd.ae_tile_row_start = mw_cmd.ae_tile_row_start;
	dsp_cmd.ae_tile_width = mw_cmd.ae_tile_width;
	dsp_cmd.ae_tile_height = mw_cmd.ae_tile_height;

	dsp_cmd.af_tile_num_col = mw_cmd.af_tile_num_col;
	dsp_cmd.af_tile_num_row = mw_cmd.af_tile_num_row;
	dsp_cmd.af_tile_col_start = mw_cmd.af_tile_col_start;
	dsp_cmd.af_tile_row_start = mw_cmd.af_tile_row_start;
	dsp_cmd.af_tile_width = mw_cmd.af_tile_width;
	dsp_cmd.af_tile_height = mw_cmd.af_tile_height;
	dsp_cmd.af_tile_active_width = mw_cmd.af_tile_active_width;
	dsp_cmd.af_tile_active_height = mw_cmd.af_tile_active_height;
	dsp_cmd.ae_pix_min_value = mw_cmd.ae_pix_min_value;
	dsp_cmd.ae_pix_max_value = mw_cmd.ae_pix_max_value;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_aaa_histogram_setup (iav_context_t *context,
	struct aaa_histogram_config __user * param)
{
	aaa_histogram_t dsp_cmd;
	struct aaa_histogram_config mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
	return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = AAA_HISTORGRAM_SETUP;
	dsp_cmd.mode = mw_cmd.mode;
	dsp_cmd.histogram_select = mw_cmd.hist_select;
	dsp_cmd.ae_file_mask[0] = mw_cmd.tile_mask[0];
	dsp_cmd.ae_file_mask[1] = mw_cmd.tile_mask[1];
	dsp_cmd.ae_file_mask[2] = mw_cmd.tile_mask[2];
	dsp_cmd.ae_file_mask[3] = mw_cmd.tile_mask[3];
	dsp_cmd.ae_file_mask[4] = mw_cmd.tile_mask[4];
	dsp_cmd.ae_file_mask[5] = mw_cmd.tile_mask[5];
	dsp_cmd.ae_file_mask[6] = mw_cmd.tile_mask[6];
	dsp_cmd.ae_file_mask[7] = mw_cmd.tile_mask[7];

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

/**
 * AAA Statistic debug for A5
 */
static int dsp_aaa_statistics_ex(iav_context_t *context,
	struct aaa_statistics_ex *param)
{
	aaa_statistics_setup1_t dsp_cmd1;
	aaa_statistics_setup2_t dsp_cmd2;
	struct aaa_statistics_ex mw_cmd;
	u32 i;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd1, 0, sizeof(dsp_cmd1));
	memset(&dsp_cmd2, 0, sizeof(dsp_cmd2));

	dsp_cmd1.cmd_code = AAA_STATISTICS_SETUP1;
	dsp_cmd1.af_horizontal_filter1_mode = mw_cmd.af_horizontal_filter1_mode;
	dsp_cmd1.af_horizontal_filter1_stage1_enb = mw_cmd.af_horizontal_filter1_stage1_enb;
	dsp_cmd1.af_horizontal_filter1_stage2_enb = mw_cmd.af_horizontal_filter1_stage2_enb;
	dsp_cmd1.af_horizontal_filter1_stage3_enb = mw_cmd.af_horizontal_filter1_stage3_enb;

	for (i = 0; i < 7; i++){
		dsp_cmd1.af_horizontal_filter1_gain[i] = mw_cmd.af_horizontal_filter1_gain[i];
	}
	for (i = 0; i < 4; i++){
		dsp_cmd1.af_horizontal_filter1_shift[i] = mw_cmd.af_horizontal_filter1_shift[i];
	}

	dsp_cmd1.af_horizontal_filter1_bias_off = mw_cmd.af_horizontal_filter1_bias_off;
	dsp_cmd1.af_horizontal_filter1_thresh = mw_cmd.af_horizontal_filter1_thresh;
	dsp_cmd1.af_vertical_filter1_thresh = mw_cmd.af_vertical_filter1_thresh;
	dsp_cmd1.af_tile_fv1_horizontal_shift = mw_cmd.af_tile_fv1_horizontal_shift;
	dsp_cmd1.af_tile_fv1_vertical_shift = mw_cmd.af_tile_fv1_vertical_shift;
	dsp_cmd1.af_tile_fv1_horizontal_weight = mw_cmd.af_tile_fv1_horizontal_weight;
	dsp_cmd1.af_tile_fv1_vertical_weight = mw_cmd.af_tile_fv1_vertical_weight;

	dsp_issue_cmd(&dsp_cmd1, sizeof(dsp_cmd1));

	dsp_cmd2.cmd_code = AAA_STATISTICS_SETUP2;
	dsp_cmd2.af_horizontal_filter2_mode = mw_cmd.af_horizontal_filter2_mode;
	dsp_cmd2.af_horizontal_filter2_stage1_enb = mw_cmd.af_horizontal_filter2_stage1_enb;
	dsp_cmd2.af_horizontal_filter2_stage2_enb = mw_cmd.af_horizontal_filter2_stage2_enb;
	dsp_cmd2.af_horizontal_filter2_stage3_enb = mw_cmd.af_horizontal_filter2_stage3_enb;

	for (i = 0; i < 7; i ++){
		dsp_cmd2.af_horizontal_filter2_gain[i] = mw_cmd.af_horizontal_filter2_gain[i];
	}
	for (i = 0; i < 4; i++){
		dsp_cmd2.af_horizontal_filter2_shift[i] = mw_cmd.af_horizontal_filter2_shift[i];
	}


	dsp_cmd2.af_horizontal_filter2_bias_off = mw_cmd.af_horizontal_filter2_bias_off;
	dsp_cmd2.af_horizontal_filter2_thresh = mw_cmd.af_horizontal_filter2_thresh;
	dsp_cmd2.af_vertical_filter2_thresh = mw_cmd.af_vertical_filter2_thresh;
	dsp_cmd2.af_tile_fv2_horizontal_shift = mw_cmd.af_tile_fv2_horizontal_shift;
	dsp_cmd2.af_tile_fv2_vertical_shift = mw_cmd.af_tile_fv2_vertical_shift;
	dsp_cmd2.af_tile_fv2_horizontal_weight = mw_cmd.af_tile_fv2_horizontal_weight;
	dsp_cmd2.af_tile_fv2_vertical_weight = mw_cmd.af_tile_fv2_vertical_weight;

	dsp_issue_cmd(&dsp_cmd2, sizeof(dsp_cmd2));

	return 0;
}

static int dsp_mctf_mv_stabilizer_setup (iav_context_t *context,
	struct mctf_mv_stab_info __user *param)
{
	VCAP_MCTF_MV_STAB_CMD dsp_cmd;
	struct mctf_mv_stab_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_VCAP_MCTF_MV_STAB;
	dsp_cmd.noise_filter_strength = mw_cmd.noise_filter_strength;
	dsp_cmd.mctf_chan = mw_cmd.mctf_chan;
	dsp_cmd.sharpen_b_chan = mw_cmd.sharpen_b_chan;
	dsp_cmd.cc_en = mw_cmd.cc_en;
	dsp_cmd.cmpr_en = mw_cmd.cmpr_en;
	dsp_cmd.cmpr_dither = mw_cmd.cmpr_dither;
	dsp_cmd.mode = mw_cmd.mode;
	dsp_cmd.image_stabilize_strength = mw_cmd.image_stabilize_strength;
	dsp_cmd.bitrate_y = mw_cmd.bitrate_y;
	dsp_cmd.bitrate_uv = mw_cmd.bitrate_uv;
	dsp_cmd.use_zmv_as_predictor = mw_cmd.use_zmv_as_predictor;
	dsp_cmd.reserved = mw_cmd.reserved;
	dsp_cmd.mctf_cfg_dbase = VIRT_TO_DSP(mctf_cfg_addr);
	dsp_cmd.cc_cfg_dbase = VIRT_TO_DSP(cc_cfg_addr);
	dsp_cmd.cmpr_cfg_dbase = VIRT_TO_DSP(cmpr_cfg_addr);

	dsp_cmd.loadcfg_type.cmds.mctf_config_update = mw_cmd.loadcfg_type.cmds.mctf_config_update;
	dsp_cmd.loadcfg_type.cmds.mctf_hist_update = mw_cmd.loadcfg_type.cmds.mctf_hist_update;
	dsp_cmd.loadcfg_type.cmds.mctf_curves_update = mw_cmd.loadcfg_type.cmds.mctf_curves_update;
	dsp_cmd.loadcfg_type.cmds.mcts_config_update = mw_cmd.loadcfg_type.cmds.mcts_config_update;
	dsp_cmd.loadcfg_type.cmds.shpb_config_update = mw_cmd.loadcfg_type.cmds.shpb_config_update;
	dsp_cmd.loadcfg_type.cmds.shpc_config_update = mw_cmd.loadcfg_type.cmds.shpc_config_update;
	dsp_cmd.loadcfg_type.cmds.shpb_fir1_update = mw_cmd.loadcfg_type.cmds.shpb_fir1_update;
	dsp_cmd.loadcfg_type.cmds.shpb_fir2_update = mw_cmd.loadcfg_type.cmds.shpb_fir2_update;
	dsp_cmd.loadcfg_type.cmds.shpc_fir1_update = mw_cmd.loadcfg_type.cmds.shpc_fir1_update;
	dsp_cmd.loadcfg_type.cmds.shpc_fir2_update = mw_cmd.loadcfg_type.cmds.shpc_fir2_update;
	dsp_cmd.loadcfg_type.cmds.shpb_alphas_update = mw_cmd.loadcfg_type.cmds.shpb_alphas_update;
	dsp_cmd.loadcfg_type.cmds.shpc_alphas_update = mw_cmd.loadcfg_type.cmds.shpc_alphas_update;
	dsp_cmd.loadcfg_type.cmds.tone_hist_update = mw_cmd.loadcfg_type.cmds.tone_hist_update;
	dsp_cmd.loadcfg_type.cmds.shpb_coring1_update = mw_cmd.loadcfg_type.cmds.shpb_coring1_update;
	dsp_cmd.loadcfg_type.cmds.shpc_coring1_update = mw_cmd.loadcfg_type.cmds.shpc_coring1_update;
	dsp_cmd.loadcfg_type.cmds.shpb_coring2_update = mw_cmd.loadcfg_type.cmds.shpb_coring2_update;
	dsp_cmd.loadcfg_type.cmds.shpb_linear_update = mw_cmd.loadcfg_type.cmds.shpb_linear_update;
	dsp_cmd.loadcfg_type.cmds.shpc_linear_update = mw_cmd.loadcfg_type.cmds.shpc_linear_update;
	dsp_cmd.loadcfg_type.cmds.shpb_inv_linear_update = mw_cmd.loadcfg_type.cmds.shpb_inv_linear_update;
	dsp_cmd.loadcfg_type.cmds.shpc_inv_linear_update = mw_cmd.loadcfg_type.cmds.shpc_inv_linear_update;
	dsp_cmd.loadcfg_type.cmds.mctf_3d_level_update = mw_cmd.loadcfg_type.cmds.mctf_3d_level_update;
	dsp_cmd.loadcfg_type.cmds.shpb_3d_level_update = mw_cmd.loadcfg_type.cmds.shpb_3d_level_update;
	dsp_cmd.loadcfg_type.cmds.cc_config_update = mw_cmd.loadcfg_type.cmds.cc_config_update;
	dsp_cmd.loadcfg_type.cmds.cc_input_table_update = mw_cmd.loadcfg_type.cmds.cc_input_table_update;
	dsp_cmd.loadcfg_type.cmds.cc_output_table_update = mw_cmd.loadcfg_type.cmds.cc_output_table_update;
	dsp_cmd.loadcfg_type.cmds.cc_3d_table_update = mw_cmd.loadcfg_type.cmds.cc_3d_table_update;
	dsp_cmd.loadcfg_type.cmds.cc_matrix_update = mw_cmd.loadcfg_type.cmds.cc_matrix_update;
	dsp_cmd.loadcfg_type.cmds.cc_blend_input_update = mw_cmd.loadcfg_type.cmds.cc_blend_input_update;
	dsp_cmd.loadcfg_type.cmds.cmpr_all_update = mw_cmd.loadcfg_type.cmds.cmpr_all_update;
	dsp_cmd.loadcfg_type.cmds.mctf_mcts_all_update = mw_cmd.loadcfg_type.cmds.mctf_mcts_all_update;
	dsp_cmd.loadcfg_type.cmds.cc_all_update = mw_cmd.loadcfg_type.cmds.cc_all_update;
	dsp_cmd.loadcfg_type.cmds.reserved = mw_cmd.loadcfg_type.cmds.reserved;

	if (copy_from_user(mctf_cfg_addr,
		(void*)mw_cmd.mctf_cfg_dram_addr, MCTF_CFG_SIZE))
		return -EFAULT;
	if (copy_from_user(cc_cfg_addr,
		(void*)mw_cmd.mctf_cc_cfg_dram_addr, SEC_CC_SIZE))
		return -EFAULT;
	if (copy_from_user(cmpr_cfg_addr,
		(void*)mw_cmd.mctf_compr_cfg_dram_addr, 544))
		return -EFAULT;

	clean_cache_aligned(mctf_cfg_addr, MCTF_CFG_SIZE);
	clean_cache_aligned(cc_cfg_addr, SEC_CC_SIZE);
	clean_cache_aligned(cmpr_cfg_addr, 544);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_mctf_gmv_setup (iav_context_t *context,
	struct mctf_gmv_info __user *param)
{
	VCAP_MCTF_GMV_CMD	dsp_cmd;
	struct mctf_gmv_info	mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;


	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_VCAP_MCTF_GMV;
	dsp_cmd.channel_id = mw_cmd.channel_id;
	dsp_cmd.stream_type = mw_cmd.stream_type;
	dsp_cmd.reserved_0 = mw_cmd.reserved_0;
	dsp_cmd.enable = mw_cmd.enable_external_gmv;
	dsp_cmd.reserved_1 = mw_cmd.reserved_1;
	dsp_cmd.gmv = mw_cmd.external_gmv;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_noise_filter_setup (iav_context_t *context, u32 strength)
{
	noise_filter_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
 	dsp_cmd.cmd_code = NOISE_FILTER_SETUP;

	dsp_cmd.strength = strength;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_black_level_global_offset(iav_context_t *context,
	struct black_level_global_offset __user *param)
{
	black_level_global_offset_t	dsp_cmd;
	struct black_level_global_offset mw_cmd;
	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = BLACK_LEVEL_GLOBAL_OFFSET;
	dsp_cmd.global_offset_ee = mw_cmd.global_offset_ee;
	dsp_cmd.global_offset_eo = mw_cmd.global_offset_eo;
	dsp_cmd.global_offset_oe = mw_cmd.global_offset_oe;
	dsp_cmd.global_offset_oo = mw_cmd.global_offset_oo;
	dsp_cmd.black_level_offset_red = mw_cmd.black_level_offset_red;
	dsp_cmd.black_level_offset_green = mw_cmd.black_level_offset_green;
	dsp_cmd.black_level_offset_blue = mw_cmd.black_level_offset_blue;
	dsp_cmd.gain_depedent_offset_red = mw_cmd.gain_depedent_offset_red;
	dsp_cmd.gain_depedent_offset_green = mw_cmd.gain_depedent_offset_green;
	dsp_cmd.gain_depedent_offset_blue = mw_cmd.gain_depedent_offset_blue;

	if (!g_info->hdr_mode) {
		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_cfa_domain_leakage_filter_setup(iav_context_t *context,
	struct cfa_leakage_filter_info __user *param)
{
	cfa_domain_leakage_filter_t dsp_cmd;
	struct cfa_leakage_filter_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CFA_DOMAIN_LEAKAGE_FILTER;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.alpha_rr = mw_cmd.alpha_rr;
	dsp_cmd.alpha_rb = mw_cmd.alpha_rb;
	dsp_cmd.alpha_br = mw_cmd.alpha_br;
	dsp_cmd.alpha_bb = mw_cmd.alpha_bb;
	dsp_cmd.saturation_level = mw_cmd.saturation_level;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;

}

static int dsp_hdr_cfa_leakage_filter_setup(iav_context_t *context,
	struct cfa_leakage_filter_info __user *param)
{
	cfa_domain_leakage_filter_t dsp_cmd;
	struct cfa_leakage_filter_info mw_cmd;
	struct iav_global_info	*g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(g_info->hdr_mode){
		dsp_cmd.cmd_code = CFA_DOMAIN_LEAKAGE_FILTER;
		dsp_cmd.enable = mw_cmd.enable;
		dsp_cmd.alpha_rr = mw_cmd.alpha_rr;
		dsp_cmd.alpha_rb = mw_cmd.alpha_rb;
		dsp_cmd.alpha_br = mw_cmd.alpha_br;
		dsp_cmd.alpha_bb = mw_cmd.alpha_bb;
		dsp_cmd.saturation_level = mw_cmd.saturation_level;

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_cfa_noise_filter(iav_context_t *context,
	struct cfa_noise_filter_info __user *param)
{
	cfa_noise_filter_t dsp_cmd;
	struct cfa_noise_filter_info mw_cmd;
	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CFA_NOISE_FILTER;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.mode = mw_cmd.mode;
	dsp_cmd.shift_coarse_ring1 = mw_cmd.shift_coarse_ring1;
	dsp_cmd.shift_coarse_ring2 = mw_cmd.shift_coarse_ring2;
	dsp_cmd.shift_fine_ring1 = mw_cmd.shift_fine_ring1;
	dsp_cmd.shift_fine_ring2 = mw_cmd.shift_fine_ring2;
	dsp_cmd.shift_center_red = mw_cmd.shift_center_red;
	dsp_cmd.shift_center_green = mw_cmd.shift_center_green;
	dsp_cmd.shift_center_blue = mw_cmd.shift_center_blue;
	dsp_cmd.target_coarse_red = mw_cmd.target_coarse_red;
	dsp_cmd.target_coarse_green = mw_cmd.target_coarse_green;
	dsp_cmd.target_coarse_blue = mw_cmd.target_coarse_blue;
	dsp_cmd.target_fine_red = mw_cmd.target_fine_red;
	dsp_cmd.target_fine_green = mw_cmd.target_fine_green;
	dsp_cmd.target_fine_blue = mw_cmd.target_fine_blue;
	dsp_cmd.cutoff_red = mw_cmd.cutoff_red;
	dsp_cmd.cutoff_green = mw_cmd.cutoff_green;
	dsp_cmd.cutoff_blue = mw_cmd.cutoff_blue;
	dsp_cmd.thresh_coarse_red = mw_cmd.thresh_coarse_red;
	dsp_cmd.thresh_coarse_green = mw_cmd.thresh_coarse_green;
	dsp_cmd.thresh_coarse_blue = mw_cmd.thresh_coarse_blue;
	dsp_cmd.thresh_fine_red = mw_cmd.thresh_fine_red;
	dsp_cmd.thresh_fine_green = mw_cmd.thresh_fine_green;
	dsp_cmd.thresh_fine_blue = mw_cmd.thresh_fine_blue;

	if (!g_info->hdr_mode) {
		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_vignette_compensation(iav_context_t *context,
	struct vignette_compensation_info __user *param)
{
	vignette_compensation_t dsp_cmd;
	struct vignette_compensation_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = VIGNETTE_COMPENSATION;
 	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.gain_shift = mw_cmd.gain_shift;

	if (copy_from_user(vignette_r_gain,
		(void*)mw_cmd.vignette_red_gain_ptr, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if (copy_from_user(vignette_ge_gain,
		(void*)mw_cmd.vignette_green_even_gain_ptr, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if (copy_from_user(vignette_go_gain,
		(void*)mw_cmd.vignette_green_odd_gain_ptr, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if (copy_from_user(vignette_b_gain,
		(void*)mw_cmd.vignette_blue_gain_ptr, MAX_VIGNETTE_NUM))
		return -EFAULT;

	dsp_cmd.tile_gain_addr = VIRT_TO_DSP(vignette_r_gain);
	dsp_cmd.tile_gain_addr_green_even = VIRT_TO_DSP(vignette_ge_gain);
	dsp_cmd.tile_gain_addr_green_odd = VIRT_TO_DSP(vignette_go_gain);
	dsp_cmd.tile_gain_addr_blue = VIRT_TO_DSP(vignette_b_gain);

	clean_cache_aligned(vignette_r_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_ge_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_go_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_b_gain, MAX_VIGNETTE_NUM * sizeof(u8));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_local_exposure(iav_context_t *context,
	struct local_exposure_info __user *param)
{
	printk("Obsolete. Please use IAV_IOC_SET_LOCAL_EXPOSURE_EX.\n");
	return -EIO;
}

static int dsp_color_correction(iav_context_t *context,
	struct color_correction_info __user *param)
{
	color_correction_t dsp_cmd;
	struct color_correction_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = COLOR_CORRECTION;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.no_interpolation = mw_cmd.no_interpolation;
	dsp_cmd.yuv422_foramt = mw_cmd.yuv422_foramt;
	dsp_cmd.uv_center = mw_cmd.uv_center;
	dsp_cmd.multi_red = mw_cmd.multi_red;
	dsp_cmd.multi_green = mw_cmd.multi_green;
	dsp_cmd.multi_blue = mw_cmd.multi_blue;
	dsp_cmd.in_lookup_table_addr = VIRT_TO_DSP(input_lookup_table);
	dsp_cmd.matrix_addr = VIRT_TO_DSP(matrix_dram_address);
	dsp_cmd.output_lookup_bypass = mw_cmd.output_lookup_bypass;
	if (dsp_cmd.output_lookup_bypass == 1) {
		dsp_cmd.out_lookup_table_addr = 0;
	} else {
		dsp_cmd.out_lookup_table_addr = VIRT_TO_DSP(output_lookup_table);
		if (mw_cmd.out_lut_addr != NULL) {
			if (copy_from_user(output_lookup_table, (void*)mw_cmd.out_lut_addr,
				NUM_OUT_LOOKUP * sizeof(u32)))
				return -EFAULT;
		}
	}
	if(mw_cmd.in_lut_addr != NULL) {
		if (copy_from_user(input_lookup_table, (void*)mw_cmd.in_lut_addr,
			NUM_IN_LOOKUP * sizeof(u32)))
			return -EFAULT;
	}
	if(mw_cmd.matrix_addr!=NULL) {
		if (copy_from_user(matrix_dram_address, (void*)mw_cmd.matrix_addr,
			NUM_MATRIX * sizeof(u32)))
			return -EFAULT;
	}
	dsp_cmd.group_index = mw_cmd.group_index;

	clean_cache_aligned(input_lookup_table, NUM_IN_LOOKUP*sizeof(u32));
	clean_cache_aligned(matrix_dram_address, NUM_MATRIX*sizeof(u32));
	if(mw_cmd.out_lut_addr != NULL) {
		clean_cache_aligned(output_lookup_table,
			NUM_OUT_LOOKUP*sizeof(u32));
	}

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_hdr_color_correction(iav_context_t *context,
	struct color_correction_info __user *param)
{
	color_correction_t dsp_cmd;
	struct color_correction_info mw_cmd;
	struct iav_global_info *g_info = context->g_info;
	u8* addr;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = COLOR_CORRECTION;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.no_interpolation = mw_cmd.no_interpolation;
	dsp_cmd.yuv422_foramt = mw_cmd.yuv422_foramt;
	dsp_cmd.uv_center = mw_cmd.uv_center;
	dsp_cmd.multi_red = mw_cmd.multi_red;
	dsp_cmd.multi_green = mw_cmd.multi_green;
	dsp_cmd.multi_blue = mw_cmd.multi_blue;
	dsp_cmd.output_lookup_bypass = mw_cmd.output_lookup_bypass;

	if(g_info->hdr_mode){
		addr = (u8*)hdr_cc_input_lookup_table +
			g_curr_hdr_buf_idx * HDR_CC_INPUT_TABLE_SIZE +
			g_curr_expo_idx * INPUT_LOOK_UP_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.in_lut_addr, INPUT_LOOK_UP_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, INPUT_LOOK_UP_TABLE_SIZE);
		dsp_cmd.in_lookup_table_addr = VIRT_TO_DSP(addr);

		addr = (u8*)hdr_cc_matrix_dram_address +
			g_curr_hdr_buf_idx * HDR_CC_MATRIX_SIZE +
			g_curr_expo_idx * MATRIX_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.matrix_addr, MATRIX_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, MATRIX_TABLE_SIZE);
		dsp_cmd.matrix_addr = VIRT_TO_DSP(addr);

		addr = (u8*)hdr_cc_output_lookup_table +
			g_curr_hdr_buf_idx * HDR_CC_OUTPUT_TABLE_SIZE +
			g_curr_expo_idx * OUTPUT_LOOK_UP_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.out_lut_addr, OUTPUT_LOOK_UP_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, OUTPUT_LOOK_UP_TABLE_SIZE);
		dsp_cmd.out_lookup_table_addr = VIRT_TO_DSP(addr);

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_hdr_yuv_vcap_no_op(iav_context_t * context)
{
	VCAP_NO_OP_CMD dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_VCAP_NO_OP;
	dsp_cmd.channel_id = 0x0;
	dsp_cmd.stream_type = 0x0;

	save_hdr_yuv_cntl_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}
static int dsp_hdr_yuv_control_param(iav_context_t* context,
	struct video_hdr_yuv_cntl_param __user *param)
{
	int rval = -1;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&g_yuv_ce_info, param, sizeof(g_yuv_ce_info)))
		return -EFAULT;

	rval = dsp_hdr_yuv_vcap_no_op(context);
	return (rval);
}

static int dsp_hdr_alpha_map(iav_context_t *context,
	struct color_correction_info __user *param)
{
	color_correction_t dsp_cmd;
	struct color_correction_info mw_cmd;
	struct iav_global_info *g_info = context->g_info;
	u8* addr;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = COLOR_CORRECTION;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.no_interpolation = mw_cmd.no_interpolation;
	dsp_cmd.yuv422_foramt = mw_cmd.yuv422_foramt;
	dsp_cmd.uv_center = mw_cmd.uv_center;
	dsp_cmd.multi_red = mw_cmd.multi_red;
	dsp_cmd.multi_green = mw_cmd.multi_green;
	dsp_cmd.multi_blue = mw_cmd.multi_blue;
	dsp_cmd.output_lookup_bypass = mw_cmd.output_lookup_bypass;

	if(g_info->hdr_mode){
		addr = (u8*)hdr_alpha_map_input_lookup_table +
			g_curr_hdr_buf_idx * HDR_CC_INPUT_TABLE_SIZE +
			g_curr_expo_idx * INPUT_LOOK_UP_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.in_lut_addr, INPUT_LOOK_UP_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, INPUT_LOOK_UP_TABLE_SIZE);
		dsp_cmd.in_lookup_table_addr = VIRT_TO_DSP(addr);

		addr = (u8*)hdr_alpha_map_matrix_dram_address +
			g_curr_hdr_buf_idx * HDR_CC_MATRIX_SIZE +
			g_curr_expo_idx * MATRIX_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.matrix_addr, MATRIX_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, MATRIX_TABLE_SIZE);
		dsp_cmd.matrix_addr = VIRT_TO_DSP(addr);

		addr = (u8*)hdr_alpha_map_output_lookup_table +
			g_curr_hdr_buf_idx * HDR_CC_OUTPUT_TABLE_SIZE +
			g_curr_expo_idx * OUTPUT_LOOK_UP_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.out_lut_addr, OUTPUT_LOOK_UP_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, OUTPUT_LOOK_UP_TABLE_SIZE);
		dsp_cmd.out_lookup_table_addr = VIRT_TO_DSP(addr);

		save_hdr_alpha_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_rgb_to_yuv_setup(iav_context_t *context,
	struct rgb_to_yuv_info __user *param)
{
	rgb_to_yuv_setup_t dsp_cmd;
	struct rgb_to_yuv_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = RGB_TO_YUV_SETUP;

	memcpy(dsp_cmd.matrix_values, (void*)mw_cmd.matrix_values,
		RGB_TO_YUV_MATRIX_SIZE*sizeof(u16));

 	dsp_cmd.y_offset = mw_cmd.y_offset;
 	dsp_cmd.u_offset = mw_cmd.u_offset;
 	dsp_cmd.v_offset = mw_cmd.v_offset;
 	dsp_cmd.group_index = mw_cmd.group_index;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_chroma_scale(iav_context_t *context,
	struct chroma_scale_info __user *param)
{
	chroma_scale_t dsp_cmd;
	struct chroma_scale_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CHROMA_SCALE;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.make_legal = mw_cmd.make_legal;
	dsp_cmd.u_weight_0 = mw_cmd.u_weight_0;
	dsp_cmd.u_weight_1 = mw_cmd.u_weight_1;
	dsp_cmd.u_weight_2 = mw_cmd.u_weight_2;
	dsp_cmd.v_weight_0 = mw_cmd.v_weight_0;
	dsp_cmd.v_weight_1 = mw_cmd.v_weight_1;
	dsp_cmd.v_weight_2 = mw_cmd.v_weight_2;
	dsp_cmd.gain_curver_addr = VIRT_TO_DSP(chroma_gain_curve);

	if (copy_from_user(chroma_gain_curve, (void*)(mw_cmd.gain_curver_addr),
		NUM_CHROMA_GAIN_CURVE*sizeof(u16)))
		return -EFAULT;
	clean_cache_aligned(chroma_gain_curve,
		NUM_CHROMA_GAIN_CURVE*sizeof(u16));

	dsp_cmd.group_index = mw_cmd.group_index;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_noise_filter(iav_context_t *context,
	struct chroma_noise_filter_info __user *param)
{
	chroma_noise_filter_t dsp_cmd;
	struct chroma_noise_filter_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CHROMA_NOISE_FILTER;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.radius = mw_cmd.radius;
	dsp_cmd.mode = mw_cmd.mode;
	dsp_cmd.thresh_u = mw_cmd.thresh_u;
	dsp_cmd.thresh_v = mw_cmd.thresh_v;
	dsp_cmd.shift_center_u = mw_cmd.shift_center_u;
	dsp_cmd.shift_center_v = mw_cmd.shift_center_v;
	dsp_cmd.shift_ring1 = mw_cmd.shift_ring1;
	dsp_cmd.shift_ring2 = mw_cmd.shift_ring2;
	dsp_cmd.target_u = mw_cmd.target_u;
	dsp_cmd.target_v = mw_cmd.target_v;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_median_filter(iav_context_t *context,
	struct chroma_median_filter __user *param)
{
	chroma_median_filter_info_t dsp_cmd;
	struct chroma_median_filter mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CHROMA_MEDIAN_FILTER;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.k0123_table_addr = VIRT_TO_DSP(k0123_table);
	dsp_cmd.u_sat_t0 = mw_cmd.u_sat_t0;
	dsp_cmd.u_sat_t1 = mw_cmd.u_sat_t1;
	dsp_cmd.v_sat_t0 = mw_cmd.v_sat_t0;
	dsp_cmd.v_sat_t1 = mw_cmd.v_sat_t1;
	dsp_cmd.u_act_t0 = mw_cmd.u_act_t0;
	dsp_cmd.u_act_t1 = mw_cmd.u_act_t1;
	dsp_cmd.v_act_t0 = mw_cmd.v_act_t0;
	dsp_cmd.v_act_t1 = mw_cmd.v_act_t1;

	if (copy_from_user(k0123_table,
		(void*)mw_cmd.k0123_table_addr, K0123_ARRAY_SIZE*sizeof(u16)))
		return -EFAULT;
	clean_cache_aligned(k0123_table, K0123_ARRAY_SIZE*sizeof(u16));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpening(iav_context_t *context,
	struct luma_sharpening_info __user *param)
{
	luma_sharpening_t dsp_cmd;
	struct luma_sharpening_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = LUMA_SHARPENING;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.use_generated_low_pass = mw_cmd.use_generated_low_pass;
	dsp_cmd.input_B_enable = mw_cmd.input_B_enable;
	dsp_cmd.input_C_enable = mw_cmd.input_C_enable;
	dsp_cmd.FIRs_input_from_B_minus_C = mw_cmd.FIRs_input_from_B_minus_C;
	dsp_cmd.coring1_input_from_B_minus_C = mw_cmd.coring1_input_from_B_minus_C;
	dsp_cmd.abs = mw_cmd.abs;
	dsp_cmd.yuv = mw_cmd.yuv;
	dsp_cmd.clip_low = mw_cmd.clip_low;
	dsp_cmd.clip_high = mw_cmd.clip_high;
	dsp_cmd.max_change_down = mw_cmd.max_change_down;
	dsp_cmd.max_change_up = mw_cmd.max_change_up;
	dsp_cmd.max_change_down_center = mw_cmd.max_change_down_center;
	dsp_cmd.max_change_up_center = mw_cmd.max_change_up_center;
	// alpha control
	dsp_cmd.grad_thresh_0 = mw_cmd.grad_thresh_0;
	dsp_cmd.grad_thresh_1 = mw_cmd.grad_thresh_1;
	dsp_cmd.smooth_shift = mw_cmd.smooth_shift;
	dsp_cmd.edge_shift = mw_cmd.edge_shift;
	dsp_cmd.alpha_table_addr = VIRT_TO_DSP(luma_sharpening_alpha_table);
	// edge control
	dsp_cmd.wide_weight = mw_cmd.wide_weight;
	dsp_cmd.narrow_weight = mw_cmd.narrow_weight;
	dsp_cmd.edge_threshold_multiplier = mw_cmd.edge_threshold_multiplier;
	dsp_cmd.edge_thresh = mw_cmd.edge_thresh;

	if (copy_from_user(luma_sharpening_alpha_table,
		(void*)mw_cmd.alpha_table_addr, NUM_ALPHA_TABLE))
		return -EFAULT;
	clean_cache_aligned(luma_sharpening_alpha_table, NUM_ALPHA_TABLE);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharp_fir(iav_context_t *context,
	struct luma_sharp_fir_info __user *param)
{
	luma_sharpening_FIR_config_t dsp_cmd;
	struct luma_sharp_fir_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if (copy_from_user(coeff_fir1_addr, (void*)mw_cmd.coeff_fir1_addr, 256))
		return -EFAULT;
	if (copy_from_user(coeff_fir2_addr, (void*)mw_cmd.coeff_fir2_addr, 256))
		return -EFAULT;
	if (copy_from_user(coring_table, (void*)mw_cmd.coring_table_addr, 256))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_FIR_CONFIG;
	dsp_cmd.enable_FIR1 = mw_cmd.enable_fir1;
	dsp_cmd.enable_FIR2 = mw_cmd.enable_fir2;
	dsp_cmd.add_in_non_alpha1 = mw_cmd.add_in_non_alpha;
	dsp_cmd.add_in_alpha1 = mw_cmd.add_in_alpha1;
	dsp_cmd.add_in_alpha2 = mw_cmd.add_in_alpha2;
	dsp_cmd.fir1_clip_low = mw_cmd.fir1_clip_low;
	dsp_cmd.fir1_clip_high = mw_cmd.fir1_clip_high;
	dsp_cmd.fir2_clip_low = mw_cmd.fir2_clip_low;
	dsp_cmd.fir2_clip_high = mw_cmd.fir2_clip_high;
	dsp_cmd.coeff_FIR1_addr = VIRT_TO_DSP(coeff_fir1_addr);
	dsp_cmd.coeff_FIR2_addr =  VIRT_TO_DSP(coeff_fir2_addr);
	dsp_cmd.coring_table_addr = VIRT_TO_DSP(coring_table);
	dsp_cmd.group_index = mw_cmd.group_index;

	clean_cache_aligned((u8 *)coeff_fir1_addr, 256);
	clean_cache_aligned((u8 *)coeff_fir2_addr, 256);
	clean_cache_aligned((u8 *)coring_table, 256);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpen_blend_ctrl(iav_context_t *context,
	struct luma_sharp_blend_info __user *param)
{
	luma_sharpening_blend_control_t	dsp_cmd;
	struct luma_sharp_blend_info	mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = LUMA_SHARPENING_BLEND_CONTROL;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.edge_threshold_multiplier = mw_cmd.edge_threshold_multiplier;
	dsp_cmd.iso_threshold_multiplier = mw_cmd.iso_threshold_multiplier;
	dsp_cmd.edge_threshold0 = mw_cmd.edge_threshold0;
	dsp_cmd.edge_threshold1 = mw_cmd.edge_threshold1;
	dsp_cmd.dir_threshold0 = mw_cmd.dir_threshold0;
	dsp_cmd.dir_threshold1 = mw_cmd.dir_threshold1;
	dsp_cmd.iso_threshold0 = mw_cmd.iso_threshold0;
	dsp_cmd.iso_threshold1 = mw_cmd.iso_threshold1;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;

}

static int dsp_luma_sharpen_level_ctrl(iav_context_t *context,
	struct luma_sharp_level_info __user *param)
{
	luma_sharpening_level_control_t	dsp_cmd;
	struct luma_sharp_level_info	mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = LUMA_SHARPENING_LEVEL_CONTROL;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.select = mw_cmd.select;
	dsp_cmd.low = mw_cmd.low;
	dsp_cmd.low_0 = mw_cmd.low_0;
	dsp_cmd.low_delta = mw_cmd.low_delta;
	dsp_cmd.low_val = mw_cmd.low_val;

	dsp_cmd.high = mw_cmd.high;
	dsp_cmd.high_0 = mw_cmd.high_0;
	dsp_cmd.high_delta = mw_cmd.high_delta;
	dsp_cmd.high_val = mw_cmd.high_val;
	dsp_cmd.base_val = mw_cmd.base_val;

	dsp_cmd.area = mw_cmd.area;
	dsp_cmd.level_control_clip_low = mw_cmd.level_control_clip_low;
	dsp_cmd.level_control_clip_low2 = mw_cmd.level_control_clip_low2;
	dsp_cmd.level_control_clip_high = mw_cmd.level_control_clip_high;
	dsp_cmd.level_control_clip_high2 = mw_cmd.level_control_clip_high2;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_rgb_gain_adjustment(iav_context_t *context,
	struct rgb_gain_info __user *param)
{
	rgb_gain_adjust_t dsp_cmd;
	struct rgb_gain_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

 	dsp_cmd.cmd_code = RGB_GAIN_ADJUSTMENT;
 	dsp_cmd.r_gain = mw_cmd.r_gain;
 	dsp_cmd.g_even_gain = mw_cmd.g_even_gain;
 	dsp_cmd.g_odd_gain = mw_cmd.g_odd_gain;
 	dsp_cmd.b_gain = mw_cmd.b_gain;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_hdr_rgb_gain_adjustment(iav_context_t *context,
	struct rgb_gain_info __user *param)
{
	rgb_gain_adjust_t dsp_cmd;
	struct rgb_gain_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

 	dsp_cmd.cmd_code = RGB_GAIN_ADJUSTMENT;
 	dsp_cmd.r_gain = mw_cmd.r_gain;
 	dsp_cmd.g_even_gain = mw_cmd.g_even_gain;
 	dsp_cmd.g_odd_gain = mw_cmd.g_odd_gain;
 	dsp_cmd.b_gain = mw_cmd.b_gain;

	save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_anti_aliasing_filter(iav_context_t *context,
	struct anti_aliasing_info __user* param)
{
	anti_aliasing_filter_t dsp_cmd;
	struct anti_aliasing_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = ANTI_ALIASING;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.threshold = mw_cmd.threshold;
	dsp_cmd.shift = mw_cmd.shift;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_hdr_anti_aliasing_filter(iav_context_t *context,
	struct anti_aliasing_info __user* param)
{
	anti_aliasing_filter_t dsp_cmd;
	struct anti_aliasing_info mw_cmd;
	struct iav_global_info	*g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(g_info->hdr_mode){
		dsp_cmd.cmd_code = ANTI_ALIASING;
		dsp_cmd.enable = mw_cmd.enable;
		dsp_cmd.threshold = mw_cmd.threshold;
		dsp_cmd.shift = mw_cmd.shift;

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}
	return 0;
}

static int dsp_digital_gain_sat_level(iav_context_t *context,
	struct digital_gain_level __user* param)
{

	digital_gain_level_t dsp_cmd;
	struct digital_gain_level mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = DIGITAL_GAIN_SATURATION_LEVEL;
	dsp_cmd.level_red = mw_cmd.level_red;
	dsp_cmd.level_green_even = mw_cmd.level_green_even;
	dsp_cmd.level_green_odd = mw_cmd.level_green_odd;
	dsp_cmd.level_blue = mw_cmd.level_blue;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_bad_pixel_correct_setup (iav_context_t *context,
	struct bad_pixel_correct_info __user* param)
{
	bad_pixel_correct_setup_t dsp_cmd;
	struct bad_pixel_correct_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = BAD_PIXEL_CORRECT_SETUP;
	dsp_cmd.dynamic_bad_pixel_detection_mode = mw_cmd.dynamic_bad_pixel_detection_mode;
	dsp_cmd.dynamic_bad_pixel_correction_method = mw_cmd.dynamic_bad_pixel_correction_method;
	dsp_cmd.correction_mode = mw_cmd.correction_mode;
	dsp_cmd.hot_pixel_thresh_addr = VIRT_TO_DSP(hot_pixel_thd_table);
	dsp_cmd.dark_pixel_thresh_addr = VIRT_TO_DSP(dark_pixel_thd_table);
	dsp_cmd.hot_shift0_4 = mw_cmd.hot_shift0_4;
	dsp_cmd.hot_shift5 = mw_cmd.hot_shift5;
	dsp_cmd.dark_shift0_4 = mw_cmd.dark_shift0_4;
	dsp_cmd.dark_shift5 = mw_cmd.dark_shift5;

	if (copy_from_user(hot_pixel_thd_table,
		(void*)mw_cmd.hot_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
		return -EFAULT;
	if (copy_from_user(dark_pixel_thd_table,
		(void*)mw_cmd.dark_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
		return -EFAULT;
	clean_cache_aligned(hot_pixel_thd_table, DYN_BPC_THD_TABLE_SIZE);
	clean_cache_aligned(dark_pixel_thd_table, DYN_BPC_THD_TABLE_SIZE);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_hdr_bad_pixel_correct_setup (iav_context_t *context,
	struct bad_pixel_correct_info __user* param)
{
	bad_pixel_correct_setup_t dsp_cmd;
	struct bad_pixel_correct_info mw_cmd;
	struct iav_global_info *g_info = context->g_info;
	u8 *addr = NULL;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = BAD_PIXEL_CORRECT_SETUP;
	dsp_cmd.dynamic_bad_pixel_detection_mode = mw_cmd.dynamic_bad_pixel_detection_mode;
	dsp_cmd.dynamic_bad_pixel_correction_method = mw_cmd.dynamic_bad_pixel_correction_method;
	dsp_cmd.correction_mode = mw_cmd.correction_mode;
	dsp_cmd.hot_shift0_4 = mw_cmd.hot_shift0_4;
	dsp_cmd.hot_shift5 = mw_cmd.hot_shift5;
	dsp_cmd.dark_shift0_4 = mw_cmd.dark_shift0_4;
	dsp_cmd.dark_shift5 = mw_cmd.dark_shift5;

	if(g_info->hdr_mode){
		addr = (u8*)hdr_bpc_hot_thd_table +
			g_curr_hdr_buf_idx * HDR_BPC_TABLE_SIZE +
			g_curr_expo_idx * DYN_BPC_THD_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.hot_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, DYN_BPC_THD_TABLE_SIZE);
		dsp_cmd.hot_pixel_thresh_addr = VIRT_TO_DSP(addr);

		addr = (u8*)hdr_bpc_dark_thd_table +
			g_curr_hdr_buf_idx * HDR_BPC_TABLE_SIZE +
			g_curr_expo_idx * DYN_BPC_THD_TABLE_SIZE;
		if(copy_from_user(addr, (void*)mw_cmd.dark_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
			return -EFAULT;
		clean_cache_aligned(addr, DYN_BPC_THD_TABLE_SIZE);
		dsp_cmd.dark_pixel_thresh_addr = VIRT_TO_DSP(addr);

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_demoasic_filter(iav_context_t *context,
	struct demoasic_filter_info __user* param)
{
	demoasic_filter_t dsp_cmd;
	struct demoasic_filter_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = DEMOASIC_FILTER;
	dsp_cmd.group_index = mw_cmd.group_index;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.clamp_directional_candidates = mw_cmd.clamp_directional_candidates;
	dsp_cmd.activity_thresh = mw_cmd.activity_thresh;
	dsp_cmd.activity_difference_thresh = mw_cmd.activity_difference_thresh;
	dsp_cmd.grad_clip_thresh = mw_cmd.grad_clip_thresh;
	dsp_cmd.grad_noise_thresh = mw_cmd.grad_noise_thresh;
	dsp_cmd.grad_noise_difference_thresh = mw_cmd.grad_noise_difference_thresh;
	dsp_cmd.zipper_noise_difference_add_thresh = mw_cmd.zipper_noise_difference_add_thresh;
	dsp_cmd.zipper_noise_difference_mult_thresh = mw_cmd.zipper_noise_difference_mult_thresh;
	dsp_cmd.max_const_hue_factor = mw_cmd.max_const_hue_factor;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_sensor_input_setup(iav_context_t *context,
	struct sensor_info *param)
{
	sensor_input_setup_t dsp_cmd;
	struct sensor_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

 	dsp_cmd.cmd_code = SENSOR_INPUT_SETUP;
 	dsp_cmd.sensor_id = mw_cmd.sensor_id;
 	dsp_cmd.field_format = mw_cmd.field_format;
 	dsp_cmd.sensor_resolution = mw_cmd.sensor_resolution;
 	dsp_cmd.sensor_pattern = mw_cmd.sensor_pattern;
	dsp_cmd.first_line_field_0 = mw_cmd.first_line_field_0;
	dsp_cmd.first_line_field_1 = mw_cmd.first_line_field_1;
	dsp_cmd.first_line_field_2 = mw_cmd.first_line_field_2;
	dsp_cmd.first_line_field_3 = mw_cmd.first_line_field_3;
	dsp_cmd.first_line_field_4 = mw_cmd.first_line_field_4;
	dsp_cmd.first_line_field_5 = mw_cmd.first_line_field_5;
	dsp_cmd.first_line_field_6 = mw_cmd.first_line_field_6;
	dsp_cmd.first_line_field_7 = mw_cmd.first_line_field_7;
	dsp_cmd.sensor_readout_mode = mw_cmd.sensor_readout_mode;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}


/**
 * Fixed pattern noise correction
 */
static int dsp_fixed_pattern_noise_correct(iav_context_t *context,
struct fixed_pattern_correct *param)
{
	fixed_pattern_noise_correct_t dsp_cmd;
	struct fixed_pattern_correct mw_cmd;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = FIXED_PATTERN_NOISE_CORRECTION;
	dsp_cmd.fpn_pixel_mode = mw_cmd.fpn_pixel_mode;
	dsp_cmd.num_of_rows = mw_cmd.num_of_rows;
	dsp_cmd.num_of_cols = mw_cmd.num_of_cols;
	dsp_cmd.fpn_pitch = mw_cmd.fpn_pitch;
 	dsp_cmd.fpn_pixels_addr =  VIRT_TO_DSP(pixel_map_addr);
	dsp_cmd.intercepts_and_slopes_addr =  VIRT_TO_DSP(fpn_reg_addr);

	if(mw_cmd.fpn_pixels_buf_size > PIXEL_MAP_MAX_SIZE)
		return -1;
	dsp_cmd.fpn_pixels_buf_size = mw_cmd.fpn_pixels_buf_size;
	dsp_cmd.intercept_shift = 3;
	dsp_cmd.row_gain_enable = 0;
 	dsp_cmd.row_gain_addr = 0;
	dsp_cmd.column_gain_enable = 0;//mw_cmd.column_gain_enable;
 	dsp_cmd.column_gain_addr = 0;//VIRT_TO_DSP(column_offset);

	if (copy_from_user(pixel_map_addr,
		(void*)mw_cmd.fpn_pixels_addr, dsp_cmd.fpn_pixels_buf_size))
		return -EFAULT;
	if (copy_from_user(fpn_reg_addr,
		(void*)mw_cmd.intercepts_and_slopes_addr, 1024))
		return -EFAULT;
	clean_cache_aligned(pixel_map_addr, dsp_cmd.fpn_pixels_buf_size);
	clean_cache_aligned(fpn_reg_addr, 1024);

//	memcpy(column_offset, (void*)mw_cmd.column_gain_addr, 4096);
//	clean_cache_aligned(column_offset, 4096);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_black_level_state_table(iav_context_t *context,
	struct black_level_state *param)
{
	black_level_state_table_t dsp_cmd;
	struct black_level_state mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = BLACK_LEVEL_STATE_TABLES;
	dsp_cmd.num_columns = mw_cmd.num_columns;
	dsp_cmd.column_frame_acc_addr = VIRT_TO_DSP(column_acc);
	dsp_cmd.column_average_acc_addr = 0;//mw_cmd.column_average_acc_addr;
	dsp_cmd.num_rows = mw_cmd.num_rows;
	dsp_cmd.row_fixed_offset_addr = 0;//mw_cmd.row_frame_offset_addr;
	dsp_cmd.row_average_acc_addr = 0;//mw_cmd.row_average_acc_addr;

	if (copy_from_user(column_acc, (void*)mw_cmd.column_frame_acc_addr, 8192))
		return -EFAULT;
	clean_cache_aligned(column_acc, 8192);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

#if 0
static int dsp_chroma_aberration_ctrl(iav_context_t *context,
	struct chroma_aberration_warp_ctrl_info *param)
{

	set_chromatic_aberration_warp_control_t dsp_cmd;
	struct chroma_aberration_warp_ctrl_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMATIC_ABERRATION_WARP_CONTROL;

	dsp_cmd.horz_warp_enable = mw_cmd.horz_warp_enable;
	dsp_cmd.vert_warp_enable = mw_cmd.vert_warp_enable;

	dsp_cmd.horz_pass_grid_array_width = mw_cmd.horz_pass_grid_array_width;
	dsp_cmd.horz_pass_grid_array_height = mw_cmd.horz_pass_grid_array_height;
	dsp_cmd.horz_pass_horz_grid_spacing_exponent = mw_cmd.horz_pass_horz_grid_spacing_exponent;
	dsp_cmd.horz_pass_vert_grid_spacing_exponent = mw_cmd.horz_pass_vert_grid_spacing_exponent;
	dsp_cmd.vert_pass_grid_array_width = mw_cmd.vert_pass_grid_array_width;
	dsp_cmd.vert_pass_grid_array_height = mw_cmd.vert_pass_grid_array_height;
	dsp_cmd.vert_pass_horz_grid_spacing_exponent = mw_cmd.vert_pass_horz_grid_spacing_exponent;
	dsp_cmd.vert_pass_vert_grid_spacing_exponent = mw_cmd.vert_pass_vert_grid_spacing_exponent;

	dsp_cmd.red_scale_factor = mw_cmd.red_scale_factor;
	dsp_cmd.blue_scale_factor = mw_cmd.blue_scale_factor;
	dsp_cmd.warp_horizontal_table_address = mw_cmd.warp_horizontal_table_address;
	dsp_cmd.warp_vertical_table_address = mw_cmd.warp_vertical_table_address;
//TODO: copy the warp table here;
//	memcpy(lnl_tone_curve, (void*)mw_cmd.tone_curve_addr, 256*sizeof(u16));
//	clean_cache_aligned(mw_cmd->tone_curve_addr, 256*sizeof(u16));
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}
#endif

static int dsp_zoom_factor (iav_context_t *context,
	struct zoom_factor_info *param)
{
	VCAP_SET_ZOOM_CMD  dsp_cmd;
	struct zoom_factor_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CMD_VCAP_SET_ZOOM;
	dsp_cmd.channel_id = 0;
	dsp_cmd.stream_type = 0;
	dsp_cmd.reserved = 0;
	dsp_cmd.zoom_x = mw_cmd.zoom_x;
	dsp_cmd.zoom_y = mw_cmd.zoom_y;
	dsp_cmd.x_center_offset = mw_cmd.x_center_offset;
	dsp_cmd.y_center_offset = mw_cmd.y_center_offset;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_luma_sharp_lnl (iav_context_t *context,
	struct luma_sharp_lnl_info *param)
{
	luma_sharpening_LNL_t dsp_cmd;
	struct luma_sharp_lnl_info mw_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_LNL;
	dsp_cmd.group_index = 0;
	dsp_cmd.enable = mw_cmd.enable;
	dsp_cmd.output_normal_luma_size_select = mw_cmd.output_normal_luma_size_select;
	dsp_cmd.output_low_noise_luma_size_select = mw_cmd.output_low_noise_luma_size_select;
	dsp_cmd.reserved = mw_cmd.reserved;
	dsp_cmd.input_weight_red = mw_cmd.input_weight_red;
	dsp_cmd.input_weight_green = mw_cmd.input_weight_green;
	dsp_cmd.input_weight_blue = mw_cmd.input_weight_blue;
	dsp_cmd.input_shift_red = mw_cmd.input_shift_red;
	dsp_cmd.input_shift_green = mw_cmd.input_shift_green;
	dsp_cmd.input_shift_blue = mw_cmd.input_shift_blue;
	dsp_cmd.input_clip_red = mw_cmd.input_clip_red;
	dsp_cmd.input_clip_green = mw_cmd.input_clip_green;
	dsp_cmd.input_clip_blue = mw_cmd.input_clip_blue;
	dsp_cmd.input_offset_red = mw_cmd.input_offset_red;
	dsp_cmd.input_offset_green = mw_cmd.input_offset_green;
	dsp_cmd.input_offset_blue = mw_cmd.input_offset_blue;
	dsp_cmd.output_normal_luma_weight_a = mw_cmd.output_normal_luma_weight_a;
	dsp_cmd.output_normal_luma_weight_b = mw_cmd.output_normal_luma_weight_b;
	dsp_cmd.output_normal_luma_weight_c = mw_cmd.output_normal_luma_weight_c;
	dsp_cmd.output_low_noise_luma_weight_a = mw_cmd.output_low_noise_luma_weight_a;
	dsp_cmd.output_low_noise_luma_weight_b = mw_cmd.output_low_noise_luma_weight_b;
	dsp_cmd.output_low_noise_luma_weight_c = mw_cmd.output_low_noise_luma_weight_c;
	dsp_cmd.output_combination_min = mw_cmd.output_combination_min;
	dsp_cmd.output_combination_max = mw_cmd.output_combination_max;
	dsp_cmd.tone_curve_addr = VIRT_TO_DSP(lnl_tone_curve);

	if (copy_from_user(lnl_tone_curve,
		(void*)mw_cmd.tone_curve_addr, 256*sizeof(u16)))
		return -EFAULT;
	clean_cache_aligned(lnl_tone_curve, 256*sizeof(u16));
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int img_dump_idsp(iav_context_t* context,
	iav_idsp_config_info_t __user * param)
{
	iav_idsp_config_info_t mw_cmd;
	amb_dsp_debug_2_t dsp_cmd;
	int output_id = 0;
	int rval = -1;

//	load_ucode_target_t tgr;
//	u8  *def_bin_addr;
//	u8  *idsp_cfg_addr;
//	u8 chip_rev = 0xB5;
	u32 *p_sec_size;

	u8* dump_buffer = kzalloc(MAX_DUMP_BUFFER_SIZE,GFP_KERNEL);
	if (dump_buffer == NULL) {
		printk("get null dump_buffer\n");
		return -1;
	}

	memset(dump_buffer, 0, MAX_DUMP_BUFFER_SIZE);
	if (copy_from_user(&mw_cmd, param, sizeof(iav_idsp_config_info_t)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(amb_dsp_debug_2_t));
	output_id = mw_cmd.id_section;
	*((int *)dump_buffer) = 0xdeadbeef;
	clean_d_cache(dump_buffer, MAX_DUMP_BUFFER_SIZE);

	dsp_cmd.cmd_code=AMB_DSP_DEBUG_2;
	dsp_cmd.mode=mw_cmd.id_section;
	dsp_cmd.dram_addr = VIRT_TO_DSP(dump_buffer);
	dsp_cmd.dram_size = MAX_DUMP_BUFFER_SIZE;

//	printk("arm %p dsp %p\n", dump_buffer, (void*)dsp_cmd.dram_addr);
	switch(output_id)
	{
		case DUMP_IDSP_0:
		case DUMP_IDSP_1:
		case DUMP_IDSP_2:
		case DUMP_IDSP_3:
		case DUMP_IDSP_4:
		case DUMP_IDSP_5:
		case DUMP_IDSP_6:
		case DUMP_IDSP_7:
		case DUMP_IDSP_100:
			dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
//			dsp_issue_cmd(&dsp_cmd, sizeof(amb_dsp_debug_2_t));

			msleep(1000);
			invalidate_d_cache(dump_buffer, MAX_DUMP_BUFFER_SIZE);
			if (*((int *)dump_buffer) == 0xdeadbeef)
			{
				printk("DSP is halted! Dumping post mortem section 0 debug data\n");
			/*	get_dsp_info(&tgr);
				def_bin_addr = (u8*)tgr.dsp.dsp_binary_data_addr;
				idsp_cfg_addr = def_bin_addr + 2048 + 16 + 6144 -64;
				memcpy(dump_buffer, idsp_cfg_addr, MAX_DUMP_BUFFER_SIZE);
				*dump_buffer = chip_rev;
				dsp_cmd.dram_size = MAX_DUMP_BUFFER_SIZE;*/
				kfree(dump_buffer);
				return -EFAULT;
			}
			else
			{
				p_sec_size = (u32*)(dump_buffer + 8);
				dsp_cmd.dram_size = ((u32)*p_sec_size) + 64; //64 for header,
				printk("sec_size %d %d\n", *p_sec_size, dsp_cmd.dram_size);
			}
			param->addr_long = dsp_cmd.dram_size;

			if(copy_to_user(param->addr,dump_buffer,dsp_cmd.dram_size))
			{
				printk("cpy to usr err\n");
				kfree(dump_buffer);
				return -EFAULT;
			}
			rval=0;
			break;

		case DUMP_FPN:
			printk("Not implemented yet %d\n", output_id);
			break;

		case DUMP_VIGNETTE:
			printk("Not implemented yet %d\n", output_id);
			break;

		default:
			printk("Unknown output id %d\n", output_id);
			rval = -1;
			break;
	}

	kfree(dump_buffer);
	return rval;
}

static int dsp_hdr_set_video_proc(iav_context_t * context,
	struct video_hdr_proc_control __user * arg)
{
	int i;
	u8 * addr = NULL, *alpha_addr = NULL, *yuv_addr = NULL;
	struct iav_global_info *g_info = context->g_info;
	VCAP_SET_VIDEO_HDR_PROC_CONTROL_CMD dsp_cmd;

	if (!is_in_3a_work_state(context)) {
		printk("CANNOT set HDR video proc in non-preview or non-encoding state!\n");
		return -EPERM;
	}

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	if (g_info->hdr_mode) {
		dsp_cmd.cmd_code = CMD_VCAP_SET_VIDEO_HDR_PROC_CONTROL;
		dsp_cmd.exp_0_updated = g_hdr_expo[0].updated;
		dsp_cmd.exp_1_updated = g_hdr_expo[1].updated;
		dsp_cmd.exp_2_updated = g_hdr_expo[2].updated;
		dsp_cmd.exp_3_updated = g_hdr_expo[3].updated;

		for (i = 0; i < MAX_EXPOSURE_NUM; ++i) {
			if (g_hdr_expo[i].updated) {
				addr = (u8*)hdr_proc_buf +
					g_curr_hdr_buf_idx * HDR_PROC_BUFFRE_SIZE +
					i * HDR_IDSP_EXPO_BUF_SIZE;
				dsp_cmd.hdr_ctrl_param_daddr[i] = VIRT_TO_DSP(addr);
				clean_cache_aligned(addr, HDR_IDSP_EXPO_BUF_SIZE);

				alpha_addr = (u8*) hdr_alpha_map_buf +
					g_curr_hdr_buf_idx*HDR_ALPHA_MAP_SIZE + i*HDR_IDSP_CMD_SIZE;
				clean_cache_aligned(alpha_addr, HDR_IDSP_CMD_SIZE);
				dsp_cmd.color_correction_cmd_alpha_map_daddr[i] = VIRT_TO_DSP(alpha_addr);
				g_hdr_expo[i].updated = 0;
			}
		}

		yuv_addr = (u8*)hdr_yuv_cntl_buf + g_curr_hdr_buf_idx * HDR_YUV_CNTL_BUFF_SIZE;
		clean_cache_aligned(yuv_addr, HDR_YUV_CNTL_BUFF_SIZE);
		dsp_cmd.yuv_ctrl_param_daddr = VIRT_TO_DSP(yuv_addr);
		dsp_cmd.low_pass_filter_radius = g_yuv_ce_info.low_pass_radius;
		dsp_cmd.contrast_enhance_gain = g_yuv_ce_info.contrast_enhance_gain;

		g_curr_hdr_buf_idx = (g_curr_hdr_buf_idx + 1) % HDR_BUF_NUM;

		img_dsp_hex(dsp_cmd, cmd_code);
		img_dsp(dsp_cmd, exp_0_updated);
		img_dsp(dsp_cmd, exp_1_updated);
		img_dsp(dsp_cmd, exp_2_updated);
		img_dsp(dsp_cmd, exp_3_updated);
		img_dsp_hex(dsp_cmd, hdr_ctrl_param_daddr[0]);
		img_dsp_hex(dsp_cmd, hdr_ctrl_param_daddr[1]);
		img_dsp_hex(dsp_cmd, hdr_ctrl_param_daddr[2]);
		img_dsp_hex(dsp_cmd, hdr_ctrl_param_daddr[3]);
		img_dsp_hex(dsp_cmd, color_correction_cmd_alpha_map_daddr[0]);
		img_dsp_hex(dsp_cmd, color_correction_cmd_alpha_map_daddr[1]);
		img_dsp_hex(dsp_cmd, color_correction_cmd_alpha_map_daddr[2]);
		img_dsp_hex(dsp_cmd, color_correction_cmd_alpha_map_daddr[3]);
		img_dsp_hex(dsp_cmd, yuv_ctrl_param_daddr);
		img_dsp_hex(dsp_cmd, low_pass_filter_radius);
		img_dsp_hex(dsp_cmd, contrast_enhance_gain);

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_hdr_set_raw_offset(iav_context_t * context,
	struct video_hdr_proc_control __user * arg)
{
	struct iav_global_info *g_info = context->g_info;
	VCAP_SET_VIDEO_HDR_PROC_CONTROL_CMD dsp_cmd;
	struct video_hdr_proc_control raw_offset;

	if(copy_from_user(&raw_offset, arg, sizeof(raw_offset)))
		return -EFAULT;

	if (!is_in_3a_work_state(context)) {
		printk("CANNOT set HDR video proc in non-preview or non-encoding state!\n");
		return -EPERM;
	}

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	if(g_info->hdr_mode){
		dsp_cmd.cmd_code = CMD_VCAP_SET_VIDEO_HDR_PROC_CONTROL;
		dsp_cmd.exp_0_start_offset_updated = 1;
		dsp_cmd.exp_1_start_offset_updated = 1;
		dsp_cmd.exp_2_start_offset_updated = 1;
		dsp_cmd.exp_3_start_offset_updated = 1;

		dsp_cmd.raw_daddr_start_offset[0] = raw_offset.raw_start_y_offset[0];
		dsp_cmd.raw_daddr_start_offset[1] = raw_offset.raw_start_y_offset[1];
		dsp_cmd.raw_daddr_start_offset[2] = raw_offset.raw_start_y_offset[2];
		dsp_cmd.raw_daddr_start_offset[3] = raw_offset.raw_start_y_offset[3];

		dsp_cmd.raw_daddr_start_offset_x[0] = raw_offset.raw_start_x_offset[0];
		dsp_cmd.raw_daddr_start_offset_x[1] = raw_offset.raw_start_x_offset[1];
		dsp_cmd.raw_daddr_start_offset_x[2] = raw_offset.raw_start_x_offset[2];
		dsp_cmd.raw_daddr_start_offset_x[3] = raw_offset.raw_start_x_offset[3];

		//Don't remove this. LPF radius should be configed each time CMD_VCAP_SET_VIDEO_HDR_PROC_CONTROL is issued.
		dsp_cmd.low_pass_filter_radius = g_yuv_ce_info.low_pass_radius;
		dsp_cmd.contrast_enhance_gain = g_yuv_ce_info.contrast_enhance_gain;

		img_dsp_hex(dsp_cmd, cmd_code);
		img_dsp(dsp_cmd, exp_0_start_offset_updated);
		img_dsp(dsp_cmd, exp_1_start_offset_updated);
		img_dsp(dsp_cmd, exp_2_start_offset_updated);
		img_dsp(dsp_cmd, exp_3_start_offset_updated);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset[0]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset[1]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset[2]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset[3]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset_x[0]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset_x[1]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset_x[2]);
		img_dsp_hex(dsp_cmd, raw_daddr_start_offset_x[3]);

		dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_hdr_start_cmd_block(iav_context_t * context, u32 exposure_index)
{
	if ((g_curr_expo_idx != exposure_index) &&
		g_hdr_expo[g_curr_expo_idx].enable) {
		img_printk("Cannot start HDR cmd block [%d] again before other cmd"
			" block [%d] closed!\n", exposure_index, g_curr_expo_idx);
		return -EAGAIN;
	}
	g_curr_expo_idx = exposure_index;
	g_hdr_expo[g_curr_expo_idx].enable = 1;
	g_hdr_expo[g_curr_expo_idx].cmd_num = 0;

	img_printk("\nStart command block for exposure [%d].\n", g_curr_expo_idx);
	return 0;
}

static int dsp_hdr_end_cmd_block(iav_context_t * context, u32 exposure_index)
{
	if (g_curr_expo_idx != exposure_index) {
		img_printk("Cannot close cmd block [%d] which was NOT opened!\n",
			exposure_index);
	}
	g_hdr_expo[g_curr_expo_idx].enable = 0;
	g_hdr_expo[g_curr_expo_idx].updated = 1;

	img_printk("End command block for exposure [%d].\n\n", g_curr_expo_idx);
	return 0;
}

static int dsp_hdr_black_level_global_offset(iav_context_t *context,
	struct black_level_global_offset __user *param)
{
	black_level_global_offset_t	dsp_cmd;
	struct black_level_global_offset mw_cmd;
	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if (g_info->hdr_mode) {
		dsp_cmd.cmd_code = BLACK_LEVEL_GLOBAL_OFFSET;
		dsp_cmd.global_offset_ee = mw_cmd.global_offset_ee;
		dsp_cmd.global_offset_eo = mw_cmd.global_offset_eo;
		dsp_cmd.global_offset_oe = mw_cmd.global_offset_oe;
		dsp_cmd.global_offset_oo = mw_cmd.global_offset_oo;
		dsp_cmd.black_level_offset_red = mw_cmd.black_level_offset_red;
		dsp_cmd.black_level_offset_green = mw_cmd.black_level_offset_green;
		dsp_cmd.black_level_offset_blue = mw_cmd.black_level_offset_blue;
		dsp_cmd.gain_depedent_offset_red = mw_cmd.gain_depedent_offset_red;
		dsp_cmd.gain_depedent_offset_green = mw_cmd.gain_depedent_offset_green;
		dsp_cmd.gain_depedent_offset_blue = mw_cmd.gain_depedent_offset_blue;

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_hdr_cfa_noise_filter(iav_context_t *context,
	struct cfa_noise_filter_info __user *param)
{

	cfa_noise_filter_t dsp_cmd;
	struct cfa_noise_filter_info mw_cmd;
	struct iav_global_info	*g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if (g_info->hdr_mode) {
		dsp_cmd.cmd_code = CFA_NOISE_FILTER;
		dsp_cmd.enable = mw_cmd.enable;
		dsp_cmd.mode = mw_cmd.mode;
		dsp_cmd.shift_coarse_ring1 = mw_cmd.shift_coarse_ring1;
		dsp_cmd.shift_coarse_ring2 = mw_cmd.shift_coarse_ring2;
		dsp_cmd.shift_fine_ring1 = mw_cmd.shift_fine_ring1;
		dsp_cmd.shift_fine_ring2 = mw_cmd.shift_fine_ring2;
		dsp_cmd.shift_center_red = mw_cmd.shift_center_red;
		dsp_cmd.shift_center_green = mw_cmd.shift_center_green;
		dsp_cmd.shift_center_blue = mw_cmd.shift_center_blue;
		dsp_cmd.target_coarse_red = mw_cmd.target_coarse_red;
		dsp_cmd.target_coarse_green = mw_cmd.target_coarse_green;
		dsp_cmd.target_coarse_blue = mw_cmd.target_coarse_blue;
		dsp_cmd.target_fine_red = mw_cmd.target_fine_red;
		dsp_cmd.target_fine_green = mw_cmd.target_fine_green;
		dsp_cmd.target_fine_blue = mw_cmd.target_fine_blue;
		dsp_cmd.cutoff_red = mw_cmd.cutoff_red;
		dsp_cmd.cutoff_green = mw_cmd.cutoff_green;
		dsp_cmd.cutoff_blue = mw_cmd.cutoff_blue;
		dsp_cmd.thresh_coarse_red = mw_cmd.thresh_coarse_red;
		dsp_cmd.thresh_coarse_green = mw_cmd.thresh_coarse_green;
		dsp_cmd.thresh_coarse_blue = mw_cmd.thresh_coarse_blue;
		dsp_cmd.thresh_fine_red = mw_cmd.thresh_fine_red;
		dsp_cmd.thresh_fine_green = mw_cmd.thresh_fine_green;
		dsp_cmd.thresh_fine_blue = mw_cmd.thresh_fine_blue;

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;

}

static int dsp_hdr_local_exposure(iav_context_t *context,
	struct local_exposure_info __user *param)
{
	local_exposure_t dsp_cmd;
	struct local_exposure_info mw_cmd;
	struct iav_global_info	*g_info = context->g_info;
	void* hdr_le_curve;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	hdr_le_curve = exposure_gain_curve+g_curr_expo_idx*NUM_EXPOSURE_CURVE*sizeof(u16);
	if (copy_from_user(hdr_le_curve, (void*)(mw_cmd.gain_curve_table_addr),
		NUM_EXPOSURE_CURVE*sizeof(u16)))
		return -EFAULT;

	if (g_info->hdr_mode) {
		dsp_cmd.cmd_code = LOCAL_EXPOSURE;
		dsp_cmd.enable = mw_cmd.enable;
		dsp_cmd.radius = mw_cmd.radius;
		dsp_cmd.luma_weight_red = mw_cmd.luma_weight_red;
		dsp_cmd.luma_weight_green = mw_cmd.luma_weight_green;
		dsp_cmd.luma_weight_blue = mw_cmd.luma_weight_blue;
		dsp_cmd.luma_weight_sum_shift = mw_cmd.luma_weight_sum_shift;
		dsp_cmd.gain_curve_table_addr = VIRT_TO_DSP(hdr_le_curve);
		dsp_cmd.luma_offset = mw_cmd.luma_offset;

		clean_cache_aligned(hdr_le_curve, NUM_EXPOSURE_CURVE*sizeof(u16));
		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_hdr_mixer(iav_context_t * context, struct hdr_mixer_info * arg)
{
	hdr_mixer_t dsp_cmd;
	struct hdr_mixer_info mw_cmd;
	struct iav_global_info * g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&mw_cmd, arg, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	if (g_info->hdr_mode) {
		dsp_cmd.cmd_code = HDR_MIXER;
		dsp_cmd.mixer_mode = mw_cmd.mixer_mode;
		dsp_cmd.radius = mw_cmd.radius;
		dsp_cmd.luma_weight_red = mw_cmd.luma_weight_red;
		dsp_cmd.luma_weight_green = mw_cmd.luma_weight_green;
		dsp_cmd.luma_weight_blue = mw_cmd.luma_weight_blue;
		dsp_cmd.threshold = mw_cmd.threshold;
		dsp_cmd.thresh_delta = mw_cmd.thresh_delta;
		dsp_cmd.long_exposure_shift = mw_cmd.long_exposure_shift;

		save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));
		img_printk("Current exposure idx [%d], mix_mode [%d].\n",
			g_curr_expo_idx, dsp_cmd.mixer_mode);
	}

	return 0;
}

static int dsp_vcap_no_op(iav_context_t * context, struct vcap_no_op __user* arg)
{
	VCAP_NO_OP_CMD dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_VCAP_NO_OP;
	dsp_cmd.channel_id = 0x0;
	dsp_cmd.stream_type = 0x0;

	save_hdr_idsp_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

//####below is for new IK only start

static int dsp_aaa_statistics_setup_ni (iav_context_t *context,
	aaa_statistics_setup_t __user * param)
{
	aaa_statistics_setup_t	dsp_cmd;

//	if (!is_in_3a_work_state(context))
//		return -EPERM;
	if(g_dsp_mode != VCAP_VIDEO_MODE)
		return -EPERM;


	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = AAA_STATISTICS_SETUP;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_mctf_mv_stabilizer_setup_ni (iav_context_t *context,
	VCAP_MCTF_MV_STAB_CMD __user *param)
{
	VCAP_MCTF_MV_STAB_CMD dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if(copy_from_user(mctf_cfg_addr, (void*)param->mctf_cfg_dbase, MCTF_CFG_SIZE))
		return -EFAULT;

	clean_cache_aligned(mctf_cfg_addr, MCTF_CFG_SIZE);

	if(param->cc_cfg_dbase !=0) {
		if(copy_from_user(cc_cfg_addr, (void*)param->cc_cfg_dbase, SEC_CC_SIZE))
			return -EFAULT;
		clean_cache_aligned(cc_cfg_addr, SEC_CC_SIZE);
	}

	if(copy_from_user(cmpr_cfg_addr, (void*)param->cmpr_cfg_dbase, 544))
		return -EFAULT;
	clean_cache_aligned(cmpr_cfg_addr, 544);

	dsp_cmd.cmd_code = CMD_VCAP_MCTF_MV_STAB;
	dsp_cmd.mctf_cfg_dbase = VIRT_TO_DSP(mctf_cfg_addr);
	dsp_cmd.cc_cfg_dbase = VIRT_TO_DSP(cc_cfg_addr);
	dsp_cmd.cmpr_cfg_dbase = VIRT_TO_DSP(cmpr_cfg_addr);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_mctf_gmv_setup_ni (iav_context_t *context,
	VCAP_MCTF_GMV_CMD __user *param)
{
	VCAP_MCTF_GMV_CMD	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CMD_VCAP_MCTF_GMV;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_black_level_global_offset_ni(iav_context_t *context,
	black_level_global_offset_t __user *param)
{
	black_level_global_offset_t	dsp_cmd;
//	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = BLACK_LEVEL_GLOBAL_OFFSET;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_cfa_domain_leakage_filter_setup_ni(iav_context_t *context,
	cfa_domain_leakage_filter_t __user *param)
{
	cfa_domain_leakage_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = CFA_DOMAIN_LEAKAGE_FILTER;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;

}

static int dsp_cfa_noise_filter_ni (iav_context_t *context,
		cfa_noise_filter_t __user *param)
{
	cfa_noise_filter_t dsp_cmd;
//	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CFA_NOISE_FILTER;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}


static int dsp_vignette_compensation_ni(iav_context_t *context,
	vignette_compensation_t __user *param)
{
	vignette_compensation_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = VIGNETTE_COMPENSATION;

	if(copy_from_user(vignette_r_gain, (void*)param->tile_gain_addr, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if(copy_from_user(vignette_ge_gain, (void*)param->tile_gain_addr_green_even, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if(copy_from_user(vignette_go_gain, (void*)param->tile_gain_addr_green_odd, MAX_VIGNETTE_NUM))
		return -EFAULT;
	if(copy_from_user(vignette_b_gain, (void*)param->tile_gain_addr_blue, MAX_VIGNETTE_NUM))
		return -EFAULT;

	dsp_cmd.tile_gain_addr = VIRT_TO_DSP(vignette_r_gain);
	dsp_cmd.tile_gain_addr_green_even = VIRT_TO_DSP(vignette_ge_gain);
	dsp_cmd.tile_gain_addr_green_odd = VIRT_TO_DSP(vignette_go_gain);
	dsp_cmd.tile_gain_addr_blue = VIRT_TO_DSP(vignette_b_gain);

	clean_cache_aligned(vignette_r_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_ge_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_go_gain, MAX_VIGNETTE_NUM * sizeof(u8));
	clean_cache_aligned(vignette_b_gain, MAX_VIGNETTE_NUM * sizeof(u8));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_local_exposure_ni (iav_context_t *context,
	local_exposure_t __user *param)
{
	local_exposure_t dsp_cmd;
//	struct iav_global_info *g_info = context->g_info;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if(copy_from_user(exposure_gain_curve, (void*)(param->gain_curve_table_addr),
		NUM_EXPOSURE_CURVE*sizeof(u16)))
		return -EFAULT;

	dsp_cmd.cmd_code = LOCAL_EXPOSURE;
	dsp_cmd.gain_curve_table_addr = VIRT_TO_DSP(exposure_gain_curve);


	clean_cache_aligned(exposure_gain_curve, NUM_EXPOSURE_CURVE*sizeof(u16));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_color_correction_ni (iav_context_t *context,
	color_correction_t __user *param)
{
	color_correction_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = COLOR_CORRECTION;
	dsp_cmd.in_lookup_table_addr = VIRT_TO_DSP(input_lookup_table);
	dsp_cmd.matrix_addr = VIRT_TO_DSP(matrix_dram_address);

	if (dsp_cmd.output_lookup_bypass == 1) {
		dsp_cmd.out_lookup_table_addr = 0;
	} else {
		dsp_cmd.out_lookup_table_addr = VIRT_TO_DSP(output_lookup_table);
		if(param->out_lookup_table_addr != 0)
			if(copy_from_user(output_lookup_table, (void*)param->out_lookup_table_addr,
				NUM_OUT_LOOKUP * sizeof(u32)))
				return -EFAULT;
	}
	if(param->in_lookup_table_addr != 0)
		if(copy_from_user(input_lookup_table, (void*)param->in_lookup_table_addr,
			NUM_IN_LOOKUP * sizeof(u32)))
			return -EFAULT;
	if(param->matrix_addr != 0)
		if(copy_from_user(matrix_dram_address, (void*)param->matrix_addr,
			NUM_MATRIX * sizeof(u32)))
			return -EFAULT;

	clean_cache_aligned(input_lookup_table, NUM_IN_LOOKUP*sizeof(u32));
	clean_cache_aligned(matrix_dram_address, NUM_MATRIX*sizeof(u32));
	clean_cache_aligned(output_lookup_table, NUM_OUT_LOOKUP*sizeof(u32));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_rgb_to_yuv_setup_ni (iav_context_t *context,
	rgb_to_yuv_setup_t __user *param)
{
	rgb_to_yuv_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = RGB_TO_YUV_SETUP;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_chroma_scale_ni (iav_context_t *context,
	chroma_scale_t __user *param)
{
	chroma_scale_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_SCALE;
	dsp_cmd.gain_curver_addr = VIRT_TO_DSP(chroma_gain_curve);

	if(copy_from_user(chroma_gain_curve, (void*)(param->gain_curver_addr),
		NUM_CHROMA_GAIN_CURVE*sizeof(u16)))
		return -EFAULT;

	clean_cache_aligned(chroma_gain_curve,
		NUM_CHROMA_GAIN_CURVE*sizeof(u16));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_noise_filter_ni (iav_context_t *context,
	chroma_noise_filter_t __user *param)
{
	chroma_noise_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_NOISE_FILTER;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_median_filter_ni (iav_context_t *context,
	chroma_median_filter_info_t __user *param)
{
	chroma_median_filter_info_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_MEDIAN_FILTER;
	dsp_cmd.k0123_table_addr = VIRT_TO_DSP(k0123_table);

	if(copy_from_user(k0123_table, (void*)param->k0123_table_addr, K0123_ARRAY_SIZE*sizeof(u16)))
		return -EFAULT;
	clean_cache_aligned(k0123_table, K0123_ARRAY_SIZE*sizeof(u16));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpening_ni (iav_context_t *context,
	luma_sharpening_t __user *param)
{
	luma_sharpening_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = LUMA_SHARPENING;
	dsp_cmd.alpha_table_addr = VIRT_TO_DSP(luma_sharpening_alpha_table);

	if(copy_from_user(luma_sharpening_alpha_table, (void*)param->alpha_table_addr, NUM_ALPHA_TABLE))
		return -EFAULT;
	clean_cache_aligned(luma_sharpening_alpha_table, NUM_ALPHA_TABLE);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharp_fir_ni (iav_context_t *context,
	luma_sharpening_FIR_config_t __user *param)
{
	luma_sharpening_FIR_config_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if(copy_from_user(coeff_fir1_addr, (void*)param->coeff_FIR1_addr, 256))
		return -EFAULT;
	if(copy_from_user(coeff_fir2_addr, (void*)param->coeff_FIR2_addr, 256))
		return -EFAULT;
	if(copy_from_user(coring_table, (void*)param->coring_table_addr, 256))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_FIR_CONFIG;
	dsp_cmd.coeff_FIR1_addr = VIRT_TO_DSP(coeff_fir1_addr);
	dsp_cmd.coeff_FIR2_addr =  VIRT_TO_DSP(coeff_fir2_addr);
	dsp_cmd.coring_table_addr = VIRT_TO_DSP(coring_table);

	clean_cache_aligned((u8 *)coeff_fir1_addr, 256);
	clean_cache_aligned((u8 *)coeff_fir2_addr, 256);
	clean_cache_aligned((u8 *)coring_table, 256);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpen_blend_ctrl_ni (iav_context_t *context,
	luma_sharpening_blend_control_t __user *param)
{
	luma_sharpening_blend_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_BLEND_CONTROL;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;

}

static int dsp_luma_sharpen_level_ctrl_ni (iav_context_t *context,
	luma_sharpening_level_control_t __user *param)
{
	luma_sharpening_level_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = LUMA_SHARPENING_LEVEL_CONTROL;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_luma_sharpen_tone_ni(iav_context_t *context,
	luma_sharpening_tone_control_t __user *param)
{
	luma_sharpening_tone_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = LUMA_SHARPENING_TONE;
	dsp_cmd.tone_based_3d_level_table_addr = VIRT_TO_DSP(luma_3d_table);

	if(copy_from_user(luma_3d_table, (void*)param->tone_based_3d_level_table_addr, LS_THREE_D_TABLE_SIZE>>1))
		return -EFAULT;
	clean_cache_aligned(luma_3d_table, LS_THREE_D_TABLE_SIZE>>1);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_rgb_gain_adjustment_ni (iav_context_t *context,
	rgb_gain_adjust_t __user *param)
{
	rgb_gain_adjust_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

 	dsp_cmd.cmd_code = RGB_GAIN_ADJUSTMENT;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_anti_aliasing_filter_ni(iav_context_t *context,
	anti_aliasing_filter_t __user* param)
{
	anti_aliasing_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = ANTI_ALIASING;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_strong_grgb_mismatch_filter_ni(iav_context_t *context,
	strong_grgb_mismatch_filter_t __user* param)
{
	strong_grgb_mismatch_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;


	dsp_cmd.cmd_code = STRONG_GRGB_MISMATCH_FILTER;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_digital_gain_sat_level_ni(iav_context_t *context,
	digital_gain_level_t __user* param)
{

	digital_gain_level_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = DIGITAL_GAIN_SATURATION_LEVEL;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_bad_pixel_correct_setup_ni (iav_context_t *context,
	bad_pixel_correct_setup_t __user* param)
{
	bad_pixel_correct_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = BAD_PIXEL_CORRECT_SETUP;
	dsp_cmd.hot_pixel_thresh_addr = VIRT_TO_DSP(hot_pixel_thd_table);
	dsp_cmd.dark_pixel_thresh_addr = VIRT_TO_DSP(dark_pixel_thd_table);

	if (copy_from_user(hot_pixel_thd_table,
		(void*)param->hot_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
		return -EFAULT;
	if (copy_from_user(dark_pixel_thd_table,
		(void*)param->dark_pixel_thresh_addr, DYN_BPC_THD_TABLE_SIZE))
		return -EFAULT;
	clean_cache_aligned(hot_pixel_thd_table, DYN_BPC_THD_TABLE_SIZE);
	clean_cache_aligned(dark_pixel_thd_table, DYN_BPC_THD_TABLE_SIZE);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_demoasic_filter_ni (iav_context_t *context,
	demoasic_filter_t __user* param)
{
	demoasic_filter_t dsp_cmd;

	if (is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = DEMOASIC_FILTER;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_sensor_input_setup_ni (iav_context_t *context,
	sensor_input_setup_t *param)
{
	sensor_input_setup_t dsp_cmd;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

 	dsp_cmd.cmd_code = SENSOR_INPUT_SETUP;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}


/**
 * Fixed pattern noise correction
 */
static int dsp_fixed_pattern_noise_correct_ni (iav_context_t *context,
	fixed_pattern_noise_correct_t *param)
{
	fixed_pattern_noise_correct_t dsp_cmd;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = FIXED_PATTERN_NOISE_CORRECTION;
 	dsp_cmd.fpn_pixels_addr =  VIRT_TO_DSP(pixel_map_addr);
	dsp_cmd.intercepts_and_slopes_addr =  VIRT_TO_DSP(fpn_reg_addr);

	if(param->fpn_pixels_buf_size > PIXEL_MAP_MAX_SIZE)
		return -1;
	dsp_cmd.intercept_shift = 3;
	dsp_cmd.row_gain_enable = 0;
 	dsp_cmd.row_gain_addr = 0;
	dsp_cmd.column_gain_enable = 0;//mw_cmd.column_gain_enable;
 	dsp_cmd.column_gain_addr = 0;//VIRT_TO_DSP(column_offset);

	if(copy_from_user(pixel_map_addr, (void*)param->fpn_pixels_addr, dsp_cmd.fpn_pixels_buf_size))
		return -EFAULT;
	clean_cache_aligned(pixel_map_addr, dsp_cmd.fpn_pixels_buf_size);

	if(copy_from_user(fpn_reg_addr, (void*)param->intercepts_and_slopes_addr, 1024))
		return -EFAULT;
	clean_cache_aligned(fpn_reg_addr, 1024);

//	memcpy(column_offset, (void*)mw_cmd.column_gain_addr, 4096);
//	clean_cache_aligned(column_offset, 4096);

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_luma_sharp_lnl_ni (iav_context_t *context,
	luma_sharpening_LNL_t *param)
{
	luma_sharpening_LNL_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;


	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_LNL;
	dsp_cmd.tone_curve_addr = VIRT_TO_DSP(lnl_tone_curve);

	if(copy_from_user(lnl_tone_curve, (void*)param->tone_curve_addr, 256*sizeof(u16)))
		return -EFAULT;
	clean_cache_aligned(lnl_tone_curve, 256*sizeof(u16));

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}


static int dsp_set_hiso_cfg_ni(iav_context_t * context, u8 __user* arg)
{
	u8 * cfg_addr = (u8*)ambarella_phys_to_virt(phys_start+0x400000);

	if(copy_from_user(cfg_addr, arg, HISO_CFG_DATA_SIZE))
		return -EFAULT;
//	print_addr((a9_high_iso_param_t*)cfg_addr);
	printk("boot hiso cfg\n");
	return 0;

}

static int dsp_update_hiso_cfg_ni(iav_context_t * context, VCAP_HISO_CONFIG_UPDATE_CMD __user* arg)
{

	static u8 buf_id = 0;
	VCAP_HISO_CONFIG_UPDATE_CMD dsp_cmd;

	u8 * dest_addr = (u8*)ambarella_phys_to_virt(phys_start+0x400000+(buf_id+1)*0x400);
	if(copy_from_user(dest_addr, (void*)arg->hiso_param_daddr, HISO_CFG_DATA_SIZE))
		return -EFAULT;

	dsp_cmd.cmd_code = CMD_VCAP_HISO_CONFIG_UPDATE;
	dsp_cmd.loadcfg_type.word = arg->loadcfg_type.word;
	dsp_cmd.hiso_param_daddr = AMBVIRT_TO_DSP(dest_addr);
	buf_id ^= 1;

	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

//for raw2enc mode

static u32 prev_addr = 0;
static u32 proc_count = 0;
static u32 raw_batch_num = 0;
static int img_count_proc_num(u32 addr)
{
#ifdef CONFIG_IMGPROC_MEM_LARGE
//	printk("addr 0x%x 0x%x cnt %d\n", addr, prev_addr, raw_batch_num);
	if(addr != prev_addr) {
		proc_count++;
		prev_addr = addr;
	}
	if(proc_count >= raw_batch_num && raw_batch_num!=0) {
		proc_count = 0;
		img_printk("process done\n");
		complete(&g_raw2yuv_comp);
	}
#endif
	return 0;
}

static int img_reset_raw2enc(iav_context_t *context)
{
	prev_addr = 0;
	proc_count = 0;
	raw_batch_num = 0;
	INIT_COMPLETION(g_raw2yuv_comp);
	return 0;
}

static int img_wait_raw2enc(iav_context_t *context)
{

	mutex_unlock(context->mutex);
	if (wait_for_completion_interruptible(&g_raw2yuv_comp)) {
		mutex_lock(context->mutex);
		iav_error("Failed to wait for raw2yuv completion!\n");
		return -EINTR;
	}
	mutex_lock(context->mutex);

	return 0;
}

static int img_feed_raw(iav_context_t *context, struct raw2enc_raw_feed_info __user *param)
{
	raw_encode_video_setup_cmd_t dsp_cmd;
	struct raw2enc_raw_feed_info mw_cmd;
	if(copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = RAW_ENCODE_VIDEO_SETUP_CMD;
	dsp_cmd.sensor_raw_start_daddr = PHYS_TO_DSP(_get_phys_addr_raw2enc( (u8*)mw_cmd.sensor_raw_start_daddr));
	dsp_cmd.daddr_offset = mw_cmd.daddr_offset;
	dsp_cmd.raw_width = mw_cmd.raw_width;
	dsp_cmd.raw_height = mw_cmd.raw_height;
	dsp_cmd.dpitch = mw_cmd.dpitch;
	dsp_cmd.raw_compressed = mw_cmd.raw_compressed;
	dsp_cmd.num_frames = mw_cmd.num_frames;

	raw_batch_num = mw_cmd.num_frames;
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}
//####for new IK only end

static int img_check_dsp_mode(iav_context_t *context)
{
	mutex_unlock(context->mutex);
	printk("mode0 = %d\n", g_dsp_mode);
	if(wait_event_interruptible(g_dsp_wq, g_dsp_mode==VCAP_VIDEO_MODE)<0) {
		mutex_lock(context->mutex);
		return -EINTR;
	}
	printk("mode1 = %d\n", g_dsp_mode);
	mutex_lock(context->mutex);
	return 0;
}

int amba_imgproc_cmd(iav_context_t *context, unsigned int cmd, unsigned long arg)
{
	int rval;

	switch (cmd) {
	case	IAV_IOC_IMG_GET_STATISTICS:
		rval = img_get_statistics(context, (struct img_statistics __user*)arg);
		break;
	case	IAV_IOC_IMG_CONFIG_STATISTICS:
		rval = dsp_aaa_statistics_setup(context, (struct aaa_statistics_config __user *)arg);
		break;
	case IAV_IOC_IMG_CONFIG_FLOAT_STATISTICS:
		rval = dsp_aaa_floating_statistics_setup(context, (struct aaa_floating_tile_config_info __user *) arg);
		break;
	case	IAV_IOC_IMG_AAA_SETUP_EX:
		rval = dsp_aaa_statistics_ex(context, (struct aaa_statistics_ex __user*)arg);
		break;
	case IAV_IOC_IMG_CONFIG_HISTOGRAM:
		rval = dsp_aaa_histogram_setup(context, (struct aaa_histogram_config __user *)arg);
		break;
	case	IAV_IOC_IMG_NOISE_FILTER_SETUP:
		rval = dsp_noise_filter_setup(context, (u32)arg);
		break;
	case	IAV_IOC_IMG_BLACK_LEVEL_GLOBAL_OFFSET:
		rval = dsp_black_level_global_offset(context, (struct black_level_global_offset __user*)arg);
		break;
	case	IAV_IOC_IMG_BAD_PIXEL_CORRECTION:
		rval = dsp_bad_pixel_correct_setup(context, (struct bad_pixel_correct_info __user*)arg);
		break;
	case	IAV_IOC_IMG_CFA_LEAKAGE_FILTER_SETUP:
		rval = dsp_cfa_domain_leakage_filter_setup(context, (struct cfa_leakage_filter_info __user*)arg);
		break;
	case	IAV_IOC_IMG_CFA_NOISE_FILTER_SETUP:
		rval = dsp_cfa_noise_filter(context, (struct cfa_noise_filter_info __user*)arg);
		break;
	case	IAV_IOC_IMG_VIGNETTE_COMPENSATION:
		rval = dsp_vignette_compensation(context, (struct vignette_compensation_info __user*)arg);
		break;
	case	IAV_IOC_IMG_LOCAL_EXPOSURE:
		rval = dsp_local_exposure(context, (struct local_exposure_info __user*)arg);
		break;
	case	IAV_IOC_IMG_COLOR_CORRECTION:
		rval = dsp_color_correction(context, (struct color_correction_info __user*)arg);
		break;
	case	IAV_IOC_IMG_RGB_TO_YUV_SETUP:
		rval = dsp_rgb_to_yuv_setup(context, (struct rgb_to_yuv_info __user*)arg);
		break;
	case	IAV_IOC_IMG_CHROMA_SCALE:
		rval = dsp_chroma_scale(context, (struct chroma_scale_info __user*)arg);
		break;
	case	IAV_IOC_IMG_CHROMA_MEDIAN_FILTER_SETUP:
		rval = dsp_chroma_median_filter(context, (struct chroma_median_filter __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_NOISE_FILTER_SETUP:
		rval = dsp_chroma_noise_filter(context, (struct chroma_noise_filter_info __user*) arg);
		break;
	case	IAV_IOC_IMG_LUMA_SHARPENING:
		rval = dsp_luma_sharpening(context, (struct luma_sharpening_info __user*)arg);
		break;
	case	IAV_IOC_IMG_LUMA_SHARPENING_FIR_CONFIG:
		rval = dsp_luma_sharp_fir(context, (struct luma_sharp_fir_info __user *)arg);
		break;
	case	IAV_IOC_IMG_LUMA_SHARPENING_BLEND_CONFIG:
		rval = dsp_luma_sharpen_blend_ctrl(context, (struct luma_sharp_blend_info __user*)arg);
		break;
	case	IAV_IOC_IMG_LUMA_SHARPENING_LEVEL_CONTROL:
		rval = dsp_luma_sharpen_level_ctrl(context, (struct luma_sharp_level_info __user*)arg);
		break;
	case	IAV_IOC_IMG_MCTF_GMV_SETUP:
		rval = dsp_mctf_gmv_setup(context, (struct mctf_gmv_info __user*)arg);
		break;
	case	IAV_IOC_IMG_MCTF_MV_STABILIZER_SETUP:
		rval = dsp_mctf_mv_stabilizer_setup(context, (struct mctf_mv_stab_info __user*)arg);
		break;
	case	IAV_IOC_IMG_RGB_GAIN_ADJUST:
		rval = dsp_rgb_gain_adjustment(context, (struct rgb_gain_info __user*)arg);
		break;
	case 	IAV_IOC_IMG_HDR_RGB_GAIN_ADJUST:
		rval = dsp_hdr_rgb_gain_adjustment(context, (struct rgb_gain_info __user*)arg);
		break;
	case	IAV_IOC_IMG_ANTI_ALIASING_CONFIG:
		rval = dsp_anti_aliasing_filter(context, (struct anti_aliasing_info __user*)arg);
		break;
	case	IAV_IOC_IMG_DIGITAL_SATURATION_LEVEL:
		rval = dsp_digital_gain_sat_level(context, (struct digital_gain_level __user *)arg);
		break;
	case	IAV_IOC_IMG_SET_ZOOM_FACTOR:
		rval = dsp_zoom_factor(context, (struct zoom_factor_info __user*)arg);
		break;
	case	IAV_IOC_IMG_DEMOSAIC_CONIFG:
		rval = dsp_demoasic_filter(context, (struct demoasic_filter_info __user *)arg);
		break;
	case	IAV_IOC_IMG_SENSOR_CONFIG:
		rval = dsp_sensor_input_setup(context, (struct sensor_info __user *)arg);
		break;
	case	IAV_IOC_IMG_STATIC_BAD_PIXEL_CORRECTION:
		rval = dsp_fixed_pattern_noise_correct(context, (struct fixed_pattern_correct __user *)arg);
		break;
	case	IAV_IOC_IMG_SET_BLACK_LVL_STATE_TABLE:
		rval = dsp_black_level_state_table(context, (struct black_level_state __user *)arg);
		break;
	case	IAV_IOC_IMG_LUMA_SHARPENING_LNL:
		rval = dsp_luma_sharp_lnl(context, (struct luma_sharp_lnl_info __user *)arg);
		break;
	case IAV_IOC_IMG_DUMP_IDSP_SEC:
		rval = img_dump_idsp(context,(iav_idsp_config_info_t __user*)arg);
		break;
	case IAV_IOC_IMG_CANCEL_GETTING_STATISTICS:
	        rval =img_statistics_ready();
	        break;
	case IAV_IOC_IMG_HDR_SET_VIDEO_PROC:
		rval = dsp_hdr_set_video_proc(context, (struct video_hdr_proc_control __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_START_CMD_BLOCK:
		rval = dsp_hdr_start_cmd_block(context, (u32)arg);
		break;
	case IAV_IOC_IMG_HDR_END_CMD_BLOCK:
		rval = dsp_hdr_end_cmd_block(context, (u32)arg);
		break;
	case IAV_IOC_IMG_HDR_BL_GLOBAL_OFFSET:
		rval = dsp_hdr_black_level_global_offset(context,
			(struct black_level_global_offset __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_CFA_NF:
		rval = dsp_hdr_cfa_noise_filter(context,
			(struct cfa_noise_filter_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_LE:
		rval = dsp_hdr_local_exposure(context,
			(struct local_exposure_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_MIXER:
		rval = dsp_hdr_mixer(context, (struct hdr_mixer_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_ALPHA_MAP:
		rval = dsp_hdr_alpha_map(context, (struct color_correction_info __user *) arg);
		break;
	case IAV_IOC_IMG_VCAP_NO_OP:
		rval = dsp_vcap_no_op(context, (struct vcap_no_op __user*)arg);
		break;
	case IAV_IOC_IMG_HDR_CC:
		rval = dsp_hdr_color_correction(context, (struct color_correction_info __user *) arg);
		break;
	case IAV_IOC_IMG_HDR_BPC:
		rval = dsp_hdr_bad_pixel_correct_setup(context, (struct bad_pixel_correct_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_ANTI_ALIASING:
		rval = dsp_hdr_anti_aliasing_filter(context, (struct anti_aliasing_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_CFA_LEAKAGE:
		rval = dsp_hdr_cfa_leakage_filter_setup(context, (struct cfa_leakage_filter_info __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_SET_ROW_OFFSET:
		rval = dsp_hdr_set_raw_offset(context, (struct video_hdr_proc_control __user *)arg);
		break;
	case IAV_IOC_IMG_HDR_YUV_CNTL_PARAM:
		rval = dsp_hdr_yuv_control_param(context, (struct video_hdr_yuv_cntl_param __user *)arg);
		break;

	//below is for new IK
	case IAV_IOC_IMG_CONFIG_FLOAT_STATISTICS_NI:
		rval = dsp_aaa_floating_statistics_setup(context, (struct aaa_floating_tile_config_info __user *) arg);
		break;
	case IAV_IOC_IMG_NOISE_FILTER_SETUP_NI:
		rval = dsp_noise_filter_setup(context, (u32)arg);
		break;
	case IAV_IOC_IMG_BLACK_LEVEL_GLOBAL_OFFSET_NI:
		rval = dsp_black_level_global_offset_ni(context, (black_level_global_offset_t __user*)arg);
		break;
	case IAV_IOC_IMG_BAD_PIXEL_CORRECTION_NI:
		rval = dsp_bad_pixel_correct_setup_ni(context, (bad_pixel_correct_setup_t __user*)arg);
		break;
	case IAV_IOC_IMG_CFA_LEAKAGE_FILTER_SETUP_NI:
		rval = dsp_cfa_domain_leakage_filter_setup_ni(context, (cfa_domain_leakage_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_CFA_NOISE_FILTER_SETUP_NI:
		rval = dsp_cfa_noise_filter_ni(context, (cfa_noise_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_RGB_GAIN_ADJUST_NI:
		rval = dsp_rgb_gain_adjustment_ni(context, (rgb_gain_adjust_t __user*)arg);
		break;
	case IAV_IOC_IMG_VIGNETTE_COMPENSATION_NI:
		rval = dsp_vignette_compensation_ni(context, (vignette_compensation_t __user*)arg);
		break;
	case IAV_IOC_IMG_LOCAL_EXPOSURE_NI:
		rval = dsp_local_exposure_ni(context, (local_exposure_t __user*)arg);
		break;
	case IAV_IOC_IMG_COLOR_CORRECTION_NI:
		rval = dsp_color_correction_ni(context, (color_correction_t __user*)arg);
		break;
	case IAV_IOC_IMG_RGB_TO_YUV_SETUP_NI:
		rval = dsp_rgb_to_yuv_setup_ni(context, (rgb_to_yuv_setup_t __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_SCALE_NI:
		rval = dsp_chroma_scale_ni(context, (chroma_scale_t __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_MEDIAN_FILTER_SETUP_NI:
		rval = dsp_chroma_median_filter_ni(context, (chroma_median_filter_info_t __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_NOISE_FILTER_SETUP_NI:
		rval = dsp_chroma_noise_filter_ni(context, (chroma_noise_filter_t __user*) arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_NI:
		rval = dsp_luma_sharpening_ni(context, (luma_sharpening_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_FIR_CONFIG_NI:
		rval = dsp_luma_sharp_fir_ni(context, (luma_sharpening_FIR_config_t __user *)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_BLEND_CNTL_NI:
		rval = dsp_luma_sharpen_blend_ctrl_ni(context, (luma_sharpening_blend_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_LEVEL_CONTROL_NI:
		rval = dsp_luma_sharpen_level_ctrl_ni(context, (luma_sharpening_level_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_TONE_NI:
		rval = dsp_luma_sharpen_tone_ni(context, (luma_sharpening_tone_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_LNL_NI:
		rval = dsp_luma_sharp_lnl_ni(context, (luma_sharpening_LNL_t __user *)arg);
		break;
	case IAV_IOC_IMG_MCTF_GMV_SETUP_NI:
		rval = dsp_mctf_gmv_setup_ni(context, (VCAP_MCTF_GMV_CMD __user*)arg);
		break;
	case IAV_IOC_IMG_MCTF_MV_STABILIZER_SETUP_NI:
		rval = dsp_mctf_mv_stabilizer_setup_ni(context, (VCAP_MCTF_MV_STAB_CMD __user*)arg);
		break;
	case IAV_IOC_IMG_AAA_SETUP_EX_NI:
		rval = dsp_aaa_statistics_ex(context, (struct aaa_statistics_ex __user*)arg);
		break;
	case IAV_IOC_IMG_ANTI_ALIASING_CONFIG_NI:
		rval = dsp_anti_aliasing_filter_ni(context, (anti_aliasing_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_DIGITAL_SATURATION_LEVEL_NI:
		rval = dsp_digital_gain_sat_level_ni(context, (digital_gain_level_t __user *)arg);
		break;
	case IAV_IOC_IMG_DEMOSAIC_CONIFG_NI:
		rval = dsp_demoasic_filter_ni(context, (demoasic_filter_t __user *)arg);
		break;
	case IAV_IOC_IMG_SENSOR_CONFIG_NI:
		rval = dsp_sensor_input_setup_ni(context, (sensor_input_setup_t __user *)arg);
		break;
	case IAV_IOC_IMG_STATIC_BAD_PIXEL_CORRECTION_NI:
		rval = dsp_fixed_pattern_noise_correct_ni(context, (fixed_pattern_noise_correct_t __user *)arg);
		break;
	case IAV_IOC_IMG_SET_BLACK_LVL_STATE_TABLE_NI:
		break;
	case IAV_IOC_IMG_GET_PHY_START_NI:
		put_user(phys_start, (u32*) arg);
		rval = 0;
		break;
	case IAV_IOC_IMG_SET_USER_START_NI:
		user_start = (u32)arg;
		rval = 0;
		break;
	case IAV_IOC_IMG_RESET_RAW2ENC_NI:
		rval = img_reset_raw2enc(context);
		break;
	case IAV_IOC_IMG_WAIT_RAW2ENC_NI:
		rval = img_wait_raw2enc(context);
		break;
	case IAV_IOC_IMG_FEED_RAW_NI:
		rval = img_feed_raw(context,(struct raw2enc_raw_feed_info __user *)arg);
		break;
	case IAV_IOC_IMG_SET_HISO_CFG_NI:
		rval = dsp_set_hiso_cfg_ni(context, (u8 __user *)arg);
		break;
	case IAV_IOC_IMG_UPDATE_HISO_CFG_NI:
		rval = dsp_update_hiso_cfg_ni(context, (VCAP_HISO_CONFIG_UPDATE_CMD __user*) arg);
		break;
	case IAV_IOC_IMG_CONFIG_STATISTICS_NI:
		rval = dsp_aaa_statistics_setup_ni(context, (aaa_statistics_setup_t __user *)arg);
		break;
	case IAV_IOC_IMG_STRONG_GRGB_MISMATCH_NI:
		rval = dsp_strong_grgb_mismatch_filter_ni(context,(strong_grgb_mismatch_filter_t __user *)arg);
		break;
	case IAV_IOC_IMG_WAIT_ENTER_PREVIEW:
		rval = img_check_dsp_mode(context);
		break;
	case IAV_IOC_IMG_HISO_TIMING:
		printk("in\n");
		rval = 0;
	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}
EXPORT_SYMBOL(amba_imgproc_cmd);

int amba_imgproc_msg(VCAP_STRM_REPORT * str_rpt, VCAP_SETUP_MODE mode)
{
#ifdef BUILD_AMBARELLA_IMGPROC_DRV
	int ptr_updated = 0;
	int phys_updated = 0;

	g_dsp_mode = mode;
	wake_up_interruptible(&g_dsp_wq);

	if((str_rpt->thumbnail_luma_addr != 0) &&
		(str_rpt->thumbnail_chroma_addr != 0) &&
		(str_rpt->screennail_luma_addr != 0) &&
		(str_rpt->screennail_chroma_addr) != 0){
		cfa_aaa_phys = DSP_TO_PHYS(str_rpt->thumbnail_luma_addr);
		rgb_aaa_phys = DSP_TO_PHYS(str_rpt->screennail_luma_addr);
		cfa_aaa_size = str_rpt->thumbnail_chroma_addr - str_rpt->thumbnail_luma_addr;
		rgb_aaa_size = str_rpt->screennail_chroma_addr - str_rpt->screennail_luma_addr;
		//img_set_cfa_start_ptr((void*)str_rpt->thumbnail_luma_addr);
		//img_set_cfa_end_ptr((void*)str_rpt->thumbnail_chroma_addr);
		//img_set_rgb_start_ptr((void*)str_rpt->screennail_luma_addr);
		//img_set_rgb_end_ptr((void*)str_rpt->screennail_chroma_addr);
		phys_updated = 1;
	} else {
		phys_updated = 0;
	}

	if ((str_rpt->cfa_3a_stat_dram_addr != 0) &&
		(str_rpt->yuv_3a_stat_dram_addr != 0) &&
		phys_updated) {
		img_set_cfa_ptr((void*)(str_rpt->cfa_3a_stat_dram_addr));
		img_set_rgb_ptr((void*)(str_rpt->yuv_3a_stat_dram_addr));
		img_set_hist_ptr((void*)(str_rpt->raw_capture_dram_addr),
			str_rpt->raw_cap_buf_pitch);
		ptr_updated = 1;
	} else {
		ptr_updated = 0;
	}

	if (ptr_updated && phys_updated) {
		img_statistics_ready();
		img_count_proc_num(str_rpt->raw_capture_dram_addr);
	}
#endif

	return 0;
}
EXPORT_SYMBOL(amba_imgproc_msg);

int img_set_phys_start_ptr(u32 input)
{
//	printk("set phys cfg 0x%x\n", input);
	phys_start = input;
	return 0;
}
EXPORT_SYMBOL(img_set_phys_start_ptr);

int img_need_remap(int remap)
{
	need_remap = remap;
	return 0;
}
EXPORT_SYMBOL(img_need_remap);

module_init(img_init);
module_exit(img_exit);


