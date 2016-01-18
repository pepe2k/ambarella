
/**
 * test_mdec.cpp
 *
 * History:
 *    2011/12/09 - [GangLiu] created file
 *    2012/4/5 - [Qingxiong Z] modify file
 *
 * Copyright (C) 2011-2013, Ambarella, Inc.
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
#include <getopt.h>
#include "general_header.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_param.h"
#include "am_mdec_if.h"
#include "am_record_if.h"
#include "am_mw.h"
#include "am_util.h"
#include "jpeg_encoder.h"
extern "C" {
#include <basetypes.h>
#include "ambas_vout.h"
}

#if PLATFORM_ANDROID
#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
#endif

#if PLATFORM_LINUX
#include "simple_audio_common.h"
#include "general_interface.h"
#include "simple_audio_sink.h"
#include "resample_filter.h"
#include "simple_audio_recorder.h"

#include "amf_rtspclient.h"
#endif

#undef AM_INFO
#undef AM_ERROR
#define AM_LOG(format, args...) \
    do{ \
        fprintf(stdout, format, ##args); \
    }while(0) \

#define AM_INFO(format, args...) AM_LOG("::: "format, ##args)
#define AM_ERROR(format, args...) AM_LOG("\n!!!(%s:%d) "format, __FILE__, __LINE__, ##args)

/*
enum ENGINE_GLOBAL_FLAG
{
    NO_FILL_PES_SSP_HEADER = 0x00001,
    NO_AUTO_RESAMPLE_AUDIO = 0x00004,
    NO_AUDIO_ON_GMF = 0x00008,
    LOOP_FOR_LOCAL_FILE = 0x00002,
    USING_FOR_NET_PB = 0x00010,
    NO_HD_WIN_NVR_PB = 0x00100,
    NVR_SHOW_ON_LCD = 0x00400, //show on lcd
    SWITCH_JUST_FOR_WIN_ARM = 0x01000, //switchbyarm, no perform cmd, no wait anything and change win immetealy
    SWITCH_JUST_FOR_WIN_ARM2 = 0x02000, //switchbyarm2, no perform cmd, wait stream ready to change win.
    SWITCH_JUST_FOR_WIN_ARM3 = 0x04000, //switchbyarm3, no perform cmd, map sd to hd, and then run hd
    SWITCH_ARM_SEEK_FEED_DSP_DEBUG = 0x10000, //switchbydsp, perform cmd, use sealess
    SWITCH_ARM_SEEK_FEED_DSP_DEBUG2 = 0x20000,//switchbydsp2, perform win-chang-cmd, use nosmealess
    SAVE_ALL_STREAMS = 0x00002,
    DEBUG_100MS_ON_07_23 = 0x100000,
    DEBUG_100MS_ON_07_23_2 = 0x200000,
    NO_QUERY_ANY_FROM_DSP = 0x400000,
    MULTI_STREAMING_SETS = 0x1000000,
};

enum SOURCE_FLAG
{
    SOURCE_ENABLE_AUDIO = 0x00001,
    SOURCE_ENABLE_VIDEO = 0x00010,
    SOURCE_FULL_HD = 0x00100,
    SOURCE_SAVE_INFO = 0x01000,

    SOURCE_EDIT_REPLACE = 0x10000,
    SOURCE_EDIT_PLAY = 0x20000,
    SOURCE_EDIT_ADDBACKGROUND = 0x40000,
};
*/
//G variable
#define	NO_ARG		0
#define	HAS_ARG		1
#define MAX_SOURCE_NAME_LEN 256
#define MAX_SOURCE_NUM 32

IMDecControl *G_pMDecControl = NULL;
JpegEncoder *G_pJpegEnc = NULL;

//rtsp audio streaming
static IRecordControl2 *G_pRecordControl = NULL;
static IStreamingControl* G_pStreamingControl = NULL;
static int g_audio_rtsp_stream_state = 0;// 0:disable 1:enable
static int g_tilemode = 5;// 0
static int g_decoder_cap = DECODER_CAP_DSP;

#define if_err_return(err) do { \
    AM_ASSERT(err == ME_OK); \
    if (err != ME_OK) { \
        AM_ERROR("[rectest] error: file %s, line %d, ret %d.\n", __FILE__, __LINE__, err); \
        return; \
    } \
} while (0)

#if PLATFORM_LINUX
#include "jpeg_decoder.h"
JpegDecoder *G_pJpegDec = NULL;
#endif
static bool g_dsp_idle = false;
typedef struct char_vector
{
    char name[MAX_SOURCE_NAME_LEN];
}char_vector;
char_vector g_source[MAX_SOURCE_NUM] = {{{0}}};
int g_source_parse_hd;
int g_source_parse_group;
char_vector g_source_save[MAX_SOURCE_NUM] = {{{0}}};

int g_mdec_num;
int g_source_num = 0;
int g_sd_source_index;
bool g_quit = false;
int g_audio_index;
int g_select_hd;

int g_source_flag[MAX_SOURCE_NUM];
int g_source_group[MAX_SOURCE_NUM];
int g_global_flag = 0;

int g_dsp_max_frm_num = 7;
int g_net_buffer_begin_num = 0;
int g_dsp_pre_buffer_len = -1;
int g_nvr_playback_mode = -1;

//auto separate file
int g_auto_savefile_duration = 0;
int g_auto_savefile_maxfilecount= 0;
static int g_display_layout = 0;

//dual vout default
static int g_vout_mask = 3;

//jpeg capture
int g_jpeg_capture_index = 0;
//repeat
//int g_last_cmd;
//int g_last_cmd_par[16];

//transcode
bool enable_transcode = false;
bool en_dump_file = false;
AM_UINT enc_width = 720;
AM_UINT enc_height = 480;
AM_UINT enc_bitrate = 1000000;//default: 1M
bool en_transcode2cloud = false;
char transcode2cloud_url[128] = "";
char transcode_file_name[128] = "";

static int g_enable_loader = 0;
char m3u8[32] = "amba.m3u8";
char host[32] = "ddrddr";
char path[32] = ".";
static int g_stream_count_in_m3u8 = 4;

//
static int g_enable_wsd = 0;

#if PLATFORM_LINUX
// audio recorder
CSimpleAudioRecorder* g_audio_recorder = NULL;
#endif
//playback zoom
#define DINPUT_WIN_STEP 20

typedef struct sZoomInfo {
    int video_width;
    int video_height;
    unsigned short input_w;
    unsigned short input_h;
    unsigned short center_x;
    unsigned short center_y;
}TZoomInfo;
static TZoomInfo g_aZoomInfo[16];
//-----------------------------
//
//
struct hint_s {
	const char *arg;
	const char *str;
};
static const struct hint_s hint[] = {
	//system state
	{"[ ]", "\t\tShow this help txt."},
	{"[Dsp Num]", "\t\tThe num of Dsp instances you request."},
	{"[Dsp MaxFrame Num]", "\t\tSet the max frame buffer used for each Dsp Instances."},
	{"[Dsp PreBuffer Num]", "\t\tUsed to smooth the network jitter, should less then maxframe -5."},
	{"[MW PreBuffer Num]", "\t\tA mw prebuffer used to smooth network jitter."},
	{"[Stream Name]", "\t\tAdd a new source to play."},
	{"[HD Index]", "\t\tIndicate this is a hd source, corresponding to the Index sd source."},
	{"[ ]", "\t\tRun mdectest on no-hd case, there will have no hd window on background."},
	{"[Vout Mask ]", "\t\tSet vout mask: LCD: bit0, HDMI: bit1."},
	{"[ ]", "\t\tUsed for local file playback, will playback loop auto."},
	{"[0/1]", "\t\tSet playback mode, used to deside max video width and height."},
	{"[ ]", "\t\tRun mdectest for multiset mode, you should add all the source on cmd line."},
	{"[Global Flag]", "\t\tInput A Global Flag."},
	{"[Layout category]", "\t\tIndicate a layout category to use."},

	{"[ ]", "\t\tDonot fill pts info to Dsp. Debug option."},
	{"[ ]", "\t\tDonot query info from driver too frequently."},
	{"[ ]", "\t\tDisable all the audio channel."},
	{"[ ]", "\t\tSet switch mechanism, will try to use seamless switch mode."},
	{"[ ]", "\t\tSet swtich mechanism, will use no-seamless switch mode."},
	{"[ ]", "\t\tSave all the network stream."},
	{"[ ]", "\t\tDonot resample audio to 48K, 2channel."},
	{"[File Duration]", "\t\tSet the file duration for auto cut during recording."},
	{"[File Count]", "\t\tSet the file counts for auto cut during recording."},
	{"[ ]", "\t\tReceive Motion Events from camera side."},
	{"[resolution]", "\t\tEnable transcode, set the resolution."},
	{"[br]", "\t\tSet the bitrate(bps) of transcoding stream."},
	{"[filename]", "\t\tDump transcoded files."},
	{"[url]", "\t\tpush transcoded stream to cloud rtmp server."},

      //Runtime option hint.
       {"", ""},
	{"-w index", "\t\tSwitch from sd to hd on window index."},
	{"-b", "\t\tBack to multi sd playback."},
	{"-s index", "\t\tSelect the Specify stream's Audio."},
	{"-p index", "\t\tPause Specify stream, used on multi sd playback."},
	{"-r index", "\t\tResume Specify stream, used on multi sd playback."},
	{"--switch", "\t\tSwitch from one set to other, used only when --multiset apper."},
	{"--play index name", "\t\tPlayback new source(name), on window index. The old source will be remaind."},
	{"--done", "\t\tMust used after --play option, to indicate MW goto play the new source."},
	{"--replace index name", "\t\tPlayback new source(name), on window index. The old source will be replaced."},
	{"--back index order_num", "\t\tBack to old source(with order order_num) on window index."},
	{"--delete index", "\t\tDelete all the source on window index."},
	{"--relayout type", "\t\treset the layout. 0: normal, 1:teleconference."},
	{"--save index", "\t\tBegin saving the index source."},
	{"--saveq index", "\t\tStop the save of index source"},
	{"--stopdecoding", "\t\tStop DSP, udec will enter IDLE"},
	{"--restartdecoding", "\t\tRestart DSP, udec will run again"},
	{"--jpegdec jpeg_file", "\t\tDecode jpeg file"},
       {"-l ", "\t\tList devices, for auto connect to RTSP service."},
	{"--enable-wsd", "\t\t enable device auto discovery, for debug only now"},
	{"--audiortsp flag", "\t\t0:Enable audio encoding and stream out by rtsp. 1:Disable it"},
	{"--configwin winindex, offset_x, offset_y", "\t\tchange target window position"},
	{"--full winindex", "\t\tchange target window to full window"},
	{"--hide [1, 0]", "\t\tin teleconference layout, hide small windows,1.hide,0.show"},
	{"--bitrate [kbps]", "\t\tset transcode bitrate, set 1Mbps: --bitrate 1024"},
	{"--framerate [fps],[reduction factor]", "\t\tset transcode framerate, 30, 24, 15, 10..., default factor is 0, fps: 1 for 29.97."},
	{"--gop 1,30,2,0", "\t\tset transcode GOP: M, N, IDR interval, GOP structure."},
	{"--demandidr [0,1]", "\t\tdemand an IDR. 0 for next frame, 1 for next I frame."},
	{"--nvrrtmp [url]", "\t\tpush transcoded stream to cloud rtmp server."},
	{"--endnvrrtmp", "\t\tstop to push transcoded stream to cloud."},
	{"--recfile [name]", "\t\tDump transcoded files."},
	{"--stoprecfile", "\t\tStop to dump transcoded files."},
	{"--loader [1,0]", "\t\tEnable/disable loader"},
	{"--m3u8  [m3u8 name]", "\t\tSet the m3u8 file name"},
	{"--host [account login name]", "\t\tSet the cloud login name"},
	{"--path [directory]", "\t\tSet the path where to save file"},
	{"--count [count]", "\t\tSet the stream count in every m3u8"},
	{"--start-audio", "\t\tStart g711 audio stream"},
	{"--stop-audio", "\t\tStop g711 audio stream"},

         {"", ""},
};
static const char *short_options = "a:w:bls:p:r:hf:";
#define NUM_OPTIONS_BEGIN 256
enum num_short_options
{
    STREAM_FLAG_HD = NUM_OPTIONS_BEGIN,
    STREAM_SAVE_INFO,

    TOTAL_MDEC_NEED,
    MD_DSP_MAX_FRAME_NUM,
    MD_NET_BEGIN_BUFFER_NUM,
    MD_DSP_PRE_BUFFER_LEN,

    NET_PALY_BACK,
    NO_FILL_PTS_INFO,
    NO_HD_NVR_PB,
    ME_VOUT_MASK,
    SWITCH_JUST_FOR_WIN,
    SWITCH_JUST_FOR_WIN2,
    SWITCH_JUST_FOR_WIN3,
    SWITCH_DO_SEEK_FEED_TO_DSP,
    SWITCH_DO_SEEK_FEED_TO_DSP2,
    SAVE_ALL_STREAM,
    TEST_BY_CC_1,
    TEST_BY_CC_2,
    MD_NO_QUERY,
    MD_NO_AUTO_RESAMPLE_AUDIO,
    MD_NO_AUDIO_ON_GMF,
    MD_LOOP_FOR_LOCAL_FILE,
    MD_MULTI_STREAM_SET,

    ENABLE_MSG_PORT,

    MD_NVR_PLAYBACK_MODE,
    MD_SEPARATE_FILE_DURATION,
    MD_SEPARATE_FILE_MAXCOUNT,
    RUN_MOTION_DETECTE_RECEIVER,

    MD_DISPLAY_LAYOUT,
    ENABLE_WSD,
    CONFIG_AUDIO_RTSP_STREAMING,
    CONFIG_TILEMODE,
    CONFIG_RENDER,
    FOOL_MODE,
    FOOL_NO_FLUSH,
    MD_DECODER_CAP,

