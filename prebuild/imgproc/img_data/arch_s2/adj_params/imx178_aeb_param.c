#include "img_struct_arch.h"

statistics_config_t imx178_tile_config = {
	1,
	1,

	32,
	32,
	0,
	0,
	128,
	128,
	128,
	128,
	0,
	0x3fac,

	12,
	8,
	0,
	0,
	340,
	512,

	16,
	16,
	0,
	0,
	256,
	256,
	256,
	256,

	0,
	16383,
};

line_t imx178_50hz_lines[] = {
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_800,0}}
		},

		{
		{{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_102400,0}}
		}
};

line_t imx178_60hz_lines[] = {
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY120, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_800,0}}
		},

		{
		{{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_102400,0}}
		}
};
 line_t imx178_60p50hz_lines[]={
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY200, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_800,0}}
		},
		{
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_6400,0}}
		},
};
line_t imx178_60p60hz_lines[]={
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY240, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY240, ISO_100, 0}}, {{SHUTTER_1BY240, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY120, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_6400,0}}
		},
		{
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_6400,0}}
		},
};
img_awb_param_t imx178_awb_param = {
	{
		{2200, 1024, 1800},	//AUTOMATIC
		{1300, 1024, 3200},	//INCANDESCENT
		{1620, 1024, 2600},	//D4000
		{1930, 1024, 1950},	//D5000
		{2200, 1024, 1800},	//SUNNY
		{2350, 1024, 1650},	//CLOUDY
		{1750, 1024, 1800},	//FLASH
		{1600, 1024, 2600},	//FLUORESCENT
		{1857, 1024, 1700},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{1750, 1024, 1800},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{1050,1600,2700,3600,-1800,4900,-1800,6100,1000,1400,1250,2100,1 },	// 0	INCANDESCENT
		 {1200,2050,2060,3300,-1800,4900,-1800,6700,2000,-1200,1500,800,2},		// 1 D4000 & CWF
		 {1500,2200,1650,2500,-1800,4900,-1800,5900,900,-100,1000,700,4},		// 2	D5000
		 {1700,2500,1400,2100,-800,3100,-800,3800,1300,-1500,1100,-200,8},		// 3 SUNNY
		 {1950,2650,1300,2000,-800,3100,-800,3800,800,-600,1000,-400,4},		// 4 CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 5
		 {2000,2500, 1900, 3000, -1200,4600, -1500,6400, 1000,-400,  1300,-200, -1},	// 6 green region
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 7    FLASH
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 8	FLUORESCENT
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 9    FLUORESCENT_2
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 10	FLUORESCENT_3
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 }},
	},
	{	{ 0 ,6},	//LUT num. AUTOMATIC  INDOOR
		{ 0, 1},	//LUT num. INCANDESCENT
		{ 1, 1},	//LUT num. D4000
		{ 2, 1},	//LUT num. D5000
		{ 2, 5},	//LUT num. SUNNY
		{ 4, 3},	//LUT num. CLOUDY
		{ 7, 1},	//LUT num. FLASH
		{ 8, 1},	//LUT num. FLUORESCENT
		{ 9, 1},	//LUT num. FLUORESCENT_H
		{11, 1},	//LUT num. UNDER WATER
		{11, 1},	//LUT num. CUSTOM
		{ 0, 7},	//LUT num. AUTOMATIC  OUTDOOR
	 }
};
u32 imx178_ae_agc_dgain[AGC_DGAIN_TABLE_LENGTH]={
		1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
		1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	};
