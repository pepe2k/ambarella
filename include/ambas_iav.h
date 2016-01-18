#ifndef __AMBAS_IAV_H
#define __AMBAS_IAV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>
#include <semaphore.h>

#include <basetypes.h>
#include <iav_drv.h>

#include <ambas_vin.h>
#include <ambas_vout.h>

#define LOCAL_HOST				"127.0.0.1"
#define IAV_TCP_PORT				5999
#define MAX_CLIENTS				8

#define IAV_SERVER_INFO(format, arg...)		printf("IAV Server Info: <%s %d>: "format , __func__, __LINE__, ##arg)
#define IAV_SERVER_WARNING(format, arg...)	printf("IAV Server Warning: <%s %d>: "format , __func__, __LINE__, ##arg)
#define IAV_SERVER_ERROR(format, arg...)	printf("IAV Server Error: <%s %d>: "format , __func__, __LINE__, ##arg)

#define IAV_CLIENT_INFO(format, arg...)		printf("IAV Client Info: <%s %d>: "format , __func__, __LINE__, ##arg)
#define IAV_CLIENT_WARNING(format, arg...)	printf("IAV Client Warning: <%s %d>: "format , __func__, __LINE__, ##arg)
#define IAV_CLIENT_ERROR(format, arg...)	printf("IAV Client Error: <%s %d>: "format , __func__, __LINE__, ##arg)

/* IAV Client Type */
enum iav_client_type {
	IAV_CLN_DSP,
	IAV_CLN_VIN,
	IAV_CLN_VOUT,
	IAV_CLN_OSD,
	IAV_CLN_CAMERA,
	IAV_CLN_PLAYBACK,
};

/* IAV Service Request */
enum iav_service_cmd_msg {
	/* CLIENT INFO */
	IAV_SRV_INF_CLIENT_NAME			= 0x00000000,
	IAV_SRV_INF_CLIENT_TYPE,

	/* DSP */
	IAV_SRV_REQ_DSP_LOAD_UCODE		= 0x00010000,
	IAV_SRV_REQ_DSP_GO_TO_IDLE,
	IAV_SRV_REQ_DSP_ENTER_PREVIEW,

	/* VIN */
	IAV_SRV_REQ_VIN_START			= 0x00020000,
	IAV_SRV_REQ_VIN_RESTART,

	/* VOUT */
	IAV_SRV_REQ_VOUT_START			= 0x00030000,
	IAV_SRV_REQ_VOUT_STOP,
	IAV_SRV_REQ_VOUT_RESTART,
	IAV_SRV_REQ_VOUT_ENABLE_VIDEO,
	IAV_SRV_REQ_VOUT_CHANGE_VIDEO_SIZE,
	IAV_SRV_REQ_VOUT_CHANGE_VIDEO_OFFSET,
	IAV_SRV_REQ_VOUT_FLIP_VIDEO,
	IAV_SRV_REQ_VOUT_ROTATE_VIDEO,
	IAV_SRV_REQ_VOUT_SELECT_FB,
	IAV_SRV_REQ_VOUT_CHANGE_OSD_SIZE,
	IAV_SRV_REQ_VOUT_CHANGE_OSD_OFFSET,
	IAV_SRV_REQ_VOUT_FLIP_OSD,
	IAV_SRV_REQ_VOUT_ROTATE_OSD,
	IAV_SRV_REQ_VOUT_CHANGE_VIDEO_SIZE_OFFSET,

	/* Notification */
	IAV_SRV_REQ_NOTIFICATION		= 0x000e0000,
	IAV_SRV_ACK_NOTIFICATION,

	/* Server Message */
	IAV_SERVER_RESPONSE			= 0x000f0000,
	IAV_SRV_NOTIFICATION,
};

/* IAV Service Response */
enum iav_service_response {
	/* Service Response */
	IAV_SRV_RES_OK				= 0x00000000,
	IAV_SRV_RES_ERROR,
};

/* IAV Server Notification Type */
enum iav_server_notification {
	IAV_SRV_NOT_HDMI_PLUGIN			= (1 << 0),
	IAV_SRV_NOT_HDMI_REMOVE			= (1 << 1),
	IAV_SRV_NOT_SET_DISPLAY_POSITION = (1 << 2),
};

struct iav_srv_inf_client_name {
	char					name[32];
};

struct iav_srv_inf_client_type {
	enum iav_client_type			type;
};

struct iav_srv_req_dsp_load_ucode {
	char					path[64];
};

struct iav_srv_req_vin_start {
	enum amba_vin_camera_location		camera;
	enum amba_video_mode			vin_mode;
	int					framerate;
	enum amba_vin_src_mirror_pattern	mirror_flip;
	int					anti_flicker;
};

struct iav_srv_req_vin_restart {
	enum amba_vin_camera_location		camera;
	enum amba_video_mode			vin_mode;
	int					framerate;
	enum amba_vin_src_mirror_pattern	mirror_flip;
	int					anti_flicker;
};

struct iav_srv_req_vout_start {
	enum amba_vout_display_device_type	device_type;
	enum amba_video_mode			vout_mode;

	int					video_enable;
	int					video_fullscreen;
	int					video_width;
	int					video_height;
	int					video_middle;
	int					video_offset_x;
	int					video_offset_y;
	enum amba_vout_flip_info		video_flip;
	enum amba_vout_rotate_info		video_rotate;