    ENABLE_TRANSCODE,
    TRANSCODE_BITRATE,
    TRANSCODE_DUMPFILE,
    TRANSCODE_CLOUD_RTMP,
    SWITCH_LOADER,
    FILE_M3U8_NAME,
    CLOUD_LOGIN_ACCOUNT,
    PATH_SAVING_FILE,
    STREAM_COUNT_IN_M3U8,
};
static struct option long_options[] = {
    {"help", NO_ARG, NULL, 'h'},
    {"num", HAS_ARG, NULL, TOTAL_MDEC_NEED},
    {"maxframe", HAS_ARG, NULL, MD_DSP_MAX_FRAME_NUM},
    {"prebuffer", HAS_ARG, NULL, MD_DSP_PRE_BUFFER_LEN},
    {"mwprebuffer", HAS_ARG, NULL, MD_NET_BEGIN_BUFFER_NUM},
    {"add", HAS_ARG, NULL, 'a'},
    {"hd", HAS_ARG, NULL, STREAM_FLAG_HD},
    {"nohd", NO_ARG, NULL, NO_HD_NVR_PB},
    {"voutmask", HAS_ARG, NULL, ME_VOUT_MASK},
    {"loop", NO_ARG, NULL, MD_LOOP_FOR_LOCAL_FILE},
    {"mode", HAS_ARG, NULL, MD_NVR_PLAYBACK_MODE},
    {"multiset", NO_ARG, NULL, MD_MULTI_STREAM_SET},
    {"flag", HAS_ARG, NULL, 'f'},
    {"layout", HAS_ARG, NULL, MD_DISPLAY_LAYOUT},

    {"nopts", NO_ARG, NULL, NO_FILL_PTS_INFO},
    {"noquery", NO_ARG, NULL, MD_NO_QUERY},
    {"noaudio", NO_ARG, NULL, MD_NO_AUDIO_ON_GMF},
    {"switchbydsp", NO_ARG, NULL, SWITCH_DO_SEEK_FEED_TO_DSP},
    {"switchbydsp2", NO_ARG, NULL, SWITCH_DO_SEEK_FEED_TO_DSP2},
    {"saveall", NO_ARG, NULL, SAVE_ALL_STREAM},
    {"noresample", NO_ARG, NULL, MD_NO_AUTO_RESAMPLE_AUDIO},
    {"duration", HAS_ARG, NULL, MD_SEPARATE_FILE_DURATION},
    {"maxfilecount", HAS_ARG, NULL, MD_SEPARATE_FILE_MAXCOUNT},
    {"md", NO_ARG, NULL, RUN_MOTION_DETECTE_RECEIVER},
    {"enc", HAS_ARG, NULL, ENABLE_TRANSCODE},
    {"br", HAS_ARG, NULL, TRANSCODE_BITRATE},
    {"recfile", HAS_ARG, NULL, TRANSCODE_DUMPFILE},
    {"nvrrtmp", HAS_ARG, NULL, TRANSCODE_CLOUD_RTMP},
    {"loader", HAS_ARG, NULL, SWITCH_LOADER},
    {"m3u8", HAS_ARG, NULL, FILE_M3U8_NAME},
    {"host", HAS_ARG, NULL, CLOUD_LOGIN_ACCOUNT},
    {"path", HAS_ARG, NULL, PATH_SAVING_FILE},
    {"count", HAS_ARG, NULL, STREAM_COUNT_IN_M3U8},

    //below is runtime option, can be removed
    {"switch", HAS_ARG, NULL, 'w'},
    {"back", NO_ARG, NULL, 'b'},
    {"list", NO_ARG, NULL, 'l'},
    {"select", HAS_ARG, NULL, 's'},
    {"pause", HAS_ARG, NULL, 'p'},
    {"resume", HAS_ARG, NULL, 'r'},
    {"save", HAS_ARG, NULL, STREAM_SAVE_INFO},

    //no used current, clear someday
    {"test1", NO_ARG, NULL, TEST_BY_CC_1},
    {"test2", NO_ARG, NULL, TEST_BY_CC_2},
    {"netstream", NO_ARG, NULL, NET_PALY_BACK},
    {"switchbyarm", NO_ARG, NULL, SWITCH_JUST_FOR_WIN},
    {"switchbyarm2", NO_ARG, NULL, SWITCH_JUST_FOR_WIN2},
    {"switchbyarm33", NO_ARG, NULL, SWITCH_JUST_FOR_WIN3},
    {"enablemsgport", NO_ARG, NULL, ENABLE_MSG_PORT},
    {"enable-wsd",NO_ARG, NULL, ENABLE_WSD},
    {"audiortsp", HAS_ARG, NULL,CONFIG_AUDIO_RTSP_STREAMING},
    {"tilemode", HAS_ARG, NULL,CONFIG_TILEMODE},
    {"render", HAS_ARG, NULL,CONFIG_RENDER},
    {"foolmode", NO_ARG, NULL,FOOL_MODE},
    {"noflush", NO_ARG, NULL,FOOL_NO_FLUSH},
    {"cap", HAS_ARG, NULL,MD_DECODER_CAP},
    {0, 0, 0, 0}
};

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
};

#define MAX_CHANNELS	20
#define DBasicScore (1<<8)
static unsigned long long performance_base = 1920*1080;
static unsigned int tot_performance_score = 0;
static unsigned int system_max_performance_score = DBasicScore * 2;//i1 1080p30 x 2
static unsigned int cur_pb_speed[MAX_CHANNELS] = {1};

static unsigned int tot_sd_number = 0;
static unsigned int tot_hd_number = 0;
static unsigned int current_play_hd = 0;

void usage(void)
{
    unsigned int i, k;

    printf("\n===========================================mdectest usage:==================================\n");
    for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
        if (hint[i].arg[0] == 0)
                        break;
        if (isalpha(long_options[i].val))
            printf("-%c ", long_options[i].val);
        else
            printf("   ");
        printf("--%-12s", long_options[i].name);
        if (hint[i].arg[0] != 0)
            printf(" %-20s", hint[i].arg);
        printf("%s\n", hint[i].str);
    }
    printf("\nTranscoding support resolutions below:\n");
    for(int i=0;i<sizeof(__encode_res)/sizeof(encode_resolution_s);i++){
        if(__encode_res[i].name[0]=='\0') continue;
        printf("\t%10s: %4u x %u,\n", ((__encode_res[i])).name, ((__encode_res[i])).width, ((__encode_res[i])).height);
    }
    //continue printf hint for run-time option
    k = i+1;
    printf("\n===========================================mdectest run time option:==================================\n");
    while(hint[k].arg[0] != 0)
    {
        printf("%-20s", hint[k].arg);
        printf("%s\n", hint[k].str);
        k++;
    }
    printf("\n");
}

//-----------------------------
//
//
void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    AM_INFO("AMF msg: %d\n", msg.code);

    if (msg.code == IMediaControl::MSG_PLAYER_EOS)
        AM_INFO("==== Playback end ====\n");
    if(msg.code == IMediaControl::MSG_EVENT_NVR_MOTION_DETECT_NOTIFY){
        AM_INFO("====Get Motion Info: %d ====\n", msg.p4);
    }
}

int GetMDInfo(MdecInfo& info)
{
    AM_ERR err;
    if ((err = G_pMDecControl->GetMdecInfo(info)) != ME_OK) {
        AM_ERROR("GetMDecInfo failed\n");
        return -1;
    }
    return 0;
}

char* trim(char *str)
{
    char *ptr = str;
    int i;

    // skip leading spaces
    for (; *ptr != '\0'; ptr++)
        if (*ptr != ' ')
            break;

    //skip (-a    xxx) to xxx
    for(; *ptr != '\0'; ptr++){
        if(*ptr != ' ')
            continue;
        for(; *ptr != '\0'; ptr++){
            if(*ptr == ' ')
                continue;
            break;
        }
        break;
    }

    // remove trailing blanks, tabs, newlines
    for (i = strlen(str) - 1; i >= 0; i--)
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
            break;

    str[i + 1] = '\0';
    return ptr;
}

int get_global_flag(const char* par)
{
    printf("get_global_flag %s\n", par);
    if(sscanf(par, "%d", &g_global_flag) != 1){
        AM_INFO("Spcify a global flag!\n");
        g_global_flag = 0;
    }
    return 0;
}

int get_decoder_cap(const char* par)
{
    if(!strcmp(par, "ffmpeg")){
        AM_INFO("Decoder cap: FFmpeg.\n");
        g_decoder_cap = DECODER_CAP_FFMPEG;
    }else if(!strcmp(par, "coreavc")){
        AM_INFO("Decoder cap: CoreAVC.\n");
        g_decoder_cap = DECODER_CAP_COREAVC;
    }else if(!strcmp(par, "dsp")){
        AM_INFO("Decoder cap: DSP.\n");
        g_decoder_cap = DECODER_CAP_DSP;
    }else{
        AM_INFO("Wrong decoder cap, use dsp by default.\n");
        g_decoder_cap = DECODER_CAP_DSP;
    }
    return 0;
}

int get_source(const char* par)
{
    if(g_source_num >= MAX_SOURCE_NUM){
        printf("get_source err: too many source\n");
        return -1;
    }
    if(strlen(par) >= MAX_SOURCE_NAME_LEN){
        printf("get_source err: too long source name\n");
        return -1;
    }
    //parse option --hd index
    int rval = 0;
    const char* ptr = par;
    const char* ptr_option = NULL;
    int filename_len = 0;
    int i, k;

    for(; *ptr != '\0'; ptr++){
        if(*ptr == ' ')
            break;
    }
    //AM_INFO("debug:%s1\n", ptr);
    filename_len = ptr - par;

    //AM_INFO("filename_len: %d\n", filename_len);
    while(*ptr != '\0'){
        //AM_INFO("DE:%d, %c, %d\n", *ptr, ' ', ' ');
        if(*ptr != ' ' && *ptr != '\t' && *ptr != '\n')
            break;
        ptr++;
    }
    //AM_INFO("debug:%s1\n", ptr);
    g_source_parse_hd = 0;
    g_source_parse_group = -1;
    int group = 0;

    if(*ptr != '\0'){
        //have trim or option
        if(*ptr != '-' || *(ptr+1) != '-' || *(ptr+2) != 'h' ||*(ptr+3) != 'd'){
            AM_INFO("Source option failed! Use --hd source!\n");
        }else{
            ptr += 4;
            for(; *ptr != '\0'; ptr++){
                if(*ptr != ' ' && *ptr != '\t' && *ptr != '\n')
                    break;
            }
            //AM_INFO("debug: %s1\n", ptr);
            if(sscanf(ptr, "%d", &group) == 1){
                g_source_parse_hd = 1;
                g_source_parse_group = group;
            }
        }
    }
    //parse option done
    if(g_source_parse_hd == 1){
        g_source_flag[g_source_num] |= SOURCE_FULL_HD;
        g_source_group[g_source_num] = g_source_parse_group;
        tot_hd_number ++;
    }else{
        g_source_flag[g_source_num] |= SOURCE_ENABLE_VIDEO;
        for(k = 0; k < g_sd_source_index; k++){
            if(g_source_group[k] == -1)
                break;
        }
        if(k < g_sd_source_index){
            //sd failed before
            rval = 1;
            g_source_group[g_source_num] = k;
        }else{
            //normal case
            g_source_group[g_source_num] = g_sd_source_index;
            g_sd_source_index++;
        }
        tot_sd_number ++;
    }

    strncpy(g_source[g_source_num].name, par, filename_len);
    g_source[g_source_num].name[filename_len] = '\0';
    printf("get_source %s, filename_len:%d, parsed:%s.\n", par, filename_len, g_source[g_source_num].name);
    return rval;
}

#define SWITCHCHK_LEFT_TIME_THRESHOLD_MS 5000//5 seconds
bool isSwitchValid()
{
    if(!G_pMDecControl){
        AM_ERROR("isSwitchValid failed, G_pMDecControl=%p.\n", G_pMDecControl);
        return false;
    }

    MdecInfo info;
    AM_ERR err=G_pMDecControl->GetMdecInfo(info);
    if(err != ME_OK){
        AM_ERROR("isSwitchValid, G_pMDecControl GetMdecInfo Failed, err=%d.\n", err);
        return false;
    }
    AM_INFO("isSwitchValid, G_pMDecControl GetMdecInfo done, check MdecUnits now.\n");

    MdecInfo::MdecUnitInfo* curInfo = NULL;
    for(int MdecUnitIdx = 0; MdecUnitIdx < 32; MdecUnitIdx++){//TODO: hard code now, should sync with MdecUnitInfo unitInfo[x] of am_mdec_if.h
        curInfo = &(info.unitInfo[MdecUnitIdx]);
        if(AM_TRUE == curInfo->isUsed &&
           AM_FALSE == curInfo->isNet)
        {
            if((curInfo->progress)+SWITCHCHK_LEFT_TIME_THRESHOLD_MS>=curInfo->length)
            {
                AM_INFO("isSwitchValid, MdecUnit %d len=%llu MS, pos=%llu MS, disable switch.\n",
                       MdecUnitIdx, curInfo->length, curInfo->progress);
                return false;
            }
            else
            {
//                AM_INFO("isSwitchValid, MdecUnit %d len=%llu MS, pos=%llu MS, go on.\n",
//                       MdecUnitIdx, curInfo->length, curInfo->curPts/90);
            }
        }
        else
        {
//            AM_INFO("isSwitchValid, MdecUnit %d is not used or be net stream, ignore it.\n", MdecUnitIdx);
        }
    }

//    AM_INFO("isSwitchValid, MdecUnits all checked, enable switch.\n");
    return true;
}

void DoDump(char* buffer)
{
    //AM_INFO("====================BEGING DUMP=============\n");
    char* ptr1, *ptr2;
    char chver[120] = {0};
    int i = 0, parindex;
    int flag =0;
    //d ff
    ptr2 = ptr1 = trim(buffer); //to 1
    if(*ptr1 == '\0'){
        G_pMDecControl->Dump(0);
        return;
    }
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    //compare chver to detect flag
    if(chver[0] == 'f' && chver[1] == 'f'){
        flag |= 1;
    }

    G_pMDecControl->Dump(flag);
    AM_INFO("DoDump Done\n");
}


