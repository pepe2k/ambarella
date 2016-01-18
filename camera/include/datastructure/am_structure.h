/*******************************************************************************
 * am_structure.h
 *
 * Histroy:
 *  2012-2-20 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMSTRUCTURE_H_
#define AMSTRUCTURE_H_

#define MAX_ENCODE_STREAM_NUM 4
#define MAX_ENCODE_BUFFER_NUM 4
#define MAX_OVERLAY_AREA_NUM  3
#define MAX_OVERLAY_CLUT_SIZE 1024
#define MAX_WARP_AREA_NUM     8
#define MAX_AUDIO_ANALY_MOD_NUM   6
#define MAX_AUDIO_ANALY_MOD_NAME_LEN 128
#define MAX_MOTION_DETECT_ROI_NUM 4

extern char* amstrdup(const char* str);

enum StreamId {
  AM_STREAM_ID_0 = 0,
  AM_STREAM_ID_1,
  AM_STREAM_ID_2,
  AM_STREAM_ID_3,
#ifdef CONFIG_ARCH_S2
  AM_STREAM_ID_4,
  AM_STREAM_ID_5,
  AM_STREAM_ID_6,
  AM_STREAM_ID_7,
#endif
  AM_STREAM_ID_MAX
};

enum VinType  {
  AM_VIN_TYPE_NONE = 0,
  AM_VIN_TYPE_RGB  = 1,
  AM_VIN_TYPE_YUV  = 2
};

enum VoutType {
  AM_VOUT_TYPE_NONE = 0,
  AM_VOUT_TYPE_LCD  = 1,
  AM_VOUT_TYPE_HDMI = 2,
  AM_VOUT_TYPE_CVBS = 3
};

enum IavState {
  AM_IAV_IDLE          = 0,
  AM_IAV_PREVIEW       = 1,
  AM_IAV_ENCODING      = 2,
  AM_IAV_STILL_CAPTURE = 3,
  AM_IAV_DECODING      = 4,
  AM_IAV_TRANSCODING   = 5,
  AM_IAV_DUPLEX        = 6,
  AM_IAV_INIT          = 7,
  AM_IAV_INVALID
};

enum LcdPanelType {
  AM_LCD_PANEL_NONE     = 0,
  AM_LCD_PANEL_DIGITAL  = 1,
  AM_LCD_PANEL_TPO489   = 2,
  AM_LCD_PANEL_TD043    = 3,
  AM_LCD_PANEL_1P3828   = 4,
  AM_LCD_PANEL_1P3831   = 5
};

enum RecordFileType {
  AM_RECORD_FILE_TYPE_NULL    = 0,
  AM_RECORD_FILE_TYPE_RAW     = 1 << 0,
  AM_RECORD_FILE_TYPE_TS      = 1 << 1,
  AM_RECORD_FILE_TYPE_TS_HTTP = 1 << 2,  // Upload TS stream through HTTP
  AM_RECORD_FILE_TYPE_TS_HLS  = 1 << 3,
  AM_RECORD_FILE_TYPE_IPTS    = 1 << 4,
  AM_RECORD_FILE_TYPE_RTSP    = 1 << 5,
  AM_RECORD_FILE_TYPE_MP4     = 1 << 6,
  AM_RECORD_FILE_TYPE_JPEG    = 1 << 7,
  AM_RECORD_FILE_TYPE_MOV     = 1 << 8,
  AM_RECORD_FILE_TYPE_MKV     = 1 << 9,
  AM_RECORD_FILE_TYPE_AVI     = 1 << 10,
};

enum StreamStatus {
  AM_STREAM_STATUS_RECORDING,
  AM_STREAM_STATUS_PLAYING,
  AM_STREAM_STATUS_STOPPED,
  AM_STREAM_STATUS_PAUSED
};

enum PhotoType {
  AM_PHOTO_JPEG,
  AM_PHOTO_RAW
};

enum AudioFormat {
  AM_AUDIO_FORMAT_NONE,
  AM_AUDIO_FORMAT_AAC,
  AM_AUDIO_FORMAT_OPUS,
  AM_AUDIO_FORMAT_PCM,
  AM_AUDIO_FORMAT_BPCM,
  AM_AUDIO_FORMAT_G726
};

enum AudioChannel {
  AM_AUDIO_CHANNEL_LEFT,
  AM_AUDIO_CHANNEL_RIGHT,
  AM_AUDIO_CHANNEL_BOTH
};

enum AudioPcmFormat {
  AM_AUDIO_PCM_FORMAT_S16LE,
  AM_AUDIO_PCM_FORMAT_S16BE
};

enum AacFormat {
  AM_AAC_FORMAT_AAC       = 0,
  AM_AAC_FORMAT_AACPLUS   = 1,
  AM_AAC_FORMAT_AACPLUSPS = 3
};

enum AacQuantizerQuality {
  AM_AAC_QUANTIZER_QUALITY_LOW     = 0,
  AM_AAC_QUANTIZER_QUALITY_HIGH    = 1,
  AM_AAC_QUANTIZER_QUALITY_HIGHEST = 2
};

enum AudioOutputChannel {
  AM_AUDIO_OUTPUT_CHANNEL_MONO  = 1,
  AM_AUDIO_OUTPUT_CHANNEL_STERO = 2
};

enum G726Law {
  AM_G726_ULAW  = 0,
  AM_G726_ALAW  = 1,
  AM_G726_PCM16 = 2
};

enum G726Rate {
  AM_G726_16K = 2,
  AM_G726_24K = 3,
  AM_G726_32K = 4,
  AM_G726_40K = 5
};

enum EncodeType {
  AM_ENCODE_TYPE_NONE = 0,
  AM_ENCODE_TYPE_H264,
  AM_ENCODE_TYPE_MJPEG,
};

enum BufferAssignMethod {
  /* Each resolution has independent buffer; YUV shares buffer with stream */
  AM_BUFFER_TO_RESOLUTION = 0,
  /* Each stream has independent buffer; YUV shares buffer with stream */
  AM_BUFFER_TO_STREAM = 1,
  /* Each stream and YUV has independent buffer */
  AM_BUFFER_TO_STREAM_YUV = 2,
};

