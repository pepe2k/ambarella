#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include <sched.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>

#include "img_abs_filter.h"
#include "img_adv_struct_arch.h"

#include "ambas_vin.h"
#include "AmbaDSP_ImgFilter.h"
#include "img_api_adv_arch.h"

#include "ambas_vin.h"

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "mw_struct_hiso.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"



#define CALI_FILES_PATH		"/ambarella/calibration"
#define	ADJ_PARAM_PATH	"/etc/idsp/adj_params"

#define SHT_TIME(SHT_Q9)		DIV_ROUND(512000000, (SHT_Q9))

#define AWB_TILE_NUM_COL		(24)
#define AWB_TILE_NUM_ROW		(16)


/**********************************************************************
 *      Internal functions
 *********************************************************************/
static int search_nearest(u32 key, u32 arr[], int size) 	//the arr is in reverse order
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	while(1) {
		if (l == r)
			return l;
		if (key > arr[m]) {
			r = m;
		} else if  (key < arr[m]) {
			l = m + 1;
		} else {
			return m;
		}
		m = (l+r) / 2;
	}
	return -1;
}

extern u32 TIME_DATA_TABLE_128[SHUTTER_TIME_TABLE_LENGTH];
int shutter_q9_to_index(u32 shutter_time)
{
	int tmp_idx;
	tmp_idx = search_nearest(shutter_time, TIME_DATA_TABLE_128, SHUTTER_TIME_TABLE_LENGTH);
	return SHUTTER_TIME_TABLE_LENGTH - tmp_idx;
}

u32 shutter_index_to_q9(int index)
{
	if (index < 0 || index >= SHUTTER_TIME_TABLE_LENGTH) {
		MW_ERROR("The index is not in the range\n");
		return -1;
	}

	return TIME_DATA_TABLE_128[index];
}


/*************************** AE lines **********************************/

static u8 G_gain_table[] = {
	ISO_100,
	ISO_150,
	ISO_200,	//6db
	ISO_300,
	ISO_400,	//12db
	ISO_600,
	ISO_800,	//18db
	ISO_1600,	//24db
	ISO_3200 ,	//30db
	ISO_6400,	//36db
	ISO_12800,	//42db
	ISO_25600,	//48db
	ISO_51200,	//54db
	ISO_102400,//60db
};
#define	GAIN_TABLE_NUM		(sizeof(G_gain_table) / sizeof(u8))

static line_t G_50HZ_LINES[] = {
	{
		{{SHUTTER_1BY16000_SEC, ISO_100, 0}},
		{{SHUTTER_1BY100_SEC, ISO_100, 0}}
	},
	{
		{{SHUTTER_1BY100_SEC, ISO_100, 0}},
		{{SHUTTER_1BY100_SEC, ISO_300, 0}}
	},
	{
		{{SHUTTER_1BY50_SEC, ISO_100, 0}},
		{{SHUTTER_1BY50_SEC, ISO_300, 0}}
	},
	{
		{{SHUTTER_1BY25_SEC, ISO_100, 0}},
		{{SHUTTER_1BY25_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_100, 0}},
		{{SHUTTER_1BY15_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_100, 0}},
		{{SHUTTER_1BY7P5_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

static line_t G_60HZ_LINES[] = {
	{
		{{SHUTTER_1BY16000_SEC, ISO_100, 0}},
		{{SHUTTER_1BY120_SEC, ISO_100, 0}}
	},
	{
		{{SHUTTER_1BY120_SEC, ISO_100, 0}},
		{{SHUTTER_1BY120_SEC, ISO_300, 0}}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, 0}},
		{{SHUTTER_1BY60_SEC, ISO_300, 0}}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_100, 0}},
		{{SHUTTER_1BY30_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_100, 0}},
		{{SHUTTER_1BY15_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_100, 0}},
		{{SHUTTER_1BY7P5_SEC, ISO_400, 0}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

static line_t G_mw_ae_lines[MAX_AE_LINES_NUM];
static int G_line_num;
static int G_line_belt_default;
static int G_line_belt_current;

static pthread_t	slow_shutter_task_id = 0;

/* ***************************************************************
	Calibration data file
******************************************************************/
mw_cali_file			G_mw_cali;

/**********************************************************************
 *      External functions
 *********************************************************************/

inline int get_vin_mode(u32 *vin_mode)
{
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, vin_mode) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
		return -1;
	}
	return 0;
}

inline int get_vin_frame_rate(u32 *pFrame_time)
{
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_FRAME_RATE, pFrame_time) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}
	return 0;
}


