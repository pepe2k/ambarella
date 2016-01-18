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
    data = mxmlNewElement(xml, "osd");
    node = mxmlNewElement(data, "res");
    mxmlElementSetAttr(node, "ul", buffer);
    printf("Content-type: application/xml\n\n");
    mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
    mxmlDelete(xml);

    return 0;

}

static int AmbaOSDPage_set_params_fb () {
    MULTI_MESSAGES * multimsg = NULL;
    MESSAGE* msg = NULL;
    int resultcount = 0;
    int i = 0;
    char* standard = NULL;

    if(receive_buffer->cmd_id ==    MULTI_SET) {
        multimsg = (MULTI_MESSAGES *)receive_buffer;
        msg = (MESSAGE*)multimsg->data;
        standard = multimsg->data;
        for(i = 0; i < multimsg->count; i++) {
            if(msg->status == STATUS_SUCCESS) {
                resultcount++;
            }
            standard = standard + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
            msg = (MESSAGE*)standard;

        }

        if(resultcount == multimsg->count) {
            PrintResult(STATUS_SUCCESS);
        } else {
            PrintResult(STATUS_FAILURE);
        }
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
        AmbaOSDPage_set_params_fb ();
        sem_post(&cmd_sem);
    }
}

int AmbaOSDPage_set_params (HDF *hdf) {
    int ret_code = 0;
    char* file_name;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    mxml_node_t *vnode = NULL;
    FILE* LOG;
    MULTI_MESSAGES* osd_buffer = NULL;
    MESSAGE* msg = NULL;
    uint32_t* time_streamid = NULL;
    uint32_t* bmp_streamid = NULL;
    uint32_t* text_streamid = NULL;
    uint32_t* time_action = NULL;
    uint32_t* bmp_action = NULL;
    uint32_t* text_action = NULL;
    OSDITEM* osd = NULL;

    file_name = hdf_get_value(hdf, "PUT.FileName", "");


    osd_buffer = (MULTI_MESSAGES *)send_buffer;
    osd_buffer->cmd_id = MULTI_SET;
    osd_buffer->count = 3;
    msg = (MESSAGE*)osd_buffer->data;
    msg->cmd_id = SET_OSD_TIME;
    time_streamid = (uint32_t *)(msg->data);
    time_action = (uint32_t *)(msg->data + sizeof(uint32_t));
    msg = (MESSAGE*)(osd_buffer->data + sizeof(ENC_COMMAND_ID) + 3*sizeof(uint32_t));
    msg->cmd_id = SET_OSD_BMP;
    bmp_streamid = (uint32_t *)(msg->data);
    bmp_action = (uint32_t *)(msg->data + sizeof(uint32_t));
    msg = (MESSAGE*)(osd_buffer->data + 2*sizeof(ENC_COMMAND_ID) + 6*sizeof(uint32_t));
    msg->cmd_id = SET_OSD_TEXT;
    text_streamid = (uint32_t *)(msg->data);
    text_action = (uint32_t *)(msg->data + sizeof(uint32_t));
    osd = (OSDITEM*)(msg->data+ 2*sizeof(uint32_t));

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

        //osd
        pnode = mxmlFindElement(tree, tree, "osd", NULL, NULL, MXML_DESCEND);
        if (!pnode) {
            ret_code= -4;
            break;
        }

        //osd_stream
        vnode = mxmlFindElement(pnode, pnode, "osd_stream", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            *time_streamid = atoi(mxmlElementGetAttr (vnode, "ul"));
            *bmp_streamid = atoi(mxmlElementGetAttr (vnode, "ul"));
            *text_streamid = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -5;
            break;
        }

        //time
        vnode = mxmlFindElement(pnode, pnode, "time_enable", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            *time_action = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -6;
            break;
        }

        //bmp
        vnode = mxmlFindElement(pnode, pnode, "bmp_enable", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            *bmp_action = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -7;
            break;
        }

        //text
        vnode = mxmlFindElement(pnode, pnode, "text_enable", NULL, NULL, MXML_DESCEND);
        if(vnode) {
            *text_action = atoi(mxmlElementGetAttr (vnode, "ul"));
        } else {
            ret_code= -8;
            break;
        }

        if(*text_action) {
            //text
            vnode = mxmlFindElement(pnode, pnode, "text", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                sprintf(osd->osd_text.text, "%s", mxmlElementGetAttr (vnode, "ul"));
            }

            //font_size
            vnode = mxmlFindElement(pnode, pnode, "font_size", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.font_size = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //font_color
            vnode = mxmlFindElement(pnode, pnode, "font_color", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.font_color = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //outline
            vnode = mxmlFindElement(pnode, pnode, "outline", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.outline = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //bold
            vnode = mxmlFindElement(pnode, pnode, "bold", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.bold = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //italic
            vnode = mxmlFindElement(pnode, pnode, "italic", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.italic = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //x
            vnode = mxmlFindElement(pnode, pnode, "offset_x", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.offset_x = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //y
            vnode = mxmlFindElement(pnode, pnode, "offset_y", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.offset_y = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //box_w
            vnode = mxmlFindElement(pnode, pnode, "box_w", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.width = atoi(mxmlElementGetAttr (vnode, "ul"));
            }

            //box_h
            vnode = mxmlFindElement(pnode, pnode, "box_h", NULL, NULL, MXML_DESCEND);
            if(vnode) {
                osd->osd_text.height = atoi(mxmlElementGetAttr (vnode, "ul"));
            }
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

    if (AmbaOSDPage_set_params (hdf) < 0) {
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

