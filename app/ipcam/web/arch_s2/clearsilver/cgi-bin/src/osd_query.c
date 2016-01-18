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

static int AmbaOSDPage_get_params_fb () {
    MESSAGE* msg = (MESSAGE*)receive_buffer->data;
    uint32_t* camtype = (uint32_t*)msg->data;
    OSDITEM* osd = NULL;
    char value[NAME_LEN];

    if((msg->cmd_id == GET_CAM_TYPE) && (msg->status == STATUS_SUCCESS)) {
        sprintf(value, "%d", *camtype);
        hdf_set_value(cgi->hdf, "camtype", value);
    }
    msg = (MESSAGE*)(receive_buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    if((msg->cmd_id == GET_OSD) && (msg->status == STATUS_SUCCESS)) {
        osd = (OSDITEM*)(msg->data);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_time_enabled);
        hdf_set_value(cgi->hdf, "time_enable", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_bmp_enabled);
        hdf_set_value(cgi->hdf, "bmp_enable", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text_enabled);
        hdf_set_value(cgi->hdf, "text_enable", value);

        hdf_set_value(cgi->hdf, "text", osd->osd_text.text);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.font_size);
        hdf_set_value(cgi->hdf, "font_size", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.font_color);
        hdf_set_value(cgi->hdf, "font_color", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.outline);
        hdf_set_value(cgi->hdf, "outline", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.bold);
        hdf_set_value(cgi->hdf, "bold", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.italic);
        hdf_set_value(cgi->hdf, "italic", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.offset_x);
        hdf_set_value(cgi->hdf, "offset_x", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.offset_y);
        hdf_set_value(cgi->hdf, "offset_y", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.width);
        hdf_set_value(cgi->hdf, "box_w", value);
        memset(value, 0, NAME_LEN);
        sprintf(value, "%d", osd->osd_text.height);
        hdf_set_value(cgi->hdf, "box_h", value);
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
    MULTI_MESSAGES* multimsg = (MULTI_MESSAGES*)receive_buffer;
    NotifySetup((mqd_t *)sv.sival_ptr);
    while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
        if (multimsg->cmd_id == MULTI_GET && multimsg->count == 2) {
            AmbaOSDPage_get_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaOSDPage_get_params () {
    MESSAGE* msg = NULL;
    uint32_t* streamid = NULL;

    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    MULTI_MESSAGES* buffer = (MULTI_MESSAGES*)send_buffer;
    buffer->cmd_id = MULTI_GET;
    buffer->count = 2;
     msg = (MESSAGE*)buffer->data;
    msg->cmd_id = GET_CAM_TYPE;
    msg = (MESSAGE*)(buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    msg->cmd_id = GET_OSD;
    streamid = (uint32_t*)msg->data;
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


    AmbaOSDPage_get_params ();
    sem_wait(&cmd_sem);
    cgi_display(cgi, "../html/osd.html");

    return 0;
}

