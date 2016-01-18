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
#define AR0330_CELL_UNIT_SIZE 2.2

typedef enum {
    IMG_2048x2048 = create_res(2048,2048),
    IMG_2048x1538 = create_res(2048,1538)
}IMGSIZE;

CGI *cgi;
sem_t  cmd_sem;

static void  receive_cb(union sigval sv);

static int AmbaDwpPage_get_params_fb () {
    FisheyeParameters *dwp_in = (FisheyeParameters *)receive_buffer->data;
    FisheyeLayout* layout =  (FisheyeLayout*)(receive_buffer->data + sizeof(FisheyeParameters));
      uint32_t mount;
    mount = dwp_in->mount;
    uint32_t imgcircle;
    imgcircle = dwp_in->max_circle;
    uint32_t maxfov;
    maxfov = dwp_in->max_fov;
    uint32_t projection;
    projection = dwp_in->projection;
    char value[NAME_LEN] = {0};

    sprintf(value, "%d", dwp_in->mount);
    hdf_set_value(cgi->hdf, "mount", value);

    if(maxfov == 185 && imgcircle == 2298 && projection == 0) {
        sprintf(value, "%d", 0);
        hdf_set_value(cgi->hdf, "vendor", value);
    } else if (maxfov == 185 && imgcircle == 3000 && projection == 0) {
        sprintf(value, "%d", 1);
        hdf_set_value(cgi->hdf, "vendor", value);
    } else if (maxfov == 187 && imgcircle == 2322 && projection == 0) {
        sprintf(value, "%d", 2);
        hdf_set_value(cgi->hdf, "vendor", value);
    } else {
        sprintf(value, "%d", 3);
        hdf_set_value(cgi->hdf, "vendor", value);
    }

    sprintf(value, "%d", maxfov);
    hdf_set_value(cgi->hdf, "maxfov", value);

    sprintf(value, "%d", imgcircle);
    hdf_set_value(cgi->hdf, "radium", value);

    sprintf(value, "%d", projection);
    hdf_set_value(cgi->hdf, "type", value);

    if(mount == AM_FISHEYE_MOUNT_CEILING) {
        sprintf(value, "%d", layout->layout - CEILING_FISHEYE);
    } else {
        sprintf(value, "%d", layout->layout);
    }
    hdf_set_value(cgi->hdf, "layout", value);

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
        if (receive_buffer->cmd_id == GET_FISHEYE_PARAMETERS && receive_buffer->status== STATUS_SUCCESS) {
            AmbaDwpPage_get_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaDwpPage_get_params () {
    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    send_buffer->cmd_id = GET_FISHEYE_PARAMETERS;

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


    AmbaDwpPage_get_params ();
    sem_wait(&cmd_sem);
    cgi_display(cgi, "../html/dwp.html");

    return 0;
}
