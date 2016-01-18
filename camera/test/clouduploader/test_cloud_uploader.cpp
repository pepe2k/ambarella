/*
 * test_cloud_uploader.cpp
 *
 * @Author: He Zhi
 * @Email : zhe@ambarella.com
 * @Time  : 31/03/2014 [Created]
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_client.h"

#include <sys/time.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

int    g_running = 1;

#define d_max_server_url_length  256
static char cloud_server_url[d_max_server_url_length] = "10.0.0.2";
static char client_tag[256] = {0};
static unsigned short cloud_server_port = DDefaultSACPServerPort;
static unsigned short local_port = 0;

static int camera_index = 0;
static int stream_id = 1;

#define DSyncPointFlag 0x01
typedef struct msg_s {
    int	cmd;
    void	*ptr;
    unsigned char arg1, arg2, arg3, flags;
} msg_t;

#define MAX_MSGS 64

typedef struct msg_queue_s {
    msg_t   msgs[MAX_MSGS];
    int read_index;
    int write_index;
    volatile int num_msgs;
    int num_readers;
    int num_writers;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_read;
    pthread_cond_t  cond_write;
    sem_t   echo_event;
} msg_queue_t;

int msg_queue_init(msg_queue_t *q)
{
    q->read_index = 0;
    q->write_index = 0;
    q->num_msgs = 0;
    q->num_readers = 0;
    q->num_writers = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_read, NULL);
    pthread_cond_init(&q->cond_write, NULL);
    sem_init(&q->echo_event, 0, 0);
    return 0;
}

void msg_queue_get(msg_queue_t *q, msg_t *msg)
{
    pthread_mutex_lock(&q->mutex);
    while (1) {
        if (q->num_msgs > 0) {
            *msg = q->msgs[q->read_index];

            if (++q->read_index == MAX_MSGS)
                q->read_index = 0;

            q->num_msgs--;

            if (q->num_writers > 0) {
                q->num_writers--;
                pthread_cond_signal(&q->cond_write);
            }

            pthread_mutex_unlock(&q->mutex);
            return;
        }

        q->num_readers++;
        pthread_cond_wait(&q->cond_read, &q->mutex);
    }
}

int msg_queue_peek(msg_queue_t *q, msg_t *msg)
{
    int ret = 0;
    pthread_mutex_lock(&q->mutex);

    if (q->num_msgs > 0) {
        *msg = q->msgs[q->read_index];

        if (++q->read_index == MAX_MSGS)
            q->read_index = 0;

        q->num_msgs--;

        if (q->num_writers > 0) {
            q->num_writers--;
            pthread_cond_signal(&q->cond_write);
        }

        ret = 1;
    }

    pthread_mutex_unlock(&q->mutex);
    return ret;
}

void msg_queue_flush_all(msg_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    while (1) {
        if (q->num_msgs > 0) {

            if (++q->read_index == MAX_MSGS)
                q->read_index = 0;

            q->num_msgs--;
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&q->mutex);
}

int msg_queue_flush_to_syncpoint(msg_queue_t *q)
{
    int skip_number = 0;

    pthread_mutex_lock(&q->mutex);
    while (1) {
        if (q->num_msgs > 0) {

            if ((skip_number > (MAX_MSGS/2)) && (q->msgs[q->read_index].flags & DSyncPointFlag)) {
                pthread_mutex_unlock(&q->mutex);
                return 1;
            }

            if (++q->read_index == MAX_MSGS)
                q->read_index = 0;

            q->num_msgs--;
            skip_number++;
        } else {
            pthread_mutex_unlock(&q->mutex);
            return 0;
        }
    }

}

void msg_queue_put(msg_queue_t *q, msg_t *msg)
{
    pthread_mutex_lock(&q->mutex);
    while (1) {
        if (q->num_msgs < MAX_MSGS) {
            q->msgs[q->write_index] = *msg;

            if (++q->write_index == MAX_MSGS)
                q->write_index = 0;

            q->num_msgs++;

            if (q->num_readers > 0) {
                q->num_readers--;
                pthread_cond_signal(&q->cond_read);
            }

            pthread_mutex_unlock(&q->mutex);
            return;
        }

        q->num_writers++;
        pthread_cond_wait(&q->cond_write, &q->mutex);
    }
}

void msg_queue_ack(msg_queue_t *q)
{
    sem_post(&q->echo_event);
}

void msg_queue_wait(msg_queue_t *q)
{
    sem_wait(&q->echo_event);
}

TUint skipDelimterSize(TU8* p)
{
    if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (0x09 == p[4])) {
        return 6;
    }

    return 0;
}

static ICloudClientAgent* create_cloud_agent(unsigned short local_port, unsigned short server_port)
{
    ICloudClientAgent* p_client_agent = NULL;
    p_client_agent = gfCreateCloudClientAgent(EProtocolType_SACP, local_port, server_port);

    return p_client_agent;
}

static EECode connect_to_cloud_server(ICloudClientAgent* p_agent, char* server_url, unsigned short local_port, unsigned short server_port, char* device_tag)
{
    EECode err = EECode_OK;
    err = p_agent->ConnectToServer(server_url, local_port, server_port, (TU8*)device_tag, (TU16)strlen(device_tag));

    return err;
}

void receive_data_and_uploading (AmTransferClient* client, ICloudClientAgent* p_agent, char* server_url, unsigned short local_port, unsigned short server_port, int camera_index, int stream_id)
{
    AmTransferPacket packet;
    EECode err;
    TInt ret = 0;
    TU8 extra_flag = 0;
    TU8 start_uploading = 0;
    TU8 connected = 0;
    TU8* pdata;
    TU32 data_size;
    TU32 skip_size;

    while (g_running) {
        DEBUG ("streamId: %d, dataLen: %d, dataType: %d\n",
            packet.header.streamId,
            packet.header.dataLen,
            (int)(packet.header.dataType));

        //printf("before ReceivePacket\n");
        ret = client->ReceivePacket (&packet);
        if (DUnlikely(ret)) {
            printf("[error] ReceivePacket fail, ret %d\n", ret);
            return;
        }
        //printf("after ReceivePacket\n");

if (!connected) {
reconnect:

    if (!g_running) {
        break;
    }

    char device_tag[128] = {0};
    snprintf(device_tag, 128, "xman_%d", camera_index);
    err = connect_to_cloud_server(p_agent, server_url, local_port, server_port, device_tag);
    if (EECode_OK != err) {
        usleep(1000000);
        goto reconnect;
    }

    connected = 1;
}

        if (!start_uploading) {
            err = p_agent->StartUploading(NULL, ESACPDataChannelSubType_H264_NALU);
            if (DUnlikely(err != EECode_OK)) {
                LOG_ERROR("StartUploading fail, err %d, %s\n", err, gfGetErrorCodeString(err));
                return;
            }
            start_uploading = 1;
        }

        if (packet.header.dataType == AM_TRANSFER_PACKET_TYPE_H264) {
            if (packet.header.streamId == stream_id) {
                skip_size = skipDelimterSize(packet.data);
                pdata = packet.data + skip_size;
                data_size = packet.header.dataLen - skip_size;
                //printf("data_size %d, %02x %02x %02x %02x, %02x %02x %02x %02x\n", data_size, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);

                extra_flag = 0;
                if (packet.header.frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
                    extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;
                }

                if ((0x01 == pdata[2]) && (0x07 == (0x1f & pdata[3]))) {
                    extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
                }

                //printf("before Uploading\n");
                err = p_agent->Uploading(pdata, data_size, extra_flag);
                //printf("after Uploading\n");
                if (EECode_OK != err) {
                    usleep(3000000);
                    p_agent->DisconnectToServer();
                    goto reconnect;
                }
            } else {
                printf("skip id %d\n", packet.header.streamId);
            }
        } else {
              //printf("skip non h264 type %d\n", packet.header.dataType);
        }
    }
}

typedef struct
{
    ICloudClientAgent* p_agent;

    unsigned short local_port;
    unsigned short server_port;
    int camera_index;
    int stream_index;

    char server_url[256];
    char client_tag[256];

    msg_queue_t data_queue;
} s_uploading_context;

static void* uploading_thread(void* p)
{
    s_uploading_context* p_context = (s_uploading_context*) p;

    if (!p) {
        printf("[error]: NULL p here!\n");
        return NULL;
    }

    AmTransferPacket* p_packet;
    EECode err;
    TU8 extra_flag = 0;
    TU8 start_uploading = 0;
    TU8 connected = 0;
    TU8* pdata;
    TU32 data_size;
    TU32 skip_size;
    msg_t msg;

    printf("[flow]: uploading_thread begin.\n");

    while (g_running) {

        if (!connected) {
reconnect:

            if (!g_running) {
                break;
            }

            if (0x0 == p_context->client_tag[0]) {
                snprintf(p_context->client_tag, 256, "xman_%d", p_context->camera_index);
            }
            printf("[flow]: before connect_to_cloud_server, client tag %s\n", p_context->client_tag);
            err = connect_to_cloud_server(p_context->p_agent, p_context->server_url, p_context->local_port, p_context->server_port, p_context->client_tag);
            if (EECode_OK != err) {
                printf("[error]: connect_to_cloud_server fail\n");
                p_context->p_agent->DisconnectToServer();
                usleep(2000000);
                goto reconnect;
                //goto waitexit;
            }

            connected = 1;
        }

        if (!start_uploading) {
            err = p_context->p_agent->StartUploading(NULL, ESACPDataChannelSubType_H264_NALU);
            if (DUnlikely(err != EECode_OK)) {
                LOG_ERROR("StartUploading fail, err %d, %s\n", err, gfGetErrorCodeString(err));
                return NULL;
            }
            start_uploading = 1;

            err = p_context->p_agent->UpdateSourceFramerate(NULL, 30, 0);
            ASSERT_OK(err);
        }

        msg_queue_get(&p_context->data_queue, &msg);

        if (!msg.ptr) {
            printf("[flow]: receive NULL, break\n");
            break;
        }

        p_packet = (AmTransferPacket*)msg.ptr;

        skip_size = skipDelimterSize(p_packet->data);
        pdata = p_packet->data + skip_size;
        data_size = p_packet->header.dataLen - skip_size;
        //printf("data_size %d, %02x %02x %02x %02x, %02x %02x %02x %02x\n", data_size, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);

        extra_flag = 0;
        if (p_packet->header.frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
            extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;
        }

        if ((0x01 == pdata[2]) && (0x07 == (0x1f & pdata[3]))) {
            extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
        }
        err = p_context->p_agent->Uploading(pdata, data_size, extra_flag);
        if (EECode_OK != err) {
            p_context->p_agent->DisconnectToServer();
            usleep(2000000);
            goto reconnect;
            //goto waitexit;
        }

    }

#if 0
    while (g_running) {
waitexit:
        if (!g_running) {
            break;
        }
        usleep(5000000);
    }
#endif

    printf("[flow]: uploading_thread end.\n");
    return NULL;
}

static AmTransferPacket data_packets[MAX_MSGS];

void receive_data_and_uploading_mthread(AmTransferClient* client, ICloudClientAgent* p_agent, char* server_url, unsigned short local_port, unsigned short server_port, int camera_index, int stream_id)
{
    TInt ret = 0;
    TInt packet_index = 0;
    TInt wait_first_idr = 1;
    pthread_t thread_id;

    printf("[flow]: receive_data_and_uploading_mthread start\n");

    s_uploading_context context;
    memset(&context, 0x0, sizeof(s_uploading_context));

    context.p_agent = p_agent;
    context.local_port = local_port;
    context.server_port = server_port;
    context.camera_index = camera_index;
    context.stream_index = stream_id;
    strncpy(context.server_url, server_url, 255);
    context.server_url[255] = 0x0;
    msg_queue_init(&context.data_queue);

    pthread_create(&thread_id, NULL, uploading_thread, &context);

    while (g_running) {

        if (context.data_queue.num_msgs > (MAX_MSGS - 2)) {
            TInt to_sync = msg_queue_flush_to_syncpoint(&context.data_queue);
            printf("flush = %d\n", to_sync);
            if (to_sync) {
                wait_first_idr = 0;
            } else {
                wait_first_idr = 1;
            }
        }

        if (packet_index >= (MAX_MSGS)) {
            packet_index = 0;
        }

        ret = client->ReceivePacket (&data_packets[packet_index]);
        if (DUnlikely(ret)) {
            printf("[error] ReceivePacket fail, ret %d\n", ret);
            msg_t msg;
            msg.ptr = NULL;
            msg_queue_put(&context.data_queue, &msg);
            return;
        }
#if 0
            printf("streamId: %d, dataLen: %d, dataType: %d\n",
                data_packets[packet_index].header.streamId,
                data_packets[packet_index].header.dataLen,
                (int)(data_packets[packet_index].header.dataType));
#endif

        if (data_packets[packet_index].header.dataType == AM_TRANSFER_PACKET_TYPE_H264) {
            if (data_packets[packet_index].header.streamId == stream_id) {
                msg_t msg;
                if (wait_first_idr) {
                    if (data_packets[packet_index].header.frameType != AM_TRANSFER_FRAME_IDR_VIDEO) {
                        continue;
                    } else if (data_packets[packet_index].header.frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
                        wait_first_idr = 0;
                        msg.flags = DSyncPointFlag;
                    }
                } else if (data_packets[packet_index].header.frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
                    msg.flags = DSyncPointFlag;
                } else {
                    msg.flags = 0;
                }

                msg.ptr = &data_packets[packet_index];
                msg_queue_put(&context.data_queue, &msg);
                packet_index ++;
            } else {
                printf("skip id %d\n", data_packets[packet_index].header.streamId);
            }
        } else {
              //printf("skip non h264 type %d\n", data_packets.header.dataType);
        }
    }

    printf("quit, send NULL data_packets\n");
    msg_t msg;
    msg.ptr = NULL;
    msg_queue_put(&context.data_queue, &msg);

    pthread_join(thread_id, NULL);
    printf("quit, after thread join\n");
}

TInt test_cloud_uploader_init_params(TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    //parse options
    for (i=1; i<argc; i++) {
        if (!strcmp("--connect", argv[i])) {
            printf("'--connect': server_url\n");
            if ((i + 1) < argc) {
                if ((d_max_server_url_length - 1) <= strlen(argv[i])) {
                    printf("[input argument error]: server url too long(%s).\n", argv[i]);
                } else {
                    snprintf(cloud_server_url, d_max_server_url_length - 1, "%s", argv[i + 1]);
                    cloud_server_url[d_max_server_url_length - 1] = 0x0;
                    printf("[input argument]: server url (%s).\n", argv[i + 1]);
                }
            } else {
                printf("[input argument error]: '--connect', should follow with server url(%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--port", argv[i])) {
            printf("'--port': server port\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                cloud_server_port = ret;
                printf("server port %hu\n", cloud_server_port);
            } else {
                printf("[input argument error]: '--port', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--clienttag", argv[i])) {
            printf("'--clienttag': client tag\n");
            if ((i + 1) < argc) {
                snprintf(&client_tag[0], 256, "%s", argv[i + 1]);
                printf("client tag %s\n", client_tag);
            } else {
                printf("[input argument error]: '--clienttag', should follow with string(client tag), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--cameraindex", argv[i])) {
            printf("'--cameraindex': camera index\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &camera_index))) {
                printf("cameraindex %d\n", camera_index);
            } else {
                printf("[input argument error]: '--cameraindex', should follow with integar(camera index), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--streamindex", argv[i])) {
            printf("'--streamindex': stream index\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &stream_id))) {
                printf("stream_id %d\n", stream_id);
            } else {
                printf("[input argument error]: '--streamindex', should follow with integar(stream index), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        }
    }

    return 0;
}

static void print_usage()
{
    printf("usage:\n");
    printf("--connect [server]\n");
    printf("--port [port]\n");
    printf("--clienttag [clienttag]\n");
    printf("--cameraindex [cameraindex]\n");
    printf("--streamindex [streamindex]\n");
}

static void sigstop (int sig_num)
{
    NOTICE ("Quit");
    g_running = 0;
}

   int
main (int argc, char **argv)
{
    int ret = -1;
    AmTransferClient *client = NULL;

    if (argc < 2) {
        print_usage();
        return 0;
    }

    signal (SIGINT, sigstop);
    signal (SIGQUIT, sigstop);
    signal (SIGTERM, sigstop);

    if ((ret = test_cloud_uploader_init_params(argc, argv)) < 0) {
        printf("test_cloud_uploader_init_params fail, return %d.\n", ret);
        return (-3);
    }

    if ((client = AmTransferClient::Create ()) == NULL) {
        ERROR ("Failed to create an instance of AmTransferClient!");
    } else {

        ICloudClientAgent * p_agent = create_cloud_agent(0, 8270);
        //if (0) {
        //    receive_data_and_uploading(client, p_agent, cloud_server_url, local_port, cloud_server_port, camera_index, stream_id);
        //} else {
            receive_data_and_uploading_mthread(client, p_agent, cloud_server_url, local_port, cloud_server_port, camera_index, stream_id);
        //}
        client->Close ();
        delete client;
        client = NULL;
        p_agent->Delete();
        p_agent = NULL;
        ret = 0;
   }

   return ret;
}
