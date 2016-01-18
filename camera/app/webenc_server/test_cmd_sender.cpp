/*******************************************************************************
 * test_cmd_sender.cpp
 *
 * History:
 *  Dec 4, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_middleware.h"
#include "encode_cmd.h"

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>


#define VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
#define MQ_SEND "/MQ_ENCODE_RECEIVE"
#define MQ_RECEIVE "/MQ_ENCODE_SEND"
#define MAX_MESSAGES (64)
#define MAX_MESSAGES_SIZE (1024)

static mqd_t msg_queue_send, msg_queue_receive;
static MESSAGE *send_buffer = NULL;
static MESSAGE *receive_buffer = NULL;

static void  receive_cb(union sigval sv);

static void NotifySetup(mqd_t *mqdp)
{
  struct sigevent sev;
  sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
  sev.sigev_notify_function = receive_cb;
  sev.sigev_notify_attributes = NULL;
  sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
  if (mq_notify(*mqdp, &sev) == -1){
    perror("mq_notify");
  }else {
    DEBUG("NotifySetup Done!\n");
  }
}

static void  receive_cb(union sigval sv)
{
  NotifySetup((mqd_t *)sv.sival_ptr);
  while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0){
    INFO("=================RECEIVE MESSAGE==================");
    switch (receive_buffer->cmd_id){
      case GET_DPTZ:
      {
        DPTZParam *pdptz = (DPTZParam *)(receive_buffer->data + sizeof(uint32_t));
        INFO("ENC_COMMAND_ID:%d, BufferID:%d, ZoomFactor:%d, offset_x:%d, offset_y:%d", receive_buffer->cmd_id,\
          pdptz->source_buffer, pdptz->zoom_factor, pdptz->offset_x, pdptz->offset_y);
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
      }

      case GET_STREAM_TYPE:
      {
        uint32_t *pstreamid = (uint32_t *)receive_buffer->data;
        EncodeType *ptype = (EncodeType *)(receive_buffer->data + sizeof(uint32_t));
        INFO("ENC_COMMAND_ID:%d, Streamid:%d,Type:%d", receive_buffer->cmd_id, *pstreamid, *ptype);
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
      }

      case GET_CBR_BITRATE:
      {
        uint32_t *pstreamid = (uint32_t *)receive_buffer->data;
        uint32_t *pbitrate = (uint32_t *)(receive_buffer->data + sizeof(uint32_t));
        INFO("ENC_COMMAND_ID:%d, Streamid:%d,bitrate:%d", receive_buffer->cmd_id, *pstreamid, *pbitrate);
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
      }

      case GET_STREAM_FRAMERATE:
      {
        uint32_t *pstreamid = (uint32_t *)receive_buffer->data;
        uint32_t *pbitrate = (uint32_t *)(receive_buffer->data + sizeof(uint32_t));
        INFO("ENC_COMMAND_ID:%d, Streamid:%d,framerate:%d", receive_buffer->cmd_id, *pstreamid, *pbitrate);
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
      }

      case GET_STREAM_SIZE:
      {
        uint32_t *pstreamid = (uint32_t *)receive_buffer->data;
        EncodeSize *pstreamsize = (EncodeSize *)(receive_buffer->data + sizeof(uint32_t));
        INFO("ENC_COMMAND_ID:%d, Streamid:%d,width:%d,height:%d,rotate:%d", receive_buffer->cmd_id, *pstreamid,\
          pstreamsize->width, pstreamsize->height, pstreamsize->rotate);
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
      }

      case MULTI_GET:
      {
        char *ptmp = (char *)receive_buffer;
        MESSAGE *msg = (MESSAGE *)(ptmp + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t));
        uint32_t *streamid = (uint32_t *)(msg->data);
        uint32_t *bitrate = (uint32_t *)(msg->data + sizeof(uint32_t));
        INFO("MULTI_GET");
        INFO("ENC_COMMAND_ID:%d, RESULT:%d,streaid:%d,bitrate:%d", msg->cmd_id, msg->status, *streamid, *bitrate);
        INFO("ENC_COMMAND_ID:%d, RESULT:%d,streaid:%d,framerate:%d", *(bitrate+1),*(bitrate+2),*(bitrate+3),*(bitrate+4));
        break;
      }

      default:
        if (receive_buffer->status){
          INFO("ENC_COMMAND_ID:%d, RESULT:%d,INFO:%s", receive_buffer->cmd_id, receive_buffer->status, receive_buffer->data);
        }
        else{
          INFO("ENC_COMMAND_ID:%d, RESULT:%d,INFO:SUCCESS!", receive_buffer->cmd_id, receive_buffer->status);
        }
        memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
        break;
    }
  }
  if (errno != EAGAIN){
    ERROR("mq_receive");
  }
}

static void send_sleep(void)
{
  while (1){
    if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
      ERROR ("mq_send failed!");
      sleep(1);
      continue;
    }
    break;
  }
  memset(send_buffer, 0, MAX_MESSAGES_SIZE);
  sleep(2);
}

int main(int argc, char *argv[])
{

  //create Message Queue

  //mq_unlink(MQ_SEND);
  //mq_unlink(MQ_RECEIVE);
  msg_queue_send = mq_open(MQ_SEND, O_WRONLY);
  if (msg_queue_send < 0)  {
      perror("Error Opening MQ: ");
      return -1;
  }
  msg_queue_receive = mq_open(MQ_RECEIVE, O_RDONLY|O_NONBLOCK);
  if (msg_queue_receive == -1)  {
      perror("Error Opening MQ: ");
      return -1;
  }

  //create buffers
  send_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
  if (send_buffer == NULL){
    ERROR("malloc fail!");
    return -1;
  }
  receive_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
  if (receive_buffer == NULL){
    ERROR("malloc fail!");
    return -1;
  }

  while (mq_receive(msg_queue_receive, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) > 0){
    INFO("POLL QUEUE TO CLEAN!");
  }

  memset(send_buffer, 0, MAX_MESSAGES_SIZE);
  memset(receive_buffer, 0, MAX_MESSAGES_SIZE);

  //bind Receive callback
  NotifySetup(&msg_queue_receive);

  uint32_t *streamid = NULL;
  MULTI_MESSAGES *buffer = (MULTI_MESSAGES *)send_buffer;
  buffer->cmd_id = MULTI_GET;
  buffer->count = 2;
  MESSAGE *msg_bitrate = (MESSAGE *)buffer->data;
  MESSAGE *msg_framerate = (MESSAGE *)(buffer->data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t));
  msg_bitrate->cmd_id = GET_CBR_BITRATE;
  streamid = (uint32_t *)msg_bitrate->data;
  *streamid = 0;
  msg_framerate->cmd_id = GET_STREAM_FRAMERATE;
  streamid = (uint32_t *)msg_framerate->data;
  *streamid = 0;
  send_sleep();

  #if 0
  send_buffer->cmd_id = SET_PRVACY_MASK;
  PRIVACY_MASK *pm_in = (PRIVACY_MASK *)send_buffer->data;
  pm_in->action = PM_ADD_INC;
  pm_in->unit = 0;
  pm_in->left = 20;
  pm_in->top = 20;
  pm_in->width = 10;
  pm_in->height = 10;
  pm_in->color = PM_COLOR_RED;
  send_sleep();
  sleep(2);

  send_buffer->cmd_id = SET_PRVACY_MASK;
  pm_in->action = PM_ADD_INC;
  pm_in->unit = 0;
  pm_in->left = 40;
  pm_in->top = 40;
  pm_in->width = 20;
  pm_in->height = 20;
  pm_in->color = PM_COLOR_BLUE;
  send_sleep();


  send_buffer->cmd_id = SET_DPTZ;
  DPTZParam *dptz_in = (DPTZParam *)send_buffer->data;
  dptz_in->source_buffer =BUFFER_1;
  dptz_in->offset_x =0;
  dptz_in->offset_y =0;
  dptz_in->zoom_factor = 2;
  send_sleep();


  send_buffer->cmd_id = GET_DPTZ;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  send_sleep();


  send_buffer->cmd_id = GET_STREAM_TYPE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  send_sleep();

  send_buffer->cmd_id = SET_CBR_BITRATE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  uint32_t *bitrate = (uint32_t *)(send_buffer->data + sizeof(uint32_t));
  *bitrate = 2000000;
  send_sleep();

  send_buffer->cmd_id = GET_CBR_BITRATE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  send_sleep();

  send_buffer->cmd_id = SET_STREAM_FRAMERATE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  uint32_t *framerate = (uint32_t *)(send_buffer->data + sizeof(uint32_t));
  *framerate = 30;
  send_sleep();

  send_buffer->cmd_id = GET_STREAM_FRAMERATE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  send_sleep();

#if 1

  send_buffer->cmd_id = SET_STREAM_SIZE;
  uint32_t *num = (uint32_t *)send_buffer->data;
  *num = 1;
  EncodeSize *encodesize = (EncodeSize *)(send_buffer->data + sizeof(uint32_t));
  encodesize->width = 1280;
  encodesize->height = 720;
  encodesize->rotate = AM_NO_ROTATE_FLIP;
  send_sleep();

#endif

  send_buffer->cmd_id = GET_STREAM_SIZE;
  streamid = (uint32_t *)send_buffer->data;
  *streamid = 0;
  send_sleep();

  send_buffer->cmd_id = SET_DPTZ;
  dptz_in->source_buffer =BUFFER_1;
  dptz_in->offset_x =0;
  dptz_in->offset_y =0;
  dptz_in->zoom_factor = 1;
  send_sleep();

  send_buffer->cmd_id = SET_PRVACY_MASK;
  pm_in->action = PM_ADD_EXC;
  pm_in->unit = 0;
  pm_in->left = 40;
  pm_in->top = 40;
  pm_in->width = 10;
  pm_in->height = 10;
  pm_in->color = PM_COLOR_BLUE;
  send_sleep();

  send_buffer->cmd_id = SET_PRVACY_MASK;
  pm_in->action = PM_REPLACE;
  pm_in->unit = 0;
  pm_in->left = 50;
  pm_in->top = 40;
  pm_in->width = 10;
  pm_in->height = 10;
  pm_in->color = PM_COLOR_RED;
  send_sleep();

  send_buffer->cmd_id = SET_PRVACY_MASK;
  pm_in->action = PM_REMOVE_ALL;
  send_sleep();
  #endif

  while(1){
    sleep(1);//sleep && wait for response
  }
#if 0
  int i = 0;
  while(1){
    sprintf(send_buffer,"%d",i);
    if (0 != mq_send (msg_queue_send, send_buffer, strlen(send_buffer) + 1, 0)) {
      ERROR ("mq_send failed!");
      return -1;
    }
    i++;
    sleep(1);
  }
#endif

  return 0;
}