enum OverlayBufferNum {
  AM_OVERLAY_BUFFER_NUM_SINGLE = 1,
  AM_OVERLAY_BUFFER_NUM_DOUBLE = 2,
  AM_OVERLAY_BUFFER_NUM_MULTI = 4,
  AM_OVERLAY_BUFFER_NUM_MAX = AM_OVERLAY_BUFFER_NUM_MULTI,
};

enum RotateMode {
  AM_NO_ROTATE_FLIP = 0,
  AM_HORIZONTAL_FLIP = (1 << 0),
  AM_VERTICAL_FLIP = (1 << 1),
  AM_ROTATE_90 = (1 << 2),

  AM_CW_ROTATE_90 = AM_ROTATE_90,
  AM_CW_ROTATE_180 = AM_HORIZONTAL_FLIP | AM_VERTICAL_FLIP,
  AM_CW_ROTATE_270 = AM_CW_ROTATE_90 | AM_CW_ROTATE_180,
};

enum FisheyeMount {
  AM_FISHEYE_MOUNT_WALL = 0,
  AM_FISHEYE_MOUNT_CEILING,
  AM_FISHEYE_MOUNT_DESKTOP,
  AM_FISHEYE_MOUNT_TOTAL_NUM
};

enum FisheyeProjection {
  AM_FISHEYE_PROJECTION_FTHETA = 0,
  AM_FISHEYE_PROJECTION_FTAN
};

enum FisheyeOrient {
  AM_FISHEYE_ORIENT_NORTH = 0,
  AM_FISHEYE_ORIENT_SOUTH,
  AM_FISHEYE_ORIENT_WEST,
  AM_FISHEYE_ORIENT_EAST,
};

enum TransformMode {
  AM_TRANSFORM_MODE_NONE = 0,
  AM_TRANSFORM_MODE_NORMAL,
  AM_TRANSFORM_MODE_PANORAMA,
  AM_TRANSFORM_MODE_SUBREGION,
  AM_TRANSFORM_MODE_TOTAL_NUM
};

struct ADeviceParameters {
    uint32_t       config_changed;
    uint32_t       audio_channel_num;
    uint32_t       audio_sample_freq;
    ADeviceParameters() :
      config_changed(0),
      audio_channel_num(2),
      audio_sample_freq(48000){}
};

struct AacEncoderParameters {
    uint32_t            config_changed;
    AacFormat           aac_format;
    AacQuantizerQuality aac_quantizer_quality;
    AudioOutputChannel  aac_output_channel;
    uint32_t            aac_bitrate;
};

struct OpusEncoderParameters {
    uint32_t config_changed;
    uint32_t opus_complexity;
    uint32_t opus_avg_bitrate;
};

struct PcmEncoderParameters {
    uint32_t config_changed;
    uint32_t pcm_output_channel;
};

struct BpcmEncoderParameters {
    uint32_t config_changed;
    uint32_t bpcm_output_channel;
};

struct G726EncoderParameters {
    uint32_t config_changed;
    G726Law  g726_law;
    G726Rate g726_rate;
};

union AudioCodecParameters {
    struct AacEncoderParameters  aac;
    struct OpusEncoderParameters opus;
    struct PcmEncoderParameters  pcm;
    struct BpcmEncoderParameters bpcm;
    struct G726EncoderParameters g726;
};

struct AudioAlertParameters {
   uint32_t config_changed;
   uint32_t audio_alert_enable;
   uint32_t audio_alert_sen;
   uint32_t audio_alert_direction;
   AudioAlertParameters ():
      config_changed (0),
      audio_alert_enable (0),
      audio_alert_sen (2),
      audio_alert_direction (1) {}
};

struct AudioParameters {
    uint32_t       config_changed;
    uint32_t       audio_stream_map;
    AudioFormat    audio_format;
    AudioCodecParameters *codec;
    ADeviceParameters    *adevice_param;
    AudioAlertParameters *audio_alert;
    AudioParameters() :
      config_changed(0),
      audio_stream_map(0),
      audio_format(AM_AUDIO_FORMAT_AAC),
      codec(NULL),
      adevice_param(NULL),
      audio_alert (NULL) {}
    ~AudioParameters() {
      delete codec;
      delete adevice_param;
      delete audio_alert;
    }
    void operator=(AudioParameters& param)
    {
      config_changed   = param.config_changed;
      audio_stream_map = param.audio_stream_map;
      audio_format     = param.audio_format;
      if (param.codec) {
        codec = new AudioCodecParameters;
        memcpy(codec, param.codec, sizeof(*codec));
      }
      if (param.adevice_param) {
        adevice_param = new ADeviceParameters();
        memcpy(adevice_param, param.adevice_param, sizeof(*adevice_param));
      }
      if (param.audio_alert) {
        audio_alert = new AudioAlertParameters();
        memcpy(audio_alert, param.audio_alert, sizeof(*audio_alert));
      }
    }
};

struct CamVideoMode {
    const char     *videoMode;
    amba_video_mode ambaMode;
    uint32_t        width;
    uint32_t        height;
    CamVideoMode() :
      videoMode(NULL),
      ambaMode(AMBA_VIDEO_MODE_AUTO),
      width(0),
      height(0){}
    CamVideoMode(const char *videomode,
                 amba_video_mode ambamode,
                 uint32_t w,
                 uint32_t h)
    : videoMode(0), ambaMode(ambamode), width(w), height(h)
    {
      if (videomode) {
        videoMode = amstrdup(videomode);
      }
    }
    ~CamVideoMode() {
      delete[] videoMode;
    }
};

struct CameraVinFPS {
    const char *fpsName;
    uint32_t    fpsValue;
    CameraVinFPS() : fpsName(0), fpsValue(AMBA_VIDEO_FPS_AUTO){}
    CameraVinFPS(const char *fps, int32_t value)
    : fpsName(0), fpsValue(value)
    {
      if (fps) {
        fpsName = amstrdup(fps);
      }
    }
    CameraVinFPS(const CameraVinFPS& fps)
    {
      fpsName  = amstrdup(fps.fpsName);
      fpsValue = fps.fpsValue;
    }
    ~CameraVinFPS()
    {
      delete[] fpsName;
    }
};

