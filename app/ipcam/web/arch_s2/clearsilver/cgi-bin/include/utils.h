#ifndef CS_UTIL_H
#define CS_UTIL_H

#include <mqueue.h>
#include <stdint.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
//Ambapack

#define PARAM_NAME_LEN 24
#define PARAM_VALUE_LEN 64
#define MAX_MESSAGES_SIZE (1024)
#define ENC_MQ_SEND "/MQ_ENCODE_RECEIVE"
#define ENC_MQ_RECEIVE "/MQ_ENCODE_SEND"
#define IMG_MQ_SEND "/MQ_IMAGE_RECEIVE"
#define IMG_MQ_RECEIVE "/MQ_IMAGE_SEND"
#define create_res(width,height) ((width << 16) + height)
#define SET_BIT(x,  n)     do {  (x) =  ((x) | ( 1 << (n))) ; } while(0)

typedef enum {
    false = 0,
    true = 1
}bool;

#define LOG_MESSG(format, args...)	\
    do {								\
        FILE* LOG; 					\
        LOG = fopen("debug","a+"); 		\
        fprintf(LOG, format"\n", ##args); 	\
        fclose(LOG);					\
    }while(0)

typedef enum {
  SET_PRVACY_MASK = 0,
  GET_PRVACY_MASK,
  GET_CAM_TYPE,
  GET_DPTZ_ZPT_ENABLE,
  SET_DPTZ,
  GET_DPTZ,
  SET_STREAM_TYPE,
  GET_STREAM_TYPE,
  SET_CBR_BITRATE,
  GET_CBR_BITRATE,
  SET_STREAM_FRAMERATE,
  GET_STREAM_FRAMERATE,
  SET_STREAM_SIZE,
  GET_STREAM_SIZE,
  SET_H264_N,
  GET_H264_N,
  SET_IDR_INTERVAL,
  GET_IDR_INTERVAL,
  SET_PROFILE,
  GET_PROFILE,
  SET_MJPEG_Q,
  GET_MJPEG_Q,
  SET_OSD_TIME,
  SET_OSD_BMP,
  SET_OSD_TEXT,
  GET_OSD,
  MULTI_SET,
  MULTI_GET,
  SET_TRANSFORM_MODE,
  GET_TRANSFORM_MODE,
  SET_TRANSFORM_REGION,
  SET_TRANSFORM_ZPT,
  GET_TRANSFORM_REGION,
  SET_FISHEYE_PARAMETERS,
  GET_FISHEYE_PARAMETERS,
  ENC_UNSUPPORTED_ID,
  ENC_CMD_COUNT,
} ENC_COMMAND_ID;

typedef enum {
  GET_IQ = 0,
  SET_IQ,
  IMG_UNSUPPORTED_ID,
  IMG_CMD_COUNT,
} IMG_COMMAND_ID;

typedef enum {
  FULL_FRAMERATE_MODE,
  WARP_MODE,
  HIGH_MEGA_MODE,
  MODE_TOTAL_NUM,
} CAMERA_MODE;

typedef struct {
    uint32_t cmd_id;
    uint32_t status; //0--success ;1--failure
    char data[0];
} MESSAGE;

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_FAILURE,
}REQUEST_STATUS;

typedef struct {
    uint32_t cmd_id;
    uint32_t count;
    char data[0];
} MULTI_MESSAGES;

typedef struct {
  int id;
  int left;
  int top;
  int width;
  int height;
  uint32_t action;
} PRIVACY_MASK;

typedef struct {
    uint32_t width;
    uint32_t height;
    int      x;
    int      y;
}Rect;

typedef struct {
    uint16_t width;
    uint16_t height;
}Resolution;

typedef struct {
    Rect       unwarp_window;
    Resolution unwarp;
    Resolution warp;
}WarpParameters;

typedef enum {
    AM_FISHEYE_MOUNT_WALL = 0,
    AM_FISHEYE_MOUNT_CEILING,
    AM_FISHEYE_MOUNT_DESKTOP,
    AM_FISHEYE_MOUNT_TOTAL_NUM
}FisheyeMount;

typedef enum {
    AM_FISHEYE_PROJECTION_FTHETA = 0,
    AM_FISHEYE_PROJECTION_FTAN
}FisheyeProjection;

typedef struct {
    uint32_t      config_changed;
    FisheyeMount      mount;
    FisheyeProjection projection;
    uint32_t          max_fov;
    uint32_t          max_circle;
    WarpParameters    layout;
}FisheyeParameters;

typedef struct {
    uint32_t layout;
}FisheyeLayout;

