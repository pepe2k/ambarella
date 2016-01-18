/**********************************************************************
 *
 * mw_get_aaa_params.c
 *
 * History:
 *	2012/12/10 - [Jingyang Qiu] Created this file
 *
 * Copyright (C) 2012 - 2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "basetypes.h"
#include "img_struct_arch.h"
#include "img_api_arch.h"

#include "mw_aaa_params.h"

#define	READ_ALIGN(value, align)		((value + align -1) / align * align)


int display_structure_size(ambarella_data_header_t *pData_header)
{
	printf("----sizeof(data_header)=%d\n", sizeof(ambarella_data_header_t));
	printf("------sizeof(LIST_CHROMA_SCALE) %d, num %d \n",
		pData_header->struct_type[LIST_CHROMA_SCALE].struct_size,
		pData_header->struct_type[LIST_CHROMA_SCALE].struct_total_num);
	printf("------sizeof(LIST_RGB2YUV) %d, num %d \n",
		pData_header->struct_type[LIST_RGB2YUV].struct_size,
		pData_header->struct_type[LIST_RGB2YUV].struct_total_num);
	printf("------sizeof(LIST_ADJ_PARAM) %d, num %d \n",
		pData_header->struct_type[LIST_ADJ_PARAM].struct_size,
		pData_header->struct_type[LIST_ADJ_PARAM].struct_total_num);
	printf("------sizeof(LIST_MANUAL_LE) %d, num %d \n",
		pData_header->struct_type[LIST_MANUAL_LE].struct_size,
		pData_header->struct_type[LIST_MANUAL_LE].struct_total_num);
	printf("------sizeof(LIST_TILE_CONFIG) %d, num %d \n",
		pData_header->struct_type[LIST_TILE_CONFIG].struct_size,
		pData_header->struct_type[LIST_TILE_CONFIG].struct_total_num);
	printf("------sizeof(LIST_LINES) %d, num %d \n",
		pData_header->struct_type[LIST_LINES].struct_size,
		pData_header->struct_type[LIST_LINES].struct_total_num);
	printf("------sizeof(LIST_AWB_PARAM) %d, num %d \n",
		pData_header->struct_type[LIST_AWB_PARAM].struct_size,
		pData_header->struct_type[LIST_AWB_PARAM].struct_total_num);
	printf("------sizeof(LIST_AE_AGC_DGAIN) %d, num %d \n",
		pData_header->struct_type[LIST_AE_AGC_DGAIN].struct_size,
		pData_header->struct_type[LIST_AE_AGC_DGAIN].struct_total_num);
	printf("------sizeof(LIST_AE_SHT_DGAIN) %d, num %d \n",
		pData_header->struct_type[LIST_AE_SHT_DGAIN].struct_size,
		pData_header->struct_type[LIST_AE_SHT_DGAIN].struct_total_num);
	printf("------sizeof(LIST_DLIGHT) %d, num %d \n",
		pData_header->struct_type[LIST_DLIGHT].struct_size,
		pData_header->struct_type[LIST_DLIGHT].struct_total_num);
	printf("------sizeof(LIST_SENSOR_CONFIG) %d, num %d \n",
		pData_header->struct_type[LIST_SENSOR_CONFIG].struct_size,
		pData_header->struct_type[LIST_SENSOR_CONFIG].struct_total_num);
	return 0;
}

int read_header(int fd, ambarella_adj_aeb_bin_parse_t *pHeader)
{
	int size = sizeof(ambarella_adj_aeb_bin_parse_t);

	memset(pHeader, 0, size);

	if (read(fd, pHeader, size) < 0) {
		perror("read");
		return -1;
	}
//	display_structure_size(&pHeader->data_header);
	memcpy(&G_adj_bin_header, &(pHeader->bin_header),
		sizeof(ambarella_adj_aeb_bin_header_t));

	return 0;
}

#define TOTAL_STRUCT_NUM		32

int read_adv_header(int fd, u32 *psize)
{
	if (psize == NULL) {
		printf("Error: The point is NULL.\n");
		return -1;
	}

	u32 buf[TOTAL_STRUCT_NUM] = {0};
	int size = TOTAL_STRUCT_NUM * sizeof(u32) / sizeof(u8);

	if (read(fd, buf, size) < 0) {
		perror("read");
		return -1;
	}
	memcpy(psize, buf, size);

	return 0;
}

extern mw_aperture_param	G_lens_aperture;

int parse_lens_params(sensor_model_t *sensor)
{
	int plens_params_fd = -1;
	int error_flag = 0;
	off_t read_addr;
	int id_num;
	u32 plens_size_list[TOTAL_STRUCT_NUM];
	u8 *buf_lens[TOTAL_STRUCT_NUM] = {NULL};
	int item = 0;

	lens_param_t lens_param_info;
	lens_piris_step_table_t *tmp_piris_step = NULL;
	lens_piris_fno_scope_t *tmp_piris_scope = NULL;

	do {
		/* parse aeb binary file */
		if ((plens_params_fd = open(sensor->load_files.lens_file, O_RDONLY, S_IREAD))
			< 0) {
			error_flag = 1;
			printf("open file:%s error\n", sensor->load_files.lens_file);
			break;
		}

		if ((read_adv_header(plens_params_fd, plens_size_list)) < 0) {
			error_flag = 1;
			perror("read_header");
			break;
		}

		lseek(plens_params_fd, TOTAL_STRUCT_NUM * 4, SEEK_SET);

		for(id_num = PIRIS_DGAIN_TABLE; id_num < PIRIS_ID_NUM; id_num++) {
			read_addr = lseek(plens_params_fd, 0, SEEK_CUR);
			lseek(plens_params_fd, READ_ALIGN(read_addr, 32), SEEK_SET);
			if (plens_size_list[id_num] == 0) {
				continue;
			}
			if ((buf_lens[id_num] = (u8 *)malloc(plens_size_list[id_num])) == NULL) {
				error_flag = 1;
				perror("malloc");
				break;
			}
			memset(buf_lens[id_num], 0, plens_size_list[id_num]);
			read(plens_params_fd, buf_lens[id_num], plens_size_list[id_num]);
		}

		id_num = PIRIS_DGAIN_TABLE;
		while (id_num < PIRIS_ID_NUM) {
			if (buf_lens[id_num] == NULL) {
				break;
			}
			item = (int)(buf_lens[id_num][0] + (buf_lens[id_num][1] << 8) +
				(buf_lens[id_num][2] << 16) + (buf_lens[id_num][3] << 24));
			switch(item) {
				case PIRIS_DGAIN_TABLE:
					lens_param_info.piris_std.dgain =
						(lens_piris_dgain_table_t *)buf_lens[id_num];
					break;
				case PIRIS_FNO_SCOPE:
					tmp_piris_scope = (lens_piris_fno_scope_t *)buf_lens[id_num];
					lens_param_info.piris_std.scope = tmp_piris_scope->table;
					break;
				case PIRIS_STEP_TABLE:
					tmp_piris_step = (lens_piris_step_table_t *)(buf_lens[id_num]);
					lens_param_info.piris_std.table = tmp_piris_step->table;
					lens_param_info.piris_std.tbl_size = tmp_piris_step->header.array_size;
					break;
				default:
					printf("Error: The type:%d is unknown\n", item);
					error_flag = 1;
					break;
			}

			if (error_flag == 1) {
				break;
			}
			id_num++;
		}
	}while (0);

	img_load_lens_param(&lens_param_info);
	G_lens_aperture.FNO_min = (u32)lens_param_info.piris_std.scope[0];
	G_lens_aperture.FNO_max =  (u32)lens_param_info.piris_std.scope[1];

	for (id_num = 0; id_num < TOTAL_STRUCT_NUM; id_num++) {
		if (buf_lens[id_num] != NULL) {
			free(buf_lens[id_num]);
			buf_lens[id_num] = NULL;
		}
	}

	if (plens_params_fd > 0) {
		close(plens_params_fd);
		plens_params_fd = -1;
	}

	if (error_flag) {
		return -1;
	}

	return 0;
}