struct PhotoParameters {
    uint32_t      config_changed;
    uint32_t      quality;
    char         *file_name_prefix;
    char         *file_store_location;

    PhotoParameters() :
      config_changed(0),
      quality(75),
      file_name_prefix(NULL),
      file_store_location(NULL){}

    ~PhotoParameters() {
      delete[] file_name_prefix;
      delete[] file_store_location;
    }

    void set_file_name_prefix(const char *prefix) {
      delete[] file_name_prefix;
      file_name_prefix = NULL;
      if (prefix) {
        file_name_prefix = amstrdup(prefix);
      }
    }

    void set_file_store_location(const char *location) {
      delete[] file_store_location;
      file_store_location = NULL;
      if (location) {
        file_store_location = amstrdup(location);
      }
    }
};

struct RecordParameters {
    uint32_t         config_changed;
    uint32_t         stream_number;
    uint32_t         file_duration;
    uint32_t         record_duration;
    uint32_t         event_stream_id;
    uint32_t         event_history_duration;
    uint32_t         event_future_duration;
    uint32_t         file_name_timestamp;
    uint32_t         stream_map;
    uint32_t         max_file_amount;
    uint32_t         rtsp_send_wait;
    uint32_t         rtsp_need_auth;
    uint32_t         stream_type[MAX_ENCODE_STREAM_NUM];
    RecordFileType   file_type[MAX_ENCODE_STREAM_NUM][MAX_ENCODE_STREAM_NUM];
    char            *file_name_prefix;
    char            *file_store_location;
    char            *ts_upload_url;
    AudioParameters *audio_params;

    RecordParameters() :
      config_changed(0),
      stream_number(0),
      file_duration(0),
      record_duration(0),
      event_stream_id(0),
      event_history_duration(0),
      event_future_duration(0),
      file_name_timestamp(0),
      stream_map(0),
      max_file_amount (0),
      rtsp_send_wait(0),
      rtsp_need_auth(0),
      file_name_prefix(NULL),
      file_store_location(NULL),
      ts_upload_url(NULL),
      audio_params(NULL)
    {
      memset(stream_type, 0, (sizeof(uint32_t) * MAX_ENCODE_STREAM_NUM));
      memset(file_type, AM_RECORD_FILE_TYPE_NULL,
             (sizeof(RecordFileType) * MAX_ENCODE_STREAM_NUM *
              MAX_ENCODE_STREAM_NUM));
    }

    ~RecordParameters() {
      delete[] file_name_prefix;
      delete[] file_store_location;
      delete[] ts_upload_url;
      delete   audio_params;
    }

    void operator=(RecordParameters& recordParam)
    {
      config_changed         = recordParam.config_changed;
      stream_number          = recordParam.stream_number;
      file_duration          = recordParam.file_duration;
      record_duration        = recordParam.record_duration;
      event_stream_id        = recordParam.event_stream_id;
      event_history_duration = recordParam.event_history_duration;
      event_future_duration  = recordParam.event_future_duration;
      file_name_timestamp    = recordParam.file_name_timestamp;
      stream_map             = recordParam.stream_map;
      max_file_amount        = recordParam.max_file_amount;
      memcpy(stream_type, recordParam.stream_type, sizeof(stream_type));
      memcpy(file_type, recordParam.file_type, sizeof(file_type));
      set_file_name_prefix(recordParam.file_name_prefix);
      set_file_store_location(recordParam.file_store_location);
      set_ts_upload_url(recordParam.ts_upload_url);
      set_audio_parameters(recordParam.audio_params);
    }

    void set_file_name_prefix(const char *prefix) {
      delete[] file_name_prefix;
      file_name_prefix = NULL;
      if (prefix) {
        file_name_prefix = amstrdup(prefix);
      }
    }

    void set_file_store_location(const char *location) {
      delete[] file_store_location;
      file_store_location = NULL;
      if (location) {
        file_store_location = amstrdup(location);
      }
    }

    void set_ts_upload_url(const char *url) {
      delete[] ts_upload_url;
      ts_upload_url = NULL;
      if (url) {
        ts_upload_url = amstrdup(url);
      }
    }

    void enable_file_name_timestamp(bool enable) {
      file_name_timestamp = (enable ? 1 : 0);
    }

    void set_audio_parameters(AudioParameters* audio) {
      if (!audio_params) {
        audio_params = new AudioParameters();
      }
      *audio_params = *audio;
    }
};

struct Resolution {
    uint16_t width;
    uint16_t height;
    Resolution(uint16_t w = 0, uint32_t h = 0):
    width(w), height(h){}
    Resolution(const Resolution &res):
      width(res.width),
      height(res.height) {}
};

struct Point {
    int x;
    int y;
    Point(int xx = 0, int yy = 0):
      x(xx),
      y(yy){}
    Point(const Point &p):
      x(p.x),
      y(p.y) {}
};

struct Rect {
    uint32_t width;
    uint32_t height;
    int      x;
    int      y;
    Rect(uint32_t w = 0, uint32_t h = 0, int xx = 0, int yy = 0):
      width(w), height(h), x(xx), y(yy){}
    Rect (const Rect &r):
      width(r.width),
      height(r.height),
      x(r.x),
      y(r.y) {}
};

struct Fraction {
    int numer;
    int denom;
    Fraction(int n = 1, int d = 1):
      numer(n),
      denom(d) {}
    Fraction(const Fraction &f):
      numer(f.numer),
      denom(f.denom) {}
};

struct EncodeSize {
    uint32_t   width;
    uint32_t   height;
    uint32_t   rotate;
    uint32_t   src_unwarp;
    Rect       src_window;
    EncodeSize(uint32_t w = 0, uint32_t h = 0, uint32_t r = 0,
               uint32_t unwarp = 0,
               uint32_t win_w = 0,
               uint32_t win_h = 0,
               uint32_t win_x = 0,
               uint32_t win_y = 0) :
        width(w), height(h), rotate(r), src_unwarp(unwarp),
        src_window(win_w, win_h, win_x, win_y)
    {
    }
    EncodeSize(uint32_t w,
               uint32_t h,
               uint32_t r,
               uint32_t unwarp,
               const Rect &win) :
        width(w), height(h), rotate(r), src_unwarp(unwarp),
        src_window(win)
    {
    }
    EncodeSize(const EncodeSize &s) :
        width(s.width), height(s.height), rotate(s.rotate),
        src_unwarp(s.src_unwarp),
        src_window(s.src_window)
    {
    }
    void SetEncodeSize(uint32_t w,
               uint32_t h,
               uint32_t r,
               uint32_t unwarp,
               const Rect &win)
    {
    width = w;
    height = h;
    rotate = r;
    src_unwarp = unwarp;
    src_window = win;
    }
};

