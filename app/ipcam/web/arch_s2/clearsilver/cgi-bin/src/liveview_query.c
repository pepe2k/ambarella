#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

#include "ClearSilver.h"
#include "mxml.h"
#include "utils.h"

#define NAME_LEN 20

CGI *cgi;
sem_t  cmd_sem;
int urlid = 0;

static void  receive_cb(union sigval sv);

static int AmbaLiveView_get_params_fb () {
    MESSAGE* msg = (MESSAGE*)(receive_buffer->data);
    uint32_t* camtype = NULL;
    uint32_t* dptz_zpt = NULL;
    EncodeSize *pstreamsize = NULL;
    char value[NAME_LEN] = {0};

    if(msg->status == STATUS_SUCCESS) {
        camtype = (uint32_t*)msg->data;
        sprintf(value, "%d", *camtype);
        hdf_set_value(cgi->hdf, "camtype", value);
    }

    msg = (MESSAGE *)(receive_buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    if(msg->status == STATUS_SUCCESS) {
        dptz_zpt = (uint32_t*)msg->data;
        sprintf(value, "%d", *dptz_zpt);
        hdf_set_value(cgi->hdf, "dptz_zpt", value);
    }
    msg = (MESSAGE *)(receive_buffer->data + 2*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    if(msg->status == STATUS_SUCCESS) {
        pstreamsize = (EncodeSize *)(msg->data);
        if(pstreamsize->width > 800 && pstreamsize->height > 600) {
            hdf_set_value(cgi->hdf, "width", "800");
            hdf_set_value(cgi->hdf, "height", "600");
        } else {
            sprintf(value, "%d", pstreamsize->width);
            hdf_set_value(cgi->hdf, "width", value);
            memset(value, 0, NAME_LEN);
            sprintf(value, "%d", pstreamsize->height);
            hdf_set_value(cgi->hdf, "height", value);
        }
    }

    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", urlid);
    //LOG_MESSG("URLID IS %d", urlid);
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
    MULTI_MESSAGES* buffer = NULL;
    NotifySetup((mqd_t *)sv.sival_ptr);
    while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
        buffer = (MULTI_MESSAGES*)receive_buffer;
        if (buffer->cmd_id == MULTI_GET && buffer->count == 3) {
            AmbaLiveView_get_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaLiveView_get_params () {
    uint32_t* streamid = NULL;
    MULTI_MESSAGES* buffer = NULL;
    MESSAGE* msg = NULL;

    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    buffer = (MULTI_MESSAGES*)send_buffer;
    buffer->cmd_id = MULTI_GET;
    buffer->count = 3;
    msg = (MESSAGE*)buffer->data;
    msg->cmd_id = GET_CAM_TYPE;
    msg = (MESSAGE*)(buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    msg->cmd_id = GET_DPTZ_ZPT_ENABLE;
    streamid = (uint32_t *)msg->data;
    *streamid = urlid;
    msg = (MESSAGE*)(buffer->data + 2*(sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t)));
    msg->cmd_id = GET_STREAM_SIZE;
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
    }
    /* else if (strstr(stream, "stream=5")) {
        urlid = 4;
    } else if (strstr(stream, "stream=6")) {
        urlid = 5;
    } else if (strstr(stream, "stream=7")) {
        urlid = 6;
    } else if (strstr(stream, "stream=8")) {
        urlid = 7;
    }*/


    AmbaLiveView_get_params ();
    sem_wait(&cmd_sem);
    cgi_display(cgi, "../html/liveview.html");

    return 0;
}

