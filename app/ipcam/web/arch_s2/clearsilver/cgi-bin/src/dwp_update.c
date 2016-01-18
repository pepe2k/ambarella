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

sem_t  cmd_sem;

static void  receive_cb(union sigval sv);

static int PrintResult (int result) {
    char buffer[8];
    sprintf(buffer, "%d", result);

    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;

    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "dewarp");
    node = mxmlNewElement(data, "res");
    mxmlElementSetAttr(node, "ul", buffer);
    printf("Content-type: application/xml\n\n");
    mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
    mxmlDelete(xml);

    return 0;

}

static int AmbaDwpPage_set_params_fb () {
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
        if (receive_buffer->cmd_id == SET_FISHEYE_PARAMETERS) {
            AmbaDwpPage_set_params_fb ();
        }
        sem_post(&cmd_sem);
    }
}

int AmbaDwpPage_set_params (HDF *hdf) {
    int ret_code = 0;
    char* file_name;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    mxml_node_t *vnode = NULL;
    FILE* LOG;
    int mount = 0;
    int layout = 0;
    uint32_t imgwidth = 2048;
    uint32_t imgheight = 2048;
    uint32_t new_imgwidth = 3200;
    uint32_t new_imgheight = 800;
    uint32_t maxcircle = 0;
    MESSAGE* dwp_buffer = NULL;
    FisheyeParameters *fisheye = NULL;
    FisheyeLayout* fisheyelayout = NULL;

    file_name = hdf_get_value(hdf, "PUT.FileName", "");


    dwp_buffer = (MESSAGE *)send_buffer;
    dwp_buffer->cmd_id = SET_FISHEYE_PARAMETERS;
    fisheye = (FisheyeParameters *)dwp_buffer->data;
    fisheyelayout =  (FisheyeLayout*)(dwp_buffer->data + sizeof(FisheyeParameters));

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

        //dewarp
        pnode = mxmlFindElement(tree, tree, "dewarp", NULL, NULL, MXML_DESCEND);
        if (!pnode) {
            ret_code= -4;
            break;
        }

        //mount
        vnode = mxmlFindElement(pnode, pnode, "mount", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            mount = atoi(mxmlElementGetAttr (vnode, "ul"));
            fisheye->mount = mount;
        } else {
            ret_code= -5;
            break;
        }

        //maxfov
        vnode = mxmlFindElement(pnode, pnode, "maxfov", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            fisheye->max_fov = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -6;
            break;
        }

        //radium
        vnode = mxmlFindElement(pnode, pnode, "radium", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            maxcircle = atoi(mxmlElementGetAttr (vnode, "ul"));
            fisheye->max_circle = (uint32_t)(maxcircle);//*100000/155);
        } else {
            ret_code= -7;
            break;
        }

        //type
        vnode = mxmlFindElement(pnode, pnode, "type", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            fisheye->projection = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -8;
            break;
        }

        //layout
        vnode = mxmlFindElement(pnode, pnode, "layout", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            layout =  atoi(mxmlElementGetAttr (vnode, "ul"));
            if (mount == AM_FISHEYE_MOUNT_CEILING) {
                layout = layout + CEILING_FISHEYE;
            }
            if( layout == CEILING_PANORAMA360) {
                fisheye->layout.unwarp_window.width = maxcircle;
                fisheye->layout.unwarp_window.height = maxcircle;
                fisheye->layout.unwarp_window.x = 0;
                fisheye->layout.unwarp_window.y = 0;
                fisheye->layout.unwarp.width = new_imgwidth/2;
                fisheye->layout.unwarp.height = new_imgheight*2;
                fisheye->layout.warp.width = new_imgwidth;
                fisheye->layout.warp.height = new_imgheight;
            } else {
                fisheye->layout.unwarp_window.width = maxcircle;
                fisheye->layout.unwarp_window.height = maxcircle;
                fisheye->layout.unwarp_window.x = 0;
                fisheye->layout.unwarp_window.y = 0;
                fisheye->layout.unwarp.width = imgwidth;
                fisheye->layout.unwarp.height = imgheight;
                fisheye->layout.warp.width = imgwidth;
                fisheye->layout.warp.height = imgheight;
            }
            fisheyelayout->layout = layout;
        } else {
            ret_code= -9;
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

    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    if (AmbaDwpPage_set_params (hdf) < 0) {
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
