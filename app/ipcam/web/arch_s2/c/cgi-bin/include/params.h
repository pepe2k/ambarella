
#ifndef PARAM_H

#define PARAM_H


#define create_res(width,height) ((width << 16) + height)
#define create_shutter(shutter_time) (shutter_time * 10)
#define create_fr(rotate,vflip,hflip) ((rotate << 2)|(vflip << 1)|(hflip))

//param req_Id

typedef enum {
	REQ_GETVERSION = 3,
	REQ_GETINFO,
	REQ_GETCAMBASIC,
	REQ_SETCAMBASIC,
	REQ_GETCAMADV,
	REQ_SETCAMADV,
	REQ_GETENC,
	REQ_SETENC,
	REQ_GETMD,
	REQ_SETMD,
	REQ_GETPT,
	REQ_SETPT,
	REQ_GETPM,
	REQ_SETPM,
	REQ_GETREC,
	REQ_SETREC,
	REQ_SETFORCEIDR
}REQ_INDEX;


//param video fps

typedef enum {
	FPS_AUTO = 0,
	FPS_1 = 512000000,
	FPS_2 = 256000000,
	FPS_3 = 170666667,
	FPS_4 = 128000000,
	FPS_5 = 102400000,
	FPS_6 = 85333333,
	FPS_10 = 51200000,
	FPS_13 = 39384615,
	FPS_15 = 34133333,
	FPS_20 = 25600000,
	FPS_24 = 21333333,
	FPS_25 = 20480000,
	FPS_29 = 17083750, //fps: 29.97
	FPS_30 = 17066667,
	FPS_50 = 10240000,
	FPS_59 = 8541875, //fps: 59.94
	FPS_60 = 8533333
}FPS_INDEX;

//param bitrate control

typedef enum {
	BRC_CBR,
	BRC_VBR,
	BRC_CBRQ,
	BRC_VBRQ
}BRC_INDEX;


//param gop_model

typedef enum {
	GOPM_SIMPLE,
	GOPM_ADV
}GOPM_INDEX;


//param profile type

typedef enum {
	PROFILE_MAIN,
	PROFILE_BASELINE
}PROFILE_INDEX;

//param video mode

typedef enum {
	VIDEO_MODE_AUTO = 0,
	VIDEO_MODE_640x480 = 8,
	VIDEO_MODE_800x600,
	VIDEO_MODE_1024x760,
	VIDEO_MODE_1280x1024,
	VIDEO_MODE_1600x1200,
	VIDEO_MODE_1280x960 = 16,
	VIDEO_MODE_480I = 0xFFF0,
	VIDEO_MODE_576I,
	VIDEO_MODE_480P,
	VIDEO_MODE_576P,
	VIDEO_MODE_720P,
	VIDEO_MODE_1080I = 0xFFF6,
	VIDEO_MODE_1080P = 0xFFF8,
	VIDEO_MODE_1080P30 = 0xFFFD
}VIDEO_MODE_INDEX;


//param vout type

typedef enum {
	VOUT_OFF,
	VOUT_CVBS = ((0 << 16)|(1 << 0)),
	VOUT_HDMI = ((0 << 16)|(1 << 2))
}VOUT_TYPE_INDEX;

//param encode mode

typedef enum {
	ENC_HIGH_FPS,
	ENC_HIGH_MP,
	ENC_LOW_DELAY
}ENC_MODE_INDEX;


//param encode resolution

typedef enum {
	RES_1920x1080 = create_res(1920,1080),
	RES_1440x1080 = create_res(1440,1080),
	RES_1280x1024 = create_res(1280,1024),
	RES_1280x960 = create_res(1280,960),
	RES_1280x720 = create_res(1280,720),
	RES_800x600 = create_res(800,600),
	RES_720x576 = create_res(720,576),
	RES_720x480 = create_res(720,480),
	RES_640x480 = create_res(640,480),
	RES_352x288 = create_res(352,288),
	RES_352x240 = create_res(352,240),
	RES_320x240 = create_res(320,240),
	RES_176x144 = create_res(176,144),
	RES_176x120 = create_res(176,120),
	RES_160x120 = create_res(160,120)
}RES_INDEX;