void DoStop(AM_INT index)
{
    AM_ERR err;
    MdecInfo info;
    if(index > 3)
        return;
    if (GetMDInfo(info) < 0)
        return;

    //G_pMDecControl->StopDecoder(index);
    return;
}

void DoResumePlay(AM_INT index)
{
    AM_INFO("DoResumePlay:%d\n", index);
    AM_ERR err;
    err = G_pMDecControl->ResumePlay(index);
    if(err != ME_OK){
        printf("ResumePlay Source %d On-the-fly Failed\n", index);
    }
}

void DoPausePlay(AM_INT index)
{
    AM_INFO("DoPausePlay:%d\n", index);
    AM_ERR err;
    err = G_pMDecControl->PausePlay(index);
    if(err != ME_OK){
        printf("PausePlay Source %d On-the-fly Failed\n", index);
    }
}

void DoStepPlay(AM_INT index)
{
    AM_INFO("DoStepPlay:%d\n", index);
    AM_ERR err;
    err = G_pMDecControl->StepPlay(index);
    if(err != ME_OK){
        printf("StepPlay Source %d On-the-fly Failed\n", index);
    }
}

void DoAddSource()
{
    printf("DoAddSource");
    AM_ERR err;
    //AM_INT flag = (g_source_parse_hd == 1) ? SOURCE_FULL_HD : 0;
    err = G_pMDecControl->AddSource(g_source[g_source_num -1].name, g_source_group[g_source_num -1], g_source_flag[g_source_num -1]);
    if(err != ME_OK){
        printf("\n=======>Add Source %s On-the-fly Failed\n\n", g_source[g_source_num -1].name);
        //need mark this failed sd source
        g_source_group[g_source_num -1] = -1;
        g_source_num--;
    }
}

void DoBackToNVR()
{
    AM_INFO("DoBackToNVR\n");
    if(!isSwitchValid())
    {
        printf("Reverct to NVR On-the-fly Failed, switch disabled.\n");
        return;
    }
    AM_ERR err;
    err = G_pMDecControl->AutoSwitch(-1);
    if(err != ME_OK){
        printf("Reverct to NVR On-the-fly Failed\n");
    }
}

void DoAutoSwitch()
{
    AM_INFO("DoAutoSwitch\n");
    if(!isSwitchValid())
    {
        printf("Select HD Source %d On-the-fly Failed, switch disabled.\n", g_select_hd);
        return;
    }
    AM_ERR err;
    err = G_pMDecControl->AutoSwitch(g_select_hd);
    if(err != ME_OK){
        printf("Select HD Source %d On-the-fly Failed\n", g_select_hd);
    }
}

void DoSwitchSet()
{
    AM_INFO("DoSwitchSet\n");
    if(!(g_global_flag & MULTI_STREAMING_SETS)){
        printf("Use, --multiset\n");
        return;
    }

    AM_ERR err;
    err = G_pMDecControl->SwitchSet(-1);//auto select set
    if(err != ME_OK){
        printf("SwitchSet On-the-fly Failed\n");
    }
}

void DoSelectAudio()
{
    AM_INFO("DoSelectAudio\n");
    AM_ERR err;
    err = G_pMDecControl->SelectAudio(g_audio_index);
    if(err != ME_OK){
        printf("Select Audio Source %d On-the-fly Failed\n", g_audio_index);
    }
}

void DoDeleteSource(char* buffer)
{
    char* ptr1, *ptr2;
    char chver[120] = {0};
    int i = 0, parindex, order, flag = 0;
    AM_ERR err;

    //--delete 0
    ptr2 = ptr1 = trim(buffer); //to 1
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    if(sscanf(chver, "%d ", &parindex) != 1){
        AM_ERROR("Use '--delete index ' to delete play stream!\n");
        return;
    }
    AM_INFO("DoDeleteSource: index:%d\n", parindex);
    err = G_pMDecControl->DeleteSource(parindex, 0);
    //err = ME_OK;
    if(err != ME_OK){
        printf("Delete Source %d On-the-fly Failed\n", parindex);
    }
    AM_INFO("DoDeleteSource Done\n");
}

void DoBackSource(char* buffer)
{
    char* ptr1, *ptr2;
    char chver[120] = {0};
    int i = 0, parindex, order, flag = 0;
    AM_ERR err;

    //--back 1 0 hd
    ptr2 = ptr1 = trim(buffer); //to 1
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    if(*ptr1 == '\0'){
        AM_ERROR("Use '--back index orderlevel [hd]' to back play stream!\n");
        return;
    }
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    if(sscanf(chver, "%d ", &parindex) != 1){
        AM_ERROR("Use '--back index orderlevel [hd]' to back play stream!\n");
        return;
    }

    for (; *ptr1 != '\0'; ptr1++)
        if (*ptr1 != ' ' && *ptr1 != '\t')
            break;
    ptr2 = ptr1;
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    chver[i] = '\0';
    if(sscanf(chver, "%d", &order) != 1){
        AM_ERROR("Use '--back index orderlevel [hd]' to back play stream!\n");
        return;
    }
    AM_INFO("DoBackSource: index:%d, order:%d\n", parindex, order);
    for (; *ptr1 != '\0'; ptr1++)
        if (*ptr1 != ' ' && *ptr1 != '\t')
            break;
    //ptr1 to hd or to end
    if(*ptr1 == '\0'){
        AM_INFO("Back a sd stream!\n");
        flag = 0;
    }else{
        if(*ptr1 != 'h' || *(ptr1+1) != 'd'){
            AM_ERROR("Use '--back index orderlevel [hd]' to back play stream!\n");
            return;
        }
        AM_INFO("Back a hd stream!\n");
        flag |= SOURCE_FULL_HD;
    }
    err = G_pMDecControl->BackSource(parindex, order, flag);
    //err = ME_OK;
    if(err != ME_OK){
        printf("Back Source %d On-the-fly Failed\n", parindex);
    }
    AM_INFO("DoBackSource Done\n");
}

void DoEditSource(char* buffer, int flag)
{
    char* ptr1, *ptr2;
    char chver[120] = {0};
    int i = 0, parindex;
    AM_ERR err;

    //--play 1 filename hd
    //--replace 1 filename
    ptr2 = ptr1 = trim(buffer); //to 1
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    if(*ptr1 == '\0'){
        AM_ERROR("Use '--play(or --replace) index filename [hd]' to edit stream!\n");
        return;
    }
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    if(sscanf(chver, "%d ", &parindex) != 1){
        AM_ERROR("Use '--play(or --replace) index filename [hd]' to edit stream!\n");
        return;
    }
    //handle filename
    for (; *ptr1 != '\0'; ptr1++)
        if (*ptr1 != ' ' && *ptr1 != '\t')
            break;
    //ptr1 to filename
    ptr2 = ptr1;
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    chver[i] = '\0';
    AM_INFO("Edit Source, index:%d, filename:%s.\n", parindex, chver);

    for (; *ptr1 != '\0'; ptr1++)
        if (*ptr1 != ' ' && *ptr1 != '\t')
            break;
    //ptr1 to hd or to end
    if(*ptr1 == '\0'){
        AM_INFO("Edit a sd stream!\n");
        flag |= SOURCE_ENABLE_VIDEO;
    }else{
        if(*ptr1 != 'h' || *(ptr1+1) != 'd'){
            AM_ERROR("Use '--play(or --replace) index filename [hd]' to edit stream!\n");
            return;
        }
        AM_INFO("Edit a hd stream!\n");
        flag |= SOURCE_FULL_HD;
    }

    err = G_pMDecControl->EditSource(parindex, chver, flag);
    //err = ME_OK;
    if(err != ME_OK){
        printf("Edit Source %d On-the-fly Failed\n", parindex);
    }
    AM_INFO("DoEditSource Done\n");
}

void DoParseSaveInfo(char* buffer)
{
    char* ptr1, *ptr2;
    char chver[120] = {0};
    int i = 0;
    int parindex;
    AM_ERR err;
    //--save 1 filename
    ptr2 = ptr1 = trim(buffer); //to 1
    for(; *ptr1 != ' ' && *ptr1 != '\t' && *ptr1 != '\0';)
        ptr1++;
    if(*ptr1 == '\0'){
        AM_ERROR("Use '--save index save_filename' to save stream!\n");
        return;
    }
    for(i = 0; ptr2 < ptr1; i++, ptr2++)
        chver[i] = *ptr2;
    if(sscanf(chver, "%d", &parindex) != 1){
        AM_ERROR("Use '--save index save_filename' to save stream!\n");
        return;
    }
    for (; *ptr1 != '\0'; ptr1++)
        if (*ptr1 != ' ' && *ptr1 != '\t')
            break;
    strncpy(chver, ptr1, sizeof(chver));
    AM_INFO("Handle Save Info Done, save %d, save Name:%s\n", parindex, chver);

    err = G_pMDecControl->SaveSource(parindex, chver, 0);
    if(err != ME_OK){
        printf("Save Source %d On-the-fly Failed\n", parindex);
    }
    AM_INFO("DoParseSaveInfo Done\n");
}

void DoParseStopSave(char* buffer)
{
    int par;
    AM_ERR err;
    if(sscanf(trim(buffer), "%d", &par) != 1){
        AM_ERROR("Use '--saveq index' to stop save index stream!\n");
        return;
    }
    AM_INFO("Handle stop save info :%d\n", par);
    err = G_pMDecControl->StopSaveSource(par, 0);
    if(err != ME_OK){
        printf("Stop Save Source %d On-the-fly Failed\n", par);
    }
    AM_INFO("DoParseStopSave Done\n");
}

void DoChangeSavingTimeDuration(char* buffer)
{
    int par;
    AM_ERR err;
    if(sscanf(trim(buffer), "%d", &par) != 1){
        AM_ERROR("Use '--duration file_duration' to change saving time duration!\n");
        return;
    }
    AM_INFO("dynamically change saving time duration: %d\n", par);
    err = G_pMDecControl->ChangeSavingTimeDuration(par);
    if(err != ME_OK){
        AM_ERROR("change saving time duration %d On-the-fly Failed\n", par);
    }else{
        //update
        g_auto_savefile_duration = par;
    }
    AM_INFO("DoChangeSavingTimeDuration Done\n");
}

void DoSwitchCycle(char* buffer)
{
    AM_UINT interval=0;
    char input_buffer[128] = {0};
    if(NULL==buffer) {
        AM_ERROR("buffer=%p invalid.\n",buffer);
        return;
    }
    if(sscanf(trim(buffer), "%d", &interval) != 1){
            interval = 3;
    }
    AM_INFO("start window 0 SD/HD switch cycle, interval %u S, you can input ANY key to stop it...\n", interval);
    interval *= 1000000;//to US
    AM_INT cnt=-1;
    AM_INT cycle=0;
    AM_INT fd = open("/dev/tty", O_RDONLY|O_NONBLOCK);
    if(fd<0) {
        perror("open /dev/tty");
        return;
    }
    do{
            if(!isSwitchValid())
            {
                AM_INFO("switch cycle Failed, switch disabled.\n");
                break;
            }
            AM_ERR err;
            cnt++;
            cycle = cnt/2+1;
            if(0==cnt%2)
            {
                AM_INFO("SD->HD %d <--\n", cycle);
                err = G_pMDecControl->AutoSwitch(0);
                if(err != ME_OK){
                    AM_INFO("SD->HD %d failed.\n",  cycle);
                    break;
                }
                AM_INFO("SD->HD %d -->\n", cycle);
            }
            else
            {
                AM_INFO("HD->SD %d <--\n", cycle);
                err = G_pMDecControl->AutoSwitch(-1);
                if(err != ME_OK){
                    AM_INFO("HD->SD %d failed.\n", cycle);
                    break;
                }
                AM_INFO("HD->SD %d -->\n", cycle);
            }
            usleep(interval);
            if (read(fd, input_buffer, sizeof(input_buffer)) < 0)
                continue;
            else
                break;
     }while(1);
    if(fd)
    {
        close(fd);
        fd=0;
    }
    AM_INFO("stop window 0 SD/HD switch cycle.\n");
}

void DoCaptureCycle(char* buffer)
{
    AM_UINT interval=0;
    char input_buffer[128] = {0};
    MdecInfo info;
    if(NULL==buffer) {
        AM_ERROR("buffer=%p invalid.\n",buffer);
        return;
    }
    if(sscanf(trim(buffer), "%d", &interval) != 1){
        interval = 300;
    }
    AM_INFO("start window 0 capture files cycle, interval %u MS, you can input ANY key to stop it...\n", interval);
    interval *= 1000;//to US
    AM_INT cycle=0;
    AM_INT fd = open("/dev/tty", O_RDONLY|O_NONBLOCK);
    if(fd<0) {
        perror("open /dev/tty");
        return;
    }
    do{
            AM_ERR err;
            cycle++;
            AM_INFO("capture %d <--\n", cycle);
            {
                int video_w;
                int video_h;
                SCaptureJpegInfo param;
                G_pMDecControl->GetStreamInfo(0, &video_w, &video_h);
                G_pMDecControl->GetMdecInfo(info);
                if(G_pJpegEnc == NULL){
                    AM_INFO("first time to capture jpeg, creat jpegencoder\n");
                    G_pJpegEnc = new JpegEncoder(info.nvrIavFd);
                }
                param.vout = 1;
                if(info.isNvr){
                    param.dec_id = 0;
                }else{
                    param.dec_id = g_mdec_num -1;
                }
                param.file_index = g_jpeg_capture_index;
                param.video_width = video_w;
                param.video_height = video_h;
                param.quality = 50;
                param.enlog = true;
                G_pJpegEnc->SetParams(&param);
                G_pJpegEnc->PlaybackCapture();
                g_jpeg_capture_index++;
            }
            AM_INFO("capture %d -->\n", cycle);
            usleep(interval);
            if (read(fd, input_buffer, sizeof(input_buffer)) < 0)
                continue;
            else
                break;
     }while(1);
    if(fd)
    {
        close(fd);
        fd=0;
    }
    AM_INFO("stop window 0 capture files cycle.\n");
}

