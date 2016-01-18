#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <mqueue.h>
#include <semaphore.h>

#include "ClearSilver.h"
#include "mxml.h"
#include "utils.h"

#define BUFFERLENGTH 128

typedef enum {
	SUCCESS,							//The operation was a success
	UNSUPPORTED_HTTP_METHOD,		//The HTTP request method was not supported by the resource
	UNSUPPORTED_RETURN_TYPE,			//An unsupported/invalid return mime-type was specified as part of the http header Accept
	UNSUPPORTED_ATTRIBUTE,			//An attribute type that is not supported by this resource was encountered
	UNKNOWN_ATTRIBUTE,				//An unknown attribute type was encountered
	UNKNOWN_PROPERTY,				//An unknown property/input parameter was encountered
	MESSAGE_PARSE_ERROR, 				// Parsing of the HTTP data (XML,JSON) failed
	ENCODESERVER_FAILURE,					// An error was encountered while reading/writing to the database
	UNKNOWN_ERROR,					//An unknown error occurred
	NOT_IN_SUBREGION_MODE,
}ZOOMPANTILT_RET_TYPE;

static CGI *cgi = NULL;
static int streamId = 0;
sem_t  sem_get;
sem_t  sem_set;

static void  receive_cb_get(union sigval sv);
static void  receive_cb_set(union sigval sv);

int ZoomPanTilt_print_xml (int res) {
	char buffer[8];
	sprintf(buffer, "%d", res);
	mxml_node_t *xml;    /* <?xml ... ?> */
	mxml_node_t *data;   /* <data> */
	mxml_node_t *node;   /* <node> */


	xml = mxmlNewXML("1.0");

	data = mxmlNewElement(xml, "zoompantilt");
	node = mxmlNewElement(data, "res");
	mxmlElementSetAttr(node, "ul", buffer);

	printf("Content-type: application/xml\n\n");
	mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);

	mxmlDelete(xml);

	return 0;
}