inline int check_state(void)
{
	iav_state_info_t info;

	memset(&info, 0, sizeof(info));
	if (ioctl(G_mw_config.fd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	if (info.state == IAV_STATE_IDLE || info.state == IAV_STATE_INIT)
		return -1;

	return 0;
}

inline int config_sensor_lens_info(void)
{
	struct amba_vin_source_info vin_info;
	amba_vin_agc_info_t agc_info;
	mw_sensor_param *sensor = &G_mw_config.sensor_params;
	u32 vin_fps;

	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_AGC_INFO, &agc_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_AGC_INFO error\n");
		return -1;
	}
	sensor->db_step = agc_info.db_step;
	sensor->max_g_db = agc_info.db_max >> 24;

	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	get_vin_frame_rate(&vin_fps);
	sensor->default_fps = vin_fps;
	sensor->current_fps = vin_fps;

	switch (vin_info.sensor_id) {
		/* Sony Sensors */
		case SENSOR_IMX185:
			sensor->sensor_id = SENSOR_IMX185;
			sprintf(sensor->name, "imx185");
			break;
		case SENSOR_IMX224:
			sensor->sensor_id = SENSOR_IMX224;
			sprintf(sensor->name, "imx224");
			break;

		/* Panasonic Sensors */
		case SENSOR_MN34210PL:
			sensor->sensor_id = SENSOR_MN34210PL;
			sprintf(sensor->name, "mn34210pl");
			break;
		case SENSOR_MN34220PL:
			sensor->sensor_id = SENSOR_MN34220PL;
			sprintf(sensor->name, "mn34220pl");
			break;

		default:
			sensor->sensor_id = SENSOR_MN34220PL;
			sprintf(sensor->name, "mn34220pl");
			break;
	}

	return 0;
}

static int set_params(void)
{
	image_property_t img_prop;
	if (load_ae_exp_lines(&G_mw_config.ae_params) < 0) {
		MW_ERROR("load_ae_exp_lines error\n");
		return -1;
	}

	if (img_hiso_get_img_property(&img_prop) < 0) {
		MW_ERROR("img_hiso_get_img_property error\n");
		return -1;
	}
	G_mw_config.image_params.brightness = img_prop.brightness;
	G_mw_config.image_params.contrast = img_prop.contrast;
	G_mw_config.image_params.saturation= img_prop.saturation;

	return 0;
}

int do_prepare_hiso_aaa(void)
{
	if (!G_mw_config.init_params.aaa_enable) {
		mw_sensor_param *sensor_params = &G_mw_config.sensor_params;
		sensor_params = &G_mw_config.sensor_params;
		if (img_hiso_lib_init(G_mw_config.fd, 1, 1) < 0) {
			MW_ERROR("img_hiso_lib_init!\n");
			return -1;
		}

		if (config_sensor_lens_info() < 0) {
			MW_ERROR("config_sensor_lens_info error!\n");
			return -1;
		}

		load_iso_containers(sensor_params->sensor_id);
		load_iso_cc_bin(sensor_params->name);
		enable_iso_cc(sensor_params->name, G_mw_config.init_params.mode);
		load_img_iso_aaa_config(sensor_params->sensor_id);
		img_hiso_prepare_idsp(G_mw_config.fd);
	}
	return 0;
}

int do_start_hiso_aaa(void)
{
	if (!G_mw_config.init_params.aaa_enable) {
		img_hiso_start_aaa(G_mw_config.fd);
		img_hiso_set_work_mode(0);
		MW_MSG("======= [DONE] start_hiso_aaa_task ======= \n");

		if (set_params() < 0) {
			MW_ERROR("reload_previous_params error!\n");
			return -1;
		}
		if (G_mw_config.ae_params.slow_shutter_enable) {
			create_slowshutter_task();
		}
		G_mw_config.init_params.aaa_enable = 1;
	}
	return 0;

}

int do_stop_hiso_aaa(void)
{

	if (G_mw_config.init_params.aaa_enable) {
		if (G_mw_config.ae_params.slow_shutter_enable) {
			destroy_slowshutter_task();
		}
		if (img_hiso_stop_aaa() < 0) {
			MW_ERROR("stop aaa error!\n");
			return -1;
		}

		usleep(1000);
		if (img_hiso_lib_deinit() < 0) {
			MW_ERROR("img_lib_deinit error!\n");
			return -1;
		}

		MW_MSG("======= [DONE] stop_hiso_aaa_task ======= \n");
		G_mw_config.init_params.aaa_enable = 0;
	}
	return 0;
}

//AE
static inline int print_ae_lines(line_t *lines,
	int line_num, u16 line_belt, int enable_convert)
{
	int i;
	MW_INFO("===== [MW] ==== Automatic Generates AE lines =====\n");
	for (i = 0; i < line_num; ++i) {
		if (i == line_belt) {
			MW_INFO("===== [MW] ====== This is the line belt. =========\n");
		}
		if (enable_convert) {
			MW_INFO(" [%d] start (1/%d, %d, %d) == end (1/%d, %d, %d)\n", (i + 1),
				SHT_TIME(lines[i].start.factor[MW_SHUTTER]),
				lines[i].start.factor[MW_DGAIN], lines[i].start.factor[MW_IRIS],
				SHT_TIME(lines[i].end.factor[MW_SHUTTER]),
				lines[i].end.factor[MW_DGAIN], lines[i].end.factor[MW_IRIS]);
		} else {
			MW_INFO(" [%d] start (%d, %d, %d) == end (%d, %d, %d)\n", (i + 1),
				lines[i].start.factor[MW_SHUTTER],
				lines[i].start.factor[MW_DGAIN], lines[i].start.factor[MW_IRIS],
				lines[i].end.factor[MW_SHUTTER],
				lines[i].end.factor[MW_DGAIN], lines[i].end.factor[MW_IRIS]);
		}
	}
	MW_INFO("======= [MW] ==== NUM [%d] ==== BELT [%d] ==========\n\n",
		line_num, line_belt);
	return 0;
}

static int generate_normal_ae_lines(mw_ae_param *p, line_t *lines, int *line_num, int *line_belt)
{
	int dst, src, i;
	int s_max, s_min, g_max, g_min, p_index_max, p_index_min;
	line_t *p_src = NULL;
	int flicker_off_shutter_time = 0;
	int longest_possible_shutter = 0, curr_belt = 0;

	amba_vin_agc_info_t vin_agc_info;

	s_max = p->shutter_time_max;
	s_min = p->shutter_time_min;
	g_max = p->sensor_gain_max;
	if ((p->lens_aperture.aperture_min != APERTURE_AUTO) ||
		(p->lens_aperture.aperture_max != APERTURE_AUTO)) {
		MW_MSG("Not support p iris lens in HISO mode\n");
	}
	p_index_max = 0;
	p_index_min  = 0;

	p_src = (p->anti_flicker_mode == 1) ? G_60HZ_LINES : G_50HZ_LINES;
	flicker_off_shutter_time = (p->anti_flicker_mode == 1) ?
		SHUTTER_1BY120_SEC : SHUTTER_1BY100_SEC;
	dst = src = curr_belt = 0;

	// create line 1 - shutter line / digital gain line
	if (s_min < flicker_off_shutter_time) {
		// create shutter line
		lines[dst] = p_src[src];
		lines[dst].start.factor[MW_SHUTTER] = s_min;
		if (s_max < flicker_off_shutter_time) {
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			++dst;
			lines[dst].start.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_DGAIN] = g_max;
			++dst;
			curr_belt = dst;
			goto GENERATE_LINES_EXIT;
		}
		++dst;
		++src;
	} else {
		// create digital gain line
		while (s_min > p_src[src].start.factor[MW_SHUTTER])
			++src;
		lines[dst] = p_src[src];
		++dst;
		++src;
	}

	// create other lines - digital gain line
	while (s_max >= p_src[src].start.factor[MW_SHUTTER]) {
		if (p_src[src].start.factor[MW_SHUTTER] == SHUTTER_INVALID)
			break;
		lines[dst] = p_src[src];
		++dst;
		++src;
	}
	lines[dst - 1].end.factor[MW_DGAIN] = g_max;

	// change min gain from sensor driver
	memset(&vin_agc_info, 0, sizeof(amba_vin_agc_info_t));
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_AGC_INFO, &vin_agc_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO");
		return 0;
	}
	g_min = (vin_agc_info.db_min >> 24);

	i = 0;
	while (G_gain_table[i] < g_min) {
		if (++i == GAIN_TABLE_NUM) {
			--i;
			break;
		}
	}
	g_min = G_gain_table[i];

	MW_INFO("---Min Gain: %d, Max Gain;%d\n", g_min, g_max);
	i = 0;
	while (i < dst) {
		if (lines[i].start.factor[MW_SHUTTER] != lines[i].end.factor[MW_SHUTTER]) {
			/* Shutter line: change min gain both for start and end points */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
			if (lines[i].end.factor[MW_DGAIN] < g_min) {
				lines[i].end.factor[MW_DGAIN] = g_min;
			}
		} else {
			/* Gain line: change min gain for start point */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
		}
		++i;
	}

	//change the min/max piris lens size
	if (p_index_min < lines[0].start.factor[MW_IRIS]) {
		 lines[0].start.factor[MW_IRIS] = p_index_min;
	}
	if (p_index_max > lines[dst].end.factor[MW_IRIS]) {
		 lines[dst].end.factor[MW_IRIS] = p_index_max;
	}
	i = 0;
	while (i < dst) {
		if (p_index_min > lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_min > lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_max < lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_max;
		}
		if (p_index_max < lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_max;
		}
		++i;
	}

	// calculate line belt of current and default
	curr_belt = dst;
	while (1) {
		longest_possible_shutter = lines[curr_belt - 1].end.factor[MW_SHUTTER];
		if (longest_possible_shutter <= G_mw_config.sensor_params.default_fps) {
			G_line_belt_default = curr_belt;
		}
		if (p->slow_shutter_enable) {
			if (longest_possible_shutter <= G_mw_config.sensor_params.current_fps) {
				MW_INFO("\tSlow shutter is enabled, set curr_belt [%d] to"
					" current frame rate [%d].\n",
					curr_belt, G_mw_config.sensor_params.current_fps);
				break;
			}
		} else {
			if (longest_possible_shutter <= G_mw_config.sensor_params.default_fps) {
				lines[curr_belt - 1].end.factor[MW_DGAIN] = g_max;
				MW_INFO("\tSlow shutter is disabled, restore curr_belt [%d] to"
					" default frame rate [%d].\n",
					curr_belt, G_mw_config.sensor_params.default_fps);
				break;
			}
		}
		--curr_belt;
		MW_INFO("\t\t\t=== curr_belt [%d] def_belt [%d] == VIN [%d fps] == \n",
			curr_belt, G_line_belt_default,
			SHT_TIME(G_mw_config.sensor_params.current_fps));
	}

