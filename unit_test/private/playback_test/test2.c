/*
 * test2.c
 *
 * History:
 *	2008/4/3 - [Oliver Li] created file
 *	2009/2/18 - [Cao Rongrong] add usb function
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include "types.h"
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

#undef SUPPORT_CAVLC

#ifdef SUPPORT_CAVLC
#include "cavlclib.h"
#endif
//#define USE_FRIENDLY_API
//#define CHECK_DELAY
#define ENABLE_RT_SCHED

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )


#define AM_IOCTL(_filp, _cmd, _arg) \
do { \
	printf("=== call "#_cmd" ...\n");	\
	if (ioctl(_filp, _cmd, _arg) < 0) {	\
		perror(#_cmd);	\
		return -1;	\
	}	\
}while (0)


/*
 * use following commands to enable timer1
 *

amba_debug -w 0x7000b014 -d 0xffffffff
amba_debug -w 0x7000b030 -d 0x15
amba_debug -r 0x7000b000 -s 0x40

*/

// the device file handle
int fd_iav;

int test_flag = 0;

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

// command line options

// vin
#include "../vin_test/vin_init.c"
#include "../vout_test/vout_init.c"

// preview
int preview_size_flag = 0;
int preview_width;
int preview_height;

//cache related
int cache_config = 0;
int disable_l2_in_idle_mode = 0;

// state control
int idle_cmd = 0;
int suspend_cmd = 0;
int resume_cmd = 0;
int preview_cmd = 0;
int decoding_cmd = 0;

// encode control
int encode_flag = 0;
int encode_frames = -1;
int encode_size = -1;

// "do not save to file" flag
int nofile_flag = 0;

// do not enter preview automatically
int nopreview_flag = 0;

// main stream encode
int main_flag = 0;
int main_encode_type;
int main_width;
int main_height;
int main_framerate = -1;

// secondary stream encode
int second_flag = 0;
int second_encode_type;
int second_width;
int second_height;
int second_framerate = -1;

// mjpeg quality
int jpeg_quality = 50;

// show state
int iav_state_flag = 0;

//force idr generation
int force_idr = 0;

//test stop and restart encoding (TEST ONLY)
int test_restart = 0;

int bitrate_change = 4000000;
int bitrate_change_flag = 0;

int frame_interval = 0;
int frame_interval_flag = 0;

int frame_interval2 = 0;
int frame_interval_flag2 = 0;

int refresh_period = 0;

int print_main_stream_pts_flag = 0;
int print_second_stream_pts_flag = 0;

int dump_idsp_bin_flag = 0;

driver_version_t driver_info;


// h.264 config
typedef struct h264_param_s {
	int h264_M;
	int h264_N;
	int h264_idr_interval;
	int h264_gop_model;
	int h264_bitrate;
	int h264_bitrate_control;
	int h264_calibration;
	int h264_vbrness;
	int h264_min_vbr;
	int h264_max_vbr;

	int h264_deblocking_filter_alpha;
	int h264_deblocking_filter_beta;
	int h264_deblocking_filter_disable;

	int h264_M_flag;
	int h264_N_flag;
	int h264_idr_interval_flag;
	int h264_gop_model_flag;
	int h264_bitrate_flag;
	int h264_bitrate_control_flag;
	int h264_calibration_flag;
	int h264_vbrness_flag;
	int h264_min_vbr_flag;
	int h264_max_vbr_flag;

	int h264_deblocking_filter_alpha_flag;
	int h264_deblocking_filter_beta_flag;
	int h264_deblocking_filter_enable_flag;

	int h264_entropy_codec;
	int h264_entropy_codec_flag;

	int force_intlc_tb_iframe;
	int force_intlc_tb_iframe_flag;

	int deintlc;
	int deintlc_flag;

	int h264_config_flag;
} h264_param_t;

h264_param_t main_h264_param;
h264_param_t second_h264_param;
h264_param_t third_h264_param;

// show h.264 config
int show_h264_config_flag = 0;

// bitstream filename
char filename[256];

