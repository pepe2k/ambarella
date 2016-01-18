/***********************************************************
 * image_server.c
 *
 * History:
 *	2010/03/25 - [Jian Tang] created file
 *
 * Copyright (C) 2008-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ***********************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <getopt.h>
#include <sched.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.h"

#include "ambas_common.h"
#include "ambas_vin.h"
#include "ambas_vout.h"

//#include "img_struct.h"
#include "img_struct_arch.h"
#include "img_api_arch.h"

#include "mw_struct.h"
#include "mw_api.h"

#include "../mediaserver/defines.h"
#include "image_param.h"


#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libdaemon/dfork.h>
#include <libdaemon/dsignal.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>
#include <libdaemon/dexec.h>
#include <linux/netlink.h>
#endif

#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
static int send_image_msg_to_kernel(nl_image_msg_t nl_msg);
#endif

#if 0
static u8						g_buffer[BUFFER_SIZE];
#endif
static basic_iq_params			g_mw_basic_iq;
static mw_image_param		g_mw_image;
static mw_awb_param			g_mw_awb;
static mw_af_param			g_mw_af;
static u32						g_mw_encode_mode = HIGH_FRAMERATE;


// Init parameter
static struct option long_options[] = {
	{"help", NO_ARG, 0, 'h'},
	{"mode", HAS_ARG, 0, 'm'},
	{"cfg-file", HAS_ARG, 0, 'f'},
	{"log-level", HAS_ARG, 0, 'l'},
	{0, 0, 0, 0},
};

static const struct hint_s hint[] = {
	{"", "\tshow usage"},
	{"0~2", "\tset encode mode, 0:high frame rate; 1:low delay; 2:high MP encoding"},
	{"file", "specify configuration file"},
	{"", "\tset log info level (0: error, 1: message, 2: info)"},
};

static const char * short_options = "hm:f:l:";

static mqd_t msg_queue_send, msg_queue_receive;
static MESSAGE *send_buffer = NULL;
static MESSAGE *receive_buffer = NULL;

static void  receive_cb_image(union sigval sv);

static void NotifySetup(mqd_t *mqdp)
{
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
	sev.sigev_notify_function = receive_cb_image;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
	if (mq_notify(*mqdp, &sev) == -1){
		perror("mq_notify");
	}else {
		APP_INFO("NotifySetup Done!\n");
	}
}

static int set_local_exposure_mode(u32 mode)
{
	if (mode != LE_CUSTOMER) {
		if (mw_set_auto_local_exposure_mode(mode) < 0) {
			APP_ERROR("mw_set_local_exposure_curve error.\n");
			return -1;
		}
	} else {
		if (img_set_auto_local_exposure(0) < 0) {
			APP_ERROR("img_set_auto_local_exposure error.\n");
			return -1;
		}
		sleep(1);
		if (mw_set_local_exposure_curve(fd_iav,
			&G_local_exposure[2]) < 0) {
			APP_ERROR("mw_set_local_exposure_curve error.\n");
			return -1;
		}
	}
	return 0;
}

static int get_lib_image_params(void)
{
	if (mw_get_auto_local_exposure_mode(&g_mw_basic_iq.local_exposure_mode) < 0) {
		APP_ERROR("mw_get_auto_local_exposure_mode error\n");
		return -1;
	}
	iq_map.local_exposure_mode = g_mw_basic_iq.local_exposure_mode;
	if (mw_get_mctf_strength(&g_mw_basic_iq.mctf_strength) < 0) {
		APP_ERROR("mw_get_mctf_strength error\n");
		return -1;
	}
	iq_map.mctf_strength = g_mw_basic_iq.mctf_strength;

	if (mw_get_exposure_level((int*)&g_mw_basic_iq.ae_target_ratio) < 0) {
		APP_ERROR("mw_get_exposure_level error\n");
		return -1;
	}
	iq_map.ae_target_ratio = g_mw_basic_iq.ae_target_ratio;

	iq_map.day_night_mode = 0;
	iq_map.back_light_comp_enable = 0;
	iq_map.dc_iris_enable = 0;

	return 0;
}

static int set_basic_image_params(basic_iq_params * iq_params)
{
	if (g_mw_basic_iq.day_night_mode != iq_params->day_night_mode) {
		if (mw_enable_day_night_mode(iq_params->day_night_mode) < 0) {
			APP_ERROR("mw_enable_day_night_mode error\n");
			return -1;
		}
	}
	#if 0
	if (g_mw_basic_iq.metering_mode != iq_params->metering_mode) {
		if (mw_set_ae_metering_mode(iq_params->metering_mode) < 0) {
			APP_ERROR("mw_set_ae_metering_mode error\n");
			return -1;
		}
	}
	#endif
	if (g_mw_basic_iq.back_light_comp_enable != iq_params->back_light_comp_enable) {
		if (mw_enable_backlight_compensation(iq_params->back_light_comp_enable) < 0) {
			APP_ERROR("mw_enable_backlight_compensation error\n");
			return -1;
		}
	}
	if (g_mw_basic_iq.local_exposure_mode != iq_params->local_exposure_mode) {
		if (set_local_exposure_mode(iq_params->local_exposure_mode) < 0) {
			APP_ERROR("set_local_exposure_mode error\n");
			return -1;
		}
	}
	if (g_mw_basic_iq.mctf_strength != iq_params->mctf_strength) {
		if (mw_set_mctf_strength(iq_params->mctf_strength) < 0) {
			APP_ERROR("mw_set_mctf_strength error\n");
			return -1;
		}
	}
	if (g_mw_basic_iq.dc_iris_enable != iq_params->dc_iris_enable) {
		if (mw_enable_dc_iris_control(iq_params->dc_iris_enable) < 0) {
			APP_ERROR("mw_enable_dc_iris_control error\n");
			return -1;
		}
	}
	if (g_mw_basic_iq.ae_target_ratio != iq_params->ae_target_ratio) {
		if (mw_set_exposure_level(iq_params->ae_target_ratio) < 0) {
			APP_ERROR("mw_set_exposure_level error\n");
			return -1;
		}
	}

	return 0;
}

static int set_image_param(char * section_name)
{
	APP_INFO("Section [%s] settings:\n", section_name);

	#if 0
	if (g_mw_basic_iq.ae_preference != iq_map.ae_preference) {
		if (mw_set_iq_preference(iq_map.ae_preference) < 0) {
			APP_PRINTF("mw_set_iq_preference error\n");
		}
		g_mw_basic_iq.ae_preference = iq_map.ae_preference;
	}
	#endif
	if (0 != memcmp(&g_mw_basic_iq.ae, &iq_map.ae, sizeof(g_mw_basic_iq.ae))) {
		if (mw_set_ae_param(&iq_map.ae) < 0) {
			APP_ERROR("mw_set_ae_param error\n");
			return -1;
		} else {
			g_mw_basic_iq.ae = iq_map.ae;
		}
	}
	if (0 != memcmp(&g_mw_image, &image_map, sizeof(g_mw_image))) {
		if (mw_set_image_param(&image_map) < 0) {
			APP_ERROR("mw_set_image_param error\n");
			return -1;
		} else {
			g_mw_image = image_map;
		}
	}
	if (0 != memcmp(&g_mw_awb, &awb_map, sizeof(g_mw_awb))) {
		if (mw_set_awb_param(&awb_map) < 0) {
			APP_ERROR("mw_set_awb_param error\n");
			return -1;
		} else {
			g_mw_awb = awb_map;
		}
	}
	if (0 != memcmp(&g_mw_basic_iq, &iq_map, sizeof(g_mw_basic_iq))) {
		if (set_basic_image_params(&iq_map) < 0) {
			APP_ERROR("set_basic_image_params error\n");
			return -1;
		} else {
			g_mw_basic_iq = iq_map;
		}
	}

	return 0;
}

#if 0
static int set_af_param(char * section_name)
{
	APP_INFO("Section [%s] settings:\n", section_name);

	if (0 != memcmp(&g_mw_af, &af_map, sizeof(mw_af_param))) {
		if (mw_set_af_param(&af_map) < 0) {
			APP_ERROR("mw_set_af_param error\n");
			return -1;
		} else {
			g_mw_af = af_map;
		}
	}

	return 0;
}

static int get_image_stat(char * section_name, u32 info)
{
	APP_INFO("Section [%s] setting:\n", section_name);
	if (mw_get_image_statistics(&image_stat_map) < 0) {
		APP_ERROR("mw_get_image_statistics");
		return -1;
	}
	return 0;
}

static int start_server(void)
{
	int flag = 1;
	struct sockaddr_in server_addr;

	APP_ASSERT(sockfd <= 0);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		APP_ERROR("socket failed %d !\n", sockfd);
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
		APP_ERROR("setsockopt\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(IMAGE_SERVER_PORT);
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		APP_ERROR("bind\n");
		return -1;
	}
	if (listen(sockfd, 10) < 0) {
		APP_ERROR("listen\n");
		return -1;
	}
	return 0;
}

#endif

static void Get_image_quality(MESSAGE *msg_in, MESSAGE *msg_out)
{
	APP_INFO("==========RECEIVE IMAGE QUALITY GETTING REQUEST!==========");
	IMG_QUALITY* pimg = (IMG_QUALITY *)(msg_out->data);
	pimg->denoise_filter = g_mw_basic_iq.mctf_strength;
	pimg->exposure_mode = g_mw_basic_iq.local_exposure_mode;
	pimg->shutter_min = g_mw_basic_iq.ae.shutter_time_min;
	pimg->shutter_max = g_mw_basic_iq.ae.shutter_time_max;
	pimg->antiflicker = g_mw_basic_iq.ae.anti_flicker_mode;
	pimg->max_gain = g_mw_basic_iq.ae.sensor_gain_max;
	pimg->exposure_target_factor = g_mw_basic_iq.ae_target_ratio;
	pimg->wbc = g_mw_awb.wb_mode;
	pimg->saturation = g_mw_image.saturation;
	pimg->brightness = g_mw_image.brightness;
	pimg->contrast = g_mw_image.contrast;
	pimg->shapenness = g_mw_image.sharpness;
	pimg->dc_iris_mode = g_mw_basic_iq.dc_iris_enable;
	pimg->backlight_comp = g_mw_basic_iq.back_light_comp_enable;
	pimg->dn_mode = g_mw_basic_iq.day_night_mode;

	msg_out->cmd_id = GET_IQ;
	msg_out->status = STATUS_SUCCESS;
}


static void Set_image_quality(MESSAGE *msg_in, MESSAGE *msg_out)
{
	APP_INFO("==========RECEIVE IMAGE QUALITY GETTING REQUEST!==========");
	IMG_QUALITY* pimg = (IMG_QUALITY *)(msg_in->data);
	iq_map.mctf_strength = pimg->denoise_filter;
	iq_map.local_exposure_mode = pimg->exposure_mode;
	iq_map.ae.shutter_time_min = DIV_ROUND(512000000, pimg->shutter_min);
	/******29.97 correction*******/
	if(iq_map.ae.shutter_time_min == 17655172) {
		iq_map.ae.shutter_time_min = 17083750;
	}
	iq_map.ae.shutter_time_max = DIV_ROUND(512000000, pimg->shutter_max);
	/******29.97 correction*******/
	if(iq_map.ae.shutter_time_max == 17655172) {
		iq_map.ae.shutter_time_max = 17083750;
	}
	iq_map.ae.anti_flicker_mode = pimg->antiflicker;
	iq_map.ae.sensor_gain_max = pimg->max_gain;
	iq_map.ae_target_ratio = pimg->exposure_target_factor;
	awb_map.wb_mode = pimg->wbc;
	image_map.saturation = pimg->saturation;
	image_map.brightness = pimg->brightness;
	image_map.contrast = pimg->contrast;
	image_map.sharpness = pimg->shapenness;
	iq_map.dc_iris_enable = pimg->dc_iris_mode;
	iq_map.back_light_comp_enable = pimg->backlight_comp;
	iq_map.day_night_mode = pimg->dn_mode;

      if(set_image_param("IMAGE") < 0) {
		msg_out->cmd_id = SET_IQ;
		msg_out->status = STATUS_FAILURE;
	} else {
		msg_out->cmd_id = SET_IQ;
		msg_out->status = STATUS_SUCCESS;
	}


}