GENERATE_LINES_EXIT:
	*line_num = dst;
	*line_belt = curr_belt;

	print_ae_lines(lines, dst, curr_belt, 1);
	return 0;
}

static inline int generate_manual_shutter_lines(mw_ae_param *p,
	line_t *lines, int *line_num, int *line_belt)
{
	int total_lines = 0;
	lines[total_lines].start.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].start.factor[MW_DGAIN] = 0;
	lines[total_lines].start.factor[MW_IRIS] = 0;
	lines[total_lines].end.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].end.factor[MW_DGAIN] = p->sensor_gain_max;
	lines[total_lines].end.factor[MW_IRIS] = 0;
	++total_lines;
	*line_num = total_lines;
	*line_belt = total_lines;

	print_ae_lines(lines, total_lines, total_lines, 1);
	return 0;
}

static int generate_ae_lines(mw_ae_param *p,
	line_t *lines, int *line_num, int *line_belt)
{
	int retv = 0;

	if (p->shutter_time_max != p->shutter_time_min) {
		retv = generate_normal_ae_lines(p, lines, line_num, line_belt);
	} else {
		retv = generate_manual_shutter_lines(p, lines, line_num, line_belt);
	}

	return retv;
}

static int load_ae_lines(line_t *line, int line_num, int line_belt)
{
	int i;
	line_t img_ae_lines[MAX_AE_LINES_NUM * MAX_HDR_EXPOSURE_NUM];
	u16 line_num_expo[MAX_HDR_EXPOSURE_NUM];
	u16 line_belt_expo[MAX_HDR_EXPOSURE_NUM];

	memcpy(img_ae_lines, line, sizeof(line_t) * line_num);
	//transfer q9 format to shutter index format
	for (i = 0; i < line_num; i++) {
		img_ae_lines[i].start.factor[MW_SHUTTER]
			= shutter_q9_to_index(line[i].start.factor[MW_SHUTTER]);
		img_ae_lines[i].end.factor[MW_SHUTTER]
			= shutter_q9_to_index(line[i].end.factor[MW_SHUTTER]);
	}

	MW_INFO("=== Convert shutter time to shutter index ===\n");
	print_ae_lines(img_ae_lines, line_num, line_belt, 0);
	if (img_hiso_format_ae_line(G_mw_config.fd, img_ae_lines, line_num,
		G_mw_config.sensor_params.db_step) < 0) {
		MW_ERROR("[img_ae_load_exp_line error] : line_num [%d] line_belt [%d].\n",
			line_num, line_belt);
		return -1;
	}
	line_num_expo[0] = (u16)line_num;
	line_belt_expo[0] = (u16)line_belt;

	line_num = line_num_expo[0] + line_num_expo[1];

	if (img_hiso_set_ae_exp_lines(img_ae_lines, line_num_expo, line_belt_expo) < 0) {
		MW_ERROR("[img_set_ae_exp_lines error] : line_num [%d] line_belt [%d].\n",
			line_num, line_belt);
		return -1;
	}

	return 0;
}