struct encode_resolution_s {
	const char 	*name;
	int		width;
	int		height;
}
__encode_res[] =
{
	{"1080p", 1920, 1080},
	{"720p", 1280, 720},
	{"480p", 720, 480},
	{"576p", 720, 576},
	{"4sif", 704, 480},
	{"4cif", 704, 576},
	{"xga", 1024, 768},
	{"vga", 640, 480},
	{"cif", 352, 288},
	{"sif", 352, 240},
	{"qvga", 320, 240},
	{"qcif", 176, 144},
	{"qsif", 176, 120},
	{"qqvga", 160, 120},
	{"svga", 800, 600},
	{"sxga", 1280, 1024},

	{"", 0, 0},

	{"2048x1536", 2048, 1536},
	{"qxga", 2048, 1536},
	{"2048x1152", 2048, 1152},
	{"1920x1080", 1920, 1080},
	{"1440x1080", 1440, 1080},
	{"1366x768", 1366, 768},
	{"1280x1024", 1280, 1024},
	{"1280x960", 1280, 960},
	{"1280x720", 1280, 720},
	{"1024x768", 1024, 768},
	{"720x480", 720, 480},
	{"720x576", 720, 576},

	{"", 0, 0},

	{"704x480", 704, 480},
	{"704x576", 704, 576},
	{"640x480", 640, 480},
	{"352x288", 352, 288},
	{"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
	{"352x240", 352, 240},
	{"320x240", 320, 240},
        {"176x144", 176, 144},
        {"176x120", 176, 120},
        {"160x120", 160, 120},


	//for preview size only to keep aspect ratio in preview image for different VIN aspect ratio
	{"16_9_vin_ntsc_preview", 720, 360},
	{"16_9_vin_pal_preview", 720, 432},
	{"4_3_vin_ntsc_preview", 720, 480},
	{"4_3_vin_pal_preview", 720, 576},
	{"5_4_vin_ntsc_preview", 672, 480},
	{"5_4_vin_pal_preview", 672, 576 },
	{"ntsc_vin_ntsc_preview", 720, 480},
	{"pal_vin_pal_preview", 720, 576},
};

int get_encode_resolution(const char *name, int *width, int *height)
{
	int i;

	for (i = 0; i < sizeof(__encode_res) / sizeof(__encode_res[0]); i++)
		if (strcmp(__encode_res[i].name, name) == 0) {
			*width = __encode_res[i].width;
			*height = __encode_res[i].height;
			return 0;
		}

	printf("resolution '%s' not found\n", name);
	return -1;
}

int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

//	if (ioctl(fd_iav, IAV_IOC_MAP_BSB, &info) < 0) {
//		perror("IAV_IOC_MAP_BSB");
//		return -1;
//	}
	AM_IOCTL(fd_iav, IAV_IOC_MAP_BSB, &info);
	bsb_mem = info.addr;
	bsb_size = info.length;

	AM_IOCTL(fd_iav, IAV_IOC_MAP_DSP, &info);

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;
	return 0;
}

void print_encode_info(iav_h264_config_t *config)
{
	printf("\t     profile = %s\n", (config->entropy_codec == IAV_ENTROPY_CABAC)? "main":"baseline");
	printf("\t           M = %d\n", config->M);
	printf("\t           N = %d\n", config->N);
	printf("\tidr interval = %d\n", config->idr_interval);
	printf("\t   gop model = %s\n", (config->gop_model == 0)? "simple":"advanced");
	printf("\t     bitrate = %d bps\n", config->average_bitrate);
	printf("\tbitrate ctrl = %s\n", (config->bitrate_control == IAV_BRC_CBR)? "cbr":"vbr");
	if (config->bitrate_control == IAV_BRC_VBR) {
		printf("\tmin_vbr_rate = %d\n", config->min_vbr_rate_factor);
		printf("\tmax_vbr_rate = %d\n", config->max_vbr_rate_factor);
	}
	printf("\t    de-intlc = %s\n", (config->deintlc_for_intlc_vin==0)? "off":"on");


	printf("\t        ar_x = %d\n", config->pic_info.ar_x);
	printf("\t        ar_y = %d\n", config->pic_info.ar_y);
	printf("\t  frame mode = %d\n", config->pic_info.frame_mode);
	printf("\t        rate = %d\n", config->pic_info.rate);
	printf("\t       scale = %d\n", config->pic_info.scale);


}

int show_h264_config(int stream)
{
	iav_h264_config_t config;

	config.stream = stream;
	if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG, &config) < 0) {
		perror("IAV_IOC_GET_ENCODE_INFO");
		return -1;
	}

	print_encode_info(&config);

	return 0;
}

int h264_config(h264_param_t *param, int stream)
{
	iav_h264_config_t config;

	config.stream = stream;
	if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG, &config) < 0) {
		perror("IAV_IOC_GET_H264_CONFIG");
		return -1;
	}

	if (param->h264_M_flag)
		config.M = param->h264_M;

	if (param->h264_N_flag)
		config.N = param->h264_N;

	if (param->h264_idr_interval_flag)
		config.idr_interval = param->h264_idr_interval;

	if (param->h264_gop_model_flag)
		config.gop_model = param->h264_gop_model;

	if (param->h264_bitrate_flag)
		config.average_bitrate = param->h264_bitrate;

	if (param->h264_bitrate_control_flag)
		config.bitrate_control = param->h264_bitrate_control;

	if (param->h264_calibration_flag)
		config.calibration = param->h264_calibration;

	if (param->h264_vbrness_flag)
		config.vbr_ness = param->h264_vbrness;

	if (param->h264_min_vbr_flag)
		config.min_vbr_rate_factor = param->h264_min_vbr;

	if (param->h264_max_vbr_flag)
		config.max_vbr_rate_factor = param->h264_max_vbr;

	//deblocking filter params
	if (param->h264_deblocking_filter_alpha_flag)
		config.slice_alpha_c0_offset_div2 = param->h264_deblocking_filter_alpha;

	if (param->h264_deblocking_filter_beta_flag)
		config.slice_beta_offset_div2 = param->h264_deblocking_filter_beta;

	if (param->h264_deblocking_filter_enable_flag)
		config.deblocking_filter_disable = param->h264_deblocking_filter_disable;

	if (param->h264_entropy_codec_flag)
		config.entropy_codec = param ->h264_entropy_codec;

	//force first field or two fields to be I-frame setting
	if (param->force_intlc_tb_iframe_flag)
		config.force_intlc_tb_iframe = param->force_intlc_tb_iframe;

	//check deinterlacing flags
	if (param->deintlc_flag)
		config.deintlc_for_intlc_vin = param->deintlc;


	if (channel >= 0) {	// specified which channel to config
		AM_IOCTL(fd_iav, IAV_IOC_SET_H264_CONFIG, &config);
	}
	else {
		AM_IOCTL(fd_iav, IAV_IOC_SET_H264_CONFIG_ALL, &config);
	}

	return 0;
}

