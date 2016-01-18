#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "types.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#ifdef BUILD_AMBARELLA_EIS
#include "ambas_eis.h"
#endif
#include "ambas_vin.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"

#include "ar0331_adj_param.c"
#include "ar0331_aeb_param.c"

#include "mn34220pl_adj_param.c"
#include "mn34220pl_aeb_param.c"

#include "imx185_adj_param.c"
#include "imx185_aeb_param.c"

#include "imx226_adj_param.c"
#include "imx226_aeb_param.c"

#include "ov5658_adj_param.c"
#include "ov5658_aeb_param.c"

#define TUNING_printf(...)
#define NO_ARG	0
#define HAS_ARG	1
#define UNIT_chroma_scale (64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#define CHECK_RVAL do{if(rval<0)	{\
	printf("error: %s,%s:%d\n",__FILE__,__func__,__LINE__);\
	return -1;}}while(0)

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

// option definition
typedef enum{
	SHARPEN_B_FILTER = 11,
	CALC_BLC,
	MCTF_GHOST,
	WARP_COMPENSATION,
	IDSP_DUMP,
	CHROMA_NOISE_FILTERING,
	CHROMA_NOISE_FILTER_STR,
	MANUAL_CONTRAST,
	AUTO_CONTRAST,
	AUTO_CONTRAST_STR,
	FPN_DETECT,
	FPN_LOAD,
	LENS_SHADING,
	SHP_LEVEL,
	MCTF_STRENGTH,
	YUV_CONTRAST_ENA,
	YUV_CONTRAST_STR,
	SPACIAL_DENOISE_STR,
	MULTI_VIN_ENA,
	WDR_ENA,
	WDR_STRENGTH,
}idsp_test_options;

static const char* short_options = "m:s:t:aA:S:Wp:d:L:C:M:H:P:y:f:Bb:Vq:R:";
static struct option long_options[] = {
	{"cc", HAS_ARG, 0, 'm'},
	{"chroma_scale", HAS_ARG, 0, 's'},
	{"3a", NO_ARG, 0, 'a'},
	{"img_work_mode", NO_ARG, 0, 'W'},
	{"agc", HAS_ARG, 0, 'A'},
	{"shutter", HAS_ARG, 0, 'S'},
	{"dbp", HAS_ARG, 0, 'd'},
	{"anti_aliasing", HAS_ARG, 0, 'L'},
	{"cfa_filter", HAS_ARG, 0, 'C'},
	{"local_exposure", HAS_ARG, 0, 'P'},
	{"chroma_median_filter", HAS_ARG, 0, 'M'},
	{"high_freq_noise_reduct", HAS_ARG, 0, 'H'},
	{"rgb2yuv", HAS_ARG, 0, 'y'},
	{"cfa_leakage", HAS_ARG, 0, 'f'},
	{"sharpening_fir", NO_ARG, 0, 'B'},
	{"coring_table", HAS_ARG, 0, 'b'},
	{"vignette_compensation", NO_ARG, 0, 'V'},
	{"mqueue", HAS_ARG, 0, 'q'},
	{"raw-proc",HAS_ARG,0,'R'},
	{"fpn",HAS_ARG,0,FPN_DETECT},
	{"loadfpn",HAS_ARG,0,FPN_LOAD},
	{"lens",HAS_ARG,0,LENS_SHADING},
	{"warp_comp",HAS_ARG, 0, WARP_COMPENSATION},
	{"cal-blc",HAS_ARG, 0, CALC_BLC},
	{"ghost", HAS_ARG, 0 , MCTF_GHOST},
	{"chroma_nf", HAS_ARG, 0, CHROMA_NOISE_FILTERING},
	{"cnf-strength", HAS_ARG, 0, CHROMA_NOISE_FILTER_STR},
	{"idsp-dump", HAS_ARG, 0, IDSP_DUMP},
	{"manual-contrast", HAS_ARG, 0, MANUAL_CONTRAST},
	{"auto-contrast", HAS_ARG, 0, AUTO_CONTRAST},
	{"auto-contrast-strength", HAS_ARG, 0, AUTO_CONTRAST_STR},
	{"sharp-level", HAS_ARG, 0, SHP_LEVEL},
	{"mctf-strength", HAS_ARG, 0, MCTF_STRENGTH},
	{"yuv-contrast-ena", HAS_ARG, 0, YUV_CONTRAST_ENA},
	{"yuv-contrast-str", HAS_ARG, 0, YUV_CONTRAST_STR},
	{"spacial-denoise-str", HAS_ARG, 0, SPACIAL_DENOISE_STR},
	{"multi-vin-ena", HAS_ARG, 0, MULTI_VIN_ENA},
	{"light-optimise-ena", HAS_ARG, 0, WDR_ENA},
	{"light-optimise-str", HAS_ARG, 0,WDR_STRENGTH},
	{0, 0, 0, 0},
};

u8 wdr_ena_flag = 0, wdr_enable = 0;
u8 wdr_str_flag = 0, wdr_strength = 0;
u8 multi_vin_flag = 0, multi_vin_ena = 0;
u8 spacial_dn_flag = 0;
int spacial_dn_strength = 0;
u8 yuv_contrast_ena_flg = 0, yuv_contrast_ena = 0;
u16 yuv_contrast_str_flg = 0, yuv_contrast_str = 64;
u8 manual_contrast_flg = 0, manual_contrast_str = 64;
u8 auto_contrast_flg = 0, auto_contrast_ena = 0;
u8 auto_contrast_str_flg = 0, auto_contrast_str = 64;
u8 idsp_dump_flg = 0, idsp_dump_id;
u8 cnf_strength_flag = 0;
int cnf_strength = 64;
u8 chroma_noise_filter_flg = 0;
int chroma_nf_input_param[9];
char pic_file_name[64] = "img";
u8 img_work_mode_flag = 0;
u8 mctf_flag, mctf_strength;
u8 start_aaa;
u8 chroma_scale_flag;
u8 cfa_filter_flag;
u8 cc_flag;
u8 anti_aliasing_flag,anti_aliasing_strength;
u8 agc_flag, shutter_flag;
u32 dbp_flag;
u32 chroma_scale;
u32 sharpen_flag,sharpen_level;
u32 shutter_index, agc_index;
u8 local_exposure_flag;
u16 chroma_median_flag,high_freq_flag,rgb2yuv_flag;
u16 cfa_leakage_flag, sharpening_fir_flag,coring_table_flag;
u16 vignette_compensation_flag,cal_lens_shading,flicker_mode;
u8 warp_compensation_flag;
char warp_raw_file[64] = "warp.raw";
u16 msg_send_flag;
int msg_param;
u8 raw_proc_flag;
static u8 cal_blc_flag;
static char blc_file_name[64];
u8 detect_fpn_flag;
u8 load_fpn_flag;
static int fpn_param[10];
static int load_fpn_param[2];
static cali_badpix_setup_t badpixel_detect_algo;
static char fpn_map_bin[32] = "bpcmap.bin";
static fpn_correction_t fpn;
static cfa_noise_filter_info_t cfa_noise_filter;
static int cfa_noise_filter_param[5];
static dbp_correction_t bad_corr;
static int bad_corr_param[3];
static chroma_median_filter_t chroma_median_setup;
static int chroma_median_param[3];
u8 coring_strength;
static luma_high_freq_nr_t luma_high_freq_noise_reduction_strength;
static int luma_high_freq_param[2];
static rgb_to_yuv_t rgb2yuv_matrix;
static int rgb2yuv_param[9];
static image_sensor_param_t app_param_image_sensor;
static sensor_config_t sensor_config_info;
static cfa_leakage_filter_t cfa_leakage_filter;
static vignette_info_t vignette_info = {0};
u8 *bsb_mem;
u32 bsb_size;
static u8 ghost_prv_flag, ghost_on;
static int vin_op_mode = AMBA_VIN_LINEAR_MODE;	//VIN is linear or HDR
extern void hist_equalilze(u16 * p_raw, u16 width, u16 height);

