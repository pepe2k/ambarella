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
#define SHUTTER_1BY10_SEC				51200000		//(512000000 / 10)
#define SHUTTER_1BY15_SEC				34133333		//(512000000 / 15)
#define SHUTTER_1BY25_SEC				20480000		//(512000000 / 25)
#define SHUTTER_1BY29_97_SEC			17083750		//(512000000 / 2997 * 100)
#define SHUTTER_1BY30_SEC				17066667		//(512000000 / 30)
#define SHUTTER_1BY50_SEC				10240000		//(512000000 / 50)
#define SHUTTER_1BY60_SEC				8533333		//(512000000 / 60)
#define SHUTTER_1BY100_SEC				5120000		//(512000000 / 100)
#define SHUTTER_1BY120_SEC				4266667		//(512000000 / 120)
#define SHUTTER_1BY240_SEC				2133333		//(512000000 / 240)
#define SHUTTER_1BY480_SEC				1066667		//(512000000 / 480)
#define SHUTTER_1BY960_SEC				533333		//(512000000 / 960)
#define SHUTTER_1BY1024_SEC				500000		//(512000000 / 1024)
#define SHUTTER_1BY8000_SEC				64000		//(512000000 / 8000)
CGI *cgi;
sem_t  cmd_sem;

static void  receive_cb(union sigval sv);

static int AmbaIQPage_get_params_fb () {
    IMG_QUALITY *iq = (IMG_QUALITY*)receive_buffer->data;
    char wb[11][17]={"Auto", "Incandescent", "D4000", "D5000", "Sunny", "Cloudy", "Flash", "Fluorescent", "Fluorescent High", "Under Water", "Custom"};
    char value[NAME_LEN];

    sprintf(value, "%d", iq->dn_mode);
    hdf_set_value(cgi->hdf, "dnmode", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->denoise_filter);
    hdf_set_value(cgi->hdf, "filter", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->exposure_mode);
    hdf_set_value(cgi->hdf, "exposure", value);
     memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->backlight_comp);
    hdf_set_value(cgi->hdf, "compensation", value);
    memset(value, 0, NAME_LEN);
    switch(iq->shutter_min) {
        case SHUTTER_1BY8000_SEC:
            strcpy(value, "1/8000");
            break;
        case SHUTTER_1BY1024_SEC:
            strcpy(value, "1/1024");
            break;
        case SHUTTER_1BY960_SEC:
            strcpy(value, "1/960");
            break;
        case SHUTTER_1BY480_SEC:
            strcpy(value, "1/480");
            break;
        case SHUTTER_1BY240_SEC:
            strcpy(value, "1/240");
            break;
        case SHUTTER_1BY120_SEC:
            strcpy(value, "1/120");
            break;
        case SHUTTER_1BY100_SEC:
            strcpy(value, "1/100");
            break;
        case SHUTTER_1BY60_SEC:
            strcpy(value, "1/60");
            break;
        case SHUTTER_1BY50_SEC:
            strcpy(value, "1/50");
            break;
        case SHUTTER_1BY30_SEC:
            strcpy(value, "1/30");
            break;
        case SHUTTER_1BY29_97_SEC:
            strcpy(value, "1/29.97");
            break;
        case SHUTTER_1BY25_SEC:
            strcpy(value, "1/25");
            break;
    }
    hdf_set_value(cgi->hdf, "shuttermin", value);
    memset(value, 0, NAME_LEN);
    switch(iq->shutter_max) {
        case SHUTTER_1BY8000_SEC:
            strcpy(value, "1/8000");
            break;
        case SHUTTER_1BY1024_SEC:
            strcpy(value, "1/1024");
            break;
        case SHUTTER_1BY960_SEC:
            strcpy(value, "1/960");
            break;
        case SHUTTER_1BY480_SEC:
            strcpy(value, "1/480");
            break;
        case SHUTTER_1BY240_SEC:
            strcpy(value, "1/240");
            break;
        case SHUTTER_1BY120_SEC:
            strcpy(value, "1/120");
            break;
        case SHUTTER_1BY100_SEC:
            strcpy(value, "1/100");
            break;
        case SHUTTER_1BY60_SEC:
            strcpy(value, "1/60");
            break;
        case SHUTTER_1BY50_SEC:
            strcpy(value, "1/50");
            break;
        case SHUTTER_1BY30_SEC:
            strcpy(value, "1/30");
            break;
        case SHUTTER_1BY29_97_SEC:
            strcpy(value, "1/29.97");
            break;
        case SHUTTER_1BY25_SEC:
            strcpy(value, "1/25");
            break;
        case SHUTTER_1BY15_SEC:
            strcpy(value, "1/15");
            break;
        case SHUTTER_1BY10_SEC:
            strcpy(value, "1/10");
            break;
    }
    hdf_set_value(cgi->hdf, "shuttermax", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->exposure_target_factor);
    hdf_set_value(cgi->hdf, "exptargetfac", value);
     memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->dc_iris_mode);
    hdf_set_value(cgi->hdf, "irismode", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->antiflicker);
    hdf_set_value(cgi->hdf, "antiflicker", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->max_gain);
    hdf_set_value(cgi->hdf, "maxgain", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->saturation);
    hdf_set_value(cgi->hdf, "saturation", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->brightness);
    hdf_set_value(cgi->hdf, "brightness", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->contrast);
    hdf_set_value(cgi->hdf, "contrast", value);
    memset(value, 0, NAME_LEN);
    sprintf(value, "%d", iq->shapenness);
    hdf_set_value(cgi->hdf, "sharpness", value);
    hdf_set_value(cgi->hdf, "wb", wb[iq->wbc]);
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
        if (receive_buffer->cmd_id == GET_IQ && receive_buffer->status == STATUS_SUCCESS) {
            AmbaIQPage_get_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaIQPage_get_params () {
    Setup_MQ(IMG_MQ_SEND, IMG_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    send_buffer->cmd_id = GET_IQ;

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

    hdf_init(&hdf);
    cgi_init(&cgi, hdf);
    sem_init(&cmd_sem, 0, 0);

    AmbaIQPage_get_params ();
    sem_wait(&cmd_sem);
    cgi_display(cgi, "../html/iq.html");

    return 0;
}

