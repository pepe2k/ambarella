/*
 * Filename : ov5658_reg_tbl.c
 *
 * History:
 *    2013/08/28 - [Johnson Diao] Create
 *
 * Copyright (C) 2013-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

	/*   < rember to update OV5658_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
	/*   < rember to update OV5658_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct ov5658_video_format_reg_table ov5658_video_format_tbl = {
	.reg = {
		0x3708,
		0x3800,
		0x3801,
		0x3802,
		0x3803,
		0x3804,
		0x3805,
		0x3806,
		0x3807,
		0x3808,
		0x3809,
		0x380a,
		0x380b,
		0x380c,
		0x380d,
		0x380e,
		0x380f,
		0x3810,
		0x3811,
		0x3812,
		0x3813,
		0x3814,
		0x3815,
		0x3820,
		0x3821,
		0x4502,
		0x4826,
		0x4837,
	},
	.table[0] = {
		.ext_reg_fill = NULL,
		.data = {
			0xe3,// 0x3708
			0x00,// 0x3800
			0x00,// 0x3801
			0x00,// 0x3802
			0x00,// 0x3803
			0x0a,// 0x3804
			0x3f,// 0x3805
			0x07,// 0x3806
			0xa3,// 0x3807
			0x0a,// 0x3808
			0x20,// 0x3809
			0x07,// 0x380a
			0x9A,// 0x380b
			0x0c,// 0x380c
			0x98,// 0x380d
			0x07,// 0x380e
			0xc0,// 0x380f
			0x00,// 0x3810
			0x10,// 0x3811
			0x00,// 0x3812
			0x06,// 0x3813
			0x11,// 0x3814
			0x11,// 0x3815
			0x10,// 0x3820
			0x1e,// 0x3821
			0x08,// 0x4502
			0x28,// 0x4826
			0x0d,// 0x4837
		},
		.downsample = 1,
		.width = 2592,
		.height = 1946,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
	},
	.table[1] = {
		.ext_reg_fill = NULL,
		.data = {
			0xe3,// 0x3708
			0x00,// 0x3800
			0x00,// 0x3801
			0x00,// 0x3802
			0xf6,// 0x3803
			0x0a,// 0x3804
			0x3f,// 0x3805
			0x06,// 0x3806
			0xad,// 0x3807
			0x05,// 0x3808
			0x00,// 0x3809
			0x02,// 0x380a
			0xd2,// 0x380b
			0x10,// 0x380c
			0x70,// 0x380d
			0x02,// 0x380e
			0xf8,// 0x380f
			0x00,// 0x3810
			0x07,// 0x3811
			0x00,// 0x3812
			0x02,// 0x3813
			0x31,// 0x3814
			0x31,// 0x3815
			0x11,// 0x3820
			0x1f,// 0x3821
			0x48,// 0x4502
			0x38,// 0x4826
			0x0a,// 0x4837
		},
		.downsample = 1,
		.width = 1280,
		.height = 722,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_59_94,
		.pll_index = 1,
	},
	/* add video format table here, if necessary */
};

/* OV5658 global gain table row size */
#define OV5658_GAIN_ROWS		(128)
#define OV5658_GAIN_COLS		(2)
#define OV5658_GAIN_DOUBLE		(47)
#define OV5658_GAIN_0DB			(127)

#define OV5658_GAIN_COL_AGC		(0)
#define OV5658_GAIN_COL_REG350A		(0)
#define OV5658_GAIN_COL_REG350B		(1)

