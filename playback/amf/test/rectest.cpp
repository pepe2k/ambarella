
/**
 * pbtest.cpp
 *
 * History:
 *    2011/7/8 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_util.h"
#include "am_record_if.h"

#if PLATFORM_ANDROID
extern "C"{
   #include "ivLDWSEngine.h"
}
#include "amba_ldws_engine.h"
#endif

#if PLATFORM_LINUX
#include <sys/time.h>
#endif

#if PLATFORM_ANDROID
#include <sys/socket.h>
#endif
#include <arpa/inet.h>

//use imgproc in android
#ifdef CONFIG_IMGPROC_IN_AMF_UT
#define __use_imgproc__
#endif

#ifdef __use_imgproc__
#include "am_param.h"
#include "vout_control_if.h"
#include "rec_pretreat_if.h"
#endif

#include "ws_discovery.h"
#if PLATFORM_ANDROID
#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
#endif
#include "am_pbif.h"

struct PbHandle{
    IPBControl *ctrl;
#if PLATFORM_ANDROID
    AMPlayer::AudioSink *audiosink;
#endif
};

#define if_err_return(err) do { \
    AM_ASSERT(err == ME_OK); \
    if (err != ME_OK) { \
        AM_ERROR("[rectest] error: file %s, line %d, ret %d.\n", __FILE__, __LINE__, err); \
        return -2; \
    } \
} while (0)

enum {
    //System
    TEST_FEEDING_PRIDATA = 0,
    ENABLE_RTSP_STREAMING,
    TEST_START_AAA,
    SELECT_CAMERA,
    SET_DSPMODE,
    SET_VOUT_MASK,
    START_LISTEN_MSG,
    MSG_NOTIFICAYION_MODE,
    MSG_SERVER_PORT,
    MSG_LOCAL_PORT,
    MSG_SERVER_IP_ADDR,

    ENALBE_TEST_OSD,
    TEST_OSD_MODE,
    TEST_OSD_INPUT_FMT,

    ENABLE_LDWS_ENGINE,
    REC_STREAM_MASK,
    RTSP_STREAMING_ONLY,
    RTSP_STREAMING_BITRATE,
    ENABLE_WSD,
    DEBUG_NOT_START_ENCODING,

    DISABLE_VIDEO,
    DISABLE_AUDIO,
};

//#define __app_set_detailed_parameters__

//-------------------------------------------------------------------
//                      Global Varibles
//-------------------------------------------------------------------
static IRecordControl2 *G_pRecordControl = NULL;
static IStreamingControl* G_pStreamingControl = NULL;

CEvent *G_pYUVcapEvent;
CThread *G_pYUVcapThread;
static AM_INT YUVCapture_Flag = 0;// 1: capture continuously; 2: capture 1 data; 0: do nothing
AM_UINT yuv_num = 0;

#if PLATFORM_ANDROID
//LDWS
static AmbaLdwsEngine* G_pLDWSEngine = NULL;
static int enable_ldws = 0;
#endif

#ifdef __use_imgproc__
static IRecPreTreat* G_pPreTreat = NULL;
#endif
static int auto_start_aaa = 0;
static int not_encoding = 0;
static int camera_index = 0;
static int specify_dspmode_voutmask = 0;
static unsigned int dsp_mode = DSPMode_CameraRecording;
static unsigned int vout_mask = (1<<eVoutLCD) | (1<<eVoutHDMI);

#define _max_filename_length_  320
static char file_name[2][_max_filename_length_] = {"testrecord.mp4", "testrecord_smallsize.mp4"};
unsigned int sample_rate = 48000;
unsigned int channels = 2;

static int need_set_path = 0;
static char path_name[_max_filename_length_] = {0};

static IParameters::ContainerType output_format[DMaxMuxerNumber] = {IParameters::MuxerContainer_AUTO, IParameters::MuxerContainer_AUTO};
IParameters::StreamFormat video_format[DMaxMuxerNumber] = {IParameters::StreamFormat_H264, IParameters::StreamFormat_H264};
IParameters::StreamFormat audio_format[DMaxMuxerNumber] = {IParameters::StreamFormat_Invalid, IParameters::StreamFormat_Invalid};//not specify, set by rec.config by default

static AM_UINT total_muxer_number = 2;

static AM_UINT saving_stratege = IParameters::MuxerSavingFileStrategy_ToTalFile;
static AM_UINT saving_condition = IParameters::MuxerSavingCondition_InputPTS;
static AM_UINT max_file_num = DefaultMaxFileNumber;
static AM_UINT total_file_num = DefaultTotalFileNumber;
static AM_UINT auto_time = DefaultAutoSavingParam;
static AM_UINT auto_frame_count = DefaultAutoSavingParam;

static int specify_dumppath = 0;
static int specify_configpath = 0;

static int record_running = 1;

static unsigned int enable_pridata_test = 0;
static unsigned int pridata_index = 0;
static unsigned char private_data_gps_base[400] = " [rectest] test GPS Info: position 12.6543, 30.4567, time 2011-11-14, 12:01:35. index %d.\n";
static unsigned char private_data_sensor_base[400] = " [rectest] test Sensor Info: speed 10.123 m/s, accel 2.1 m/ss, direction 0,0,90. index %d.\n";
static unsigned char private_data_buffer[480] = {0};


static int streaming_only = 0;
static int streaming_bitrate = 1000000; //default 1Mbps
static int enable_wsd = 0;
static unsigned int rtsp_enabled = 0;
static unsigned int rtsp_output_index_mask = 0;
static unsigned int rtsp_server_index = 0;
static unsigned int rec_stream_mask = 0b11;//bit 0: audio stream encoding flag, bit 1: video stream encoding flag

//skychen,2012_11_21
//msg notification
static AM_INT local_socket = -1;
static AM_UINT msg_notification_mode = 0;
static AM_U16 msg_client_local_port = 8484;
static AM_U16 msg_server_port = 4848;
static char msg_server_ip_addr[64] = "192.168.0.2";
static struct sockaddr_in msg_dest_addr;

//test osd related
typedef struct _testOSDArea {
    int osd_id;
    int width, height;
    int offset_x, offset_y;

    int vout_width, vout_height;

    unsigned char* p_data;
    unsigned char* p_data_base;
    unsigned char r, g, b, inited;
    unsigned int* p_clut;

    unsigned char color_index, color_index_step;
    unsigned char input_format, iav_success;
    unsigned char disabled, tobe_disabled;
    unsigned char reserved0, reserved1;

    //loop
    unsigned char r_ran, g_ran, b_ran, alpha_ran;
    int move_x_ran, move_y_ran, size_x_ran, size_y_ran;

    //life cycle
    unsigned int display_tick, display_duration, hiden_duration;
} testOSDArea;

enum {
    ETestOSDMode_CLUT = 0,
    ETestOSDMode_RGBA, //old one
};

enum {
    ETestOSDInput_color_index = 0,
    ETestOSDInput_RGBA,
};

static int test_osd_running = 0;
static unsigned char  test_osd = 0;
static unsigned char  test_osd_number = 3;
static unsigned char test_osd_mode = ETestOSDMode_CLUT;
static unsigned char test_osd_input_format = ETestOSDInput_color_index;

static testOSDArea test_osd_area[4];

static unsigned char r_ran_seed = 0x12, g_ran_seed = 0x45, b_ran_seed = 0x89;
static unsigned char r_ran_step = 0x34, g_ran_step = 0x67, b_ran_step = 0xcd;

static int move_x_ran_seed = 100, move_y_ran_seed = 100, size_x_ran_seed = 400, size_y_ran_seed = 300;
static int move_x_ran_step = 100, move_y_ran_step = 100, size_x_ran_step = 10, size_y_ran_step = 10;

static void update_osd_color(testOSDArea* area)
{
    if (!area) {
        AM_ERROR("NULL input here, in update_color\n");
        return;
    }

    if (ETestOSDInput_color_index == area->input_format) {
        area->color_index += area->color_index_step;
    } else if (ETestOSDInput_RGBA == area->input_format) {
        area->r += area->r_ran;
        area->g += area->g_ran;
        area->b += area->b_ran;
    } else {
        AM_ERROR("BAD input format %d, inited %d, in area %p\n", area->input_format, area->inited, area);
    }
}

static void generate_osd_content(testOSDArea* area)
{
    unsigned int color = 0;
    unsigned int* p;
    int w = 0, h = 0;

    if (!area) {
        AM_ERROR("NULL input here, in generate_osd_content\n");
        return;
    }

    if (!area->p_data) {
        AM_ERROR("NULL data pointer here, in generate_osd_content\n");
        return;
    }

    if (ETestOSDInput_color_index == area->input_format) {
        memset(area->p_data, area->color_index, area->width * area->height);
    } else if (ETestOSDInput_RGBA == area->input_format) {
        color = 0xff000000 | ((area->r) << 16) | ((area->g) << 8) | (area->b);
        p = (unsigned int*)area->p_data;
        for (h = 0; h < area->height; h ++) {
            for (w = 0; w < area->width; w ++) {
                *p++ = color;
            }
        }
    } else {
        AM_ERROR("BAD input format %d, inited %d, in area %p\n", area->input_format, area->inited, area);
    }
}

static void update_osd_size_position(testOSDArea* area)
{
    if (!area) {
        AM_ERROR("NULL input here, in update_osd_size_position\n");
        return;
    }

    AM_ERROR("to do\n");
}

static void init_osd_parameters(testOSDArea* area)
{
    if (!area) {
        AM_ERROR("NULL input here, in init_osd_parameters\n");
        return;
    }

    area->display_duration = 20;
    area->hiden_duration = 10;

    area->color_index_step = 17;
    area->color_index = 29;
}

static void release_osd_area(testOSDArea* area)
{
    if (!area) {
        AM_ERROR("NULL input here, in release_osd_area\n");
        return;
    }

    if (area->p_data_base) {
        free(area->p_data_base);
        area->p_data_base = NULL;
    }
    area->p_data = NULL;

    if (area->p_clut) {
        free(area->p_clut);
        area->p_clut = NULL;
    }

    area->inited = 0;
}

static int alloc_osd_area(testOSDArea* area, unsigned char input_format)
{
    if (!area) {
        AM_ERROR("NULL input here, in alloc_osd_area\n");
        return (-1);
    }

    //safe check
    if (area->inited) {
        release_osd_area(area);
    }

    // 4bytes ycbcr
    if (ETestOSDMode_RGBA == input_format) {
        area->p_data_base = (unsigned char*)malloc(4 * area->width * area->height + 4);
        if (!area->p_data_base) {
            AM_ERROR("not enough memory, request width %d, height %d, size %d\n",  area->width, area->height,  4* area->width * area->height);
            return (-2);
        }
        area->p_data = (unsigned char*)(((unsigned int)(area->p_data_base + 3))&(~3));
    } else if (ETestOSDMode_CLUT == input_format) {
        area->p_data_base = (unsigned char*)malloc(area->width * area->height + 4);
        if (!area->p_data_base) {
            AM_ERROR("not enough memory, request width %d, height %d, size %d\n",  area->width, area->height,  area->width * area->height);
            return (-2);
        }
        AM_WARNING("[debug info]: alloc_osd_area for new area(%p, id %d), width %d, height %d, size %d, base %p, data %p\n", area, area->osd_id, area->width, area->height, area->width * area->height, area->p_data_base, area->p_data);
        area->p_data = (unsigned char*)(((unsigned int)(area->p_data_base + 3))&(~3));

        area->p_clut = (unsigned int*)malloc(256 * 4);
        if (!area->p_clut) {
            AM_ERROR("not enough memory, request clut table\n");
            return (-3);
        }
        GeneratePresetYCbCr422_YCbCrAlphaCLUT(area->p_clut, 1, 0);
    } else {
        AM_ERROR("not supported osd_mode %d\n", input_format);
        return (-4);
    }

    area->inited = 1;
    area->input_format = input_format;

    return 0;
}

#if PLATFORM_ANDROID
static AM_ERR LDWS_Event_Handler(eLDWSOutputEvent event)
{
    if(event == eOENone){
        //AM_WARNING("eOENone!!\n");
    }else if(event == eOELaneLost){
        AM_WARNING("eOELaneLost!!\n");
    }else if(event == eOELaneDetected){
        AM_WARNING("eOELaneDetected!!\n");
    }else if(event == eOELaneDepature){
        AM_WARNING("eOELaneDepature!!\n");
    }
    return ME_OK;
}
#endif

//-------------------------------------------------------------------
//                      Helper Functions
//-------------------------------------------------------------------
IParameters::ContainerType get_output_format(const char *format)
{
    if (strcmp(format, "mp4") == 0)
        return IParameters::MuxerContainer_MP4;

    if (strcmp(format, "ts") == 0)
        return IParameters::MuxerContainer_TS;

    if (strcmp(format, "3gp") == 0)
        return IParameters::MuxerContainer_3GP;

    if (strcmp(format, "amr") == 0)
        return IParameters::MuxerContainer_AMR;

    if (strcmp(format, "avi") == 0)
        return IParameters::MuxerContainer_AVI;

    if (strcmp(format, "mov") == 0)
        return IParameters::MuxerContainer_MOV;

    if (strcmp(format, "mkv") == 0)
        return IParameters::MuxerContainer_MKV;

    printf("invalid output formate: %s\n", format);
    return IParameters::MuxerContainer_AUTO;
}

IParameters::StreamFormat get_audio_format(const char *format)
{
    if (strcmp(format, "aac") == 0)
        return IParameters::StreamFormat_AAC;

    if (strcmp(format, "mp2") == 0)
        return IParameters::StreamFormat_MP2;

    if (strcmp(format, "ac3") == 0)
        return IParameters::StreamFormat_AC3;

    if (strcmp(format, "adpcm") == 0)
        return IParameters::StreamFormat_ADPCM;

    printf("invalid audio format: %s\n", format);
    return IParameters::StreamFormat_Invalid;
}

struct encode_stream_conf_s{
    AM_UINT enc_width;
    AM_UINT enc_height;
    int video_bitrate;
    int audio_bitrate;
    int framerate;
    int M;
    int N;
    int IDR_INTERVAL;
};

static encode_stream_conf_s encode_stream_conf[2] = {0};
static encode_stream_conf_s* pencode_stream_conf = NULL;
static int enable_video = -1;
static int enable_audio = -1;

struct encode_resolution_s {
    const char *name;
    AM_UINT width;
    AM_UINT height;
}
__encode_res[] = {
    {"1080p", 1920, 1080},
    {"720p", 1280, 720},
    {"480p", 720, 480},
    {"576p", 720, 576},
    {"4sif", 704, 480},
    {"4cif", 704, 576},
    {"xga", 1024, 768},
    {"vga", 640, 480},
    {"cif", 352, 288},
    {"sif", 352, 240},
    {"wqvga", 432, 240},
    {"qvga", 320, 240},
    {"qcif", 176, 144},
    {"qsif", 176, 120},
    {"qqvga", 160, 120},
    {"svga", 800, 600},
    {"sxga", 1280, 1024},

    {"", 0, 0},

    //{"2048x1536", 2048, 1536},
    //{"qxga", 2048, 1536},
    //{"2048x1152", 2048, 1152},
    {"1920x1080", 1920, 1080},
    {"1440x1080", 1440, 1080},
    {"1366x768", 1366, 768},
    {"1280x1024", 1280, 1024},
    {"1280x960", 1280, 960},
    {"1280x720", 1280, 720},
    {"1024x768", 1024, 768},
    {"800x480", 800, 480},
    {"720x480", 720, 480},
    {"720x576", 720, 576},

    {"", 0, 0},

    {"704x480", 704, 480},
    {"704x576", 704, 576},
    {"640x480", 640, 480},
    {"352x288", 352, 288},
    {"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
    {"352x240", 352, 240},
    {"320x240", 320, 240},
    {"176x144", 176, 144},
    {"176x120", 176, 120},
    {"160x120", 160, 120},

    //for preview size only to keep aspect ratio in preview image for different VIN aspect ratio
    {"16_9_vin_ntsc_preview", 720, 360},
    {"16_9_vin_pal_preview", 720, 432},
    {"4_3_vin_ntsc_preview", 720, 480},
    {"4_3_vin_pal_preview", 720, 576},
    {"5_4_vin_ntsc_preview", 672, 480},
    {"5_4_vin_pal_preview", 672, 576 },
    {"ntsc_vin_ntsc_preview", 720, 480},
    {"pal_vin_pal_preview", 720, 576},
};

int get_encode_resolution(const char *name, AM_UINT *width, AM_UINT *height)
{
     int i;

     for (i = 0; i < ARRAY_SIZE(__encode_res); i++)
         if (strcmp(__encode_res[i].name, name) == 0) {
             *width = __encode_res[i].width;
             *height = __encode_res[i].height;
             AM_INFO("get_encode_resolution: %u, %u.\n", *width, *height);
             return 0;
         }

     AM_ERROR("resolution '%s' not found.\n", name);
     return -1;
}

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
    {"filename", HAS_ARG, 0, 'f'},
    {"pkg-fmt", HAS_ARG, 0, 'p'},
    {"vid-fmt", HAS_ARG, 0, 'h'},
    {"novideo", NO_ARG, 0, DISABLE_VIDEO},
    {"noaudio", NO_ARG, 0, DISABLE_AUDIO},
    {"main-stream", HAS_ARG, 0, 'v'},
    {"second-stream", HAS_ARG, 0, 'V'},
    {"video-bitrate", HAS_ARG, 0, 'b'},
    {"aud-fmt", HAS_ARG, 0, 'a'},
    {"sample-rate", HAS_ARG, 0, 'r'},
    {"channels", HAS_ARG, 0, 'c'},
    {"number", HAS_ARG, 0, 'n'},
    {"config", HAS_ARG, 0, 'u'},
    {"storage", HAS_ARG, 0, 's'},
    {"time", HAS_ARG, 0, 't'},
    {"frame-cnt", HAS_ARG, 0, 'x'},
    {"maxfilenumber", HAS_ARG, 0, 'm'},
    {"totalfilenumber", HAS_ARG, 0, 'o'},
    {"streaming-rtsp", HAS_ARG, 0, ENABLE_RTSP_STREAMING},
    {"feedpridata", NO_ARG, 0, TEST_FEEDING_PRIDATA},
    {"start3a", NO_ARG, 0, TEST_START_AAA},
    {"camera", HAS_ARG, 0, SELECT_CAMERA},
    {"dspmode", HAS_ARG, 0, SET_DSPMODE},
    {"voutmask", HAS_ARG, 0, SET_VOUT_MASK},
    {"dumpyuv", NO_ARG, 0, 'y'},
    {"msg", HAS_ARG, 0, MSG_NOTIFICAYION_MODE},
    {"serverport", HAS_ARG, 0, MSG_SERVER_PORT},
    {"localport", HAS_ARG, 0, MSG_LOCAL_PORT},
    {"serverip", HAS_ARG, 0, MSG_SERVER_IP_ADDR},
    {"enable-osd", NO_ARG, 0, ENALBE_TEST_OSD},
    {"osd-mode", HAS_ARG, 0, TEST_OSD_MODE},
    {"osd-input", HAS_ARG, 0, TEST_OSD_INPUT_FMT},
    {"enableldws", NO_ARG, 0, ENABLE_LDWS_ENGINE},
    {"recmask", HAS_ARG, 0, REC_STREAM_MASK},

    //debug use
    {"noencoding", NO_ARG, 0, DEBUG_NOT_START_ENCODING},
    //debug, rtsp server
    {"streaming-only", NO_ARG, 0, RTSP_STREAMING_ONLY},
    {"streaming-bitrate", HAS_ARG, 0, RTSP_STREAMING_BITRATE},
    {"enable-wsd", NO_ARG, 0, ENABLE_WSD},
//    {"container", HAS_ARG, 0, TEST_FEEDING_PRIDATA},
    {0, 0, 0, 0}
};

static const char *short_options = "hf:p:v:V:b:a:r:c:n:u:s:t:x:m:o:y";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
    {"filename",	                        ""	"specify filename to save"},
    {"mp4|avi|mov|mkv",	""	"specify output packaging format"},
    {"h264",	                        "\t"	"specify video compression format"},
    {"",	                        "\t"	"disable video"},
    {"",	                        "\t"	"disable audio"},
    {"resolution",	                        "\t"	"specify the resolution of the main stream"},
    {"resolution",	                        "\t"	"specify the resolution of the second stream"},
    {"bps",	                        "\t"	"specify the bitrate, specify stream first before this option"},
    {"aac|ac3|adpcm|mp2",	        ""	"specify audio compression format"},
    {"samplerate",	                ""	"specify audio sample rate"},
    {"channels",	                ""	"1 - mono, 2 - stero"},
    {"number",                 ""          "muxer number"},
    {"config",                 ""          "config path"},
    {"storage",	                ""	"specify storage strategy: 0 - auto-separate, 1 - manual-separate, 2 - no-separate(default)"},
    {"time",                     "" "specify auto-cut file's duration"},
    {"frame-cnt",              "" "specify auto-cut file's frame count"},
    {"maxfilenumber",         "" "max file numbers(exceed will delete earliest file)"},
    {"totalfilenumber",         "" "total file numbers(reached this limit will stop recording)"},
    {"streaming-rtsp",      "" "start rstp server, output index enabled"},
    {"feedpridata",	                ""	"test feeding private data"},
    {"start3a",                      ""  "init vin, start aaa before recording"},
    {"camera",                       ""  "select camera, 0 or 1"},
    {"dspmode",                      ""  "request dsp mode, 2: camera recording, 4: udec, 5: duplex low delay"},
    {"voutmask",                      ""  "request vout mask(1<<0 means LCD, 1<<1 means HDMI)"},
    {"dumpyuv",                       ""  "dump yuv data. On the fly: 'y' to start/stop capturing; 'Y' to capture one picture"},
    {"msg",                              ""  "set msg notification mode, 0: disable msg notification, 1:work ad msg client, 2: work as msg server"},
    {"serverport",                      ""  "msg notification: server port"},
    {"localport",                        ""  "msg notification: client local port"},
    {"serverip",                         ""  "msg notification: server ip addr"},
    {"enable-osd",                    ""  "enable-osd"},
    {"osd-mode",                      ""  "osd-mode"},
    {"osd-input",                       "" "osd-input"},
    {"enableldws",                     ""  "enable iv-ldws engine"},
    {"recmask",                         ""  "config recode stream: only audio, only video, both audio and video"},
    {"noencoding",                      ""  "not start encoding"},
    {"streaming-only",      "" "disable save video/audio data to files"},
    {"streaming-bitrate",      "" " set streaming-bitrate, bps, default 1Mbps"},
    {"enable-wsd",      "" " enable device auto discovery when rtsp streaming enabled"},
};

void usage(void)
{
    unsigned int i;

    printf("\nrectest usage:\n");
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

    printf("\nSupported resolutions:\n");
    for(int i=0;i<sizeof(__encode_res)/sizeof(encode_resolution_s);i++){
        if(__encode_res[i].name[0]=='\0') continue;
        printf("\t%s:  %u x %u,\n", ((__encode_res[i])).name, ((__encode_res[i])).width, ((__encode_res[i])).height);
    }
    printf("\n");
}

int init_param(int argc, char **argv)
{
    int ch;
    IParameters::ContainerType o_format;
    int option_index = 0;
    opterr = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

        switch (ch) {

        case 'f':
            {
                memset(file_name[0], 0, _max_filename_length_);
                strncpy(file_name[0], optarg, _max_filename_length_ - 1);
                char* ptt = strrchr(optarg, '.');
                int cpylen;
                if (ptt) {
                    memset(&file_name[1][0], 0, _max_filename_length_);
                    cpylen = (int)(ptt - optarg);
                    ptt ++;
                    if (IParameters::MuxerContainer_Invalid != AM_GetContainerTypeFromString(ptt)) {
                        if (cpylen > (_max_filename_length_ - 32)) {
                            cpylen = _max_filename_length_ - 32;
                        }
                        memcpy(&file_name[1][0], optarg, cpylen);
                        strncat(file_name[1], "_smallsize.", _max_filename_length_ - 1);
                        strncat(&file_name[1][0], ptt, _max_filename_length_ -1);
                         file_name[1][_max_filename_length_ - 1] = '\0';
                    } else {
                        snprintf(file_name[1], _max_filename_length_ - 1, "%s_%s", optarg, "smallsize");
                    }
                } else {
                    snprintf(file_name[1], _max_filename_length_ - 1, "%s_%s", optarg, "smallsize");
                }
            }
            break;

        case 'u':
            memset(path_name, 0, _max_filename_length_);
            strncpy(path_name, optarg, _max_filename_length_ - 1);
            need_set_path = 1;
            break;

        case 'p':
            o_format = get_output_format(optarg);
            if (o_format >= IParameters::MuxerContainer_TotolNum) {
                printf("BAD container format.\n");
                return -1;
            }
            output_format[0] = output_format[1] = o_format;
            break;

        case DISABLE_VIDEO:
            enable_video = 0;
            break;

        case DISABLE_AUDIO:
            enable_audio = 0;
            break;

        case 'v':{
                pencode_stream_conf = &encode_stream_conf[0];
                if (get_encode_resolution(optarg,
                    &(pencode_stream_conf->enc_width), &(pencode_stream_conf->enc_height)) < 0){
                    AM_ERROR("get_encode_resolution error, set main stream resolution to default.\n");
                }
                break;
            }
            break;

        case 'V':{
                pencode_stream_conf = &encode_stream_conf[1];
                if (get_encode_resolution(optarg,
                    &(pencode_stream_conf->enc_width), &(pencode_stream_conf->enc_height)) < 0){
                    AM_ERROR("get_encode_resolution error, set second stream resolution to default.\n");
                }
                break;
            }
            break;

        case 'b':{
                if(!pencode_stream_conf){
                    AM_ERROR("set video bitrate, but stream is not specified!\n");
                    usage();
                    return -1;
                }
                pencode_stream_conf->video_bitrate = atoi(optarg);
            }
            break;

        case 'n':
            total_muxer_number = atoi(optarg);
            if ((total_muxer_number >2) || (total_muxer_number==0)) {
                total_muxer_number = 2;
            }
            break;

        case 's':
            saving_stratege = atoi(optarg);
            printf("    saving_stratege %d.\n", saving_stratege);
            if (saving_stratege > ((AM_UINT)IParameters::MuxerSavingFileStrategy_ToTalFile)) {
                saving_stratege = (AM_UINT)IParameters::MuxerSavingFileStrategy_ToTalFile;
            }
            break;

        case 't':
            auto_time = atoi(optarg);
            printf("    auto_time %d.\n", auto_time);
            saving_condition = IParameters::MuxerSavingCondition_InputPTS;
            break;

        case 'x':
            auto_frame_count = atoi(optarg);
            printf("    auto_frame_count %d.\n", auto_frame_count);
            saving_condition = IParameters::MuxerSavingCondition_FrameCount;
            break;

        case 'm':
            max_file_num = atoi(optarg);
            printf("    max_file_num %d.\n", max_file_num);
            break;

        case 'o':
            total_file_num = atoi(optarg);
            printf("    total_file_num %d.\n", total_file_num);
            break;

        case TEST_FEEDING_PRIDATA:
            enable_pridata_test = 1;
            printf("    enable pridata feeding test.\n");
            break;

        case ENABLE_RTSP_STREAMING:
            rtsp_enabled = 1;
            rtsp_output_index_mask = atoi(optarg);
            printf("    enable rtsp streaming, mask 0x%x.\n", rtsp_output_index_mask);
            break;

	 case RTSP_STREAMING_ONLY:
            streaming_only = 1;
            printf("    streaming only set , saving data to files will be disabled\n");
            break;
        case RTSP_STREAMING_BITRATE:
	     streaming_bitrate = atoi(optarg);
            printf("    streaming bitrate set , new bitrate = %d bps\n",streaming_bitrate);
            break;
        case ENABLE_WSD:
            enable_wsd = 1;
            printf("    enable device auto discovery, when rtsp streaming enabled\n");
            break;
        case TEST_START_AAA:
            printf("set auto start aaa before recording.\n");
            auto_start_aaa = 1;
            break;

        case SELECT_CAMERA:
            camera_index = atoi(optarg);
            printf("select camera index %d.\n", camera_index);
            break;

        case SET_DSPMODE:
            dsp_mode = atoi(optarg);
            printf("select dspmode %d.\n", dsp_mode);
            specify_dspmode_voutmask = 1;
            break;

        case SET_VOUT_MASK:
            vout_mask = atoi(optarg);
            printf("request vout mask %d.\n", vout_mask);
            specify_dspmode_voutmask = 1;
            break;

        case DEBUG_NOT_START_ENCODING:
            not_encoding = 1;
            printf("request not encoding.\n");
            break;

        case 'a':
            if ((audio_format[0] = get_audio_format(optarg)) ==
                IParameters::StreamFormat_Invalid)
                return  -1;
                audio_format[1] = audio_format[0];      // currently use the same fmt for both audio stream
            break;

        case 'r':
            sample_rate = atoi(optarg);
            break;

        case 'c':
            if ((channels != 1) && (channels != 2)) {
                printf("channels should be 1 or 2!\n");
                return -1;
            }
            channels = atoi(optarg);
            break;

        case 'y':
            YUVCapture_Flag = 1;
            break;

        case MSG_NOTIFICAYION_MODE:
            msg_notification_mode = atoi(optarg);
            break;

        case MSG_SERVER_PORT:
            msg_server_port = atoi(optarg);
            break;

        case MSG_LOCAL_PORT:
            msg_client_local_port = atoi(optarg);
            break;

        case MSG_SERVER_IP_ADDR:
            strncpy(msg_server_ip_addr, optarg, strlen(optarg));
            break;

        case ENALBE_TEST_OSD:
            test_osd = 1;
            AM_WARNING("[input params]: enable osd testing\n");
            break;

        case TEST_OSD_MODE:
            test_osd_mode = atoi(optarg);
            AM_WARNING("[input params]: select osd mode %d\n", test_osd_mode);
            if (test_osd_mode > ETestOSDMode_RGBA) {
                AM_ERROR("[input params]: select osd mode, BAD input params %d, select default\n", test_osd_mode);
                test_osd_mode = ETestOSDMode_CLUT;
            }
            break;

        case TEST_OSD_INPUT_FMT:
            test_osd_input_format = atoi(optarg);
            AM_WARNING("[input params]: select osd input format %d\n", test_osd_input_format);
            if (test_osd_input_format > ETestOSDInput_RGBA) {
                AM_ERROR("[input params]: select osd input format, BAD input params %d, select default\n", test_osd_input_format);
                test_osd_input_format = ETestOSDInput_color_index;
            }
            break;

#if PLATFORM_ANDROID
        case ENABLE_LDWS_ENGINE:
            enable_ldws = 1;
            break;
#endif

        case REC_STREAM_MASK:
            rec_stream_mask = atoi(optarg);
            printf("rec_stream_mask %d.\n", rec_stream_mask);
            break;

        default:
            printf("unknown option found: %d\n", ch);
            usage();
            return -1;
        }
    }
    return 0;
}

//-------------------------------------------------------------------
//                      Key Functions
//-------------------------------------------------------------------

static void write_private_data()
{
    if (!G_pRecordControl) {
        printf("!!Error: NULL G_pRecordControl in write_private_data.\n");
        return;
    }
    snprintf((char*)private_data_buffer, sizeof(private_data_buffer), (char*)private_data_gps_base, pridata_index);
    G_pRecordControl->AddPrivateDataPacket((AM_U8*)private_data_buffer, strlen((char*)private_data_buffer), (AM_U16)IParameters::PrivateDataType_GPSInfo, 1);
    snprintf((char*)private_data_buffer, sizeof(private_data_buffer), (char*)private_data_sensor_base, pridata_index);
    G_pRecordControl->AddPrivateDataPacket((AM_U8*)private_data_buffer, strlen((char*)private_data_buffer), (AM_U16)IParameters::PrivateDataType_SensorInfo, 3);
    pridata_index ++;
}

//msg notification
static int setup_datagram_socket(bool non_blocking)
{
    if(local_socket > 0){
        //setup one new socket
        close(local_socket);
        local_socket = -1;
    }
    struct sockaddr_in  servaddr;
    int reuse_flag = 1;
    local_socket = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr    =  htonl(INADDR_ANY);
    servaddr.sin_port   =   htons(msg_client_local_port);

    if (local_socket < 0) {
        AM_ERROR("unable to create dgram socket\n");
        return local_socket;
    }

    if (setsockopt(local_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_flag, sizeof reuse_flag) < 0) {
        AM_ERROR("setsockopt(SO_REUSEADDR) error\n");
        close(local_socket);
        return -1;
    }

    if (bind(local_socket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        AM_ERROR("bind() error (port number: %d)\n", msg_client_local_port);
        close(local_socket);
        return -1;
    }

    if (non_blocking) {
        int cur_flags = fcntl(local_socket, F_GETFL, 0);
        if (fcntl(local_socket, F_SETFL, cur_flags|O_NONBLOCK) != 0) {
            AM_ERROR("failed to make non-blocking\n");
            close(local_socket);
            return -1;
        }
    }

    memset(&msg_dest_addr, 0x0, sizeof(msg_dest_addr));
    msg_dest_addr.sin_family = AF_INET;
    msg_dest_addr.sin_port = htons(msg_server_port);
    msg_dest_addr.sin_addr.s_addr = inet_addr(msg_server_ip_addr);

    return local_socket;
}

static void _post_msg_to_server(struct sockaddr_in* dest_addr, int local_socket)
{
    char msg[256] = "[IDR] current frame is IDR frame!!";
    AM_UINT size = strlen(msg);
    AM_INT ret;
    ret = sendto(local_socket, msg, size, 0, (struct sockaddr*)dest_addr, sizeof(struct sockaddr));
    printf("sendto ret %d\n", ret);
}

static int start_audiortsp(PbHandle *handle,char *stream_url);
static int start_audiortsp(int client_id,PbHandle *handle);
static int stop_audiortsp(PbHandle *handle);

static int get_param_str(char *dst, char *src)
{
    int len = 0;
    char *dst_ = dst;
    char *src_ = src;
    while(src_ && *src_ != '\n' && *src_ != '\0'){
        if(*src_ != ' '){
            *dst_++ = *src_;
            ++len;
        }
        src_++;
    }
    *dst_ = '\0';
    return len;
}
void RunMainLoop()
{
    char buffer_old[1024] = {0};
    char buffer[1024];
    int flag_stdin = 0;
    int paused = 0;
    int ret;
    unsigned int param0, param1, param2, param3,param4;
    char param5[64] = {0};

    int audio_rtsp_started = 0;
    PbHandle pbHandle;

    bool audio_rec_closed = false;
    bool hdmi_preview_freezed = false;

    flag_stdin = fcntl(STDIN_FILENO,F_GETFL);
    if(fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1)
        printf("stdin_fileno set error.\n");

    while (record_running) {
        //add sleep to avoid affecting the performance
        usleep(10000);

        //write test pridata if needed
        if (enable_pridata_test) {
            write_private_data();
        }

        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
            continue;

        if (buffer[0] == '\n')
            buffer[0] = buffer_old[0];

        switch (buffer[0]) {
            case 'T':
                if(audio_rtsp_started){
                    printf("T audiortsp has been started.\n");
                }else{
                    char stream_url[1024];
                    memset(stream_url,0,sizeof(stream_url));
                    if(!get_param_str(stream_url,&buffer[1])){
                        printf("T audiortsp Failed to call NVR, please input rtsp-url\n");
                    }else{
                        memset(&pbHandle,0,sizeof(pbHandle));
                        if(!start_audiortsp(&pbHandle,stream_url)){
                            audio_rtsp_started = 1;
                        }else{
                            printf("T audiortsp Failed to call NVR\n");
                        }
                    }
                }
                break;
            case 't':
                if(audio_rtsp_started){
                    printf("audio rtsp has been started.\n");
                    break;
                }
                if(!enable_wsd){
                    printf("Failed to call NVR, please quit and enable-wsd first.\n");
                    break;
                }
                memset(&pbHandle,0,sizeof(pbHandle));
                if(start_audiortsp(0,&pbHandle) < 0){
                    printf("Failed to call NVR, please try again\n");
                    break;
                }
                audio_rtsp_started = 1;
                break;
            case 'Q':
		  if(audio_rtsp_started){
                    audio_rtsp_started = 0;
                    stop_audiortsp(&pbHandle);
                    printf("Quit Talk, audiortsp\n");
                }
                break;

            case 'q':   // exit
                printf("Quit\n");
                YUVCapture_Flag = 0;
                record_running = 0;
                if(audio_rtsp_started){
                    audio_rtsp_started = 0;
                    stop_audiortsp(&pbHandle);
                }
                break;

            case ' ':   // pause, resume
                buffer_old[0] = buffer[0];
                if (paused == 0) {
                    printf("rectest:pause.\n");
                    G_pRecordControl->PauseRecord();
                    paused = 1;
                } else {
                    printf("rectest:resume.\n");
                    G_pRecordControl->ResumeRecord();
                    paused = 0;
                }
                break;

            case 'c':
                if (!G_pRecordControl)
                    break;
                if (!strncmp(buffer+1, "pre-display", 11)){
                    ret = sscanf(buffer, "cpre-display:%d,%d,%d,%d,%d", &param0, &param1, &param2, &param3, &param4);
                    if (5 == ret) {
                        G_pRecordControl->UpdatePreviewDisplay(param0, param1, param2, param3, param4);
                    } else {
                        printf("change preview display's cmd should be: 'cpre-display:size_x,size_y,offset_x,offset_y,alpha' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "bitrate", 7)){
                    ret = sscanf(buffer, "cbitrate:%d", &param0);
                    if (1 == ret) {
                        G_pRecordControl->SetVideoStreamBitrate(0, 0, param0);//hard code ,fix me
                    } else {
                        printf("change enc bitrate's cmd should be: 'cbitrate:bitrate' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "framerate", 9)){
                    ret = sscanf(buffer, "cframerate:%d,%d", &param0, &param1);
                    if (2 == ret) {
                        G_pRecordControl->SetVideoStreamFramerate(0, 0, param0, param1);
                    } else {
                        printf("change enc framerate's cmd should be: 'cframerate:framerate_reduce_factor,framerate_integer' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "idr", 3)){
                    ret = sscanf(buffer, "cidr:%d,%d", &param0,&param1);
                    if (2 == ret) {
                        G_pRecordControl->DemandIDR(0);
                    } else {
                        printf("demand idr's cmd should be: 'cidr:method,pts' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "gop", 3)){
                    ret = sscanf(buffer, "cgop:%d,%d,%d,%d", &param0, &param1, &param2, &param3);
                    if (4 == ret) {
                        G_pRecordControl->UpdateGOPStructure(0, param0, param1, param2);
                    } else {
                        printf("update gop's cmd should be: 'cgop:M,N,idrinterval,gop' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "display", 7)){
                    ret = sscanf(buffer, "cdisplay:%d,%d,%d,%d", &param0, &param1, &param2, &param3);
                    if (4 == ret) {
                        G_pRecordControl->SetDisplayMode(param0, param1, param2, param3);
                    } else {
                        printf("cdisplay's cmd should be: 'cdisplay:preview_enable,playback_enabled,preview_vout,pb_vout_index' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "capjpeg", 7)){
                    ret = sscanf(buffer, "ccapjpeg:%d,%d", &param0, &param1);
                    printf("request jpeg size %dx%d.\n", param0, param1);
                    if (2 == ret) {
                        G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_CAPTURE_JPEG, ((param0&0xffff)<<16) | (param1&0xffff));
                    } else {
                        printf("cdisplay's cmd should be: 'ccapjpeg:pic_w,pic_h' + 'enter' \n");
                    }
                } else if (!strncmp(buffer + 1, "osddisable", 10)) {
                    if (!strncmp(buffer + 1, "osddisableall", 13)) {
                        printf("[input cmd] disable osd all.\n");
                        test_osd_area[0].tobe_disabled = 1;
                        test_osd_area[1].tobe_disabled = 1;
                        test_osd_area[2].tobe_disabled = 1;
                        break;
                    }
                    ret = sscanf(buffer + 1, "osddisable:%d", &param0);
                    if (1 == ret) {
                        if (param0 < 4) {
                            printf("[input cmd] disable osd %d.\n", param0);
                            test_osd_area[param0].tobe_disabled = 1;
                        } else {
                            printf("cosddisable's cmd should with correct osd_id, %d out of range\n", param0);
                        }
                    } else {
                        printf("cosddisable's cmd should be: 'cosddisable:%%d\n");
                    }
                } else if (!strncmp(buffer + 1, "osdenable", 9)) {
                     if (!strncmp(buffer + 1, "osdenableall", 12)) {
                        printf("[input cmd] enable osd all.\n");
                        test_osd_area[0].tobe_disabled = 0;
                        test_osd_area[1].tobe_disabled = 0;
                        test_osd_area[2].tobe_disabled = 0;
                        break;
                    }
                    ret = sscanf(buffer + 1, "osdenable:%d", &param0);
                    if (1 == ret) {
                        if (param0 < 4) {
                            printf("[input cmd] enable osd %d.\n", param0);
                            test_osd_area[param0].tobe_disabled = 0;
                        } else {
                            printf("cosdenable's cmd should with correct osd_id, %d out of range\n", param0);
                        }
                    } else {
                        printf("cosdenable's cmd should be: 'cosdenable:%%d\n");
                    }
                }
                break;

            case 'Y':   // capture one yuv data
                    if(YUVCapture_Flag!=1){YUVCapture_Flag = 2;}
                break;

            case 'y':   // capture yuv data
                if(YUVCapture_Flag){
                    YUVCapture_Flag = 0;
                }else{
                    YUVCapture_Flag = 1;
                }
                break;

            case 'p':
            case 'P':
                G_pRecordControl->PrintState();
                break;

            case 'h':
                usage();
                break;

            case 's':
                printf("Enable streaming(main stream).\n");
                if (G_pStreamingControl) {
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 0);//video
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 1);//audio
                }
                break;
            case 'S':
                printf("Enable streaming(second stream).\n");
                if (G_pStreamingControl) {
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 0);//video
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 1);//audio
                }
                break;
            case 'd':
                printf("disable streaming(main stream).\n");
                if (G_pStreamingControl) {
                    G_pStreamingControl->DisableStreamming(rtsp_server_index, 0, 0);//video
                    G_pStreamingControl->DisableStreamming(rtsp_server_index, 0, 1);//audio
                }
                break;
            case 'D':
                printf("disable streaming(second stream).\n");
                if (G_pStreamingControl) {
                    G_pStreamingControl->DisableStreamming(rtsp_server_index, 1, 0);//video
                    G_pStreamingControl->DisableStreamming(rtsp_server_index, 1, 1);//audio
                }
                break;
            case '-':
                printf("%s\n", buffer);
                if(!strncmp(buffer+1, "stopmsg", 7)){
                    msg_notification_mode = 0;
                }else if(!strncmp(buffer+1, "startmsg:", 9)){
                    //-startmsg:local_port server_port server_ip
                    ret = sscanf(buffer+1, "startmsg:%d %d %s", &param0, &param1, param5);
                    if(ret == 3){
                        msg_client_local_port = param0;
                        msg_server_port = param1;
                        strncpy(msg_server_ip_addr, param5, strlen(param5));
                        msg_notification_mode = 2;
                    }
                    printf("msg_notification_mode %d,strlen(buffer) %s\n", msg_notification_mode, msg_server_ip_addr);
                }else if(!strncmp(buffer+1, "restartmsg", 10)){
                    //restart
                    msg_notification_mode = 1;
                }else if(!strncmp(buffer+1, "closeaudiorec", 13)){
                    //-closeaudiorec to close audio HAL
                    if(audio_rec_closed == false){
                        G_pRecordControl->CloseAudioHAL();
                        audio_rec_closed = true;
                    }
                }else if(!strncmp(buffer+1, "reopenaudiorec", 14)){
                    //-reopenaudiorec to reopen audio HAL
                    if(audio_rec_closed == true){
                        G_pRecordControl->ReopenAudioHAL();
                        audio_rec_closed = false;
                    }
                }else if(!strncmp(buffer+1, "freezehdmi", 10)){
                    printf("freeze HDMI preview...\n");
                    G_pRecordControl->FreezeResumeHDMIPreview(1);
                }else if(!strncmp(buffer+1, "resumehdmi", 10)){
                    printf("resume HDMI preview...\n");
                    G_pRecordControl->FreezeResumeHDMIPreview(2);
                }else if(!strncmp(buffer+1, "jpeg", 4)){
                     printf("capture JPEG...\n");
                     ret = sscanf(buffer+1, "jpeg:%s", param5);
                     if(ret == 1){
                        G_pRecordControl->CaptureJPEG(param5);
                     }
                }
                break;

            default:
                break;
        }
    }

    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
        AM_ERROR("stdin_fileno set error");
}

AM_ERR YUVCapThread(void *context)
{
    char name[16];
    AM_ERR err;

    char datetime_buffer[128];
    time_t lasttime;

    while (1) {
        if(yuv_num>9999)yuv_num = 0;
        if(YUVCapture_Flag==0){
            if (G_pYUVcapEvent->Wait(1000) == ME_OK)
                break;
        }else{
            memset(datetime_buffer, 0, sizeof(datetime_buffer));
            lasttime = time(NULL);
            strftime(datetime_buffer, sizeof(datetime_buffer), "%Y%m%d_%H%M%S", gmtime(&lasttime));
            sprintf(name, "%04u_%s.yuv", yuv_num, datetime_buffer);
            err = G_pRecordControl->CaptureYUV(name);
            if(err!= ME_OK){
                printf("CaptureYUV error: %d!\n", err);
            }else{
                yuv_num++;
            }
            if(YUVCapture_Flag==2){YUVCapture_Flag = 0;}
        }
    }

    printf("YUVCapThread exit.\n");
    return ME_OK;
}

static void sigstop(int a)
{
    record_running = 0;
}

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    static unsigned int seq_num[2] = {0};
    unsigned int start_index = 0;

    AM_INFO("AMF msg: %d\n", msg.code);
    if (msg.code == IMediaControl::MSG_RECORDER_EOS) {
        printf("==== Record end ====\n");
        record_running = 0;
    } else if (msg.code == IMediaControl::MSG_NEWFILE_GENERATED_POST_TO_APP) {
        //update file
        printf("==== New file generated, p2 %d, p4 %d, p5 %d, seq_num[0] %d, seq_num[1] %d ====\n", msg.p2, msg.p4, msg.p5, seq_num[0], seq_num[1]);

        //calculate start index, not care wrapper case, todo
        if (msg.p2 > 4) {
            start_index = msg.p2 - 4;
        }

        if (msg.p4 > 1) {
            printf("BAD muxer index %d, please check, set it to 0 for safe.\n", msg.p4);
            msg.p4 = 0;
        }

        //main stream write file
        if (msg.p4 == 0) {
            AM_WriteHLSConfigfile((char*)"girls_hd.m3u8", file_name[0], start_index, msg.p2, AM_GetStringFromContainerType((IParameters::ContainerType)msg.p5), seq_num[msg.p4]);
        } else {
            AM_WriteHLSConfigfile((char*)"girls.m3u8", file_name[1], start_index, msg.p2, AM_GetStringFromContainerType((IParameters::ContainerType)msg.p5), seq_num[msg.p4]);
        }
    }else if (IMediaControl::MSG_OS_ERROR_POST_TO_APP == msg.code) {
        printf("==== MSG_OS_ERROR ====\n");
        record_running = 0;
    }else if(IMediaControl::MSG_EVENT_NOTIFICATION_TO_APP == msg.code){
        if(msg_notification_mode == 1){
            printf("====post one msg notification to server====\n");
            printf("====msg type: %d, stream id: %d====\n", msg.p3, msg.p2);
            if(local_socket == -1){
                setup_datagram_socket(false);
            }
            _post_msg_to_server(&msg_dest_addr, local_socket);
        }else if(msg_notification_mode == 2){
            printf("update msg server config\n");
            setup_datagram_socket(false);
            _post_msg_to_server(&msg_dest_addr, local_socket);
            msg_notification_mode = 1;
        }
    }
}

//skychen, 2012_11_21
int HandleMSGNotification(void* thiz, AM_UINT msg_type, void* msg_body)
{
    switch(msg_type){
        case AM_MSG_TYPE_IDR_NOTIFICATION:
            printf("Get IDR notification msg, current frame is IDR frame!!\n");
            break;
        default:
            printf("Unknown msg type: %d!!\n", msg_type);
            break;
    }
    return 0;
}

void* _test_osd_thread_argb(void* p)
{
    int i = 0;
    AM_ERR err;

    //debug
    sleep(3);

    //hard code here
    test_osd_area[0].offset_x = 100;
    test_osd_area[0].offset_y = 20;
    test_osd_area[0].width = 64;
    test_osd_area[0].height = 20;

    test_osd_area[1].offset_x = 300;
    test_osd_area[1].offset_y = 50;
    test_osd_area[1].width = 128;
    test_osd_area[1].height = 30;

    test_osd_area[2].offset_x = 100;
    test_osd_area[2].offset_y = 400;
    test_osd_area[2].width = 160;
    test_osd_area[2].height = 30;

    //init memory
    for (i = 0; i < test_osd_number; i ++) {
        test_osd_area[i].vout_width = 1280;//hard code here
        test_osd_area[i].vout_height = 720;

        if (alloc_osd_area(&test_osd_area[i], test_osd_mode) < 0) {
            AM_ERROR("alloc_osd_area(i %d) fail, exit\n", i);
            test_osd_running = 0;
            break;
        }

        init_osd_parameters(&test_osd_area[i]);
        update_osd_color(&test_osd_area[i]);
    }

    //add osd area
    for (i = 0; i < test_osd_number; i ++) {
        test_osd_area[i].osd_id = G_pRecordControl->AddOsdBlendArea(test_osd_area[i].offset_x, test_osd_area[i].offset_y, test_osd_area[i].width, test_osd_area[i].height);
        if (test_osd_area[i].osd_id < 0) {
            AM_ERROR("AddOsdBlendArea(i %d) fail, return %d\n", i, test_osd_area[i].osd_id);
            test_osd_area[i].iav_success = 0;
        } else {
            test_osd_area[i].iav_success = 1;
        }
    }

    //running
    while (test_osd_running) {
        usleep(100000);
        for (i = 0; i < test_osd_number; i ++) {
            if (!test_osd_area[i].iav_success) {
                continue;
            }

            if ((!test_osd_area[i].disabled) && (test_osd_area[i].tobe_disabled)) {
                //disable osd
                AM_WARNING("RemoveOsdBlendArea(i %d), osd_id %d, width %d, height %d, offx %d, offy %d\n", i, test_osd_area[i].osd_id, test_osd_area[i].width, test_osd_area[i].height, test_osd_area[i].offset_x, test_osd_area[i].offset_y);
                err = G_pRecordControl->RemoveOsdBlendArea(test_osd_area[i].osd_id);
                if (ME_OK != err) {
                    AM_ERROR("RemoveOsdBlendArea(i %d, osd_id %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
                }
                test_osd_area[i].disabled = 1;
                continue;
            } else if ((test_osd_area[i].disabled) && (!test_osd_area[i].tobe_disabled)) {
                //enable osd
                test_osd_area[i].osd_id = G_pRecordControl->AddOsdBlendArea(test_osd_area[i].offset_x, test_osd_area[i].offset_y, test_osd_area[i].width, test_osd_area[i].height);
                if (test_osd_area[i].osd_id < 0) {
                    AM_ERROR("AddOsdBlendAreaCLUT(i %d) fail, return %d\n", i, test_osd_area[i].osd_id);
                    test_osd_area[i].iav_success = 0;
                    continue;
                } else {
                    test_osd_area[i].iav_success = 1;
                }
            } else if (test_osd_area[i].disabled) {
                continue;
            }

            if (test_osd_area[i].display_tick >= test_osd_area[i].display_duration) {
                generate_osd_content(&test_osd_area[i]);
                err = G_pRecordControl->UpdateOsdBlendArea(test_osd_area[i].osd_id, test_osd_area[i].p_data, test_osd_area[i].width, test_osd_area[i].height);
                update_osd_color(&test_osd_area[i]);
                if (ME_OK != err) {
                    AM_ERROR("UpdateOsdBlendArea(osd_id %d, i %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
                }
                test_osd_area[i].display_tick = 0;
            } else {
                test_osd_area[i].display_tick ++;
            }
        }
    }

    //release
    for (i = 0; i < test_osd_number; i ++) {
        if (test_osd_area[i].disabled || (!test_osd_area[i].iav_success)) {
            continue;
        }
        err = G_pRecordControl->RemoveOsdBlendArea(test_osd_area[i].osd_id);
        if (ME_OK != err) {
            AM_ERROR("RemoveOsdBlendArea(i %d, osd_id %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
        }
    }

    for (i = 0; i < test_osd_number; i ++) {
        release_osd_area(&test_osd_area[i]);
    }

    return NULL;
}

void* _test_osd_thread_clut(void* p)
{
    int i = 0;
    AM_ERR err;
    IParameters::OSDInputDataFormat input_format;

    //debug
    sleep(3);

    //hard code here
    test_osd_area[0].offset_x = 100;
    test_osd_area[0].offset_y = 20;
    //test_osd_area[0].width = 64;
    //test_osd_area[0].height = 20;
    test_osd_area[0].width = 128;
    test_osd_area[0].height = 50;

    test_osd_area[1].offset_x = 300;
    test_osd_area[1].offset_y = 200;
    //test_osd_area[1].width = 128;
    //test_osd_area[1].height = 30;
    test_osd_area[1].width = 256;
    test_osd_area[1].height = 60;

    test_osd_area[2].offset_x = 500;
    test_osd_area[2].offset_y = 600;
    //test_osd_area[2].width = 160;
    //test_osd_area[2].height = 30;
    test_osd_area[2].width = 320;
    test_osd_area[2].height = 80;

    if (ETestOSDInput_color_index == test_osd_input_format) {
        input_format = IParameters::OSDInputDataFormat_YUVA_CLUT;
    } else if (ETestOSDInput_RGBA == test_osd_input_format) {
        input_format = IParameters::OSDInputDataFormat_RGBA;
    } else {
        AM_ERROR("BAD input format %d\n", test_osd_input_format);
        return NULL;
    }

    //init memory
    for (i = 0; i < test_osd_number; i ++) {
        test_osd_area[i].vout_width = 1280;//hard code here
        test_osd_area[i].vout_height = 720;

        if (alloc_osd_area(&test_osd_area[i], test_osd_input_format) < 0) {
            AM_ERROR("alloc_osd_area(i %d) fail, exit\n", i);
            test_osd_running = 0;
            break;
        }

        init_osd_parameters(&test_osd_area[i]);
        update_osd_color(&test_osd_area[i]);
    }

    //add osd area
    for (i = 0; i < test_osd_number; i ++) {
        test_osd_area[i].osd_id = G_pRecordControl->AddOsdBlendAreaCLUT(test_osd_area[i].offset_x, test_osd_area[i].offset_y, test_osd_area[i].width, test_osd_area[i].height);
        if (test_osd_area[i].osd_id < 0) {
            AM_ERROR("AddOsdBlendAreaCLUT(i %d) fail, return %d\n", i, test_osd_area[i].osd_id);
            test_osd_area[i].iav_success = 0;
        } else {
            test_osd_area[i].iav_success = 1;
        }
    }

    //running
    while (test_osd_running) {
        usleep(100000);
        for (i = 0; i < test_osd_number; i ++) {
            if (!test_osd_area[i].iav_success) {
                continue;
            }

            if ((!test_osd_area[i].disabled) && (test_osd_area[i].tobe_disabled)) {
                //disable osd
                AM_WARNING("RemoveOsdBlendArea(i %d), osd_id %d, width %d, height %d, offx %d, offy %d\n", i, test_osd_area[i].osd_id, test_osd_area[i].width, test_osd_area[i].height, test_osd_area[i].offset_x, test_osd_area[i].offset_y);
                err = G_pRecordControl->RemoveOsdBlendAreaCLUT(test_osd_area[i].osd_id);
                if (ME_OK != err) {
                    AM_ERROR("RemoveOsdBlendArea(i %d, osd_id %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
                }
                test_osd_area[i].disabled = 1;
                continue;
            } else if ((test_osd_area[i].disabled) && (!test_osd_area[i].tobe_disabled)) {
                //enable osd
                test_osd_area[i].osd_id = G_pRecordControl->AddOsdBlendAreaCLUT(test_osd_area[i].offset_x, test_osd_area[i].offset_y, test_osd_area[i].width, test_osd_area[i].height);
                if (test_osd_area[i].osd_id < 0) {
                    AM_ERROR("AddOsdBlendAreaCLUT(i %d) fail, return %d\n", i, test_osd_area[i].osd_id);
                    test_osd_area[i].iav_success = 0;
                    continue;
                } else {
                    test_osd_area[i].iav_success = 1;
                }
            } else if (test_osd_area[i].disabled) {
                continue;
            }

            if (test_osd_area[i].display_tick >= test_osd_area[i].display_duration) {
                generate_osd_content(&test_osd_area[i]);
                AM_WARNING("UpdateOsdBlendArea(i %d), area %p, width %d, height %d, data %p, color_index %d, color value 0x%08x\n", i, &test_osd_area[i], test_osd_area[i].width, test_osd_area[i].height, test_osd_area[i].p_data, test_osd_area[i].color_index, test_osd_area[i].p_clut[test_osd_area[i].color_index]);
                err = G_pRecordControl->UpdateOsdBlendAreaCLUT(test_osd_area[i].osd_id, test_osd_area[i].p_data, test_osd_area[i].p_clut, test_osd_area[i].width, test_osd_area[i].height, input_format);
                update_osd_color(&test_osd_area[i]);
                if (ME_OK != err) {
                    AM_ERROR("UpdateOsdBlendArea(osd_id %d, i %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
                }
                test_osd_area[i].display_tick = 0;
            } else {
                test_osd_area[i].display_tick ++;
            }
        }
    }

    //release
    for (i = 0; i < test_osd_number; i ++) {
        if (test_osd_area[i].disabled || (!test_osd_area[i].iav_success)) {
            continue;
        }
        err = G_pRecordControl->RemoveOsdBlendArea(test_osd_area[i].osd_id);
        if (ME_OK != err) {
            AM_ERROR("RemoveOsdBlendArea(i %d, osd_id %d) fail, return %d\n", i, test_osd_area[i].osd_id, err);
        }
    }

    for (i = 0; i < test_osd_number; i ++) {
        if (!test_osd_area[i].iav_success) {
            continue;
        }
        release_osd_area(&test_osd_area[i]);
    }

    return NULL;
}

//////////////////////////////////////////////////
static void client_info(void *usr, char *uuid,char *ipaddr,char *stream_url){
    printf("\t client, uuid [%s], ipaddr[%s],stream_url[%s]\n",uuid,ipaddr,stream_url);
}
static void DoDisplayClient(){
    printf("WSDiscoveryService::getClientList\n");
    WSDiscoveryService::GetInstance()->getClientList(client_info,NULL);
    printf("WSDiscoveryClient::getDeviceList END\n");
}

static int client_num = 0;
struct client_node_t{
    char uuid[64];
    char ipaddr[64];
    char stream_url[1024];
    struct client_node_t *next;
} * client_list = NULL;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

static int free_client_list(){
    pthread_mutex_lock(&client_mutex);
    while(client_list){
        client_node_t *next = client_list->next;
        delete client_list;
        client_list = next;
    }
    client_num = 0;
    pthread_mutex_unlock(&client_mutex);
    return 0;
}

static int add_client(char *uuid,char *ipaddr,char *stream_url){
    pthread_mutex_lock(&client_mutex);
    client_node_t *node = client_list;
    while(node){
         if(!strcmp(node->uuid,uuid)){
             snprintf(node->ipaddr,sizeof(node->ipaddr),"%s",ipaddr);
             snprintf(node->stream_url,sizeof(node->stream_url),"%s",stream_url);
             pthread_mutex_unlock(&client_mutex);
             return 1;//duplicate
         }
         node = node->next;
    }

    node = new client_node_t;
    snprintf(node->uuid,sizeof(node->uuid),"%s",uuid);
    snprintf(node->ipaddr,sizeof(node->ipaddr),"%s",ipaddr);
    snprintf(node->stream_url,sizeof(node->stream_url),"%s",stream_url);
    node->next = NULL;
    if(client_list){
        node->next = client_list;
    }
    client_list = node;
    ++client_num;
    pthread_mutex_unlock(&client_mutex);
    return 0;
}

static int remove_client(char *uuid){
    pthread_mutex_lock(&client_mutex);
    client_node_t *prev = NULL,*curr = client_list;
    while(curr){
        if(!strcmp(curr->uuid,uuid)){
            if(prev){
                prev->next = curr->next;
            }else{
                client_list = curr->next;
            }
            delete curr;
            --client_num;
            pthread_mutex_unlock(&client_mutex);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
    return -1;
}

static int get_client_url(int client, char *url,int size_url){
    pthread_mutex_lock(&client_mutex);
    if(client >= client_num){
        pthread_mutex_unlock(&client_mutex);
        return -1;
    }
    //TODO, now the head return
    snprintf(url,size_url,"%s",client_list->stream_url);
    pthread_mutex_unlock(&client_mutex);
    return 0;
}
class ClientObserver:public IWsdClientObserver{
public:
    virtual void onClientChanged(char *uuid,char *ipaddr,char *stream_url){
        printf("ClientObserver::onClientChanged,uuid[%s],ipaddr[%s],stream_url[%s]\n",uuid,ipaddr ? ipaddr:"NULL",stream_url ? stream_url:"NULL");
        if(ipaddr){
            add_client(uuid,ipaddr,stream_url);
        }else{
            remove_client(uuid);
        }
    }
};

#include <string.h>
static int start_audiortsp(PbHandle *handle,char *stream_url){
    if(!stream_url){
        return -1;
    }
    char rtsp_url[2048];
    if(!strstr(stream_url,"audio_only")){
        if(!strstr(stream_url,"?")){
            snprintf(rtsp_url,sizeof(rtsp_url),"%s?audio_only",stream_url);
        }else{
            snprintf(rtsp_url,sizeof(rtsp_url),"%s&audio_only",stream_url);
        }
    }else{
        snprintf(rtsp_url,sizeof(rtsp_url),"%s",stream_url);
    }
    printf("start_audiortsp -- rtspurl[%s]\n",rtsp_url);
#if PLATFORM_ANDROID
    handle->audiosink = (new MediaPlayerService::AudioOutput(1));
    if(!handle->audiosink){
        return -1;
    }
    if((handle->ctrl = CreateActivePBControl(handle->audiosink)) == NULL){
        delete handle->audiosink;
        return -1;
    }
#else
    if((handle->ctrl = CreateActivePBControl(NULL)) == NULL){
        return -1;
    }
#endif

    if(handle->ctrl->PrepareFile(rtsp_url) != ME_OK){
#if PLATFORM_ANDROID
        delete handle->audiosink;
#endif
        handle->ctrl->Delete();
        return -1;
    }

    if(handle->ctrl->PlayFile(rtsp_url) != ME_OK){
#if PLATFORM_ANDROID
        delete handle->audiosink;
#endif
        handle->ctrl->Delete();
        return -1;
    }
    return 0;
}

static int start_audiortsp(int client_id,PbHandle *handle){
    char client_url[1024];
    if(get_client_url(client_id,client_url,sizeof(client_url)) < 0){
        return -1;
    }
    char rtsp_url[2048];
    snprintf(rtsp_url,sizeof(rtsp_url),"%s?audio_only",client_url);
#if PLATFORM_ANDROID
    handle->audiosink = (new MediaPlayerService::AudioOutput(1));
    if(!handle->audiosink){
        return -1;
    }
    if((handle->ctrl = CreateActivePBControl(handle->audiosink)) == NULL){
        delete handle->audiosink;
        return -1;
    }
#else
    if((handle->ctrl = CreateActivePBControl(NULL)) == NULL){
        return -1;
    }
#endif

    if(handle->ctrl->PrepareFile(rtsp_url) != ME_OK){
#if PLATFORM_ANDROID
        delete handle->audiosink;
#endif
        handle->ctrl->Delete();
        return -1;
    }

    if(handle->ctrl->PlayFile(rtsp_url) != ME_OK){
#if PLATFORM_ANDROID
        delete handle->audiosink;
#endif
        handle->ctrl->Delete();
        return -1;
    }
    return 0;
}

static int stop_audiortsp(PbHandle *handle){
    if(handle->ctrl){
        handle->ctrl->StopPlay();
        handle->ctrl->Delete();
        handle->ctrl = NULL;
    }
#if PLATFORM_ANDROID
    if(handle->audiosink){
        delete handle->audiosink;
        handle->audiosink = NULL;
    }
#endif
    return 0;
}

//-------------------------------------------------------------------
//                      Main Function
//-------------------------------------------------------------------
extern "C" int main(int argc, char **argv)
{
    AM_UINT output_index;
    AM_UINT stream_index;
    AM_UINT stream_type;
    AM_UINT stream_format;
    AM_ERR err;
    char* filename;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    int iav_fd = -1;
    pthread_t test_osd_thread;
    void* p_jointhead;

    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    // parse parameters of 'rectest'
    if (init_param(argc, argv) < 0) {
        usage();
        return -1;
    }

    if(msg_notification_mode == 2){//work as msg server, listen for msg notification
        printf("just to work as msg server to listen for msg notificatioin\n");
        CSimpleDataBase* pSimpleDataBase = new CSimpleDataBase();

        pSimpleDataBase->Start(NULL, HandleMSGNotification, msg_server_port);

        RunMainLoop();

        msg_notification_mode = 0;
        pSimpleDataBase->Stop();
        delete pSimpleDataBase;
        pSimpleDataBase = NULL;
        return 0;
    }

    if (camera_index<0 || camera_index>1) {
        printf("BAD camera_index %d, use default 0.\n", camera_index);
        camera_index = 0;
    }

    if (specify_dspmode_voutmask) {
        if(dsp_mode  !=  DSPMode_CameraRecording && dsp_mode != DSPMode_DuplexLowdelay){
            dsp_mode = DSPMode_DuplexLowdelay;
        }
        if(dsp_mode == DSPMode_DuplexLowdelay && (total_muxer_number > 1)){
            total_muxer_number = 1;
        }
    }

    if (ME_OK != AMF_Init()) {
        AM_ERROR("AMF_Init Error.\n");
        return -2;
    }

#ifdef __use_imgproc__
    if (auto_start_aaa) {
        //initvin, start 3a
        printf("[aaa] open /dev/iav.\n");
        iav_fd = open("/dev/iav", O_RDWR, 0);
        if (iav_fd < 0) {
            printf("open iav fail.\n");
            return -1;
        }
        printf("[aaa] open /dev/iav %d.\n", iav_fd);
        G_pPreTreat = AM_CreateRecPreTreat(iav_fd);
        if (!G_pPreTreat) {
            printf("AM_CreateRecPreTreat fail.\n");
            return -2;
        }
/*
        //set preview c size
        printf("start init vin, enter preview, w %d, h %d, camera index %d.\n", DefaultPreviewCWidth, DefaultPreviewCHeight, camera_index);
        err = G_pPreTreat->EnterPreview(DefaultPreviewCWidth, DefaultPreviewCHeight, camera_index);
        AM_ASSERT(ME_OK == err);
        printf("start trigger aaa.\n");
        err = G_pPreTreat->StartImgProc();
        printf("trigger aaa done.\n");
*/
    }