//param encode high mega resolution

typedef enum {
	HIGHRES_2592x1944 = create_res(2592,1944), //2592 x 1944 (5.0M)
	HIGHRES_2560x1440 = create_res(2560,1440), //2560 x 1440 (3.7M)
	HIGHRES_2304x1296 = create_res(2304,1296), //2304 x 1296 (3.0M)
	HIGHRES_2048x1536 = create_res(2048,1536)  //2048 x 1536 (3.0M)
}HIGHRES_INDEX;

//param shutter limit

typedef enum {
	SHUTTER_8000 = 64000,	//	1/8000
	SHUTTER_1024 = 500000,	//	1/1024
	SHUTTER_960 = 533333,	//	1.960
	SHUTTER_480 = 1066667,	//	1/480
	SHUTTER_240 = 2133333,	//	1/240
	SHUTTER_120 = 4266667,	//	1/120
	SHUTTER_100 = 5120000,	//	1/100
	SHUTTER_60 = 8533333,	//	1/60
	SHUTTER_50 = 10240000,	//	1/50
	SHUTTER_30 = 17066667,	//	1/30
	SHUTTER_25 = 20480000,	//	1/25
	SHUTTER_15 = 34133333,	//	1/15
	SHUTTER_7 = 68266667	//	1/7.5
}SHUTTER_INDEX;



//param flip rotate

typedef enum {
	FR_NORMAL = create_fr(0,0,0),
	FR_HFLIP = create_fr(0,0,1),
	FR_VFLIP = create_fr(0,1,0),
	FR_ROTATE_90 = create_fr(1,0,0),
	FR_ROTATE_180 = create_fr(0,1,1),
	FR_ROTATE_270 = create_fr(1,1,1)
}FR_INDEX;


//param wb control

typedef enum {
	WB_AUTO = 1,
	WB_HOLD = 100,
	WB_INCANDESCENT = 2,
	WB_D4000,
	WB_D5000,
	WB_SUNNY,
	WB_CLOUDY,
	WB_FLASH,
	WB_FLUORESCENT,
	WB_FLUORESCENT_HIGH,
	WB_UNDER_WATER,
	WB_CUSTOM
}WB_INDEX;

//param lens id

typedef enum {
	LENS_FIXED_FOCUS,
	LENS_OTHERS = 100,
	LENS_TAMRON18X,
	LENS_JCD661
}LENS_INDEX;

//param add focus mode

typedef enum {
	AF_CAF,
	AF_SAF,
	AF_MAN = 3,
	AF_CALIB,
	AF_DEBUG
}AF_MODE_INDEX;

//param gain

typedef enum {
	GAIN_AUTO,
	GAIN_0db = 0,
	GAIN_3db = 3,
	GAIN_6db = 6,
	GAIN_9db = 9,
	GAIN_12db = 12,
	GAIN_15db = 15,
	GAIN_18db = 18,
	GAIN_24db = 24,
	GAIN_30db = 30,
	GAIN_36db = 36,
	GAIN_42db = 42,
	GAIN_48db = 48,
	GAIN_54db = 54,
	GAIN_60db = 60
}GAIN_INDEX;



//param focus distance


//param color type

typedef enum {
	COLOR_BLACK = 0,
	COLOR_RED,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_MEGENTA,
	COLOR_CYAN,
	COLOR_WHITE
}COLOR_INDEX;



//param osd text size


typedef enum {
	FTSIZE_SMALLER = 16,
	FTSIZE_SMALL = 24,
	FTSIZE_NORMAL = 32,
	FTSIZE_LARGE = 40,
	FTSIZE_LARGER = 48
}FTSIZE_INDEX;



#endif