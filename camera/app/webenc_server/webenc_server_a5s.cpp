/*******************************************************************************
 * webenc_server.cpp
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
#include <unistd.h>

#define VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
#define MQ_SEND "/MQ_ENCODE_SEND"
#define MQ_RECEIVE "/MQ_ENCODE_RECEIVE"
#define MAX_MESSAGES (64)
#define MAX_MESSAGES_SIZE (1024)

static mqd_t msg_queue_send, msg_queue_receive;
static MESSAGE *send_buffer = NULL;
static MESSAGE *receive_buffer = NULL;
static AmConfig *config = NULL;
static VDeviceParameters *vdevConfig = NULL;
//static AmEncodeDevice *encode_device = NULL;
static AmWebCam *webcam = NULL;
static bool error = false;

char text[][128] = {
  "Ambarella Flexible Linux SDK",
  "True Type Support",
  "安霸中国(Chinese)",
  "여보세요(Korean)",
};

char ttfFile[][128] = {
  "/usr/share/fonts/Vera.ttf",
  "/usr/share/fonts/Lucida.ttf",
  "/usr/share/fonts/gbsn00lp.ttf",
  "/usr/share/fonts/UnPen.ttf",
};

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

static void Set_prvacy_mask(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE PRIVACY MASK SETTING REQUEST!==========");
  if (! webcam->set_privacy_mask((PRIVACY_MASK *)msg_in->data)){
    //INFO("ADD PM successfully!");
    msg_out->cmd_id = SET_PRVACY_MASK;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_prvacy_mask successful!");
  }
  else {
    //INFO("fail to ADD PM");
    msg_out->cmd_id = SET_PRVACY_MASK;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_prvacy_mask Fail!");
  }
}

static void Set_dptz(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE DPTZ SETTING REQUEST!==========");
  if (! webcam->set_digital_ptz((DPTZParam *)msg_in->data)){
    msg_out->cmd_id = SET_DPTZ;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_dptz successful!");
  }
  else {
    msg_out->cmd_id = SET_DPTZ;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_dptz Fail!");
  }
}

static void Get_dptz(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE DPTZ GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  DPTZParam dptz_out, *pdptz_out;
  if (! webcam->get_digital_ptz(*streamid, &dptz_out)){
    msg_out->cmd_id = GET_DPTZ;
    msg_out->status = STATUS_SUCCESS;
    uint32_t *pstreamid = (uint32_t *)msg_out->data;
    *pstreamid = *streamid;
    pdptz_out = (DPTZParam *)(msg_out->data + sizeof(uint32_t));
    *pdptz_out = dptz_out;
    INFO("Get_dptz successful!");
  }
  else {
    msg_out->cmd_id = GET_DPTZ;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_dptz Fail!");
  }
}

static void Set_stream_type(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM TYPE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  const EncodeType *type = (const EncodeType *)(msg_in->data + sizeof(uint32_t));
  if (webcam->set_stream_type(*streamid, *type)){
    msg_out->cmd_id = SET_STREAM_TYPE;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_stream_type successful!");
  }
  else {
    msg_out->cmd_id = SET_STREAM_TYPE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_type Fail!");
  }
}

static void Get_stream_type(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM TYPE GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  EncodeType type, *ptype;
  if (webcam->get_stream_type(*streamid, &type)){
    msg_out->cmd_id = GET_STREAM_TYPE;
    msg_out->status = STATUS_SUCCESS;
    uint32_t *pstreamid = (uint32_t *)msg_out->data;
    *pstreamid = *streamid;
    ptype = (EncodeType *)(msg_out->data + sizeof(uint32_t));
    *ptype = type;
    INFO("Get_stream_type successful!");
    //ERROR("streamid:%d,type:%d",*pstreamid,*ptype);
  }
  else {
    msg_out->cmd_id = GET_STREAM_TYPE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_type Fail!");
  }
}

static void Set_cbr_bitrate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE CBR BITRATE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t *bitrate = (uint32_t *)(msg_in->data + sizeof(uint32_t));
  if (webcam->set_cbr_bitrate(*streamid, *bitrate)){
    msg_out->cmd_id = SET_CBR_BITRATE;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_cbr_bitrate successful!");
  }
  else {
    msg_out->cmd_id = SET_CBR_BITRATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_cbr_bitrate Fail!");
  }
}

static void Get_cbr_bitrate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE CBR BITRATE GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t bitrate, *pbitrate;
  if (webcam->get_cbr_bitrate(*streamid, &bitrate)){
    msg_out->cmd_id = GET_CBR_BITRATE;
    msg_out->status = STATUS_SUCCESS;
    uint32_t *pstreamid = (uint32_t *)msg_out->data;
    *pstreamid = *streamid;
    pbitrate = (uint32_t *)(msg_out->data + sizeof(uint32_t));
    *pbitrate = bitrate;
    //INFO("bitrate:%p",pbitrate);
    INFO("Get_cbr_bitrate successful!");
  }
  else {
    msg_out->cmd_id = GET_CBR_BITRATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_cbr_bitrate Fail!");
  }
}


static void Set_stream_framerate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM FRAMERATE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t *framerate = (uint32_t *)(msg_in->data + sizeof(uint32_t));
  //ERROR("streamid:%d,framerate:%d",*streamid, *framerate);
  if (webcam->set_stream_framerate(*streamid, *framerate)){
    msg_out->cmd_id = SET_STREAM_FRAMERATE;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_stream_framerate successful!");
  }
  else {
    msg_out->cmd_id = SET_STREAM_FRAMERATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_framerate Fail!");
  }
}

static void Get_stream_framerate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM FRAMERATE GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t framerate, *pframerate;
  if (webcam->get_stream_framerate(*streamid, &framerate)){
    msg_out->cmd_id = GET_STREAM_FRAMERATE;
    msg_out->status = STATUS_SUCCESS;
    uint32_t *pstreamid = (uint32_t *)msg_out->data;
    *pstreamid= *streamid;
    pframerate = (uint32_t *)(msg_out->data + sizeof(uint32_t));
    *pframerate = framerate;
    INFO("Get_stream_framerate successful!");
  }
  else {
    msg_out->cmd_id = GET_STREAM_FRAMERATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_framerate Fail!");
  }
}

static void Set_stream_size(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM SIZE SETTING REQUEST!==========");
  uint32_t *num = (uint32_t *)msg_in->data;
  EncodeSize *encodesize = (EncodeSize *)(msg_in->data + sizeof(uint32_t));
  //ERROR("Num:%d,width:%d,height:%d,rotate:%d", *num, encodesize->width,encodesize->height,encodesize->rotate);
  if (webcam->set_stream_size_all(*num, encodesize)){
    msg_out->cmd_id = SET_STREAM_SIZE;
    msg_out->status = STATUS_SUCCESS;
    webcam->start_encode();
    INFO("Set_stream_framerate successful!");
  }
  else {
    msg_out->cmd_id = SET_STREAM_SIZE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_framerate Fail!");
  }
}

static void Get_stream_size(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM SIZE GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  EncodeSize encodesize, *pencodesize;
  if (webcam->get_stream_size(*streamid, &encodesize)){
    msg_out->cmd_id = GET_STREAM_SIZE;
    msg_out->status = STATUS_SUCCESS;
    uint32_t *pstreamid = (uint32_t *)msg_out->data;
    *pstreamid= *streamid;
    pencodesize = (EncodeSize *)(msg_out->data + sizeof(uint32_t));
    *pencodesize = encodesize;
    INFO("Get_stream_size successful!");
  }
  else {
    msg_out->cmd_id = GET_STREAM_SIZE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_size Fail!");
  }
}

static void Multi_Set(MULTI_MESSAGES *msg_in, MULTI_MESSAGES *msg_out)
{
  INFO("==========RECEIVE MULTI SETTING REQUEST!==========");
  uint32_t msg_count = msg_in->count;
  uint32_t i = 0, flag_unknown_cmd = 0;
  char *data = msg_in->data;
  char *data_output = msg_out->data;
  msg_out->cmd_id = MULTI_SET;
  msg_out->count = 0;
  for (i = 0;i < msg_count;i++){
    uint32_t cmd_id = *(uint32_t *)data;
    switch(cmd_id){
      case SET_PRVACY_MASK:
        Set_prvacy_mask((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(PRIVACY_MASK);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_DPTZ:
        Set_dptz((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(DPTZParam);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_STREAM_TYPE:
        Set_stream_type((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeType);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_CBR_BITRATE:
        Set_cbr_bitrate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_STREAM_FRAMERATE:
        Set_stream_framerate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

       case SET_STREAM_SIZE:
        {
        Set_stream_size((MESSAGE *)data, (MESSAGE *)data_output);
        uint32_t *pnum = (uint32_t *)(data + sizeof(COMMAND_ID) + sizeof(uint32_t));
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeSize) * (*pnum);
        data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;
        }

      default:
        flag_unknown_cmd = 1;
        break;
    }

    if (flag_unknown_cmd){
      memset(msg_out, 0, MAX_MESSAGES_SIZE);
      msg_out->cmd_id = MULTI_SET;
      msg_out->count = 0;
      INFO("Containing UNSUPPORTED_ID!");
      break;
    }

  }

}

static inline uint32_t get_status(char *data_out)
{
  uint32_t *status = (uint32_t *)(data_out + sizeof(COMMAND_ID));
  return *status;
}
static void Multi_Get(MULTI_MESSAGES *msg_in, MULTI_MESSAGES *msg_out)
{
  INFO("==========RECEIVE MULTI GET REQUEST!==========");
  uint32_t msg_count = msg_in->count;
  uint32_t i = 0, flag_unknown_cmd = 0;
  char *data = msg_in->data;
  char *data_output = msg_out->data;
  msg_out->cmd_id = MULTI_GET;
  msg_out->count = 0;
  for (i = 0;i < msg_count;i++){
    uint32_t cmd_id = *(uint32_t *)data;
    switch(cmd_id){
      case GET_DPTZ:
        Get_dptz((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(DPTZParam);
        }
        msg_out->count++;
        break;

      case GET_STREAM_TYPE:
        Get_stream_type((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeType);
        }
        msg_out->count++;
        break;

      case GET_CBR_BITRATE:
        Get_cbr_bitrate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        msg_out->count++;
        break;

      case GET_STREAM_FRAMERATE:
        Get_stream_framerate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) +sizeof(uint32_t);
        }
        msg_out->count++;
        break;

      case GET_STREAM_SIZE:
        Get_stream_size((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeSize);
        }
        msg_out->count++;
        break;

      default:
        flag_unknown_cmd = 1;
        break;
    }

    if (flag_unknown_cmd){
      memset(msg_out, 0, MAX_MESSAGES_SIZE);
      msg_out->cmd_id = MULTI_GET;
      msg_out->count = 0;
      INFO("Containing UNSUPPORTED_ID!");
      break;
    }

  }

}


static void  receive_cb(union sigval sv)
{
  while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0){
    //INFO("RECEIVE MESSAGE:%s", receive_buffer);
    switch (receive_buffer->cmd_id){
      case SET_PRVACY_MASK:
        Set_prvacy_mask((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case SET_DPTZ:
        Set_dptz((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case GET_DPTZ:
        Get_dptz((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case SET_STREAM_TYPE:
        Set_stream_type((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case GET_STREAM_TYPE:
        Get_stream_type((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case SET_CBR_BITRATE:
        Set_cbr_bitrate((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case GET_CBR_BITRATE:
        Get_cbr_bitrate((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case SET_STREAM_FRAMERATE:
        Set_stream_framerate((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case GET_STREAM_FRAMERATE:
        Get_stream_framerate((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case SET_STREAM_SIZE:
        Set_stream_size((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case GET_STREAM_SIZE:
        Get_stream_size((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
        break;

      case MULTI_SET:
        Multi_Set((MULTI_MESSAGES *)receive_buffer, (MULTI_MESSAGES *)send_buffer);
        break;

      case MULTI_GET:
        Multi_Get((MULTI_MESSAGES *)receive_buffer, (MULTI_MESSAGES *)send_buffer);
        break;

      default:
        send_buffer->cmd_id = UNSUPPORTED_ID;
        sprintf(send_buffer->data, "UNSUPPORTED_ID!");
        ERROR("NOT SUPPORTED COMMAND!");
        break;
    }

    while(1){
      if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
        ERROR ("mq_send failed!");
        continue;
      }
      break;
    }

    memset(send_buffer, 0, MAX_MESSAGES_SIZE);
    memset(receive_buffer, 0, MAX_MESSAGES_SIZE);
  }

  if (errno != EAGAIN){
    ERROR("mq_receive");
  }

  NotifySetup((mqd_t *)sv.sival_ptr);
}

static void usage()
{
  ERROR("Usage: test_encode_server "
            "[cli]"
            "| [daemon]");
}

static void sighandler(int sig_no)
{
  /* Save stream configuration */
  if (!error) {
    for (uint32_t i = 0; i < vdevConfig->stream_number; ++i) {
      config->set_stream_config(config->stream_config(i), i);
    }
  }

  if (webcam->stop_encode()) {
    INFO("Stop all streams successfully!");
  } else {
    ERROR("Failed to stop all streams!");
    error = true;
  }

  delete config;
  delete webcam;

	exit(1);
}