int goto_idle(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	printf("goto_idle done\n");
	return 0;
}

int enable_preview(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENABLE_PREVIEW, 0);
	printf("enable_preview done\n");
	return 0;
}

int enter_decoding(void)
{
	if (ioctl(fd_iav, IAV_IOC_START_DECODE, 0) < 0) {
		perror("IAV_IOC_START_DECODE");
		return -1;
	}

	printf("enter_decoding done\n");
	return 0;
}

const char *default_filename_nfs = "/mnt/media/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";
const char *default_filename;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tcp_flag = 0;
int port = 2000;

int create_file_tcp(const char *name)
{
	int sock;
	struct sockaddr_in sa;
	char fname[256];
	const char *host_ip_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("sock");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	host_ip_addr = getenv("HOST_IP_ADDR");
	if (host_ip_addr == NULL)
		host_ip_addr = default_host_ip_addr;
	sa.sin_addr.s_addr = inet_addr(host_ip_addr);
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("connect");
		return -1;
	}

	strcpy(fname, name);
	if (send(sock, fname, sizeof(fname), MSG_NOSIGNAL) < 0) {
		perror("send");
		return -1;
	}

	port++;
	return sock;
}

int create_file_nfs(const char *name)
{
	int fd;

	if ((fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(name);
		return -1;
	}

	return fd;
}


/*************************** USB ***********************************/
int fd_usb = 0;
int usb_flag = 0;
int usb_port = 2000;
int usb_pkt_size;
struct amb_usb_buf *usb_buf = NULL;

int usb_write(int port, const void *buf, int length)
{
	int rval = 0, len1, ret_len = 0;
	int actual_len = usb_pkt_size - USB_HEAD_SIZE;

	while(length > 0){

		len1  = length < actual_len ? length : actual_len;
		usb_buf->head.port_id = port;
		usb_buf->head.size = len1;
		usb_buf->head.flag1 = USB_PORT_OPEN;

		rval = write(fd_usb, usb_buf, USB_HEAD_SIZE);
		if(rval < 0){
			perror("usb_write");
			return rval;
		}

		//printf("port_id = %d: n = %d, size = %d\n", usb_buf->head.port_id,
		//	len1+USB_HEAD_SIZE, usb_buf->head.size);

		rval = write(fd_usb, buf + ret_len, len1);
		if(rval < 0){
			perror("usb_write");
			return rval;
		}

		ret_len += rval;
		length -= rval;
	}

	return ret_len;
}

/*
int request_usb_port(u16 port)
{
	int rval = 0;
	struct amb_usb_notify notify;

	notify.bNotifyType = PORT_STATUS_CHANGE;
	notify.port_id = port;
	notify.value = PORT_CONNECT;

	rval = ioctl(fd_usb, AMB_DATA_STREAM_STATUS_CHANGE, &notify);
	if (rval < 0) {
		perror("read command error");
 		return rval;
	}

	return port;
}
*/

int create_file_usb(const char *name)
{
	int rval = 0, port;
	char fname[256];

	//port = request_usb_port(usb_port);
	port = usb_port++;

	strcpy(fname, name);
	rval = usb_write(port, fname, sizeof(fname));
	if(rval < 0)
		return -1;

	return port;
}

int set_response(int response)
{
	struct amb_usb_rsp rsp;

	rsp.signature = AMB_STATUS_TOKEN;
	rsp.response = response;

	if (ioctl(fd_usb, AMB_DATA_STREAM_WR_RSP, &rsp) < 0) {
		perror("ioctl error");
		return -1;
	}

	return 0;
}

int get_usb_command(struct amb_usb_cmd *cmd)
{
	int rval = 0;

	rval = ioctl(fd_usb, AMB_DATA_STREAM_RD_CMD, cmd);
	if (rval < 0) {
		perror("read command error");
 		return rval;
	}

	if (cmd->signature != AMB_COMMAND_TOKEN) {
		printf("Wrong signature: %08x\n", cmd->signature);
		return -EINVAL;
	}

	//printf("signature = 0x%x, command = 0x%x, parameter[0] = %d\n",
	//	cmd->signature, cmd->command, cmd->parameter[0]);

	return 0;
}

int init_usb(void)
{
	int rval = 0;
	struct amb_usb_cmd cmd;

	fd_usb = open("/dev/amb_gadget", O_RDWR);
	if (fd_usb < 0) {
		perror("can't open device");
		return -1;
	}

	/* Read usb command */
	get_usb_command(&cmd);

	/* Process usb command */
	switch (cmd.command)
	{
	case USB_CMD_SET_MODE:

		usb_pkt_size = cmd.parameter[1];
		usb_buf = malloc(usb_pkt_size);
		/* response to the host */
		set_response(AMB_RSP_SUCCESS);
		break;

	default:
		printf ("Unknown command: %08x\n", cmd.command);
		rval = -1;
		break;
	}

	return rval;
}

/*******************************************************************/

int write_data(int fd, void * data, int bytes)
{
	int ret;
	if (nofile_flag)
		return 0;

	if (usb_flag){
		if ((ret = usb_write(fd, data, bytes)) < 0) {
			perror("usb_write");
			return -1;
		}
	} else {
		if ((ret = write(fd, data, bytes)) < 0 ) {
			perror("write");
			return -1;
		}
	}

	return ret;
}
int create_file(const char *name)
{
	if (tcp_flag)
		return create_file_tcp(name);
	else if(usb_flag)
		return create_file_usb(name);
	else
		return create_file_nfs(name);
}


int write_file(int fd, bits_info_t *desc, u32 *size)
{
	static unsigned int whole_pic_size=0;
	u32 pic_size = desc->pic_size;

	//remove align

	whole_pic_size  += (pic_size & (~(1<<23)));

	if(pic_size>>23){
		//end of frame
		pic_size = pic_size & (~(1<<23));
	 	 //padding some data to make whole picture to be 32 byte aligned
		pic_size += (((whole_pic_size + 31) & ~31)- whole_pic_size);
		//rewind whole pic size counter
		// printf("whole %d, pad %d \n", whole_pic_size, (((whole_pic_size + 31) & ~31)- whole_pic_size));
		 whole_pic_size = 0;
	}

	if (size != NULL)
		*size = pic_size;
	if (desc->start_addr + pic_size <= (u32)bsb_mem + bsb_size) {
		if (write_data(fd, (void*)desc->start_addr, pic_size) < 0) {
				perror("write(1)");
				return -1;
		}
	} else {
		u32 size = (u32)bsb_mem + bsb_size - desc->start_addr;
		u32 remain = pic_size - size;
		if (write_data(fd, (void*)desc->start_addr, size) < 0) {
			perror("write(2)");
			return -1;
		}
		if (write_data(fd, bsb_mem, remain) < 0) {
			perror("write(3)");
			return -1;
		}
	}

	return 0;
}

int write_h264_header_info(int fd, int stream)
{
	iav_h264_config_t config;
	int version = 0x00000004;
	int size = sizeof(config);

	config.stream = stream;
	if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG, &config) < 0) {
		perror("IAV_IOC_GET_H264_CONFIG");
		return -1;
	}

	if (write_data(fd, &version, sizeof(version)) < 0 ||
				write_data(fd, &size, sizeof(size)) < 0 ||
				write_data(fd, &config, sizeof(config)) < 0) {
			perror("write_data(4)");
			return -1;
	}

	print_encode_info(&config);

	return 0;
}