void DoReLayout(char* buffer)
{
    AM_INT layout = 0;
    AM_INT target_win = 0;//default
    char input_buffer[128] = {0};
    if(NULL == buffer) {
        AM_ERROR("buffer=%p invalid.\n",buffer);
        return;
    }

    if(sscanf(trim(buffer), "%d %d", &layout, &target_win) < 1){
        AM_ERROR("Use '--relayout index' to relayout window\n");
        return;
    }

    if (layout < 0 || layout >= LAYOUT_CNT) {
        //AM_ERROR("invalid layout index %d. Layout Range: %d~%d\n", IMDecControl::MD_LAYOUT_TABLE, IMDecControl::MD_LAYOUT_SINGLE_HD_FULLSCREEN);
        return ;
    }

    AM_ERR err;
    err = G_pMDecControl->ReLayout((LAYOUT_TYPE) layout, target_win);
    if (err != ME_OK) {
        AM_ERROR("ReLayout failure, err:%d layout:%d, win: %d\n", err, layout, target_win);
        return ;
    }
}

void DoDisconnectHw()
{
    AM_ERR err;
    if(g_dsp_idle == false){
        err = G_pMDecControl->DisconnectHw(0);
        if(err == ME_OK){
            g_dsp_idle = true;
        }
    }
    return;
}

void DoReconnectHw()
{
    AM_ERR err;
#if PLATFORM_LINUX
    if(G_pJpegDec){
        G_pJpegDec->Delete();
        G_pJpegDec = NULL;
    }
#endif
    if(g_dsp_idle == true){
        err = G_pMDecControl->ReconnectHw(0);
        if(err == ME_OK){
            g_dsp_idle = false;
        }
    }
    return;
}
#if PLATFORM_LINUX
void DoJpegDec(char* buffer)
{
    AM_INT ret;
    char jpeg_file[128] = {0};
    AM_INT bydsp = 0;
    AM_INT fbIndex = 1;

    sscanf(trim(buffer), "%s %d %d", jpeg_file, &bydsp, &fbIndex);
    AM_INFO("jpeg file %s, bydsp:%d, fbIndex:%d\n", jpeg_file, bydsp, fbIndex);

    if(G_pJpegDec == NULL){
        G_pJpegDec = JpegDecoder::Create(JpegDecoder::VOUT_HDMI, -1, (bydsp == 1), fbIndex);
        if(G_pJpegDec == NULL){
            AM_ERROR("creat jpegdecoder failed!\n");
            return;
        }
    }
    if(bydsp == 1 && (g_dsp_idle == false)){
        //dsp enter idle
        G_pMDecControl->DisconnectHw(0);
        g_dsp_idle = true;
    }
    G_pJpegDec->JpegDec(jpeg_file);
    return;
}
#endif
static void handle_audio_rtsp_streaming(AM_INT flag)
{
    AM_ERR err = ME_OK;
    AM_UINT rtsp_server_index = 0;

    if(flag == 0){
        //disable
        AM_ASSERT(G_pRecordControl);
        AM_ASSERT(G_pStreamingControl);
        G_pStreamingControl->RemoveStreamingServer(rtsp_server_index);
        G_pRecordControl->StopRecord();
        G_pRecordControl->Delete();
        G_pRecordControl = NULL;
        G_pStreamingControl = NULL;

        AM_INFO("disable audio rtsp streaming done!\n");
        g_audio_rtsp_stream_state = flag;
        return;
    }

    //enable
    AM_UINT stream_index;
    AM_UINT total_muxer_number = 1;
    AM_UINT no_saving_file = 1;

    AM_ASSERT(G_pRecordControl == NULL);
    if((G_pRecordControl = CreateRecordControl2(no_saving_file)) == NULL){
        AM_ERROR("Cannot create record control\n");
        return;
    }
    G_pStreamingControl = IStreamingControl::GetInterfaceFrom(G_pRecordControl);
    if(!G_pStreamingControl){
        AM_ERROR("Cannot get IStreamingControl interface!\n");
        return;
    }
    //set prperty
    G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_STREAM_ONLY_AUDIO, 0b01);

    err = G_pRecordControl->SetTotalOutputNumber(total_muxer_number);
    if(err != ME_OK){
        AM_ERROR("SetTotalOutputNumber!\n");
        return;
    }

    for(AM_UINT output_index = 0; output_index < total_muxer_number; output_index++){
        //setup ouput/muxer
        if(no_saving_file == 0){
            err = G_pRecordControl->SetupOutput(output_index, "stream_audio_only.ts", IParameters::MuxerContainer_AUTO);
        }
        if(err != ME_OK){
            AM_ERROR("SetupOutput!\n");
            return;
        }

        err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_Audio, IParameters::StreamFormat_Invalid/*aac default*/);
        if_err_return(err);
        //app set detailed_parameters
        err = G_pRecordControl->SetAudioStreamChannelNumber(output_index, stream_index, 2);// 2 channels
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamSampleFormat(output_index, stream_index, IParameters::SampleFMT_S16);//S16
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamBitrate(output_index, stream_index, 128000);//128 kbit/s
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamSampleRate(output_index, stream_index, 48000/*sample_rate*/);
        if_err_return(err);

        //setup streaming rtsp
        G_pStreamingControl->AddStreamingServer(rtsp_server_index, IParameters::StreammingServerType_RTSP, IParameters::StreammingServerMode_MulticastSetAddr);
        //only main stream
        G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 1);//audio

        if((G_pRecordControl->StartRecord()) < 0){
            AM_ERROR("Engine StartRecord failed!\n");
            return;
        }
        AM_INFO("enable audio rtsp streaming done!\n");
        g_audio_rtsp_stream_state = flag;
        return;
    }
    return;
}

void DoAudioRTSPStream(char* buffer)
{
    AM_ERR err = ME_OK;
    AM_INT flag;
    AM_UINT rtsp_server_index = 0;

    sscanf(trim(buffer), "%d", &flag);
    if(g_audio_rtsp_stream_state == flag){
        return;
    }
    handle_audio_rtsp_streaming(flag);
    return;
}

void DoConfigTargetWindow(char* buffer)
{
    AM_INT target;

    CParam par(4);
    par[0] = -1;
    par[1] = -1;
    par[2] = -1;
    par[3] = -1;

    if(sscanf(trim(buffer), "%d %d %d %d %d", &target, &par[0], &par[1], &par[2], &par[3]) < 1){
        AM_ERROR("use cmd '--configwin target [center_x center_y width height]' to config window position and size!\n");
        return;
    }

    AM_INFO("target window %d, center: %d %d, size %d %d\n", target, par[0], par[1], par[2], par[3]);
    G_pMDecControl->ConfigTargetWindow(target, par);

    return;
}

void DoHideSmallWindow(AM_BOOL en)
{
    AM_INT target = -1;

    CParam par(1);
    par[0] = en ? LAYOUT_TELE_HIDE_SMALL : LAYOUT_TELE_SHOW_SMALL;

    AM_INFO("DoHideSmallWindow %d\n", en);
    G_pMDecControl->ConfigTargetWindow(target, par);

    return;
}

void DoG711AudioStream(AM_BOOL bStart)
{
#if PLATFORM_LINUX
    AM_ERR err;
    if (bStart == AM_TRUE) {
        if (g_audio_recorder == NULL) {
            g_audio_recorder = CSimpleAudioRecorder::Create();
            if (!g_audio_recorder) {
                AM_ERROR("failed to create CSimpleAudioRecorder!\n");
                return;
            }
            SAudioInfo info;
            info.bEnableRTSP = AM_FALSE;
            strcpy((char*)info.cURL, "g711.back");
            info.bSave = AM_TRUE;
            strcpy((char*)info.cFileName, "./audio_g711");
            info.eFormateType = AUDIO_ALAW;
            info.uChannel = 1;
            info.uSampleRate = 48000;
            err = g_audio_recorder->ConfigMe(info);
            if (err != ME_OK) {
                AM_ERROR("config recorder failed!\n");
                g_audio_recorder->Delete();
                return;
            }

            err = g_audio_recorder->Start();
            if (err != ME_OK) {
                AM_ERROR("start recorder failed!\n");
                g_audio_recorder->Delete();
                return;
            }
        }
    } else {
        if (g_audio_recorder) {
            g_audio_recorder->Stop();
            g_audio_recorder->Delete();
            g_audio_recorder = NULL;
        }
    }
#endif
    AM_INFO("DoG711AudioStream [%s] done!\n", (bStart == AM_TRUE)? "start audio stream":"stop audio stream");
    return;
}
//////////////////////////////////////////////////
#include "ws_discovery.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include  <sys/param.h>
#include  <sys/ioctl.h>
#include <linux/if.h>
#include <fcntl.h>
#include <poll.h>

static int create_wsd_socket(char *path){
    int sock_fd, ret;
    socklen_t len;
    struct sockaddr_un addr;

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sock_fd == -1){
        return -1;
    }

    unlink(path);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    ret = bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1){
        close(sock_fd);
        return -1;
    }
    return sock_fd;
}
static int close_wsd_socket(int sock_fd){
    return close(sock_fd);
}
static int wsd_socket_sendto(int sock_fd,char *buf,int len,char *dst_path){
    struct sockaddr_un to_addr;
    socklen_t addr_len = sizeof(to_addr);
    memset(&to_addr, 0, sizeof(to_addr));
    to_addr.sun_family = AF_UNIX;
    strncpy(to_addr.sun_path, dst_path, sizeof(to_addr)-1);
    return sendto(sock_fd, buf,len, 0, (struct sockaddr*)&to_addr,addr_len);
}

static int wsd_socket_recv(int sock_fd,char *buf,int len){
    struct sockaddr_un from_addr;
    socklen_t addr_len = sizeof(from_addr);
    memset(&from_addr, 0, sizeof(from_addr));
    return recvfrom(sock_fd, buf, len, 0, (struct sockaddr*)&from_addr, &addr_len);
}

//define unix domain socket path
#if PLATFORM_ANDROID
    #define wsd_path_write "/data/data/pwsd_write"
    #define wsd_path_read "/data/data/pwsd_read"
#else
    #define wsd_path_write "/tmp/pwsd_write"
    #define wsd_path_read "/tmp/pwsd_read"
#endif

static int wsd_write_fd =-1;
static int wsd_read_fd = -1;

static int  wsd_socket_init(){
    wsd_write_fd = create_wsd_socket((char*)wsd_path_write);
    wsd_read_fd = create_wsd_socket((char*)wsd_path_read);
    return 0;
}
static int wsd_socket_close(){
    close(wsd_write_fd);
    wsd_write_fd = -1;
    close(wsd_read_fd);
    wsd_read_fd = -1;
    return 0;
}
static int wsd_write(char *buf,int len){
    return wsd_socket_sendto(wsd_write_fd,buf,len,(char*)wsd_path_read);
}

static int wsd_read(char *buf,int len){
    return wsd_socket_recv(wsd_read_fd,buf,len);
}

static void device_info(void *usr, char *uuid,char *ipaddr,char *stream_url){
    printf("\t device, uuid [%s], ipaddr[%s],stream_url[%s]\n",uuid,ipaddr,stream_url);
}
static void DoDisplayDevice(){
    //WSDiscoveryClient::GetInstance()->dumpDeviceList();
    printf("WSDiscoveryClient::getDeviceList\n");
    WSDiscoveryClient::GetInstance()->getDeviceList(device_info,NULL);
    printf("WSDiscoveryClient::getDeviceList END\n");
}

//for NVR + 4 cloudCameras(720p) case,
#define  RTSP_MAX_SOURCE_NUM 4
static int rtsp_source_num = 0;
struct rtsp_source_node_t{
    char uuid[64];
    int index;
    struct rtsp_source_node_t *next;
} * rtsp_source_list = NULL;
static pthread_mutex_t rtsp_source_mutex = PTHREAD_MUTEX_INITIALIZER;

static int get_index(int mask){
    int i;
    for(i = 0; i < RTSP_MAX_SOURCE_NUM;i++){
        if(!(mask & (1 << i)))
            return i;
    }
    return -1;
}
static int free_rtsp_source_list(){
    pthread_mutex_lock(&rtsp_source_mutex);
    while(rtsp_source_list){
        rtsp_source_node_t *next = rtsp_source_list->next;
        delete rtsp_source_list;
        rtsp_source_list = next;
    }
    pthread_mutex_unlock(&rtsp_source_mutex);
    return 0;
}

static int add_rtsp_source(char *uuid,int *index){
    int index_mask = 0;
    pthread_mutex_lock(&rtsp_source_mutex);
    rtsp_source_node_t *node = rtsp_source_list;
    while(node){
         if(!strcmp(node->uuid,uuid)){
             *index = node->index;
             pthread_mutex_unlock(&rtsp_source_mutex);
             return 1;//duplicate
         }
         index_mask |= 1 << node->index;
         node = node->next;
    }

    if(rtsp_source_num >= RTSP_MAX_SOURCE_NUM){
        AM_ERROR("Too many sources, now max 4 sources supported\n");
        pthread_mutex_unlock(&rtsp_source_mutex);
        return -1;
    }else{
        rtsp_source_node_t *node = new rtsp_source_node_t;
        node->index = get_index(index_mask);
        snprintf(node->uuid,sizeof(node->uuid),"%s",uuid);
        node->next = NULL;
        *index = node->index;
        if(rtsp_source_list){
            node->next = rtsp_source_list;
        }
        rtsp_source_list = node;
        ++rtsp_source_num;
    }
    pthread_mutex_unlock(&rtsp_source_mutex);
    return 0;
}

