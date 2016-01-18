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

static void  receive_cb(union sigval sv);

static int AmbaPMPage_get_params_fb () {
    MESSAGE* msg = (MESSAGE*)receive_buffer->data;
    char name[NAME_LEN] = {0};
    uint32_t *camtype = (uint32_t*)(msg->data);
    uint32_t* pmid;

    if((msg->cmd_id == GET_CAM_TYPE) && (msg->status == STATUS_SUCCESS)) {
        sprintf(name, "%d", *camtype);
        hdf_set_value(cgi->hdf, "camtype", name);
    }
    msg = (MESSAGE*)(receive_buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    if((msg->cmd_id == GET_PRVACY_MASK) && (msg->status == STATUS_SUCCESS)) {
        pmid = (uint32_t *)(msg->data);
        sprintf(name, "%d", *pmid);
        hdf_set_value(cgi->hdf, "pm_id", name);
    }
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
        if (multimsg->cmd_id == MULTI_GET && multimsg->count== 2) {
            AmbaPMPage_get_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaPMPage_get_params () {
    MESSAGE* msg = NULL;
    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    MULTI_MESSAGES* buffer = (MULTI_MESSAGES*)send_buffer;
    buffer->cmd_id = MULTI_GET;
    buffer->count = 2;
    msg = (MESSAGE*)buffer->data;
    msg->cmd_id = GET_CAM_TYPE;
    msg = (MESSAGE*)(buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
    msg->cmd_id = GET_PRVACY_MASK;

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

    AmbaPMPage_get_params ();
    sem_wait(&cmd_sem);
    cgi_display(cgi, "../html/pm.html");

    return 0;
}