u32 imx178_ae_sht_dgain[SHUTTER_DGAIN_TABLE_LENGTH] =
{
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,
	1024,1032,1031,1024,1034,1028,1033,1032,1024,1033,1031,1023,1035,1031,1024,1038,
	1031,1025,1038,1031,1027,1024,1031,1029,1023,1032,1031,1024,1035,1032,1024,1037,
	1032,1025,1024,1034,1028,1027,1037,1033,1031,1024,1037,1032,1024,1039,1033,1025,
	1024,1034,1029,1027,1038,1034,1031,1024,1039,1033,1026,1024,1036,1030,1030,1024,
	1036,1035,1028,1024,1040,1032,1027,1024,1036,1034,1030,1024,1042,1037,1030,1029,
	1024,1037,1035,1029,1024,1041,1033,1026,1024,1038,1034,1033,1027,1024,1043,1036,
	1033,1030,1024,1043,1038,1031,1028,1024,1038,1037,1032,1025,1024,1041,1035,1034,
	1029,1024,1045,1038,1032,1031,1024,1042,1039,1031,1027,1024,1041,1038,1035,1029,
	1027,1024,1041,1040,1036,1030,1029,1024,1043,1042,1036,1031,1030,1024,1045,1044,
	1037,1033,1030,1024,1048,1044,1037,1035,1030,1024,1049,1044,1038,1037,1030,1025,
	1024,1045,1041,1041,1035,1032,1030,1024,1049,1046,1039,1036,1033,1025,1024,1049,
	1042,1041,1036,1030,1029,1024,1048,1047,1042,1037,1036,1030,1025,1024,1048,1045,
	1043,1037,1034,1031,1025,1024,1052,1046,1045,1040,1034,1034,1028,1024,1056,1050,
	1046,1044,1038,1035,1032,1026,1024,1054,1048,1047,1042,1036,1035,1030,1025,1024,
	1054,1051,1049,1043,1041,1038,1033,1032,1029,1024,1059,1053,1049,1048,1042,1039,
	1036,1030,1028,1024,1057,1056,1051,1048,1046,1040,1039,1034,1030,1029,1024,1061,
	1058,1052,1051,1046,1043,1040,1034,1034,1029,1025,1024,1059,1058,1054,1049,1048,
	1043,1040,1037,1032,1031,1026,1024,1063,1058,1057,1052,1049,1047,1041,1041,1036,
	1032,1030,1025,1024,1065,1061,1060,1054,1053,1050,1046,1045,1040,1038,1036,1031,
	1031,1026,1024,1068,1063,1063,1058,1055,1054,1048,1047,1044,1039,1038,1033,1031,
	1028,1024,1072,1068,1065,1063,1058,1057,1053,1049,1048,1043,1041,1038,1034,1033,
	1028,1025,1024,1070,1070,1066,1063,1062,1059,1053,1052,1050,1044,1044,1041,1036,
	1036,1032,1027,1028,1024,1110,1110,1103,1098,1097,1090,1086,1084,1077,1073,1071,
	1064,1061,1057,1050,1048,1044,1037,1036,1031,1024,1125,1120,1112,1112,1106,1099,
	1099,1092,1086,1085,1078,1073,1072,1065,1060,1058,1051,1047,1044,1037,1034,1031,
	1024,1134,1131,1123,1122,1117,1110,1109,1103,1097,1096,1090,1084,1083,1077,1072,
	1070,1063,1059,1057,1050,1047,1044,1037,1034,1030,1024,1150,1145,1138,1138,1132,
	1126,1126,1119,1114,1113,1106,1102,1101,1094,1091,1088,1081,1079,1075,1068,1067,
	1062,1056,1055,1049,1044,1043,1036,1031,1030,1024,1165,1163,1156,1154,1151,1144,
	1143,1138,1132,1131,1126,1120,1120,1114,1109,1109,1102,1098,1096,1089,1088,1084,
	1078,1077,1072,1066,1066,1060,1055,1054,1047,1044,1042,1036,1034,1030,1024,1191,
	1186,1181,1181,1175,1171,1170,1164,1161,1158,1152,1151,1147,1141,1141,1135,1131,
	1130,1124,1121,1119,1112,1111,1107,1101,1101,1096,1091,1090,1084,1081,1079,1073,
	1072,1067,1062,1062,1056,1053,1050,1044,1043,1038,1033,1033,1027,1024,1223,1217,
	1216,1212,1207,1207,1201,1199,1196,1191,1190,1186,1181,1181,1175,1173,1170,1165,
	1165,1160,1156,1155,1149,1147,1144,1139,1139,1133,1130,1128,1122,1122,1117,1113,
	1112,1107,1104,1102,1096,1096,1091,1087,1086,1080,1079,1076,1071,1070,1065,1062,
	1060,1054,1054,1050,1045,1045,1039,1037,1034,1028,1028,1024,1266,1263,1255,1251,
	1246,1238,1235,1228,1221,1218,1210,1205,1201,1192,1190,1183,1176,1173,1165,1160,
	1155,1147,1144,1138,1131,1127,1120,1115,1110,1102,1099,1092,1085,1082,1075,1069,
	1065,1057,1053,1048,1040,1037,1030,1024};

u8 imx178_dlight[2] = {128,4};
sensor_config_t imx178_sensor_config  = {
	RG,//pattern
	48,//max_gain_db
	60,//gain step
	3,//shutter_delay
	2,//agc_delay
};