static void process_cmd(int cmd_id) {
	void (*processlist[CMD_COUNT]) (MESSAGE*, MESSAGE*);
	processlist[GET_IQ] = Get_image_quality;
	processlist[SET_IQ] = Set_image_quality;
	(*processlist[cmd_id])((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
}

static void  receive_cb_image(union sigval sv)
{
	while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0){
		//INFO("RECEIVE MESSAGE:%s", receive_buffer);
		int i = 0;
		while(i < CMD_COUNT) {
			if(i == receive_buffer->cmd_id) {
				process_cmd(i);
				break;
			}
			i++;
		}
		if(i == CMD_COUNT) {
			send_buffer->cmd_id = UNSUPPORTED_ID;
			sprintf(send_buffer->data, "UNSUPPORTED_ID!");
			APP_ERROR("NOT SUPPORTED COMMAND!");
		}

		while(1){
			if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
				APP_ERROR("mq_send failed!");
				continue;
			}
			break;
		}

		memset(send_buffer, 0, MAX_MESSAGES_SIZE);
		memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
	}
	NotifySetup((mqd_t *)sv.sival_ptr);
}

int create_message_queue( struct mq_attr* attr) {
	mq_unlink(IMG_MQ_SEND);
	mq_unlink(IMG_MQ_RECEIVE);
	msg_queue_send = mq_open(IMG_MQ_SEND, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG, attr);
	if (msg_queue_send < 0)  {
		perror("Error Opening MQ: ");
		return -1;
	}
	msg_queue_receive = mq_open(IMG_MQ_RECEIVE, O_RDONLY | O_CREAT |O_NONBLOCK, S_IRWXU | S_IRWXG, attr);
	if (msg_queue_receive == -1)  {
		perror("Error Opening MQ: ");
		return -1;
	}

	//create buffers
	send_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
	if (send_buffer == NULL){
		APP_ERROR("malloc fail!");
		return -1;
	}
	receive_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
	if (receive_buffer == NULL){
		APP_ERROR("malloc fail!");
		return -1;
	}

	while (mq_receive(msg_queue_receive, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) > 0){
		APP_INFO("POLL QUEUE TO CLEAN!");
	}

	memset(send_buffer, 0, MAX_MESSAGES_SIZE);
	memset(receive_buffer, 0, MAX_MESSAGES_SIZE);

	//bind Receive callback
	NotifySetup(&msg_queue_receive);
	return 0;
}