static int remove_rtsp_source(char *uuid,int *index){
    pthread_mutex_lock(&rtsp_source_mutex);
    rtsp_source_node_t *prev = NULL,*curr = rtsp_source_list;
    while(curr){
        if(!strcmp(curr->uuid,uuid)){
            if(prev){
                prev->next = curr->next;
            }else{
                rtsp_source_list = curr->next;
            }
            *index = curr->index;
            delete curr;
            --rtsp_source_num;
            pthread_mutex_unlock(&rtsp_source_mutex);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&rtsp_source_mutex);
    return -1;
}

class DeviceObserver:public IWsdObserver{
public:
    virtual void onDeviceChanged(char *uuid,char *ipaddr,char *stream_url){
        printf("DeviceObserver::onDeviceChanged,uuid[%s],ipaddr[%s],stream_url[%s]\n",uuid,ipaddr ? ipaddr:"NULL",stream_url ? stream_url:"NULL");
        if(ipaddr){
            int index;
            int ret = add_rtsp_source(uuid,&index);
            if(ret < 0){
                return;
            }
            if(!stream_url){
                return;//stream_url not got yet.
            }
            char command[2048];
            int len;
	     if(ret){
                //duplicate, services were disconnected and connected again.
                // deleteSource and then addSource
                len = snprintf(command,sizeof(command),"--delete %d\n",index);
                wsd_write(command, len);
	     }
            len = snprintf(command,sizeof(command),"--play %d %s\n",index,stream_url);
            wsd_write(command, len);
        }else{
            //delete source only
            int index;
            int ret = remove_rtsp_source(uuid,&index);
            if(ret < 0) return;
            char command[512];
            int len = snprintf(command,sizeof(command),"--delete %d\n",index);
            wsd_write(command, len);
        }
    }
};


static int get_input(char *buf,int len){
    int recv_len,n;
    struct pollfd p[2] = {{STDIN_FILENO, POLLIN, 0},{wsd_read_fd,POLLIN,0}};
    while(1){
        n = poll(p, 2, 100);
        if(n > 0){
            if (p[0].revents & POLLIN){
                return read(STDIN_FILENO, buf, len);
            }
            if(p[1].revents & POLLIN){
                return wsd_read(buf,len);
            }
        }else if(n < 0){
            if(errno == EINTR) continue;
            return -EIO;
        }else{
            return -ETIMEDOUT;
        }
    }
}

unsigned int max_possiable_speed(int channel_index)
{
    int i;
    int width, height;
    unsigned int ret = 1;

    AM_ASSERT(G_pMDecControl);
    if (!G_pMDecControl) {
        return 1;
    }

    if (current_play_hd) {
        // 1x's case
        G_pMDecControl->GetStreamInfo(channel_index, &width, &height);
        i = (unsigned int)((unsigned long long)width * (unsigned long long)height * DBasicScore / performance_base);
        if (i) {
            ret = system_max_performance_score/i;
            return ret;
        } else {
            AM_ERROR("invalid width %d, or height %d\n", width, height);
            return 1;
        }
    } else {
        if (channel_index >= g_sd_source_index) {
            AM_ERROR("BAD channel index %d, tot sd number %d\n", channel_index, g_sd_source_index);
            return 1;
        }
        tot_performance_score = 0;
        for (i = 0; i < g_sd_source_index; i ++) {
            if (i == channel_index) {
                continue;
            }
            G_pMDecControl->GetStreamInfo(i, &width, &height);
            tot_performance_score += (unsigned int)((unsigned long long)width * (unsigned long long)height * DBasicScore * cur_pb_speed[i] / performance_base);
        }

        G_pMDecControl->GetStreamInfo(channel_index, &width, &height);

        if (tot_performance_score < system_max_performance_score) {
            ret = (unsigned int)((unsigned long long)width * (unsigned long long)height * DBasicScore / performance_base);
            ret = (system_max_performance_score - tot_performance_score)/ret;
            return ret;
        } else {
            AM_ERROR("error, tot_performance_score %d >= system_max_performance_score %d\n", tot_performance_score, system_max_performance_score);
        }
    }

    return 1;
}

void clear_speed_settings(void)
{
    int i = 0;
    for (i = 0; i < MAX_CHANNELS; i ++) {
        cur_pb_speed[i] = 1;
    }
}

void RunMainLoop()
{
    char ch;
    char buffer_old[128] = {0};
    char buffer[128] = {0};
    MdecInfo info;
    static int flag_stdin = 0;
    int par;
    int rval;
    //
    char buffer_back[128] = {0};

    //playback zoom
    int render_id;
    unsigned short input_w, input_h, center_x, center_y;
    int video_width=0;
    int video_height=0;

    flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
    if(fcntl(STDIN_FILENO, F_SETFL, flag_stdin&(~O_NONBLOCK))== -1)
        AM_ERROR("stdin_fileno set error");

    while (1)
    {
        if(g_quit)
            break;
            /*
        GetMDInfo(info);

        if(info.state == IMDecControl::STATE_HAS_ERROR){
            AM_ERROR("======Play Error!======\n");
            return;
        }

        if(info.state == IMDecControl::STATE_COMPLETED){
           if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
                AM_ERROR("stdin_fileno set error");
            AM_INFO("======Play Completed!======\n");
            return;
        }

        //add sleep to avoid affecting the performance
        usleep(10000);
        */
        //AM_INFO("CCC1\n");
        memset(buffer, 0, sizeof(buffer));
#if 0
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0){
            AM_ERROR("read from stdin err, debug me:%s\n", buffer);
            continue;
        }
#else
        int recv_len;
        if((recv_len = get_input(buffer,sizeof(buffer))) < 0){
            if(recv_len != -ETIMEDOUT)
            {
                AM_ERROR("get_input err, debug me:%s\n", buffer);
            }
            continue;
        }
#endif
        ch = buffer[0];
        if(ch == '-'){
            ch = buffer[1];
        }
        if(ch == '\n' && buffer_back[0] != 0){
            AM_INFO("Repeat Cmd:%s", buffer_back);//buffer_back has a line change
            memcpy(buffer, buffer_back, sizeof(buffer));
            ch = buffer[0];
            if(ch == '-'){
                ch = buffer[1];
            }
        }else{
            memset(buffer_back, 0, sizeof(buffer_back));
            //AM_INFO("deb%sg\n", buffer_back);
            memcpy(buffer_back, buffer, sizeof(buffer));
        }
        //g_last_cmd = ch;
        switch (ch) {
        case 'q':    // exit
            g_quit = true;
            break;

        case ' ':    // pause
            buffer_old[0] = buffer[0];
            printf("DoPause\n");
            //DoPause();
            break;

        case 'r':
            if(sscanf(trim(buffer), "%d", &par) != 1){
                AM_INFO("Using -r simple to resume HD.\n");
                par = -1;
            }
            DoResumePlay(par);
            break;

        case 'p':
            if(sscanf(trim(buffer), "%d", &par) != 1){
                AM_INFO("Using -p simple to Pause HD.\n");
                par = -1;
            }
            DoPausePlay(par);
            break;

        case 'e':
            if(sscanf(trim(buffer), "%d", &par) != 1){
                AM_INFO("Using -e simple to step HD.\n");
                par = -1;
            }
            DoStepPlay(par);
            break;

        case 'a':
            rval = get_source(trim(buffer));
            if(rval < 0)
                break;
            if(rval == 0)
                g_source_num++;
            DoAddSource();
            break;

        case 'h':
            usage();
            break;

        case 's':
            if(sscanf(trim(buffer), "%d", &g_audio_index) != 1){
                AM_INFO("Select a Audio to playback!\n");
            }else{
                DoSelectAudio();
            }
            break;

        case 'd':
            DoDump(buffer);
            break;

        case 'w':
            if(sscanf(trim(buffer), "%d", &g_select_hd) != 1){
                AM_INFO("Select a HD Source to Full windows Play!\n");
            }else{
                DoAutoSwitch();
                clear_speed_settings();
                current_play_hd = 1;
            }
            break;

        case 'b':
            DoBackToNVR();
            clear_speed_settings();
            current_play_hd = 0;
            break;
        case 'l':
            DoDisplayDevice();
            break;
        case 't':
            if(sscanf(trim(buffer), "%d", &par) != 1){
                AM_INFO("Well your can set test flag.\n");
                par = -1;
            }
            G_pMDecControl->Test(par);
            break;

        case '-':
            //some misc cmdline
            if(buffer[2] == 's' && buffer[3] == 'a' && buffer[4] == 'v' && buffer[5] == 'e' && buffer[6] == 'q'){
                DoParseStopSave(buffer);
            }else if(buffer[2] == 's' && buffer[3] == 'a' && buffer[4] == 'v' && buffer[5] == 'e'){
                DoParseSaveInfo(buffer);
            }else if(buffer[2] == 'p' && buffer[3] == 'l' && buffer[4] == 'a' && buffer[5] == 'y'){
                DoEditSource(buffer, SOURCE_EDIT_PLAY);
            }else if(buffer[2] == 'd' && buffer[3] == 'o' && buffer[4] == 'n' && buffer[5] == 'e'){
                G_pMDecControl->EditSourceDone();
            }else if(buffer[2] == 'r' && buffer[3] == 'e' && buffer[4] == 'p' && buffer[5] == 'l' && buffer[6] == 'a' && buffer[7] == 'c' && buffer[8] == 'e'){
                DoEditSource(buffer, SOURCE_EDIT_REPLACE);
            }else if(buffer[2] == 'b' && buffer[3] == 'a' && buffer[4] == 'c' && buffer[5] == 'k'){
                DoBackSource(buffer);
            }else if(buffer[2] == 'd' && buffer[3] == 'e' && buffer[4] == 'l' && buffer[5] == 'e' && buffer[6] == 't' && buffer[7] == 'e'){
                DoDeleteSource(buffer);
            }
            //change saving file duration dynamically
            else if(strncmp("duration", buffer+2, 8) == 0){
                //--duration XXX
                DoChangeSavingTimeDuration(buffer);
            }
            else if(buffer[2] == 'c' && buffer[3] == 'y' && buffer[4] == 'c' && buffer[5] == 's' && buffer[6] == 'w')
            {
                DoSwitchCycle(buffer);
            }
            else if(buffer[2] == 'c' && buffer[3] == 'y' && buffer[4] == 'c' && buffer[5] == 'c' && buffer[6] == 'a')
            {
                DoCaptureCycle(buffer);
            }else if(buffer[2] == 's' && buffer[3] == 'w' && buffer[4] == 'i' && buffer[5] == 't' && buffer[6] == 'c' && buffer[7] == 'h'){
                DoSwitchSet();
            } else if (strncmp("relayout", buffer+2, 8) == 0) {
                //--layout xxx
                DoReLayout(buffer);
            }
            else if(strncmp("hide", buffer+2, 4) == 0){
                AM_INT hide;
                if(sscanf(trim(buffer), "%d", &hide) < 1){
                    AM_ERROR("Use '--hide 1/0' to hide small window\n");
                }
                DoHideSmallWindow(hide);
            }
            //transcode related
            else if(strncmp("bitrate", buffer+2, 7) == 0){
                AM_INT br;
                if(sscanf(trim(buffer), "%d", &br) < 1){
                    AM_ERROR("Use '--bitrate [kbps]' to set bitrate\n");
                }
                G_pMDecControl->SetTranscodeBitrate(br);
            }else if(strncmp("framerate", buffer+2, 9) == 0){
                AM_INT fps;
                AM_INT factor;
                if(sscanf(trim(buffer), "%d,%d", &fps, &factor) < 2){
                    AM_ERROR("Use '--framerate [fps],[reduction factor]' to set framerate, 1 for 29.97\n");
                }
                G_pMDecControl->SetTranscodeFramerate(fps, factor);
            }else if(strncmp("gop", buffer+2, 3) == 0){
                AM_INT m, n, interval, structure;
                if(sscanf(trim(buffer), "%d,%d,%d,%d", &m, &n, &interval, &structure) < 4){
                    AM_ERROR("Use '--gop m,n,interval,structure' to set GOP [%s]\n", trim(buffer));
                }
                G_pMDecControl->SetTranscodeGOP(m, n, interval, structure);
            }else if(strncmp("demandidr", buffer+2, 9) == 0){
                AM_INT idr;
                if(sscanf(trim(buffer), "%d", &idr) < 1){
                    AM_ERROR("Use '--demandidr [0,1]' to demandidr, 1 for next frame, 0 for next I frame.\n");
                }
                G_pMDecControl->TranscodeDemandIDR((idr==1) ? AM_TRUE : AM_FALSE);
            }
            //--stop, stop dsp
            else if(strncmp("stopdecoding", buffer+2, 12) == 0){
                DoDisconnectHw();
            }
            else if(strncmp("restartdecoding", buffer+2, 15) == 0){
                DoReconnectHw();
            }
            else if(strncmp("jpegdec", buffer+2, 7) == 0){
#if PLATFORM_LINUX
                DoJpegDec(buffer);
#endif
            }
            //enable audio encoding and stream out by rtsp
            else if(strncmp("audiortsp", buffer+2, 9) == 0){
                DoAudioRTSPStream(buffer);
            }
            //move target window
            else if(strncmp("configwin", buffer+2, 9) == 0){
                DoConfigTargetWindow(buffer);
            }
            else if(strncmp("full", buffer+2, 4) == 0){
                int target;
                if(sscanf(trim(buffer), "%d", &target) < 1){
                    AM_ERROR("use cmd '--full target !\n");
                }
                CParam par(4);
                par[0] = 0;
                par[1] = 0;
                par[2] = 0;
                par[3] = 0;
                G_pMDecControl->ConfigTargetWindow(target, par);
            }
            else if(strncmp("layout", buffer+2, 6) == 0){
                G_pMDecControl->ReLayout(LAYOUT_TYPE_RECOVER);
            }
            else if(strncmp("render", buffer+2, 6) == 0){
                int render, win, dsp;
                if(sscanf(trim(buffer), "%d,%d,%d", &render, &win, &dsp) < 3){
                    AM_ERROR("use cmd '--render target, target !\n");
                }else{
                    G_pMDecControl->ConfigRender(render, win, dsp);
                }
            }else if(strncmp("rendone", buffer+2, 7) == 0){
                G_pMDecControl->ConfigRenderDone();
            }else if(strncmp("rtmp", buffer+2, 4) == 0){
                int index;
                char rtmpUrl[128]={0};
                if(sscanf(trim(buffer), "%d,%s", &index, rtmpUrl) < 2){
                    AM_ERROR("use cmd '--rtmp target,rtmpUrl !\n");
                }else{
                    AM_INFO("Cmd Info: --rtmp %d,%s.\n", index, rtmpUrl);
                    G_pMDecControl->SaveSource(index, rtmpUrl, SAVE_INJECTOR_FLAG);
                }
            }else if(strncmp("endrtmp", buffer+2, 7) == 0){
                int index;
                if(sscanf(trim(buffer), "%d", &index) < 1){
                    AM_ERROR("use cmd '--endrtmp target!\n");
                }else{
                    AM_INFO("Cmd Info: --endrtmp %d,%s.\n", index);
                    G_pMDecControl->StopSaveSource(index, SAVE_INJECTOR_FLAG);
                }
            }else if(strncmp("rtsp", buffer+2, 4) == 0){
                int index;
                char streamName[128]={0};
                if(sscanf(trim(buffer), "%d,%s", &index, streamName) < 2){
                    AM_ERROR("use cmd '--rtsp target,streamName !\n");
                }else{
                    AM_INFO("Cmd Info: --rtsp %d,%s.\n", index, streamName);
                    G_pMDecControl->SaveSource(index, streamName, SAVE_INJECTOR_RTSP_FLAG);
                }
            }else if(strncmp("endrtsp", buffer+2, 7) == 0){
                int index;
                if(sscanf(trim(buffer), "%d", &index) < 1){
                    AM_ERROR("use cmd '--endrtsp target!\n");
                }else{
                    AM_INFO("Cmd Info: --endrtsp %d\n", index);
                    G_pMDecControl->StopSaveSource(index, SAVE_INJECTOR_RTSP_FLAG);
                }
            }else if(strncmp("audioproxy", buffer+2, 4) == 0){
                class MyGetToken{
                public:
                    static int getToken(char *src,char *dst){
                        char *ptr = src;
                        int i = 0;
                        while(ptr){
                            if(*ptr == ',' || *ptr == '\0')
                                break;
                            dst[i++] = *ptr;
                            ++ptr;
                        }
                        dst[i] = '\0';
                        return i + 1;
                    }
                };
                char rtspUrl[1024] = {0};
                char streamName[128]={0};
                char *buf = trim(buffer);
                buf += MyGetToken::getToken(buf,rtspUrl);
                buf += MyGetToken::getToken(buf,streamName);
                AM_INFO("Cmd Info: --audioproxy %s,%s\n",(rtspUrl[0] == '\0') ? "NULL":rtspUrl,streamName);
                G_pMDecControl->StartAudioProxy(rtspUrl,streamName);
            }else if(strncmp("endaudioproxy", buffer+2, 7) == 0){
                char rtspUrl[1024];
                if(sscanf(trim(buffer), "%s", rtspUrl) < 1){
                    AM_ERROR("use cmd '--endaudioproxy target!\n");
                }else{
                    AM_INFO("Cmd Info: --endaudioproxy %s\n", rtspUrl);
                    G_pMDecControl->StopAudioProxy(rtspUrl);
                }
            }else if(strncmp("livets", buffer+2, 6) == 0) {
                class MyGetToken{
                public:
                    static int getToken(char *src,char *dst){
                        char *ptr = src;
                        int i = 0;
                        while(ptr){
                            if(*ptr == ',' || *ptr == '\0')
                                break;
                            dst[i++] = *ptr;
                            ++ptr;
                        }
                        dst[i] = '\0';
                        return i + 1;
                    }
                };

                char streamName[128]={0};
                char inputAddress[128] = {0};
                char s_inputPort[32] = {0};
                char s_isRawUdp[32] = {0};
                int inputPort,isRawUdp;
                char *buf = trim(buffer);
                buf += MyGetToken::getToken(buf,streamName);
                buf += MyGetToken::getToken(buf,inputAddress);
                buf += MyGetToken::getToken(buf,s_inputPort);
                MyGetToken::getToken(buf,s_isRawUdp);
                inputPort = atoi(s_inputPort);
                isRawUdp = atoi(s_isRawUdp);
                AM_ERROR("use cmd '--livets <streamName>,[inputIpAddress],<inputPort>,<isRawUdp>\n");
                AM_INFO("Cmd Info: --livets %s,%s,%d,%d\n",streamName,(inputAddress[0] == '\0') ? "NULL":inputAddress,inputPort,isRawUdp);
                G_pMDecControl->SaveInputSource(streamName,inputAddress,inputPort,isRawUdp);
                //G_pMDecControl->SaveInputSource("test.livets",NULL,1234,0);
            }else if(strncmp("endlivets", buffer+2, 9) ==0){
                char streamName[128] = {0};
                if(sscanf(trim(buffer), "%s", streamName) < 1){
                    AM_ERROR("use cmd '--endlivets streamName!\n");
                }else{
                    AM_INFO("Cmd Info: --endlivets %s\n", streamName);
                    G_pMDecControl->StopInputSource(streamName);
                }
                //G_pMDecControl->StopInputSource("test.livets");
            }else if(strncmp("nvrrtmp", buffer+2, 4) == 0){
                char rtmpUrl[128]={0};
                if(sscanf(trim(buffer), "%s", rtmpUrl) < 1){
                    AM_ERROR("use cmd '--nvrrtmp rtmpUrl !\n");
                }else{
                    AM_INFO("Cmd Info: --nvrrtmp %s.\n", rtmpUrl);
                    G_pMDecControl->UploadNVRTranscode2Cloud(rtmpUrl, UPLOAD_NVR_INJECTOR_RTMP_FLAG);
                }
            }else if(strncmp("endnvrrtmp", buffer+2, 7) == 0){
                AM_INFO("Cmd Info: --endnvrrtmp.\n");
                G_pMDecControl->StopUploadNVRTranscode2Cloud(UPLOAD_NVR_INJECTOR_RTMP_FLAG);
            }else if(strncmp("recfile", buffer+2, 4) == 0){
                char fileName[128]={0};
                if(sscanf(trim(buffer), "%s", fileName) < 1){
                    AM_ERROR("use cmd '--recfile fileName !\n");
                }else{
                    AM_INFO("Cmd Info: --recfile %s.\n", fileName);
                    if(en_dump_file){
                        AM_INFO("recfile ING...\n");
                    }else{
                        G_pMDecControl->RecordToFile(AM_TRUE, fileName);
                        en_dump_file = true;
                    }
                }
            }else if(strncmp("stoprecfile", buffer+2, 7) == 0){
                AM_INFO("Cmd Info: --stoprecfile.\n");
                if(en_dump_file){
                    G_pMDecControl->RecordToFile(AM_FALSE);
                    en_dump_file = false;
                }else{
                    AM_ERROR("recfile is NOT running..\n");
                }
            }else if(buffer[2] == 's' && buffer[3] == 'e' && buffer[4] == 'e' && buffer[5] == 'k'){
                int index;
                AM_U64 s;
                if(sscanf(trim(buffer), "%d,%llu", &index, &s) < 2){
                    AM_ERROR("use cmd '--seek winindex,time[s] !\n");
                }else{
                    AM_INFO("Cmd Info: --seek %d,%llu.\n", index, s);
                    G_pMDecControl->SeekTo(s*1000, index);
                }
            }else if (strncmp("start-audio", buffer+2, 11) == 0) {
                AM_INFO("Cmd Info: --start-audio.\n");
                DoG711AudioStream(AM_TRUE);
            }else if (strncmp("stop-audio", buffer+2, 10) == 0) {
                AM_INFO("Cmd Info: --stop-audio.\n");
                DoG711AudioStream(AM_FALSE);
            }else{
                AM_INFO("Unkonw cmd line %s\n", buffer);
            }
            break;

        case 'c':
            //some cmd

            //speed related, 'cp xxx'
            if ('p' == buffer[1]) {
                unsigned int speed, speed_frac;
                unsigned int possiable_speed = 1;
                unsigned int dec_id = 0;
                IParameters::DecoderFeedingRule feeding_rules = IParameters::DecoderFeedingRule_AllFrames;

                if (!G_pMDecControl) {
                    break;
                }

                if ('a' == buffer[2]) {
                    //auto mode, if speed >= 4, use I ONLY mode
                    if (3 == sscanf(buffer, "cpa:%d %x.%x", &dec_id, &speed, &speed_frac)) {
                        if (dec_id >= MAX_CHANNELS) {
                            AM_ERROR("BAD channel number %d\n", dec_id);
                            break;
                        }
                        possiable_speed = max_possiable_speed(dec_id);
                        AM_INFO("specify playback speed %x.%x, max possiable %d\n", speed, speed_frac, possiable_speed);
                        if (speed > possiable_speed) {
                            AM_INFO("speed(%d) > possiable_speed(%d), use I only mode\n", speed, possiable_speed);
                            feeding_rules = IParameters::DecoderFeedingRule_IOnly;
                            cur_pb_speed[dec_id] = 1;
                        } else {
                            feeding_rules = IParameters::DecoderFeedingRule_AllFrames;
                            cur_pb_speed[dec_id] = speed;
                        }
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed & 0x7f, speed_frac, feeding_rules);
                    } else if (2 == sscanf(buffer, "cpa:%x.%x", &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x\n", speed, speed_frac);
                        dec_id = -1;//hd, hard code here
                        if (speed > 2) {
                            AM_INFO("speed(%d) > 2, use I only mode\n", speed);
                            feeding_rules = IParameters::DecoderFeedingRule_IOnly;
                        } else {
                            feeding_rules = IParameters::DecoderFeedingRule_AllFrames;
                        }
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed & 0x7f, speed_frac, feeding_rules);
                    } else if (3 == sscanf(buffer, "cpabw:%d %x.%x", &dec_id, &speed, &speed_frac)) {
                        AM_INFO("specify playback speed(bw) %x.%x\n", speed, speed_frac);
                        if (speed > 2) {
                            AM_INFO("speed(%d) > 2, use I only mode\n", speed);
                            feeding_rules = IParameters::DecoderFeedingRule_IOnly;
                        } else {
                            feeding_rules = IParameters::DecoderFeedingRule_NotValid;
                        }
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed | 0x80, speed_frac, feeding_rules);
                    } else if (2 == sscanf(buffer, "cpabw:%x.%x", &speed, &speed_frac)) {
                        AM_INFO("specify playback speed(bw) %x.%x\n", speed, speed_frac);
                        dec_id = -1;//hd, hard code here
                        if (speed > 2) {
                            AM_INFO("speed(%d) > 2, use I only mode\n", speed);
                            feeding_rules = IParameters::DecoderFeedingRule_IOnly;
                        } else {
                            feeding_rules = IParameters::DecoderFeedingRule_NotValid;
                        }
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed | 0x80, speed_frac, feeding_rules);
                    } else {
                        AM_INFO("you should type 'cpabw:udec_id speed.speed_frac' and enter to specify playback speed, if speed >=2x, will choose I only mode\n");
                    }
                } else if ('i' == buffer[2]) {
                    //I only mode
                    if (3 == sscanf(buffer, "cpi:%d %x.%x", &dec_id, &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x, feed I only\n", speed, speed_frac);
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_IOnly);
                    } else if (2 == sscanf(buffer, "cpi:%x.%x", &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x, feed I only\n", speed, speed_frac);
                        dec_id = -1;//hd, hard code here
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_IOnly);
                    } else {
                        AM_INFO("you should type 'cpi:udec_id speed.speed_frac' and enter to specify playback speed, feed I only\n");
                    }
                } else if ('r' == buffer[2]) {
                    //I only mode
                    if (3 == sscanf(buffer, "cpr:%d %x.%x", &dec_id, &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x, feed Ref only\n", speed, speed_frac);
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_RefOnly);
                    } else if (2 == sscanf(buffer, "cpr:%x.%x", &speed, &speed_frac)) {
                        dec_id = -1;//hd, hard code here
                        AM_INFO("specify playback speed %x.%x, feed Ref only\n", speed, speed_frac);
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_RefOnly);
                    } else {
                        AM_INFO("you should type 'cpr:udec_id speed.speed_frac' and enter to specify playback speed, feed Ref only\n");
                    }
                } else if (':' == buffer[2]) {
                    //only set playback speed
                    if (3 == sscanf(buffer, "cp:%d %x.%x", &dec_id, &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x\n", speed, speed_frac);
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_NotValid);
                    } else if (2 == sscanf(buffer, "cp:%x.%x", &speed, &speed_frac)) {
                        AM_INFO("specify playback speed %x.%x\n", speed, speed_frac);
                        dec_id = -1;//hd, hard code here
                        G_pMDecControl->UpdatePBSpeed(dec_id, speed, speed_frac, IParameters::DecoderFeedingRule_NotValid);
                    } else {
                        AM_INFO("you should type 'cp:udec_id speed.speed_frac' and enter to specify playback speed, only issue speed cmd to DSP\n");
                    }
                } else if (('c' == buffer[2])) {
                    //clear speed setting
                    if (1 == sscanf(buffer, "cpc:%d", &dec_id)) {
                        AM_INFO("clear playback speed setting, for udec %d\n", dec_id);
                        G_pMDecControl->UpdatePBSpeed(dec_id, 0x01, 0x00, IParameters::DecoderFeedingRule_AllFrames);
                    } else {
                         AM_INFO("you should type 'cpc:udec_id' and enter to clear playback speed setting, reset to 1x, all frame mode\n");
                    }
                } else if (('m' == buffer[2])) {
                    //only change pb-strategy
                    if (2 == sscanf(buffer, "cpm:%d strategy %d", &dec_id, &speed)) {
                        AM_INFO("set playback mode %d, for udec %d\n", speed, dec_id);
                        G_pMDecControl->UpdatePBSpeed(dec_id, InvalidPBSpeedParam, InvalidPBSpeedParam, (IParameters::DecoderFeedingRule)speed);
                    } else {
                        AM_INFO("you should type 'cpm:udec_id strategy' and enter to set playback mode(0: all frames, 1: ref only, 2: I only, 3: IDR only)\n");
                    }
                } else {
                    AM_INFO("you should type 'cpa:udec_id speed.speed_frac' and enter to specify playback speed\n");
                    AM_INFO("\tor type 'cpi:udec_id speed.speed_frac' and enter to specify playback speed, I only mode\n");
                    AM_INFO("\tor type 'cpr:udec_id speed.speed_frac' and enter to specify playback speed, feed Ref only\n");
                    AM_INFO("\tor type 'cp:udec_id speed.speed_frac' and enter to specify playback speed\n");
                    AM_INFO("\tor type 'cpc:udec_id' and enter to clear playback speed setting\n");
                    AM_INFO("\tor type 'cpm:udec_id strategy' and enter to set playback mode(0: all frames, 1: ref only, 2: I only, 3: IDR only)\n");
                }
            } else if ('z' == buffer[1]) {
                int render_id=-1;
                static unsigned short input_w=0, input_h=0, center_x=0, center_y=0;
                if ('2' == buffer[2]) {
                    //mode 2, specify input rect
                    if (5 == sscanf(buffer + 4, "%d,%hd,%hd,%hd,%hd", &render_id, &input_w, &input_h, &center_x, &center_y)) {
                        G_pMDecControl->GetStreamInfo(render_id, &video_width, &video_height);
                        if(input_w <= video_width
                            && input_h <= video_height
                            && (center_x+(input_w/2))<=video_width
                            && (center_x-(input_w/2))>=0
                            && (center_y+(input_h/2))<=video_height
                            && (center_y-(input_h/2))>=0){
                            G_pMDecControl->PlaybackZoom(render_id, input_w, input_h, center_x, center_y);
                            g_aZoomInfo[render_id].video_width = video_width;
                            g_aZoomInfo[render_id].video_height = video_height;
                            g_aZoomInfo[render_id].input_w=input_w;
                            g_aZoomInfo[render_id].input_h=input_h;
                            g_aZoomInfo[render_id].center_x=center_x;
                            g_aZoomInfo[render_id].center_y=center_y;
                        }else{
                            AM_ERROR("zoom parameters excced expect range!\n");
                        }
                    } else {
                        AM_ERROR("you should type 'cz2:render_id,input_with,input_height,center_x,center_y', use decimal format\n");
                    }
                } else if('o' == buffer[2]){
                if (1 == sscanf(buffer + 4, "%d", &render_id)) {
                        if(render_id>=16)
                        {
                            AM_ERROR("render_id=%d out of range.\n");
                        }else{
                                G_pMDecControl->GetStreamInfo(render_id, &video_width, &video_height);
                                if(g_aZoomInfo[render_id].video_width!=video_width
                                    || g_aZoomInfo[render_id].video_height!=video_height)
                                {
                                    g_aZoomInfo[render_id].video_width = video_width;
                                    g_aZoomInfo[render_id].video_height = video_height;
                                    g_aZoomInfo[render_id].input_w=video_width;
                                    g_aZoomInfo[render_id].input_h=video_height;
                                    g_aZoomInfo[render_id].center_x=video_width/2;
                                    g_aZoomInfo[render_id].center_y=video_height/2;
                                }
                                if((g_aZoomInfo[render_id].center_x + (g_aZoomInfo[render_id].input_w+1)/2 + DINPUT_WIN_STEP/2) <= video_width && (g_aZoomInfo[render_id].center_y + (g_aZoomInfo[render_id].input_h+1)/2 + DINPUT_WIN_STEP/2) <= video_height){
                                    g_aZoomInfo[render_id].input_w += DINPUT_WIN_STEP;
                                    g_aZoomInfo[render_id].input_h += DINPUT_WIN_STEP;
                                    G_pMDecControl->PlaybackZoom(render_id, g_aZoomInfo[render_id].input_w, g_aZoomInfo[render_id].input_h, g_aZoomInfo[render_id].center_x, g_aZoomInfo[render_id].center_y);
                                }else{
                                    AM_ERROR("cannot zoom out now, current input win width %d, height %d, video size %d x %d\n", g_aZoomInfo[render_id].input_w, g_aZoomInfo[render_id].input_h, video_width, video_height);
                                }
                        }
                  } else {
                            AM_ERROR("you should type 'czo:render_id', use decimal format\n");
                  }
                }else if('i' == buffer[2]){
                if (1 == sscanf(buffer + 4, "%d", &render_id)) {
                        if(render_id>=16)
                        {
                            AM_ERROR("render_id=%d out of range.\n");
                        }else{
                            G_pMDecControl->GetStreamInfo(render_id, &video_width, &video_height);
                            if(g_aZoomInfo[render_id].video_width!=video_width
                                    || g_aZoomInfo[render_id].video_height!=video_height)
                            {
                                g_aZoomInfo[render_id].video_width = video_width;
                                g_aZoomInfo[render_id].video_height = video_height;
                                g_aZoomInfo[render_id].input_w=video_width;
                                g_aZoomInfo[render_id].input_h=video_height;
                                g_aZoomInfo[render_id].center_x=video_width/2;
                                g_aZoomInfo[render_id].center_y=video_height/2;
                            }
                            if((g_aZoomInfo[render_id].input_w > DINPUT_WIN_STEP) && (g_aZoomInfo[render_id].input_h > DINPUT_WIN_STEP)){
                                g_aZoomInfo[render_id].input_w -= DINPUT_WIN_STEP;
                                g_aZoomInfo[render_id].input_h -= DINPUT_WIN_STEP;
                                G_pMDecControl->PlaybackZoom(render_id, g_aZoomInfo[render_id].input_w, g_aZoomInfo[render_id].input_h, g_aZoomInfo[render_id].center_x, g_aZoomInfo[render_id].center_y);
                            }else{
                                AM_ERROR("cannot zoom in now, current input win width %d, height %d, video size %d x %d\n", g_aZoomInfo[render_id].input_w, g_aZoomInfo[render_id].input_h, video_width, video_height);
                            }
                        }
                    } else {
                        AM_ERROR("you should type 'czi:render_id', use decimal format\n");
                    }
                } else {
                    AM_WARNING("you should type 'cz1:render_id,zoomfactorx,zoomfactory', zoomfactor use hex format\n");
                    AM_WARNING("or type 'cz2:render_id,input_with,input_height,center_x,center_y', use decimal format\n");
                }
            }else if('j' == buffer[1]){
                int win_index;
                if(1 == sscanf(buffer + 3, "%d", &win_index)){
                    int video_w;
                    int video_h;
                    SCaptureJpegInfo param;
                    if(win_index==-1){
                        G_pMDecControl->GetMdecInfo(info);
                        if(info.isNvr){
                            //FIXME: only 2x2 here
                            if(G_pMDecControl->GetStreamInfo(0, &video_w, &video_h)==ME_OK){
                                video_w *= 2;
                                video_h *= 2;
                            }
                        }
                    }else{
                        G_pMDecControl->GetStreamInfo(win_index, &video_w, &video_h);
                        G_pMDecControl->GetMdecInfo(info);
                        AM_INFO("GetMdecInfo, isnvr?%s.\n", info.isNvr?"yes":"no");
                    }
                    if(G_pJpegEnc == NULL){
                        AM_INFO("first time to capture jpeg, creat jpegencoder\n");
                        G_pJpegEnc = new JpegEncoder(info.nvrIavFd);
                    }
                    param.vout = 1;
                    if(info.isNvr){
                        param.dec_id = (win_index==-1)?0xff:win_index;
                    }else{
                        param.dec_id = (win_index==-1)?0xff:(g_mdec_num -1);
                    }
                    param.file_index = g_jpeg_capture_index;
                    param.video_width = video_w;
                    param.video_height = video_h;
                    param.enlog = true;
                    G_pJpegEnc->SetParams(&param);
                    G_pJpegEnc->PlaybackCapture();
                    g_jpeg_capture_index++;
                }else{
                    AM_ERROR("can not capture jpeg, win_index %d\n", win_index);
                }
            }else{
                AM_WARNING("you should type 'cj:window_index' to capture jpeg pic\n");
            }
            break;

        default:
            AM_INFO("Unkonw cmd line:%c\n", ch);
            break;
        }
    }

    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
        AM_ERROR("stdin_fileno set error");
    AM_INFO("\nExit RunMainLoop on MdecTest.\n");
}