int load_ae_exp_lines(mw_ae_param *ae)
{
	memset(G_mw_ae_lines, 0, sizeof(G_mw_ae_lines));

	if (ae->shutter_time_max < ae->shutter_time_min) {
		MW_INFO("shutter limit max [%d] is less than shutter min [%d]. Tie them to shutter min\n",
			ae->shutter_time_max, ae->shutter_time_min);
		ae->shutter_time_max = ae->shutter_time_min;
	}
	if (generate_ae_lines(ae, G_mw_ae_lines,
		&G_line_num, &G_line_belt_current) < 0) {
		MW_ERROR("generate_ae_lines error\n");
		return -1;
	}
	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}
	return 0;
}

int get_ae_exposure_lines(mw_ae_line *lines, u32 num)
{
	int i;

	for (i = 0; i < num && i < G_line_num; ++i) {
		lines[i].start.factor[MW_SHUTTER] = G_mw_ae_lines[i].start.factor[MW_SHUTTER];
		lines[i].start.factor[MW_DGAIN] = G_mw_ae_lines[i].start.factor[MW_DGAIN];
		lines[i].start.factor[MW_IRIS] = G_mw_ae_lines[i].start.factor[MW_IRIS];
		lines[i].end.factor[MW_SHUTTER] = G_mw_ae_lines[i].end.factor[MW_SHUTTER];
		lines[i].end.factor[MW_DGAIN] = G_mw_ae_lines[i].end.factor[MW_DGAIN];
		lines[i].end.factor[MW_IRIS] = G_mw_ae_lines[i].end.factor[MW_IRIS];
	}
	print_ae_lines(G_mw_ae_lines, i, G_line_belt_current, 1);

	return 0;
}