int stop_encode(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_STOP_ENCODE, 0);
	printf("stop_encode\n");
	return 0;
}

#ifdef CHECK_DELAY

#include "amba_debug.h"
char *debug_mem;
struct amba_debug_mem_info debug_mem_info;

void init_timer(void)
{
	static int init = 0;
	int fd;

	if (init)
		return;

	fd = open("/dev/ambad", O_RDWR, 0);
	if (fd < 0) {
		perror("/dev/ambad");
		return;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_MEM_INFO, &debug_mem_info) < 0) {
		perror("AMBA_DEBUG_IOC_GET_MEM_INFO");
		return;
	}

	debug_mem = (char *)mmap(NULL, (debug_mem_info.ahb_size +
		debug_mem_info.apb_size + debug_mem_info.ddr_size),
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (debug_mem == MAP_FAILED) {
		perror("mmap");
		return;
	}
}

#endif

typedef struct video_frame_s {
	u32	size;
	u32	pts;
	u32	flags;
	u32	seq;
} video_frame_t;

#ifdef USE_FRIENDLY_API

int write_stream(int fd_h264, int fd_h264_size, int fd_mjpeg, int fd_mjpeg_size,
	int *total_frames, int *total_bytes)
{
	u32 i;
	iav_bs_fifo_info_t bs_info;
	iav_bits_info_t desc[32];
	iav_bits_info_t *pdesc;
	int fd, fd_size;
	video_frame_t frame;

	bs_info.count = sizeof(desc)/sizeof(desc[0]);
	bs_info.descs = &desc[0];
	if (iav_read_bsb(fd_iav, &bs_info) < 0) {
		perror("iav_read_bsb");
		return -1;
	}

	pdesc = bs_info.descs;
	for (i = 0; i < bs_info.count; i++, pdesc++) {
		if (pdesc->pic_type == JPEG_STREAM) {
			fd = fd_mjpeg;
			fd_size = fd_mjpeg_size;
		} else {
			fd = fd_h264;
			fd_size = fd_h264_size;
		}

		if (write(fd, pdesc->start_addr, pdesc->size) < 0) {
			perror("write 1");
			return -1;
		}

		if (pdesc->more) {
			if (write(fd, pdesc->start_addr2, pdesc->size2) < 0) {
				perror("write 2");
				return -1;
			}
		}


		frame.size = pdesc->pic_size;
		frame.pts = pdesc->PTS;
		frame.flags = pdesc->pic_type;
		frame.seq = *total_frames;
		if (write(fd_size, &frame, sizeof(frame)) < 0) {
			perror("write");
			return -1;
		}

		// statistics
		(*total_frames)++;
		(*total_bytes) += pdesc->pic_size;
		if (*total_frames % 30 == 0) {
			printf("frames %d, bytes = %d\n", *total_frames, *total_bytes);
		}
	}

	return 0;
}