int get_encode_resolution(const char *name, AM_UINT *width, AM_UINT *height)
{
     int i;

     for (i = 0; i < ARRAY_SIZE(__encode_res); i++)
         if (strcmp(__encode_res[i].name, name) == 0) {
             *width = __encode_res[i].width;
             *height = __encode_res[i].height;
             printf("get_encode_resolution: %u, %u.\n", *width, *height);
             return 0;
         }

     printf("resolution '%s' not found.\n", name);
     return -1;
}

int ParseOption(int argc, char** argv)
{
    int temp = 0;
    int ch;
    int option_index = 0;
    opterr = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
    {
        switch (ch)
        {
        case 'a':
            //printf("test:%s",optarg);
            if (get_source(optarg) < 0)
                return -1;
            g_source_num++;
            break;

        case 'h':
            usage();
            return -2;

        case 'f':
            if(get_global_flag(optarg) < 0)
                return -1;
            break;

        case STREAM_FLAG_HD:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_source_flag[g_source_num -1] |= SOURCE_FULL_HD;
            g_source_group[g_source_num -1] = temp;
            g_sd_source_index--;
            break;

        case STREAM_SAVE_INFO:
            strncpy(g_source_save[g_source_num - 1].name, optarg, MAX_SOURCE_NAME_LEN - 2);
            g_source_save[g_source_num - 1].name[MAX_SOURCE_NAME_LEN - 1] = '\0';
            if(strlen(g_source_save[g_source_num -1].name) == 0){
                AM_INFO("Please input the filename you wanted for stream %d dump. Now use dump_stream.es as default.\n", g_source_num -1);
                strncpy(g_source_save[g_source_num - 1].name, "dump_stream.es", MAX_SOURCE_NAME_LEN - 2);
            }
            g_source_flag[g_source_num -1] |= SOURCE_SAVE_INFO;
            break;

        case ENABLE_TRANSCODE:
                if (get_encode_resolution(optarg, &enc_width, &enc_height) < 0){
                    AM_ERROR("get_encode_resolution error, set resolution to 720X480.\n");
                }
                enable_transcode = true;
                g_global_flag |= HAVE_TRANSCODE;
            break;

        case TRANSCODE_BITRATE:
            if(sscanf(optarg, "%d", &enc_bitrate) != 1){
                AM_ERROR("get bireate error! set to 1M.\n");
                enc_bitrate = 1000000;
            }
            break;

        case TRANSCODE_DUMPFILE:
            en_dump_file = true;
            if(sscanf(optarg, "%s", &transcode_file_name) != 1){
                return -1;
            }
            break;

        case TRANSCODE_CLOUD_RTMP:
            if(sscanf(optarg, "%s", &transcode2cloud_url) != 1){
                return -1;
            }
            en_transcode2cloud = true;
            break;

        case TOTAL_MDEC_NEED:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_mdec_num = temp;
            break;

        case MD_DSP_MAX_FRAME_NUM:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_dsp_max_frm_num = temp;
            break;

        case MD_DSP_PRE_BUFFER_LEN:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_dsp_pre_buffer_len = temp;
            break;

        case MD_DISPLAY_LAYOUT:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_display_layout = temp;
            break;

        case MD_NET_BEGIN_BUFFER_NUM:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_net_buffer_begin_num = temp;
            break;

        case ENABLE_WSD:
            g_enable_wsd = 1;
            break;

        case NET_PALY_BACK:
            g_global_flag |= USING_FOR_NET_PB;
            break;

        case NO_HD_NVR_PB:
            g_global_flag |= NO_HD_WIN_NVR_PB;
            break;

        case ME_VOUT_MASK:
            if(sscanf(optarg, "%d", &temp) != 1){
                return -1;
            }
            g_vout_mask = temp;
            break;

        case NO_FILL_PTS_INFO:
            g_global_flag |= NO_FILL_PES_SSP_HEADER;
            break;

        case SWITCH_JUST_FOR_WIN:
            g_global_flag |= SWITCH_JUST_FOR_WIN_ARM;
            break;

        case SWITCH_JUST_FOR_WIN2:
            g_global_flag |= SWITCH_JUST_FOR_WIN_ARM2;
            break;

        case SWITCH_JUST_FOR_WIN3:
            g_global_flag |= SWITCH_JUST_FOR_WIN_ARM3;
            break;

        case SWITCH_DO_SEEK_FEED_TO_DSP:
            g_global_flag |= SWITCH_ARM_SEEK_FEED_DSP_DEBUG;
            break;

        case SWITCH_DO_SEEK_FEED_TO_DSP2:
            g_global_flag |= SWITCH_ARM_SEEK_FEED_DSP_DEBUG2;
            break;

        case SAVE_ALL_STREAM:
            g_global_flag |= SAVE_ALL_STREAMS;
            break;

        case TEST_BY_CC_1:
            g_global_flag |= DEBUG_100MS_ON_07_23;
            break;

        case TEST_BY_CC_2:
            g_global_flag |= DEBUG_100MS_ON_07_23_2;
            break;

        case MD_NO_QUERY:
            g_global_flag |= NO_QUERY_ANY_FROM_DSP;
            break;

        case MD_NO_AUTO_RESAMPLE_AUDIO:
            g_global_flag |= NO_AUTO_RESAMPLE_AUDIO;
            break;

        case MD_NO_AUDIO_ON_GMF:
            g_global_flag |= NO_AUDIO_ON_GMF;
            break;

        case MD_NVR_PLAYBACK_MODE:
            g_nvr_playback_mode = atoi(optarg);
            break;

        case MD_SEPARATE_FILE_DURATION:
            g_auto_savefile_duration = atoi(optarg);
            break;

        case MD_SEPARATE_FILE_MAXCOUNT:
            g_auto_savefile_maxfilecount = atoi(optarg);
            break;

        case RUN_MOTION_DETECTE_RECEIVER:
            g_global_flag |= ADD_MD_RECEIVER;
            break;

        case MD_LOOP_FOR_LOCAL_FILE:
            g_global_flag |= LOOP_FOR_LOCAL_FILE;
            break;

        case MD_MULTI_STREAM_SET:
            g_global_flag |= MULTI_STREAMING_SETS;
            break;

        case CONFIG_AUDIO_RTSP_STREAMING:
            g_audio_rtsp_stream_state = atoi(optarg);
            break;

        case FOOL_MODE:
            g_global_flag |= NOTHING_DONOTHING;
            break;
        case FOOL_NO_FLUSH:
            g_global_flag |= NOTHING_NOFLUSH;
            break;

        case CONFIG_TILEMODE:
            g_tilemode = atoi(optarg);
            if ((0 == g_tilemode) || (5 == g_tilemode)) {
                printf("--tilemode %d\n", g_tilemode);
            } else {
                printf("error input argument, --tilemode %d\n", g_tilemode);
                g_tilemode = 0;
            }
            break;

        case MD_DECODER_CAP:
            get_decoder_cap(optarg);
            break;

        case SWITCH_LOADER:
            g_enable_loader = atoi(optarg);
            break;
        case FILE_M3U8_NAME:
            sscanf(optarg, "%s", m3u8);
            break;

        case CLOUD_LOGIN_ACCOUNT:
            sscanf(optarg, "%s", host);
            break;

        case PATH_SAVING_FILE:
            sscanf(optarg, "%s", path);
            break;

        case STREAM_COUNT_IN_M3U8:
            g_stream_count_in_m3u8 = atoi(optarg);
            break;

        default:
            printf("unknown option found: %d\n", ch);
            return -1;
        }
    }
    return 0;
}

