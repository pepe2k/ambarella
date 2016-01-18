
/**
 * pbtest.cpp
 *
 * History:
 *    2009/12/18 - [Oliver Li] created file
 *    2010/07/20 - [He Zhi] add -sharedfd option
 *
 * desc: testplayer for AMF pb-engine
 *    usage: pbtest mediafile
 *          Quit: press 'q'
 *          Pause: press ' '
 *          Seek: press 'g sec.msec'
 *    options: [-sharedfd : use setDataSource(sharedfd) other than filename ]
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
#include <string.h>
#include <signal.h>

#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_pbif.h"
#include "am_mw.h"
#include "am_util.h"
#if PLATFORM_ANDROID
#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
#endif

#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
#if PLATFORM_ANDROID
#define SHOW_TIME_ON_OSD 0
#else
#define SHOW_TIME_ON_OSD 0
#endif
#endif

#if SHOW_TIME_ON_OSD
//#include <../../../prebuild/third-party/ortp/include/ortp/port.h>
#include <../../../build/include/basetypes.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#endif

extern "C" {
//#include "libavcodec/avcodec.h"
extern void set_dump_config(int start, int end, int start_x, int end_x, int start_y,int end_y, int dump_ext);
}
enum PlayMode
{
    NORMAL_PLAY,
    FF_2X_SPEED,
    FF_4X_SPEED,
    FF_8X_SPEED,
    FF_16X_SPEED,
    FF_30X_SPEED,
    BACKWARD_PLAY,
    FB_1X_SPEED,
    FB_2X_SPEED,
    FB_4X_SPEED,
    FB_8X_SPEED,
};


IPBControl *G_pPBControl;
CEvent *G_pTimerEvent;
CThread *G_pTimerThread;
static char filename[260];
AM_U64	g_half_length;
static unsigned int gbMainLoopRun = 1;

unsigned short g_msg_port_number = 4848;//hard code here
unsigned int g_enable_msg_port = 0;
CSimpleDataBase* g_simple_data_base = NULL;

static char g_workpath[DAMF_MAX_FILENAME_LEN] = {0};
static char g_configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};

static struct timeval g_tvbefore, g_tvafter;

struct PB_MSG
{
	int	id;
	void	*arg;
	IPBControl::PB_DIR	dir;
	IPBControl::PB_SPEED	speed;
	AM_U64	pts;
};

#define MAX_LOG_CONFIG_CNT 10

struct OptionStruct{

	struct slogSection{
		AM_UINT index;
		SLogConfig config;
	};

	AM_UINT use_shared_fd;
	AM_UINT show_time_osd;
	AM_UINT log_index;
	struct slogSection log_section[MAX_LOG_CONFIG_CNT];
    AM_UINT startStepMode;
    AM_UINT startStepCnt;
    AM_UINT dumpMode;
    AM_UINT startDumpCnt;
    AM_UINT endDumpCnt;

    AM_UINT specify_workpath;

    AM_INT play_video_es;
    AM_INT also_demuxflie;

    AM_UINT speciframerate_num, speciframerate_den;

    AM_INT bFitScreen;
    AM_INT idxFitVout;    // 0x01: LCD; 0x02: HDMI; 0x03: LCD&HDMI;
    AM_INT bQuickSeek;    //quick seek at file beginning test, especially for sw pipeline decoder
    AM_INT inXus;    //quick seek at file beginning test, especially for sw pipeline decoder

    AM_UINT set_dspmode_voutmask;
    AM_UINT dspMode;// 4: udec, 5: duplex
    AM_UINT voutMask;

    AM_U8 enable_dewarp;
//debug use
    AM_U8 discard_half_audio;
    AM_U8 enable_color_test;
    AM_U8 tilemode;
    AM_U8 preset_prefetch_count;
    AM_U32 preset_bits_fifo_size;
    AM_U32 preset_ref_cache_size;
    AM_U8 enable_auto_vout;
    AM_U8 enable_loopplay;
};

static struct OptionStruct G_option;

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    AM_INFO("AMF msg: %d\n", msg.code);

    if (msg.code == IMediaControl::MSG_PLAYER_EOS) {
        //gettimeofday(&g_tvafter,NULL);
        AM_INFO("==== Playback end ====, total time about %ld seconds.\n", g_tvafter.tv_sec - g_tvbefore.tv_sec);
    }
}

void PrintHelp()
{
    printf("pbtest usage:\n");
    printf("pbtest filename -option option_argument..:\n");
    printf("options:\n");
    printf("   -step start_frame_number: enter step mode from the frame number == start_frame_number.\n");
    printf("   -dump start_frame_number end_frame_number: dump video decoder data between start and end frame number.\n");
    printf("                                                                          dump what type of data is controlled by decoder(TXT format or BINARY format).\n");
    printf("   -time: show time on HDMI(OSD), linux only, default is enabled.\n");
    printf("   -notime: not show time on HDMI(OSD), linux only.\n");
    printf("   -sharedfd: play android opend file descriptor, android only.\n.\n");


    printf(" [--playvideoes demux_file]: play video es mode, only for hw format.\n");
    printf(" [--specifyvideoframerate num den]: specify video framerate, only for hardware format.\n");
    printf(" [--fitscreen voutIdx]: fit screen to picture size (voutIdx: 1:LCD; 2:HDMI; 3:LCD&HDMI).\n");
    printf(" [--quickSeek delayUS]: quick seek at file beginning test, especially for sw pipeline decoder.\n");
    printf("   --dspmode: dsp working mode, 2 means CameraRecording, 4 means UDEC, 5 means Duplex.\n.\n");
    printf("   --voutmask: request vout mask, 0<<1 means LCD, 1<<1 means HDMI.\n.\n");
    printf(" [--tilemode mode]: specify tilemode for UDEC, mode [0|5].\n");
    printf(" [--prefetch prefetch_frm_count]: enable prefetch in playback.\n");
    printf(" [--fifosize size_value]: specify bits fifo size with unit as MB.\n");
    printf(" [--refcachesize ref_cache_size]: specify ref cache size with unit as KB.\n");
    printf(" [--autovout]: enable vout mode&frame_rate auto detection via codec information in playback.\n");
    printf(" [--autoar]: enable auto ar.\n");
    printf(" [--disableautoar]: disable auto ar.\n");
    printf(" [--dewarp]: enable dewarp test.\n");
    printf(" [--discardhalfaudio]: discard half of audio data for test.\n");

    printf("pbtest on the fly usage:\n");
    printf("   press ' ' + ENTER: pause/resume\n");
    printf("   press 'q' + ENTER: quit\n");
    printf("   press 's' + ENTER: switch to step mode\n");
    printf("   press 'g'xx'' + ENTER: seek to 'xx' second\n");
    printf("   press 'j'xx'' + ENTER: seek to 'xx' or 'xx+/-halffiletime' second, dead loop, press any key to break up.\n");
    printf("   press 'k'xx'' + ENTER: 1.pause; 2.seek to 'xx' or 'xx+/-halffiletime' second; 3.resume, dead loop, press any key to break up.\n");
    printf("   press 'm'xx'' + ENTER: 1.pause; 2.seek to 'xx' or 'xx+/-halffiletime' second, dead loop, press any key to break up; 3.resume.\n");
    printf("   press 'p' + ENTER: print pb-engine/filters current states\n");
    printf("   press 't': tests, sub-option for vout settings:\n");
    printf("       press 'tzoomin:xx': zoomin xx(xx is vout_id, 0 means LCD, 1 means HDMI)\n");
    printf("       press 'tzoomout:xx': zoomout xx\n");
    printf("       press 'tmove:xx,x_off,y_off': set x_offset and y_offset for xx display\n");
    printf("       press 'tsize:xx,x_size,y_size': set x_size and y_size for xx display\n");
    printf("       press 'trotate:xx,rotate': set rotation for xx display, rotate=1 means rotation degree = 90, 0 means ratation degree = 0\n");
    printf("       press 'tenable:xx,enable': set enable/disable for xx display, 1 means enable, 0 means disable\n");
    printf("       press 'tflip:xx,flip': flip display(vertical flip), flip=0 or 1.\n");
    printf("       press 'tmirror:xx,mirror': mirror display(horizontal flip), mirror= 0 or 1.\n");
    printf("       press 'tsrect:x,y,w,h': set source rect(x,y,w,h).\n");
    printf("       press 'tdrect:xx,x,y,w,h': set dest(xx) rect(x,y,w,h).\n");
    printf("       press 'tmode:xx,modex,modey': set vout(xx) source->dest rect scale mode.\n");
}

void PrintPBConfigHelp()
{
    printf("pb.config location: %s/pb.config\n", AM_GetPath(AM_PathConfig));
    printf("pb.config usage:\n");

    printf("Line_1: filter selection\n");
    printf("\tuse active_pb_engine, use active_record_engine, use general decoder, use private audio decoder, encoder, use ffmpeg muxer2, error code record level.\n");
    printf("Line_2: Basic Settings\n");
    printf("\tdisable_audio, disable_video, disable_subtitle, select_vout, select_ppmode, select_engine_ver\n");
    printf("Line_3: Video Decoder Mode Settings\n");
    printf("\tmpeg12, mpeg4, h264, vc1_wmv3, rv40, hybrid_Mpeg4, others\n");
    printf("Line_4 - Line_6:  Deinerlaced Parameters Settings");
    printf("                  See pb.config in detail.\n");
    printf("Line_7: Error Handling/Concealment Settings\n");
    printf("        enable_error_handling, select_concealment_mode, error_concealment_buffer_id, app_behavior\n");
    printf("Line_8: Debug Mode Settings\n");
    printf("        hybrid_mpeg4_input_number, ffmpeg_decoder_selection, force_decode(0: disable,1:enable), validation_only(0: disable,1:enable), force low delay(skip B picture), use general decoder.\n");
    printf("Line_9 - Line_13: Deblock related\n");
    printf("        deblocking_flag, pquant mode, and pquant table.\n");
}

void PrintLogConfigHelp()
{
    printf("log.config location: %s/log.config\n", AM_GetPath(AM_PathConfig));
    printf("log.config usage:\n");

    printf("Line_1: global config\n");
    printf("global log level: 0: NONE, 1: ERROR, 2: WANRING, 3: NORMAL, 4: DEBUG, 5:VERBOSE\n");
    printf("global log option: bit0 Basic info,...\n");
    printf("global log output: bit0 is console, bit1 is logcat(android), bit2 is file(save all AM_* info in pb.log)\n");
    printf("Line_2 - : each module's Log Settings\n");
    printf("                  Format:     LogLevel, LogOptions, LogOutput\n");
    printf("                  LogLevel:	  [0-5]  0: NONE; 1: ERROR; 2: WANRING; 3: NORMAL; 4: DEBUG; 5:VERBOSE;\n");
    printf("                  LogOptions: bit pattern->  | bit5:32 | bit4:16 | bit3:8 | bit2:4 | bit1:2 | bit0:1 |\n");
    printf("                              bit0: 1 Basic info; bit1: pts; bit2: filter state; bit3: performance; bit4: destructor; bit5: binary; bit6: command\n");
    printf("                  LogOutput:  bit pattern->   | bit6:64 | bit5:32 | bit4:16 | bit3:8 | bit2:4 | bit1:2 | bit0:1 |\n");
    printf("                              bit0: console; bit1: logcat; bit2 is file(save AMLOG_* info in pb.log); bit3: dump es as the whole file; bit4: dump es as separate files for each frame; bit5: enable debug feature; bit6: disable debug feature\n");
    printf("                  see pb.config in detail\n");
}

AM_ERR TimerThread(void *context)
{
	while (1) {
		if (G_pTimerEvent->Wait(1000) == ME_OK)
			break;
		//
	}
	return ME_OK;
}

char *trim(char *str)
{
	char *ptr = str;
	int i;

	// skip leading spaces
	for (; *ptr != '\0'; ptr++)
		if (*ptr != ' ')
			break;

	// remove trailing blanks, tabs, newlines
	for (i = strlen(str) - 1; i >= 0; i--)
		if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
			break;
	str[i + 1] = '\0';

	return ptr;
}

int GetPbInfo(IPBControl::PBINFO &info)
{
	AM_ERR err;
	if ((err = G_pPBControl->GetPBInfo(info)) != ME_OK) {
		AM_ERROR("GetPBInfo failed\n");
		return -1;
	}
	return 0;
}

void DoZoomIn()
{
    printf("DoZoomIn.\n");
    AM_INT pos_x, pos_y;
    AM_INT x_size, y_size;
    G_pPBControl->GetCurrentVideoPictureSize(&pos_x, &pos_y, &x_size, &y_size);
    if (x_size > 40 && y_size > 40) {
        G_pPBControl->VideoDisplayZoom(pos_x, pos_y, x_size - 30, y_size - 30);
    }
}

void DoZoomOut()
{
    printf("DoZoomOut.\n");
    AM_INT pos_x, pos_y;
    AM_INT x_size, y_size;
    G_pPBControl->GetCurrentVideoPictureSize(&pos_x, &pos_y, &x_size, &y_size);
    G_pPBControl->VideoDisplayZoom(pos_x, pos_y, x_size + 30, y_size + 30);
}

void DoChangeDisplaySize(int vout_id)
{
    printf("DoChangeDisplayPostion %d.\n", vout_id);
    AM_INT pos_x, pos_y;
    AM_INT x_size, y_size;
    G_pPBControl->GetDisplayPositionSize(vout_id, &pos_x, &pos_y, &x_size, &y_size);
    if (x_size > 20 && y_size > 20) {
        G_pPBControl->SetDisplayPositionSize(vout_id, pos_x, pos_y, x_size - 10, y_size - 10);
    }
}

void DoChangeInputCenter(int input_center_x,int input_center_y)
{
    printf("DoChangeInputCenter,input_center_x %d, input_center_y %d\n", input_center_x, input_center_y);
    G_pPBControl->ChangeInputCenter( input_center_x, input_center_y);
}

void DoUpdateDewarpWidth(unsigned int enable, unsigned int width_top, unsigned int width_bottom)
{
    AM_ERR err = G_pPBControl->SetDeWarpControlWidth(enable, width_top, width_bottom);
    AM_ASSERT(ME_OK == err);
}

void DoMove(int vout_id, int x_off, int y_off)
{
    printf("DoMove %d, x_off %d, y_off %d.\n", vout_id, x_off, y_off);
    AM_INT pos_x, pos_y;
    AM_INT x_size, y_size;
    G_pPBControl->GetDisplayPositionSize(vout_id, &pos_x, &pos_y, &x_size, &y_size);
    G_pPBControl->SetDisplayPositionSize(vout_id, x_off, y_off, x_size-x_off, y_size-y_off);
}

void DoSetDisplaySize(int vout_id, int xsize, int ysize)
{
    printf("DoSize %d, xsize %d, ysize %d.\n", vout_id, xsize, ysize);
    AM_INT pos_x, pos_y;
    AM_INT x_size, y_size;
    G_pPBControl->GetDisplayPositionSize(vout_id, &pos_x, &pos_y, &x_size, &y_size);
    G_pPBControl->SetDisplayPositionSize(vout_id, pos_x, pos_y, xsize, ysize);
}

void DoSetDisplayPositionSize(int vout_id, int x_off, int y_off, int xsize, int ysize)
{
    printf("DoMoveSize %d, x_off %d, y_off %d, xsize %d, ysize %d.\n", vout_id, x_off, y_off, xsize, ysize);
    G_pPBControl->SetDisplayPositionSize(vout_id, x_off, y_off, xsize, ysize);
}

void DoEnableVoutAAR(int enable)
{
    printf("DoEnableVoutAAR enable %d.\n", enable);
    G_pPBControl->EnableVoutAAR(enable);
}

void DoRotate(int vout_id, int rotate)
{
    printf("DoRotate %d, %d.\n", vout_id, rotate);
    if (rotate)
        G_pPBControl->SetDisplayRotation(vout_id, 1);
    else
        G_pPBControl->SetDisplayRotation(vout_id, 0);
}

void DoFlip(int vout_id, int flip)
{
    printf("DoFlip %d, %d.\n", vout_id, flip);
    G_pPBControl->SetDisplayFlip(vout_id, flip);
}

void DoMirror(int vout_id, int mirror)
{
    printf("DoMirror %d, %d.\n", vout_id, mirror);
    G_pPBControl->SetDisplayMirror(vout_id, mirror);
}

void DoEnableVout(int vout_id, int enable)
{
    printf("DoEnableVout %d, %d.\n", vout_id, enable);
    if (enable)
        G_pPBControl->EnableVout(vout_id, 1);
    else
        G_pPBControl->EnableVout(vout_id, 0);
}

void DoSetsrcRect(int xpos, int ypos, int xsize, int ysize)
{
    printf("DoSetsrcRect xpos %d, ypos %d, xsize %d, ysize %d.\n", xpos, ypos, xsize, ysize);
    G_pPBControl->SetVideoSourceRect(xpos, ypos, xsize, ysize);
}

void DoSetdestRect(int vout_id, int xpos, int ypos, int xsize, int ysize)
{
    printf("DoSetdestRect %d, xpos %d, ypos %d, xsize %d, ysize %d.\n", vout_id, xpos, ypos, xsize, ysize);
    G_pPBControl->SetVideoDestRect(vout_id, xpos, ypos, xsize, ysize);
}

void DoSetscaleMode(int vout_id, int mode_x, int mode_y)
{
    printf("DoSetscaleMode %d, mode_x %d, mode_y %d.\n", vout_id, mode_x, mode_y);
    G_pPBControl->SetVideoScaleMode(vout_id, mode_x, mode_y);
}

void DoPause()
{
	AM_ERR err;
	IPBControl::PBINFO info;

	if (GetPbInfo(info) < 0)
		return;

	if (info.state == IPBControl::STATE_STOPPED) {
		AM_ERROR("not playing\n");
		return;
	}

	if (info.state == IPBControl::STATE_PAUSED) {
		err = G_pPBControl->ResumePlay();
		if (err != ME_OK)
			AM_ERROR("Resume failed\n");
		return;
	}
	else {

		err = G_pPBControl->PausePlay();
		if (err != ME_OK)
			AM_ERROR("Paused failed\n");
		//PrintPBInfo(&info);
		return;
	}
}

void DoFast()
{
}

void DoSlow()
{
}

void DoForward()
{
}

void DoBackward()
{
}

void DoStop()
{
}

void DoLoad()
{
}

void DoPrintState()
{
#ifdef AM_DEBUG
    G_pPBControl->PrintState();
#endif
}


void DoGoto(AM_U64 ms)
{
	AM_ERR err;
	IPBControl::PBINFO info;

	if (GetPbInfo(info) < 0)
		return;

	if (info.state == IPBControl::STATE_STOPPED) {
		AM_ERROR("not playing\n");
		return;
	}
	if(info.length <= ms){
		//ms = abs(info.length - 1000);
		AM_ERROR("@@~~~~~~seek time bigger than length, no seek.~~~~~~@@\n");
              return;
	}
	err = G_pPBControl->Seek(ms);
	if (err != ME_OK)
		AM_ERROR("Resume failed\n");
}

void DoSetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option)
{
#ifdef AM_DEBUG
	G_pPBControl->SetLogConfig(index, level, option);
#endif
}

void DoFitScreenToPicture(AM_INT vout)
{
    AM_ERR err;
    AM_INT x_offset = 0, y_offset = 0;
    AM_INT pic_width = 0, pic_height = 0, pic_temp = 0;
    AM_INT vout_width = 0, vout_height = 0;

    err = G_pPBControl->GetVideoPictureSize(&pic_width, &pic_height);
    if (err != ME_OK) {
		AM_ERROR("DoFitScreenToPicture: GetVideoPictureSize failed\n");
        return ;
    }

    // if 16:9, do nothing
    // if non-16:9, fit display to picture size
    if (pic_width * 9  == 16 * pic_height) {
        AM_ERROR("DoFitScreenToPicture: do nothing for 16:9 video\n");
        return ;
    }

    if (vout == 0) {    // LCD
        pic_temp = pic_width;
        pic_width = pic_height;
        pic_height = pic_temp;
    }

    err = G_pPBControl->GetDisplayDimension(vout, &vout_width, &vout_height);
    if (err != ME_OK) {
		AM_ERROR("DoFitScreenToPicture: GetDisplayDimension failed\n");
        return ;
    }

    float ratio_x=0,ratio_y=0,ratio=0;
    ratio_x = (float) pic_width/vout_width;
    ratio_y = (float)pic_height/vout_height;
    ratio = ratio_x>ratio_y?ratio_x:ratio_y;
    if(ratio_x>ratio_y) {
        x_offset=0;
        y_offset=(vout_height-pic_height/ratio)/2;
    } else {
        y_offset=0;
        x_offset=(vout_width-pic_width/ratio)/2;
    }
    pic_width=(float)pic_width/ratio;
    pic_height=(float)pic_height/ratio;
    pic_width=(pic_width+15)&(~15);//align to 16 bytes
    pic_height=(pic_height+15)&(~15);

    if(  ( (x_offset + pic_width) > vout_width ) ||( (y_offset + pic_height) > vout_height )
        || x_offset<0 ||y_offset<0 || pic_width<0 ||pic_height<0) {
        AM_ERROR("DoFitScreenToPicture: out-of-range\n");
    } else {
        G_pPBControl->SetDisplayPositionSize(vout, x_offset, y_offset, pic_width, pic_height);
        AM_INFO("DoFitScreenToPicture: SetDisplayPositionSize vout=%d x_offset=%d y_offset=%d pic_width=%d pic_height=%d\n",
            vout, x_offset, y_offset, pic_width, pic_height);
    }
}

/*A5S DV Playback Test */
void DoA5SPlay(AM_UINT mode)
{
    G_pPBControl->A5SPlayMode(mode);
}