static local_exposure_t local_exposure = { 1, 4, 16, 16, 16, 6,
	{1024,1054,1140,1320,1460,1538,1580,1610,1630,1640,1640,1635,1625,1610,1592,1563,
	1534,1505,1475,1447,1417,1393,1369,1345,1321,1297,1273,1256,1238,1226,1214,1203,
	1192,1180,1168,1157,1149,1142,1135,1127,1121,1113,1106,1098,1091,1084,1080,1077,
	1074,1071,1067,1065,1061,1058,1055,1051,1048,1045,1044,1043,1042,1041,1040,1039,
	1038,1037,1036,1035,1034,1033,1032,1031,1030,1029,1029,1029,1029,1028,1028,1028,
	1028,1027,1027,1027,1026,1026,1026,1026,1025,1025,1025,1025,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}};
tone_curve_t tone_curve = {
	{/* red */
	0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
	64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
	128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
	193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
	257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
	321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
	385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
	449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
	514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
	578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
	642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
	706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
	770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
	834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
	899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
	963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
	{/* green */
	0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
	64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
	128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
	193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
	257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
	321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
	385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
	449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
	514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
	578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
	642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
	706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
	770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
	834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
	899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
	963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
	{/* blue */
	0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
	64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
	128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
	193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
	257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
	321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
	385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
	449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
	514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
	578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
	642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
	706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
	770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
	834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
	899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
	963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
};
static coring_table_t coring = {{
          18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 12, 8, 6, 6, 4, 4,
	    4, 4, 6, 6, 8, 12, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
          18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18}};

u16 gain_curve[NUM_CHROMA_GAIN_CURVE] = {256, 299, 342, 385, 428, 471, 514, 557, 600, 643, 686, 729, 772, 815, 858, 901,
	 936, 970, 990,1012,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1012, 996, 956, 916, 856, 796, 736, 676, 616, 556, 496, 436, 376, 316, 256};

static chroma_filter_t chroma_noise_filter_param = {1,{255, 255},{0, 0},{200, 200},50,32};
u16 warp_horizontal_table[32*48];
u16 warp_vertical_table[32*48];

int fd_iav;

char bin_fn[64][64]= {
	"default_MCTF.bin",
	"S2_cmpr_config.bin",
	"lowiso_MCTF.bin",
	"high_iso/01_mctf_level_based.bin",
	"high_iso/02_mctf_vlf_adapt.bin",
	"high_iso/03_mctf_l4c_edge.bin",
	"high_iso/04_mctf_mix_124x.bin",
	"high_iso/05_mctf_mix_main.bin",
	"high_iso/06_mctf_c4.bin",
	"high_iso/07_mctf_c8.bin",
	"high_iso/08_mctf_cA.bin",
	"high_iso/09_mctf_vvlf_adapt.bin",
	"high_iso/10_sec_cc_l4c_edge.bin",
	"high_iso/11_cc_reg_normal.bin",
	"high_iso/12_cc_3d_normal.bin",
	"high_iso/13_cc_out_normal.bin",
	"high_iso/14_thresh_dark_normal.bin",
	"high_iso/15_thresh_hot_normal.bin",
	"high_iso/16_fir1_normal.bin",
	"high_iso/17_fir2_normal.bin",
	"high_iso/18_coring_normal.bin",
	"high_iso/19_lnl_tone_normal.bin",
	"high_iso/20_chroma_scale_gain_normal.bin",
	"high_iso/21_local_exposure_gain_normal.bin",
	"high_iso/22_cmf_table_normal.bin",
	"high_iso/23_alpha_luma.bin",
	"high_iso/24_alpha_chroma.bin",
	"high_iso/25_cc_reg.bin",
	"high_iso/26_cc_3d.bin",
	"high_iso/27_fir1_spat_dep.bin",
	"high_iso/28_fir2_spat_dep.bin",
	"high_iso/29_coring_spat_dep.bin",
	"high_iso/30_fir1_vlf.bin",
	"high_iso/31_fir2_vlf.bin",
	"high_iso/32_coring_vlf.bin",
	"high_iso/33_fir1_nrl.bin",
	"high_iso/34_fir2_nrl.bin",
	"high_iso/35_coring_nrl.bin",
	"high_iso/36_fir1_124x.bin",
	"high_iso/37_coring_coring4x.bin",
	"high_iso/38_coring_coring2x.bin",
	"high_iso/39_coring_coring1x.bin",
	"high_iso/40_fir1_high_noise.bin",
	"high_iso/41_coring_high_noise.bin",
	"high_iso/42_alpha_unity.bin",
	"high_iso/43_zero.bin",
	"high_iso/46_mctf_1.bin",
	"high_iso/47_mctf_2.bin",
	"high_iso/48_mctf_3.bin",
	"high_iso/49_mctf_4.bin",
	"high_iso/50_mctf_5.bin",
	"high_iso/51_mctf_6.bin",
	"high_iso/52_mctf_7.bin",
	"high_iso/53_mctf_8.bin"
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"value", "\t\t\tload color correction"},
	{"64 unit", "\tchroma scale"},
	{"", "\t\t\t\tturn 3a on"},
	{"", "\t\twork mode"},
	{"value", "\t\tsensor gain index"},
	{"value", "\t\tsensor e-shutter speed index"},
	{"value", "\t\tdynamic bad pixel strength"},
	{"0~3", "\tanti aliasing filter strength"},
	{"enable, weight, weight, weight, threshold", "\tcfa filter noise filter"},
	{"0|1", "\tlocal exposure enable"},
	{"0~255", "chroma median filter strength"},
	{"0~255  coarse, fine", "high frequncy noise reduction filter strength"},
	{"value", "\t\tload rgb2yuv matrix"},
	{"value", "\tcfa leakage filter"},
	{"", "\t\tconfig sharpening fir"},
	{"unavailable", "\tload coring table"},
	{"unavailable", "\t\tconfig vignette compensation"},
	{"", "\t\t\tmessage queue"},
	{"raw file name","\traw processing from memory"},
	{"","\t\t\tstatic bad pixel maping"},
	{"","\t\t\tload bad pixel data"},
	{"","\t\t\tstatic lens shading calibration."},
	{"raw filename","\twarp compensation."},
	{"raw filename","\tBLC calculation."},
	{"","\t\t\tMCTF ghost setting."},
	{"","\t\t\tChroma noise filter setting."},
	{"0~256", "\tChroma noise filter strength"},
	{"1~7,100","\tIDSP dump information."},
	{"0~128", "\tmanual contrast strength"},
	{"0|1", "\tauto contrast enable"},
	{"0~128", "auto contrast strength"},
	{"0~11", "\t\tsharpness level"},
	{"0~11", "\tmctf strength"},
	{"0|1", "\tyuv contrast on/off"},
	{"0~128", "\tyuv contrast strength"},
	{"0~10", "\tspacial denoise strength"},
	{"0|1", "\tmulti-CFA vin option"},
	{"0|1", "\tlighting optimiser on/off"},
	{"0~128", "\tlighting optimise strength"},
	{0, 0},
};

void usage(void){
	int i;

	printf("test_idsp usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

static void sigstop()
{
	img_stop_aaa();
	printf("3A is off.\n");
	exit(1);
}

int load_dsp_cc_table(int fd_iav)
{
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8* reg, *matrix, *sec_cc;
	char filename[128];
	int file, count;
	int rval = 0;

	reg = malloc(CC_REG_SIZE);
	matrix = malloc(CC_3D_SIZE);
	sec_cc = malloc(SEC_CC_SIZE);
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("reg.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("3D.bin cannot be opened\n");
		return -1;
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	close(file);

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		rval = -1;
		goto load_dsp_cc_table_exit;
	}

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;

	rval = img_dsp_set_color_correction_reg(&color_corr_reg);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_color_correction(fd_iav, &color_corr);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_tone_curve(fd_iav, &tone_curve);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_set_sec_cc_en(fd_iav, 0);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

	rval = img_dsp_enable_color_correction(fd_iav);
	if (rval < 0) {
		goto load_dsp_cc_table_exit;
	}

load_dsp_cc_table_exit:
	if (reg != NULL) {
		free(reg);
		reg = NULL;
	}
	if (matrix != NULL) {
		free(matrix);
		matrix = NULL;
	}
	if (sec_cc != NULL) {
		free(sec_cc);
		sec_cc = NULL;
	}

	if (file >= 0) {
		close(file);
		file = -1;
	}
	return rval;
}

int load_adj_cc_table(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 *matrix;
	u8 i, adj_mode = 4;

	matrix = malloc(CC_3D_SIZE);

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("3D.bin cannot be opened\n");
			free(matrix);
			return -1;
		}
		if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			printf("read %s error\n",filename);
			free(matrix);
			close(file);
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}
	free(matrix);
	return 0;
}

int map_bsb(int fd_iav)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB2");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}

int start_aaa_foo(int fd_iav, char* sensor_name, image_sensor_param_t app_param_is)
{
	aaa_api_t custom_aaa_api = {0};
	int rval = -1;

	rval = load_dsp_cc_table(fd_iav);
	CHECK_RVAL;
	rval = load_adj_cc_table(sensor_name);
	CHECK_RVAL;
	rval = img_load_image_sensor_param(&app_param_is);
	CHECK_RVAL;
	rval = img_register_aaa_algorithm(custom_aaa_api);
	CHECK_RVAL;
	rval = img_start_aaa(fd_iav);
	CHECK_RVAL;
	rval = img_set_work_mode(0);
	CHECK_RVAL;
	rval = img_dsp_set_video_mctf_compression_enable(0);
	CHECK_RVAL;
	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);
	img_set_chroma_noise_filter_max_radius(32<<resource_setup.max_chroma_noise_shift);
	return 0;
}

static int get_multi_arg(char *optarg, int *argarr, int argcnt)
{
	int i;
	char *delim = ",:-";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;

	return 0;
}


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	int i;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case	'm':
			cc_flag = 1;
			break;
		case	's':
			chroma_scale_flag = 1;
			chroma_scale = atoi(optarg);
			break;
		case	'a':
			start_aaa = 1;
			break;
		case 'W':
			img_work_mode_flag = 1;
			break;
		case	'A':
			agc_flag = 1;
			agc_index= atoi(optarg);
			break;
		case	'S':
			shutter_flag = 1;
			shutter_index = atoi(optarg);
			break;
		case	'd':
			dbp_flag = 1;
			if (get_multi_arg(optarg, bad_corr_param, ARRAY_SIZE(bad_corr_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(bad_corr_param), ch);
				break;
			}
			bad_corr.enable = bad_corr_param[0];
			bad_corr.dark_pixel_strength = bad_corr_param[1];
			bad_corr.hot_pixel_strength = bad_corr_param[2];
			break;
		case	'L':
			anti_aliasing_flag = 1;
			anti_aliasing_strength = atoi(optarg);
			break;
		case	'C':
			cfa_filter_flag = 1;
			if (get_multi_arg(optarg, cfa_noise_filter_param, ARRAY_SIZE(cfa_noise_filter_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(cfa_noise_filter_param), ch);
				break;
			}
			cfa_noise_filter.enable = cfa_noise_filter_param[0];
			cfa_noise_filter.noise_level[2] = cfa_noise_filter_param[1];// R/G/B, 0-8192
			cfa_noise_filter.original_blend_str[2] = cfa_noise_filter_param[2];// R/G/B, 0-256
			cfa_noise_filter.extent_regular[2] = cfa_noise_filter_param[3];// R/G/B, 0-256
			cfa_noise_filter.extent_fine[2] = cfa_noise_filter_param[4];// R/G/B, 0-256
			cfa_noise_filter.strength_fine[2] = cfa_noise_filter_param[4];// R/G/B, 0-256
			cfa_noise_filter.selectivity_regular = cfa_noise_filter_param[4];// 0-256
			cfa_noise_filter.selectivity_fine = cfa_noise_filter_param[4];// 0-256
			break;
		case	'P':
			local_exposure_flag = 1;
			local_exposure.enable = atoi(optarg);
			break;
		case	'M':
			if (get_multi_arg(optarg, chroma_median_param, ARRAY_SIZE(chroma_median_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(chroma_median_param), ch);
				break;
			}
			if((chroma_median_param[1]<256)&&(chroma_median_param[2]<256)){
				chroma_median_setup.enable = chroma_median_param[0];
				chroma_median_setup.cb_str = chroma_median_param[1];
				chroma_median_setup.cr_str = chroma_median_param[2];
				chroma_median_flag = 1;
			}else{
				printf("chroma_median_strength exceed 256!\n");
				return -1;
			}
			break;
		case CHROMA_NOISE_FILTERING:
			chroma_noise_filter_flg = 1;
			if (get_multi_arg(optarg, chroma_nf_input_param, ARRAY_SIZE(chroma_nf_input_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(chroma_nf_input_param), ch);
				break;
			}
			chroma_noise_filter_param.enable = chroma_nf_input_param[0];
			chroma_noise_filter_param.noise_level[0] = chroma_nf_input_param[1];
			chroma_noise_filter_param.noise_level[1] = chroma_nf_input_param[2];
			chroma_noise_filter_param.original_blend_str[0] = chroma_nf_input_param[3];
			chroma_noise_filter_param.original_blend_str[1] = chroma_nf_input_param[4];
			chroma_noise_filter_param.extent_fine[0] = chroma_nf_input_param[5];
			chroma_noise_filter_param.extent_fine[1] = chroma_nf_input_param[6];
			chroma_noise_filter_param.selectivity_fine = chroma_nf_input_param[7];
			chroma_noise_filter_param.radius= chroma_nf_input_param[8];
			break;
		case CHROMA_NOISE_FILTER_STR:
			cnf_strength_flag = 1;
			cnf_strength = atoi(optarg);
			break;
		case IDSP_DUMP:
			idsp_dump_flg = 1;
			idsp_dump_id = atoi(optarg);
			break;
		case MANUAL_CONTRAST:
			manual_contrast_flg = 1;
			manual_contrast_str = atoi(optarg);
			break;
		case AUTO_CONTRAST:
			auto_contrast_flg = 1;
			auto_contrast_ena = atoi(optarg);
			break;
		case AUTO_CONTRAST_STR:
			auto_contrast_str_flg = 1;
			auto_contrast_str = atoi(optarg);
			break;
		case	'H':
			high_freq_flag = 1;
			if(get_multi_arg(optarg,luma_high_freq_param,ARRAY_SIZE(luma_high_freq_param))<0){
				printf("need %d args for opt %c!\n",ARRAY_SIZE(luma_high_freq_param),ch);
				break;
			}
			luma_high_freq_noise_reduction_strength.strength_coarse = luma_high_freq_param[0];
			luma_high_freq_noise_reduction_strength.strength_fine = luma_high_freq_param[1];
			break;
		case	'y':
			rgb2yuv_flag = 1;
			if (get_multi_arg(optarg, rgb2yuv_param, ARRAY_SIZE(rgb2yuv_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(rgb2yuv_param), ch);
				break;
			}
			for(i = 0;i<9;i++)
				rgb2yuv_matrix.matrix_values[i] = rgb2yuv_param[i];
			break;
		case	'f':
			cfa_leakage_flag = 1;
			cfa_leakage_filter.enable = atoi(optarg);
			break;
		case 'B':
			sharpening_fir_flag = 1;
			break;
		case	'b':
			coring_table_flag = 1;
			coring_strength = atoi(optarg);
			break;
		case 'V':
			vignette_compensation_flag = 1;
			break;
		case	WARP_COMPENSATION:
			warp_compensation_flag = 1;
			strcpy(warp_raw_file, optarg);
			break;
		case	'q':
			msg_send_flag = 1;
			msg_param = atoi(optarg);
			break;
		case	'R':
			raw_proc_flag = 1;
			strcpy(pic_file_name, optarg);
			break;
		case	FPN_DETECT:
			detect_fpn_flag = 1;
			if (get_multi_arg(optarg, fpn_param, ARRAY_SIZE(fpn_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(fpn_param), ch);
				break;
			}
			badpixel_detect_algo.cap_width = fpn_param[0];
			badpixel_detect_algo.cap_height= fpn_param[1];
			badpixel_detect_algo.block_w= fpn_param[2];
			badpixel_detect_algo.block_h = fpn_param[3];
			badpixel_detect_algo.badpix_type = fpn_param[4];
			badpixel_detect_algo.upper_thres= fpn_param[5];
			badpixel_detect_algo.lower_thres= fpn_param[6];
			badpixel_detect_algo.detect_times = fpn_param[7];
			badpixel_detect_algo.agc_idx = fpn_param[8];
			badpixel_detect_algo.shutter_idx= fpn_param[9];
			break;
		case	FPN_LOAD:
			load_fpn_flag = 1;
			if (get_multi_arg(optarg, load_fpn_param, ARRAY_SIZE(load_fpn_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(load_fpn_param), ch);
				break;
			}
			break;
		case	LENS_SHADING:
			cal_lens_shading = 1;
			flicker_mode = atoi(optarg);
			break;
		case	SHARPEN_B_FILTER:
			break;
		case	CALC_BLC:
			cal_blc_flag = 1;
			memcpy(blc_file_name, optarg, 64);
			break;
		case	MCTF_GHOST:
			ghost_prv_flag = 1;
			ghost_on = atoi(optarg);
			break;
		case SHP_LEVEL:
			sharpen_flag = 1;
			sharpen_level = atoi(optarg);
			break;
		case MCTF_STRENGTH:
			mctf_flag = 1;
			mctf_strength = atoi(optarg);
			break;
		case YUV_CONTRAST_ENA:
			yuv_contrast_ena_flg = 1;
			yuv_contrast_ena = atoi(optarg);
			break;
		case YUV_CONTRAST_STR:
			yuv_contrast_str_flg = 1;
			yuv_contrast_str = atoi(optarg);
			break;
		case SPACIAL_DENOISE_STR:
			spacial_dn_flag = 1;
			spacial_dn_strength = atoi(optarg);
			break;
		case MULTI_VIN_ENA:
			multi_vin_flag = 1;
			multi_vin_ena = atoi(optarg);
			break;
		case WDR_ENA:
			wdr_ena_flag = 1;
			wdr_enable = atoi(optarg);
			break;
		case WDR_STRENGTH:
			wdr_str_flag = 1;
			wdr_strength = atoi(optarg);
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static int save_raw(void){
	int	fd_raw;
	iav_raw_info_t		raw_info;
	char file_name[256];

	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		return -1;
	}

	printf("raw_addr = %p\n", raw_info.raw_addr);
	printf("resolution: %dx%d\n", raw_info.width, raw_info.height);

	sprintf(file_name, "%s.raw", pic_file_name);
	fd_raw = open(file_name, O_WRONLY | O_CREAT, 0666);
	if (write(fd_raw, raw_info.raw_addr, raw_info.width * raw_info.height * 2) < 0) {
		perror("write(save_raw)");
		close(fd_raw);
		return -1;
	}

	printf("raw picture written to %s\n", file_name);
	close(fd_raw);
	return 0;
}

int load_mctf_bin()
{
	int rval = 0;
	int i;
	int file = -1, count;
	u8* bin_buff = NULL;
	char filename[256];
	idsp_one_def_bin_t one_bin;
	idsp_def_bin_t bin_map;
#define MCTF_BIN_SIZE	27600

	img_dsp_get_default_bin_map(&bin_map);
	bin_buff = malloc(MCTF_BIN_SIZE);
	for (i = 0; i < bin_map.num; i++) {
		memset(filename, 0, sizeof(filename));
		memset(bin_buff, 0, MCTF_BIN_SIZE);
		sprintf(filename, "%s/%s", IMGPROC_PARAM_PATH, bin_fn[i]);
		if((file = open(filename, O_RDONLY, 0))<0) {
			printf("%s cannot be opened\n",filename);
			free(bin_buff);
			return -1;
		}
		if((count = read(file, bin_buff, bin_map.one_bin[i].size)) != bin_map.one_bin[i].size) {
			printf("read %s error\n",filename);
			close(file);
			free(bin_buff);
			return -1;
		}
		one_bin.size = bin_map.one_bin[i].size;
		one_bin.type = bin_map.one_bin[i].type;
		rval = img_dsp_load_default_bin((u32)bin_buff, one_bin);
		if (rval < 0) {
			free(bin_buff);
			close(file);
			return -1;
		}
		close(file);
	}

	if (bin_buff != NULL) {
		free(bin_buff);
		bin_buff = NULL;
	}

	return 0;
}

inline int get_vin_frame_rate(u32 *pFrame_time)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}
	return 0;
}

inline int get_system_sharpen_filter_config(iav_sharpen_filter_cfg_t *sharpen_filter)
{
	if (ioctl(fd_iav, IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX, sharpen_filter) < 0) {
		perror("IAV_IOC_GET_SHARPEN_FILTER_CONFIG_EX");
		return -1;
	}
	return 0;
}
inline int get_sensor_bayer_pattern(u8 *pattern)
{
	struct amba_video_info video_info;

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0){
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO error");
		return -1;
	}
	*pattern = video_info.pattern;
	return 0;
}

inline int get_sensor_step(u8 *step)
{
	amba_vin_agc_info_t sensor_db_info;

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_AGC_INFO, &sensor_db_info) < 0){
		perror("IAV_IOC_VIN_SRC_GET_AGC_INFO error");
		return -1;
	}
	*step = (u64)0x6000000/sensor_db_info.db_step;
	return 0;
}

int get_is_info(image_sensor_param_t* info, char* sensor_name, sensor_config_t* config)
{
	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}
	u8 pattern = 0;
	u8 step = 0;
	u32 vin_fps;
	iav_sharpen_filter_cfg_t sharpen_filter;
	get_vin_frame_rate(&vin_fps);
	get_system_sharpen_filter_config(&sharpen_filter);
	switch (vin_info.sensor_id) {
		case SENSOR_AR0331:
		{
			amba_vin_sensor_op_mode op_mode;
			if (ioctl(fd_iav, IAV_IOC_VIN_GET_OPERATION_MODE, &op_mode) < 0) {
				perror("IAV_IOC_VIN_GET_OPERATION_MODE");
				return -1;
			}
			if(op_mode == AMBA_VIN_LINEAR_MODE)
			{
				memcpy(config ,&ar0331_linear_sensor_config,sizeof(sensor_config_t));
				info->p_adj_param = &ar0331_linear_adj_param;
				vin_op_mode = 0;
				info->p_awb_param = &ar0331_linear_awb_param;
				sprintf(sensor_name, "ar0331_linear");
			}
			else
			{
				printf("HDR MODE\n");
				memcpy(config ,&ar0331_sensor_config,sizeof(sensor_config_t));
				info->p_adj_param = &ar0331_adj_param;
				vin_op_mode = 1;	//HDR mode
				info->p_awb_param = &ar0331_awb_param;
				sprintf(sensor_name, "ar0331");
			}
			config->sensor_lb =AR_0331;
			info->p_rgb2yuv = ar0331_rgb2yuv;
			info->p_chroma_scale = &ar0331_chroma_scale;
			info->p_50hz_lines = ar0331_50hz_lines;
			info->p_60hz_lines = ar0331_60hz_lines;
			info->p_tile_config = &ar0331_tile_config;
			info->p_ae_agc_dgain = ar0331_ae_agc_dgain;
			info->p_ae_sht_dgain = ar0331_ae_sht_dgain;
			info->p_dlight_range = ar0331_dlight;
			info->p_manual_LE = ar0331_manual_LE;
			printf("AR0331\n");
			break;
		}
		case SENSOR_MN34220PL:
			info->p_adj_param = &mn34220pl_adj_param;
			info->p_rgb2yuv = mn34220pl_rgb2yuv;
			info->p_chroma_scale = &mn34220pl_chroma_scale;
			info->p_awb_param = &mn34220pl_awb_param;
			if(vin_fps ==AMBA_VIDEO_FPS_60){
				info->p_50hz_lines = mn34220pl_60p50hz_lines;
				info->p_60hz_lines = mn34220pl_60p60hz_lines;
			}else{
				info->p_50hz_lines = mn34220pl_50hz_lines;
				info->p_60hz_lines = mn34220pl_60hz_lines;
			}
			info->p_tile_config = &mn34220pl_tile_config;
			info->p_ae_agc_dgain = mn34220pl_ae_agc_dgain;
			info->p_ae_sht_dgain = mn34220pl_ae_sht_dgain;
			info->p_dlight_range = mn34220pl_dlight;
			info->p_manual_LE = mn34220pl_manual_LE;
			memcpy(config ,&mn34220pl_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =MN_34220PL;
			sprintf(sensor_name, "mn34220pl");
			break;
		case SENSOR_IMX185:
			info->p_adj_param = &imx185_adj_param;
			info->p_rgb2yuv = imx185_rgb2yuv;
			info->p_chroma_scale = &imx185_chroma_scale;
			info->p_awb_param = &imx185_awb_param;
			info->p_50hz_lines = imx185_50hz_lines;
			info->p_60hz_lines = imx185_60hz_lines;
			info->p_tile_config = &imx185_tile_config;
			info->p_ae_agc_dgain = imx185_ae_agc_dgain;
			info->p_ae_sht_dgain = imx185_ae_sht_dgain;
			info->p_dlight_range = imx185_dlight;
			info->p_manual_LE = imx185_manual_LE;
			memcpy(config ,&imx185_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_185;
			sprintf(sensor_name, "imx185");
			break;
		case SENSOR_IMX226:
			if(sharpen_filter.sharpen_b_enable)
				info->p_adj_param = &imx226_adj_param;
			else
				info->p_adj_param = &imx226_adj_param_shpA;
			info->p_rgb2yuv = imx226_rgb2yuv;
			info->p_chroma_scale = &imx226_chroma_scale;
			info->p_awb_param = &imx226_awb_param;
			info->p_50hz_lines = imx226_50hz_lines;
			info->p_60hz_lines = imx226_60hz_lines;
			info->p_tile_config = &imx226_tile_config;
			info->p_ae_agc_dgain = imx226_ae_agc_dgain;
			info->p_ae_sht_dgain = imx226_ae_sht_dgain;
			info->p_dlight_range = imx226_dlight;
			info->p_manual_LE = imx226_manual_LE;
			memcpy(config ,&imx226_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =IMX_226;
			sprintf(sensor_name, "imx226");
			break;
		case SENSOR_OV5658:
			info->p_adj_param = &ov5658_adj_param;
			info->p_rgb2yuv = ov5658_rgb2yuv;
			info->p_chroma_scale = &ov5658_chroma_scale;
			info->p_awb_param = &ov5658_awb_param;
			info->p_50hz_lines = ov5658_50hz_lines;
			info->p_60hz_lines = ov5658_60hz_lines;
			info->p_tile_config = &ov5658_tile_config;
			info->p_ae_agc_dgain = ov5658_ae_agc_dgain;
			info->p_ae_sht_dgain = ov5658_ae_sht_dgain;
			info->p_dlight_range = ov5658_dlight;
			info->p_manual_LE = ov5658_manual_LE;
			memcpy(config ,&ov5658_sensor_config,sizeof(sensor_config_t));
			config->sensor_lb =OV_5658;
			sprintf(sensor_name, "ov5658");
			break;
		default:
			printf("sensor Id [0x%x] is not supported.\n", vin_info.sensor_id);
			return -1;
	}

	get_sensor_bayer_pattern(&pattern);
	get_sensor_step(&step);

	config->pattern = (bayer_pattern)pattern;
	config->gain_step = step;
	return 0;
}

static int cal_blc(u16* raw_sum_buffer, u16 raw_width, u16 raw_height, blc_level_t* blc_offset)
{
	int  height_index, width_index;
	s32  gr_sum = 0, r_sum = 0, b_sum = 0, gb_sum = 0;
	s32  gr_sum_row, r_sum_row, b_sum_row, gb_sum_row;
	int    pix_index = 0;
	int    r_cnt = 0, b_cnt = 0, gr_cnt = 0, gb_cnt = 0;

	for(height_index = 0; height_index < raw_height; height_index++){
		gr_sum_row = 0;
		r_sum_row   = 0;
		b_sum_row  = 0;
		gb_sum_row = 0;
		for(width_index = 0; width_index < raw_width; width_index += 2){
			int tmp0, tmp1;

			pix_index = height_index* raw_width + width_index;
			// calculate gr, r, b, gb in expective row
			// Gb & B
			if((height_index +1)%2 == 1){
				tmp0 = *(raw_sum_buffer + pix_index);
				tmp1 = *(raw_sum_buffer + pix_index+1);
				if(tmp0 > 0 && tmp0 < 32 * 64)
					gb_sum_row = gb_sum_row + tmp0;
				if(tmp1 > 0 && tmp1 < 32 * 64)
					b_sum_row   = b_sum_row + tmp1;
				//printf("%d %d\n",tmp0,tmp1);
			}
			// R & Gr
			else{
				tmp0 = *(raw_sum_buffer + pix_index);
				tmp1 = *(raw_sum_buffer + pix_index+1);
				if(tmp0 > 0 && tmp0 < 32* 64)
					r_sum_row  = r_sum_row + tmp0;
				if(tmp1 > 0 && tmp1 < 32 * 64)
					gr_sum_row   = gr_sum_row + tmp1;
				//printf("%d %d\n",tmp0,tmp1);
			}
		}
		// average value in each row
		if(gb_sum_row != 0){
			gb_sum_row = gb_sum_row /(raw_width/2);
			gb_cnt++;
		}
		if(b_sum_row != 0){
			b_sum_row = b_sum_row /(raw_width/2);
			b_cnt++;
		}
		if(r_sum_row != 0){
			r_sum_row = r_sum_row/(raw_width/2);
			r_cnt++;
		}
		if(gr_sum_row != 0){
			gr_sum_row = gr_sum_row /(raw_width/2);
			gr_cnt++;
		}
		// sum
		gr_sum += gr_sum_row;
		r_sum   += r_sum_row;
		b_sum  += b_sum_row;
		gb_sum += gb_sum_row;
	}

	printf("%d %d %d %d\n%6d %6d %6d %6d\n",gr_sum,r_sum,b_sum,gb_sum,gr_cnt,r_cnt,b_cnt,gb_cnt);

	if (gr_cnt >0 && r_cnt >0 && b_cnt > 0 && gb_cnt >0){
		blc_offset->gr_offset = gr_sum / gr_cnt;
		blc_offset->r_offset   = r_sum  / r_cnt;
		blc_offset->b_offset  = b_sum / b_cnt;
		blc_offset->gb_offset = gb_sum / gb_cnt;
		return 0;
	}
	else
		return -1;

}

static void crop_raw(u16 *p_raw, int width, int height)
{
	int fd_croped_raw = -1;
	u16 *p_croped_raw = NULL;
	u8* src = (u8*)p_raw;
	int tmp;
	int left_boundary = 0, right_boundary = 0;
	int x_idx, y_idx = 0;
	char *croped_raw_name = "./warp_croped.raw";

	if(p_croped_raw == NULL){
		p_croped_raw = (u16*)malloc(height*height*sizeof(u16));
	}

	left_boundary = 0;
	right_boundary = left_boundary + height - 1;

	for(y_idx = 0; y_idx < height; y_idx ++){
		for(x_idx = left_boundary; x_idx <= right_boundary; x_idx++){
			tmp = *(src + y_idx * width + x_idx);
			if(tmp > 255){
				printf("index %d\n",y_idx * width + x_idx);
			}
			*(p_croped_raw + y_idx * height + (x_idx - left_boundary)) = tmp*(1<<6);
		}
	}

	hist_equalilze(p_croped_raw, height, height);

	if((fd_croped_raw = open(croped_raw_name, O_WRONLY | O_CREAT, 0666)) < 0){
		perror("open(crop_raw)");
		goto crop_raw_exit;
	}
	if (write(fd_croped_raw, p_croped_raw, height*height*sizeof(u16)) < 0) {
		perror("write(crop_raw)");
		goto crop_raw_exit;
	}
	printf("Save warp_croped.raw done.\n");

	memset(p_raw, 0, width*height*sizeof(u16));
	memcpy(p_raw, p_croped_raw, height*height*sizeof(u16));

crop_raw_exit:

	if(fd_croped_raw > 0){
		close(fd_croped_raw);
		fd_croped_raw = -1;
	}

	if(p_croped_raw != NULL){
		free(p_croped_raw);
		p_croped_raw = NULL;
	}
}

static void warp_compensation()
{
	int rval = -1;
	u16* raw_buff = NULL;
	iav_raw_info_t raw_info = {0};
	int height = 0, width = 0;
	u32 raw_size = 0;
	int count;
	int fd_raw = -1;
	FILE* fd_hori_table = NULL;
	FILE* fd_ver_table = NULL;
	warp_cal_info_t warp_detect_setup;
	static warp_correction_t warp_mw_cmd;

	if((fd_raw = open(warp_raw_file, O_RDONLY, 0))<0) {
		printf("raw file open error!\n");
		goto warp_cal_exit;
	}

	// for imx121, we use 2048x2048 warp
	height = 2048;
	width = 2080;
	raw_size = height*width*2;
	raw_buff = malloc(raw_size);
	if(raw_buff < 0){
		printf("malloc raw_buffer error!\n");
		goto warp_cal_exit;
	}

	count = read(fd_raw, raw_buff, raw_size/2);
	if(count != raw_size/2){
		printf("raw file read error!\n");
		goto warp_cal_exit;
	}
	crop_raw(raw_buff, width, height);
	raw_info.width = height;
	raw_info.height = height;
	raw_info.bayer_pattern = RG;
	/*input*/
	warp_detect_setup.raw_addr = raw_buff;
	warp_detect_setup.width = raw_info.width;
	warp_detect_setup.height = raw_info.height;
	warp_detect_setup.bayer = raw_info.bayer_pattern;
	warp_detect_setup.mode = 0;

	warp_mw_cmd.warp_control = 1;
	warp_mw_cmd.warp_horizontal_table_address = (u32)warp_horizontal_table;
	warp_mw_cmd.warp_vertical_table_address = (u32)warp_vertical_table;
	warp_mw_cmd.actual_left_top_x = 0;
	warp_mw_cmd.actual_left_top_y = 0;
	warp_mw_cmd.actual_right_bot_x = 2048;
	warp_mw_cmd.actual_right_bot_y = 2048;
	warp_mw_cmd.zoom_x = 1<<16;
	warp_mw_cmd.zoom_y = 1<<16;
	warp_mw_cmd.x_center_offset = 0;
	warp_mw_cmd.y_center_offset = 0;
	warp_mw_cmd.vert_warp_enable = 1;
	warp_mw_cmd.force_v4tap_disable = 0;
	warp_mw_cmd.reserved_2 = 0;
	warp_mw_cmd.hor_skew_phase_inc = 0;
	warp_mw_cmd.dummy_window_x_left = 0;
	warp_mw_cmd.dummy_window_y_top = 0;
	warp_mw_cmd.dummy_window_width = 4016;
	warp_mw_cmd.dummy_window_height = 3016;
	warp_mw_cmd.cfa_output_width = 4016;
	warp_mw_cmd.cfa_output_height = 3016;
	warp_mw_cmd.extra_sec2_vert_out_vid_mode = 0;

	rval = img_cal_warp(&warp_detect_setup, &warp_mw_cmd);
	if (rval<0){
		printf("img_cal_warp error!\n");
		goto warp_cal_exit;
	}

	int i,j;
	if((fd_hori_table = fopen("./hori.txt", "w+")) == NULL) {
		printf("warp horizental table file open error!\n");
		goto warp_cal_exit;
	}
	for(i = 0; i < 48; i++){
		for(j = 0; j < 32; j++){
			fprintf(fd_hori_table, "%d",(s16)warp_horizontal_table[i*32+j]);
			if((j+1)%32 == 0){
				fprintf(fd_hori_table, "\n");
			}else{
				fprintf(fd_hori_table, "\t");
			}
		}
	}

	if((fd_ver_table = fopen("./ver.txt", "w+")) == NULL) {
		printf("warp vertical table file open error!\n");
		goto warp_cal_exit;
	}
	for(i = 0; i < 48; i++){
		for(j = 0; j < 32; j++){
			fprintf(fd_ver_table, "%d",(s16)warp_vertical_table[i*32+j]);
			if((j+1)%32 == 0){
				fprintf(fd_ver_table, "\n");
			}else{
				fprintf(fd_ver_table, "\t");
			}
		}
	}
warp_cal_exit:

	if(fd_raw > 0){
		close(fd_raw);
		fd_raw = -1;
	}

	if(fd_hori_table != NULL){
		fclose(fd_hori_table);
		fd_hori_table = NULL;
	}

	if(fd_ver_table != NULL){
		fclose(fd_ver_table);
		fd_ver_table = NULL;
	}

	if(raw_buff != NULL){
		free(raw_buff);
		raw_buff = NULL;
	}
}

static void vignette_compensation()
{
	int fd_lenshading = -1;
	int count;
	static u32 gain_shift;
	static u16 vignette_table[33*33*4] = {0};
	if((fd_lenshading = open("./lens_shading.bin", O_RDONLY, 0))<0) {
		printf("lens_shading.bin cannot be opened\n");
		goto vignette_compensation_exit;
	}
	count = read(fd_lenshading, vignette_table, 4*VIGNETTE_MAX_SIZE*sizeof(u16));
	if(count != 4*VIGNETTE_MAX_SIZE*sizeof(u16)){
		printf("read lens_shading.bin error\n");
		goto vignette_compensation_exit;
	}
	count = read(fd_lenshading, &gain_shift, sizeof(u32));
	if(count != sizeof(u32)){
		printf("read lens_shading.bin error\n");
		goto vignette_compensation_exit;
	}
	printf("gain_shift is %d\n",gain_shift);
	vignette_info.enable = 1;
	vignette_info.gain_shift = (u8)gain_shift;
	vignette_info.vignette_red_gain_addr = (u32)(vignette_table + 0*VIGNETTE_MAX_SIZE);
	vignette_info.vignette_green_even_gain_addr = (u32)(vignette_table + 1*VIGNETTE_MAX_SIZE);
	vignette_info.vignette_green_odd_gain_addr = (u32)(vignette_table + 2*VIGNETTE_MAX_SIZE);
	vignette_info.vignette_blue_gain_addr = (u32)(vignette_table + 3*VIGNETTE_MAX_SIZE);
	img_dsp_set_vignette_compensation(fd_iav, &vignette_info);

vignette_compensation_exit:
	if(fd_lenshading > 0){
		close(fd_lenshading);
		fd_lenshading = -1;
	}
}

static void lens_shading_cal()
{
	vignette_cal_t vig_detect_setup;
	iav_mmap_info_t mmap_info;
	int fd_lenshading = -1;
	int rval = -1;
	u16* raw_buff = NULL;
	u32 lookup_shift;
	iav_raw_info_t raw_info;
	static u16 tab_buf[33*33*4] = {0};
	blc_level_t blc;
	u32 raw_vin_width = 0, raw_vin_height = 0;

	//Capture raw here
	printf("Raw capture started...\n");
	rval = ioctl(fd_iav, IAV_IOC_MAP_DSP, &mmap_info);
	if (rval < 0) {
		perror("IAV_IOC_MAP_DSP");
		goto vignette_cal_exit;
	}
	rval = ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info);
	if (rval < 0) {
		perror("IAV_IOC_READ_RAW_INFO");
		goto vignette_cal_exit;
	}

	raw_vin_width = raw_info.width;
	raw_vin_height = raw_info.height;
	raw_buff = (u16*)malloc(raw_vin_width*raw_vin_height*sizeof(u16));
	memcpy(raw_buff,raw_info.raw_addr,(raw_vin_width * raw_vin_height * 2));
	/*input*/
	vig_detect_setup.raw_addr = raw_buff;
	vig_detect_setup.raw_w = raw_vin_width;
	vig_detect_setup.raw_h = raw_vin_height;
	vig_detect_setup.bp = raw_info.bayer_pattern;
	vig_detect_setup.threshold = 4095;
	vig_detect_setup.compensate_ratio = 896;
	vig_detect_setup.lookup_shift = 255;
	/*output*/
	vig_detect_setup.r_tab = tab_buf + 0*VIGNETTE_MAX_SIZE;
	vig_detect_setup.ge_tab = tab_buf + 1*VIGNETTE_MAX_SIZE;
	vig_detect_setup.go_tab = tab_buf + 2*VIGNETTE_MAX_SIZE;
	vig_detect_setup.b_tab = tab_buf + 3*VIGNETTE_MAX_SIZE;
	img_dsp_get_global_blc(&blc);
	vig_detect_setup.blc.r_offset = blc.r_offset;
	vig_detect_setup.blc.gr_offset = blc.gr_offset;
	vig_detect_setup.blc.gb_offset = blc.gb_offset;
	vig_detect_setup.blc.b_offset = blc.b_offset;
	rval = img_cal_vignette(&vig_detect_setup);
	if (rval<0){
		printf("img_cal_vignette error!\n");
		goto vignette_cal_exit;
	}
	lookup_shift = vig_detect_setup.lookup_shift;
	if((fd_lenshading = open("./lens_shading.bin", O_CREAT | O_TRUNC | O_WRONLY, 0777))<0) {
		printf("vignette table file open error!\n");
		goto vignette_cal_exit;
	}
	rval = write(fd_lenshading, tab_buf, (4*VIGNETTE_MAX_SIZE*sizeof(u16)));
	if (rval<0){
		printf("vignette table file write error!\n");
		goto vignette_cal_exit;
	}
	rval = write(fd_lenshading, &lookup_shift, sizeof(lookup_shift));
	if (rval<0){
		printf("vignette table file write error!\n");
		goto vignette_cal_exit;
	}
	rval = write(fd_lenshading,&raw_vin_width,sizeof(raw_vin_width));
	if(rval<0){
		printf("vignette table file write error!\n");
		goto vignette_cal_exit;
	}
	rval = write(fd_lenshading,&raw_vin_height,sizeof(raw_vin_height));
	if(rval<0){
		printf("vignette table file write error!\n");
		goto vignette_cal_exit;
	}
vignette_cal_exit:
	if(fd_lenshading > 0){
		close(fd_lenshading);
		fd_lenshading = -1;
	}

	if(raw_buff != NULL){
		free(raw_buff);
		raw_buff = NULL;
	}
}

static void fixed_pattern_noise_cal()
{
	int fd_bpc = -1;
	int height = 0, width = 0;
	u8* fpn_map_addr = NULL;
	u32 bpc_num = 0;
	u32 raw_pitch;

	memset(&cfa_noise_filter,0,sizeof(cfa_noise_filter));
	img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);

	bad_corr.dark_pixel_strength = 0;
	bad_corr.hot_pixel_strength = 0;
	bad_corr.enable = 0;
	img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
	img_dsp_set_anti_aliasing(fd_iav,0);

	width = badpixel_detect_algo.cap_width;
	height = badpixel_detect_algo.cap_height;

	raw_pitch = ROUND_UP(width/8, 32);
	fpn_map_addr = (u8 *)malloc(raw_pitch*height);
	if (fpn_map_addr == NULL){
		printf("can not malloc memory for bpc map\n");
		goto fpn_cal_exit;
	}
	memset(fpn_map_addr,0,(raw_pitch*height));

	badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
	badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

	bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
	printf("Totol number is %d\n",bpc_num);

	if((fd_bpc = open(fpn_map_bin, O_CREAT | O_TRUNC | O_WRONLY, 0777))<0) {
		printf("map file %s open error!\n", fpn_map_bin);
		goto fpn_cal_exit;
	}
	write(fd_bpc, fpn_map_addr, (raw_pitch*height));
fpn_cal_exit:
	if(fd_bpc > 0){
		close(fd_bpc);
		fd_bpc = -1;
	}

	if(fpn_map_addr != NULL){
		free(fpn_map_addr);
		fpn_map_addr = NULL;
	}
}

static int load_fixed_pattern_noise()
{
	int file = -1, count = -1;
	u8* fpn_map_addr = NULL;
	u32 raw_pitch;
	int height = 0, width = 0;
	u32 fpn_map_size = 0;

	memset(&cfa_noise_filter,0,sizeof(cfa_noise_filter));
	img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
	bad_corr.dark_pixel_strength = 0;
	bad_corr.hot_pixel_strength = 0;
	bad_corr.enable = 0;
	img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
	img_dsp_set_anti_aliasing(fd_iav,0);

	width = load_fpn_param[0];
	height = load_fpn_param[1];
	raw_pitch = ROUND_UP((width/8), 32);
	fpn_map_size = raw_pitch*height;
	fpn_map_addr = malloc(fpn_map_size);
	if(fpn_map_addr == NULL){
		return -1;
	}
	memset(fpn_map_addr,0,(raw_pitch*height));

	if((file = open(fpn_map_bin, O_RDONLY, 0))<0) {
		printf("%s cannot be opened\n", fpn_map_bin);
		free(fpn_map_addr);
		fpn_map_addr = NULL;
		return -1;
	}
	if((count = read(file, fpn_map_addr, fpn_map_size)) != fpn_map_size) {
		printf("read %s error\n", fpn_map_bin);
		free(fpn_map_addr);
		close(file);
		return -1;
	}

	fpn.enable = 3;
	fpn.fpn_pitch = raw_pitch;
	fpn.pixel_map_height = height;
	fpn.pixel_map_width = width;
	fpn.pixel_map_size = fpn_map_size;
	fpn.pixel_map_addr = (u32)fpn_map_addr;
	img_dsp_set_static_bad_pixel_correction(fd_iav,&fpn);

	if(fpn_map_addr != NULL){
		free(fpn_map_addr);
		fpn_map_addr = NULL;
	}
	if (file >= 0) {
		close(file);
	}
	return 0;
}

void idsp_dump_bin(int fd_iav, u8 id)
{
	char bin_filename[32];
	int fd_bin_dump = -1;
	u8 *bin_buffer = NULL;
	iav_idsp_config_info_t	dump_idsp_info;
	int rval = -1;

	if(id >= 8 && id != 100){
		printf("err: unknow id section.\n");
		return;
	}

	if(bin_buffer == NULL){
		bin_buffer = (u8*)malloc(MAX_DUMP_BUFFER_SIZE);
	}
	if(bin_buffer != NULL){
		memset(bin_buffer, 0, MAX_DUMP_BUFFER_SIZE);
	}else{
		return;
	}

	dump_idsp_info.id_section = id;
	dump_idsp_info.addr = bin_buffer;

	rval = ioctl(fd_iav, IAV_IOC_IMG_DUMP_IDSP_SEC, &dump_idsp_info);
	if (rval < 0) {
		perror("IAV_IOC_IMG_DUMP_IDSP_SEC");
	}

	sprintf(bin_filename, "./idsp_dump_bin_%d.bin",id);
	fd_bin_dump = open(bin_filename, O_WRONLY | O_CREAT, 0666);
	if(fd_bin_dump < 0){
		printf("Failed to open bin file %s.\n",bin_filename);
		goto idsp_dump_exit;
	}

	if (write(fd_bin_dump, bin_buffer, dump_idsp_info.addr_long) < 0) {
		printf("Failed to write to bin file %s.\n",bin_filename);
		goto idsp_dump_exit;
	}

idsp_dump_exit:
	if(bin_buffer != NULL){
		free(bin_buffer);
		bin_buffer = NULL;
	}

	if(fd_bin_dump > 0){
		close(fd_bin_dump);
		fd_bin_dump = -1;
	}
}
void show_lib_version()
{
	img_lib_version_t img_algo_info;
	img_lib_version_t img_dsp_info;

	img_algo_get_lib_version(&img_algo_info);
	printf("\n[Algo lib info]: ");
	printf("Algo lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		img_algo_info.description, img_algo_info.major,
		img_algo_info.minor, img_algo_info.patch,
		img_algo_info.mod_time);
	img_dsp_get_lib_version(&img_dsp_info);
	printf("\n[DSP lib info]: ");
	printf("DSP lib Version : %s-%d.%d.%d (Last updated: %x)\n\n",
		img_dsp_info.description, img_dsp_info.major,
		img_dsp_info.minor, img_dsp_info.patch,
		img_dsp_info.mod_time);
}

int main(int argc, char **argv)
{
	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);
 	int rval = -1;
	img_config_info_t img_config_info;

	char sensor_name[32];

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0){
		printf("error: init_param\n");
		return -1;
	}
	if (map_bsb(fd_iav) < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	if(cc_flag) {
		rval = load_dsp_cc_table(fd_iav);
		CHECK_RVAL;
		return 0;
	}
	if (cal_blc_flag) {
		int file, count;
		blc_level_t blc;
		u16 *raw_buffer = NULL;
		int width = 4096, height =2160;

		if ((file = open(blc_file_name, O_RDONLY, 0)) < 0) {
			printf("%s cannot be opened\n", blc_file_name);
			return -1;
		}
		raw_buffer = malloc(width*height*2);
		if((count = read(file, raw_buffer, width*height*2)) != (width*height*2)) {
			printf("read %s error\n", blc_file_name);
			free(raw_buffer);
			raw_buffer = NULL;
			close(file);
			return -1;
		}
		close(file);
		memset(&blc, 0, sizeof(blc));
		cal_blc(raw_buffer, width, height, &blc);
		printf("blc oo %d oe %d eo %d ee %d\n", blc.gr_offset, blc.r_offset, blc.b_offset, blc.gb_offset);
		free(raw_buffer);
		return 0;

	}
	if(chroma_scale_flag) {
	}
	if(shutter_flag) {
		rval = img_set_sensor_shutter_index(fd_iav, shutter_index);
		return 0;
	}
	if(agc_flag) {
		rval = img_set_sensor_agc_index(fd_iav, agc_index, 16);
		return 0;
	}
	if(dbp_flag){
		rval = img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &bad_corr);
		CHECK_RVAL;
		return 0;
	}
	if(anti_aliasing_flag){
		u8 strength = anti_aliasing_strength;
		rval = img_dsp_set_anti_aliasing(fd_iav, strength);
		CHECK_RVAL;
		return 0;
	}
	if(cfa_filter_flag){
		rval = img_dsp_set_cfa_noise_filter(fd_iav, &cfa_noise_filter);
		CHECK_RVAL;
		return 0;
	}
	if(local_exposure_flag){
		rval = img_dsp_set_local_exposure(fd_iav, &local_exposure);
		CHECK_RVAL;
		return 0;
	}
	if(chroma_median_flag){
		rval = img_dsp_set_chroma_median_filter(fd_iav, &chroma_median_setup);
		CHECK_RVAL;
		return 0;
	}
	if(chroma_noise_filter_flg){
		rval = img_dsp_set_chroma_noise_filter(fd_iav, IMG_VIDEO, &chroma_noise_filter_param);
		CHECK_RVAL;
	}
	if(cnf_strength_flag){
		img_set_chroma_noise_filter_strength(cnf_strength);
	}
	if(high_freq_flag){
		img_dsp_set_luma_high_freq_noise_reduction(fd_iav, &luma_high_freq_noise_reduction_strength);
		return 0;
	}
	if(rgb2yuv_flag){
		rgb2yuv_matrix.u_offset = 0;
		rgb2yuv_matrix.v_offset = 128;
		rgb2yuv_matrix.y_offset = 128;
		rval = img_dsp_set_rgb2yuv_matrix(fd_iav, &rgb2yuv_matrix);
		CHECK_RVAL;
		return 0;
	}
	if(cfa_leakage_flag){
		cfa_leakage_filter.alpha_bb = 32;
		cfa_leakage_filter.alpha_br = 32;
		cfa_leakage_filter.alpha_rb = 32;
		cfa_leakage_filter.alpha_rr = 32;
		cfa_leakage_filter.saturation_level = 1024;
		rval = img_dsp_set_cfa_leakage_filter(fd_iav, &cfa_leakage_filter);
		CHECK_RVAL;
		return 0;
	}
	if(sharpening_fir_flag){
	//	rval = img_dsp_set_sharpening_fir(fd_iav, &fir, 0); // mode == 0 for video mode
		CHECK_RVAL;
		return 0;
	}
	if(coring_table_flag){
		int i;
		for (i=0;i<256;i++)
			coring.coring[i] = coring_strength;
		rval = img_dsp_set_sharpen_a_coring(fd_iav, IMG_VIDEO, &coring);
		CHECK_RVAL;
		return 0;
	}
	if(warp_compensation_flag){
		warp_compensation();
	}
	if(vignette_compensation_flag){
		vignette_compensation();
	}
	if(cal_lens_shading){
		lens_shading_cal();
	}
	if(detect_fpn_flag){
		fixed_pattern_noise_cal();
	}
	if(load_fpn_flag){
		load_fixed_pattern_noise();
	}
	if(raw_proc_flag) {
		save_raw();
		return 0;
	}
	if(msg_send_flag) {
//		img_set_work_mode(msg_param);
		u32 cntl = 0xffff;
		cntl &= ~(u32)(1<<msg_param);
		if(ioctl(fd_iav, IAV_IOC_IMG_NOISE_FILTER_SETUP, cntl)<0)
			perror("IAV_IOC_IMG_NOISE_FILTER_SETUP");
		return 0;
	}
	if(ghost_prv_flag) {
		video_mctf_ghost_prv_info_t gp_switch[2] = {{0,0,0},{1,1,1}};
		img_dsp_set_video_mctf_ghost_prv(fd_iav, &gp_switch[ghost_on%2]);
	}
	if(idsp_dump_flg){
		idsp_dump_bin(fd_iav,idsp_dump_id);
	}
	if(manual_contrast_flg){
		img_set_color_contrast(manual_contrast_str);
	}
	if(auto_contrast_flg){
		img_set_auto_color_contrast(auto_contrast_ena);
	}
	if(auto_contrast_str_flg){
		img_set_auto_color_contrast_strength(auto_contrast_str);
	}
	if(mctf_flag) {
		img_set_mctf_strength(mctf_strength);
	}
	if(sharpen_flag){
		img_set_sharpness(sharpen_level);
	}
	if(yuv_contrast_ena_flg){
		img_set_yuv_contrast_enable(yuv_contrast_ena);
	}
	if(yuv_contrast_str_flg){
		img_set_yuv_contrast_strength(yuv_contrast_str);
	}
	if(spacial_dn_flag){
		img_set_spacial_denoise(spacial_dn_strength);
	}
	if(wdr_ena_flag){
		int tgt_ratio = 1024;
		img_set_wdr_enable(wdr_enable);
		tgt_ratio = (wdr_enable == 1)? 512 : 1024;
		img_ae_set_target_ratio(tgt_ratio);
	}
	if(wdr_str_flag){
		img_set_wdr_strength(wdr_strength);
	}
	if(multi_vin_flag){
		img_aaa_stat_t multi_vin_stat[4];
		aaa_tile_report_t multi_vin_tile[4];

		if(img_config_working_status(fd_iav, &img_config_info) < 0){
			printf("error: img_config_working_status\n");
			return -1;
		}
		if(img_lib_init(img_config_info.defblc_enable, img_config_info.sharpen_b_enable) < 0){
			printf("error: img_lib_init\n");
			return -1;
		}

		img_dsp_config_statistics_info(fd_iav, &imx185_tile_config);

		while(1){
			img_dsp_get_statistics_multi_vin(fd_iav, multi_vin_stat, multi_vin_tile);
			// deal with the 3A process
			sleep(1);
		}
	}
	if(start_aaa){
		if(img_config_working_status(fd_iav, &img_config_info) < 0){
			printf("error: img_config_working_status\n");
			return -1;
		}
		if(img_lib_init(img_config_info.defblc_enable, img_config_info.sharpen_b_enable) < 0){
			printf("error: img_lib_init\n");
			return -1;
		}
		show_lib_version();
		if(load_mctf_bin() < 0){
			printf("error: load_mctf_bin\n");
			return -1;
		}
		if(get_is_info(&app_param_image_sensor, sensor_name, &sensor_config_info) < 0){
			printf("error: get_is_info\n");
			return -1;
		}
		if (img_config_sensor_info(&sensor_config_info) < 0) {
			printf("error: img_config_sensor_info\n");
			return -1;
		}
		if (img_config_sensor_hdr_mode(vin_op_mode) < 0) {
			printf("error: img_config_sensor_hdr_mode\n");
			return -1;
		}
		img_config_lens_info(LENS_CMOUNT_ID);
		img_lens_init();

		start_aaa_foo(fd_iav, sensor_name, app_param_image_sensor);

		while(1){ sleep(2); }
	}

	return 0;
}