static int start_mq(void)
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_MESSAGES;
	attr.mq_msgsize = MAX_MESSAGES_SIZE;
	attr.mq_curmsgs = 0;
	if (create_message_queue( &attr) < 0) {
		APP_ERROR("Failed to create mq.");
		return -1;
	}
	return 0;
}

#if 0
static int connect_server(void)
{
	struct sockaddr_in client_addr;

	APP_ASSERT(sockfd2 <= 0);
	socklen_t length = sizeof(client_addr);

	APP_INFO("Listen to %d\n", IMAGE_SERVER_PORT);
	if ((sockfd2 = accept(sockfd, (struct sockaddr *)&client_addr, &length)) < 0) {
		APP_PRINTF("accept failed %d\n", sockfd2);
		return -1;
	}
	APP_INFO("Port [%d] connected!\n", IMAGE_SERVER_PORT);
	return 0;
}

static int disconnect_server(void)
{
	APP_ASSERT(sockfd2 > 0);
	close(sockfd2);
	sockfd2 = -1;
	return 0;
}

int send_text(u8 *pBuffer, u32 size)
{
	int retv = send(sockfd2, pBuffer, size, MSG_NOSIGNAL);
	if ((u32)retv != size) {
		APP_ERROR("send() returns %d\n", retv);
		return -1;
	}
	return 0;
}

