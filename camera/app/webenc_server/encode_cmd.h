#ifndef __ENCODE_CMD_H
#define __ENCODE_CMD_H

typedef enum {
  FULL_FRAMERATE_MODE,
  WARP_MODE,
  HIGH_MEGA_MODE,
  MODE_TOTAL_NUM,
} CAMERA_MODE;

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
  UNSUPPORTED_ID,
  CMD_COUNT,
} ENC_COMMAND_ID;

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
  TRANSFORM_REGION_REPLACE = 0,
  TRANSFORM_REGION_APPEND,
}TRANSFORM_REGION_SETTING;


typedef struct {
  uint32_t layout;
} FISHEYE_LAYOUT;

struct OSDText {
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
  OSDText() {
    font_size = 32;
    font_color = 0;
    outline = 0;
    bold = 0;
    italic = 0;
    offset_x = 0;
    offset_y = 0;
    width = 0;
    height = 0;
  }
};
struct OSDITEM {
  uint32_t osd_time_enabled;
  uint32_t osd_bmp_enabled;
  uint32_t osd_text_enabled;
  OSDText osd_text;
  OSDITEM() {
    osd_time_enabled = 0;
    osd_bmp_enabled = 0;
    osd_text_enabled = 0;
  }
};

struct IQ_MODE_STATUS {
  int dc_iris_mode_enable;
  int backlight_compensation_enable;
  int daynight_mode_enable;
  IQ_MODE_STATUS() {
    dc_iris_mode_enable = 0;
    backlight_compensation_enable = 0;
    daynight_mode_enable = 0;
  }
};

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

#endif