#endif

//android hard code to 44.1khz
#if PLATFORM_ANDROID
    sample_rate = 44100;
#endif

    strcpy(path_name, AM_GetPath(AM_PathConfig));
    if (need_set_path) {
        AM_SetPathes(path_name, path_name);
    }

    //read global cfg from rec.config, optional
    snprintf(configfilename, sizeof(configfilename), "%s/rec.config", AM_GetPath(AM_PathConfig));
    AM_LoadGlobalCfg(configfilename);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);

    if ((G_pRecordControl = CreateRecordControl2(streaming_only)) == NULL) {
        AM_ERROR("Cannot create record control\n");
        return -1;
    }

    //config msg server ip addr and port
    if(msg_notification_mode == 1){//work as msg client to send msg notification
        //setup local socket
        setup_datagram_socket(false);
        AM_WARNING("msg_notification_mode %d, %d, ", msg_notification_mode, local_socket);
    }else{
        msg_notification_mode = 0;
    }

    //get streaming interface
    if (rtsp_enabled) {
        G_pStreamingControl = IStreamingControl::GetInterfaceFrom(G_pRecordControl);
        if (!G_pStreamingControl) {
            printf("!!Error: cannot get IStreamingControl interface.\n");
            return -2;
        }
    }

    if (specify_dspmode_voutmask) {
        if (DSPMode_CameraRecording!= dsp_mode) {
            AM_WARNING("rectest try not CameraRecording mode(%d), vout mask %d.\n", dsp_mode, vout_mask);
            G_pRecordControl->SetWorkingMode(dsp_mode, vout_mask);
        } else {
            //need invoke this? test engine's case 'no one invoke this api'
            G_pRecordControl->SetWorkingMode(dsp_mode, vout_mask);
        }
    }

    if (not_encoding) {
        printf("before G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_NOT_START_ENCODING, 1).\n");
        G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_NOT_START_ENCODING, 1);
    }

    printf("before G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_SELECT_CAMERA, %d).\n", camera_index);
    G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_SELECT_CAMERA, camera_index);

    if (rec_stream_mask == 0b01) {
        printf("only audio mode!\n");
        G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_STREAM_ONLY_AUDIO, rec_stream_mask);
    } else if (rec_stream_mask == 0b10) {
        printf("only video mode!\n");
        G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_STREAM_ONLY_VIDEO, rec_stream_mask);
    }

    printf("G_pRecordControl %p,G_pStreamingControl %p.\n", G_pRecordControl, G_pStreamingControl);
    //set output/muxer number
    err = G_pRecordControl->SetTotalOutputNumber(total_muxer_number);
    if_err_return(err);

    printf("saving_stratege %d, condition %d, autotime %d, auto_frame_count %d.\n", saving_stratege, saving_condition, auto_time, auto_frame_count);
    if (saving_condition == IParameters::MuxerSavingCondition_FrameCount) {
        err = G_pRecordControl->SetSavingStrategy((IParameters::MuxerSavingFileStrategy)saving_stratege, (IParameters::MuxerSavingCondition)saving_condition, IParameters::MuxerAutoFileNaming_ByNumber, auto_frame_count);
    } else if (saving_condition == IParameters::MuxerSavingCondition_InputPTS || saving_condition == IParameters::MuxerSavingCondition_CalculatedPTS) {
        err = G_pRecordControl->SetSavingStrategy((IParameters::MuxerSavingFileStrategy)saving_stratege, (IParameters::MuxerSavingCondition)saving_condition, IParameters::MuxerAutoFileNaming_ByNumber, auto_time);
    }

    err = G_pRecordControl->SetMaxFileNumber(max_file_num);

    //skychen, 2012_7_17
    err = G_pRecordControl->SetTotalFileNumber(total_file_num);


    if(enable_video==0){
        err = G_pRecordControl->DisableVideo();
        if_err_return(err);
    }
    if(enable_audio==0){
        err = G_pRecordControl->DisableAudio();
        if_err_return(err);
    }

    for (output_index=0; output_index<total_muxer_number; output_index++) {
        //setup ouput/muxer
        err = G_pRecordControl->SetupOutput(output_index, file_name[output_index], output_format[output_index]);
        if_err_return(err);

        //add video stream
        err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_Video, video_format[output_index]);
        if_err_return(err);

        if(encode_stream_conf[output_index].enc_width!=0 && encode_stream_conf[output_index].enc_height!=0){
            err = G_pRecordControl->SetVideoStreamDimention(output_index, stream_index,
                encode_stream_conf[output_index].enc_width, encode_stream_conf[output_index].enc_height);
            if_err_return(err);
        }
        if(encode_stream_conf[output_index].video_bitrate!=0){
            err = G_pRecordControl->SetVideoStreamBitrate(output_index, stream_index,
                encode_stream_conf[output_index].video_bitrate);
            if_err_return(err);
        }