int receive_text(u8 *pBuffer, u32 size)
{
	int retv = recv(sockfd2, pBuffer, size, MSG_WAITALL);
	if (retv <= 0) {
		if (retv == 0) {
			APP_INFO("Port [%d] closed\n\n", IMAGE_SERVER_PORT);
			return -2;
		}
		APP_ERROR("recv() returns %d\n", retv);
		return -1;
	}
	return retv;
}

static Section * find_section(char * name)
{
	int i = 0;
	while (NULL != Params[i].name) {
		if (strcmp(name, Params[i].name) == 0)
			break;
		++i;
	}
	if (NULL == Params[i].name)
		return NULL;
	else
		return &Params[i];
}

static int do_get_param(Request *req)
{
	u8 *name = g_buffer;
	char *content = (char *)g_buffer;
	int retv = 0;
	Section * section;
	Ack ack;

	// Get section name
	retv = receive_text(name, req->dataSize);
	name[req->dataSize] = 0;
	APP_INFO("Section Name is : %s\n", (char *)name);

	section = find_section((char *)name);
	if (NULL == section) {
		ack.result = -1;
		ack.info = -1;
		APP_INFO("Section [%s] is not found!\n", (char *)name);
		return send_text((u8 *)&ack, sizeof(ack));
	} else {
		retv = (*section->get)(section->name, req->info);
		retv = mw_output_params(section->map, &content);
		APP_INFO("\n%s\n", content);
		ack.result = retv;
		ack.info = strlen(content);
		send_text((u8 *)&ack, sizeof(ack));
		return send_text((u8 *)content, ack.info);
	}
}

static int do_set_param(Request *req)
{
	u8 *name = g_buffer;
	char *content = (char *)g_buffer;
	int retv = 0;
	Section * section;
	Ack ack;

	retv = receive_text(name, req->dataSize);
	name[req->dataSize] = 0;
	APP_INFO("Section Name is : %s\n", (char *)name);
	section = find_section((char *)name);
	ack.info = ack.result = (NULL == section) ? -1 : 0;
	retv = send_text((u8 *)&ack, sizeof(ack));
	if (ack.result == -1) {
		APP_INFO("Section [%s] is not found!\n", (char *)name);
		return -1;
	}

	// Receive parameter settings
	retv = receive_text((u8 *)content, req->info);
	content[req->info] = 0;
	if (0) {
		APP_INFO("===== Parameter Setting is : \n%s", content);
	} else {
		APP_INFO("===== Parameter Setting is : \n%s", content);
		retv = mw_config_params(section->map, content);
		retv = (*section->set)(section->name);
		ack.result = ack.info = retv;
	}
	retv = send_text((u8 *)&ack, sizeof(ack));
	return retv;
}

static int do_aaa_control(Request *req)
{
	int retv = 0;
	Ack ack;

	APP_INFO("Process AAA control id [%d].\n", (int)req->info);
	switch (req->info) {
	case AAA_START:
		retv = mw_start_aaa(fd_iav);
		if (retv < 0) {
			APP_ERROR("mw_start_aaa failed!\n");
		}
		APP_INFO("AAA control : Start AAA done !\n");
		break;
	case AAA_STOP:
		retv = mw_stop_aaa();
		APP_INFO("AAA control : Stop AAA done !\n");
		break;
	default:
		APP_PRINTF("Unknown AAA control id [%d]\n", (int)req->info);
		break;
	}
	ack.result = ack.info = retv;
	retv = send_text((u8 *)&ack, sizeof(ack));

	return retv;
}

static int process_request(Request *req)
{
	APP_INFO("Process Request ID [%d]\n", (int)req->id);

	switch (req->id) {
	case REQ_GET_PARAM:
		do_get_param(req);
		break;
	case REQ_SET_PARAM:
		do_set_param(req);
		break;
	case REQ_AAA_CONTROL:
		do_aaa_control(req);
		break;
	default:
		APP_PRINTF("Unknown request id [%d]\n", (int)req->id);
		break;
	}
	return 0;
}

static void main_loop(void)
{
#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
	if (start_server() < 0) {
		APP_ERROR("start_server");
		return;
	}
#endif
	while (1) {
		if (connect_server() < 0)
			break;

		while (1) {
			Request req;
			if (receive_text((u8 *)&req, sizeof(req)) < 0)
				break;

			process_request(&req);
		}

		disconnect_server();
	}
}
#endif

static void main_handle(void)
{
	if (start_mq() < 0) {
		APP_ERROR("start_mq");
		return;
	}
	return;
}