#else

#ifdef CHECK_DELAY

u32 read_counter(void)
{
	return *(u32 *)((0x7000b010 - debug_mem_info.apb_start +
		debug_mem_info.ahb_size) + debug_mem);
}

void print_delay(bs_fifo_info_t *fifo_info)
{
	static u32 frames = 0;
	static u32 max = 0;
	static u32 maxmax = 0;

	u32 counter = read_counter();
	u32 delta = fifo_info->rt_counter - counter;

	if (delta > max)
		max = delta;

	frames += fifo_info->count;
	if ((frames % 30) == 0) {
		if (max > maxmax)
			maxmax = max;
		printf("=== delay = %d, max = %d\n", max, maxmax);
		max = 0;
	}
}

#endif

int write_stream(int fd_main, int fd_main_size, int fd_second, int fd_second_size, int fd_third,  int fd_third_size,
	int *total_frames, int *total_bytes)
{
	u32 i;
	bs_fifo_info_t fifo_info;
	video_frame_t frame;
	bits_info_t *desc;
	int fd, fd_size;
	int print_interval = 30;
	static u64 main_pts = 0;
	static u64 second_pts = 0;

	static u32 curr_main_frames = 0;
	static u32 curr_second_frames = 0;
	static u32 curr_third_frames = 0;
	static u32 pre_main_frames = 0;
	static u32 pre_second_frames = 0;
	//static u32 pre_third_frames = 0;


	static u32 curr_main_bytes = 0;
	static u32 curr_second_bytes = 0;
	static u32 pre_main_bytes = 0;
	static u32 pre_second_bytes = 0;

	static u32 time_interval_us;
	static struct timeval pre_time = {0,0};
	static struct timeval curr_time = {0,0};
	u32 *curr_bytes = NULL;
	//u32 *pre_bytes;

	if (ioctl(fd_iav, IAV_IOC_READ_BITSTREAM, &fifo_info) < 0) {

		if (errno != EAGAIN)
			perror("IAV_IOC_READ_BITSTREAM");

		printf("read bitstream done\n");
		return -1;
	}
	gettimeofday(&curr_time, NULL);

#ifdef CHECK_DELAY
	print_delay(&fifo_info);
#endif

	desc = &fifo_info.desc[0];

	if (second_flag)
		print_interval += 30;

	for (i = 0; i < fifo_info.count; i++, desc++)
	{
		u32 pic_size = 0;

		/* debug information for force idr
		if (desc->pic_type == IDR_FRAME) {
			printf("IDR:%d\n", *total_frames + 1);
		} */

		// select file
		if (desc->pic_type == JPEG_STREAM) 	// M-JPEG
		{
			//current method is triple streaming is H.264 +H.264 +MJPEG only, so whenever there is triple streaming,
			//MJPEG is always on third stream,  may need other way to distinguish if we support H.264 +MJPEG+MJPEG type
			//of triple streaming
			if (fd_third > 0) {
				fd = fd_third;
				fd_size = fd_third_size;
				curr_third_frames++;
			} else if (fd_second > 0) {
				fd = fd_second;
				fd_size = fd_second_size;
				curr_second_frames++;

				if (print_second_stream_pts_flag) {
					printf("2nd MJPEG PTS:%lld, diff:%lld, frames No: %d \n", (u64)desc->PTS, desc->PTS - second_pts, *total_frames);
					second_pts = desc->PTS;
				}
				curr_bytes = &curr_second_bytes;
				//pre_bytes = &pre_second_bytes;

			} else {
				fd = fd_main;
				fd_size = fd_main_size;
				curr_main_frames++;

				if (print_main_stream_pts_flag) {
					printf("main MJPEG PTS:%lld, diff:%lld, frames No: %d\n", (u64)desc->PTS, desc->PTS - main_pts, *total_frames);
					main_pts = desc->PTS;
				}
				curr_bytes = &curr_main_bytes;
				//pre_bytes = &pre_main_bytes;
			}
		}
		else
		{	// H.264
#ifdef SUPPORT_CAVLC
			u8 *frame_start;
			u32 frame_size;
#endif


			if ((desc->frame_num & 0xC0000000) == 0)
			{
				fd = fd_main;
				fd_size = fd_main_size;
				curr_main_frames++;

				if (print_main_stream_pts_flag) {
					printf("main H264 PTS:%lld, diff:%lld, frames No: %d \n", (u64)desc->PTS, desc->PTS - main_pts, *total_frames);
					main_pts = desc->PTS;
				}
				curr_bytes = &curr_main_bytes;
				//pre_bytes = &pre_main_bytes;
			}
			else
			{
				fd = fd_second;
				fd_size = fd_second_size;
				curr_second_frames++;

				if (print_second_stream_pts_flag) {
					printf("2nd H264 PTS:%lld,  diff:%lld, frames No: %d \n", (u64)desc->PTS, desc->PTS - second_pts, *total_frames);
					second_pts = desc->PTS;
				}
				curr_bytes = &curr_second_bytes;
				//pre_bytes = &pre_second_bytes;
			}

#ifdef SUPPORT_CAVLC
			if(desc->cavlc_pjpeg ) {
				//printf("type %d, size %d, start addr 0x%x \n", desc->pic_type, desc->pic_size, desc->start_addr);
				if (cavlc_encode_frame(fd_iav, desc, &frame_start, &frame_size) < 0) {
					printf("cavlc_encode_frame failed \n");
					return -1;
				}
				//fill the descriptor with new encoded CAVLC frame
				desc->start_addr 	= (u32)frame_start;
				desc->pic_size 	= frame_size;
			}
#endif
		}

		pic_size = desc->pic_size;

		if (desc->cavlc_pjpeg) {
			//no need to consider data wrap up for cavlc encoded data, because it's frame by frame, never wrap up
			write_data(fd, (void *)desc->start_addr, desc->pic_size);
			pic_size = desc->pic_size;
		} else {
			if (write_file(fd, desc, &pic_size) < 0)
				return -1;
		}

		frame.size = desc->pic_size;
		frame.pts = desc->PTS;
		frame.flags = desc->pic_type;
		frame.seq = *total_frames;

		if (write_data(fd_size, &frame, sizeof(frame)) < 0 ) {
			perror("write_data");
			return -1;
		}

		(*total_frames)++;

		(*total_bytes) += pic_size;
		*curr_bytes += pic_size;

		if (*total_frames % print_interval == 0)
		{
			time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 + curr_time.tv_usec - pre_time.tv_usec;

			if (curr_main_frames)
				printf("main:\t%4d %s, %2d fps, %10d bytes, %5d kbps\n", curr_main_frames, nofile_flag ? "discard" : "frames",
				DIV_ROUND((curr_main_frames - pre_main_frames) * 1000000, time_interval_us), curr_main_bytes,
				pre_time.tv_sec ? (int)((curr_main_bytes - pre_main_bytes) * 8000000LL /time_interval_us /1024) : 0);

//			if (second_flag)

			if(curr_second_frames)
				printf("second:\t%4d %s, %2d fps, %10d bytes, %5d kbps\n", curr_second_frames, nofile_flag ? "discard" : "frames",
				DIV_ROUND((curr_second_frames - pre_second_frames) * 1000000, time_interval_us), curr_second_bytes,
				pre_time.tv_sec ? (int)((curr_second_bytes - pre_second_bytes) * 8000000LL /time_interval_us /1024) : 0);
/*
			if (third_flag)
				printf("third %d \n", curr_third_frames);
			printf("\n");
*/
			pre_time = curr_time;
			pre_main_frames = curr_main_frames;
			pre_second_frames = curr_second_frames;
			//pre_third_frames = curr_third_frames;
			pre_main_bytes = curr_main_bytes;
			pre_second_bytes = curr_second_bytes;

		}
	}

	return 0;
}
#endif