	int					fb_id;
	int					osd_rescale;
	int					osd_width;
	int					osd_height;
	int					osd_middle;
	int					osd_offset_x;
	int					osd_offset_y;
	enum amba_vout_flip_info		osd_flip;
	enum amba_vout_rotate_info		osd_rotate;

	enum amba_vout_lcd_model		lcd_model;
};

struct iav_srv_req_vout_stop {
	enum amba_vout_display_device_type	device_type;
};

struct iav_srv_req_vout_restart {
	enum amba_vout_display_device_type	device_type;
	enum amba_video_mode			vout_mode;

	int					video_enable;
	int					video_fullscreen;
	int					video_width;
	int					video_height;
	int					video_middle;
	int					video_offset_x;
	int					video_offset_y;
	enum amba_vout_flip_info		video_flip;
	enum amba_vout_rotate_info		video_rotate;

	int					fb_id;
	int					osd_rescale;
	int					osd_width;
	int					osd_height;
	int					osd_middle;
	int					osd_offset_x;
	int					osd_offset_y;
	enum amba_vout_flip_info		osd_flip;
	enum amba_vout_rotate_info		osd_rotate;

	enum amba_vout_lcd_model		lcd_model;
};

struct iav_srv_req_vout_enable_video {
	enum amba_vout_display_device_type	device_type;
	int					enable;
};

struct iav_srv_req_vout_change_video_size {
	enum amba_vout_display_device_type	device_type;
	int					width;
	int					height;
};

struct iav_srv_req_vout_change_video_offset {
	enum amba_vout_display_device_type	device_type;
	int					offset_x;
	int					offset_y;
};

struct iav_srv_req_vout_change_video_size_offset{
	enum amba_vout_display_device_type	device_type;
	int					offset_x;
	int					offset_y;
	int					width;
	int					height;
};

struct iav_srv_req_vout_flip_video {
	enum amba_vout_display_device_type	device_type;
	enum amba_vout_flip_info		flip;
};

struct iav_srv_req_vout_rotate_video {
	enum amba_vout_display_device_type	device_type;
	enum amba_vout_rotate_info		rotate;
};

struct iav_srv_req_vout_select_fb {
	enum amba_vout_display_device_type	device_type;
	int					fb_id;
};

struct iav_srv_req_vout_change_osd_size {
	enum amba_vout_display_device_type	device_type;
	int					width;
	int					height;
};

struct iav_srv_req_vout_change_osd_offset {
	enum amba_vout_display_device_type	device_type;
	int					offset_x;
	int					offset_y;
};

struct iav_srv_req_vout_flip_osd {
	enum amba_vout_display_device_type	device_type;
	enum amba_vout_flip_info		flip;
};

struct iav_srv_req_vout_rotate_osd {
	enum amba_vout_display_device_type	device_type;
	enum amba_vout_rotate_info		rotate;
};

struct iav_srv_req_notification {
	unsigned int				req_notification;
};

struct iav_srv_ack_notification {
	unsigned int				suceed;
};

struct iav_srv_response {
	unsigned int				cmd_id;
	enum iav_service_response		response;
};

struct iav_srv_notification {
	unsigned int				notification;
};

union iav_srv_payload {
	struct iav_srv_inf_client_name			client_name;
	struct iav_srv_inf_client_type			client_type;

	struct iav_srv_req_dsp_load_ucode		load_ucode;

	struct iav_srv_req_vin_start			start_vin;
	struct iav_srv_req_vin_restart			restart_vin;

	struct iav_srv_req_vout_start			start_vout;
	struct iav_srv_req_vout_stop			stop_vout;
	struct iav_srv_req_vout_restart			restart_vout;
	struct iav_srv_req_vout_enable_video		enable_video;
	struct iav_srv_req_vout_change_video_size	video_size;
	struct iav_srv_req_vout_change_video_offset	video_offset;
	struct iav_srv_req_vout_flip_video		video_flip;
	struct iav_srv_req_vout_rotate_video		video_rotate;
	struct iav_srv_req_vout_select_fb		fb_select;
	struct iav_srv_req_vout_change_osd_size		osd_size;
	struct iav_srv_req_vout_change_osd_offset	osd_offset;
	struct iav_srv_req_vout_flip_osd		osd_flip;
	struct iav_srv_req_vout_rotate_osd		osd_rotate;

	struct iav_srv_req_notification			req_notification;
	struct iav_srv_ack_notification			ack_notification;

	struct iav_srv_response				response;
	struct iav_srv_notification			notification;
	struct iav_srv_req_vout_change_video_size_offset video_size_offset;
};

/* IAV Command/Message */
struct iav_command_message {
	enum iav_service_cmd_msg		type;
	unsigned int				cmd_msg_id;
	int					need_response;
	union iav_srv_payload			payload;
};

/* IAV Service Client */
struct iav_srv_client {
	pthread_mutex_t				mtx;
	int					alive;
	unsigned int				msg_id;
	pthread_t				thread_id;
	int					socket;
	char					name[32];
	enum iav_client_type			type;
	unsigned int				req_notification;
	int					wait_ack;
};

#endif