#ifdef __app_set_detailed_parameters__
        /********************************************/
        /* video stream: optional, set parameters, default value */
        /********************************************/
       if (output_index == 0) {
            //main stream

            err = G_pRecordControl->SetVideoStreamDimention(output_index, stream_index, DefaultMainVideoWidth, DefaultMainVideoHeight);//1080p
            if_err_return(err);

            err = G_pRecordControl->SetVideoStreamFramerate(output_index, stream_index, 90000, 3003);//29.97fps
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamBitrate(output_index, stream_index, 8000000);//8M
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamLowdelay(output_index, stream_index, 0);//have B picture
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamEntropyType(output_index, stream_index, IParameters::EntropyType_H264_CABAC);//default CABAC
            if_err_return(err);
        } else {
            //sub stream
            err = G_pRecordControl->SetVideoStreamDimention(output_index, stream_index, DefaultPreviewCWidth, DefaultPreviewCHeight);// 1/4 D1
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamFramerate(output_index, stream_index, 90000, 3003);//29.97fps
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamBitrate(output_index, stream_index, 2000000);// 2M
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamLowdelay(output_index, stream_index, 0);//have B picture
            if_err_return(err);
            err = G_pRecordControl->SetVideoStreamEntropyType(output_index, stream_index, IParameters::EntropyType_H264_CABAC);//default CABAC
            if_err_return(err);
        }