struct OverlayAreaBuffer {
    OverlayBufferNum num;
    uint32_t         active_id;
    uint32_t         next_id;
    uint32_t         max_size;
    uint8_t          *clut_addr;
    uint8_t          *data_addr[AM_OVERLAY_BUFFER_NUM_MAX];

    OverlayAreaBuffer():
      num(AM_OVERLAY_BUFFER_NUM_SINGLE),
      active_id(0),
      next_id(0),
      max_size(0),
      clut_addr(NULL)
    {
      memset(data_addr, 0, sizeof(data_addr));
    }
    ~OverlayAreaBuffer() {
    }
};

struct OverlayFormat {
    OverlayAreaBuffer buffer[MAX_OVERLAY_AREA_NUM];
    overlay_insert_ex_t insert;
};


struct Overlay {
    uint16_t enable;
    uint16_t width;
    uint16_t pitch;
    uint16_t height;
    uint16_t offset_x;
    uint16_t offset_y;
    uint32_t clut_size;
    uint8_t  *clut_addr;
    uint8_t  *data_addr;
    Overlay():
      enable(0),
      width(0),
      pitch(0),
      height(0),
      offset_x(0),
      offset_y(0),
      clut_size(0),
      clut_addr(NULL),
      data_addr(NULL)
    {
    }
    ~Overlay(){
    }
};

struct OverlayClut {
    uint8_t v;
    uint8_t u;
    uint8_t y;
    uint8_t alpha;
    OverlayClut(uint8_t Y = 0, uint8_t U = 0, uint8_t V = 0, uint8_t a = 0):
      v(V),
      u(U),
      y(Y),
      alpha(a){}
    ~OverlayClut(){}
};

struct Font {
    const char     *ttf;
    uint32_t size;
    uint32_t outline_width;
    int32_t  ver_bold;
    int32_t  hor_bold;
    int32_t  italic;
    uint32_t disable_anti_alias;
    Font() :
      ttf(NULL),
      size(0),
      outline_width(0),
      ver_bold(0),
      hor_bold(0),
      italic(0),
      disable_anti_alias(0) {}

    ~Font(){
    }
};

struct TextBox {
    uint16_t width;
    uint16_t height;
    Font     *font;
    OverlayClut *font_color;
    OverlayClut *outline_color;
    OverlayClut *background_color;
    TextBox():
      width(0),
      height(0),
      font(NULL),
      font_color(NULL),
      outline_color(NULL),
      background_color(NULL) {}

     ~TextBox() {
    }
};

#ifdef CONFIG_ARCH_S2

struct WarpParameters {
    Rect       unwarp_window;
    Resolution unwarp;
    Resolution warp;
    WarpParameters():
      unwarp_window(),
      unwarp(),
      warp() {}
    WarpParameters(const WarpParameters &w):
      unwarp_window(w.unwarp_window),
      unwarp(w.unwarp),
      warp(w.warp) {}

};

struct WarpMap {
    uint32_t cols;
    uint32_t rows;
    uint32_t hor_spacing;
    uint32_t ver_spacing;
    int16_t  *addr;
};

struct WarpControl {
    uint32_t   id;
    Resolution input;
    Point      input_offset;
    Resolution output;
    Point      output_offset;
    uint32_t   rotate;
    WarpMap    hor_map;
    WarpMap    ver_map;
};

struct PanTiltAngle {
    int pan;
    int tilt;
    PanTiltAngle(int p = 0, int t = 0) :
      pan(p),
      tilt(t){}
};

struct TransformParameters {
    uint32_t      id;
    Rect          region;           // all mode
    Fraction      zoom;             // all mode except NONE

    uint32_t      hor_angle_range;  // wall: PANORAMA, ceiling: PANORAMA

    int           roi_top_angle;    // ceiling: NORMAL/PANORAMA
    FisheyeOrient orient;           // ceiling: NORMAL/PANORAMA

    Point         roi_center;       // wall: SUBREGION, ceiling: SUBREGION
    PanTiltAngle  pantilt_angle;    // wall: SUBREGION, ceiling: SUBREGION

    Rect          source;           // NONE
    TransformParameters():
          id(0), region(), zoom(), hor_angle_range(0), roi_top_angle(0),
          orient(AM_FISHEYE_ORIENT_NORTH), roi_center(), pantilt_angle(),
          source(){}
    TransformParameters(uint32_t i,
                       const Rect &r,
                       const Fraction &z,
                       uint32_t h_angle,
                       int top_angle,
                       FisheyeOrient o,
                       const Point &p,
                       const PanTiltAngle &pt,
                       const Rect &s) :
        id(i), region(r), zoom(z), hor_angle_range(h_angle),
            roi_top_angle(top_angle), orient(o), roi_center(p),
            pantilt_angle(pt), source(s)
    {
    }
    void SetTransformParameters(uint32_t i,
                       const Rect &r,
                       const Fraction &z,
                       uint32_t h_angle,
                       int top_angle,
                       FisheyeOrient o,
                       const Point &p,
                       const PanTiltAngle &pt,
                       const Rect &s)
     {
        id = i;
        region = r;
        zoom = z;
        hor_angle_range = h_angle;
        roi_top_angle = top_angle;
        orient = o;
        roi_center = p;
        pantilt_angle = pt;
        source = s;
     }
};

