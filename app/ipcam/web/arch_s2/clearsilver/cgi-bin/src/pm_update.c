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
    char buffer[8];
    sprintf(buffer, "%d", result);

    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;

    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "privacymask");
    node = mxmlNewElement(data, "res");
    mxmlElementSetAttr(node, "ul", buffer);
    printf("Content-type: application/xml\n\n");
    mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
    mxmlDelete(xml);

    return 0;

}

static int AmbaPMPage_set_params_fb () {
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
    if (receive_buffer->cmd_id == SET_PRVACY_MASK) {
        AmbaPMPage_set_params_fb ();
    }
    //memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
    sem_post(&cmd_sem);
    }
}

int AmbaPMPage_set_params (HDF *hdf) {
    int ret_code = 0;
    char* file_name;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    mxml_node_t *vnode = NULL;
    FILE* LOG;

    file_name = hdf_get_value(hdf, "PUT.FileName", "");

    send_buffer->cmd_id = SET_PRVACY_MASK;
    PRIVACY_MASK *pm_in = (PRIVACY_MASK *)send_buffer->data;

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
        ret_code= -3;
        break;
    }

    //privacymask
    pnode = mxmlFindElement(tree, tree, "privacymask", NULL, NULL, MXML_DESCEND);
    if (!pnode) {
        ret_code= -4;
        break;
    }

    //pm_action
    vnode = mxmlFindElement(pnode, pnode, "pm_action", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->action = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -5;
        break;
    }

    //pm_id
    vnode = mxmlFindElement(pnode, pnode, "pm_id", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->id = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -6;
        break;
    }

    //pm_left
    vnode = mxmlFindElement(pnode, pnode, "pm_left", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->left= atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -7;
        break;
    }

    //pm_top
    vnode = mxmlFindElement(pnode, pnode, "pm_top", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->top = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -8;
        break;
    }

    //pm_w
    vnode = mxmlFindElement(pnode, pnode, "pm_w", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->width = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -9;
        break;
    }

    //pm_h
    vnode = mxmlFindElement(pnode, pnode, "pm_h", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->height = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -10;
        break;
    }
    #if 0
    //pm_color
    vnode = mxmlFindElement(pnode, pnode, "pm_color", NULL, NULL, MXML_DESCEND);
    if(vnode) {
        pm_in->color = atoi(mxmlElementGetAttr (vnode, "ul"));
    } else {
        ret_code= -10;
        break;
    }

    pm_in->unit = 0;
    #endif
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

    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    if (AmbaPMPage_set_params (hdf) < 0) {
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