int dump_idsp_bin(void)
{
	int fd;
	u8* buffer;
	iav_dump_idsp_info_t dump_info;
	buffer = malloc(1024*8);
	char full_filename[256];

	dump_info.mode = 1;
	dump_info.pBuffer = buffer;
	if (ioctl(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info) < 0) {
		perror("IAV_IOC_DUMP_IDSP_INFO");
		return -1;
	}
	sleep(1);
	dump_info.mode = 2;
	if (ioctl(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info) < 0) {
		perror("IAV_IOC_DUMP_IDSP_INFO");
		return -1;
	}
	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	sprintf(full_filename, "%s.bin", filename);

	if ((fd = create_file(full_filename)) < 0)
		return -1;

	if (write_data(fd, buffer, 1024*8) < 0) {
		perror("write(5)");
		return -1;
	}
	dump_info.mode = 0;
	if (ioctl(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info) < 0) {
		perror("IAV_IOC_DUMP_IDSP_INFO");
		return -1;
	}

	free(buffer);
	return 0;
}

static const char *get_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "preview";
	case 2:	return "encoding";
	case 3:	return "still capture";
	case 4:	return "decoding";
	default: return "???";
	}
}

static const char *get_dsp_op_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "encode mode";
	case 1:	return "decode mode";
	case 2:	return "reset mode";
	default: return "???";
	}
}

