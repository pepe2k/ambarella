#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

#include "ClearSilver.h"
#include "mxml.h"
#include "utils.h"

#define NAME_LEN 32
#define STREAM_NUM 8
#define create_fr(rotate,vflip,hflip) ((rotate << 2)|(vflip << 1)|(hflip))

typedef enum {
    HIGHRES_2592x1944 = create_res(2592,1944), //2592 x 1944 (5.0M)
    HIGHRES_2560x1440 = create_res(2560,1440), //2560 x 1440 (3.7M)
    HIGHRES_2304x1296 = create_res(2304,1296), //2304 x 1296 (3.0M)
    HIGHRES_2048x1536 = create_res(2048,1536)  //2048 x 1536 (3.0M)
}HIGHRES_INDEX;

typedef enum {
    FR_NORMAL = create_fr(0,0,0),
    FR_HFLIP = create_fr(0,0,1),
    FR_VFLIP = create_fr(0,1,0),
    FR_ROTATE_90 = create_fr(1,0,0),
    FR_ROTATE_180 = create_fr(0,1,1),
    FR_ROTATE_270 = create_fr(1,1,1)
}FR_INDEX;

CGI *cgi;
sem_t  cmd_sem;
int urlid = 0;
uint32_t camtype;

static void  receive_cb(union sigval sv);