#endif
        //TODO, enable_wsd  change the default resolution
        //     streaming-only,change default bitrate
        if (output_index == 0){
            if(enable_wsd){
                err = G_pRecordControl->SetVideoStreamDimention(output_index, stream_index, 1280,720);
                if_err_return(err);
            }
	     if(streaming_only){
                err = G_pRecordControl->SetVideoStreamBitrate(output_index, stream_index, streaming_bitrate);
                if_err_return(err);
                printf("set resution 720p and set bitrate %d\n",streaming_bitrate);
            }
        }

        //add audio stream
        err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_Audio, audio_format[output_index]);
        if_err_return(err);

#ifdef __app_set_detailed_parameters__
        /********************************************/
        /* audio stream: optional, set parameters, default value */
        /********************************************/
        err = G_pRecordControl->SetAudioStreamSampleFormat(output_index, stream_index, IParameters::SampleFMT_S16);//S16
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamBitrate(output_index, stream_index, 128000);//128 kbit/s
        if_err_return(err);
#endif
        err = G_pRecordControl->SetAudioStreamChannelNumber(output_index, stream_index, channels);// 2 channels
        if_err_return(err);
        //android need set to 44.1khz
        err = G_pRecordControl->SetAudioStreamSampleRate(output_index, stream_index, sample_rate);
        if_err_return(err);

        if (enable_pridata_test) {
            //add private stream
            err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_PrivateData, IParameters::StreamFormat_PrivateData);
        }

    }

    //setup streaming related
    if (rtsp_enabled) {
        G_pStreamingControl->AddStreamingServer(rtsp_server_index, IParameters::StreammingServerType_RTSP, IParameters::StreammingServerMode_MulticastSetAddr);
        //main stream
        if (rtsp_output_index_mask & 0x1) {
            if (rec_stream_mask & 0b10)
                G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 0);//video
            if (rec_stream_mask & 0b01)
                G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 1);//audio
        }
        //second stream
        if(total_muxer_number > 1){
            if (rtsp_output_index_mask & 0x2) {
                if (rec_stream_mask & 0b10)
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 0);//video
                if (rec_stream_mask & 0b01)
                    G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 1);//audio
            }
        }
    }

    if ((G_pRecordControl->SetAppMsgCallback(ProcessAMFMsg, NULL)) != ME_OK) {
        AM_ERROR("SetAppMsgCallback failed\n");
        return -1;
    }

    if ((G_pRecordControl->StartRecord()) < 0) {
        printf("Engine StartRecord failed!\n");
        return -1;
    }

