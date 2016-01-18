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
#define ENCODE_TYPE_EN_BIT                  0
#define ENCODE_FPS_EN_BIT                   1
#define ENCODE_STREAM_SIZE_EN_BIT               2
#define ENCODE_H264_N_EN_BIT  3
#define ENCODE_IDR_INTERVAL_EN_BIT  4
#define ENCODE_PROFILE_EN_BIT  5
#define ENCODE_CBR_BITRATE_EN_BIT  6
#define ENCODE_MJPEG_QUALITY_EN_BIT  7


sem_t  cmd_sem;

static void  receive_cb(union sigval sv);

static int PrintResult (int result) {
    char buffer[8];
    sprintf(buffer, "%d", result);

    mxml_node_t *xml;
    mxml_node_t *data;
    mxml_node_t *node;

    xml = mxmlNewXML("1.0");
    data = mxmlNewElement(xml, "encode");
    node = mxmlNewElement(data, "res");
    mxmlElementSetAttr(node, "ul", buffer);
    printf("Content-type: application/xml\n\n");
    mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
    mxmlDelete(xml);

    return 0;

}

static int AmbaEncPage_set_params_fb () {
    MULTI_MESSAGES * multimsg = NULL;
    MESSAGE* msg = NULL;
    int resultcount = 0;
    int i = 0;

    do {
        if(receive_buffer->cmd_id == SET_STREAM_TYPE) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id == SET_STREAM_FRAMERATE) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id == SET_STREAM_SIZE) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id == SET_H264_N) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }
        if(receive_buffer->cmd_id == SET_IDR_INTERVAL) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }
        if(receive_buffer->cmd_id == SET_PROFILE) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id == SET_CBR_BITRATE) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id == SET_MJPEG_Q) {
            if(receive_buffer->status == STATUS_SUCCESS) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }

        if(receive_buffer->cmd_id ==    MULTI_SET) {
            multimsg = (MULTI_MESSAGES *)receive_buffer;
            msg = (MESSAGE*)multimsg->data;
            for(i = 0; i < multimsg->count; i++) {
                if(msg->status == STATUS_SUCCESS) {
                    resultcount++;
                }
                msg = (MESSAGE*)((char*)msg + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t));
            }
            //LOG_MESSG("MSG COUNT is %d", multimsg->count);
            //LOG_MESSG("RESULT   COUNT IS %d,", resultcount);
            if(resultcount == multimsg->count) {
                PrintResult(STATUS_SUCCESS);
            } else {
                PrintResult(STATUS_FAILURE);
            }
            break;
        }
    } while(0);

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
        AmbaEncPage_set_params_fb ();
        sem_post(&cmd_sem);
    }
}