static const char *get_dsp_encode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "busy";
	case 2:	return "pause";
	case 3:	return "flush";
	case 4:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_encode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stop";
	case 1:	return "video";
	case 2:	return "sjpeg";
	case 3:	return "mjpeg";
	case 4:	return "fast3a";
	case 5:	return "rjpeg";
	case 6:	return "timer";
	case 7:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "h264dec";
	case 2:	return "h264dec idle";
	case 3:	return "transit h264dec to idle";
	case 4:	return "transit h264dec to h264dec idle";
	case 5:	return "jpeg still";
	case 6:	return "transit jpeg still to idle";
	case 7:	return "multiscene";
	case 8:	return "transit multiscene to idle";
	case 9:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stopped";
	case 1:	return "idle";
	case 2:	return "jpeg";
	case 3:	return "h.264";
	case 4:	return "multi";
	default: return "???";
	}
}

int show_state(void)
{
	iav_state_info_t info;

	if (ioctl(fd_iav, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	printf("  vout_irq_count = %d\n", info.vout_irq_count);
	printf("   vin_irq_count = %d\n", info.vin_irq_count);
	printf("  vdsp_irq_count = %d\n", info.vdsp_irq_count);
	printf("           state = %d [%s]\n",
		info.state, get_state_str(info.state));
	printf("     dsp_op_mode = %d [%s]\n",
		info.dsp_op_mode, get_dsp_op_mode_str(info.dsp_op_mode));
	printf("dsp_encode_state = %d [%s]\n",
		info.dsp_encode_state, get_dsp_encode_state_str(info.dsp_encode_state));
	printf(" dsp_encode_mode = %d [%s]\n",
		info.dsp_encode_mode, get_dsp_encode_mode_str(info.dsp_encode_mode));
	printf("dsp_decode_state = %d [%s]\n",
		info.dsp_decode_state, get_dsp_decode_state_str(info.dsp_decode_state));
	printf("    decode_state = %d [%s]\n",
		info.decode_state, get_dsp_decode_mode_str(info.decode_state));
	printf(" encode timecode = %d\n", info.encode_timecode);
	printf("      encode pts = %d\n", info.encode_pts);

	return 0;
}

#define	NO_ARG				0
#define	HAS_ARG				1
#define	SYSTEM_OPTIONS_BASE		0
#define	VIN_OPTIONS_BASE		10
#define	VOUT_OPTIONS_BASE		20
#define	DECODING_OPTIONS_BASE		190

enum numeric_short_options {
	//System
	SYSTEM_IDLE = SYSTEM_OPTIONS_BASE,
	SHOW_IAV_STATE,
	SUSPEND_DSP,
	RESUME_DSP,

	//test with L2 cache settings
	DISABLE_L2_IN_IDLE_MODE,

	//Vin
	VIN_NUMVERIC_SHORT_OPTIONS,

	//Vout
	VOUT_NUMERIC_SHORT_OPTIONS,

	//Decoding
	START_DECODING = DECODING_OPTIONS_BASE,
};

static struct option long_options[] = {
	//System
	{"idle",                  NO_ARG,   0,   SYSTEM_IDLE,                            },
	{"state",                 NO_ARG,   0,   SHOW_IAV_STATE,                         },
	{"suspend",               NO_ARG,   0,   SUSPEND_DSP,                            },
	{"resume",                NO_ARG,   0,   RESUME_DSP,                             },
	{"disablel2",                HAS_ARG,   0,   DISABLE_L2_IN_IDLE_MODE,                },


	//Vin
	VIN_LONG_OPTIONS()

	//Vout
	VOUT_LONG_OPTIONS()

	//Decoding
	{"decoding",              NO_ARG,   0,   START_DECODING,                         },

	{0,                       0,        0,   0                                       },
};

static const char *short_options = "ef:h:i:m:p:q:v:F:H:J:M:N:S:C:V:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	//System
	{"", "\t\tenter idle state"},
	{"", "\t\tshow driver state"},
	{"", "\t\tsuspend dsp"},
	{"", "\t\tresume dsp"},
	{"", "\t\tdisablel2, disable l2 cache in idle mode"},

	//Vin
	VIN_PARAMETER_HINTS()

	//Vout
	VOUT_PARAMETER_HINTS()

	//Decoding
	{"", "\t\tenter decoding state"},
};

void usage(void)
{
	int i;

	printf("test2 usage:\n");
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

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("<nil>\n");
		} else
			printf("%s\n", __vin_modes[i].name);
	}
	printf("\n\n");

	printf("vout mode:  ");
	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		printf("%s\n", vout_res[i].name);
	printf("\n\n");

	printf("resolution:  ");
	for (i = 0; i < sizeof(__encode_res)/sizeof(__encode_res[0]); i++) {
		if (__encode_res[i].name[0] == '\0') {
			printf("<nil>\n");
		} else
			printf("%s\n", __encode_res[i].name);
	}
	printf("\n\n");
}