static int AmbaZoomPanTilt_get_params_fb () {
	MULTI_MESSAGES *mbuffer = (MULTI_MESSAGES *)receive_buffer;
	MESSAGE* buffer = (MESSAGE*)mbuffer->data;
	uint32_t *modenum = (uint32_t *)buffer->data;
	TransformMode *transformmode = (TransformMode*)(buffer->data + sizeof(uint32_t));

	int selector;
	int i;
	bool canptz = false;
	TransformParameter* transform = NULL;
	for (i = 0 ;i < *modenum; i++){
		if(*(transformmode + i) == AM_TRANSFORM_MODE_SUBREGION && i == (streamId-1)) {
			selector = i;
			canptz = true;
			break;
		}
	}
	if(canptz == true) {
		buffer = (MESSAGE*)(buffer->data + sizeof(uint32_t) + (*modenum)*sizeof(uint32_t));
		if(buffer->cmd_id == GET_TRANSFORM_REGION && buffer->status == STATUS_SUCCESS) {
			transform = (TransformParameter *)(buffer->data + sizeof(uint32_t) + selector*sizeof(TransformParameter));

			char value[32];
			mxml_node_t *xml;    /* <?xml ... ?> */
			mxml_node_t *data;   /* <data> */
			mxml_node_t *node;   /* <node> */
			mxml_node_t *group;  /* <group> */


			xml = mxmlNewXML("1.0");

			data = mxmlNewElement(xml, "zoompantilt");
			node = mxmlNewElement(data, "res");
			mxmlElementSetAttr(node, "ul", "0");

			node = mxmlNewElement(data, "id");
			sprintf(value, "%d", transform->id);
			mxmlElementSetAttr(node, "ui", value);

			group = mxmlNewElement(data, "region");
			node = mxmlNewElement(group, "width");
			sprintf(value, "%d", transform->region.width);
			mxmlElementSetAttr(node, "ui", value);
			node = mxmlNewElement(group, "height");
			sprintf(value, "%d", transform->region.height);
			mxmlElementSetAttr(node, "ui", value);
			node = mxmlNewElement(group, "x");
			sprintf(value, "%d", transform->region.x);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(group, "y");
			sprintf(value, "%d", transform->region.y);
			mxmlElementSetAttr(node, "i", value);

			group = mxmlNewElement(data, "zoom");
			node = mxmlNewElement(group, "numer");
			sprintf(value, "%d", transform->zoom.numer);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(group, "denom");
			sprintf(value, "%d", transform->zoom.denom);
			mxmlElementSetAttr(node, "i", value);

			node = mxmlNewElement(data, "hor_angle_range");
			sprintf(value, "%d", transform->hor_angle_range);
			mxmlElementSetAttr(node, "ui", value);
			node = mxmlNewElement(data, "roi_top_angle");
			sprintf(value, "%d", transform->roi_top_angle);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(data, "orient");
			sprintf(value, "%d", transform->orient);
			mxmlElementSetAttr(node, "i", value);

			group = mxmlNewElement(data, "roi_center");
			node = mxmlNewElement(group, "x");
			sprintf(value, "%d", transform->roi_center.x);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(group, "y");
			sprintf(value, "%d", transform->roi_center.y);
			mxmlElementSetAttr(node, "i", value);

			group = mxmlNewElement(data, "pantilt_angle");
			node = mxmlNewElement(group, "pan");
			sprintf(value, "%d", transform->pantilt_angle.pan);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(group, "tilt");
			sprintf(value, "%d", transform->pantilt_angle.tilt);
			mxmlElementSetAttr(node, "i", value);

			group = mxmlNewElement(data, "source");
			node = mxmlNewElement(group, "width");
			sprintf(value, "%d", transform->source.width);
			mxmlElementSetAttr(node, "ui", value);
			node = mxmlNewElement(group, "height");
			sprintf(value, "%d", transform->source.height);
			mxmlElementSetAttr(node, "ui", value);
			node = mxmlNewElement(group, "x");
			sprintf(value, "%d", transform->source.x);
			mxmlElementSetAttr(node, "i", value);
			node = mxmlNewElement(group, "y");
			sprintf(value, "%d", transform->source.y);
			mxmlElementSetAttr(node, "i", value);

			printf("Content-type: application/xml\n\n");
			mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);

			mxmlDelete(xml);
		} else {
			ZoomPanTilt_print_xml(NOT_IN_SUBREGION_MODE);
		}
	} else {
		ZoomPanTilt_print_xml(NOT_IN_SUBREGION_MODE);
	}

	return 0;
}

static void NotifySetup_get(mqd_t *mqdp) {
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
	sev.sigev_notify_function = receive_cb_get;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
	if (mq_notify(*mqdp, &sev) == -1){
		LOG_MESSG("%s", "mq_notify");
	}
}

static void  receive_cb_get(union sigval sv) {
	NotifySetup_get((mqd_t *)sv.sival_ptr);
	while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
		MULTI_MESSAGES *buffer = (MULTI_MESSAGES *)receive_buffer;
		if (buffer->cmd_id == MULTI_GET && buffer->count == 2) {
			MESSAGE* mode = (MESSAGE*)buffer->data;
			if(mode->cmd_id == GET_TRANSFORM_MODE && mode->status == STATUS_SUCCESS) {
				AmbaZoomPanTilt_get_params_fb ();
			} else {
				ZoomPanTilt_print_xml(UNKNOWN_ERROR);
			}
		} else {
			ZoomPanTilt_print_xml(UNKNOWN_ERROR);
		}
            sem_post(&sem_get);
	}
}

static void NotifySetup_set(mqd_t *mqdp) {
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
	sev.sigev_notify_function = receive_cb_set;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
	if (mq_notify(*mqdp, &sev) == -1){
		LOG_MESSG("%s", "mq_notify");
	}
}

static void  receive_cb_set(union sigval sv) {
	NotifySetup_set((mqd_t *)sv.sival_ptr);
	while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0) {
		if (receive_buffer->cmd_id == SET_TRANSFORM_REGION && receive_buffer->status== STATUS_SUCCESS){
			ZoomPanTilt_print_xml(SUCCESS);
		} else {
			ZoomPanTilt_print_xml(UNKNOWN_ERROR);
		}
            sem_post(&sem_set);
	}
}