int AmbaEncPage_parse_xml (HDF *hdf, ENCODE_PAGE* encode_ret, uint32_t* encode_bits_ret) {
    int ret_code = 0;
    char* file_name;
    mxml_node_t *tree = NULL;
    mxml_node_t *pnode = NULL;
    const char* encodetype = NULL;
    const char* framerate = NULL;
    const char* encodesize = NULL;
    const char* fliprotate = NULL;
	const char* h264_n = NULL;
	const char* idr_interval = NULL;
	const char* profile = NULL;
	const char* quality = NULL;
    const char* bitrate = NULL;
    FILE* LOG;
    ENCODE_PAGE encode;
    uint32_t encode_bits = 0;

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

        //encoders
        pnode = mxmlFindElement(tree, tree, "encoders", NULL, NULL, MXML_DESCEND);
        if (!pnode) {
            ret_code= -4;
            break;
        }
        memset(&encode, 0, sizeof(encode));

        //encodetype
        pnode = mxmlFindElement(tree, tree, "encodetype", NULL, NULL, MXML_DESCEND);
        if (pnode) {
            encodetype = mxmlElementGetAttr (pnode, "ul");
             if (encodetype) {
                encode.encodetype = atoi(encodetype);
                SET_BIT(encode_bits, ENCODE_TYPE_EN_BIT);
            }
        }

        //framerate
        pnode = mxmlFindElement(tree, tree, "fps", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             framerate = mxmlElementGetAttr (pnode, "ul");
             if (framerate) {
                encode.framerate = atoi(framerate);
                SET_BIT(encode_bits, ENCODE_FPS_EN_BIT);
            }
        }

        //encodesize
        pnode = mxmlFindElement(tree, tree, "resolution", NULL, NULL, MXML_DESCEND);
        if (pnode) {
            encodesize = mxmlElementGetAttr (pnode, "ul");
             if (encodesize) {
                encode.streamsize.rotate = AM_NO_ROTATE_FLIP;
                switch(atoi(encodesize)) {
                    case(RES_1920x1080):
                        encode.streamsize.width = 1920;
                        encode.streamsize.height = 1080;
                        break;
                    case(RES_1440x1080):
                        encode.streamsize.width = 1440;
                        encode.streamsize.height = 1080;
                        break;
                    case(RES_1280x1024):
                        encode.streamsize.width = 1280;
                        encode.streamsize.height = 1024;
                        break;
                    case(RES_1280x960):
                        encode.streamsize.width = 1280;
                        encode.streamsize.height = 960;
                        break;
                    case(RES_1280x720):
                        encode.streamsize.width = 1280;
                        encode.streamsize.height = 720;
                        break;
                    case(RES_800x600):
                        encode.streamsize.width = 800;
                        encode.streamsize.height = 600;
                        break;
                    case(RES_720x576):
                        encode.streamsize.width = 720;
                        encode.streamsize.height = 576;
                        break;
                    case(RES_720x480):
                        encode.streamsize.width = 720;
                        encode.streamsize.height = 480;
                        break;
                    case(RES_640x480):
                        encode.streamsize.width = 640;
                        encode.streamsize.height = 480;
                        break;
                    case(RES_352x288):
                        encode.streamsize.width = 352;
                        encode.streamsize.height = 288;
                        break;
                    case(RES_352x240):
                        encode.streamsize.width = 352;
                        encode.streamsize.height = 240;
                        break;
                    case(RES_320x240):
                        encode.streamsize.width = 320;
                        encode.streamsize.height = 240;
                        break;
                    default:
                        ret_code = -5;
                        break;
                }
                SET_BIT(encode_bits, ENCODE_STREAM_SIZE_EN_BIT);
            }
        }

        //fliprotate
        pnode = mxmlFindElement(tree, tree, "fliprotate", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             fliprotate = mxmlElementGetAttr (pnode, "ul");
             if (fliprotate) {
                encode.streamsize.rotate = atoi(fliprotate);
            }
        }

        //h264_n
        pnode = mxmlFindElement(tree, tree, "h264_n", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             h264_n = mxmlElementGetAttr (pnode, "ul");
             if (h264_n) {
                encode.h264_n = atoi(h264_n);
                SET_BIT(encode_bits, ENCODE_H264_N_EN_BIT);
            }
        }

        //idr_interval
        pnode = mxmlFindElement(tree, tree, "idr_interval", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             idr_interval = mxmlElementGetAttr (pnode, "ul");
             if (idr_interval) {
                encode.idr_interval = atoi(idr_interval);
                SET_BIT(encode_bits, ENCODE_IDR_INTERVAL_EN_BIT);
            }
        }

        //profile
        pnode = mxmlFindElement(tree, tree, "profile", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             profile = mxmlElementGetAttr (pnode, "ul");
             if (profile) {
                encode.profile = atoi(profile);
                SET_BIT(encode_bits, ENCODE_PROFILE_EN_BIT);
            }
        }

        //bitrate
        pnode = mxmlFindElement(tree, tree, "cbr_avg_bps", NULL, NULL, MXML_DESCEND);
        if (pnode) {
            bitrate = mxmlElementGetAttr (pnode, "ul");
             if (bitrate) {
                encode.bitrate = atoi(bitrate);
                SET_BIT(encode_bits, ENCODE_CBR_BITRATE_EN_BIT);
            }
        }

        //mjpeg_quality
        pnode = mxmlFindElement(tree, tree, "mjpeg_quality", NULL, NULL, MXML_DESCEND);
        if (pnode) {
             quality = mxmlElementGetAttr (pnode, "ul");
             if (quality) {
                encode.quality = atoi(quality);
                SET_BIT(encode_bits, ENCODE_MJPEG_QUALITY_EN_BIT);
            }
        }

        if ((!encodetype) && (!framerate) && (!fliprotate) && (!encodesize) \
            && (!h264_n)&& (!idr_interval)&& (!profile)&& (!bitrate) \
            && (!quality)) {
              ret_code= -6;
              break;
        } else {
              ret_code = 0;
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

    if (ret_code == 0) {
        memcpy(encode_ret, &encode, sizeof(encode));
        *encode_bits_ret = encode_bits;
    }
    //LOG_MESSG("ret_code is %d", ret_code);
    return ret_code;
}

int NumberOf1(int i) {
    int count = 0;
    while (i) {
        ++ count;
        i = (i - 1) & i;
    }
    return count;
}
int PackData(uint32_t enable_bits, ENCODE_PAGE* encode, uint32_t streamid) {
    MULTI_MESSAGES* multimsg = NULL;
    char *data = NULL;
    MESSAGE* msg = NULL;
    uint32_t* id = NULL;
    EncodeType* encodetype = NULL;
    uint32_t* framerate = NULL;
    EncodeSize* streamsize = NULL;
    uint16_t* h264_n = NULL;
    uint8_t *idr_interval = NULL;
    uint8_t *profile = NULL;
    uint32_t* bitrate = NULL;
    uint8_t *quality = NULL;
    uint32_t numberof1 = 0;

    numberof1 = NumberOf1(enable_bits);

    if (numberof1 == 1) {
        switch(enable_bits) {
            case(1):
                send_buffer->cmd_id = SET_STREAM_TYPE;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                encodetype = (EncodeType*)(send_buffer->data + sizeof(uint32_t));
                *encodetype = encode->encodetype;
                break;
            case(2):
                send_buffer->cmd_id = SET_STREAM_FRAMERATE;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                framerate = (uint32_t*)(send_buffer->data + sizeof(uint32_t));
                *framerate = encode->framerate;
                break;
            case(4):
                send_buffer->cmd_id = SET_STREAM_SIZE;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                streamsize = (EncodeSize*)(send_buffer->data + sizeof(uint32_t));
                *streamsize = encode->streamsize;
                break;
            case(8):
                send_buffer->cmd_id = SET_H264_N;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                h264_n = (uint16_t*)(send_buffer->data + sizeof(uint32_t));
                *h264_n = encode->h264_n;
                break;
            case(16):
                send_buffer->cmd_id = SET_IDR_INTERVAL;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                idr_interval = (uint8_t*)(send_buffer->data + sizeof(uint32_t));
                *idr_interval = encode->idr_interval;
                break;
            case(32):
                send_buffer->cmd_id = SET_PROFILE;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                profile = (uint8_t*)(send_buffer->data + sizeof(uint32_t));
                *profile = encode->profile;
                break;
            case(64):
                send_buffer->cmd_id = SET_CBR_BITRATE;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                bitrate = (uint32_t*)(send_buffer->data + sizeof(uint32_t));
                *bitrate = encode->bitrate;
                break;
            case(128):
                send_buffer->cmd_id = SET_MJPEG_Q;
                id = (uint32_t *)send_buffer->data;
                *id = streamid;
                quality = (uint8_t*)(send_buffer->data + sizeof(uint32_t));
                *quality = encode->quality;
                break;
            default:
                break;
         }
    } else {
        multimsg = (MULTI_MESSAGES*)send_buffer;
        multimsg->cmd_id = MULTI_SET;
        multimsg->count = numberof1;
        msg = (MESSAGE*)multimsg->data;
        data = (char*)multimsg->data;

        if(enable_bits & 1) {
            msg->cmd_id = SET_STREAM_TYPE;
            id = (uint32_t *)(msg->data);
            *id = streamid;
            encodetype = (EncodeType*)(msg->data + sizeof(uint32_t));
            *encodetype = encode->encodetype;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeType);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_STREAM_FRAMERATE;
            id = (uint32_t *)msg->data;
            *id = streamid;
            framerate = (uint32_t*)(msg->data + sizeof(uint32_t));
            *framerate = encode->framerate;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_STREAM_SIZE;
            id = (uint32_t *)msg->data;
            *id = streamid;
            streamsize = (EncodeSize*)(msg->data + sizeof(uint32_t));
            *streamsize = encode->streamsize;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeSize);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_H264_N;
            id = (uint32_t *)msg->data;
            *id = streamid;
            h264_n = (uint16_t*)(msg->data + sizeof(uint32_t));
            *h264_n = encode->h264_n;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_IDR_INTERVAL;
            id = (uint32_t *)msg->data;
            *id = streamid;
            idr_interval = (uint8_t*)(msg->data + sizeof(uint32_t));
            *idr_interval = encode->idr_interval;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_PROFILE;
            id = (uint32_t *)msg->data;
            *id = streamid;
            profile = (uint8_t*)(msg->data + sizeof(uint32_t));
            *profile = encode->profile;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_CBR_BITRATE;
            id = (uint32_t *)msg->data;
            *id = streamid;
            bitrate = (uint32_t*)(msg->data + sizeof(uint32_t));
            *bitrate = encode->bitrate;
            data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        enable_bits = enable_bits >> 1;
        if(enable_bits & 1) {
            msg = (MESSAGE*)data;
            msg->cmd_id = SET_MJPEG_Q;
            id = (uint32_t *)msg->data;
            *id = streamid;
            quality = (uint8_t*)(msg->data + sizeof(uint32_t));
            *quality = encode->quality;
        }
    }

    return 0;
}