#ifdef __use_imgproc__
    if (auto_start_aaa) {
        printf("start trigger aaa.\n");
        err = G_pPreTreat->StartImgProc(camera_index);
        printf("trigger aaa done.\n");
    }
#endif

    IWsdClientObserver  *observer = NULL;
    if(rtsp_enabled && enable_wsd){
        AM_UINT rtsp_port;
        AM_S8 url[1024];
        G_pStreamingControl->GetStreamingPortUrl(0, rtsp_port, url,sizeof(url));
        WSDiscoveryService::GetInstance()->setRtspInfo(rtsp_port, (const char *)url);
        observer = new ClientObserver;
        WSDiscoveryService::GetInstance()->registerObserver(observer);
        WSDiscoveryService::GetInstance()->start();
        printf("WSDiscovery start OK, port %d, url %s\n",rtsp_port,url);
    }

    //create yuv capture thread
    if ((G_pYUVcapEvent = CEvent::Create()) == NULL) {
        AM_ERROR("Cannot create event\n");
        return -1;
    }
    if ((G_pYUVcapThread = CThread::Create("yuvcapturer", YUVCapThread, NULL)) == NULL) {
        AM_ERROR("Create YUV capture thread failed\n");
        return -1;
    }

#if PLATFORM_ANDROID
     if(enable_ldws){
        G_pLDWSEngine = new AmbaLdwsEngine((void*)LDWS_Event_Handler);
        if(G_pLDWSEngine){
            SYUVData yuvdata;
            yuvdata.buffer_id = 3;//preview c
            err = G_pRecordControl->CaptureYUV(NULL, &yuvdata);
            if(err != ME_OK){
                AM_ERROR("CaptureYUV!!\n");
            }
            AM_WARNING("CaptureYUV size %d x %d, pitch %d\n", yuvdata.image_width, yuvdata.image_height, yuvdata.image_step);
            yuvdata.type = eCSYVU420SP;
            err = G_pLDWSEngine->InitLDWS(yuvdata.image_width, yuvdata.image_height, yuvdata.image_step, (eLDWSColorSpace)yuvdata.type);
            if(err != ME_OK){
                AM_ERROR("InitLDWS!!\n");
            }

            //load LDWS config
            //Sldws_engine_config ldwsconfig;
            //LoadLdwsConfig(&ldwsconfig);
            //err = G_pLDWSEngine->ConfigEngine(&ldwsconfig);
            if(err != ME_OK){
                AM_ERROR("ConfigEngine!!\n");
            }
           err = G_pLDWSEngine->StartEngine((void*)G_pRecordControl);
           if(err != ME_OK){
                AM_ERROR("StartEngine!!failed!\n");
           }
           AM_WARNING("StartEngine done!\n");
        }
    }