struct FisheyeParameters {
    uint32_t          config_changed;
    FisheyeMount      mount;
    FisheyeProjection projection;
    uint32_t          max_fov;
    uint32_t          max_circle;
    WarpParameters    layout;
    FisheyeParameters():
      config_changed(0),
      mount(AM_FISHEYE_MOUNT_WALL),
      projection(AM_FISHEYE_PROJECTION_FTHETA),
      max_fov(0),
      max_circle(0)
    {
      memset(&layout, 0, sizeof(WarpParameters));
    }
    bool set(FisheyeParameters &fish) {
      mount = fish.mount;
      projection = fish.projection;
      max_fov = fish.max_fov;
      max_circle = fish.max_circle;
      layout.unwarp.width = fish.layout.unwarp.width;
      layout.unwarp.height = fish.layout.unwarp.height;
      layout.unwarp_window.width = fish.layout.unwarp_window.width;
      layout.unwarp_window.height = fish.layout.unwarp_window.height;
      layout.unwarp_window.x = fish.layout.unwarp_window.x;
      layout.unwarp_window.y = fish.layout.unwarp_window.y;
      layout.warp.width = fish.layout.warp.width;
      layout.warp.height = fish.layout.warp.height;
      config_changed = 1;
      return true;
    }
    bool get(FisheyeParameters *fish) {
      if (fish) {
        fish->mount = mount;
        fish->projection = projection;
        fish->max_fov = max_fov;
        fish->max_circle = max_circle;
        fish->layout.unwarp.width = layout.unwarp.width;
        fish->layout.unwarp.height = layout.unwarp.height;
        fish->layout.unwarp_window.width = layout.unwarp_window.width;
        fish->layout.unwarp_window.height = layout.unwarp_window.height;
        fish->layout.unwarp_window.x = layout.unwarp_window.x;
        fish->layout.unwarp_window.y = layout.unwarp_window.y;
        fish->layout.warp.width = layout.warp.width;
        fish->layout.warp.height = layout.warp.height;
      }
      return true;
    }
};

#endif

struct StreamEncodeFormat {
    iav_encode_format_ex_t           encode_format;
    iav_change_framerate_factor_ex_t encode_framerate;
#ifdef CONFIG_ARCH_S2
    uint32_t                         src_unwarp;
    Rect                             src_window;
#endif
};

struct StreamParameters {
    uint32_t                 config_changed;
    StreamEncodeFormat       encode_params;
    iav_bitrate_info_ex_t    bitrate_info;
    iav_h264_config_ex_t     h264_config;
    iav_jpeg_config_ex_t     mjpeg_config;
    iav_change_qp_limit_ex_s h264_qp;
    OverlayFormat            overlay;
};


struct EncoderParameters {
    uint32_t                          config_changed;
    uint32_t                          yuv_buffer_id;
    Resolution                        yuv_buffer_size;
    BufferAssignMethod                buffer_assign_method;
    Resolution                        max_source_buffer[MAX_ENCODE_BUFFER_NUM];
    iav_system_setup_info_ex_t        system_setup_info;
    iav_system_resource_setup_ex_t    system_resource_info;
    iav_source_buffer_type_all_ex_t   buffer_type_info;
    iav_source_buffer_format_all_ex_t buffer_format_info;
    iav_digital_zoom_ex_t             dptz_main_info;
#ifdef CONFIG_ARCH_S2
    WarpParameters                    warp_layout;
    iav_warp_control_ex_t             warp_control_info;
    iav_warp_dptz_ex_t                dptz_warp_info[MAX_ENCODE_BUFFER_NUM][MAX_WARP_AREA_NUM];
#endif
    EncoderParameters() {
      memset(&system_setup_info, 0, sizeof(system_setup_info));
      memset(&system_resource_info, 0, sizeof(system_resource_info));
      memset(&buffer_type_info, 0, sizeof(buffer_type_info));
      memset(&buffer_format_info, 0, sizeof(buffer_format_info));
      memset(&dptz_main_info, 0, sizeof(dptz_main_info));
#ifdef CONFIG_ARCH_S2
      memset(&warp_layout, 0, sizeof(warp_control_info));
      memset(&warp_control_info, 0, sizeof(warp_control_info));
      memset(dptz_warp_info, 0, sizeof(dptz_warp_info));
#endif
      config_changed = 0;
      yuv_buffer_id = 1; /* Second buffer */
      buffer_assign_method = AM_BUFFER_TO_RESOLUTION;
    }
    ~EncoderParameters() {
    }
};


struct BitrateMode {
    uint32_t macroblocks;
    uint32_t extremely_low_kbps;
    uint32_t low_kbps;
    uint32_t medium_kbps;
    uint32_t high_kbps;
    BitrateMode(uint32_t mb,
                uint32_t extremely_low,
                uint32_t low,
                uint32_t medium,
                uint32_t high) :
        macroblocks(mb),
        extremely_low_kbps(extremely_low),
        low_kbps(low),
        medium_kbps(medium),
        high_kbps(high)
    {
    }
};

struct audio_analy_param {
    char aa_mod_names[MAX_AUDIO_ANALY_MOD_NAME_LEN];
    uint32_t aa_mod_th;
    audio_analy_param(uint32_t mod_th) :
        aa_mod_th(mod_th){}
    audio_analy_param() :
        aa_mod_th(0){}
};

static BitrateMode gBitrateModeList[] = {
    BitrateMode(8160, 600,  1500,  2500,  4000),    // 1080p30
    BitrateMode(6120, 450,  1200,  2000,  4000),    // 1440x1080p30
    BitrateMode(5120, 350,  1000,  2000,  4000),    // 1280x1024p30
    BitrateMode(3600, 300,   800,  1500,  3000),    // 720p30
    BitrateMode(1920, 180,   500,  1000,  2000),    // 800x600p30
    BitrateMode(1620, 150,   500,  1000,  2000),    // 576p30
    BitrateMode(1350, 135,   500,  1000,  2000),    // 480p30
    BitrateMode(1200, 120,   400,   800,  1500),    // 640x480p30
    BitrateMode(396,   40,   120,   250,   400),    // 352x288p30
    BitrateMode(300,   35,   120,   250,   400),    // 320x240p30
};

struct YBufferFormat {
    uint8_t *y_addr;
    uint32_t y_width;
    uint32_t y_height;
};