int set_ae_exposure_lines(mw_ae_line *lines, u32 num)
{
	int i;
	u16 curr_belt;

	for (i = 0; i < num - 1; ++i) {
		if (lines[i+1].start.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			break;
		}
		if (lines[i].start.factor[MW_SHUTTER] > lines[i+1].start.factor[MW_SHUTTER]) {
			MW_MSG("AE line [%d] shutter time [%d] must be larger than line [%d] shutter time [%d].\n",
				i+1, lines[i+1].start.factor[MW_SHUTTER], i, lines[i].start.factor[MW_SHUTTER]);
			return -1;
		}
	}
	memset(G_mw_ae_lines, 0, sizeof(G_mw_ae_lines));
	for (i = 0, curr_belt = 0; i < num; i++) {
		if (lines[i].start.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			break;
		}
		G_mw_ae_lines[i].start.factor[MW_SHUTTER] = lines[i].start.factor[MW_SHUTTER];
		G_mw_ae_lines[i].start.factor[MW_DGAIN] = lines[i].start.factor[MW_DGAIN];
		G_mw_ae_lines[i].start.factor[MW_IRIS] = lines[i].start.factor[MW_IRIS];
		G_mw_ae_lines[i].end.factor[MW_SHUTTER] = lines[i].end.factor[MW_SHUTTER];
		G_mw_ae_lines[i].end.factor[MW_DGAIN] = lines[i].end.factor[MW_DGAIN];
		G_mw_ae_lines[i].end.factor[MW_IRIS] = lines[i].end.factor[MW_IRIS];
		MW_DEBUG("AE line [%d], shutter [%d], vin [%d]\n",
			i, lines[i].end.factor[MW_SHUTTER], G_mw_config.sensor_params.current_fps);
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_config.sensor_params.current_fps) {
			curr_belt = i + 1;
		}
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_config.sensor_params.default_fps) {
			G_line_belt_default = i + 1;
		}
	}
	G_line_num = i;
	G_line_belt_current = curr_belt;

	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}

	return 0;
}

