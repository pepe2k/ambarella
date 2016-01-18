#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

#include "ClearSilver.h"
#include "mxml.h"
#include "utils.h"

/*Must have feedback to browser*/
static int PrintResult () {
    char buffer[8];
    sprintf(buffer, "%d", STATUS_SUCCESS);

    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;

    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "liveview");
    node = mxmlNewElement(data, "res");
    mxmlElementSetAttr(node, "ul", buffer);
    printf("Content-type: application/xml\n\n");
    mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
    mxmlDelete(xml);
    return 0;
}

int AmbaLiveView_set_params (HDF *hdf, int bufferid) {
    int ret_code = 0;
    char* file_name;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    mxml_node_t *qnode = NULL;
    mxml_node_t *vnode = NULL;
    FILE* LOG;
    DPTZ_PARAM *dptz = NULL;
    TRANS_PARAM* zpt = NULL;

    file_name = hdf_get_value(hdf, "PUT.FileName", "");


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

        //liveview
        pnode = mxmlFindElement(tree, tree, "liveview", NULL, NULL, MXML_DESCEND);
        if (pnode) {
            send_buffer->cmd_id = SET_DPTZ;
            dptz = (DPTZ_PARAM *)send_buffer->data;

            //action
            vnode = mxmlFindElement(pnode, pnode, "action", NULL, NULL, MXML_DESCEND);
            if (vnode) {
                dptz->source_buffer = bufferid;
                dptz->offset_x = atoi(mxmlElementGetAttr (vnode, "x"));
                dptz->offset_y = atoi(mxmlElementGetAttr (vnode, "y"));
                dptz->zoom_factor = atoi(mxmlElementGetAttr (vnode, "z"));
            } else{
                ret_code = -6;
                break;
            }
        }

         //fishcam ZPT
        qnode = mxmlFindElement(tree, tree, "fishcam", NULL, NULL, MXML_DESCEND);
        if (qnode) {
            send_buffer->cmd_id = SET_TRANSFORM_ZPT;
            zpt = (TRANS_PARAM*)(send_buffer->data);
            //action
            vnode = mxmlFindElement(qnode, qnode, "action", NULL, NULL, MXML_DESCEND);
            if (vnode) {
                zpt->id = bufferid;
                zpt->zoom.numer = atoi(mxmlElementGetAttr (vnode, "z"));
                zpt->zoom.denom = 10;
                zpt->pantilt_angle.pan = atoi(mxmlElementGetAttr (vnode, "p"));
                zpt->pantilt_angle.tilt = atoi(mxmlElementGetAttr (vnode, "t"));
            } else{
                ret_code = -6;
                break;
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
    return ret_code;
}

int main()
{
    CGI *cgi = NULL;
    HDF *hdf = NULL;
    int bufferid = 0;
    char* stream = NULL;


    hdf_init(&hdf);
    cgi_init(&cgi, hdf);
    hdf_set_value(cgi->hdf,"Config.Upload.TmpDir","/tmp/cgiupload");
    hdf_set_value(cgi->hdf,"Config.Upload.Unlink","0");
    cgi_parse(cgi);


   stream = hdf_get_value(cgi->hdf,"Query.stream",NULL);
    if(strstr(stream, "1")) {
        bufferid = BUFFER_1;
    } else if (strstr(stream, "2")) {
        bufferid = BUFFER_2;
    } else if (strstr(stream, "3")) {
        bufferid = BUFFER_3;
    } else if (strstr(stream, "4")) {
        bufferid = BUFFER_4;
    }


     Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);

    if (AmbaLiveView_set_params (hdf, bufferid) == 0) {
        while (1){
            if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
                LOG_MESSG("%s", "mq_send failed!");
                sleep(1);
                continue;
            }
            break;
        }
    }
    PrintResult ();

    //hdf_dump(hdf,"<br>");


    return 0;
}