struct VDeviceParameters {
    uint32_t config_changed;
    uint32_t vin_number;
    uint32_t vout_number;
    uint32_t stream_number;
    VinType  *vin_list;
    VoutType *vout_list;
    VDeviceParameters() {
      config_changed = 0;
      vout_number = 0;
      vout_list = NULL;
      vin_number = 0;
      vin_list = NULL;
      stream_number = 0;
    }
    ~VDeviceParameters() {
      delete[] vout_list;
      delete[] vin_list;
    }
};

struct VinParameters {
    uint32_t                 config_changed;
    uint32_t                 source;
    amba_video_mode          video_mode;
    amba_vin_src_mirror_mode mirror_mode;
    uint32_t                 framerate;
    /* VIN YUV Only */
    int32_t                  vin_eshutter_time;
    int32_t                  vin_agc_db;
};

struct VoutParameters {
    uint32_t                config_changed;
    int32_t                 is_video_csc_enabled; //Is color conversion enabled
    int32_t                 is_video_enabled;     //Is video enabled
    int32_t                 framebuffer_id;       //Framebuffer ID: 0, 1
    uint8_t                 bg_color_r;           //Red
    uint8_t                 bg_color_g;           //Green
    uint8_t                 bg_color_b;           //Blue
    uint8_t                 reserved;             //For alignment
    amba_vout_tailored_info tailored_info;        //Auto copy framebuffer
    amba_vout_rotate_info   video_rotate;         //Rotate video layer
    amba_vout_flip_info     video_flip;           //Flip video layer
    amba_video_mode         video_mode;           //Video mode/Resolution
    amba_vout_video_size    vout_video_size;      //Video size
    amba_vout_video_offset  video_offset;         //Video offset
    amba_vout_osd_rescale   osd_rescale;          //OSD rescale
    amba_vout_osd_offset    osd_offset;           //OSD offset
    LcdPanelType            lcd_type;             //LCD type used in LCD config
};

struct WifiParameters {
   uint32_t config_changed;
   char     *wifi_mode;
   char     *wifi_ssid;
   char     *wifi_key;

   WifiParameters () :
      config_changed (0),
      wifi_mode (NULL),
      wifi_ssid (NULL),
      wifi_key (NULL) {}

   ~WifiParameters ()
   {
      delete[] wifi_mode;
      delete[] wifi_ssid;
      delete[] wifi_key;
   }

   void set_wifi_mode (const char *mode)
   {
      delete[] wifi_mode;
      wifi_mode = NULL;
      if (mode) {
         wifi_mode = amstrdup (mode);
      }
   }

   void set_wifi_ssid (const char *ssid)
   {
      delete[] wifi_ssid;
      wifi_ssid = NULL;
      if (ssid) {
         wifi_ssid = amstrdup (ssid);
      }
   }

   void set_wifi_key (const char *key)
   {
      delete[] wifi_key;
      wifi_key = NULL;
      if (key) {
         wifi_key = amstrdup (key);
      }
   }
};

struct AudioDetectParameters {
   uint32_t config_changed;
   uint32_t audio_channel_number;
   uint32_t audio_sample_rate;
   uint32_t audio_chunk_bytes;
   uint32_t enable_alert_detect;
   uint32_t audio_alert_sensitivity;
   uint32_t audio_alert_direction;
   uint32_t enable_analysis_detect;
   uint32_t audio_analysis_direction;
   uint32_t audio_analysis_mod_num;
   audio_analy_param aa_param[MAX_AUDIO_ANALY_MOD_NUM];

};

enum MotionDetectParam {
  MD_ROI_LEFT = 0,
  MD_ROI_RIGHT,
  MD_ROI_TOP,
  MD_ROI_BOTTOM,
  MD_ROI_THRESHOLD,
  MD_ROI_SENSITIVITY,
  MD_ROI_VALID,
  MD_ROI_OUTPUT_MOTION,
  MD_ROI_PARAM_MAX_NUM,
};

struct MotionDetectParameters {
  uint32_t config_changed;
  char algorithm[8];
  uint32_t value[MAX_MOTION_DETECT_ROI_NUM][MD_ROI_PARAM_MAX_NUM];
};