static AM_ERR checkVOUTValid(int& vout_mask)
{
    AM_INT iavFd=0, num = 0;

    if ((iavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        AM_ERROR("checkVOUTValid failed.\n");
        return ME_ERROR;
    }

    if (::ioctl(iavFd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
        perror("IAV_IOC_VOUT_GET_SINK_NUM");
        AM_ERROR("checkVOUTValid failed.\n");
        ::close(iavFd);
        return ME_ERROR;
    }

    if (num < 1) {
        AM_ERROR("Please load vout driver!\n");
        AM_ERROR("checkVOUTValid failed.\n");
        ::close(iavFd);
        return ME_ERROR;
    }

    //init vout config struct
    for (AM_INT vout_id = eVoutLCD; vout_id <eVoutCnt; vout_id++) {
        struct amba_vout_sink_info  sink_info;
        AM_INT i, sink_id = -1, sink_type;

        if (eVoutLCD == vout_id) {
            sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
        } else {
            sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
        }

        for (i = num - 1; i >= 0; i--) {
            sink_info.id = i;
            if (::ioctl(iavFd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
                perror("IAV_IOC_VOUT_GET_SINK_INFO");
                AM_ERROR("**IAV_IOC_VOUT_GET_SINK_INFO fail!\n");
                AM_ERROR("checkVOUTValid failed.\n");
                ::close(iavFd);
               return ME_ERROR;
            }

            if (sink_info.source_id == vout_id &&
                sink_info.sink_type == sink_type) {
                sink_id = sink_info.id;
                break;
            }
        }

        if(-1 == sink_id
            || sink_info.state != AMBA_VOUT_SINK_STATE_RUNNING
            || 0==sink_info.sink_mode.video_en){
            vout_mask &= ~(1<<vout_id);
            AM_WARNING("vout %d is invalid, sink_id=%d, state=%d, video_en=%d, mdfed vout_mask=%d\n", vout_id, sink_id, sink_info.state, sink_info.sink_mode.video_en, vout_mask);
        }

    }

    ::close(iavFd);
    return ME_OK;
}

static AM_INT _config_update_callback(void* thiz, AM_UINT flag)
{
    AM_ERROR("to do, implement here, thiz %p, flag %08x\n", thiz, flag);
    return 0;
}

extern "C" int main(int argc, char **argv)
{
    AM_ERR err;
    AM_INT i;
    AM_INT flag = 0;

#if PLATFORM_ANDROID
    AMPlayer::AudioSink *audiosink;
    audiosink = (new MediaPlayerService::AudioOutput(1));
#endif
    i = ParseOption(argc, argv);
    if (i < 0){
        if(i != -2)
            AM_ERROR("ParseOption Failed..\n");
        return -1;
    }

    if (ME_OK != AMF_Init()) {
        AM_ERROR("AMF_Init Error.\n");
        return -2;
    }
come_on_again:
    if((0==g_vout_mask) && (!enable_transcode))
    {
        printf("user should sepcify at least one VOUT, g_vout_mask=%d not supported.\n", g_vout_mask);
        return -1;
    }
    printf("[user input]: sepcified vout_mask %d.\n", g_vout_mask);
    //check VOUT valid
    checkVOUTValid(g_vout_mask);
    AM_INFO("After checkVOUTValid(): vout mask %d\n", g_vout_mask);
    memset(g_aZoomInfo, 0, sizeof(TZoomInfo)*16);
    DeviceObserver *observer = NULL;
    if(g_enable_wsd){
        wsd_socket_init();
        observer = new DeviceObserver();
        WSDiscoveryClient::GetInstance()->registerObserver(observer);
        WSDiscoveryClient::GetInstance()->start();
    }

    //in transcode mode, prebuffer is not needed
    if(enable_transcode) g_dsp_pre_buffer_len = 0;

    CParam par(CONSTRUCT_PAR_NUM);
    par[REQUEST_DSP_NUM] = g_mdec_num;
    par[GLOBAL_FLAG] = g_global_flag;
    par[DSP_MAX_FRAME_NUM] = g_dsp_max_frm_num;
    par[NET_BUFFER_BEGIN_NUM] = g_net_buffer_begin_num;
    par[DSP_PRE_BUFFER_LEN] = g_dsp_pre_buffer_len;
    par[AUTO_SEPARATE_FILE_DURATION] = g_auto_savefile_duration;
    par[AUTO_SEPARATE_FILE_MAXCOUNT] = g_auto_savefile_maxfilecount;
    par[DISPLAY_LAYOUT] = g_display_layout;
    par[VOUT_MASK] = g_vout_mask;
    par[DSP_TILEMODE] = g_tilemode;
    par[DECODER_CAP] = g_decoder_cap;
    AM_INFO("CreateActiveMDecControl: mdec num:%d, flag:%d, max_frm_num:%d, buffer_net:%d, vout mask %d\n", par[0], par[1], par[2], par[3], par[8]);

#if PLATFORM_ANDROID
    G_pMDecControl = CreateActiveMDecControl(audiosink, par);
#else
    G_pMDecControl = CreateActiveMDecControl(NULL, par);
#endif

    if(G_pMDecControl == NULL)
        return -3;
    //printf("get_source %d\n", 1);

    if((G_pMDecControl->SetAppMsgCallback(ProcessAMFMsg, NULL)) != ME_OK) {
        AM_ERROR("SetAppMsgCallback failed\n");
        return -1;
    }

    for(i = 0; i < g_source_num; i++)
    {
        flag = g_source_flag[i];
        flag |= SOURCE_ENABLE_VIDEO;
        //DEBUG
        if(g_global_flag & MULTI_STREAMING_SETS){
            if(g_global_flag & NO_HD_WIN_NVR_PB){
                if(i < g_mdec_num)
                    err = G_pMDecControl->AddSource(g_source[i].name, g_source_group[i], flag);
                else
                    break;//edit after start
            }else{
                if(i < ((g_mdec_num-1) * 2))
                    err = G_pMDecControl->AddSource(g_source[i].name, g_source_group[i], flag);
                else
                    break;//edit after start
            }
        }else{
            err = G_pMDecControl->AddSource(g_source[i].name, g_source_group[i], flag);
        }
        if(g_global_flag & SAVE_ALL_STREAMS){
            char name[50];
            snprintf(name, 49, "%d_stream.ts", i);
            err = G_pMDecControl->SaveSource(i, name, 0);
        }else{
            if(flag & SOURCE_SAVE_INFO){
                err = G_pMDecControl->SaveSource(i, g_source_save[i].name, 0);
            }
        }
        /*if(i == 0){
            AM_INFO("xxx%d\n", SOURCE_ENABLE_AUDIO|SOURCE_ENABLE_VIDEO);
            err = G_pMDecControl->AddSource(g_source[i].name, i, SOURCE_ENABLE_AUDIO |SOURCE_ENABLE_VIDEO);
        }else if(i == 4 && (flag & SOURCE_FULL_HD)){
            err = G_pMDecControl->AddSource(g_source[i].name, 0, SOURCE_FULL_HD);
        }else{
            err = G_pMDecControl->AddSource(g_source[i].name, i, SOURCE_ENABLE_VIDEO);
        }*/

        if(err != ME_OK){
            printf("\n=======>Add %d Source %s Failed\n", i, g_source[i].name);
            g_source_group[i] = -1;
            //return -1;
        }
    }
    //RunMainLoop();
    //return -1;

    if(g_nvr_playback_mode != -1){
        AM_INFO("set NVR playback mode: %d\n", g_nvr_playback_mode);
        G_pMDecControl->SetNvrPlayBackMode(g_nvr_playback_mode);
    }
    if(G_pMDecControl->Prepare() != ME_OK)
    {
        printf("Prepare Failed!\n");
        return -1;
    }

    if(enable_transcode &&
        (ME_OK != G_pMDecControl->SetTranscode(enc_width, enc_height, enc_bitrate))){
        printf("SetTranscode Failed!\n");
        return -1;
    }
    if(en_dump_file){
        err = G_pMDecControl->RecordToFile(AM_TRUE, transcode_file_name);
        if (g_enable_loader) {
            G_pMDecControl->ConfigLoader(AM_TRUE, path, m3u8, host, g_stream_count_in_m3u8);
        }
    } else {
        g_enable_loader = 0;
    }

    AM_INFO("Start  Play, Source Num: %d.\n", g_source_num);
    err = G_pMDecControl->Start();
    if( err == ME_OK){
        AM_INFO("Start done.\n");
    }else{
        AM_INFO("Start failed, ret = %d.\n", err);
        return -1;
    }
    //G_pMDecControl->Dump();
    sleep(2);
    int group_set;
    if(g_global_flag & MULTI_STREAMING_SETS && !(g_global_flag & NO_HD_WIN_NVR_PB)){
        for(i = (g_mdec_num-1) * 2; i < g_source_num; i++){
            flag = g_source_flag[i];
            if(flag & SOURCE_FULL_HD){
                group_set = g_source_group[i];
                flag |= SOURCE_EDIT_PLAY |SOURCE_EDIT_ADDBACKGROUND;
            }else{
                group_set = g_source_group[i] -(g_mdec_num-1);
                flag |= SOURCE_EDIT_ADDBACKGROUND;
            }
            err = G_pMDecControl->EditSource(group_set, g_source[i].name, flag);
            if(g_global_flag & SAVE_ALL_STREAMS){
                char name[50];
                snprintf(name, 49, "%d_stream.ts", i);
                err = G_pMDecControl->SaveSource(i, name, 0);
            }else{
                if(flag & SOURCE_SAVE_INFO){
                    err = G_pMDecControl->SaveSource(i, g_source_save[i].name, 0);
                }
            }
        }
    }else if((g_global_flag & MULTI_STREAMING_SETS) && (g_global_flag & NO_HD_WIN_NVR_PB)){
        for(i = (g_mdec_num); i < g_source_num; i++){
            flag = g_source_flag[i];
            if(flag & SOURCE_FULL_HD){
                group_set = g_source_group[i];
                flag |= SOURCE_EDIT_PLAY |SOURCE_EDIT_ADDBACKGROUND;
            }else{
                group_set = g_source_group[i] -(g_mdec_num);
                flag |= SOURCE_EDIT_ADDBACKGROUND;
            }
            err = G_pMDecControl->EditSource(group_set, g_source[i].name, flag);
            if(g_global_flag & SAVE_ALL_STREAMS){
                char name[50];
                snprintf(name, 49, "%d_stream.ts", i);
                err = G_pMDecControl->SaveSource(i, name, 0);
            }else{
                if(flag & SOURCE_SAVE_INFO){
                    err = G_pMDecControl->SaveSource(i, g_source_save[i].name, 0);
                }
            }
        }
    }

    if(g_global_flag & ADD_MD_RECEIVER){
        err = G_pMDecControl->CreateMotionDetectReceiver();
    }

    current_play_hd = 0;

    //enable audio rtsp streaming
    if(g_audio_rtsp_stream_state == 1)
        handle_audio_rtsp_streaming(1);

    //init speed
    clear_speed_settings();

    if(en_transcode2cloud){
        AM_INFO("enable push stream to cloud: %s.\n", transcode2cloud_url);
        G_pMDecControl->UploadNVRTranscode2Cloud(transcode2cloud_url, UPLOAD_NVR_INJECTOR_RTMP_FLAG);
    }

    RunMainLoop();

    //stop g711 audio stream
    DoG711AudioStream(AM_FALSE);

    if(g_enable_wsd){
        WSDiscoveryClient::GetInstance()->Delete();
        delete observer;
        wsd_socket_close();
        free_rtsp_source_list();
    }

    if(G_pJpegEnc){
        delete G_pJpegEnc;
        G_pJpegEnc = NULL;
    }

    //delete recode_engine
    if(g_audio_rtsp_stream_state == 1)
        handle_audio_rtsp_streaming(0);

    G_pMDecControl->Stop();
    G_pMDecControl->Delete();
    //AM_INFO("G_pMDecControl->Delete() done.\n");
    while (1)
    {
        char ch;
        char buffer[128] = {0};
        memset(buffer, 0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0){
            AM_ERROR("read from stdin err, debug me:%s\n", buffer);
            continue;
        }
        ch = buffer[0];
        if('q'==ch)
        {
            break;
        }
        else if('x'==ch)
        {
            g_quit = false;
            goto come_on_again;
        }
        else
        {
            AM_ERROR("q to real exit, x to again.\n");
        }
    }

    AMF_Terminate();
    AM_INFO("AMF_Terminate() done.\n");
    return 0;
}