int set_ae_switch_points(mw_ae_point *points, u32 num)
{
	int i, j;
	joint_t *switch_point = NULL;

	for (i = 0; i < num; ++i) {
		for (j = 0; j < G_line_num; j++) {
			if (points[i].factor[MW_SHUTTER] == SHUTTER_INVALID) {
				continue;
			}
			if (G_mw_ae_lines[j].start.factor[MW_SHUTTER] == points[i].factor[MW_SHUTTER]) {
				break;
			}
		}
		if (j == G_line_num) {
			MW_MSG("Invalid switch point [%d, %d].\n",
				points[i].factor[MW_SHUTTER], points[i].factor[MW_DGAIN]);
			continue;
		}
		switch_point = (points[i].pos == MW_AE_END_POINT) ? &G_mw_ae_lines[j].end
			: &G_mw_ae_lines[j].start;
		switch_point->factor[MW_DGAIN] = points[i].factor[MW_DGAIN];
	}

	if (load_ae_lines(G_mw_ae_lines, G_line_num, G_line_belt_current) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d]\n",
			G_line_num, G_line_belt_current);
		return -1;
	}

	return 0;
}

static inline int set_vsync_vin_framerate(u32 frame_time)
{
	int rval;

	if ((rval = ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_SET_FRAME_RATE, frame_time)) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
		return rval;
	}

	return 0;
}