static int get_library_params(void)
{
	// Get AE settings
	if (mw_get_ae_param(&g_mw_basic_iq.ae) < 0) {
		APP_ERROR("mw_get_ae_param error\n");
		return -1;
	}
	memcpy(&iq_map.ae, &g_mw_basic_iq.ae, sizeof(mw_ae_param));
	// Get Image settings
	if (mw_get_image_param(&g_mw_image) < 0) {
		APP_ERROR("mw_get_image_param error\n");
		return -1;
	}
	memcpy(&image_map, &g_mw_image, sizeof(mw_image_param));
	// Get AWB settings
	if (mw_get_awb_param(&g_mw_awb) < 0) {
		APP_ERROR("mw_get_awb_param error\n");
		return -1;
	}
	memcpy(&awb_map, &g_mw_awb, sizeof(mw_awb_param));
	// Get AF settings
	if (mw_get_af_param(&g_mw_af) < 0) {
		APP_ERROR("mw_get_af_param error\n");
		return -1;
	}
	memcpy(&af_map, &g_mw_af, sizeof(mw_af_param));
	// Get other settings
	#if 0
	if (mw_get_iq_preference(&g_mw_basic_iq.ae_preference) < 0) {
		APP_ERROR("mw_get_iq_preference error\n");
		return -1;
	}
	iq_map.ae_preference = g_mw_basic_iq.ae_preference;
	if (mw_get_ae_metering_mode(&g_mw_basic_iq.metering_mode) < 0) {
		APP_ERROR("mw_get_ae_metering_mode error\n");
		return -1;
	}
	iq_map.metering_mode = g_mw_basic_iq.metering_mode;
	if (mw_get_auto_local_exposure_mode(&g_mw_basic_iq.local_exposure_mode) < 0) {
		APP_ERROR("mw_get_auto_local_exposure_mode error\n");
		return -1;
	}
	iq_map.local_exposure_mode = g_mw_basic_iq.local_exposure_mode;
	if (mw_get_mctf_strength(&g_mw_basic_iq.mctf_strength) < 0) {
		APP_ERROR("mw_get_mctf_strength error\n");
		return -1;
	}
	iq_map.mctf_strength = g_mw_basic_iq.mctf_strength;
	#endif
	if(get_lib_image_params() < 0) {
		APP_ERROR("get_lib_image_params error\n");
		return -1;
	}
	return 0;
}

static int load_config_file(void)
{
	char filename[CFG_FILE_LENTH];
	char cmd[128];
	char string[FILE_CONTENT_SIZE];
	int file;

	if (!cfg_file_flag) {
		switch (g_mw_encode_mode) {
		case HIGH_FRAMERATE:
			sprintf(cmd, "cp %s/config/image.cfg %s/%s",
				SERVER_CONFIG_PATH, SERVER_CONFIG_PATH, cfg_file);
			break;
		case LOW_DELAY:
			sprintf(cmd, "cp %s/config/image_lowdelay.cfg %s/%s",
				SERVER_CONFIG_PATH, SERVER_CONFIG_PATH, cfg_file);
			break;
		case HIGH_MP:
			sprintf(cmd, "cp %s/config/image_highmp.cfg %s/%s",
				SERVER_CONFIG_PATH, SERVER_CONFIG_PATH, cfg_file);
			break;
		default :
			APP_ERROR("Invalid encode mode : %d.\n", g_mw_encode_mode);
			return -1;
			break;
		}
		system(cmd);
		snprintf(cfg_filename, CFG_FILE_LENTH , "%s/%s", SERVER_CONFIG_PATH, cfg_file);
		cfg_filename[CFG_FILE_LENTH - 1] = '\0';
	}
	strncpy(filename, cfg_filename, CFG_FILE_LENTH);
	filename[CFG_FILE_LENTH - 1] = '\0';

	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		APP_PRINTF("Open user settings file [%s] failed! Use default config file [%s].\n",
			cfg_filename, cfg_file);
		snprintf(filename, CFG_FILE_LENTH,"%s/%s", SERVER_CONFIG_PATH, cfg_filename);
		filename[CFG_FILE_LENTH - 1] = '\0';
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			APP_PRINTF("Open default config file [%s] failed.\n", filename);
			return 0;
		}
	}
	read(file, string, FILE_CONTENT_SIZE);
	close(file);

	mw_config_params(ImageMap, (char *)string);

	return 0;
}

static int config_imager(void)
{
	load_config_file();

	#if 0
	if (mw_set_iq_preference(iq_map.ae_preference) < 0) {
		APP_PRINTF("mw_set_iq_preference error\n");
	}
	#endif
	if (mw_set_ae_param(&iq_map.ae) < 0) {
		APP_ERROR("mw_set_ae_param error\n");
		return -1;
	}
	if (mw_set_image_param(&image_map) < 0) {
		APP_ERROR("mw_set_image_param error\n");
		return -1;
	}
	if (mw_set_awb_param(&awb_map) < 0) {
		APP_ERROR("mw_set_awb_param error\n");
		return -1;
	}
	if (set_basic_image_params(&iq_map) < 0) {
		APP_ERROR("set_basic_image_params error\n");
		return -1;
	}
	g_mw_basic_iq = iq_map;
	g_mw_image = image_map;
	g_mw_awb = awb_map;

	return 0;
}

