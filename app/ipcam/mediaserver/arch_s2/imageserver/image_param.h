/***********************************************************
 * image_param.h
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

#ifndef _IMAGE_PARAM_H_
#define _IMAGE_PARAM_H_
#define CFG_FILE_LENTH (256)

#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
#define NETLINK_IMAGE_PROTOCAL	20
#define MAX_NL_MSG_LEN 1024
#endif

#define IMG_MQ_SEND "/MQ_IMAGE_SEND"
#define IMG_MQ_RECEIVE "/MQ_IMAGE_RECEIVE"
#define MAX_MESSAGES (64)
#define MAX_MESSAGES_SIZE (1024)

typedef enum {
  GET_IQ = 0,
  SET_IQ,
  UNSUPPORTED_ID,
  CMD_COUNT,
} IMG_COMMAND_ID;

typedef struct {
  uint32_t cmd_id;
  uint32_t status; //0--success ;1--failure
  char data[0];
} MESSAGE;

typedef enum {
  STATUS_SUCCESS = 0,
  STATUS_FAILURE,
}REQUEST_STATUS;

typedef enum {
	LE_STOP = 0,
	LE_AUTO,
	LE_1X = 0x40,
	LE_2X = 0x80,
	LE_3X = 0xC0,
	LE_4X = 0x100,
	LE_CUSTOMER = 0x200,
} LOCAL_EXPOSURE_MODE;


typedef struct {
	mw_ae_param		ae;
	//int					ae_preference;
	u32					day_night_mode;
	//u32					metering_mode;
	u32					back_light_comp_enable;
	u32					local_exposure_mode;
	u32					mctf_strength;
	u32					dc_iris_enable;
	//u32					dc_iris_duty_balance;
	u32					ae_target_ratio;
} basic_iq_params;

#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
typedef enum {
	IMAGE_CMD_START_AAA = 0,
	IMAGE_CMD_STOP_AAA,
} NL_IMAGE_AAA_CMD;
typedef enum {
	IMAGE_STATUS_START_AAA_SUCCESS = 0,
	IMAGE_STATUS_START_AAA_FAIL,
	IMAGE_STATUS_STOP_AAA_SUCCESS,
	IMAGE_STATUS_STOP_AAA_FAIL,
} NL_IMAGE_AAA_STATUS;
typedef enum {
	IMAGE_SESSION_CMD_CONNECT = 0,
	IMAGE_SESSION_CMD_DISCONNECT,
}NL_IMAGE_SESSION_CMD;
typedef enum {
	IMAGE_SESSION_STATUS_CONNECT_SUCCESS = 0,
	IMAGE_SESSION_STATUS_CONNECT_FAIL,
	IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS,
	IMAGE_SESSION_STATUS_DISCONNECT_FAIL,
}NL_IMAGE_SESSION_STATUS;
typedef enum {
	IMAGE_MSG_INDEX_SESSION_CMD = 0,
	IMAGE_MSG_INDEX_SESSION_STATUS,
	IMAGE_MSG_INDEX_AAA_CMD,
	IMAGE_MSG_INDEX_AAA_STATUS,
}NL_IMAGE_MSG_INDEX;
typedef struct nl_image_msg_s {
	u32 pid;
	s32 index;
	union {
		s32 session_cmd;
		s32 session_status;
		s32 image_cmd;
		s32 image_status;
	} msg;
} nl_image_msg_t;
typedef struct nl_image_config_s {
	s32 fd_nl;
	s32 image_init;
	s32 nl_connected;
	nl_image_msg_t nl_msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
} nl_image_config_t;
#endif

typedef struct {
    uint32_t dn_mode;
    uint32_t denoise_filter;
    uint32_t exposure_mode;
    uint32_t backlight_comp;
    uint32_t exposure_target_factor;
    uint32_t dc_iris_mode;
    uint32_t antiflicker;
    uint32_t shutter_min;
    uint32_t shutter_max;
    uint32_t max_gain;
    uint32_t saturation;
    uint32_t brightness;
    uint32_t hue;
    uint32_t contrast;
    uint32_t shapenness;
    uint32_t wbc;
}IMG_QUALITY;

static basic_iq_params		iq_map;
static mw_image_param	image_map;
static mw_awb_param		awb_map;
static mw_af_param		af_map;
#if 0
static mw_image_stat_info	image_stat_map;
#endif

static Mapping ImageMap[] = {
	{"anti_flicker",	&iq_map.ae.anti_flicker_mode,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"shutter_min",		&iq_map.ae.shutter_time_min,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"shutter_max",		&iq_map.ae.shutter_time_max,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"max_gain",			&iq_map.ae.sensor_gain_max,		MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	//{"slow_shutter",		&iq_map.ae.slow_shutter_enable,	MAP_TO_U32,	1.0,		NO_LIMIT,		0.0,		0.0,	},
	//{"vin_fps",			&iq_map.ae.current_vin_fps,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	//{"ae_preference",		&iq_map.ae_preference,			MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"dn_mode",			&iq_map.day_night_mode,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	//{"metering_mode",		&iq_map.metering_mode,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"backlight_comp",		&iq_map.back_light_comp_enable,	MAP_TO_U32,	0.0,		MIN_MAX_LIMIT,	0.0,		2.0,	},
	{"local_exposure",		&iq_map.local_exposure_mode,		MAP_TO_U32,	0.0,		MIN_MAX_LIMIT,	0.0,		5.0,	},
	{"mctf_strength",		&iq_map.mctf_strength,			MAP_TO_U32,	1.0,		NO_LIMIT,		0.0,		0.0,	},
	{"dc_iris",			&iq_map.dc_iris_enable,			MAP_TO_U32,	0.0,		MIN_MAX_LIMIT,	0.0,		2.0,	},
	//{"dc_iris_duty",		&iq_map.dc_iris_duty_balance,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"ae_target_ratio",		&iq_map.ae_target_ratio,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{"saturation",			&image_map.saturation,			MAP_TO_S32,	64.0,		NO_LIMIT,		0.0,		0.0,	},
	{"brightness",			&image_map.brightness,			MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"hue",				&image_map.hue,				MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"contrast",			&image_map.contrast,				MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"sharpness",			&image_map.sharpness,			MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{"wbc",				&awb_map.wb_mode,				MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{NULL,			NULL,						-1,	0.0,					0,	0.0,	0.0,		},
};

static Mapping AFMap[] = {
	{"lens_type",			&af_map.lens_type,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"af_mode",			&af_map.af_mode,			MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"af_tile_mode",		&af_map.af_tile_mode,			MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"zm_dist",			&af_map.zm_dist,				MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"fs_dist",			&af_map.fs_dist,				MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"fs_near",			&af_map.fs_near,				MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"fs_far",				&af_map.fs_far,				MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{NULL,			NULL,						-1,	0.0,					0,	0.0,	0.0,		},
};

#if 0
static Mapping ImageStatMap[] = {
	{"agc",				&image_stat_map.agc,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"shutter",			&image_stat_map.shutter,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{NULL,			NULL,						-1,	0.0,					0,	0.0,	0.0,		},
};

static int get_image_stat(char * section_name, u32 info);
static int set_af_param(char *section_name);
#endif

static int set_image_param(char * section_name);

typedef struct {
	char *		name;
	Mapping *	map;
	get_func		get;
	set_func		set;
} Section;

#if 0
static Section Params[] = {
	{"IMAGE",		ImageMap,		get_func_null,		set_image_param},
	{"IQAF",		AFMap,			get_func_null,		set_af_param},
	{"STAT",		ImageStatMap,	get_image_stat,		set_func_null},
	{NULL, NULL, NULL}
};
#endif

static int		fd_iav;

#if 0
static int		sockfd = -1;
static int		sockfd2 = -1;
#endif

static const char *cfg_file = "image.cfg";

static int		cfg_file_flag = 0;
static char	cfg_filename[CFG_FILE_LENTH];

#ifdef CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON
static nl_image_config_t nl_image_config;
static char image_pid_file[CFG_FILE_LENTH];
#endif

int		G_log_level = 0;

static mw_local_exposure_curve G_local_exposure[] = {
	{	// Max LE 2048
		{2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
		2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
		2047, 2043, 2037, 2029, 2020, 2009, 1997, 1985, 1972, 1958, 1943, 1928, 1913, 1898, 1883, 1867,
		1851, 1836, 1820, 1804, 1788, 1773, 1757, 1742, 1727, 1712, 1697, 1682, 1667, 1653, 1638, 1624,
		1610, 1597, 1583, 1570, 1556, 1543, 1530, 1518, 1505, 1493, 1480, 1468, 1456, 1445, 1433, 1422,
		1411, 1400, 1389, 1378, 1367, 1357, 1347, 1337, 1326, 1317, 1307, 1297, 1288, 1279, 1269, 1260,
		1251, 1242, 1234, 1225, 1217, 1208, 1200, 1192, 1184, 1176, 1168, 1160, 1153, 1145, 1138, 1130,
		1123, 1116, 1109, 1102, 1095, 1088, 1081, 1075, 1068, 1062, 1055, 1049, 1043, 1037, 1031, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,}
	},
	{	// Max LE 3072
		{3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072,
		3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072, 3072,
		3070, 3063, 3053, 3039, 3023, 3005, 2985, 2964, 2942, 2918, 2894, 2870, 2844, 2819, 2793, 2767,
		2741, 2716, 2690, 2664, 2638, 2613, 2588, 2563, 2538, 2513, 2489, 2465, 2441, 2418, 2395, 2372,
		2350, 2328, 2306, 2285, 2263, 2243, 2222, 2202, 2182, 2162, 2143, 2124, 2105, 2087, 2069, 2051,
		2033, 2016, 1999, 1982, 1965, 1949, 1933, 1917, 1901, 1886, 1871, 1856, 1841, 1827, 1813, 1799,
		1785, 1771, 1758, 1744, 1731, 1718, 1706, 1693, 1681, 1669, 1657, 1645, 1633, 1622, 1610, 1599,
		1588, 1577, 1566, 1556, 1545, 1535, 1525, 1515, 1505, 1495, 1485, 1476, 1466, 1457, 1448, 1439,
		1430, 1421, 1412, 1403, 1395, 1386, 1378, 1370, 1361, 1353, 1345, 1337, 1330, 1322, 1314, 1307,
		1299, 1292, 1285, 1278, 1270, 1263, 1256, 1249, 1243, 1236, 1229, 1223, 1216, 1210, 1203, 1197,
		1191, 1185, 1178, 1172, 1166, 1160, 1155, 1149, 1143, 1137, 1132, 1126, 1120, 1115, 1109, 1104,
		1099, 1094, 1088, 1083, 1078, 1073, 1068, 1063, 1058, 1053, 1048, 1043, 1039, 1034, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,}
	},
	{	// Max LE 4095
		{4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
		4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
		4092, 4083, 4068, 4049, 4027, 4002, 3974, 3945, 3914, 3881, 3848, 3814, 3779, 3744, 3709, 3673,
		3637, 3602, 3566, 3531, 3496, 3461, 3426, 3392, 3358, 3324, 3291, 3259, 3226, 3194, 3163, 3132,
		3102, 3071, 3042, 3013, 2984, 2956, 2928, 2900, 2873, 2847, 2820, 2795, 2769, 2744, 2720, 2696,
		2672, 2648, 2625, 2603, 2580, 2558, 2537, 2515, 2494, 2473, 2453, 2433, 2413, 2394, 2375, 2356,
		2337, 2319, 2301, 2283, 2266, 2248, 2231, 2214, 2198, 2182, 2166, 2150, 2134, 2119, 2103, 2088,
		2074, 2059, 2045, 2030, 2016, 2003, 1989, 1976, 1962, 1949, 1936, 1923, 1911, 1898, 1886, 1874,
		1862, 1850, 1838, 1827, 1816, 1804, 1793, 1782, 1771, 1761, 1750, 1739, 1729, 1719, 1709, 1699,
		1689, 1679, 1669, 1660, 1650, 1641, 1632, 1623, 1614, 1605, 1596, 1587, 1578, 1570, 1561, 1553,
		1545, 1537, 1529, 1520, 1513, 1505, 1497, 1489, 1482, 1474, 1467, 1459, 1452, 1445, 1437, 1430,
		1423, 1416, 1409, 1403, 1396, 1389, 1382, 1376, 1369, 1363, 1356, 1350, 1344, 1338, 1331, 1325,
		1319, 1313, 1307, 1301, 1296, 1290, 1284, 1278, 1273, 1267, 1262, 1256, 1251, 1245, 1240, 1235,
		1229, 1224, 1219, 1214, 1209, 1204, 1199, 1194, 1189, 1184, 1179, 1174, 1170, 1165, 1160, 1156,
		1151, 1146, 1142, 1137, 1133, 1128, 1124, 1120, 1115, 1111, 1107, 1103, 1098, 1094, 1090, 1086,
		1082, 1078, 1074, 1070, 1066, 1062, 1058, 1054, 1050, 1047, 1043, 1039, 1035, 1032, 1028, 1024,}
	},
};

#endif //  _IMAGE_PARAM_H_