/* This is 16-step gain table, OV5658_GAIN_ROWS = 81, OV5658_GAIN_COLS = 3 */
const s16 OV5658_GAIN_TABLE[OV5658_GAIN_ROWS][OV5658_GAIN_COLS] = {
//	The db_max=0x30000000(48 db) db_step=0x00++0x60c183 (0.377953 db)
	{0x7,0xff}, /*index 0 actual gain = 47.889034db  call gain= 48.000000db deviation=0.110966db*/
	{0x7,0xfe}, /*index 1 actual gain = 47.604225db  call gain= 47.622047db deviation=0.017822db*/
	{0x7,0xfd}, /*index 2 actual gain = 47.309760db  call gain= 47.244094db deviation=-0.065665db*/
	{0x7,0xfc}, /*index 3 actual gain = 47.004960db  call gain= 46.866142db deviation=-0.138819db*/
	{0x7,0xfb}, /*index 4 actual gain = 46.689075db  call gain= 46.488189db deviation=-0.200886db*/
	{0x7,0xfa}, /*index 5 actual gain = 46.361267db  call gain= 46.110236db deviation=-0.251030db*/
	{0x7,0xf9}, /*index 6 actual gain = 46.020600db  call gain= 45.732283db deviation=-0.288316db*/
	{0x7,0xf8}, /*index 7 actual gain = 45.666025db  call gain= 45.354331db deviation=-0.311694db*/
	{0x7,0xf7}, /*index 8 actual gain = 45.296356db  call gain= 44.976378db deviation=-0.319979db*/
	{0x7,0xf6}, /*index 9 actual gain = 44.910253db  call gain= 44.598425db deviation=-0.311828db*/
	{0x7,0xf5}, /*index 10 actual gain = 44.506186db  call gain= 44.220472db deviation=-0.285713db*/
	{0x7,0xf4}, /*index 11 actual gain = 44.082400db  call gain= 43.842520db deviation=-0.239880db*/
	{0x7,0xf3}, /*index 12 actual gain = 43.636872db  call gain= 43.464567db deviation=-0.172305db*/
	{0x7,0xf2}, /*index 13 actual gain = 43.167250db  call gain= 43.086614db deviation=-0.080636db*/
	{0x7,0xf1}, /*index 14 actual gain = 42.670778db  call gain= 42.708661db deviation=0.037883db*/
	{0x7,0xf0}, /*index 15 actual gain = 42.144199db  call gain= 42.330709db deviation=0.186509db*/
	{0x3,0xff}, /*index 16 actual gain = 41.868434db  call gain= 41.952756db deviation=0.084322db*/
	{0x3,0xfe}, /*index 17 actual gain = 41.583625db  call gain= 41.574803db deviation=-0.008822db*/
	{0x3,0xfd}, /*index 18 actual gain = 41.289160db  call gain= 41.196850db deviation=-0.092309db*/
	{0x3,0xfc}, /*index 19 actual gain = 40.984360db  call gain= 40.818898db deviation=-0.165463db*/
	{0x3,0xfb}, /*index 20 actual gain = 40.668475db  call gain= 40.440945db deviation=-0.227530db*/
	{0x3,0xfa}, /*index 21 actual gain = 40.340667db  call gain= 40.062992db deviation=-0.277675db*/
	{0x3,0xf9}, /*index 22 actual gain = 40.000000db  call gain= 39.685039db deviation=-0.314961db*/
	{0x3,0xf8}, /*index 23 actual gain = 39.645425db  call gain= 39.307087db deviation=-0.338338db*/
	{0x3,0xf7}, /*index 24 actual gain = 39.275757db  call gain= 38.929134db deviation=-0.346623db*/
	{0x3,0xf6}, /*index 25 actual gain = 38.889653db  call gain= 38.551181db deviation=-0.338472db*/
	{0x3,0xf5}, /*index 26 actual gain = 38.485586db  call gain= 38.173228db deviation=-0.312357db*/
	{0x3,0xf4}, /*index 27 actual gain = 38.061800db  call gain= 37.795276db deviation=-0.266524db*/
	{0x3,0xf3}, /*index 28 actual gain = 37.616272db  call gain= 37.417323db deviation=-0.198949db*/
	{0x3,0xf2}, /*index 29 actual gain = 37.146650db  call gain= 37.039370db deviation=-0.107280db*/
	{0x3,0xf1}, /*index 30 actual gain = 36.650178db  call gain= 36.661417db deviation=0.011239db*/
	{0x3,0xf0}, /*index 31 actual gain = 36.123599db  call gain= 36.283465db deviation=0.159865db*/
	{0x1,0xff}, /*index 32 actual gain = 35.847834db  call gain= 35.905512db deviation=0.057678db*/
	{0x1,0xfe}, /*index 33 actual gain = 35.563025db  call gain= 35.527559db deviation=-0.035466db*/
	{0x1,0xfd}, /*index 34 actual gain = 35.268560db  call gain= 35.149606db deviation=-0.118954db*/
	{0x1,0xfc}, /*index 35 actual gain = 34.963761db  call gain= 34.771654db deviation=-0.192107db*/
	{0x1,0xfb}, /*index 36 actual gain = 34.647875db  call gain= 34.393701db deviation=-0.254174db*/
	{0x1,0xfa}, /*index 37 actual gain = 34.320067db  call gain= 34.015748db deviation=-0.304319db*/
	{0x1,0xf9}, /*index 38 actual gain = 33.979400db  call gain= 33.637795db deviation=-0.341605db*/
	{0x1,0xf8}, /*index 39 actual gain = 33.624825db  call gain= 33.259843db deviation=-0.364982db*/
	{0x1,0xf7}, /*index 40 actual gain = 33.255157db  call gain= 32.881890db deviation=-0.373267db*/
	{0x1,0xf6}, /*index 41 actual gain = 32.869054db  call gain= 32.503937db deviation=-0.365117db*/
	{0x1,0xf5}, /*index 42 actual gain = 32.464986db  call gain= 32.125984db deviation=-0.339002db*/
	{0x1,0xf4}, /*index 43 actual gain = 32.041200db  call gain= 31.748031db deviation=-0.293168db*/
	{0x1,0xf3}, /*index 44 actual gain = 31.595672db  call gain= 31.370079db deviation=-0.225593db*/
	{0x1,0xf2}, /*index 45 actual gain = 31.126050db  call gain= 30.992126db deviation=-0.133924db*/
	{0x1,0xf1}, /*index 46 actual gain = 30.629578db  call gain= 30.614173db deviation=-0.015405db*/
	{0x1,0xf0}, /*index 47 actual gain = 30.103000db  call gain= 30.236220db deviation=0.133221db*/
	{0x0,0xff}, /*index 48 actual gain = 29.827234db  call gain= 29.858268db deviation=0.031034db*/
	{0x0,0xfe}, /*index 49 actual gain = 29.542425db  call gain= 29.480315db deviation=-0.062110db*/
	{0x0,0xfd}, /*index 50 actual gain = 29.247960db  call gain= 29.102362db deviation=-0.145598db*/
	{0x0,0xfc}, /*index 51 actual gain = 28.943161db  call gain= 28.724409db deviation=-0.218751db*/
	{0x0,0xfb}, /*index 52 actual gain = 28.627275db  call gain= 28.346457db deviation=-0.280819db*/
	{0x0,0xfa}, /*index 53 actual gain = 28.299467db  call gain= 27.968504db deviation=-0.330963db*/
	{0x0,0xf9}, /*index 54 actual gain = 27.958800db  call gain= 27.590551db deviation=-0.368249db*/
	{0x0,0xf8}, /*index 55 actual gain = 27.604225db  call gain= 27.212598db deviation=-0.391626db*/
	{0x0,0xf7}, /*index 56 actual gain = 27.234557db  call gain= 26.834646db deviation=-0.399911db*/
	{0x0,0xf6}, /*index 57 actual gain = 26.848454db  call gain= 26.456693db deviation=-0.391761db*/
	{0x0,0xf5}, /*index 58 actual gain = 26.444386db  call gain= 26.078740db deviation=-0.365646db*/
	{0x0,0xf4}, /*index 59 actual gain = 26.020600db  call gain= 25.700787db deviation=-0.319813db*/
	{0x0,0xf3}, /*index 60 actual gain = 25.575072db  call gain= 25.322835db deviation=-0.252237db*/
	{0x0,0xf2}, /*index 61 actual gain = 25.105450db  call gain= 24.944882db deviation=-0.160568db*/
	{0x0,0xf1}, /*index 62 actual gain = 24.608978db  call gain= 24.566929db deviation=-0.042049db*/
	{0x0,0xf0}, /*index 63 actual gain = 24.082400db  call gain= 24.188976db deviation=0.106577db*/
	{0x0,0x7f}, /*index 64 actual gain = 23.806634db  call gain= 23.811024db deviation=0.004390db*/
	{0x0,0x7e}, /*index 65 actual gain = 23.521825db  call gain= 23.433071db deviation=-0.088754db*/
	{0x0,0x7d}, /*index 66 actual gain = 23.227360db  call gain= 23.055118db deviation=-0.172242db*/
	{0x0,0x7c}, /*index 67 actual gain = 22.922561db  call gain= 22.677165db deviation=-0.245395db*/
	{0x0,0x7b}, /*index 68 actual gain = 22.606675db  call gain= 22.299213db deviation=-0.307463db*/
	{0x0,0x7a}, /*index 69 actual gain = 22.278867db  call gain= 21.921260db deviation=-0.357607db*/
	{0x0,0x79}, /*index 70 actual gain = 21.938200db  call gain= 21.543307db deviation=-0.394893db*/
	{0x0,0x78}, /*index 71 actual gain = 21.583625db  call gain= 21.165354db deviation=-0.418271db*/
	{0x0,0x77}, /*index 72 actual gain = 21.213957db  call gain= 20.787402db deviation=-0.426555db*/
	{0x0,0x76}, /*index 73 actual gain = 20.827854db  call gain= 20.409449db deviation=-0.418405db*/
	{0x0,0x75}, /*index 74 actual gain = 20.423786db  call gain= 20.031496db deviation=-0.392290db*/
	{0x0,0x74}, /*index 75 actual gain = 20.000000db  call gain= 19.653543db deviation=-0.346457db*/
	{0x0,0x73}, /*index 76 actual gain = 19.554472db  call gain= 19.275591db deviation=-0.278882db*/
	{0x0,0x72}, /*index 77 actual gain = 19.084850db  call gain= 18.897638db deviation=-0.187212db*/
	{0x0,0x71}, /*index 78 actual gain = 18.588379db  call gain= 18.519685db deviation=-0.068693db*/
	{0x0,0x70}, /*index 79 actual gain = 18.061800db  call gain= 18.141732db deviation=0.079933db*/
	{0x0,0x3f}, /*index 80 actual gain = 17.786034db  call gain= 17.763780db deviation=-0.022255db*/
	{0x0,0x3e}, /*index 81 actual gain = 17.501225db  call gain= 17.385827db deviation=-0.115398db*/
	{0x0,0x3d}, /*index 82 actual gain = 17.206760db  call gain= 17.007874db deviation=-0.198886db*/
	{0x0,0x3c}, /*index 83 actual gain = 16.901961db  call gain= 16.629921db deviation=-0.272040db*/
	{0x0,0x3b}, /*index 84 actual gain = 16.586075db  call gain= 16.251969db deviation=-0.334107db*/
	{0x0,0x3a}, /*index 85 actual gain = 16.258267db  call gain= 15.874016db deviation=-0.384251db*/
	{0x0,0x39}, /*index 86 actual gain = 15.917600db  call gain= 15.496063db deviation=-0.421537db*/
	{0x0,0x38}, /*index 87 actual gain = 15.563025db  call gain= 15.118110db deviation=-0.444915db*/
	{0x0,0x37}, /*index 88 actual gain = 15.193357db  call gain= 14.740157db deviation=-0.453199db*/
	{0x0,0x36}, /*index 89 actual gain = 14.807254db  call gain= 14.362205db deviation=-0.445049db*/
	{0x0,0x35}, /*index 90 actual gain = 14.403186db  call gain= 13.984252db deviation=-0.418934db*/
	{0x0,0x34}, /*index 91 actual gain = 13.979400db  call gain= 13.606299db deviation=-0.373101db*/
	{0x0,0x33}, /*index 92 actual gain = 13.533872db  call gain= 13.228346db deviation=-0.305526db*/
	{0x0,0x32}, /*index 93 actual gain = 13.064250db  call gain= 12.850394db deviation=-0.213857db*/
	{0x0,0x31}, /*index 94 actual gain = 12.567779db  call gain= 12.472441db deviation=-0.095338db*/
	{0x0,0x30}, /*index 95 actual gain = 12.041200db  call gain= 12.094488db deviation=0.053288db*/
	{0x0,0x1f}, /*index 96 actual gain = 11.765434db  call gain= 11.716535db deviation=-0.048899db*/
	{0x0,0x1e}, /*index 97 actual gain = 11.480625db  call gain= 11.338583db deviation=-0.142043db*/
	{0x0,0x1d}, /*index 98 actual gain = 11.186160db  call gain= 10.960630db deviation=-0.225530db*/
	{0x0,0x1c}, /*index 99 actual gain = 10.881361db  call gain= 10.582677db deviation=-0.298684db*/
	{0x0,0x1b}, /*index 100 actual gain = 10.565476db  call gain= 10.204724db deviation=-0.360751db*/
	{0x0,0x1a}, /*index 101 actual gain = 10.237667db  call gain= 9.826772db deviation=-0.410896db*/
	{0x0,0x19}, /*index 102 actual gain = 9.897000db  call gain= 9.448819db deviation=-0.448182db*/
	{0x0,0x18}, /*index 103 actual gain = 9.542425db  call gain= 9.070866db deviation=-0.471559db*/
	{0x0,0x17}, /*index 104 actual gain = 9.172757db  call gain= 8.692913db deviation=-0.479844db*/
	{0x0,0x16}, /*index 105 actual gain = 8.786654db  call gain= 8.314961db deviation=-0.471693db*/
	{0x0,0x15}, /*index 106 actual gain = 8.382586db  call gain= 7.937008db deviation=-0.445578db*/
	{0x0,0x14}, /*index 107 actual gain = 7.958800db  call gain= 7.559055db deviation=-0.399745db*/
	{0x0,0x13}, /*index 108 actual gain = 7.513272db  call gain= 7.181102db deviation=-0.332170db*/
	{0x0,0x12}, /*index 109 actual gain = 7.043650db  call gain= 6.803150db deviation=-0.240501db*/
	{0x0,0x11}, /*index 110 actual gain = 6.547179db  call gain= 6.425197db deviation=-0.121982db*/
	{0x0,0x10}, /*index 111 actual gain = 6.020600db  call gain= 6.047244db deviation=0.026644db*/
	{0x0,0xf}, /*index 112 actual gain = 5.744834db  call gain= 5.669291db deviation=-0.075543db*/
	{0x0,0xe}, /*index 113 actual gain = 5.460025db  call gain= 5.291339db deviation=-0.168687db*/
	{0x0,0xd}, /*index 114 actual gain = 5.165560db  call gain= 4.913386db deviation=-0.252174db*/
	{0x0,0xc}, /*index 115 actual gain = 4.860761db  call gain= 4.535433db deviation=-0.325328db*/
	{0x0,0xb}, /*index 116 actual gain = 4.544876db  call gain= 4.157480db deviation=-0.387395db*/
	{0x0,0xa}, /*index 117 actual gain = 4.217067db  call gain= 3.779528db deviation=-0.437540db*/
	{0x0,0x9}, /*index 118 actual gain = 3.876401db  call gain= 3.401575db deviation=-0.474826db*/
	{0x0,0x8}, /*index 119 actual gain = 3.521825db  call gain= 3.023622db deviation=-0.498203db*/
	{0x0,0x7}, /*index 120 actual gain = 3.152157db  call gain= 2.645669db deviation=-0.506488db*/
	{0x0,0x6}, /*index 121 actual gain = 2.766054db  call gain= 2.267717db deviation=-0.498337db*/
	{0x0,0x5}, /*index 122 actual gain = 2.361986db  call gain= 1.889764db deviation=-0.472222db*/
	{0x0,0x4}, /*index 123 actual gain = 1.938200db  call gain= 1.511811db deviation=-0.426389db*/
	{0x0,0x3}, /*index 124 actual gain = 1.492672db  call gain= 1.133858db deviation=-0.358814db*/
	{0x0,0x2}, /*index 125 actual gain = 1.023050db  call gain= 0.755906db deviation=-0.267145db*/
	{0x0,0x1}, /*index 126 actual gain = 0.526579db  call gain= 0.377953db deviation=-0.148626db*/
	{0x0,0x0}, /*index 127 actual gain = 0.000000db  call gain= 0.000000db deviation=0.000000db*/
};