static CamVideoMode gVideoModeList[] =
{CamVideoMode(     "auto",AMBA_VIDEO_MODE_AUTO       ,   0,    0),
 CamVideoMode(   "native",AMBA_VIDEO_MODE_HDMI_NATIVE,   0,    0),
 CamVideoMode(     "480i",AMBA_VIDEO_MODE_480I       , 720,  480),
 CamVideoMode(     "576i",AMBA_VIDEO_MODE_576I       , 720,  576),
 CamVideoMode(     "480p",AMBA_VIDEO_MODE_D1_NTSC    , 720,  480),
 CamVideoMode(     "576p",AMBA_VIDEO_MODE_D1_PAL     , 720,  480),
 CamVideoMode(     "720p",AMBA_VIDEO_MODE_720P       ,1280,  720),
 CamVideoMode(   "720p50",AMBA_VIDEO_MODE_720P_PAL   ,1280,  720),
 CamVideoMode(   "720p30",AMBA_VIDEO_MODE_720P30     ,1280,  720),
 CamVideoMode(   "720p25",AMBA_VIDEO_MODE_720P25     ,1280,  720),
 CamVideoMode(   "720p24",AMBA_VIDEO_MODE_720P24     ,1280,  720),
 CamVideoMode(    "1080i",AMBA_VIDEO_MODE_1080I      ,1920, 1080),
 CamVideoMode(  "1080i50",AMBA_VIDEO_MODE_1080I_PAL  ,1920, 1080),
 CamVideoMode(    "1080p",AMBA_VIDEO_MODE_1080P      ,1920, 1080),
 CamVideoMode(  "1080p50",AMBA_VIDEO_MODE_1080P_PAL  ,1920, 1080),
 CamVideoMode(  "1080p30",AMBA_VIDEO_MODE_1080P30    ,1920, 1080),
 CamVideoMode(  "1080p25",AMBA_VIDEO_MODE_1080P25    ,1920, 1080),
 CamVideoMode(  "1080p24",AMBA_VIDEO_MODE_1080P24    ,1920, 1080),
 CamVideoMode(  "2160p30",AMBA_VIDEO_MODE_2160P30    ,3840, 2160),
 CamVideoMode(  "2160p25",AMBA_VIDEO_MODE_2160P25    ,3840, 2160),
 CamVideoMode(  "2160p24",AMBA_VIDEO_MODE_2160P24    ,3840, 2160),
 CamVideoMode("2160p24se",AMBA_VIDEO_MODE_2160P24_SE ,4096, 2160),
 CamVideoMode(       "d1",AMBA_VIDEO_MODE_D1_NTSC    , 720,  480),
 CamVideoMode(   "d1_pal",AMBA_VIDEO_MODE_D1_PAL     , 720,  480),
 CamVideoMode(     "qvga",AMBA_VIDEO_MODE_320_240    , 320,  240),
 CamVideoMode(     "hvga",AMBA_VIDEO_MODE_HVGA       , 320,  480),
 CamVideoMode(      "vga",AMBA_VIDEO_MODE_VGA        , 640,  480),
 CamVideoMode(     "wvga",AMBA_VIDEO_MODE_WVGA       , 800,  480),
 CamVideoMode(    "wsvga",AMBA_VIDEO_MODE_WSVGA      ,1024,  600),
 CamVideoMode(     "qxga",AMBA_VIDEO_MODE_QXGA       ,2048, 1536),
 CamVideoMode(      "xga",AMBA_VIDEO_MODE_XGA        , 800,  600),
 CamVideoMode(     "wxga",AMBA_VIDEO_MODE_WXGA       ,1280,  800),
 CamVideoMode(     "uxga",AMBA_VIDEO_MODE_UXGA       ,1600, 1200),
 CamVideoMode(    "qsxga",AMBA_VIDEO_MODE_QSXGA      ,2592, 1944),
 CamVideoMode(       "3m",AMBA_VIDEO_MODE_QXGA       ,2048, 1536),
 CamVideoMode(   "3m_169",AMBA_VIDEO_MODE_2304x1296  ,2304, 1296),
 /*
 CamVideoMode(       "4m",AMBA_VIDEO_MODE_4M_4_3     ,    ,     ),
 CamVideoMode(   "4m_169",AMBA_VIDEO_MODE_4M_16_9    ,    ,     ),
 */
 CamVideoMode(       "5m",AMBA_VIDEO_MODE_QSXGA      ,2592, 1944),
 CamVideoMode(   "5m_169",AMBA_VIDEO_MODE_5M_16_9    ,2976, 1674),
 CamVideoMode(  "240x800",AMBA_VIDEO_MODE_240_400    , 240,  400),
 CamVideoMode(  "320x240",AMBA_VIDEO_MODE_320_240    , 320,  240),
 CamVideoMode(  "320x288",AMBA_VIDEO_MODE_320_288    , 320,  288),
 CamVideoMode(  "360x240",AMBA_VIDEO_MODE_360_240    , 360,  240),
 CamVideoMode(  "360x288",AMBA_VIDEO_MODE_360_288    , 360,  288),
 CamVideoMode(  "480x640",AMBA_VIDEO_MODE_480_640    , 480,  640),
 CamVideoMode(  "480x800",AMBA_VIDEO_MODE_480_800    , 480,  800),
 CamVideoMode(  "640x400",AMBA_VIDEO_MODE_640_400    , 640,  400),
 CamVideoMode(  "960x240",AMBA_VIDEO_MODE_960_240    , 960,  240),
 CamVideoMode(  "960x540",AMBA_VIDEO_MODE_960_540    , 960,  540),
 CamVideoMode( "1024x768",AMBA_VIDEO_MODE_XGA        ,1024,  768),
 CamVideoMode( "1280x720",AMBA_VIDEO_MODE_720P       ,1280,  720),
 CamVideoMode( "1280x960",AMBA_VIDEO_MODE_1280_960   ,1280,  960),
 CamVideoMode("1280x1024",AMBA_VIDEO_MODE_SXGA       ,1280, 1024),
 CamVideoMode("1600x1200",AMBA_VIDEO_MODE_UXGA       ,1600, 1200),
 CamVideoMode("1920x1440",AMBA_VIDEO_MODE_1920_1440  ,1920, 1440),
 CamVideoMode("1920x1080",AMBA_VIDEO_MODE_1080P      ,1920, 1080),
 CamVideoMode("2048x1152",AMBA_VIDEO_MODE_2048_1152  ,2048, 1152),
 CamVideoMode("2048x1536",AMBA_VIDEO_MODE_QXGA       ,2048, 1536),
 CamVideoMode("2560x1440",AMBA_VIDEO_MODE_2560x1440  ,2560, 1440),
 CamVideoMode("2592x1944",AMBA_VIDEO_MODE_QSXGA      ,2592, 1944),
 CamVideoMode("2208x1242",AMBA_VIDEO_MODE_2208x1242  ,2208, 1242),
 CamVideoMode("2304x1296",AMBA_VIDEO_MODE_2304x1296  ,2304, 1296),
 CamVideoMode("2304x1536",AMBA_VIDEO_MODE_2304x1536  ,2304, 1536),
 CamVideoMode("4096x2160",AMBA_VIDEO_MODE_4096x2160  ,4096, 2160),
 CamVideoMode("4000x3000",AMBA_VIDEO_MODE_4000x3000  ,4000, 3000),
 CamVideoMode("4016x3016",AMBA_VIDEO_MODE_4016x3016  ,4016, 3016)};

