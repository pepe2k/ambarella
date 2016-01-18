
#define NEW_RTSP_CLIENT 1
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>
#include <fcntl.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "audio_if.h"
#include "am_util.h"
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "simple_audio_common.h"
#include "general_interface.h"
#include "simple_audio_sink.h"
#include "resample_filter.h"
#include "simple_audio_recorder.h"

#include "amf_rtspclient.h"


///////////////////////////////////////////

void RunMainLoop()
{
    char buffer_old[1024] = {0};
    char buffer[1024];
    bool flag_run = true;
    int flag_stdin = 0;

    char name[128];
    int index = -1;
    int type = -1;
    int ret = -1;
    flag_stdin = fcntl(STDIN_FILENO,F_GETFL);
    if(fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1)
        printf("stdin_fileno set error.\n");

    while (flag_run) {

        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
            continue;

        if (buffer[0] == '\n')
            buffer[0] = buffer_old[0];

        switch (buffer[0]) {
            case 'q':
                printf("Quit!\n");
                flag_run = false;
                break;
            default:
                break;
        }
    }

    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)

    return;
}

extern "C" int main(int argc, char **argv)
{
    AM_ERR err;
    //start rtsp server
    RtspMediaSession::start_rtsp_server();

    CSimpleAudioRecorder* recorder = CSimpleAudioRecorder::Create();
    if (!recorder) {
        printf("Create recorder failed!\n");
        return -1;
    }
    SAudioInfo info;
    info.bEnableRTSP = AM_TRUE;
    strcpy((char*)info.cURL, "g711.back");
    info.bSave = AM_FALSE;
    strcpy((char*)info.cFileName, "./audio_g711");
    info.eFormateType = AUDIO_ALAW;
    info.uChannel = 1;
    info.uSampleRate = 48000;
    err = recorder->ConfigMe(info);
    if (err != ME_OK) {
        printf("config recorder failed!\n");
        return -1;
    }

    err = recorder->Start();
    if (err != ME_OK) {
        printf("start recorder failed!\n");
        return -1;
    }
    RunMainLoop();

    recorder->Stop();
    recorder->Delete();

    //stop rtsp server
    RtspMediaSession::stop_rtsp_server();
    return 0;
}