h264_param_t *curr_param = &main_h264_param;

int get_gop_model(const char *name)
{
	if (strcmp(name, "simple") == 0)
		return IAV_GOP_SIMPLE;

	if (strcmp(name, "advanced") == 0)
		return IAV_GOP_ADVANCED;

	printf("invalid gop model: %s\n", name);
	return -1;
}

int get_bitrate_control(const char *name)
{
	int quant;

	if (strcmp(name, "cbr") == 0)
		return IAV_BRC_CBR;

	if (strcmp(name, "vbr") == 0)
		return IAV_BRC_VBR;

	quant = atoi(name);
	if (quant < 52 || quant > 104) {
		printf("invalid bitrate control: %s\n", name);
		return -1;
	}

	return quant;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		//System
		case SYSTEM_IDLE:
			idle_cmd = 1;
			break;
		case SHOW_IAV_STATE:
			iav_state_flag = 1;
			break;
		case SUSPEND_DSP:
			suspend_cmd = 1;
			break;
		case RESUME_DSP:
			resume_cmd = 1;
			break;
		case DISABLE_L2_IN_IDLE_MODE:
			disable_l2_in_idle_mode = atoi(optarg);
			printf("disable l2 in idle mode %d\n", disable_l2_in_idle_mode);
			cache_config = 1;
			break;

		//Vin
		VIN_INIT_PARAMETERS()

		//Vout
		VOUT_INIT_PARAMETERS()

		//Decoding
		case START_DECODING:
			decoding_cmd = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}


int change_frame_rate(void)
{
#if 0
	iav_change_framerate_t	change_framerate;


	if(frame_interval_flag) {
		change_framerate.frame_interval[0] = frame_interval;
		printf("change main frame interval to %d \n", frame_interval);
	}
	else
		change_framerate.frame_interval[0] = 1;

	if(frame_interval_flag2) {
		change_framerate.frame_interval[1] = frame_interval2;
		printf("change second frame interval to %d \n", frame_interval2);
	}
	else
		change_framerate.frame_interval[1] = change_framerate.frame_interval[0];


	if(ioctl(fd_iav, IAV_IOC_CHANGE_FRAME_RATE, &change_framerate ) < 0) {
		perror("Error in IAV_IOC_CHANGE_FRAME_RATE , frame_interval2 must be integer multiple " \
			"of frame_interval, and M must be 1 for both main and secondary H.264\t");

		return -1;
	}
	else {
		if(frame_interval > 0)
			printf("main frame rate changed to be 1/%d of VIN \n", frame_interval );
		if(frame_interval2 > 0)
			printf("second frame rate changed to be 1/%d of VIN \n", frame_interval2 );
		return 0;
	}
#else
	return 0;
#endif
}

static void sigstop()
{
	int state;

	if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		exit(2);
	}

	//if it is encoding, then stop encoding
	if (state == IAV_STATE_ENCODING) {
		if (stop_encode() < 0) {
			printf("Cannot stop encoding...\n");
			exit(3);
		}
	}
	exit(1);
}


int main(int argc, char **argv)
{
	// register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (cache_config) {
		iav_cache_config_t cache_config;
		cache_config.disable_l2_in_idle_mode = disable_l2_in_idle_mode;
		cache_config.disable_l2_in_non_idle_mode = 0;
		cache_config.try_512k_in_udec_mode = 0;
		printf("IAV_IOC_CONFIG_L2CACHE: disable l2 in idle mode %d.\n", cache_config.disable_l2_in_idle_mode);
		ioctl(fd_iav, IAV_IOC_CONFIG_L2CACHE, &cache_config);
	}

	// Suspend or Resume DSP
	if (suspend_cmd) {
		return ioctl(fd_iav, IAV_IOC_SUSPEND_DSP, NULL);
	}
	if (resume_cmd) {
		return ioctl(fd_iav, IAV_IOC_RESUME_DSP, NULL);
	}

	// Dynamically change vout
	if (dynamically_change_vout())
		return 0;

	// select channel: for A3 DVR
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}


	if (dump_idsp_bin_flag) {
		if (dump_idsp_bin() < 0){
			perror("dump_idsp_bin failed");
			return -1;
		}
		return 0;
	}

	// enter idle state command
	if (vout_flag[VOUT_0] || vin_flag || idle_cmd) {
		if (goto_idle() < 0)
			return -1;
	}

	//check vout
	if (check_vout() < 0)
		return -1;

	//Init vout
	if (vout_flag[VOUT_0] && (init_vout(VOUT_0, 1) < 0 || post_init_vout() < 0))
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 1) < 0)
		return -1;

	// set vin
	if (vin_flag) {
		if (init_vin(vin_mode) < 0)
			return -1;
	}

	return 0;
}

