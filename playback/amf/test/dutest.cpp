
/**
 * dutest.cpp
 *
 * History:
 *    2012/02/29 - [Zhi He] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
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
#include "am_pbif.h"

//use imgproc in android
#if PLATFORM_ANDROID
#define __use_imgproc__
#endif

#ifdef __use_imgproc__
#include "am_param.h"
#include "vout_control_if.h"
#include "rec_pretreat_if.h"
#endif

#if PLATFORM_ANDROID
#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
#endif

#define if_err_return(err) do { \
    AM_ASSERT(err == ME_OK); \
    if (err != ME_OK) { \
        AM_ERROR("[dutest] error: file %s, line %d, ret %d.\n", __FILE__, __LINE__, err); \
        return -2; \
    } \
} while (0)

enum {
    //System
    TEST_FEEDING_PRIDATA = 0,
    ENABLE_RTSP_STREAMING,
    TEST_START_AAA,
    SELECT_CAMERA,
    SELECT_DSP_MODE,
    SET_VOUT_MASK,
    LOOP_PLAY,
    START_PLAYBACK_AT_BEGINNING,

    DEBUG_NOT_START_ENCODING,
    DEBUG_DISCARD_HALF_AUDIO_PACKET,
};

//#define __app_set_detailed_parameters__

//-------------------------------------------------------------------
//                      Global Varibles
//-------------------------------------------------------------------
static IRecordControl2 *G_pRecordControl = NULL;
static IStreamingControl* G_pStreamingControl = NULL;
static ISimplePlayback* G_pSimplePBControl = NULL;
static IMediaControlOnTheFly* G_pMediaOnTheFlyControl = NULL;

#ifdef __use_imgproc__
static IRecPreTreat* G_pPreTreat = NULL;
#endif

#if PLATFORM_ANDROID
static AMPlayer::AudioSink *G_pAudiosink = NULL;
#endif

static int auto_start_aaa = 0;
static int camera_index = 0;

#define _max_filename_length_  320
static char file_name[2][_max_filename_length_] = {"testrecord.mp4", "testrecord_smallsize.mp4"};
unsigned int sample_rate = 48000;
unsigned int channels = 2;

static char playback_file_name[_max_filename_length_] = "pbfile";
static unsigned int playback_enabled = 0;
static unsigned int playback_start_at_beginning = 1;
static unsigned int playback_started = 0;
unsigned int dsp_mode = DSPMode_DuplexLowdelay;
unsigned int vout_mask = ((1<<eVoutLCD) | (1<<eVoutHDMI));
static int not_encoding = 0;
static int discard_half_audio_packet = 0;
unsigned int loop_play_mode = 1;

static int need_set_path = 0;
static char path_name[_max_filename_length_] = {0};

static IParameters::ContainerType output_format[DMaxMuxerNumber] = {IParameters::MuxerContainer_AUTO, IParameters::MuxerContainer_AUTO};
IParameters::StreamFormat video_format[DMaxMuxerNumber] = {IParameters::StreamFormat_H264, IParameters::StreamFormat_H264};
IParameters::StreamFormat audio_format[DMaxMuxerNumber] = {IParameters::StreamFormat_Invalid, IParameters::StreamFormat_Invalid};//not specify, set by rec.config by default

static AM_UINT total_muxer_number = 2;

static AM_UINT saving_stratege = IParameters::MuxerSavingFileStrategy_ToTalFile;
static AM_UINT saving_condition = IParameters::MuxerSavingCondition_InputPTS;
static AM_UINT max_file_num = DefaultMaxFileNumber;
static AM_UINT auto_time = DefaultAutoSavingParam;
static AM_UINT auto_frame_count = DefaultAutoSavingParam;

static int specify_dumppath = 0;
static int specify_configpath = 0;

static int record_running = 1;

static unsigned int enable_pridata_test = 0;
static unsigned int pridata_index = 0;
static unsigned char private_data_gps_base[400] = " [dutest] test GPS Info: position 12.6543, 30.4567, time 2011-11-14, 12:01:35. index %d.\n";
static unsigned char private_data_sensor_base[400] = " [dutest] test Sensor Info: speed 10.123 m/s, accel 2.1 m/ss, direction 0,0,90. index %d.\n";
static unsigned char private_data_buffer[480] = {0};


static unsigned int rtsp_enabled = 0;
static unsigned int rtsp_output_index_mask = 0;
static unsigned int rtsp_server_index = 0;

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

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
    {"filename", HAS_ARG, 0, 'f'},
    {"pbfile", HAS_ARG, 0, 'F'},
    {"pkg-fmt", HAS_ARG, 0, 'p'},
    {"vid-fmt", HAS_ARG, 0, 'v'},
    {"aud-fmt", HAS_ARG, 0, 'a'},
    {"sample-rate", HAS_ARG, 0, 'r'},
    {"channels", HAS_ARG, 0, 'c'},
    {"number", HAS_ARG, 0, 'n'},
    {"config", HAS_ARG, 0, 'u'},
    {"storage", HAS_ARG, 0, 's'},
    {"time", HAS_ARG, 0, 't'},
    {"frame-cnt", HAS_ARG, 0, 'x'},
    {"maxfilenumber", HAS_ARG, 0, 'm'},
    {"streaming-rtsp", HAS_ARG, 0, ENABLE_RTSP_STREAMING},
    {"feedpridata", NO_ARG, 0, TEST_FEEDING_PRIDATA},
    {"start3a", NO_ARG, 0, TEST_START_AAA},
    {"camera", HAS_ARG, 0, SELECT_CAMERA},
    {"dspmode", HAS_ARG, 0, SELECT_DSP_MODE},
    {"voutmask", HAS_ARG, 0, SET_VOUT_MASK},
    {"loop", HAS_ARG, 0, LOOP_PLAY},
    {"beginpb", HAS_ARG, 0, START_PLAYBACK_AT_BEGINNING},

        //debug use
    {"noencoding", NO_ARG, 0, DEBUG_NOT_START_ENCODING},
    {"discardhalfaudio", NO_ARG, 0, DEBUG_DISCARD_HALF_AUDIO_PACKET},
//    {"container", HAS_ARG, 0, TEST_FEEDING_PRIDATA},
    {0, 0, 0, 0}
};

static const char *short_options = "hf:p:v:a:r:c:n:u:s:t:x:m:F:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
    {"filename",	                        ""	"specify filename to save"},
    {"pbfile",	                        ""	"specify playback filename"},
    {"mp4|avi|mov|mkv",	""	"specify output packaging format"},
    {"h264",	                        "\t"	"specify video compression format"},
    {"aac|ac3|adpcm|mp2",	        ""	"specify audio compression format"},
    {"samplerate",	                ""	"specify audio sample rate"},
    {"channels",	                ""	"1 - mono, 2 - stero"},
    {"number",                 ""          "muxer number"},
    {"config",                 ""          "config path"},
    {"storage",	                ""	"specify storage strategy: 0 - auto-separate, 1 - manual-separate, 2 - no-separate(default)"},
    {"time",                     "" "specify auto-cut file's duration"},
    {"frame-cnt",              "" "specify auto-cut file's frame count"},
    {"maxfilenumber",         "" "max file numbers(exceed will delete earliest file)"},
    {"streaming-rtsp",      "" "start rstp server, output index enabled"},
    {"feedpridata",	                ""	"test feeding private data"},
    {"start3a",                      ""  "init vin, start aaa before recording"},
    {"dspmode",                      ""  "request dsp mode, 2: camera recording, 4: udec, 5: duplex low delay"},
    {"voutmask",                      ""  "request vout mask(1<<0 means LCD, 1<<1 means HDMI)"},
    {"beginpb",                     ""  "begin playback at beginnig"},

    {"noencoding",                      ""  "not start encoding"},
};

void usage(void)
{
    unsigned int i;

    printf("\ndutest usage:\n");
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
    printf("\n");

    printf("dutest runtime cmd usage:\n");
    printf(" change playback display: type 'cpb-display:size_x,size_y,pos_x,pos_y' + 'enter'.\n");
    printf(" change preview display: type 'cpre-display:size_x,size_y,pos_x,pos_y,alpha' + 'enter'.\n");
    printf(" change encoding bitrate: type 'cbitrate:bitrate(Mbps)' + 'enter'.\n");
    printf(" change encoding framerate: type 'cframerate:reduce_factor,framerate_integer' + 'enter', actual enc framerate = vin framerate/reduce_factor, or framerate_integer.\n");
    printf(" change encoding gop: type 'cgop:M,N,idr_interval,gop_structure' + 'enter'.\n");
    printf(" demand idr: type 'cidr:method, pts' + 'enter'.\n");
    printf(" beginpb 1: begin playback at beginnig.\n");

    printf("dutest runtime playback usage:\n");
    printf(" playback: type 'p' + ' ' + 'enter', pause/resume.\n");
    printf(" playback: type 'pg0' + 'enter', seek to 0 second.\n");
    printf(" playback: type 'pplay' + ' ' + 'filename' + 'enter', playfile.\n");
}

int init_param(int argc, char **argv)
{
    int ch;
    IParameters::ContainerType o_format;
    int option_index = 0;
    opterr = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

        switch (ch) {
        case 'h':
            usage();
            exit(0);
            break;

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

        case 'F':
            memset(playback_file_name, 0, _max_filename_length_);
            strncpy(playback_file_name, optarg, _max_filename_length_ - 1);
            playback_enabled = 1;
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

        case 'v':
            // only h264 formate supported
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

        case TEST_FEEDING_PRIDATA:
            enable_pridata_test = 1;
            printf("    enable pridata feeding test.\n");
            break;

        case ENABLE_RTSP_STREAMING:
            rtsp_enabled = 1;
            rtsp_output_index_mask = atoi(optarg);
            printf("    enable rtsp streaming, mask 0x%x.\n", rtsp_output_index_mask);
            break;

        case TEST_START_AAA:
            printf("set auto start aaa before recording.\n");
            auto_start_aaa = 1;
            break;

        case SELECT_CAMERA:
            camera_index = atoi(optarg);
            printf("select camera index %d.\n", camera_index);
            break;

        case SELECT_DSP_MODE:
            dsp_mode = atoi(optarg);
            printf("select dsp mode %d.\n", dsp_mode);
            if (DSPMode_CameraRecording != dsp_mode && DSPMode_DuplexLowdelay != dsp_mode) {
                printf("BAD dsp_mode %d, use default DSPMode_DuplexLowdelay.\n", dsp_mode);
                dsp_mode = DSPMode_DuplexLowdelay;
            }
            break;

        case SET_VOUT_MASK:
            vout_mask = atoi(optarg);
            printf("request vout mask %d.\n", vout_mask);
            break;

        case DEBUG_NOT_START_ENCODING:
            not_encoding = 1;
            printf("request not encoding.\n");
            break;

        case DEBUG_DISCARD_HALF_AUDIO_PACKET:
            discard_half_audio_packet = 1;
            printf("request discard half audio packet.\n");
            break;

        case LOOP_PLAY:
            break;

        case START_PLAYBACK_AT_BEGINNING:
            playback_start_at_beginning = atoi(optarg);
            printf("begin playback at beginning %d.\n", playback_start_at_beginning);
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

        default:
            printf("unknown option found: %d\n", ch);
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

void RunMainLoop()
{
    char buffer_old[128] = {0};
    char buffer[128];
    int flag_stdin = 0;
    int paused = 0;
    int pb_paused = 0;
    unsigned int seek_time = 0;
    AM_ERR err;
    unsigned int ret, param0, param1, param2, param3, param4;

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
            case 'q':   // exit
                printf("Quit\n");
                record_running = 0;
                break;

            case ' ':   // pause, resume
                //ignore record pause/resume in current stage
                buffer_old[0] = buffer[0];
                if (paused == 0) {
                    printf("dutest:pause.\n");
                    //G_pRecordControl->PauseRecord();
                    paused = 1;
                } else {
                    printf("dutest:resume.\n");
                    //G_pRecordControl->ResumeRecord();
                    paused = 0;
                }
                break;

            case 'P':
                G_pRecordControl->PrintState();
                break;

            case 'p':
                //playback related
                if (G_pSimplePBControl) {
                    if (!strncmp(buffer+1, " ", 1)) {
                        if (0 == playback_started) {
                            printf("playback not started, you can type 'pplay + ' ' + filename' to play new file, or type 'pstart' to play previous file.\n");
                            continue;
                        }
                        if (!pb_paused) {
                            err = G_pSimplePBControl->PausePlay();
                            pb_paused = 1;
                        } else {
                            err = G_pSimplePBControl->ResumePlay();
                            pb_paused = 0;
                        }
                    } else if (!strncmp(buffer+1, "g", 1)) {
                        if (0 == playback_started) {
                            printf("playback not started, you can type 'pplay + ' ' + filename' to play new file, or type 'pstart' to play previous file.\n");
                            continue;
                        }
                        if (1 == sscanf(buffer, "pg%d", &seek_time)) {
                            printf("before seek to %d seconds.\n", seek_time);
                            err = G_pSimplePBControl->SeekPlay(((unsigned long long)seek_time)*1000);
                            printf("seek to %d seconds done.\n", seek_time);
                        } else {
                            printf("wrong seek string %s, should be like 'pg10' + 'enter'.\n", buffer);
                        }
                    } else if (!strncmp(buffer+1, "start", 5)) {
                        if (0 == playback_started) {
                            err = G_pSimplePBControl->PlayFile(playback_file_name);
                            if (ME_OK != err) {
                                printf("playback file %s fail, err %d.\n", playback_file_name, err);
                            } else {
                                printf("echo: playback start success.\n");
                                playback_started = 1;
                            }
                        } else {
                            printf("playback already started.\n");
                        }
                    } else if (!strncmp(buffer+1, "stop", 4)) {
                        if (1 == playback_started) {
                            err = G_pSimplePBControl->StopPlay();
                            if (ME_OK != err) {
                                printf("stop playback file %s fail, err %d.\n", playback_file_name, err);
                            } else {
                                printf("echo: playback stop success.\n");
                            }
                            playback_started = 0;
                        } else {
                            printf("playback already stopped.\n");
                        }
                    } else if (!strncmp(buffer+1, "play", 4)) {
                        strncpy((char*)playback_file_name, (char*)buffer + 6, _max_filename_length_ - 1);
                        playback_file_name[_max_filename_length_ - 1] = 0x0;
                        err = G_pSimplePBControl->PlayFile(playback_file_name);
                        if (ME_OK != err) {
                            printf("play new file %s fail, err %d.\n", playback_file_name, err);
                        } else {
                            printf("echo: play new file success.\n");
                            playback_started = 1;
                        }
                    } else if (!strncmp(buffer+1, "speedup", 7)) {
                        err = G_pSimplePBControl->SetPBProperty(IPBControl::DEC_PROPERTY_SPEEDUP_REALTIME_PLAYBACK, 1);
                        if (ME_OK != err) {
                            printf("speed up cmd fail.\n");
                        } else {
                            printf("speed up cmd success.\n");
                        }
                    }
                }  else {
                    printf("no playback engine.\n");
                }
                break;

            case 'c':
                if (!G_pMediaOnTheFlyControl) {
                    break;
                }
                //change on the fly
                if (!strncmp(buffer+1, "pb-display", 10)) {
                    ret = sscanf(buffer, "cpb-display:%d,%d,%d,%d", &param0, &param1, &param2, &param3);
                    if (4 == ret) {
                        G_pMediaOnTheFlyControl->UpdatePBDisplay(param0, param1, param2, param3);
                        printf("echo: cpb-display:%d,%d,%d,%d\n", param0, param1, param2, param3);
                    } else {
                        printf("change playback display's cmd should be: 'cpb-display:size_x,size_y,offset_x,offset_y' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "pre-display", 11)){
                    ret = sscanf(buffer, "cpre-display:%d,%d,%d,%d,%d", &param0, &param1, &param2, &param3, &param4);
                    if (5 == ret) {
                        G_pMediaOnTheFlyControl->UpdatePreviewDisplay(param0, param1, param2, param3, param4);
                        printf("echo: cpre-display:%d,%d,%d,%d,%d\n", param0, param1, param2, param3, param4);
                    } else {
                        printf("change preview display's cmd should be: 'cpre-display:size_x,size_y,offset_x,offset_y,alpha' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "bitrate", 7)){
                    ret = sscanf(buffer, "cbitrate:%d", &param0);
                    if (1 == ret) {
                        G_pMediaOnTheFlyControl->UpdateEncBitrate(param0);
                        printf( "echo: cbitrate:%d\n", param0);
                    } else {
                        printf("change enc bitrate's cmd should be: 'cbitrate:bitrate' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "framerate", 9)){
                    ret = sscanf(buffer, "cframerate:%d,%d", &param0,&param1);
                    if (2 == ret) {
                        G_pMediaOnTheFlyControl->UpdateEncFramerate(param0,param1);
                        printf("echo: cframerate:%d,%d\n", param0,param1);
                    } else {
                        printf("change enc framerate's cmd should be: 'cframerate:framerate_reduce_factor,framerate_integer' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "idr", 3)){
                    ret = sscanf(buffer, "cidr:%d,%d", &param0,&param1);
                    if (2 == ret) {
                        G_pMediaOnTheFlyControl->DemandIDR(param0,param1);
                        printf("echo: cidr:%d,%d\n", param0,param1);
                    } else {
                        printf("demand idr's cmd should be: 'cidr:method,pts' + 'enter' \n");
                    }
                } else if (!strncmp(buffer+1, "gop", 3)){
                    ret = sscanf(buffer, "cgop:%d,%d,%d,%d", &param0, &param1, &param2, &param3);
                    if (4 == ret) {
                        G_pMediaOnTheFlyControl->UpdateEncGop(param0, param1, param2, param3);
                        printf("echo: cgop:%d,%d,%d,%d\n", param0, param1, param2, param3);
                    } else {
                        printf("update gop's cmd should be: 'cgop:M,N,idrinterval,gop' + 'enter' \n");
                    }
                }
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

            default:
                break;
        }
    }

    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
        AM_ERROR("stdin_fileno set error");
}

static void sigstop(int a)
{
    record_running = 0;
}

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    static unsigned int seq_num[2] = {0};
    unsigned int start_index = 0;

    printf("AMF msg: %d\n", msg.code);
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
    }
}

static void check_parameters()
{
    //should in duplex mode if playback enabled
    if (playback_enabled && (DSPMode_DuplexLowdelay != dsp_mode)) {
        dsp_mode = DSPMode_DuplexLowdelay;
        printf("enable playback, should in duplex mode.\n");
    }

    //should have no secondary stream in duplex mode
    if ((DSPMode_DuplexLowdelay == dsp_mode) && (total_muxer_number > 1)) {
        total_muxer_number = 1;
        printf("duplex mode, should have no dual stream encoding.\n");
    }
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

    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    // parse parameters of 'dutest'
    if (init_param(argc, argv) < 0) {
        usage();
        return -1;
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

        if (camera_index<0 || camera_index>1) {
            printf("BAD camera_index %d, use default 0.\n", camera_index);
            camera_index = 0;
        }

        //set preview c size
        printf("start init vin, enter preview, w %d, h %d, camera index %d.\n", DefaultPreviewCWidth, DefaultPreviewCHeight, camera_index);
        err = G_pPreTreat->EnterPreview(DefaultPreviewCWidth, DefaultPreviewCHeight, camera_index);
        AM_ASSERT(ME_OK == err);

        printf("start trigger aaa.\n");
        err = G_pPreTreat->StartImgProc(0);
        printf("trigger aaa done.\n");
    }
#endif

    if (ME_OK != AMF_Init()) {
        AM_ERROR("AMF_Init Error.\n");
        return -2;
    }

//android hard code to 44.1khz
#if PLATFORM_ANDROID
    sample_rate = 44100;
    G_pAudiosink = (new MediaPlayerService::AudioOutput(1));
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

    if ((G_pRecordControl = CreateRecordControlDuplex()) == NULL) {
        AM_ERROR("Cannot create record control\n");
        return -1;
    }

    //get streaming interface
    if (rtsp_enabled) {
        G_pStreamingControl = IStreamingControl::GetInterfaceFrom(G_pRecordControl);
        if (!G_pStreamingControl) {
            printf("!!Error: cannot get IStreamingControl interface.\n");
            return -2;
        }
    }

    //get simple playback interface
//if (playback_enabled) {
    G_pSimplePBControl = ISimplePlayback::GetInterfaceFrom(G_pRecordControl);
    if (!G_pSimplePBControl) {
        printf("!!Error: cannot get ISimplePlayback interface.\n");
        return -3;
    }
#if PLATFORM_ANDROID
    G_pSimplePBControl->SetAudioSink(G_pAudiosink);
#endif
//}

    if (discard_half_audio_packet) {
        printf("before set debug dec property: discard half audio packet.\n");
        G_pSimplePBControl->SetPBProperty(IPBControl::DEC_DEBUG_PROPERTY_DISCARD_HALF_AUDIO_PACKET, 1);
    }

    G_pMediaOnTheFlyControl = IMediaControlOnTheFly::GetInterfaceFrom(G_pRecordControl);

    check_parameters();

    err = G_pRecordControl->SetWorkingMode(dsp_mode, vout_mask);
    if_err_return(err);

    if (not_encoding) {
        printf("before G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_NOT_START_ENCODING, 0).\n");
        G_pRecordControl->SetProperty(IRecordControl2::REC_PROPERTY_NOT_START_ENCODING, 0);
    }

    printf("G_pRecordControl %p,G_pStreamingControl %p, G_pPBControl %p.\n", G_pRecordControl, G_pStreamingControl, G_pSimplePBControl);
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

    for (output_index=0; output_index<total_muxer_number; output_index++) {
        //setup ouput/muxer
        err = G_pRecordControl->SetupOutput(output_index, file_name[output_index], output_format[output_index]);
        if_err_return(err);

        //add video stream
        err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_Video, video_format[output_index]);
        if_err_return(err);

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

        //add audio stream
        err = G_pRecordControl->NewStream(output_index, stream_index, IParameters::StreamType_Audio, audio_format[output_index]);
        if_err_return(err);

#ifdef __app_set_detailed_parameters__
        /********************************************/
        /* audio stream: optional, set parameters, default value */
        /********************************************/
        err = G_pRecordControl->SetAudioStreamChannelNumber(output_index, stream_index, 2);// 2 channels
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamSampleFormat(output_index, stream_index, IParameters::SampleFMT_S16);//S16
        if_err_return(err);
        err = G_pRecordControl->SetAudioStreamBitrate(output_index, stream_index, 128000);//128 kbit/s
        if_err_return(err);