int ZoomPanTilt_parse_xml(HDF *hdf, TransformParameter* transform_ret) {
      int ret_code = 0;
      char* file_name;
      mxml_node_t *tree =  NULL;
      mxml_node_t *root =  NULL;
      mxml_node_t *pnode =  NULL;
      mxml_node_t *vnode =  NULL;
      TransformParameter transform;
      FILE* LOG;

      file_name = hdf_get_value(hdf, "PUT.FileName", "");

      if (!hdf || !transform_ret) {
            return -1;
      }
      do {

            //XML File
            LOG = fopen(file_name, "r");
            if (!LOG) {
                ZoomPanTilt_print_xml(UNKNOWN_ERROR);
                ret_code = -2;
                break;
            }

            //XML tree
            tree = mxmlLoadFile (NULL, LOG, MXML_TEXT_CALLBACK);
            if (!tree) {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -3;
                break;
            }

            //zoompantilt
            root = mxmlFindElement(tree, tree, "zoompantilt", NULL, NULL, MXML_DESCEND);
            if (!root) {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -4;
                break;
            }
            memset(&transform, 0, sizeof(transform));

            //id
            vnode = mxmlFindElement(root, root, "id", NULL, NULL, MXML_DESCEND);
            if (vnode) {
               transform.id = atoi(mxmlElementGetAttr (vnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -4;
                break;
            }

             //width
            vnode = mxmlFindElement(root, root, "region", NULL, NULL, MXML_DESCEND);
	      pnode = mxmlFindElement(vnode, vnode, "width", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.region.width = atoi(mxmlElementGetAttr (pnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -5;
                break;
            }

            //height
            pnode = mxmlFindElement(vnode, vnode, "height", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.region.height = atoi(mxmlElementGetAttr (pnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -6;
                break;
            }

             //x
            pnode = mxmlFindElement(vnode, vnode, "x", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.region.x = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -7;
                break;
            }

            //y
            pnode = mxmlFindElement(vnode, vnode, "y", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.region.y = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -8;
                break;
            }

            //zoom numer
            vnode = mxmlFindElement(root, root, "zoom", NULL, NULL, MXML_DESCEND);
		pnode = mxmlFindElement(vnode, vnode, "numer", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.zoom.numer = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -9;
                break;
            }

            //zoom demon
            pnode = mxmlFindElement(vnode, vnode, "denom", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.zoom.denom = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -10;
                break;
            }

            //hor_angle_range
            vnode = mxmlFindElement(root, root, "hor_angle_range", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.hor_angle_range = atoi(mxmlElementGetAttr (vnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -11;
                break;
            }

            //roi_top_angle
            vnode = mxmlFindElement(root, root, "roi_top_angle", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.roi_top_angle = atoi(mxmlElementGetAttr (vnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -12;
                break;
            }

            //orient
            vnode = mxmlFindElement(root, root, "orient", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.orient = atoi(mxmlElementGetAttr (vnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -13;
                break;
            }

             //roi_center x
            vnode = mxmlFindElement(root, root, "roi_center", NULL, NULL, MXML_DESCEND);
		pnode = mxmlFindElement(vnode, vnode, "x", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.roi_center.x = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -14;
                break;
            }

            //roi_center y
            pnode = mxmlFindElement(vnode, vnode, "y", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.roi_center.y = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -15;
                break;
            }

            //pantilt_angle pan
            vnode = mxmlFindElement(root, root, "pantilt_angle", NULL, NULL, MXML_DESCEND);
		pnode = mxmlFindElement(vnode, vnode, "pan", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.pantilt_angle.pan = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -16;
                break;
            }

             //pantilt_angle tilt
            pnode = mxmlFindElement(vnode, vnode, "tilt", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.pantilt_angle.tilt = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -17;
                break;
            }

             //source width
            vnode = mxmlFindElement(root, root, "source", NULL, NULL, MXML_DESCEND);
		pnode = mxmlFindElement(vnode, vnode, "width", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.source.width= atoi(mxmlElementGetAttr (pnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -18;
                break;
            }

            //source height
            pnode = mxmlFindElement(vnode, vnode, "height", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.source.height = atoi(mxmlElementGetAttr (pnode, "ui"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -19;
                break;
            }

            //source x
            pnode = mxmlFindElement(vnode, vnode, "x", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.source.x = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -20;
                break;
            }

            //source y
            pnode = mxmlFindElement(vnode, vnode, "y", NULL, NULL, MXML_DESCEND);
            if (pnode) {
               transform.source.y = atoi(mxmlElementGetAttr (pnode, "i"));
            } else {
                ZoomPanTilt_print_xml(MESSAGE_PARSE_ERROR);
                ret_code= -21;
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

        //if correct, copy it back
        if (ret_code == 0) {
           memcpy(transform_ret, &transform, sizeof(transform));
        }

	return ret_code;
}

int ZoomPanTilt_set_request(HDF *hdf) {
    Setup_MQ();
    //bind Receive callback
    NotifySetup_set(&msg_queue_receive);

    send_buffer->cmd_id = SET_TRANSFORM_REGION;
    uint32_t* tmp;
    tmp = (uint32_t*)send_buffer->data;
    *tmp = 1;
    tmp = (uint32_t*)(send_buffer->data + sizeof(uint32_t));
    *tmp = TRANSFORM_REGION_APPEND;
    TransformParameter* transform = (TransformParameter *)(send_buffer->data+ 2*sizeof(uint32_t));

    if (ZoomPanTilt_parse_xml(hdf, transform) < 0) {
        return -1;
    }
    while (1){
        if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
            LOG_MESSG("%s", "mq_send failed!");
            sleep(1);
            continue;
        }
        break;
    }
    sem_wait(&sem_set);
    return 0;
}

int ZoomPanTilt_get_request() {
    Setup_MQ();
    //bind Receive callback
    NotifySetup_get(&msg_queue_receive);

    MULTI_MESSAGES *buffer = (MULTI_MESSAGES *)send_buffer;
    buffer->cmd_id = MULTI_GET;
    buffer->count = 2;
    MESSAGE* msg = (MESSAGE*)buffer->data;
    msg->cmd_id = GET_TRANSFORM_MODE;
    msg = (MESSAGE*)(buffer->data + sizeof(MESSAGE));
    msg->cmd_id = GET_TRANSFORM_REGION;

    while (1){
        if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
            LOG_MESSG("%s", "mq_send failed!");
            sleep(1);
            continue;
        }
        break;
    }

    sem_wait(&sem_get);
    return 0;
}

int AmbaZoomPanTilt_query (HDF *hdf) {
	char* req_method;
	char *stream = NULL;

	req_method = hdf_get_value(hdf, "CGI.RequestMethod", "");
      stream = hdf_get_value(cgi->hdf,"Query.stream",NULL);
	if(strstr(stream, "1")) {
		streamId = 1;
	} else if (strstr(stream, "2")) {
		streamId = 2;
	} else if (strstr(stream, "3")) {
		streamId = 3;
	} else if (strstr(stream, "4")) {
		streamId = 4;
	}

	if (!strncmp(req_method, "PUT", 4)) {
		ZoomPanTilt_set_request(hdf);
	} else if (strncmp(req_method, "GET", 4) != 0) {
		ZoomPanTilt_print_xml(UNSUPPORTED_HTTP_METHOD);
	} else {
            ZoomPanTilt_get_request();
	}
	return 0;
}

int main()
{
	HDF *hdf = NULL;

	hdf_init(&hdf);
	cgi_init(&cgi, hdf);
      sem_init(&sem_get, 0, 0);
      sem_init(&sem_set, 0, 0);
	hdf_set_value(cgi->hdf,"Config.Upload.TmpDir","/tmp");
	hdf_set_value(cgi->hdf,"Config.Upload.Unlink","0");
	cgi_parse(cgi);

	if (AmbaZoomPanTilt_query (hdf) < 0) {
		return -1;
	}
	//hdf_dump(hdf,"<br>");

	return 0;
}