static int init_server(void)
{
	if (mw_start_aaa(fd_iav) < 0) {
		APP_ERROR("mw_start_aaa");

#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
		nl_image_config.nl_msg.pid = getpid();
		nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
		nl_image_config.nl_msg.msg.image_status = IMAGE_STATUS_START_AAA_FAIL;
		send_image_msg_to_kernel(nl_image_config.nl_msg);
#endif
		return -1;
	}
#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
	nl_image_config.nl_msg.pid = getpid();
	nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
	nl_image_config.nl_msg.msg.image_status = IMAGE_STATUS_START_AAA_SUCCESS;
	send_image_msg_to_kernel(nl_image_config.nl_msg);
#endif
	if (mw_set_log_level(G_log_level) < 0) {
		APP_ERROR("mw_set_log_level");
		return -1;
	}
	if (get_library_params() < 0) {
		APP_ERROR("get_library_params");
		return -1;
	}
	if (config_imager() < 0) {
		APP_ERROR("config_imager");
		return -1;
	}
	APP_INFO("[Done] init image_server\n");
	return 0;
}


static int create_server(void)
{
	if (create_pid_file(IMAGE_SERVER_PROC) < 0) {
		APP_ERROR("create_pid_file");
		return -1;
	}
	if (init_server() < 0) {
		APP_ERROR("init_server");
		return -1;
	}

#if 0
#ifndef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
	if (start_server() < 0) {
		APP_ERROR("start_server");
		return -1;
	}
#endif
#endif
	APP_INFO("[Done] create image_server\n");
	return 0;
}

#ifndef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
static void sigstop(int signo)
{
	mw_stop_aaa();
	delete_pid_file(IMAGE_SERVER_PROC);
	APP_PRINTF("Exit image_server!\n");
	exit(1);
}
#endif

void usage(void)
{
	int i;
	printf("image_server usage:\n");
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

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
		case 'm':
			g_mw_encode_mode = atoi(optarg);
			if (g_mw_encode_mode >= TOTAL_ENCODE_MODE) {
				printf("Invalid encode mode : %d.\n", g_mw_encode_mode);
				return -1;
			}
			break;
		case 'f':
			strncpy(cfg_filename, optarg, CFG_FILE_LENTH);
			cfg_file_flag = 1;
			cfg_filename[CFG_FILE_LENTH - 1] = '\0';
			if(strlen(optarg) >= CFG_FILE_LENTH){
				printf("Warning:arg too long,exiting...\n");
				return -1;
			}
			break;
		case 'l':
			G_log_level = atoi(optarg);
			break;
		default:
			printf("Unknown option found : %c\n", ch);
			return -1;
		}
	}
	return 0;
}

int parse_default_configs(void)
{
	char string[FILE_CONTENT_SIZE], *content;

	content = string;
	mw_config_params(ImageMap, NULL);
	mw_output_params(ImageMap, &content);
//	printf("\n%s\n", content);

	mw_config_params(AFMap, NULL);
	mw_output_params(AFMap, &content);
//	printf("\n%s\n", content);

	return 0;
}

#ifndef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	parse_default_configs();

	if (create_server() < 0) {
		APP_ERROR("create_server");
		return -1;
	}
	main_handle();

	#if 0
	while (1) {
		main_loop();
	}
	#endif
	while (1) {
		sleep(1);
	}

	return 0;
}
#else
static int init_image_config(int argc, char *argv[])
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	parse_default_configs();

	/*if (create_server() < 0) {
		APP_ERROR("create_server");
		return -1;
	}*/

	return 0;
}

static int send_image_msg_to_kernel(nl_image_msg_t nl_msg)
{
	struct sockaddr_nl daddr;
	struct msghdr msg;
	struct nlmsghdr *nlhdr = NULL;
	struct iovec iov;

	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;
	daddr.nl_pad = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_send_buf;
	nlhdr->nlmsg_pid = getpid();
	nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(nl_msg));
	nlhdr->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlhdr), &nl_msg, sizeof(nl_msg));

	memset(&msg, 0, sizeof(struct msghdr));
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_name = (void *)&daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(nl_image_config.fd_nl, &msg, 0);

	return 0;
}

static int recv_image_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (nl_image_config.fd_nl > 0) {
		ret = recvmsg(nl_image_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened for receive message!\n");
		ret = -1;
	}

	return ret;
}

static int check_recv_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	if (nlhdr->nlmsg_len <  sizeof(struct nlmsghdr)) {
		printf("Corruptted kernel message!\n");
		return -1;
	}
	msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
	if (msg_len < sizeof(nl_image_msg_t)) {
		printf("Unknown kernel message!!\n");
		return -1;
	}

	return 0;
}

static int process_image_cmd(int image_cmd)
{
	int ret = 0;

	if (image_cmd == IMAGE_CMD_START_AAA) {
		if (nl_image_config.image_init == 0) {
			if (create_server() < 0) {
				APP_ERROR("create_server");
				return -1;
			} else {
				nl_image_config.image_init = 1;
			}
		} else {
			ret = mw_start_aaa(fd_iav);
			nl_image_config.nl_msg.pid = getpid();
			nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
			if (ret < 0) {
				printf("Start AAA failed!\n");
				nl_image_config.nl_msg.msg.image_status =
					IMAGE_STATUS_START_AAA_FAIL;
				send_image_msg_to_kernel(nl_image_config.nl_msg);
			} else {
				nl_image_config.nl_msg.msg.image_status =
					IMAGE_STATUS_START_AAA_SUCCESS;
				send_image_msg_to_kernel(nl_image_config.nl_msg);
			}
		}
	} else if (image_cmd == IMAGE_CMD_STOP_AAA) {
		ret = mw_stop_aaa();
		nl_image_config.nl_msg.pid = getpid();
		nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
		if (ret < 0) {
			printf("Stop AAA failed!\n");
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_STOP_AAA_FAIL;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		} else {
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_STOP_AAA_SUCCESS;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		}
	} else {
		printf("Unrecognized kernel message!\n");
		ret = -1;
	}

	return ret;
}