#endif
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
            G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 0);//video
            G_pStreamingControl->EnableStreamming(rtsp_server_index, 0, 1);//audio
        }
        //second stream
        if (rtsp_output_index_mask & 0x2) {
            G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 0);//video
            G_pStreamingControl->EnableStreamming(rtsp_server_index, 1, 1);//audio
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

    //if have playback
    if (playback_start_at_beginning && playback_enabled) {
        printf("playback file(%s) at the beginning.\n", playback_file_name);
        if (G_pSimplePBControl) {
            playback_started = 1;
            err = G_pSimplePBControl->PlayFile(playback_file_name);
            if (ME_OK != err) {
                printf("playback file %s fail, err %d.\n", playback_file_name, err);
            } else {
                printf("playback file(%s) started.\n", playback_file_name);
            }
        }
    }

    RunMainLoop();

    if (playback_started && G_pSimplePBControl) {
        err = G_pSimplePBControl->StopPlay();
        if (ME_OK != err) {
            printf("stop playback file %s fail, err %d.\n", playback_file_name, err);
        }
    }

    if (rtsp_enabled) {
        G_pStreamingControl->RemoveStreamingServer(rtsp_server_index);
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
        G_pRecordControl->ExitPreview();
        printf("ExitPreview okay!\n");

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

#if PLATFORM_ANDROID
    if (G_pAudiosink) {
        delete G_pAudiosink;
        G_pAudiosink = NULL;
    }
#endif

    return 0;
}