int AmbaEncPage_set_params (HDF *hdf, uint32_t streamid) {
    ENCODE_PAGE encode;
    uint32_t encode_bits = 0;

    if (AmbaEncPage_parse_xml (hdf, &encode, &encode_bits) < 0) {
        PrintResult(STATUS_FAILURE);
        return -1;
    } else {
        PackData(encode_bits, &encode, streamid);
        while (1){
            if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
                LOG_MESSG("%s", "mq_send failed!");
                sleep(1);
                continue;
            }
            break;
        }
    }
    return 0;
}

int main()
{
    CGI *cgi = NULL;
    HDF *hdf = NULL;
    uint32_t streamid = 0;
    char* stream = NULL;

    hdf_init(&hdf);
    cgi_init(&cgi, hdf);
    hdf_set_value(cgi->hdf,"Config.Upload.TmpDir","/tmp/cgiupload");
    hdf_set_value(cgi->hdf,"Config.Upload.Unlink","0");
    cgi_parse(cgi);

    stream = hdf_get_value(cgi->hdf,"Query.stream",NULL);
    if(strstr(stream, "1")) {
        streamid = 0;
    } else if (strstr(stream, "2")) {
        streamid = 1;
    } else if (strstr(stream, "3")) {
        streamid = 2;
    } else if (strstr(stream, "4")) {
        streamid = 3;
    }

    Setup_MQ(ENC_MQ_SEND, ENC_MQ_RECEIVE);
    //bind Receive callback
    NotifySetup(&msg_queue_receive);

    if(AmbaEncPage_set_params (hdf, streamid) < 0) {
        return -1;
    }
    sem_wait(&cmd_sem);
    //hdf_dump(hdf,"<br>");

    return 0;
}

