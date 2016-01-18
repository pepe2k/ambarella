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

sem_t  cmd_sem;


static void  receive_cb(union sigval sv);

static int PrintResult (int result) {
    printf("Content-type: application/json\n\n");
    printf("{\"res\":%d}", result);

    return 0;

}

static int AmbaIQPage_set_params_fb () {
    if (receive_buffer->status == STATUS_SUCCESS) {
        PrintResult(STATUS_SUCCESS);
    } else {
        PrintResult(STATUS_FAILURE);
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
    NotifySetup((mqd_t *)sv.sival_ptr);
    while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
    if (receive_buffer->cmd_id == SET_IQ) {
        AmbaIQPage_set_params_fb ();
    }
    memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
    sem_post(&cmd_sem);
    }
}

int AmbaIQPage_set_params (HDF *hdf) {
    int ret_code = 0;
    char* file_name;
    FILE* LOG;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    mxml_node_t *vnode = NULL;

    file_name = hdf_get_value(hdf, "PUT.FileName", "");

    send_buffer->cmd_id = SET_IQ;
    IMG_QUALITY *iq = (IMG_QUALITY *)send_buffer->data;

    if (!hdf) {
        return -1;
    }

    do{

    LOG = fopen(file_name, "r");
    if (!LOG) {
        ret_code = -2;
        break;
    }

    tree = mxmlLoadFile (NULL, LOG, MXML_TEXT_CALLBACK);
    if (!tree) {
        ret_code =  -18;
        break;
    }

    //iq
    pnode = mxmlFindElement(tree, tree, "imagequality", NULL, NULL, MXML_DESCEND);
    if (!pnode) {
        ret_code= -3;
        break;
    }

    //wb
    vnode = mxmlFindElement(pnode, pnode, "wb", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->wbc = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -4;
        break;
    }

    //shuttermax
    vnode = mxmlFindElement(pnode, pnode, "shuttermax", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->shutter_max = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -5;
        break;
    }

    //shuttermin
    vnode = mxmlFindElement(pnode, pnode, "shuttermin", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->shutter_min = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -6;
        break;
    }

   //sharpness
    vnode = mxmlFindElement(pnode, pnode, "sharpness", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->shapenness = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -7;
        break;
    }

    //contrast
    vnode = mxmlFindElement(pnode, pnode, "contrast", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->contrast = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -7;
        break;
    }

    //brightness
    vnode = mxmlFindElement(pnode, pnode, "brightness", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->brightness = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -9;
        break;
    }

    //saturation
    vnode = mxmlFindElement(pnode, pnode, "saturation", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->saturation = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -10;
        break;
    }

    //maxgain
    vnode = mxmlFindElement(pnode, pnode, "maxgain", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->max_gain = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -11;
        break;
    }

    //antiflicker
    vnode = mxmlFindElement(pnode, pnode, "antiflicker", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->antiflicker = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -12;
        break;
    }

    //irismode
    vnode = mxmlFindElement(pnode, pnode, "irismode", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->dc_iris_mode = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -13;
        break;
    }

    //exptargetfac
    vnode = mxmlFindElement(pnode, pnode, "exptargetfac", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->exposure_target_factor = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -15;
        break;
    }

     //compensation
    vnode = mxmlFindElement(pnode, pnode, "compensation", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->backlight_comp = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -16;
        break;
    }

    //exposure
    vnode = mxmlFindElement(pnode, pnode, "exposure", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->exposure_mode = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -17;
        break;
    }

    //filter
    vnode = mxmlFindElement(pnode, pnode, "filter", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->denoise_filter = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -18;
        break;
    }

     //dnmode
    vnode = mxmlFindElement(pnode, pnode, "dnmode", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        iq->dn_mode = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -19;
        break;
    }

    } while(0);

    //check point validity, but not check ret code
    if (LOG) {
        fclose(LOG);
        remove(file_name);
    }

    if (tree) {
        mxmlDelete(tree);
    }

    return ret_code ;
}

int main()
{
    CGI *cgi = NULL;
    HDF *hdf = NULL;

    hdf_init(&hdf);
    cgi_init(&cgi, hdf);
    sem_init(&cmd_sem, 0, 0);
    hdf_set_value(cgi->hdf,"Config.Upload.TmpDir","/tmp/cgiupload");
    hdf_set_value(cgi->hdf,"Config.Upload.Unlink","0");
    cgi_parse(cgi);

    Setup_MQ(IMG_MQ_SEND, IMG_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    if (AmbaIQPage_set_params (hdf) < 0) {
        PrintResult(STATUS_FAILURE);
        } else {
            while (1){
                if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
                    LOG_MESSG("%s", "mq_send failed!");
                    sleep(1);
                    continue;
            }
            break;
        }
        sem_wait(&cmd_sem);
    }
    //hdf_dump(hdf,"<br>");


    return 0;
}