typedef enum {
    WALL_FISHEYE,
    WALL_NORMAL,
    WALL_FISHEYE_SUBREGION_PANORAMA180,
    CEILING_FISHEYE,
    CEILING_NORTH_SOUTH,
    CEILING_WEST_EAST,
    CEILING_SUBREGIONX4,
    CEILING_PANORAMA360,
} LAYOUT_TYPE;

typedef enum {
    AM_TRANSFORM_MODE_NONE = 0,
    AM_TRANSFORM_MODE_NORMAL,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_TOTAL_NUM
}TransformMode;

typedef enum {
    AM_STREAM_ID_0 = 0,
    AM_STREAM_ID_1,
    AM_STREAM_ID_2,
    AM_STREAM_ID_3,
    AM_STREAM_ID_4,
    AM_STREAM_ID_5,
    AM_STREAM_ID_6,
    AM_STREAM_ID_7,
    AM_STREAM_ID_MAX
}StreamId;

typedef enum {
    AM_ENCODE_TYPE_NONE = 0,
    AM_ENCODE_TYPE_H264,
    AM_ENCODE_TYPE_MJPEG,
}EncodeType;

typedef struct {
    uint32_t   width;
    uint32_t   height;
    uint32_t   rotate;
    uint32_t   src_unwarp;
    Rect src_window;
}EncodeSize;

typedef struct {
    int numer;
    int denom;
}Fraction;

typedef enum {
    AM_FISHEYE_ORIENT_NORTH = 0,
    AM_FISHEYE_ORIENT_SOUTH,
    AM_FISHEYE_ORIENT_WEST,
    AM_FISHEYE_ORIENT_EAST,
}FisheyeOrient;

typedef struct {
    int x;
    int y;
}Point;

typedef struct {
    int pan;
    int tilt;
}PanTiltAngle;

typedef enum {
    TRANSFORM_REGION_REPLACE = 0,
    TRANSFORM_REGION_APPEND,
}TransformRegionSetting;

typedef struct {
    uint32_t      id;
    Rect          region;           // all mode
    Fraction      zoom;             // all mode except NONE

    uint32_t      hor_angle_range;  // wall: PANORAMA, ceiling: PANORAMA

    int           roi_top_angle;    // ceiling: NORMAL/PANORAMA
    FisheyeOrient orient;           // ceiling: NORMAL/PANORAMA

    Point         roi_center;       // wall: SUBREGION, ceiling: SUBREGION
    PanTiltAngle  pantilt_angle;    // wall: SUBREGION, ceiling: SUBREGION

    Rect          source;           // NONE
}TransformParameter;

typedef struct {
    uint32_t id;
    Fraction      zoom;
    PanTiltAngle  pantilt_angle;
}TRANS_PARAM;


typedef struct {
    uint32_t source_buffer;
    uint32_t zoom_factor;
    int offset_x;
    int offset_y;
} DPTZ_PARAM;

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
    RES_3200x800 = create_res(3200,800),
    RES_2048x2048 = create_res(2048,2048),
    RES_2048x1024 = create_res(2048,1024),
    RES_1024x1024 = create_res(1024,1024)
}RES_INDEX;

typedef enum {
    AM_NO_ROTATE_FLIP = 0,
    AM_HORIZONTAL_FLIP = (1 << 0),
    AM_VERTICAL_FLIP = (1 << 1),
    AM_ROTATE_90 = (1 << 2),

    AM_CW_ROTATE_90 = AM_ROTATE_90,
    AM_CW_ROTATE_180 = AM_HORIZONTAL_FLIP | AM_VERTICAL_FLIP,
    AM_CW_ROTATE_270 = AM_CW_ROTATE_90 | AM_CW_ROTATE_180,
}RotateMode;

typedef enum {
    BUFFER_1 = 0,
    BUFFER_2,
    BUFFER_3,
    BUFFER_4,
    BUFFER_TOTAL_NUM,
} BUFFER_ID;

typedef struct {
    EncodeType encodetype;
    uint32_t framerate;
    EncodeSize streamsize;
    uint16_t h264_n;
    uint8_t idr_interval;
    uint8_t profile;
    uint32_t bitrate;
    uint8_t quality;
}ENCODE_PAGE;

typedef struct {
    char text[32];
    uint32_t font_size;
    uint32_t font_color;
    uint32_t outline;
    uint32_t bold;
    uint32_t italic;
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t width;
    uint32_t height;
}OSDText;

typedef struct {
    uint32_t osd_time_enabled;
    uint32_t osd_bmp_enabled;
    uint32_t osd_text_enabled;
    OSDText osd_text;
}OSDITEM;

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

extern mqd_t msg_queue_send, msg_queue_receive;
extern MESSAGE *send_buffer;
extern MESSAGE *receive_buffer;

int Setup_MQ ();
void SendData ();

#endif