static CameraVinFPS gFpsList[] =
{CameraVinFPS("auto"  , (uint32_t)AMBA_VIDEO_FPS_AUTO)  ,
 CameraVinFPS("0"     , (uint32_t)AMBA_VIDEO_FPS_AUTO)  ,
 CameraVinFPS("1"     , (uint32_t)AMBA_VIDEO_FPS_1)     ,
 CameraVinFPS("2"     , (uint32_t)AMBA_VIDEO_FPS_2)     ,
 CameraVinFPS("3"     , (uint32_t)AMBA_VIDEO_FPS_3)     ,
 CameraVinFPS("4"     , (uint32_t)AMBA_VIDEO_FPS_4)     ,
 CameraVinFPS("5"     , (uint32_t)AMBA_VIDEO_FPS_5)     ,
 CameraVinFPS("6"     , (uint32_t)AMBA_VIDEO_FPS_6)     ,
 CameraVinFPS("10"    , (uint32_t)AMBA_VIDEO_FPS_10)    ,
 CameraVinFPS("12"    , (uint32_t)AMBA_VIDEO_FPS_12)    ,
 CameraVinFPS("13"    , (uint32_t)AMBA_VIDEO_FPS_13)    ,
 CameraVinFPS("15"    , (uint32_t)AMBA_VIDEO_FPS_15)    ,
 CameraVinFPS("20"    , (uint32_t)AMBA_VIDEO_FPS_20)    ,
 CameraVinFPS("24"    , (uint32_t)AMBA_VIDEO_FPS_24)    ,
 CameraVinFPS("25"    , (uint32_t)AMBA_VIDEO_FPS_25)    ,
 CameraVinFPS("30"    , (uint32_t)AMBA_VIDEO_FPS_30)    ,
 CameraVinFPS("50"    , (uint32_t)AMBA_VIDEO_FPS_50)    ,
 CameraVinFPS("60"    , (uint32_t)AMBA_VIDEO_FPS_60)    ,
 CameraVinFPS("120"   , (uint32_t)AMBA_VIDEO_FPS_120)   ,
 CameraVinFPS("29.97" , (uint32_t)AMBA_VIDEO_FPS_29_97) ,
 CameraVinFPS("59.94" , (uint32_t)AMBA_VIDEO_FPS_59_94) ,
 CameraVinFPS("23.976", (uint32_t)AMBA_VIDEO_FPS_23_976),
 CameraVinFPS("12.5"  , (uint32_t)AMBA_VIDEO_FPS_12_5)  ,
 CameraVinFPS("6.25"  , (uint32_t)AMBA_VIDEO_FPS_6_25)  ,
 CameraVinFPS("3.125" , (uint32_t)AMBA_VIDEO_FPS_3_125) ,
 CameraVinFPS("7.5"   , (uint32_t)AMBA_VIDEO_FPS_7_5)   ,
 CameraVinFPS("3.75"  , (uint32_t)AMBA_VIDEO_FPS_3_75)};

#ifdef CONFIG_ARCH_S2
typedef struct {
  uint32_t width;
  uint32_t height;
} RECT;

typedef struct {
  int id;
  int left;
  int top;
  int width;
  int height;
  uint32_t action;
} PRIVACY_MASK;

#else
/* types used by privacy mask start */
typedef struct {
  int unit;   /* 0- percent ,1-pixel */
  int left;
  int top;
  int width;
  int height;
  /* 0:Black, 1:Red, 2:Blue, 3:Green, 4:Yellow, 5:Magenta, 6:Cyan, 7:White */
  uint32_t color;
  uint32_t action;
} PRIVACY_MASK;
#endif

typedef struct {
  uint16_t start_x;
  uint16_t start_y;
  uint16_t width;
  uint16_t height;
  /* 0:Black, 1:Red, 2:Blue, 3:Green, 4:Yellow, 5:Magenta, 6:Cyan, 7:White */
  uint32_t color;
  uint32_t action;
} PRIVACY_MASK_RECT;

typedef struct {
  uint8_t *start;
  uint8_t *end;
  uint32_t size;
} MMapInfo;

typedef struct {
  uint8_t v;
  uint8_t u;
  uint8_t y;
  uint8_t a;
} CLUT; //color look up table

typedef enum {
  PM_ADD_INC = 0, /* add include region */
  PM_ADD_EXC,     /* add exclude region */
  PM_REPLACE,     /* replace with new region */
  PM_REMOVE_ALL,  /* remove all regions */
  PM_ACTIONS_NUM,
} PRIVACY_MASK_ACTION;

typedef enum {
  PM_UNIT_PERCENT = 0,
  PM_UNIT_PIXEL,
} PRIVACY_MASK_UNIT;

typedef enum {
  PM_COLOR_BALCK = 0,
  PM_COLOR_RED,
  PM_COLOR_BLUE,
  PM_COLOR_GREEN,
  PM_COLOR_YELLOW,
  PM_COLOR_MAGENTA,
  PM_COLOR_CYAN,
  PM_COLOR_WHITE,
} PRIVACY_MASK_COLOR;

typedef enum {
    F_BLACK,
    F_RED,
    F_BLUE,
    F_GREEN,
    F_YELLOW,
    F_MAGENTA,
    F_CYAN,
    F_WHITE,
}OSD_FONT_COLOR;

typedef struct node {
  int group_index;
  PRIVACY_MASK pm_data;
  struct node* pNext;
} PrivacyMaskNode;

typedef struct {
  uint32_t source_buffer;
  uint32_t zoom_factor_x;
  uint32_t zoom_factor_y;
  int offset_x;
  int offset_y;
} DPTZOrgParam;

#ifdef CONFIG_ARCH_S2
typedef struct {
    uint32_t id;
    Fraction      zoom;
    PanTiltAngle  pantilt_angle;
}TransParam;
#endif

typedef struct {
  uint32_t source_buffer;
  uint32_t zoom_factor;
  int offset_x;
  int offset_y;
} DPTZParam;

typedef enum {
  BUFFER_1 = 0,
  BUFFER_2,
  BUFFER_3,
  BUFFER_4,
  BUFFER_TOTAL_NUM,
} BUFFER_ID;

typedef struct {
  uint16_t width;
  uint16_t height;
} BufferFormat;

typedef struct {
  DPTZParam  dptz;
  PRIVACY_MASK  pm;
} DPTZPrivacyMaskParam;

struct LBRStreamParams {
    uint32_t enable_lbr;
    uint32_t motion_control;
    uint32_t low_light_control;
    uint32_t frame_drop;
    uint32_t auto_target;       //if auto is 1, ignore other fields
    uint32_t bitrate_ceiling;   //useful when auto is 0
};

struct LBRParameters {
    uint32_t config_changed;
    uint32_t mse_motion_low_threshold;
    uint32_t mse_motion_high_threshold;
    uint32_t mog2_motion_low_threshold;
    uint32_t mog2_motion_high_threshold;
    uint32_t noise_low_threshold;
    uint32_t noise_high_threshold;
    LBRStreamParams stream_params[AM_STREAM_ID_MAX];
};

/* types used by privacy mask end */

#endif /* AMSTRUCTURE_H_ */