static int AmbaEncPage_get_params_fb () {
    MESSAGE *msg = NULL;
    uint32_t* tmp = NULL;
    EncodeType* encodetype = NULL;
    uint32_t* framerate = NULL;
    EncodeSize *pstreamsize = NULL;
    uint16_t *h264_n = NULL;
    uint8_t *idr_interval = NULL;
    uint8_t *profile = NULL;
    uint32_t* bitrate = NULL;
    uint8_t *quality = NULL;
    char value[NAME_LEN] = {0};

    msg = (MESSAGE*)(receive_buffer->data);
    if(msg->status == STATUS_SUCCESS) {
        tmp = (uint32_t*)msg->data;
        camtype = *tmp;
        sprintf(value, "%d", camtype);
        hdf_set_value(cgi->hdf, "camtype", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    if(msg->status == STATUS_SUCCESS) {
        encodetype = (EncodeType *)(msg->data);
        sprintf(value, "%d", *encodetype);
        hdf_set_value(cgi->hdf, "encodetype", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 2*sizeof(ENC_COMMAND_ID) + 2*sizeof(uint32_t) \
		+ sizeof(uint32_t) + sizeof(EncodeType));
    if(msg->status == STATUS_SUCCESS) {
        framerate = (uint32_t *)(msg->data);
        sprintf(value, "%d", *framerate);
        //LOG_MESSG("fps is %d", *framerate);
        hdf_set_value(cgi->hdf, "fps", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 3*sizeof(ENC_COMMAND_ID) + 3*sizeof(uint32_t) \
		+ 2*sizeof(uint32_t) + sizeof(EncodeType));
    if(msg->status == STATUS_SUCCESS) {
        pstreamsize = (EncodeSize *)(msg->data);
        //LOG_MESSG("wid is %d, h is %d, r is %d", pstreamsize->width, pstreamsize->height, pstreamsize->rotate);
        if(pstreamsize->width == 1920 && pstreamsize->height == 1080) {
            sprintf(value, "%d", RES_1920x1080);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 1440 && pstreamsize->height == 1080) {
            sprintf(value, "%d", RES_1440x1080);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 1280 && pstreamsize->height == 1024) {
            sprintf(value, "%d", RES_1280x1024);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 1280 && pstreamsize->height == 960) {
            sprintf(value, "%d", RES_1280x960);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 1280 && pstreamsize->height == 720) {
            sprintf(value, "%d", RES_1280x720);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 800 && pstreamsize->height == 600) {
            sprintf(value, "%d", RES_800x600);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 720 && pstreamsize->height == 576) {
            sprintf(value, "%d", RES_720x576);
            hdf_set_value(cgi->hdf, "resolution", value);
        }  else if(pstreamsize->width == 720 && pstreamsize->height == 480) {
            sprintf(value, "%d", RES_720x480);
            hdf_set_value(cgi->hdf, "resolution", value);
        }  else if(pstreamsize->width == 640 && pstreamsize->height == 480) {
            sprintf(value, "%d", RES_640x480);
            hdf_set_value(cgi->hdf, "resolution", value);
        }  else if(pstreamsize->width == 352 && pstreamsize->height == 288) {
            sprintf(value, "%d", RES_352x288);
            hdf_set_value(cgi->hdf, "resolution", value);
        }  else if(pstreamsize->width == 352 && pstreamsize->height == 240) {
            sprintf(value, "%d", RES_352x240);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 320 && pstreamsize->height == 240) {
            sprintf(value, "%d", RES_320x240);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 2048 && pstreamsize->height == 2048){
            sprintf(value, "%d", RES_2048x2048);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 1024 && pstreamsize->height == 1024){
            sprintf(value, "%d", RES_1024x1024);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 2048 && pstreamsize->height == 1024){
            sprintf(value, "%d", RES_2048x1024);
            hdf_set_value(cgi->hdf, "resolution", value);
        } else if(pstreamsize->width == 3200 && pstreamsize->height == 800){
            sprintf(value, "%d", RES_3200x800);
            hdf_set_value(cgi->hdf, "resolution", value);
        }
    }
    //LOG_MESSG("resolution is %s", value);
    sprintf(value, "%d", pstreamsize->rotate);
    hdf_set_value(cgi->hdf, "fliprotate", value);

    msg = (MESSAGE *)(receive_buffer->data + 4*sizeof(ENC_COMMAND_ID) + 4*sizeof(uint32_t) + \
        2*sizeof(uint32_t) + sizeof(EncodeType) + sizeof(EncodeSize));
    if(msg->status == STATUS_SUCCESS) {
        h264_n = (uint16_t *)(msg->data);
        sprintf(value, "%d", *h264_n);
        hdf_set_value(cgi->hdf, "h264_n", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 5*sizeof(ENC_COMMAND_ID) + 5*sizeof(uint32_t) + \
        2*sizeof(uint32_t) + sizeof(EncodeType) + sizeof(EncodeSize) + sizeof(uint16_t));
    if(msg->status == STATUS_SUCCESS) {
        idr_interval = (uint8_t *)(msg->data);
        sprintf(value, "%d", *idr_interval);
        hdf_set_value(cgi->hdf, "idr_interval", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 6*sizeof(ENC_COMMAND_ID) + 6*sizeof(uint32_t) + \
        2*sizeof(uint32_t) + sizeof(EncodeType) + sizeof(EncodeSize) + sizeof(uint16_t) + sizeof(uint8_t));
    if(msg->status == STATUS_SUCCESS) {
        profile = (uint8_t *)(msg->data);
        sprintf(value, "%d", *profile);
        hdf_set_value(cgi->hdf, "profile", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 7*sizeof(ENC_COMMAND_ID) + 7*sizeof(uint32_t) + \
        2*sizeof(uint32_t) + sizeof(EncodeType) + sizeof(EncodeSize) + sizeof(uint16_t) + 2*sizeof(uint8_t));
    if(msg->status == STATUS_SUCCESS) {
        bitrate = (uint32_t *)(msg->data);
        sprintf(value, "%d", *bitrate);
        hdf_set_value(cgi->hdf, "cbr_avg_bps", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + 8*sizeof(ENC_COMMAND_ID) + 8*sizeof(uint32_t) + \
        3*sizeof(uint32_t) + sizeof(EncodeType) + sizeof(EncodeSize) + sizeof(uint16_t) + 2*sizeof(uint8_t));
    if(msg->status == STATUS_SUCCESS) {
        quality = (uint8_t *)(msg->data);
        sprintf(value, "%d", *quality);
        hdf_set_value(cgi->hdf, "mjpeg_quality", value);
    }

    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", urlid);
    hdf_set_value(cgi->hdf, "streamid", value);

    return 0;
}

static void NotifySetup(mqd_t *mqdp) {
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
    sev.sigev_notify_function = receive_cb;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
    if (mq_notify(*mqdp, &sev) == -1){
        LOG_MESSG("%s", "mq_notify");
    }
}

static void  receive_cb(union sigval sv) {
    NotifySetup((mqd_t *)sv.sival_ptr);
    while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
    MULTI_MESSAGES* buffer = (MULTI_MESSAGES*)receive_buffer;
    if (receive_buffer->cmd_id == MULTI_GET && buffer->count == 9) {
        AmbaEncPage_get_params_fb ();
    }
    sem_post(&cmd_sem);
    }
}



int AmbaEncPage_get_params () {
    MESSAGE *msg = NULL;
    uint32_t* streamid = NULL;
    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    MULTI_MESSAGES *buffer = (MULTI_MESSAGES *)send_buffer;
    buffer->cmd_id = MULTI_GET;
    buffer->count = 9;
    msg = (MESSAGE *)buffer->data;

    msg->cmd_id = GET_CAM_TYPE;

    msg = (MESSAGE*)(buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    msg->cmd_id = GET_STREAM_TYPE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 2*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_STREAM_FRAMERATE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 3*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_STREAM_SIZE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 4*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_H264_N;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 5*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_IDR_INTERVAL;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 6*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_PROFILE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 7*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_CBR_BITRATE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    msg = (MESSAGE *)(buffer->data + 8*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_MJPEG_Q;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;

    while (1){
        if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
        	LOG_MESSG("%s", "mq_send failed!");
        	sleep(1);
        	continue;
        }
        break;
    }
    return 0;
}


int main()
{
    HDF *hdf = NULL;
    char* stream = NULL;

    hdf_init(&hdf);
    cgi_init(&cgi, hdf);
    cgi_parse(cgi);
    sem_init(&cmd_sem, 0, 0);

    stream = hdf_get_value(cgi->hdf,"Query.stream",NULL);
    if(strstr(stream, "1")) {
        urlid = 0;
    } else if (strstr(stream, "2")) {
        urlid = 1;
    } else if (strstr(stream, "3")) {
        urlid = 2;
    } else if (strstr(stream, "4")) {
        urlid = 3;
    }/* else if (strstr(stream, "stream=5")) {
        urlid = 4;
    } else if (strstr(stream, "stream=6")) {
        urlid = 5;
    } else if (strstr(stream, "stream=7")) {
        urlid = 6;
    } else if (strstr(stream, "stream=8")) {
        urlid = 7;
    }*/

    AmbaEncPage_get_params ();
    sem_wait(&cmd_sem);
    if(camtype == WARP_MODE) {
        cgi_display(cgi, "../html/dwp_enc.html");
    } else {
        cgi_display(cgi, "../html/enc.html");
    }

    return 0;
}