int get_sensor_aaa_params_from_bin(sensor_model_t * sensor)
{
	int adj_params_fd = -1;
	int aeb_params_fd = -1;
	u8 *buffer[LIST_TOTAL_NUM] = {NULL};
	int ret = 0;
	int error_flag = 0;
	int id_num, size, align_num, total_num;
	ambarella_adj_aeb_bin_parse_t parse_adj_bin, parse_aeb_bin;
	image_sensor_param_t sensor_adj_param;
	sensor_config_t *pSensor_param = NULL;
	ambarella_data_header_t  *pAdj_struct_info,  *pAeb_struct_info;
	off_t read_addr;

	do {
		/* parse adj binary file */
		if ((adj_params_fd = open(sensor->load_files.adj_file, O_RDONLY, S_IREAD))
			< 0) {
			ret = -1;
			printf("open file:%s error\n", sensor->load_files.adj_file);
			break;
		}

		if ((ret = read_header(adj_params_fd, &parse_adj_bin)) < 0) {
			perror("read_header");
			break;
		}

		pAdj_struct_info = &parse_adj_bin.data_header;
		for (id_num = LIST_CHROMA_SCALE; id_num < LIST_TILE_CONFIG; id_num++) {
			align_num = pAdj_struct_info->struct_type[id_num].struct_align;
			read_addr = lseek(adj_params_fd, 0, SEEK_CUR);
			lseek(adj_params_fd, READ_ALIGN(read_addr, align_num), SEEK_SET);

			size = pAdj_struct_info->struct_type[id_num].struct_size;
			total_num = pAdj_struct_info->struct_type[id_num].struct_total_num;
			if ((buffer[id_num] = (u8 *)malloc(READ_ALIGN(size, align_num) * total_num))
				== NULL) {
				error_flag = 1;
				perror("malloc");
				break;
			}
			memset(buffer[id_num], 0, READ_ALIGN(size, align_num)* total_num);
			read(adj_params_fd, buffer[id_num], READ_ALIGN(size, align_num) * total_num);

			lseek(adj_params_fd, size - READ_ALIGN(size, align_num), SEEK_CUR);

		}
		if (error_flag) {
			break;
		}

		/* parse aeb binary file */
		if ((aeb_params_fd = open(sensor->load_files.aeb_file, O_RDONLY, S_IREAD))
			< 0) {
			ret = -1;
			printf("open file:%s error\n", sensor->load_files.aeb_file);
			break;
		}

		if ((ret = read_header(aeb_params_fd, &parse_aeb_bin)) < 0) {
			perror("read_header");
			break;
		}

		pAeb_struct_info = &parse_aeb_bin.data_header;
		for (id_num = LIST_TILE_CONFIG; id_num < LIST_TOTAL_NUM; id_num++) {
			align_num = pAeb_struct_info->struct_type[id_num].struct_align;
			read_addr = lseek(aeb_params_fd, 0, SEEK_CUR);
			lseek(aeb_params_fd, READ_ALIGN(read_addr, align_num), SEEK_SET);

			size = pAeb_struct_info->struct_type[id_num].struct_size;
			total_num = pAeb_struct_info->struct_type[id_num].struct_total_num;
			if ((buffer[id_num] = (u8 *)malloc(READ_ALIGN(size, align_num) * total_num))
				== NULL) {
				error_flag = 1;
				perror("malloc");
				break;
			}
			memset(buffer[id_num], 0, READ_ALIGN(size, align_num)* total_num);
			read(aeb_params_fd, buffer[id_num], READ_ALIGN(size, align_num) * total_num);

			lseek(aeb_params_fd, size - READ_ALIGN(size, align_num), SEEK_CUR);
		}
		if (error_flag) {
			break;
		}

		/* copy the data to 3A struct */
		if ((pSensor_param = (sensor_config_t *)malloc(sizeof(sensor_config_t)))
			== NULL) {
			perror("malloc pSensor_param");
			ret = -1;
			break;
		}

		memset(pSensor_param, 0, sizeof(sensor_config_t));
		memcpy(pSensor_param, buffer[LIST_SENSOR_CONFIG] +
			pAeb_struct_info->struct_type[LIST_SENSOR_CONFIG].struct_size *
			sensor->offset_num[LIST_SENSOR_CONFIG], sizeof(sensor_config_t));

		pSensor_param->sensor_lb = sensor->sensor_id;

		if (img_config_sensor_info(pSensor_param) < 0) {
			printf("img_config_sensor_info error!\n");
			ret = -1;
			break;
		}

		printf("Sensor is %s Mode!\n", sensor->sensor_op_mode ? "HDR" : "Normal");

		if (img_config_sensor_hdr_mode(sensor->sensor_op_mode) < 0) {
			printf("img_config_sensor_hdr_mode error!\n");
			ret = -1;
			break;
		}

		memset(&sensor_adj_param, 0, sizeof(sensor_adj_param));
		sensor_adj_param.p_chroma_scale =
			(chroma_scale_filter_t *)(buffer[LIST_CHROMA_SCALE] +
			READ_ALIGN(pAdj_struct_info->struct_type[LIST_CHROMA_SCALE].struct_size,
			pAdj_struct_info->struct_type[LIST_CHROMA_SCALE].struct_align)
			* sensor->offset_num[LIST_CHROMA_SCALE]);

		sensor_adj_param.p_rgb2yuv = (rgb_to_yuv_t *)(buffer[LIST_RGB2YUV] +
			READ_ALIGN(pAdj_struct_info->struct_type[LIST_RGB2YUV].struct_size,
			pAdj_struct_info->struct_type[LIST_RGB2YUV].struct_align)
			* sensor->offset_num[LIST_RGB2YUV]);

		sensor_adj_param.p_adj_param = (adj_param_t *)(buffer[LIST_ADJ_PARAM] +
			READ_ALIGN(pAdj_struct_info->struct_type[LIST_ADJ_PARAM].struct_size,
			pAdj_struct_info->struct_type[LIST_ADJ_PARAM].struct_align)
			* sensor->offset_num[LIST_ADJ_PARAM]);

		sensor_adj_param.p_manual_LE = (local_exposure_t *)(buffer[LIST_MANUAL_LE] +
			READ_ALIGN(pAdj_struct_info->struct_type[LIST_MANUAL_LE].struct_size,
			pAdj_struct_info->struct_type[LIST_MANUAL_LE].struct_align)
			* sensor->offset_num[LIST_MANUAL_LE]);

		sensor_adj_param.p_tile_config = (statistics_config_t *)(buffer[LIST_TILE_CONFIG] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_TILE_CONFIG].struct_size,
			pAeb_struct_info->struct_type[LIST_TILE_CONFIG].struct_align)
			* sensor->offset_num[LIST_TILE_CONFIG]);

		sensor_adj_param.p_50hz_lines = (line_t *)(buffer[LIST_LINES] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_LINES].struct_size,
			pAeb_struct_info->struct_type[LIST_LINES].struct_align)
			* sensor->offset_num[LIST_LINES]);
		sensor_adj_param.p_60hz_lines = (line_t *)(buffer[LIST_LINES] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_LINES].struct_size,
			pAeb_struct_info->struct_type[LIST_LINES].struct_align)
			* (sensor->offset_num[LIST_LINES] + 1));

		sensor_adj_param.p_awb_param = (img_awb_param_t *)(buffer[LIST_AWB_PARAM] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_AWB_PARAM].struct_size,
			pAeb_struct_info->struct_type[LIST_AWB_PARAM].struct_align)
			* sensor->offset_num[LIST_AWB_PARAM]);

		sensor_adj_param.p_ae_agc_dgain = (u32 *)(buffer[LIST_AE_AGC_DGAIN] +
				READ_ALIGN(pAeb_struct_info->struct_type[LIST_AE_AGC_DGAIN].struct_size,
				pAeb_struct_info->struct_type[LIST_AE_AGC_DGAIN].struct_align)
				* sensor->offset_num[LIST_AE_AGC_DGAIN]);

		sensor_adj_param.p_ae_sht_dgain = (u32 *)(buffer[LIST_AE_SHT_DGAIN] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_AE_SHT_DGAIN].struct_size,
			pAeb_struct_info->struct_type[LIST_AE_SHT_DGAIN].struct_align)
			* sensor->offset_num[LIST_AE_SHT_DGAIN]);

		sensor_adj_param.p_dlight_range = (u8 *)(buffer[LIST_DLIGHT] +
			READ_ALIGN(pAeb_struct_info->struct_type[LIST_DLIGHT].struct_size,
			pAeb_struct_info->struct_type[LIST_DLIGHT].struct_align)
			* sensor->offset_num[LIST_DLIGHT]);

		if((ret = img_load_image_sensor_param(&sensor_adj_param)) < 0) {
			printf("img_load_image_sensor_param error!\n");
			break;
		}

	} while (0);

	for (id_num = LIST_CHROMA_SCALE; id_num < LIST_TOTAL_NUM; id_num++) {
		if (buffer[id_num] != NULL) {
			free(buffer[id_num]);
			buffer[id_num] = NULL;
		}
	}

	if (strcmp(sensor->load_files.lens_file, "") != 0) {
		parse_lens_params(sensor);
	}

	if (pSensor_param != NULL) {
		free(pSensor_param);
		pSensor_param = NULL;
	}

	if (aeb_params_fd > 0) {
		close(aeb_params_fd);
		aeb_params_fd = -1;
	}

	if (adj_params_fd > 0) {
		close(adj_params_fd);
		adj_params_fd = -1;
	}

	return ret;
}