#endif

    if (test_osd) {
        test_osd_running = 1;
        if (ETestOSDMode_CLUT == test_osd_mode) {
            pthread_create(&test_osd_thread, NULL, _test_osd_thread_clut, NULL);
        } else if (ETestOSDMode_RGBA == test_osd_mode) {
            pthread_create(&test_osd_thread, NULL, _test_osd_thread_argb, NULL);
        } else {
            test_osd = 0;
            test_osd_running = 0;
            AM_ERROR("BAD test osd mode %d\n", test_osd_mode);
        }
    }

    RunMainLoop();

    if (test_osd) {
        test_osd_running = 0;
        pthread_join(test_osd_thread, &p_jointhead);
    }

    G_pYUVcapEvent->Signal();
    G_pYUVcapThread->Delete();
    G_pYUVcapEvent->Delete();

    if(local_socket > 0){
        close(local_socket);
        local_socket = -1;
        msg_notification_mode = 0;
    }

    if (rtsp_enabled) {
        G_pStreamingControl->RemoveStreamingServer(rtsp_server_index);
        if(enable_wsd){
            WSDiscoveryService::GetInstance()->Delete();
            delete observer;
            free_client_list();
        }
    }

    if (G_pRecordControl) {
        G_pRecordControl->StopRecord();
        printf("StopRecord okay!\n");

#ifdef __use_imgproc__
        //exit before enter idle
        if (G_pPreTreat) {
            printf("Start G_pPreTreat->StopImgProc().\n");
            G_pPreTreat->StopImgProc();
            printf("Start G_pPreTreat->Delete().\n");
            G_pPreTreat->Delete();
            printf("Start G_pPreTreat->Delete() done.\n");
            G_pPreTreat = NULL;
        }
#endif

        //app need not call ExitPreview, the ExitPreview is handled By camara
        if (rec_stream_mask & 0b10) {
            G_pRecordControl->ExitPreview();
            printf("ExitPreview okay!\n");
        }

        G_pRecordControl->Delete();
        G_pRecordControl = NULL;
    }

    AMF_Terminate();

#ifdef __use_imgproc__
    if (iav_fd >= 0) {
        printf("Start close fd %d.\n", iav_fd);
        close(iav_fd);
        printf("close fd %d done.\n", iav_fd);
        iav_fd = -1;
    }
#endif

    return 0;
}