static int update_vin_frame_rate(void)
{
	if (ioctl(G_mw_config.fd, IAV_IOC_UPDATE_VIN_FRAMERATE_EX, 0) < 0) {
		perror("IAV_IOC_UPDATE_VIN_FRAMERATE_EX");
		return -1;
	}
	MW_INFO("[Done] update_vin_frame_rate !\n");
	return 0;
}

void slow_shutter_task(void *arg)
{
	#define	HISTORY_LENGTH		(1)
	static int transition_counter = 0;
	u32 curr_frame_time, default_frame_time, target_frame_time;
	u8 ae_cursor;
	int vin_tick = 0;
	char	vin_int_array[8];
	u16 ae_belt[MAX_HDR_EXPOSURE_NUM] = {0};

	if (vin_tick <= 0) {
		vin_tick = open(VIN0_VSYNC, O_RDONLY);
		if (vin_tick < 0) {
			MW_MSG("CANNOT OPEN [%s].\n", VIN0_VSYNC);
			return;
		}
	}

	while (G_mw_config.ae_params.slow_shutter_enable) {
		read(vin_tick, vin_int_array, 8);
		img_hiso_get_ae_cursor(&ae_cursor);
		default_frame_time = G_mw_config.sensor_params.default_fps;
		get_vin_frame_rate(&curr_frame_time);

		G_line_belt_current = MAX(ae_cursor, G_line_belt_default);
		G_line_belt_current = MIN(G_line_belt_current, G_line_num);

		target_frame_time = G_mw_ae_lines[G_line_belt_current-1].start.factor[0];
		if (target_frame_time < default_frame_time)
			target_frame_time = default_frame_time;

		if (target_frame_time != curr_frame_time) {
			++transition_counter;
			if (transition_counter == HISTORY_LENGTH) {
				read(vin_tick, vin_int_array, 8);
				set_vsync_vin_framerate(target_frame_time);
				update_vin_frame_rate();
				ae_belt[0] = (u16)G_line_belt_current;
				img_hiso_set_ae_loosen_belt(ae_belt);
				G_mw_config.sensor_params.current_fps = target_frame_time;
				MW_INFO(" [CHANGE] = def [%d], curr [%d], target [%d], belt [%d].\n",
					default_frame_time, curr_frame_time,
					target_frame_time, G_line_belt_current);
				transition_counter = 0;
			}
		}
	}

	/* Todo:
	 * Exit slow shutter task, restore the sensor framerate to default.
	 */
	if (G_mw_config.sensor_params.current_fps >
		G_mw_config.sensor_params.default_fps) {
		read(vin_tick, vin_int_array, 8);
		set_vsync_vin_framerate(G_mw_config.sensor_params.default_fps);
		update_vin_frame_rate();
		ae_belt[0] = (u16)G_line_belt_current;
		img_hiso_set_ae_loosen_belt(ae_belt);
	}
	close(vin_tick);
	vin_tick = 0;
}

int create_slowshutter_task(void)
{
	// create slow shutter thread
	if (slow_shutter_task_id == 0) {
		if (pthread_create(&slow_shutter_task_id, NULL,
			(void *)slow_shutter_task, NULL) != 0) {
			MW_MSG("Failed. Can't create thread <slow_shutter_task> !\n");
		}
		MW_INFO("Create thread <slow_shutter_task> successful !\n");
	}
	return 0;
}

int destroy_slowshutter_task(void)
{

	if (slow_shutter_task_id != 0) {
		if (pthread_join(slow_shutter_task_id, NULL) != 0) {
			MW_MSG("Failed. Can't destroy thread <slow_shutter_task> !\n");
		}
		MW_INFO("Destroy thread <slow_shutter_task> successful !\n");
	}
	slow_shutter_task_id = 0;
	return 0;
}
