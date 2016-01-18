#include "img_struct_arch.h"

statistics_config_t imx185_tile_config = {
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
	0x3fff,

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
 line_t imx185_50hz_lines[] = {
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

 line_t imx185_60hz_lines[] = {
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
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_102400,0}}
		}

};
 line_t imx185_60p50hz_lines[]={
		{
		{{SHUTTER_1BY8000, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_100,0}}
		},

		{
		{{SHUTTER_1BY200, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_300, 0}}
		},

		{
		{{SHUTTER_1BY120, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_800,0}}
		},
		{
		{{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_102400,0}}
		},
};
line_t imx185_60p60hz_lines[]={
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
		{{SHUTTER_1BY60, ISO_100, 0}}, {{SHUTTER_1BY60, ISO_102400,0}}
		},
};
img_awb_param_t imx185_awb_param = {
	{
		{1620, 1024, 2300},	//AUTOMATIC
		{1060, 1024, 4020},	//INCANDESCENT
		{1360, 1024, 3030},	//D4000
		{1620, 1024, 2300},	//D5000
		{1860, 1024, 2000},	//SUNNY
		{2080, 1024, 1880},	//CLOUDY
		{1598, 1024, 1875},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{1860, 1024, 2000},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{800,1500,3400,4300,-2500,5800,-2500,7500,1300,1900,1000,3400,1},	// 0	INCANDESCENT
		 {990,1800,2500,3750,-2200,5050,-2500,7800,1300,560,1000,2400,2},	// 1    D4000
		 {1200,1950,1850,2800,-1700,4200,-2300,6900,1000,350,1000,1310,4},	// 2	 D5000
		 {1450,2180,1700,2280,-1100,3480,-800,3850,1200,-400,1000,380,8},	// 3    SUNNY
		 {1780,2370,1550,2150,-700,2900,-800,3850,1000,-650,1000,50,4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },					// 5    ...
		 {1810,2300,2160,2900,-1400,4900,-1200,5400,1600,-1300,3000,-2700,-1},		// 6	GREEN REGION
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
u32 imx185_ae_agc_dgain[AGC_DGAIN_TABLE_LENGTH]=
{
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

u32 imx185_ae_sht_dgain[SHUTTER_DGAIN_TABLE_LENGTH] =
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
	1024,1023,1023,1023,1023,1023,1023,1022,1022,1022,1022,1022,1022,1021,1021,1021,
	1021,1021,1021,1021,1020,1020,1020,1020,1020,1020,1020,1020,1020,1020,1020,1020,
	1020,1020,1020,1020,1020,1020,1020,1021,1021,1021,1021,1024,1023,1021,1024,1021,
	1023,1024,1025,1027,1024,1030,1024,1025,1024,1025,1021,1026,1027,1024,1028,1024,
	1025,1025,1027,1024,1028,1024,1025,1026,1027,1024,1028,1024,1025,1026,1027,1024,
	1028,1024,1025,1025,1027,1024,1028,1024,1026,1025,1021,1024,1023,1025,1024,1027,
	1024,1028,1027,1024,1026,1023,1024,1023,1026,1023,1024,1025,1021,1026,1022,1024,
	1024,1021,1023,1024,1026,1023,1023,1025,1024,1028,1024,1024,1027,1024,1024,1027,
	1024,1036,1029,1024,1040,1032,1024,1041,1033,1024,1038,1030,1024,1033,1024,1034,
	1030,1024,1035,1031,1024,1036,1031,1024,1037,1031,1024,1038,1030,1024,1024,1030,
	1025,1024,1030,1026,1024,1030,1028,1024,1032,1033,1029,1024,1038,1031,1024,1038,
	1031,1024,1024,1031,1024,1024,1033,1029,1029,1024,1034,1031,1024,1037,1032,1025,
	1024,1035,1029,1029,1024,1034,1035,1029,1024,1040,1032,1025,1024,1034,1031,1030,
	1024,1037,1033,1025,1024,1038,1032,1034,1029,1024,1041,1033,1025,1024,1035,1030,
	1030,1024,1038,1036,1030,1026,1024,1036,1035,1031,1024,1024,1038,1033,1034,1029,
	1024,1044,1037,1030,1030,1024,1038,1037,1030,1025,1024,1038,1034,1033,1027,1024,
	1042,1036,1034,1030,1024,1044,1040,1034,1034,1029,1024,1046,1041,1035,1035,1030,
	1024,1047,1041,1035,1035,1029,1024,1047,1040,1035,1035,1028,1024,1046,1039,1035,
	1033,1026,1024,1045,1040,1038,1035,1029,1028,1024,1043,1043,1039,1033,1034,1029,
	1024,1049,1043,1037,1037,1030,1024,1024,1044,1040,1040,1034,1030,1029,1024,1048,
	1046,1040,1038,1035,1029,1029,1024,1046,1046,1040,1034,1035,1029,1023,1024,1047,
	1042,1042,1038,1033,1032,1029,1024,1050,1048,1043,1039,1038,1033,1029,1029,1024,
	1049,1049,1044,1038,1039,1033,1028,1028,1024,1050,1050,1046,1041,1041,1037,1032,
	1031,1028,1024,1053,1052,1047,1043,1042,1037,1033,1033,1028,1024,1057,1052,1047,
	1047,1042,1037,1038,1033,1028,1028,1024,1053,1053,1049,1045,1044,1041,1036,1035,
	1032,1028,1026,1024,1054,1052,1050,1046,1043,1041,1037,1034,1033,1028,1024,1024,
	1056,1053,1052,1048,1044,1044,1040,1036,1036,1031,1027,1028,1024,1058,1058,1054,
	1050,1051,1047,1043,1043,1039,1035,1035,1031,1027,1027,1024,1059,1059,1055,1051,
	1051,1047,1043,1043,1039,1035,1036,1031,1027,1028,1024,1061,1062,1058,1054,1054,
	1050,1047,1046,1043,1039,1039,1035,1032,1031,1027,1025,1024,1063,1061,1060,1056,
	1055,1052,1049,1048,1045,1041,1041,1038,1034,1034,1031,1027,1027,1024,1065,1066,
	1062,1059,1059,1056,1053,1053,1049,1047,1046,1043,1042,1040,1036,1036,1033,1030,
	1030,1027,1024,1071,1068,1064,1065,1061,1058,1058,1055,1053,1052,1049,1047,1046,
	1042,1041,1039,1036,1036,1033,1030,1030,1027,1024,1073,1070,1067,1067,1064,1062,
	1061,1058,1056,1055,1053,1051,1050,1047,1046,1044,1041,1040,1038,1035,1035,1032,
	1029,1029,1027,1024,1076,1073,1070,1070,1068,1065,1065,1062,1060,1060,1057,1055,
	1055,1052,1051,1050,1047,1046,1044,1042,1041,1039,1036,1036,1034,1031,1031,1029,
	1026,1026,1024,1076,1076,1074,1071,1071,1069,1067,1067,1065,1063,1062,1060,1059,
	1058,1055,1054,1053,1051,1050,1049,1046,1046,1044,1042,1042,1039,1037,1037,1035,
	1033,1033,1030,1028,1028,1026,1024,1024,1077,1073,1071,1066,1063,1060,1056,1052,
	1049,1045,1042,1038,1034,1032,1028,1024,
};
u8 imx185_dlight[2] = {128,4};

sensor_config_t imx185_sensor_config = {
	RG,//pattern
	48,//max_gain_db
	20,//gain step
	2,//shutter_delay
	1,//agc_delay
};