static int process_image_session_status(int session_status)
{
	int ret = 0;

	switch (session_status) {
	case IMAGE_SESSION_STATUS_CONNECT_SUCCESS:
		nl_image_config.nl_connected = 1;
		printf("Connection established with kernel.\n");
		break;
	case IMAGE_SESSION_STATUS_CONNECT_FAIL:
		nl_image_config.nl_connected = 0;
		printf("Failed to establish connection with kernel!\n");
		break;
	case IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS:
		nl_image_config.nl_connected = 0;
		printf("Connection removed with kernel.\n");
		break;
	case IMAGE_SESSION_STATUS_DISCONNECT_FAIL:
		nl_image_config.nl_connected = 0;
		printf("Failed to remove connection with kernel!\n");
		break;
	default:
		printf("Unrecognized session status from kernel!\n");
		ret = -1;
		break;
	}

	return ret;
}


static int process_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	nl_image_msg_t *kernel_msg;
	int ret = 0;

	if (check_recv_image_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	kernel_msg = (nl_image_msg_t *)NLMSG_DATA(nlhdr);

	if(kernel_msg->index == IMAGE_MSG_INDEX_AAA_CMD) {
		if (process_image_cmd(kernel_msg->msg.image_cmd) < 0) {
			ret = -1;
		}
	} else if (kernel_msg->index == IMAGE_MSG_INDEX_SESSION_STATUS) {
		if (process_image_session_status(kernel_msg->msg.session_status) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int init_netlink()
{
	u32 pid;
	struct sockaddr_nl saddr;

	nl_image_config.fd_nl = socket(AF_NETLINK,
		SOCK_RAW, NETLINK_IMAGE_PROTOCAL);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(nl_image_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	nl_image_config.nl_connected = 0;
	nl_image_config.image_init = 0;

	return 0;
}

static int nl_send_image_session_cmd(int cmd)
{
	int ret = 0;

	if (cmd == IMAGE_SESSION_CMD_DISCONNECT) {
		mw_stop_aaa();
	}

	nl_image_config.nl_msg.pid = getpid();
	nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_SESSION_CMD;
	nl_image_config.nl_msg.msg.session_cmd = cmd;
	send_image_msg_to_kernel(nl_image_config.nl_msg);

	if (cmd == IMAGE_SESSION_CMD_CONNECT) {
		ret = recv_image_msg_from_kernel();

		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process session status!\n");
			}
		} else {
			printf("Error for getting session status!\n");
		}
	}

	return ret;
}

static int start_aaa_for_non_idle(void)
{
	int ret = 0;
	int state;

	mw_get_iav_state(fd_iav, &state);

	if ((state == MW_IAV_STATE_ENCODING) || (state == MW_IAV_STATE_PREVIEW)) {
		if (nl_image_config.image_init == 0) {
			if (create_server() < 0) {
				APP_ERROR("create_server");
			} else {
				nl_image_config.image_init = 1;
			}
		} else {
			ret = mw_start_aaa(fd_iav);
		}
	}

	return ret;
}

static void * netlink_loop(void * data)
{
	int ret;

	if (nl_send_image_session_cmd(IMAGE_SESSION_CMD_CONNECT) < 0) {
		printf("Failed to establish connection with kernel!\n");
	}

	if (!nl_image_config.nl_connected) {
		return NULL;
	}

	// start 3A for preview/encode state
	start_aaa_for_non_idle();

	while (nl_image_config.nl_connected) {
		ret = recv_image_msg_from_kernel();
		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process the msg from kernel!\n");
			}
		}
		else {
			printf("Error for getting msg from kernel!\n");
		}
	}

	return NULL;
}

#if 0
static void * socket_loop(void * data)
{
	while (1) {
		main_loop();
	}

	return NULL;
}
#endif

static void * mq_loop(void * data)
{
	main_handle();
	while (1) {
		sleep(1);
	}


	return NULL;
}

static const char * get_pid_file_proc()
{
	return image_pid_file;
}