void DoA5SPlayNM(AM_INT start_n, AM_INT end_m)
{
    G_pPBControl->A5SPlayNM(start_n, end_m);
}
/*end A5S DV Test*/

#if SHOW_TIME_ON_OSD
const char *fb_dev = "/dev/fb0";
int fd_fb;
struct fb_var_screeninfo var;
int fb_dev_pitch;
char *fb_dev_base;
char *fb_dev_base_bak;
char *fb_dev_test_1;
char *fb_dev_test_0;
int fb_dev_size;

typedef struct bmp_s {
	FILE *fp;
	int width;
	int height;
	int pitch;
} bmp_t;

bmp_t bmp;

u32 read_dword_le(u8 *buffer)
{
	return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

u16 read_word_le(u8 *buffer)
{
	return (buffer[1] << 8) | buffer[0];
}

#define ALIGN_CENTER			0
#define ALIGN_TOP_OR_LEFT		1
#define ALIGN_BOTTOM_OR_RIGHT		2

int x_align = ALIGN_BOTTOM_OR_RIGHT;
int y_align = ALIGN_TOP_OR_LEFT;

void close_time(void)
{
	if (munmap(fb_dev_base, fb_dev_size) < 0) {
		perror("munmap___fb_dev_base");
	}

	free(fb_dev_test_1);

	close(fd_fb);
}



int do_show_bmp(const char *filename, int xoff, int yoff, int ori)
{
	u8 buffer[54];
	u8 *line_buffer;
	u8 *fb_ptr;
	int i, j;
	int offset;
	int pic_width;
	int pic_height;

	if (filename && filename[0]) {
		bmp.fp  = fopen(filename, "r");
		if (bmp.fp == NULL) {
			perror(filename);
			goto Error;
		}
	}

	if(ori)
		fb_dev_test_0 = fb_dev_test_1;
	else
		fb_dev_test_0 = fb_dev_base;


	if (fread(buffer, sizeof(buffer), 1, bmp.fp) != 1) {
		perror("read1");
		return -1;
	}

	if (buffer[0] != 'B' || buffer[1] != 'M') {
		AM_ERROR("%s is not a bitmap file\n", filename);
		goto Error;
	}

	if (read_word_le(buffer + 28) != 24) {
		AM_ERROR("Please use 24-bit bitmap!\n");
		goto Error;
	}


	offset = read_dword_le(buffer + 10);
	fseek(bmp.fp, offset, SEEK_SET);
	//AM_INFO("seek to %d\n", offset);


	line_buffer=(u8*)malloc(bmp.pitch);
	if (line_buffer == NULL) {
		AM_ERROR("Cannot alloc %d bytes\n", bmp.pitch);
		goto Error;
	}

	pic_height = bmp.height;
	pic_width = bmp.width;

	fb_ptr = (u8*)fb_dev_test_0 + (yoff + pic_height) * fb_dev_pitch + xoff * (var.bits_per_pixel >> 3);

	if (var.bits_per_pixel == 16) {
		for (i = 0; i < pic_height; i++) {
			u8 *ptr = fb_ptr;
			u8 *pline;

			fb_ptr -= fb_dev_pitch;
			ptr = fb_ptr;

			if (fread(line_buffer, bmp.pitch, 1, bmp.fp) != 1) {
				perror("read2");
				goto Error;
			}

			pline = line_buffer;
			for (j = 0; j < pic_width; j++) {
				unsigned b = *pline++;
				unsigned g = *pline++;
				unsigned r = *pline++;
				b >>= 3; g >>= 2; r >>= 3;
				*(u16*)ptr = (r << 11) | (g << 5) | b;
				ptr += 2;
			}
		}
	} else if (var.bits_per_pixel == 32) {
		for (i = 0; i < pic_height; i++) {
			u8 *ptr = fb_ptr;
			u8 *pline;

			fb_ptr -= fb_dev_pitch;
			ptr = fb_ptr;

			if (fread(line_buffer, bmp.pitch, 1, bmp.fp) != 1) {
				perror("read2");
				goto Error;
			}

			pline = line_buffer;
			for (j = 0; j < pic_width; j++) {
				unsigned b = *pline++;
				unsigned g = *pline++;
				unsigned r = *pline++;
				*(u32*)ptr = (255 << 24) | (r << 16) | (g << 8) | b;
				ptr += 4;
			}
		}
	}

	free(line_buffer);
	fclose(bmp.fp);
//	if (ioctl(fd_fb, FBIOPAN_DISPLAY, &var) < 0) {
//		perror("FBIOPAN_DISPLAY");
//		goto Error;
//	}
	return 0;

Error:
	free(line_buffer);
        if (bmp.fp) {
            fclose(bmp.fp);
        }
	if(ori)
		free(fb_dev_test_0);
	else
		if (munmap(fb_dev_test_0, fb_dev_size) < 0) {
			perror("munmap");
		}
	close(fd_fb);
	return -1;
}



void refresh_play_sec(time_t tv_sec)
{
	int xoff,yoff;
	int second;
	int second_tens,second_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;



	second = tv_sec%60;
    if (second < 0) {
        second +=60;
        if (second <0) {
            AM_ERROR("(pbtest:refresh_play_sec)should not comes here.!!!must have bugs.\n");
        }
    }
	second_tens = second/10;
	second_unit = second%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff + bmp.height + 5;



//display play second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


	return;
}

void refresh_play_sec_and_min(time_t tv_sec)
{
	int xoff,yoff;
	int second,minutes;
	int second_tens,second_unit,minutes_tens,minutes_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;


	minutes = (tv_sec%3600)/60;
	second = tv_sec%60;
	second_tens = second/10;
	second_unit = second%10;
	minutes_tens = minutes/10;
	minutes_unit = minutes%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff + bmp.height + 5;



//display play second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);



//display play minutes number
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


	return;
}

void refresh_play_sec_and_min_hour(time_t tv_sec)
{
	int xoff,yoff;
	int second,minutes,hour;
	int second_tens,second_unit,minutes_tens,minutes_unit,hour_tens,hour_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;


	hour = tv_sec/3600;
	minutes = (tv_sec%3600)/60;
	second = tv_sec%60;
	second_tens = second/10;
	second_unit = second%10;
	minutes_tens = minutes/10;
	minutes_unit = minutes%10;
	hour_tens= hour/10;
	hour_unit = hour%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff + bmp.height + 5;



//display play second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);



//display play minutes number
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


//display play hour number
	sprintf(filename,"/usr/local/bin/%d.bmp",hour_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
 	sprintf(filename,"/usr/local/bin/%d.bmp",hour_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


	return;
}


void refresh_sys_sec(time_t tv_sec)
{
	int xoff,yoff;
	int second;
	int second_tens,second_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;



	second = tv_sec%60;
	second_tens = second/10;
	second_unit = second%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff;


//display sys second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


	return;
}


void refresh_sys_sec_and_min(time_t tv_sec)
{
	int xoff,yoff;
	int second,minutes;
	int second_tens,second_unit,minutes_tens,minutes_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;


	minutes = (tv_sec%3600)/60;
	second = tv_sec%60;
	second_tens = second/10;
	second_unit = second%10;
	minutes_tens = minutes/10;
	minutes_unit = minutes%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff;


//display sys second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


//display sys minutes number
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


	return;
}

void refresh_sys_sec_and_min_hour(time_t tv_sec)
{
	int xoff,yoff;
	int second,minutes,hour;
	int second_tens,second_unit,minutes_tens,minutes_unit,hour_tens,hour_unit;
	char filename[50]={0};
	int screen_xoff;
	int screen_yoff;


	hour = tv_sec/3600;
	minutes = (tv_sec%3600)/60;
	second = tv_sec%60;
	second_tens = second/10;
	second_unit = second%10;
	minutes_tens = minutes/10;
	minutes_unit = minutes%10;
	hour_tens= hour/10;
	hour_unit = hour%10;

	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;
	xoff = screen_xoff;
	yoff = screen_yoff;


//display sys second number
	sprintf(filename,"/usr/local/bin/%d.bmp",second_unit);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",second_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


//display sys minutes number
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
	sprintf(filename,"/usr/local/bin/%d.bmp",minutes_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);


//display sys hour number
	sprintf(filename,"/usr/local/bin/%d.bmp",hour_unit);
	xoff -= (bmp.width)*2;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);
 	sprintf(filename,"/usr/local/bin/%d.bmp",hour_tens);
	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp(filename,xoff,yoff,0);



	return;
}

void copy_sys_time_bmp_to_mem(time_t	tv_sec, time_t	tv_sec_previous)
{

	if(!(tv_sec % 60) && (tv_sec_previous % 60) && !(tv_sec % 3600) && (tv_sec_previous % 3600))
		refresh_sys_sec_and_min_hour(tv_sec);
	else if(!(tv_sec % 60) && (tv_sec_previous % 60))
		refresh_sys_sec_and_min(tv_sec);
	else
		refresh_sys_sec(tv_sec);


	return;
}

void copy_play_time_bmp_to_mem(AM_U64 position_previous, AM_U64 position)
{

	if(!(position % 60) && (position_previous % 60) && !(position % 3600) && (position_previous % 3600))
		refresh_play_sec_and_min_hour(position);
	else if(!(position % 60) && (position_previous % 60))
		refresh_play_sec_and_min(position);
	else
		refresh_play_sec(position);


	return;
}



int do_show_original_time(void)
{
	u8 buffer[54];
	int screen_xoff;
	int screen_yoff;
	int xoff,yoff;


	bmp.fp  = fopen("/usr/local/bin/S.bmp", "r");
	if (bmp.fp == NULL) {
		perror(filename);

	}

	if (fread(buffer, sizeof(buffer), 1, bmp.fp) != 1) {
			perror("read1");
	}

	fclose(bmp.fp);

	if (buffer[0] != 'B' || buffer[1] != 'M') {
		AM_ERROR("%s is not a bitmap file\n", "S.bmp");
	}

	bmp.width = read_dword_le(buffer + 18);
	bmp.height = read_dword_le(buffer + 22);
	bmp.pitch = (bmp.width * 3 + 3) & ~3;


	screen_xoff = var.xres - bmp.width;
	screen_yoff = 0;


//display sys second

	xoff = screen_xoff;
	yoff = screen_yoff;
	do_show_bmp("/usr/local/bin/second.bmp",xoff,yoff,1);

//display sys 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

//display sys minutes

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/minutes.bmp",xoff,yoff,1);

//display sys 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

//display sys hour

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/hour.bmp",xoff,yoff,1);

//display sys 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);


//display S:

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/S.bmp",xoff,yoff,1);






//display play second

	xoff = screen_xoff;
	yoff = yoff + bmp.height + 5;
	do_show_bmp("/usr/local/bin/second.bmp",xoff,yoff,1);


//display play 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);


//display play minutes

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/minutes.bmp",xoff,yoff,1);


//display play 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

//display play hour

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/hour.bmp",xoff,yoff,1);


//display play 00

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/0.bmp",xoff,yoff,1);

//display P:

	xoff -= bmp.width;
	yoff = yoff;
	do_show_bmp("/usr/local/bin/P.bmp",xoff,yoff,1);

	return 0;

}

int show_original_time(void)
{

	int rval = 0;



	fd_fb = open(fb_dev, O_RDWR);
	if (fd_fb > 0) AM_INFO("open %s\n", fb_dev);
	else {
              G_option.show_time_osd = 0;
		perror(fb_dev);
		return -1;
	}

	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("FBIOGET_VSCREENINFO");
		rval = -1;
		goto Error1;
	}





	AM_INFO("xres: %d, yres: %d, bpp: %d\n", var.xres, var.yres, var.bits_per_pixel);


	fb_dev_pitch = var.xres * (var.bits_per_pixel >> 3);
	fb_dev_size = fb_dev_pitch * var.yres;
	fb_dev_base = (char*)mmap(NULL, fb_dev_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (fb_dev_base == MAP_FAILED) {
		perror("mmap");
		rval = -1;
		goto Error1;
	}

	AM_INFO("clear osd\n");
	memset(fb_dev_base, 0, fb_dev_size);
	fb_dev_test_1 = (char*)malloc(fb_dev_size);
	memset(fb_dev_test_1, 0, fb_dev_size);
	do_show_original_time();

	memcpy(fb_dev_base,fb_dev_test_1,fb_dev_size);
	if (ioctl(fd_fb, FBIOPAN_DISPLAY, &var) < 0) {
		perror("FBIOPAN_DISPLAY");
		rval = -1;
		goto Error2;
	}

	rval = 0;

	return rval;

Error2:
	if (munmap(fb_dev_base, fb_dev_size) < 0) {
		perror("munmap");
	}

Error1:
	close(fd_fb);
	return rval;
}


#endif

void _get_seek_time(char* pbuffer, AM_UINT* seconds, AM_UINT* ms)
{
    if(!pbuffer || !seconds || !ms)
        return;

    *seconds = atoi(&pbuffer[1]);
    char *pms = strchr(&pbuffer[1], '.');
    if (pms == NULL) {
        *ms = 0;
    }
    else {
        pms++;
        char *enter = strchr(pms, '\n');
        if(enter) *enter = '\0';
        int len = strlen(pms);
        if (len == 1) {
            *ms = atoi(pms) * 100;
        }
        if (len == 2) {
            *ms = atoi(pms) * 10;
        }
        if (len >= 3) {
            *(pms+3) = '\0';
            *ms = atoi(pms);
        }
    }
}

void RunMainLoop()
{
	char buffer_old[128] = {0};
	char buffer[128];
	IPBControl::PBINFO info;
	static int flag_stdin = 0;

#if SHOW_TIME_ON_OSD
	static struct timeval curr_time = {0,0};
	static struct timeval curr_time_bak = {0,0};
	static struct timeval curr_time_diff = {0,0};
	static struct timeval curr_time_diff_previous = {0,0};
	AM_U64	position_previous = 0;
	int zz = 0;
	int jj = 0;

	if (G_option.show_time_osd) {
		info.position= 0;
		show_original_time();
		//gettimeofday(&curr_time, NULL);
		curr_time_bak.tv_sec = curr_time.tv_sec;
	}
#endif
	flag_stdin = fcntl(STDIN_FILENO,F_GETFL);
	if(fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1)
			AM_ERROR("stdin_fileno set error");

	while (gbMainLoopRun) {
		G_pPBControl->GetPBInfo(info);

		if(info.state == IPBControl::STATE_ERROR){
			AM_ERROR("======Play Error!======\n");
			return;
		}

		if(info.state == IPBControl::STATE_COMPLETED){
#if SHOW_TIME_ON_OSD
			if (G_option.show_time_osd) {
				close_time();
			}
#endif
			if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
				AM_ERROR("stdin_fileno set error");
			AM_INFO("======Play Completed!======\n");
			return;
		}

#if SHOW_TIME_ON_OSD
		if (G_option.show_time_osd) {
			//gettimeofday(&curr_time, NULL);

			info.position = (info.position)/1000;
			curr_time_diff.tv_sec = curr_time.tv_sec - curr_time_bak.tv_sec;
			if(curr_time_diff.tv_sec != curr_time_diff_previous.tv_sec){
	//			AM_INFO("11111___pbtest__pb_pos=%lld\n",info.position);
				copy_sys_time_bmp_to_mem(curr_time_diff.tv_sec,curr_time_diff_previous.tv_sec);
				curr_time_diff_previous.tv_sec = curr_time_diff.tv_sec;

				jj = 1;
			}

			if(info.position != position_previous){
	//			AM_INFO("222222___pbtest__pb_pos=%lld\n",info.position);
				copy_play_time_bmp_to_mem(position_previous, info.position);
				position_previous = info.position;
				jj = 1;
			}
			if(jj){
				jj = 0;
				if (ioctl(fd_fb, FBIOPAN_DISPLAY, &var) < 0) {
					perror("FBIOPAN_DISPLAY");
				}
			}
		}
#endif
		//add sleep to avoid affecting the performance
		usleep(10000);
#if SHOW_TIME_ON_OSD
		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
			continue;
#else
		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
			continue;
#endif
		if (buffer[0] == '\n')
			buffer[0] = buffer_old[0];

		switch (buffer[0]) {
		case 'q':	// exit
			printf("Quit\n");
#if SHOW_TIME_ON_OSD
			if (G_option.show_time_osd) {
				close_time();
			}
#endif
			if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
				AM_ERROR("stdin_fileno set error");
			return;

		case ' ':	// pause
			buffer_old[0] = buffer[0];
			printf("DoPause\n");
			DoPause();
			break;

		case 'p':   //print state, debug only
                    if (!strncmp(buffer+1, "speedup", 7)) {
                        G_pPBControl->SetPBProperty(IPBControl::DEC_PROPERTY_SPEEDUP_REALTIME_PLAYBACK, 1);
                        printf("speed up cmd success.\n");
                    } else {
                        printf("DoPrintState.\n");
                        DoPrintState();
                    }
                    break;

		case 'a':	// fast
			printf("DoFast\n");
			DoFast();
			break;

		case 'w':	// slow
			printf("DoSlow\n");
			DoSlow();
			break;

		case 'f':	// set forward speed
			printf("DoForward\n");
			DoForward();
			break;

		case 'b':	// set backward speed
			printf("DoBackward\n");
			DoBackward();
			break;

            case 's':	// step mode
                printf("STEP\n");
                G_pPBControl->Step();
                break;

		case 'l':	// load filename
			printf("DoLoad\n");
			strcpy(filename, trim(buffer + 1));
			DoLoad();
			break;

		case 'g':
			{
                            AM_UINT seconds=0;
                            AM_UINT ms=0;
				_get_seek_time(buffer, &seconds, &ms);
				printf("seek to: %d.%03d\n", seconds, ms);
				DoGoto(seconds * 1000 + ms);	// seek
			}
			break;

            case 'j':
			{
                            AM_UINT cnt=0;
                            AM_UINT seconds=0;
                            AM_UINT ms=0;
				_get_seek_time(buffer, &seconds, &ms);
                            do{
                                    ++cnt;
                                    if(seconds * 1000 + ms > g_half_length)
                                    {
                                        printf("=========================== seek time %d, to: %lld.%03lld, \n", cnt, seconds-(cnt%2)*g_half_length/1000, ms-(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms - (cnt%2)*g_half_length);	// seek
                                    }
                                    else
                                    {
                                        printf("=========================== seek time %d, to: %lld.%03lld, \n", cnt, seconds+(cnt%2)*g_half_length/1000, ms+(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms + (cnt%2)*g_half_length);	// seek
                                    }
                                    usleep(16000);
                                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
                                        continue;
                                    else
                                        break;
                             }while(1);
			}
			break;

                case 'k':
			{
                            AM_UINT cnt=0;
                            AM_UINT seconds=0;
                            AM_UINT ms=0;
				_get_seek_time(buffer, &seconds, &ms);
                            do{
                                    ++cnt;
                                    DoPause();
                                    if(seconds * 1000 + ms > g_half_length)
                                    {
                                        printf("=========================== pause-seek-resume time %d, to: %lld.%03lld, \n", cnt, seconds-(cnt%2)*g_half_length/1000, ms-(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms - (cnt%2)*g_half_length);	// seek
                                    }
                                    else
                                    {
                                        printf("=========================== pause-seek-resume time %d, to: %lld.%03lld, \n", cnt, seconds+(cnt%2)*g_half_length/1000, ms+(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms + (cnt%2)*g_half_length);	// seek
                                    }
                                    DoPause();
                                    usleep(16000);
                                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
                                        continue;
                                    else
                                        break;
                             }while(1);
			}
			break;

              case 'm':
			{
                            AM_UINT cnt=0;
                            AM_UINT seconds=0;
                            AM_UINT ms=0;
				_get_seek_time(buffer, &seconds, &ms);
                            printf("=========================== pause.\n");
                            DoPause();
                            do{
                                    ++cnt;
                                    if(seconds * 1000 + ms > g_half_length)
                                    {
                                        printf("=========================== seek time %d, to: %lld.%03lld, \n", cnt, seconds-(cnt%2)*g_half_length/1000, ms-(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms - (cnt%2)*g_half_length);	// seek
                                    }
                                    else
                                    {
                                        printf("=========================== seek time %d, to: %lld.%03lld, \n", cnt, seconds+(cnt%2)*g_half_length/1000, ms+(cnt%2)*g_half_length%1000);
                                        DoGoto(seconds * 1000 + ms + (cnt%2)*g_half_length);	// seek
                                    }
                                    usleep(16000);
                                    if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
                                        continue;
                                    else
                                        break;
                             }while(1);
                            printf("=========================== resume.\n");
                            DoPause();
			}
			break;

		case '\n':
			break;

		case 'd':
			{
				AM_INT ret = sscanf(buffer, "dlog %d:%d,%d", &G_option.log_section[0].index, &G_option.log_section[0].config.log_level, &G_option.log_section[0].config.log_option);
				if (ret == 3) {
					printf("Set log config: index=%d, log_level=%d, log_option=%d.\n", G_option.log_section[0].index, G_option.log_section[0].config.log_level, G_option.log_section[0].config.log_option);
					DoSetLogConfig(G_option.log_section[0].index, G_option.log_section[0].config.log_level, G_option.log_section[0].config.log_option);
				} else {
					printf("Set dynamic log: input 'dlog filter_index:log_level,log_option' .\n");
				}
			}
			break;

            case 't'://test
                {
                    AM_INT ret, vout_id;
                    if (!strncmp(buffer+1, "zoomin", 6)) {

                            DoZoomIn();

                    } else if (!strncmp(buffer+1, "zoomout", 7)) {

                            DoZoomOut();

                    } else if (!strncmp(buffer+1, "move", 4)) {
                        AM_INT x_off, y_off;
                        ret = sscanf(buffer, "tmove:%d,%d,%d", &vout_id, &x_off, &y_off);
                        if (ret == 3) {
                            DoMove(vout_id, x_off, y_off);
                        }
                    } else if (!strncmp(buffer+1, "size", 4)) {
                        AM_INT x_size, y_size;
                        ret = sscanf(buffer, "tsize:%d,%d,%d", &vout_id, &x_size, &y_size);
                        if (ret == 3) {
                            DoSetDisplaySize(vout_id, x_size, y_size);
                        }
                    } else if (!strncmp(buffer+1, "movsize", 7)) {
                        AM_INT x_off, y_off;
                        AM_INT x_size, y_size;
                        ret = sscanf(buffer, "tmovsize:%d,%d,%d,%d,%d", &vout_id, &x_off, &y_off, &x_size, &y_size);
                        if (ret == 5) {
                            DoSetDisplayPositionSize(vout_id, x_off, y_off, x_size, y_size);
                        }
                    } else if (!strncmp(buffer+1, "enableaar", 9)) {
                        AM_INT enable;
                        ret = sscanf(buffer, "tenableaar:%d", &enable);
                        if (ret == 1) {
                            DoEnableVoutAAR(enable);
                        }
                    } else if (!strncmp(buffer+1, "rotate", 6)) {
                        AM_INT rotate;
                        ret = sscanf(buffer, "trotate:%d,%d", &vout_id, &rotate);
                        if (ret == 2) {
                            DoRotate(vout_id, rotate);
                        }
                    } else if (!strncmp(buffer+1, "enable", 6)) {
                        AM_INT enable;
                        ret = sscanf(buffer, "tenable:%d,%d", &vout_id, &enable);
                        if (ret == 2) {
                            DoEnableVout(vout_id, enable);
                        }
                    } else if (!strncmp(buffer+1, "flip", 4)) {
                        AM_INT flip;
                        ret = sscanf(buffer, "tflip:%d,%d", &vout_id, &flip);
                        if (ret == 2) {
                            DoFlip(vout_id, flip);
                        }
                    } else if (!strncmp(buffer+1, "mirror", 6)) {
                        AM_INT mirror;
                        ret = sscanf(buffer, "tmirror:%d,%d", &vout_id, &mirror);
                        if (ret == 2) {
                            DoMirror(vout_id, mirror);
                        }
                    } else if (!strncmp(buffer+1, "srect", 5)) {
                        AM_INT x, y, w, h;
                        ret = sscanf(buffer, "tsrect:%d,%d,%d,%d", &x, &y, &w, &h);
                        if (ret == 4) {
                            DoSetsrcRect(x, y, w, h);
                        }
                    } else if (!strncmp(buffer+1, "drect", 5)) {
                        AM_INT x, y, w, h;
                        ret = sscanf(buffer, "tdrect:%d,%d,%d,%d,%d", &vout_id, &x, &y, &w, &h);
                        if (ret == 5) {
                            DoSetdestRect(vout_id, x, y, w, h);
                        }
                    } else if (!strncmp(buffer+1, "mode", 4)) {
                        AM_INT modex, modey;
                        ret = sscanf(buffer, "tmode:%d,%d,%d", &vout_id, &modex, &modey);
                        if (ret == 3) {
                            DoSetscaleMode(vout_id, modex, modey);
                        }
                    } else if (!strncmp(buffer+1, "change", 6)) {
                        ret = sscanf(buffer, "tchange:%d", &vout_id);
                        if (ret == 1) {
                            DoChangeDisplaySize(vout_id);
                        }
                    } else if (!strncmp(buffer+1, "center", 6)){
                        AM_INT input_center_x, input_center_y;
                        ret = sscanf(buffer, "tcenter:%d,%d", &input_center_x,&input_center_y);
                        if(ret == 2){
                            DoChangeInputCenter(input_center_x, input_center_y);
                        }
                   } else if (!strncmp(buffer+1, "dewarpw", 7)){
                        AM_UINT enable, width_top, width_bottom;
                        ret = sscanf(buffer + 1, "dewarpw:%d,%d,%d", &enable, &width_top, &width_bottom);
                        printf("ret %d.\n", ret);
                        if(3 == ret){
                            printf("enable %d, width_top %d, width_bottom %d\n", enable, width_top, width_bottom);
                            DoUpdateDewarpWidth(enable, width_top, width_bottom);
                        } else {
                            printf("BAD\n");
                        }
                   }
                }
                break;
            case 'h':
                PrintHelp();
                break;
            case 'c'://A5S DV test
            {
                AM_INT ret;
                if (!strncmp(buffer+1, "play1", 5))
                {
                    DoA5SPlay(NORMAL_PLAY);
                }else if (!strncmp(buffer+1, "play2", 5)){
                    DoA5SPlay(FF_2X_SPEED);
                }else if (!strncmp(buffer+1, "play4", 5)){
                    DoA5SPlay(FF_4X_SPEED);
                }else if (!strncmp(buffer+1, "play8", 5)){
                    DoA5SPlay(FF_8X_SPEED);
                }else if (!strncmp(buffer+1, "back", 4)){
                    DoA5SPlay(FB_1X_SPEED);
                }else if (!strncmp(buffer+1, "playnm", 6)){
                    AM_INT n, m;
                    ret = sscanf(buffer, "cplaynm %d %d", &n, &m);
                    if (ret == 2) {
                        AM_INFO("A5S Play From %d To %d\n", n, m);
                        DoA5SPlayNM(n, m);
                    }
                }else if (!strncmp(buffer+1, "back2", 6)){
                }else{
                    AM_INFO("A5S DV Test Cmd Wrong.\n");
                }
            }//end case 'c'
                break;
            default:
                break;
		}
	}

#if SHOW_TIME_ON_OSD
	close_time();
#endif
	if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
		AM_ERROR("stdin_fileno set error");
}

AM_INT ParseOption(int argc, char **argv)
{
	int i=0;
	if (argc < 2) {
		//AM_INFO("usage: pbtest filename\n");
        PrintHelp();
        PrintPBConfigHelp();
        PrintLogConfigHelp();
		return -1;
	} else if (argc == 2) {
	    if(!strcmp("-h", argv[1])) {
	        PrintHelp();
	        PrintPBConfigHelp();
                PrintLogConfigHelp();
	        return -1;
		} else if(!strcmp("--h", argv[1])) {
	        PrintHelp();
	        PrintPBConfigHelp();
                PrintLogConfigHelp();
	        return -1;
		} else if(!strcmp("--help", argv[1])) {
	        PrintHelp();
	        PrintPBConfigHelp();
                PrintLogConfigHelp();
	        return -1;
		}
	}

	memset(&G_option,0,sizeof(G_option));
        G_option.show_time_osd = 1;
        G_option.dspMode = DSPMode_UDEC;//default is UDEC mode
        G_option.voutMask = (1<<eVoutLCD) | (1<<eVoutHDMI);
#if TARGET_USE_AMBARELLA_S2_DSP
        G_option.tilemode = 5;
        G_option.preset_prefetch_count = 30;
        G_option.preset_bits_fifo_size = 16777216;//16M
        G_option.preset_ref_cache_size = 737280;//720K
#endif
        AM_INFO("default, show time on osd.\n");
	//parse options
	for(i=2;i<argc;i++)
	{
		if(!strcmp("-sharedfd",argv[i])) {
			G_option.use_shared_fd=1;
		} else if (!strcmp("-dlog",argv[i])) {
			if (3!=sscanf(argv[i+1],"%d:%d,%d",&G_option.log_section[G_option.log_index].index, &G_option.log_section[G_option.log_index].config.log_level, &G_option.log_section[G_option.log_index].config.log_option)) {
				AM_ERROR("-dlog argument error, should be 'index:log_level,log_option.\n'");
				return 1;
			}
			G_option.log_index ++;
			i ++;
		} else if(!strcmp("--enablemsgport",argv[i])) {
		        AM_INFO("--enablemsgport, enable msg port.\n");
			g_enable_msg_port =1;
		} else if(!strcmp("--enablecolortest",argv[i])) {
		        printf("--enablecolortest, enable color test.\n");
			G_option.enable_color_test = 1;
		} else if(!strcmp("-time",argv[i])) {
		        AM_INFO("-time, show time on osd.\n");
			G_option.show_time_osd=1;
		} else if(!strcmp("-notime",argv[i])) {
		        AM_INFO("-notime, not show time on osd.\n");
			G_option.show_time_osd=0;
		} else if(!strcmp("--loop",argv[i])) {
		        AM_INFO("--loop, loop play single file.\n");
			G_option.enable_loopplay = 1;
		}  else if(!strcmp("-step",argv[i])) {
			G_option.startStepMode=1;

            if ((i+1)>= argc) {
                AM_INFO("-step, should with start cnt.\n");
                G_option.startStepCnt = 0;
            } else {
                sscanf(argv[i+1], "%d", &G_option.startStepCnt);
                i++;
            }

		} else if (!strcmp("-dump",argv[i])) {
            if ((i+2)>= argc) {
                AM_INFO("-dump, should with start/end cnt.\n");
                G_option.dumpMode = 0;
            } else {
                G_option.dumpMode = 1;
                sscanf(argv[i+1], "%d", &G_option.startDumpCnt);
                sscanf(argv[i+2], "%d", &G_option.endDumpCnt);
                i += 2;
                AM_INFO("-dump, should with start %d/end %d cnt.\n", G_option.startDumpCnt, G_option.endDumpCnt);
                set_dump_config(G_option.startDumpCnt, G_option.endDumpCnt, 0, 0, 0, 0, 0);
            }

            } else if (!strcmp("-path",argv[i])) {
                if ((i+1)>= argc) {
                    AM_INFO("-path, should with path string.\n");
                    G_option.specify_workpath = 0;
                } else {
                    G_option.specify_workpath = 1;
                    snprintf(g_workpath, sizeof(g_workpath), "%s", argv[i+1]);
                    AM_INFO("-path, workpath = %s.\n", g_workpath);
                    i += 1;
                }
            } else if (!strcmp("--playvideoes",argv[i])) {
                if ((i+1)>= argc) {
                    AM_INFO("--playvideoes.\n");
                    G_option.play_video_es = 1;
                } else {
                    AM_INFO("--playvideoes.\n");
                    G_option.play_video_es = 1;
                    sscanf(argv[i+1], "%d", &G_option.also_demuxflie);
                    AM_INFO("--playvideoes, demuxfile %d.\n", G_option.also_demuxflie);
                    i ++;
                }
            } else if (!strcmp("--specifyvideoframerate",argv[i])){
                if ((i+2)>= argc) {
                    AM_INFO("--specifyvideoframerate, should with two integer: num/den.\n");
                } else {
                    sscanf(argv[i+1], "%d", &G_option.speciframerate_num);
                    sscanf(argv[i+2], "%d", &G_option.speciframerate_den);
                    i += 2;
                    AM_INFO("--specifyvideoframerate, num %d, den %d.\n", G_option.speciframerate_num, G_option.speciframerate_den);
                }
            } else if (!strcmp("--fitscreen",argv[i])) {
                AM_INFO("--fitscreen.\n");
                G_option.bFitScreen = 1;
                sscanf(argv[i+1], "%d", &G_option.idxFitVout);
                AM_INFO("--fitscreen, idxFitVout %d.\n", G_option.idxFitVout);
                i++;
            }
            else if(!strcmp("--quickSeek",argv[i])) {//quick seek at file beginning test, especially for sw pipeline decoder(bug#2308 case 2: sw pipeline decoder)
                AM_INFO("--quickSeek.\n");
                G_option.bQuickSeek = 1;
                if((i+1)>= argc)
                {
                    G_option.inXus = 0;
                }
                else
                {
                    sscanf(argv[i+1], "%d", &G_option.inXus);
                }
                AM_INFO("--quickSeek, inXus  %d.\n", G_option.inXus);
                i++;
            } else if (!strcmp("--dspmode",argv[i])) {
                if((i+1) < argc) {
                    G_option.dspMode = atoi(argv[i+1]);
                    G_option.set_dspmode_voutmask = 1;
                }
                AM_WARNING("--dspmode, request mode %d.\n", G_option.dspMode);
                i++;
            } else if (!strcmp("--voutmask",argv[i])) {
                if((i+1) < argc) {
                    G_option.voutMask = atoi(argv[i+1]);
                    G_option.set_dspmode_voutmask = 1;
                }
                AM_WARNING("--voutmask, request voutmask %d.\n", G_option.voutMask);
                i++;
            } else if (!strcmp("--tilemode",argv[i])) {
                if((i+1) < argc) {
                    G_option.tilemode = atoi(argv[i+1]);
                }

                if ((0 == G_option.tilemode) || (5 == G_option.tilemode)) {
                    printf("--tilemode, request tilemode %d.\n", G_option.tilemode);
                } else {
                    AM_ERROR("BAD input param --tilemode %d\n", G_option.tilemode);
                    G_option.tilemode = 0;
                }
                i++;
            } else if (!strcmp("--prefetch",argv[i])) {
                if((i+1) < argc) {
                    G_option.preset_prefetch_count= atoi(argv[i+1]);
                }
                printf("--prefetch, request prefetch count %d.\n", G_option.preset_prefetch_count);
                i++;
            } else if (!strcmp("--fifosize",argv[i])) {
                if((i+1) < argc) {
                        G_option.preset_bits_fifo_size= atoi(argv[i+1]);
                }
                if (G_option.preset_bits_fifo_size>0) {
                    printf("--fifosize, request fifosize %d MB.\n", G_option.preset_bits_fifo_size);
                    G_option.preset_bits_fifo_size *= (1024*1024);
                } else {
                    AM_ERROR("BAD input param --fifosize %d MB\n", G_option.preset_bits_fifo_size);
                }
                i++;
            } else if (!strcmp("--refcachesize",argv[i])) {
                if((i+1) < argc) {
                    G_option.preset_ref_cache_size= atoi(argv[i+1]);
                }
                if (G_option.preset_ref_cache_size>0) {
                    printf("--refcachesize, request refcachesize %d KB.\n", G_option.preset_ref_cache_size);
                    G_option.preset_ref_cache_size *= 1024;
                } else {
                    AM_ERROR("BAD input param --refcachesize %d KB\n", G_option.preset_ref_cache_size);
                }
                i++;
            } else if (!strcmp("--autovout",argv[i])) {
                G_option.enable_auto_vout = 1;
                i++;
            } else if (!strcmp("--dewarp",argv[i])) {
                G_option.enable_dewarp = 1;
                AM_WARNING("--dewarp, enable dewarp test\n");
            } else if (!strcmp("--discardhalfaudio",argv[i])) {
                G_option.discard_half_audio = 1;
                AM_WARNING("--discardhalfaudio, request discard half audio packet, debug only!\n");
            } else {
                AM_ERROR("NOT processed option(%s).\n", argv[i]);
            }
	}
	return 0;
}

static AM_INT _msg_callback(void* thiz, AM_UINT msg_type, void* msg_body)
{
    SSimpleDataPiece* piece;
    AM_UINT i = 0;

    AM_WARNING("debug callback, thiz %p, msg_type %08x\n", thiz, msg_type);

    //debug code: process msg
    switch (msg_type) {
        case AM_MSG_TYPE_NEW_RTSP_URL:
            if (msg_body) {
                piece = (SSimpleDataPiece*)msg_body;
                if (piece->p_name) {
                    AM_WARNING("[msg callback]: source ip %s\n", piece->p_name);
                }
                if (piece->p_url) {
                    AM_WARNING("[msg callback]: rtsp url %s(pre), stream count %d\n", piece->p_url, piece->stream_count);
                }
                for (i = 0; i < piece->stream_count; i++) {
                    AM_WARNING("[msg callback]: stream %d: %s\n", i, piece->stream[i]);
                }
            }
            break;

        default:
            AM_ERROR("to do, for msg %d\n", msg_type);
            break;
    }
    return 0;
}

static void sigstop(int arg)
{
    AM_WARNING("pbtest interruptted to quit.\n");
    gbMainLoopRun = 0;
}

extern "C" int main(int argc, char **argv)
{
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

#if PLATFORM_ANDROID
    AMPlayer::AudioSink *audiosink;
#endif
    if (ParseOption(argc, argv)) {
        return 1;
    }

    if (AMF_Init() != ME_OK) {
        AM_ERROR("AMF_Init() failed\n");
        return -1;
    }

    //specify work path, optional
    if (G_option.specify_workpath == 1) {
        AM_SetPathes(g_workpath, g_workpath);
    }

    //read global cfg from pb.config, optional
    snprintf(g_configfilename, sizeof(g_configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    AM_LoadGlobalCfg(g_configfilename);
    snprintf(g_configfilename, sizeof(g_configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(g_configfilename);
    AM_OpenGlobalFiles();
    AM_GlobalFilesSaveBegin(argv[1]);

    //start msg listener
    if (g_enable_msg_port) {
        if (!g_simple_data_base) {
            g_simple_data_base = new CSimpleDataBase();
        }
        AM_ASSERT(g_simple_data_base);
        g_simple_data_base->Start(NULL, _msg_callback, g_msg_port_number);
    }

	if ((G_pTimerEvent = CEvent::Create()) == NULL) {
		AM_ERROR("Cannot create event\n");
		return -1;
	}
#if PLATFORM_ANDROID
       audiosink = (new MediaPlayerService::AudioOutput(1));
if (!g_GlobalCfg.mUseActivePBEngine) {
	if ((G_pPBControl = CreatePBControl(audiosink)) == NULL) {
		AM_ERROR("Cannot create pbcontrol\n");
		return -1;
	}
} else {
	if ((G_pPBControl = CreateActivePBControl(audiosink)) == NULL) {
		AM_ERROR("Cannot create pbcontrol\n");
		return -1;
	}
}
#else
if (!g_GlobalCfg.mUseActivePBEngine) {
    G_pPBControl = CreatePBControl(NULL);
} else {
    G_pPBControl = CreateActivePBControl(NULL);
}
#endif

    if (G_option.set_dspmode_voutmask) {
        AM_WARNING("pbtest dsp mode(%d), request vout_mask 0x%x.\n", G_option.dspMode, G_option.voutMask);
        G_pPBControl->SetWorkingMode(G_option.dspMode, G_option.voutMask);
    }

    if (G_option.enable_dewarp) {
        AM_WARNING("pbtest enable dewarp feature.\n");
        G_pPBControl->EnableDewarpFeature(1, 0);
    }

    if (G_option.enable_color_test) {
        AM_WARNING("pbtest enable color test.\n");
        G_pPBControl->SetPBProperty(IPBControl::DEC_DEBUG_PROPERTY_COLOR_TEST, 1);
    }

    if (G_option.enable_loopplay) {
        AM_WARNING("pbtest enable loop play\n");
        G_pPBControl->Loopplay(1);
    }

    G_pPBControl->SetPBProperty(IPBControl::DSP_TILE_MODE, G_option.tilemode);
    G_pPBControl->SetPBProperty(IPBControl::DSP_PREFETCH_COUNT, G_option.preset_prefetch_count);
    G_pPBControl->SetPBProperty(IPBControl::DSP_BITS_FIFO_SIZE, G_option.preset_bits_fifo_size);
    G_pPBControl->SetPBProperty(IPBControl::DSP_REF_CACHE_SIZE, G_option.preset_ref_cache_size);
    if (G_option.enable_auto_vout) {
        G_pPBControl->SetPBProperty(IPBControl::DEC_DEBUG_PROPERTY_AUTO_VOUT, G_option.enable_auto_vout);
    }

	if ((G_pPBControl->SetAppMsgCallback(ProcessAMFMsg, NULL)) != ME_OK) {
		AM_ERROR("SetAppMsgCallback failed\n");
		return -1;
	}

	if ((G_pTimerThread = CThread::Create("timer", TimerThread, NULL)) == NULL) {
		AM_ERROR("Create timer thread failed\n");
		return -1;
	}

	AM_ERR err;

        if (G_option.startStepMode) {
            AM_INFO("***start with step mode, %d.\n", G_option.startStepCnt);
            err = G_pPBControl->StartwithStepMode(G_option.startStepCnt);
        }

        if (G_option.play_video_es) {
            AM_INFO("***start with play video es mode, demux file? %d.\n", G_option.also_demuxflie);
            err = G_pPBControl->StartwithPlayVideoESMode(G_option.also_demuxflie, 0, 0);
        }

        if (G_option.speciframerate_num && G_option.speciframerate_den) {
            AM_WARNING("***start with specify video frame rate, num %d, den %d.\n", G_option.speciframerate_num, G_option.speciframerate_den);
            err = G_pPBControl->SpecifyVideoFrameRate(G_option.speciframerate_num, G_option.speciframerate_den);
        }

        if (G_option.discard_half_audio) {
            AM_WARNING("before set dec property, discard half audio packet.\n");
            G_pPBControl->SetPBProperty(IPBControl::DEC_DEBUG_PROPERTY_DISCARD_HALF_AUDIO_PACKET, 1);
        }

    if (!G_option.use_shared_fd) {
        err = G_pPBControl->PrepareFile(argv[1]);
        if( err == ME_OK) {
            if (G_option.bFitScreen) {
                if (G_option.idxFitVout & 0x01) {
                    DoFitScreenToPicture(0);   // LCD
                }
                if (G_option.idxFitVout & 0x02) {
                    DoFitScreenToPicture(1);   // HDMI
                }
            }

            AM_INFO("PrepareFile url=%s done.\n", argv[1]);
            //gettimeofday(&g_tvbefore,NULL);
            err = G_pPBControl->PlayFile(argv[1]);
            AM_INFO("PlayFile url=%s.\n", argv[1]);
            if(G_option.bQuickSeek)//quick seek at file beginning test, especially for sw pipeline decoder(bug#2308 case 2: sw pipeline decoder)
            {
                AM_INFO("QuickSeek to 0 in %d us.\n", G_option.inXus);
                usleep(G_option.inXus);
                DoGoto(0);
            }
        } else {
            AM_ERROR("PrepareFile url=%s failed.\n", argv[1]);
            return -1;
        }
    } else {
        char filetmp[200]={0};
        int fd = open(argv[1], O_RDONLY, 0666);
        if (fd == -1) {
            AM_ERROR("error: open_file fail.\n");
            return -1;
        }
        sprintf(filetmp,"sharedfd://%d:0:0",fd);
        AM_INFO("PrepareFile use sharedfd=%s.\n", filetmp);
        err = G_pPBControl->PrepareFile(filetmp);

        if( err == ME_OK) {
            if (G_option.bFitScreen) {
                if (G_option.idxFitVout & 0x01) {
                    DoFitScreenToPicture(0);   // LCD
                }
                if (G_option.idxFitVout & 0x02) {
                    DoFitScreenToPicture(1);   // HDMI
                }
            }

            AM_INFO("PlayFile use sharedfd=%s.\n", filetmp);
            //gettimeofday(&g_tvbefore,NULL);
            err = G_pPBControl->PlayFile(filetmp);
        }
    }

/* ddr: if return -1 here, 	G_pTimerThread/G_pPBControl/G_pTimerEvent won't be deleted
	if (err != ME_OK) {
		AM_INFO("Cannot play %s\n", argv[1]);
		return -1;
	}

#ifdef AM_DEBUG
	//set log config
	if(G_option.log_index) {
		AM_UINT i=0;
		for(; i<G_option.log_index; i++) {
			G_pPBControl->SetLogConfig(G_option.log_section[i].index, G_option.log_section[i].config.log_level, G_option.log_section[i].config.log_option);
		}
	}
#endif*/
        IPBControl::PBINFO info;
       GetPbInfo(info);
       g_half_length = info.length/2;

	RunMainLoop();

	G_pPBControl->StopPlay();

	G_pTimerEvent->Signal();
	G_pTimerThread->Delete();

	G_pPBControl->Delete();
AM_INFO("G_pPBControl->Delete() done.\n");
	G_pTimerEvent->Delete();
AM_INFO("G_pTimerEvent->Delete() done.\n");
#if PLATFORM_ANDROID
       delete audiosink;
AM_INFO("delete audiosink done.\n");
#endif

    if (g_simple_data_base) {
        g_simple_data_base->Stop();
        delete g_simple_data_base;
        g_simple_data_base = NULL;
    }

//	todo - cause crash
	AMF_Terminate();
AM_INFO("AMF_Terminate() done.\n");

       AM_GlobalFilesSaveEnd(argv[1]);
       AM_CloseGlobalFiles();

	return 0;
}