int main(int argc, char *argv[])
{
  if ((argc != 2)
      || (!is_str_equal("cli", argv[1])
          && !is_str_equal("daemon", argv[1]))) {
    usage();
    exit(0);
  }

  if (is_str_equal("daemon", argv[1])) {
    daemon(1, 0);
  }

  config = new AmConfig();
  //If you have different config, please set it here
  config->set_vin_config_path(VIN_CONFIG);
  //config->set_vout_config_path    (vout_config_path);
  //config->set_vdevice_config_path (vdevice_config_path);
  //config->set_record_config_path  (record_config_path);

  if (!config || !config->load_vdev_config()) {
    ERROR("Faild to get VideoDevice's configurations!");
    return -1;
  }

  vdevConfig = config->vdevice_config();
  if (! vdevConfig) {
    ERROR("Faild to get VideoDevice's configurations!");
    return -1;
  }
  //encode_device = new AmEncodeDevice(vdevConfig);

  webcam = new AmWebCam(config);
  if (!webcam->init()) {
    ERROR("Failed to init webcam.");
    return -1;
  }

  //create Message Queue

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MESSAGES_SIZE;
  attr.mq_curmsgs = 0;
  mq_unlink(MQ_SEND);
  mq_unlink(MQ_RECEIVE);
  msg_queue_send = mq_open(MQ_SEND, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG, &attr);
  if (msg_queue_send < 0)  {
      perror("Error Opening MQ: ");
      return -1;
  }
  msg_queue_receive = mq_open(MQ_RECEIVE, O_RDONLY | O_CREAT |O_NONBLOCK, S_IRWXU | S_IRWXG, &attr);
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

  //start encode
  if (webcam->start_encode()) {
    INFO("Start all streams successfully!");
  } else {
    ERROR("Failed to start all streams!");
    error = true;
  }

  signal(SIGINT, sighandler);
  signal(SIGQUIT, sighandler);
  signal(SIGTERM, sighandler);

//SET this flag to test OSD
#if 0
  TextBox box;
  Point offset[MAX_OVERLAY_AREA_NUM] = { Point(20, 60), Point(1000, 20),Point(900, 600)};
  memset(&box, 0, sizeof(TextBox));

  box.width = 700;
  box.height = 80;
  webcam->add_overlay_time(0, 0, offset[0], &box);
  webcam->add_overlay_bitmap(0, 1, offset[1],
      "/usr/local/bin/Ambarella-256x128-8bit.bmp");
  box.width = 380;
  box.height = 120;
  webcam->add_overlay_text(0, 2, offset[2], &box, text[0]);

  sleep(3);

  webcam->add_overlay_text(0, 2, offset[2], &box, text[1]);

#endif

  while (1){
    sleep(1);
  }

  return 0;
}