int main(int argc, char *argv[])
{
	pid_t pid;
	/* Reset signal handlers */
	if (daemon_reset_sigs(-1) < 0) {
		daemon_log(LOG_ERR, "Failed to reset all signal handlers: %s",
			strerror(errno));
		return 1;
	}
	/* Unblock signals */
	if (daemon_unblock_sigs(-1) < 0) {
		daemon_log(LOG_ERR, "Failed to unblock all signals: %s",
			strerror(errno));
	}
	/* Set indetification string for the daemon for both syslog and PID file */
	daemon_log_ident = daemon_ident_from_argv0(argv[0]);
	daemon_pid_file_ident = daemon_log_ident;
	//(LOG_ERR, "The log ident is %s\n",daemon_log_ident);
	daemon_pid_file_proc = get_pid_file_proc;
	sprintf (image_pid_file, "/var/run/%s.pid", daemon_pid_file_ident);
	/* Check if we are called with -k parameter */
	if (argc >= 2 && !strcmp(argv[1], "-k")) {
		int ret;
		/* Kill daemon with SIGTERM */
		/* Check if the new function daemon_pid_file_kill_wait() is available,
			if it is, use it. */
		if ((ret = daemon_pid_file_kill_wait(SIGTERM, 5)) < 0)
			daemon_log(LOG_WARNING, "Failed to kill daemon: %s",
				strerror(errno));
		return ret < 0 ? 1 : 0;
	}
	/* Check that the daemon is not rung twice a the same time */
	if ((pid = daemon_pid_file_is_running()) >= 0) {
		daemon_log(LOG_ERR, "Daemon already running on PID file %u", pid);
		return 1;
	}
	/* Prepare for return value passing from the initialization procedure
		of the daemon process */
	if (daemon_retval_init() < 0) {
		daemon_log(LOG_ERR, "Failed to create pipe.");
		return 1;
	}
	/* Do the fork */
	if ((pid = daemon_fork()) < 0) {
		/* Exit on error */
		daemon_retval_done();
		return 1;
	} else if (pid) {  /* The parent */
		int ret;
		/* Wait for 20 seconds for the return value passed from the daemon
			process */
		if ((ret = daemon_retval_wait(20)) < 0) {
			daemon_log(LOG_ERR, "Count not receive return value from daemon"
				"process: %s", strerror(errno));
		}
		//daemon_log(ret != 0 ? LOG_ERR : LOG_INFO,
		//	"Daemon returned %i as return value.", ret);
		return ret;
	} else {  /* The daemon */
		int fd, quit = 0;
		fd_set fds;
		pthread_t sock_thread;
		pthread_t nl_thread;
		/* Close FDs */
		if (daemon_close_all(-1) < 0) {
			daemon_log(LOG_ERR, "Failed to close all file descriptors: %s",
				strerror(errno));
			/* Send the error condition to the parent process */
			daemon_retval_send(1);
			goto finish;
		}
		/* Create the PID file */
		if (daemon_pid_file_create() < 0) {
			daemon_log(LOG_ERR, "Could not create PID file (%s).",
				strerror(errno));
			daemon_retval_send(2);
			goto finish;
		}
		/* Initialize signal handling */
		if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0) < 0) {
			daemon_log(LOG_ERR, "Could not register signal handlers (%s).",
				strerror(errno));
			daemon_retval_send(3);
			goto finish;
		}
		/* Init task */
		init_image_config(argc, argv);
		init_netlink();
		/* Send OK to parent process */
		daemon_retval_send(0);
		daemon_log(LOG_INFO, "Sucessfully started");
		/* Main task*/
		pthread_create(&nl_thread, NULL, (void *)netlink_loop, (void *)NULL);
		#if 0
		pthread_create(&sock_thread, NULL, (void *)socket_loop, (void *)NULL);
		#endif
		pthread_create(&sock_thread, NULL, (void *)mq_loop, (void *)NULL);
		/* Prepare for select() on the signal fd */
		FD_ZERO(&fds);
		fd = daemon_signal_fd();
		FD_SET(fd, &fds);
		while (!quit) {
			fd_set fds2 = fds;
			/* Wait for an incoming signal */
			if (select(FD_SETSIZE, &fds2, 0, 0, 0) < 0) {
				/* If we've been interrupted by an incoming signal, continue */
				if (errno == EINTR)
					continue;
				daemon_log(LOG_ERR, "select(): %s", strerror(errno));
				break;
			}
			/* Check if a signal has been recieved */
			if (FD_ISSET(fd, &fds2)) {
				int sig;
				/* Get signal */
				if ((sig = daemon_signal_next()) <= 0) {
					daemon_log(LOG_ERR, "daemon_signal_next() failed: %s",
						strerror(errno));
					break;
				}
				/* Dispatch signal */
				switch (sig) {
					case SIGINT:
					case SIGQUIT:
					case SIGTERM:
						daemon_log(LOG_WARNING, "Got SIGINT, SIGQUIT or"
							"SIGTERM.");
						if (nl_image_config.nl_connected == 1) {
							nl_image_config.nl_connected = 0;
							if (nl_send_image_session_cmd(
								IMAGE_SESSION_CMD_DISCONNECT) < 0) {
								printf("Failed to remove connection with"
									"kernel!\n");
							}
						}
						quit = 1;
						break;
					case SIGHUP:
						daemon_log(LOG_INFO, "Got a HUP");
						daemon_exec("/", NULL, "/bin/ls", "ls", (char*) NULL);
						break;
				}
			}
		}
		/* Do a cleanup */
finish:
		delete_pid_file(IMAGE_SERVER_PROC);
		daemon_log(LOG_INFO, "Exiting...");
		daemon_retval_send(255);
		daemon_signal_done();
		daemon_pid_file_remove();
		return 0;
	}
}
#endif

#define __END_OF_FILE__

