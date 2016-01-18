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

#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "mw_struct.h"
#include "mw_api.h"

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ENC_MQ_SEND "/MQ_ENCODE_SEND"
#define ENC_MQ_RECEIVE "/MQ_ENCODE_RECEIVE"
#define MAX_MESSAGES (64)
#define MAX_MESSAGES_SIZE (1024)
#define	DIV_ROUND(divident, divider)    (((divident)+((divider)>>1)) / (divider))

const char* mode_list[MODE_TOTAL_NUM] = {"fullfr", "dewarp", "highmega"};

static mqd_t msg_queue_send, msg_queue_receive;
static MESSAGE *send_buffer = NULL;
static MESSAGE *receive_buffer = NULL;
static AmConfig *config = NULL;
static AmWebCam *webcam = NULL;
static AmFisheyeCam *fishcam = NULL;
static AmCam *camtype = NULL;
static FisheyeParameters *fisheye_config = NULL;
static bool error = false;
static int32_t camera_mode = -1;
static TransformMode transform_mode_saved[MAX_FISHEYE_REGION_NUM];
static uint32_t transform_mode_num = 0;
static TransformParameters transform_parameter_saved[MAX_FISHEYE_REGION_NUM];
static uint32_t transform_parameter_num = 0;
static FisheyeParameters fisheyeparameters_saved;
static FISHEYE_LAYOUT layout_saved;
OSDITEM osditem[MAX_ENCODE_STREAM_NUM];



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

static void  receive_cb_cam(union sigval sv);


static void NotifySetup(mqd_t *mqdp)
{
  struct sigevent sev;
  sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
  sev.sigev_notify_function = receive_cb_cam;
  sev.sigev_notify_attributes = NULL;
  sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
  if (mq_notify(*mqdp, &sev) == -1){
    perror("mq_notify");
  }else {
    DEBUG("NotifySetup Done!\n");
  }
}

static bool resume_osd()
{
  bool ret = false;
  uint32_t i;

  TextBox box;
  memset(&box, 0, sizeof(TextBox));
  Point time_offset;
  Point bmp_offset;
  Point text_offset;
  Font font;
  OverlayClut coloryuv;
  OverlayClut outlineyuv;
  EncodeSize encodesize;

  for(i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
    camtype->get_stream_size(i, &encodesize);
    box.width = encodesize.width * 55 /100;
    box.height = encodesize.height * 12 /100;
    time_offset.x = encodesize.width * 15 /1000;
    time_offset.y = encodesize.height * 83 /1000;
    bmp_offset.x = encodesize.width * 78 /100;
    bmp_offset.y = encodesize.height * 28 /1000;
    if(osditem[i].osd_time_enabled) {
      camtype->add_overlay_time(i, 0, time_offset, &box);
    }
    if(osditem[i].osd_bmp_enabled) {
      camtype->add_overlay_bitmap(i, 1, bmp_offset, "/usr/local/bin/Ambarella-256x128-8bit.bmp");
    }
    if(osditem[i].osd_text_enabled) {
      text_offset.x = encodesize.width * osditem[i].osd_text.offset_x / 100;
      text_offset.y = encodesize.width * osditem[i].osd_text.offset_y / 100;
      box.width = encodesize.width * osditem[i].osd_text.width / 100;
      box.height = encodesize.width * osditem[i].osd_text.width / 100;
      font.size = osditem[i].osd_text.font_size;
      font.outline_width = osditem[i].osd_text.outline;
      font.ver_bold = osditem[i].osd_text.bold;
      font.hor_bold = osditem[i].osd_text.bold;
      font.italic = osditem[i].osd_text.italic;
      font.ttf = "/usr/share/fonts/Vera.ttf";
      switch(osditem[i].osd_text.font_color) {
      case(F_BLACK):
        coloryuv.v = 128;
        coloryuv.u = 128;
        coloryuv.y = 16;
        coloryuv.alpha = 0xff;
        break;
      case(F_RED):
        coloryuv.v = 240;
        coloryuv.u = 90;
        coloryuv.y = 82;
        coloryuv.alpha = 0xff;
        break;
      case(F_BLUE):
        coloryuv.v = 110;
        coloryuv.u = 240;
        coloryuv.y = 41;
        coloryuv.alpha = 0xff;
        break;
      case(F_GREEN):
        coloryuv.v = 34;
        coloryuv.u = 54;
        coloryuv.y = 145;
        coloryuv.alpha = 0xff;
        break;
      case(F_YELLOW):
        coloryuv.v = 146;
        coloryuv.u = 16;
        coloryuv.y = 210;
        coloryuv.alpha = 0xff;
        break;
      case(F_MAGENTA):
        coloryuv.v = 222;
        coloryuv.u = 202;
        coloryuv.y = 107;
        coloryuv.alpha = 0xff;
        break;
      case(F_CYAN):
        coloryuv.v = 16;
        coloryuv.u = 166;
        coloryuv.y = 170;
        coloryuv.alpha = 0xff;
        break;
      case(F_WHITE):
        coloryuv.v = 128;
        coloryuv.u = 128;
        coloryuv.y = 235;
        coloryuv.alpha = 0xff;
        break;
      default:
        break;
      }
     outlineyuv.v = 128;
     outlineyuv.u = 128;
     outlineyuv.y = 12;
     outlineyuv.alpha = 0xff;

     box.font = &font;
     box.font_color = &coloryuv;
     box.outline_color = &outlineyuv;
     camtype->add_overlay_text(i, 2, text_offset, &box, osditem[i].osd_text.text);
    }
  }

  if(camera_mode != WARP_MODE ) {
    webcam->reset_dptz_pm();
  }

  ret = true;
  return ret;
}

static bool start_encode()
{
    return camtype->start_encode() && resume_osd();
}

static void Set_prvacy_mask(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE PRIVACY MASK SETTING REQUEST!==========");
  if (! camtype->set_privacy_mask((PRIVACY_MASK*)msg_in->data)){
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

static void Get_prvacy_mask(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE PRIVACY MASK GETTING REQUEST!==========");
  uint32_t* pmid = (uint32_t *)(msg_out->data);
  if (!camtype->get_privacy_mask(pmid)) {
      //ERROR("pmid is %d", *pmid);
      msg_out->cmd_id = GET_PRVACY_MASK;
      msg_out->status = STATUS_SUCCESS;
      INFO("Get_prvacy_mask successful!");
    }
    else {
      msg_out->cmd_id = GET_PRVACY_MASK;
      msg_out->status = STATUS_FAILURE;
      INFO("Get_prvacy_mask Fail!");
    }
}

static void Set_osd_time(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE OSD TIME SETTING REQUEST!==========");
  uint32_t* streamid = (uint32_t*)msg_in->data;
  uint32_t* action = (uint32_t*)(msg_in->data + sizeof(uint32_t));
  TextBox box;
  Point offset;
  memset(&box, 0, sizeof(TextBox));
  EncodeSize encodesize;
  camtype->get_stream_size(*streamid, &encodesize);
  box.width = encodesize.width * 55 /100;
  box.height = encodesize.height * 12 /100;
  offset.x = encodesize.width * 15 /1000;
  offset.y = encodesize.height * 83 /1000;

  if(*action == 1) {
    if (camtype->add_overlay_time(*streamid, 0, offset, &box)){
      osditem[*streamid].osd_time_enabled = 1;
      msg_out->cmd_id = SET_OSD_TIME;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_TIME;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else if (*action == 0) {
    if (camtype->remove_overlay(*streamid, 0)){
      osditem[*streamid].osd_time_enabled = 0;
      msg_out->cmd_id = SET_OSD_TIME;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_TIME;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else {
    msg_out->cmd_id = SET_OSD_TIME;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_osd Fail!");
  }
}

static void Set_osd_bmp(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE OSD BMP SETTING REQUEST!==========");
  uint32_t* streamid = (uint32_t*)msg_in->data;
  uint32_t* action = (uint32_t*)(msg_in->data + sizeof(uint32_t));
  TextBox box;
  Point offset;
  memset(&box, 0, sizeof(TextBox));
  EncodeSize encodesize;
  camtype->get_stream_size(*streamid, &encodesize);
  box.width = encodesize.width * 55 /100;
  box.height = encodesize.height * 12 /100;
  offset.x = encodesize.width * 78 /100;
  offset.y = encodesize.height * 28 /1000;

  if(*action == 1) {
    if (camtype->add_overlay_bitmap(*streamid, 1, offset, "/usr/local/bin/Ambarella-256x128-8bit.bmp")){
      osditem[*streamid].osd_bmp_enabled = 1;
      msg_out->cmd_id = SET_OSD_BMP;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_BMP;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else if(*action == 0) {
    if (camtype->remove_overlay(*streamid, 1)){
      osditem[*streamid].osd_bmp_enabled = 0;
      msg_out->cmd_id = SET_OSD_BMP;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_BMP;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else {
    msg_out->cmd_id = SET_OSD_BMP;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_osd Fail!");
  }
}

static void Set_osd_text(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE OSD TEXT SETTING REQUEST!==========");
  uint32_t* streamid = (uint32_t*)msg_in->data;
  uint32_t* action = (uint32_t*)(msg_in->data + sizeof(uint32_t));
  OSDITEM* osd = (OSDITEM*)(msg_in->data+ 2*sizeof(uint32_t));
  OverlayClut coloryuv;
  OverlayClut outlineyuv;
  EncodeSize encodesize;
  camtype->get_stream_size(*streamid, &encodesize);

  Point offset;
  offset.x = encodesize.width * osd->osd_text.offset_x / 100;
  offset.y = encodesize.height * osd->osd_text.offset_y / 100;
  TextBox box;
  box.width = encodesize.width * osd->osd_text.width / 100;
  box.height = encodesize.height * osd->osd_text.height / 100;
  Font font;
  font.size = osd->osd_text.font_size;
  font.outline_width = osd->osd_text.outline;
  font.ver_bold = osd->osd_text.bold;
  font.hor_bold = osd->osd_text.bold;
  font.italic = osd->osd_text.italic;
  font.ttf = "/usr/share/fonts/Vera.ttf";

  switch(osd->osd_text.font_color) {
    case(F_BLACK):
      coloryuv.v = 128;
      coloryuv.u = 128;
      coloryuv.y = 16;
      coloryuv.alpha = 0xff;
      break;
    case(F_RED):
      coloryuv.v = 240;
      coloryuv.u = 90;
      coloryuv.y = 82;
      coloryuv.alpha = 0xff;
      break;
    case(F_BLUE):
      coloryuv.v = 110;
      coloryuv.u = 240;
      coloryuv.y = 41;
      coloryuv.alpha = 0xff;
      break;
    case(F_GREEN):
      coloryuv.v = 34;
      coloryuv.u = 54;
      coloryuv.y = 145;
      coloryuv.alpha = 0xff;
      break;
    case(F_YELLOW):
      coloryuv.v = 146;
      coloryuv.u = 16;
      coloryuv.y = 210;
      coloryuv.alpha = 0xff;
      break;
    case(F_MAGENTA):
      coloryuv.v = 222;
      coloryuv.u = 202;
      coloryuv.y = 107;
      coloryuv.alpha = 0xff;
      break;
    case(F_CYAN):
      coloryuv.v = 16;
      coloryuv.u = 166;
      coloryuv.y = 170;
      coloryuv.alpha = 0xff;
      break;
    case(F_WHITE):
      coloryuv.v = 128;
      coloryuv.u = 128;
      coloryuv.y = 235;
      coloryuv.alpha = 0xff;
      break;
    default:
      break;
  }
  outlineyuv.v = 128;
  outlineyuv.u = 128;
  outlineyuv.y = 12;
  outlineyuv.alpha = 0xff;

  box.font = &font;
  box.font_color = &coloryuv;
  box.outline_color = &outlineyuv;

  if(*action == 1) {
    if (camtype->add_overlay_text(*streamid, 2, offset, &box, osd->osd_text.text)){
      osditem[*streamid].osd_text_enabled = 1;
      sprintf(osditem[*streamid].osd_text.text, "%s", osd->osd_text.text);
      osditem[*streamid].osd_text.font_size = font.size;
      osditem[*streamid].osd_text.font_color = osd->osd_text.font_color;
      osditem[*streamid].osd_text.outline = font.outline_width;
      osditem[*streamid].osd_text.bold = font.ver_bold;
      osditem[*streamid].osd_text.italic = font.italic;
      osditem[*streamid].osd_text.offset_x = osd->osd_text.offset_x;
      osditem[*streamid].osd_text.offset_y = osd->osd_text.offset_y;
      osditem[*streamid].osd_text.width = osd->osd_text.width;
      osditem[*streamid].osd_text.height = osd->osd_text.height;
      msg_out->cmd_id = SET_OSD_TEXT;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_TEXT;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else if(*action == 0) {
    if (camtype->remove_overlay(*streamid, 2)){
      osditem[*streamid].osd_text_enabled = 0;
      memset(&(osditem[*streamid].osd_text), 0, sizeof(OSDText));
      osditem[*streamid].osd_text.font_size = 32;
      msg_out->cmd_id = SET_OSD_TEXT;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_osd successful!");
    }
    else {
      msg_out->cmd_id = SET_OSD_TEXT;
      msg_out->status = STATUS_FAILURE;
      INFO("Set_osd Fail!");
    }
  } else {
    msg_out->cmd_id = SET_OSD_TEXT;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_osd Fail!");
  }

}

static void Get_osd(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE OSD GET REQUEST!==========");
  uint32_t* streamid;
  OSDITEM* osd;
  msg_out->cmd_id = GET_OSD;
  msg_out->status = STATUS_SUCCESS;
  streamid = (uint32_t*)msg_in->data;
  osd = (OSDITEM*)(msg_out->data);
  *osd = osditem[*streamid];
  //ERROR("bmp IS %d", osd[*streamid].osd_bmp_enabled);
  INFO("Get_osd successful!");
}

static void Set_dptz(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE DPTZ SETTING REQUEST!==========");
  if (! camtype->set_digital_ptz((DPTZParam *)msg_in->data)){
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

static void Get_cam_type(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE CAM TYPE GET REQUEST!==========");
  msg_out->cmd_id = GET_CAM_TYPE;
  msg_out->status = STATUS_SUCCESS;
  uint32_t *camtype = (uint32_t *)msg_out->data;
  *camtype = camera_mode;
  INFO("Get_cam_type successful!");
}

static void Get_dptz_zpt_enable(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE ZOOM/PAN/TILT ENABLE OR NOT GET REQUEST!==========");
  uint32_t *id = (uint32_t *)msg_in->data;
  msg_out->cmd_id = GET_DPTZ_ZPT_ENABLE;
  msg_out->status = STATUS_SUCCESS;
  uint32_t *dptz_zpt = (uint32_t *)msg_out->data;
  if(camera_mode != WARP_MODE) {
    *dptz_zpt = 1;
  } else {
    if(transform_mode_saved[*id] == AM_TRANSFORM_MODE_SUBREGION) {
      *dptz_zpt = 1;
    } else{
      *dptz_zpt = 0;
    }
  }
  INFO("Get_dptz_zpt_enable successful!");
}

static void Get_dptz(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE DPTZ GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  DPTZParam dptz_out, *pdptz_out;
  if (! camtype->get_digital_ptz(*streamid, &dptz_out)){
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
  uint32_t i;
  bool streamexist = false;
  EncodeType streamtype;
  uint32_t *streamid = (uint32_t *)msg_in->data;
  const EncodeType *type = (const EncodeType *)(msg_in->data + sizeof(uint32_t));
  uint32_t totalstream = camtype->get_stream_max_num();

  if ( camtype->stop_encode() && camtype->set_stream_type(*streamid, *type)) {
    for(i = 0; i < totalstream; i++) {
      if(camtype->get_stream_type(i, &streamtype)) {
        if(streamtype != AM_ENCODE_TYPE_NONE) {
            streamexist = true;
            break;
        }
      }
    }
    if(streamexist) {
      if(start_encode()) {
        msg_out->cmd_id = SET_STREAM_TYPE;
        msg_out->status = STATUS_SUCCESS;
        INFO("Set_stream_type successful!");
      }
      else {
      if(camera_mode == WARP_MODE) {
        /*Start encode failed, then revert back*/
        fishcam->set_stream_type(*streamid, AM_ENCODE_TYPE_NONE);
        start_encode();
      }
      msg_out->cmd_id = SET_STREAM_TYPE;
      msg_out->status = STATUS_FAILURE;
      }
    } else {
      msg_out->cmd_id = SET_STREAM_TYPE;
      msg_out->status = STATUS_SUCCESS;
      INFO("Set_stream_type successful!");
    }
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
  if (camtype->get_stream_type(*streamid, &type)){
    msg_out->cmd_id = GET_STREAM_TYPE;
    msg_out->status = STATUS_SUCCESS;
    ptype = (EncodeType *)(msg_out->data);
    *ptype = type;
    INFO("Get_stream_type successful!");
    //ERROR("streamid:%d,type:%d",*streamid,*ptype);
  }
  else {
    msg_out->cmd_id = GET_STREAM_TYPE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_type Fail!");
  }
}

static void Set_stream_n(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM N SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint16_t *h264_n = (uint16_t *)(msg_in->data + sizeof(uint32_t));
  if (camtype->set_stream_N(*streamid, *h264_n)){
    msg_out->cmd_id = SET_H264_N;
    msg_out->status = STATUS_SUCCESS;
    //ERROR("h264_n:%d", *h264_n);
    INFO("Set_stream_n successful!");
  }
  else {
    msg_out->cmd_id = SET_CBR_BITRATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_n Fail!");
  }
}

static void Get_stream_n(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM N GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint16_t h264_n, *ph264_n;
  if (camtype->get_stream_N(*streamid, &h264_n)){
    msg_out->cmd_id = GET_H264_N;
    msg_out->status = STATUS_SUCCESS;
    ph264_n = (uint16_t *)msg_out->data;
    *ph264_n = h264_n;
    //ERROR("ph264_n:%d", *ph264_n);
    INFO("Get_stream_n successful!");
  }
  else {
    msg_out->cmd_id = GET_H264_N;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_n Fail!");
  }
}

static void Set_stream_idr(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM IDR SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t *idr_interval = (uint8_t *)(msg_in->data + sizeof(uint32_t));
  if (camtype->set_stream_idr(*streamid, *idr_interval)){
    msg_out->cmd_id = SET_IDR_INTERVAL;
    msg_out->status = STATUS_SUCCESS;
    //ERROR("idr_interval:%d", *idr_interval);
    INFO("Set_stream_idr successful!");
  }
  else {
    msg_out->cmd_id = SET_IDR_INTERVAL;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_idr Fail!");
  }
}

static void Get_stream_idr(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM IDR GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t idr, *pidr;
  if (camtype->get_stream_idr(*streamid, &idr)){
    msg_out->cmd_id = GET_IDR_INTERVAL;
    msg_out->status = STATUS_SUCCESS;
    pidr = (uint8_t *)msg_out->data;
    *pidr = idr;
    //ERROR("pidr:%d", *pidr);
    INFO("Get_stream_idr successful!");
  }
  else {
    msg_out->cmd_id = GET_IDR_INTERVAL;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_idr Fail!");
  }
}

static void Set_stream_profile(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM PROFILE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t *profile = (uint8_t *)(msg_in->data + sizeof(uint32_t));
  if (camtype->stop_encode() && camtype->set_stream_profile(*streamid, *profile) && start_encode()){
    msg_out->cmd_id = SET_PROFILE;
    msg_out->status = STATUS_SUCCESS;
    //ERROR("profile:%d", *profile);
    INFO("Set_stream_profile successful!");
  }
  else {
    msg_out->cmd_id = SET_PROFILE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_profile Fail!");
  }
}

static void Get_stream_profile(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM IDR GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t profile, *pprofile;
  if (camtype->get_stream_profile(*streamid, &profile)){
    msg_out->cmd_id = GET_PROFILE;
    msg_out->status = STATUS_SUCCESS;
    pprofile = (uint8_t *)msg_out->data;
    *pprofile = profile;
    //ERROR("pprofile:%d", *pprofile);
    INFO("Get_stream_profile successful!");
  }
  else {
    msg_out->cmd_id = GET_PROFILE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_profile Fail!");
  }
}

static void Set_cbr_bitrate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE CBR BITRATE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t *bitrate = (uint32_t *)(msg_in->data + sizeof(uint32_t));
  //ERROR("streamid:%d,bitrate:%d",*streamid, *bitrate);
  if (camtype->set_cbr_bitrate(*streamid, *bitrate)){
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
  if (camtype->get_cbr_bitrate(*streamid, &bitrate)){
    msg_out->cmd_id = GET_CBR_BITRATE;
    msg_out->status = STATUS_SUCCESS;
    pbitrate = (uint32_t *)(msg_out->data);
    *pbitrate = bitrate;
    //ERROR("bitrate:%d", *pbitrate);
    INFO("Get_cbr_bitrate successful!");
  }
  else {
    msg_out->cmd_id = GET_CBR_BITRATE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_cbr_bitrate Fail!");
  }
}

static void Set_mjpeg_quality(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE MJPEG QUALITY SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t *quality = (uint8_t *)(msg_in->data + sizeof(uint32_t));
  if (camtype->set_mjpeg_quality(*streamid, *quality)){
    msg_out->cmd_id = SET_MJPEG_Q;
    msg_out->status = STATUS_SUCCESS;
    //ERROR("quality:%d", *quality);
    INFO("Set_mjpeg_quality successful!");
  }
  else {
    msg_out->cmd_id = SET_MJPEG_Q;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_mjpeg_quality Fail!");
  }
}

static void Get_mjpeg_quality(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE MJPEG QUALITY GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint8_t quality, *pquality;
  if (camtype->get_mjpeg_quality(*streamid, &quality)){
    msg_out->cmd_id = GET_MJPEG_Q;
    msg_out->status = STATUS_SUCCESS;
    pquality = (uint8_t *)msg_out->data;
    *pquality = quality;
    //ERROR("pquality:%d", *pquality);
    INFO("Get_mjpeg_quality successful!");
  }
  else {
    msg_out->cmd_id = GET_MJPEG_Q;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_mjpeg_quality Fail!");
  }
}

static void Set_stream_framerate(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM FRAMERATE SETTING REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  uint32_t *framerate = (uint32_t *)(msg_in->data + sizeof(uint32_t));
  //ERROR("streamid:%d,framerate:%d",*streamid, *framerate);
  if (camtype->set_stream_framerate(*streamid, *framerate)){
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
  if (camtype->get_stream_framerate(*streamid, &framerate)){
    msg_out->cmd_id = GET_STREAM_FRAMERATE;
    msg_out->status = STATUS_SUCCESS;
    pframerate = (uint32_t *)(msg_out->data);
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
  EncodeType type;
  uint32_t *streamid = (uint32_t *)msg_in->data;
  EncodeSize *encodesize = (EncodeSize *)(msg_in->data + sizeof(uint32_t));
  //ERROR("Num:%d,width:%d,height:%d,rotate:%d", *streamid, encodesize->width,encodesize->height,encodesize->rotate);
  if (camtype->get_stream_type(*streamid, &type)) {
    if(type == AM_ENCODE_TYPE_MJPEG) {
      if((encodesize->rotate > 3) && ((encodesize->height & 0xF) || (encodesize->width & 0x7))) {
        msg_out->cmd_id = SET_STREAM_SIZE;
        msg_out->status = STATUS_FAILURE;
        INFO("Set_stream_size Fail!");
        return;
      }
    }
  }
  if (camtype->set_stream_size(*streamid, encodesize) && start_encode()){
    msg_out->cmd_id = SET_STREAM_SIZE;
    msg_out->status = STATUS_SUCCESS;
    INFO("Set_stream_size successful!");
  }
  else {
    msg_out->cmd_id = SET_STREAM_SIZE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_stream_size Fail!");
  }
}

static void Get_stream_size(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE STREAM SIZE GET REQUEST!==========");
  uint32_t *streamid = (uint32_t *)msg_in->data;
  EncodeSize encodesize, *pencodesize;
  if (camtype->get_stream_size(*streamid, &encodesize)){
    msg_out->cmd_id = GET_STREAM_SIZE;
    msg_out->status = STATUS_SUCCESS;
    pencodesize = (EncodeSize *)(msg_out->data);
    *pencodesize = encodesize;
    INFO("Get_stream_size successful!");
  }
  else {
    msg_out->cmd_id = GET_STREAM_SIZE;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_stream_size Fail!");
  }
}

/*************begin fishcam sepcial setting***************/
static void Set_transform_mode(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE TRANSFORM MODE SETTING REQUEST!==========");
  uint32_t i = 0;
  uint32_t *num = (uint32_t *)msg_in->data;
  TransformMode *transformmode = (TransformMode *)(msg_in->data + sizeof(uint32_t));
  //ERROR("Num:%d,width:%d,height:%d,rotate:%d", *num, encodesize->width,encodesize->height,encodesize->rotate);
  if (fishcam->set_transform_mode_all(*num, transformmode)){
    msg_out->cmd_id = SET_TRANSFORM_MODE;
    msg_out->status = STATUS_SUCCESS;
    memset(transform_mode_saved, 0, sizeof(TransformMode)*MAX_FISHEYE_REGION_NUM);
    for (i = 0 ;i < *num;i++){
      transform_mode_saved[i] = *(transformmode + i);
    }
    transform_mode_num = *num;
    //fishcam->start_encode();
    INFO("Set_transform_mode successful!");
  }
  else {
    msg_out->cmd_id = SET_TRANSFORM_MODE;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_transform_mode Fail!");
  }
}

static void Get_transform_mode(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE TRANSFORM MODE GETTING REQUEST!==========");
  uint32_t i = 0;
  uint32_t *num = (uint32_t *)msg_out->data;
  *num = transform_mode_num;
  TransformMode *transformmode = (TransformMode *)(msg_out->data + sizeof(uint32_t));
  msg_out->cmd_id = GET_TRANSFORM_MODE;
  msg_out->status = STATUS_SUCCESS;
  for (i = 0 ;i < *num;i++){
    *(transformmode + i) = transform_mode_saved[i];
  }

  //fishcam->start_encode();
  INFO("Get_transform_mode successful!");
}

static void Set_transform_region(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE TRANSFORM REGION SETTING REQUEST!==========");
  uint32_t i = 0;
  uint32_t *num = (uint32_t *)msg_in->data;
  uint32_t *type = (uint32_t *)(msg_in->data + sizeof(uint32_t));
  TransformParameters *transformparameters = (TransformParameters *)(msg_in->data + 2*sizeof(uint32_t));
  //ERROR("Num:%d,width:%d,height:%d,rotate:%d", *num, encodesize->width,encodesize->height,encodesize->rotate);
  if (fishcam->set_transform_region(*num, transformparameters)){
    msg_out->cmd_id = SET_TRANSFORM_REGION;
    msg_out->status = STATUS_SUCCESS;
    if(*type == TRANSFORM_REGION_REPLACE) {
      for (i = 0 ;i < *num;i++){
        transform_parameter_saved[i] = *(transformparameters + i);
        transform_parameter_num = *num;
      }
    } else {
      i = transformparameters->id;
      transform_parameter_saved[i] = *(transformparameters);
    }

    //fishcam->start_encode();
    INFO("Set_transform_mode successful!");
  }
  else {
    msg_out->cmd_id = SET_TRANSFORM_REGION;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_transform_mode Fail!");
  }
}

static void Set_transform_zpt(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE TRANSFORM REGION SETTING REQUEST!==========");
  TransParam *zpt = (TransParam *)(msg_in->data);
  uint32_t i = zpt->id;
  //ERROR("id:%d,numer:%d,demon:%d,pan:%d,tilt:%d", zpt->id, zpt->zoom.numer,zpt->zoom.denom,zpt->pantilt_angle.pan, zpt->pantilt_angle.tilt);
  TransformParameters transform_parameter_tmp = transform_parameter_saved[i];
  transform_parameter_tmp.zoom.denom = zpt->zoom.denom;
  transform_parameter_tmp.zoom.numer = zpt->zoom.numer;
  transform_parameter_tmp.pantilt_angle.pan = zpt->pantilt_angle.pan;
  transform_parameter_tmp.pantilt_angle.tilt = zpt->pantilt_angle.tilt;
  if (fishcam->set_transform_region(1, &transform_parameter_tmp)){
    msg_out->cmd_id = SET_TRANSFORM_ZPT;
    msg_out->status = STATUS_SUCCESS;
    transform_parameter_saved[i] = transform_parameter_tmp;

    //fishcam->start_encode();
    INFO("Set_transform_zpt successful!");
  }
  else {
    msg_out->cmd_id = SET_TRANSFORM_ZPT;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_transform_zpt Fail!");
  }
}

static void Get_transform_region(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE TRANSFORM REGION SETTING REQUEST!==========");
  uint32_t i = 0;
  uint32_t *num = (uint32_t *)msg_out->data;
  TransformParameters *transformparameters = (TransformParameters *)(msg_out->data + sizeof(uint32_t));
  msg_out->cmd_id = GET_TRANSFORM_REGION;
  msg_out->status = STATUS_SUCCESS;
  *num = transform_parameter_num;
  for (i = 0 ;i < *num;i++){
    *(transformparameters + i) = transform_parameter_saved[i];
  }
  //fishcam->start_encode();
  INFO("Set_transform_mode successful!");
}

static int transform_mode_setting(uint32_t num, TransformMode *transformmode)
{
  INFO("==========RECEIVE TRANSFORM MODE SETTING REQUEST!==========");
  uint32_t i = 0;
  if (fishcam->set_transform_mode_all(num, transformmode)){
    memset(transform_mode_saved, 0, sizeof(TransformMode)*MAX_FISHEYE_REGION_NUM);
    for (i = 0 ;i < num;i++){
      transform_mode_saved[i] = *(transformmode + i);
    }
    transform_mode_num = num;
    //fishcam->start_encode();
    INFO("Set_transform_mode successful!");
  }
  else {
    INFO("Set_transform_mode Fail!");
    return -1;
  }
  return 0;
}

static int transform_region_setting(uint32_t num, TransformParameters *transformparameters)
{
  INFO("==========RECEIVE TRANSFORM REGION SETTING REQUEST!==========");
  uint32_t i = 0;
    /*for(i = 0; i<num;i++)  ERROR("width:%d,height:%d,x:%d,y:%d,angle1:%d,angle2:%d,", transformparameters[i].region.width, transformparameters[i].region.height, transformparameters[i].region.x,\
            transformparameters[i].region.y, transformparameters[i].hor_angle_range, transformparameters[i].roi_top_angle);*/
  if (fishcam->set_transform_region(num, transformparameters)){
    for (i = 0 ;i < num;i++){
        transform_parameter_saved[i] = *(transformparameters + i);
        transform_parameter_num = num;
    }
    //fishcam->start_encode();
    INFO("Set_transform_region successful!");
  }
  else {
    INFO("Set_transform_region Fail!");
    return -1;
  }
  return 0;
}

static int stream_type_fish_setting(uint32_t num, uint32_t* streamid, EncodeType *type)
{
  uint32_t i = 0;

  for(i = 0; i < num; i++) {
      if (fishcam->set_stream_type(streamid[i], type[i])){
        INFO("Set_stream_type successful!");
      } else {
        INFO("Set_stream_type Fail!");
        return -1;
    }
  }
  return 0;
}

static int stream_size_fish_setting(uint32_t num, EncodeSize *encodesize)
{
  INFO("==========RECEIVE STREAM SIZE SETTING REQUEST!==========");
  if (fishcam->stop_encode() && fishcam->set_stream_size_all(num, encodesize)){
    //fishcam->Start();
    start_encode();
    INFO("Set_stream_size_fish successful!");
  }
  else {
    INFO("Set_stream_size_fish Fail!");
    return -1;
  }
  return 0;
}

static int Set_fisheye_layout(uint32_t imgwidth, uint32_t imgheight, uint32_t layout)
{
     INFO("==========BEGIN FISHEYE LAYOUT SETTING!==========");
     int ret_code = 0;
     TransformMode transformmode[4];
     TransformParameters transform[4] = {TransformParameters(0, Rect(2048, 2048), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(2048, 2048)),
                                  TransformParameters(0, Rect(2048, 2048), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(2048, 2048)),
                                  TransformParameters(0, Rect(2048, 2048), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(2048, 2048)),
                                  TransformParameters(0, Rect(2048, 2048), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(2048, 2048))};
     uint32_t streamid[4] = {AM_STREAM_ID_0, AM_STREAM_ID_1, AM_STREAM_ID_2,AM_STREAM_ID_3};
     EncodeType encodetype[4] = {AM_ENCODE_TYPE_H264, AM_ENCODE_TYPE_H264, AM_ENCODE_TYPE_H264, AM_ENCODE_TYPE_H264};
     EncodeSize encodesize[4] = {EncodeSize(2048, 2048),
                                    EncodeSize(2048, 2048),
                                    EncodeSize(2048, 2048),
                                    EncodeSize(2048, 2048)};
     uint32_t trans_number = 0;
     uint32_t stream_number = 0;

     switch (layout){
      case WALL_FISHEYE:
      case CEILING_FISHEYE:
        trans_number = 1;
        stream_number = 1;

        transformmode[0] = AM_TRANSFORM_MODE_NONE;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth, imgheight), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(imgwidth, imgheight));
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth, imgheight, 0, 0, Rect());

        break;

      case WALL_NORMAL:
        trans_number = 1;
        stream_number = 1;

        transformmode[0] = AM_TRANSFORM_MODE_NORMAL;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth, imgheight), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth, imgheight, 0, 0, Rect());
        break;

      case WALL_FISHEYE_SUBREGION_PANORAMA180:
        trans_number = 3;
        stream_number = 3;
        transformmode[0] = AM_TRANSFORM_MODE_PANORAMA;
        transformmode[1] = AM_TRANSFORM_MODE_NONE;
        transformmode[2] = AM_TRANSFORM_MODE_SUBREGION;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth, imgheight/2, 0, imgheight/2), Fraction(),
                                  180, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());
        transform[1].SetTransformParameters(1, Rect(imgwidth/2, imgheight/2), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(imgwidth, imgheight, 0, 0));
        transform[2].SetTransformParameters(2, Rect(imgwidth/2, imgheight/2, imgwidth/2, 0), Fraction(3,2),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());

        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[2].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect(0,0,imgwidth/2,0));
        encodesize[0].SetEncodeSize(imgwidth, imgheight/2, 0, 0, Rect(0,0,0,imgheight/2));
        encodesize[1].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect());

        break;

      case CEILING_NORTH_SOUTH:
        trans_number = 2;
        stream_number = 2;
        transformmode[0] = AM_TRANSFORM_MODE_NORMAL;
        transformmode[1] = AM_TRANSFORM_MODE_NORMAL;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth, imgheight/2), Fraction(),
                                  0, 90, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());
        transform[1].SetTransformParameters(1, Rect(imgwidth, imgheight/2, 0, imgheight/2), Fraction(),
                                  0, 90, AM_FISHEYE_ORIENT_SOUTH,
                                  Point(), PanTiltAngle(), Rect());
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth, imgheight/2, 0, 0, Rect());
        encodesize[1].SetEncodeSize(imgwidth, imgheight/2, 0, 0, Rect(0,0,0,imgheight/2));

        break;

      case CEILING_WEST_EAST:
        trans_number = 2;
        stream_number = 2;
        transformmode[0] = AM_TRANSFORM_MODE_NORMAL;
        transformmode[1] = AM_TRANSFORM_MODE_NORMAL;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth, imgheight/2), Fraction(),
                                  0, 90, AM_FISHEYE_ORIENT_WEST,
                                  Point(), PanTiltAngle(), Rect());
        transform[1].SetTransformParameters(1, Rect(imgwidth, imgheight/2, 0, imgheight/2), Fraction(),
                                  0, 90, AM_FISHEYE_ORIENT_EAST,
                                  Point(), PanTiltAngle(), Rect());
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth, imgheight/2, 0, 0, Rect());
        encodesize[1].SetEncodeSize(imgwidth, imgheight/2, 0, 0, Rect(0,0,0,imgheight/2));

        break;

      case CEILING_SUBREGIONX4:
        trans_number = 4;
        stream_number = 4;
        transformmode[0] = AM_TRANSFORM_MODE_SUBREGION;
        transformmode[1] = AM_TRANSFORM_MODE_SUBREGION;
        transformmode[2] = AM_TRANSFORM_MODE_SUBREGION;
        transformmode[3] = AM_TRANSFORM_MODE_SUBREGION;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth/2, imgheight/2, 0, 0), Fraction(3,2),
                                  0, 0, AM_FISHEYE_ORIENT_EAST,
                                  Point(), PanTiltAngle(), Rect());
        transform[1].SetTransformParameters(1, Rect(imgwidth/2, imgheight/2, imgwidth/2, 0), Fraction(3,2),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());
        transform[2].SetTransformParameters(2, Rect(imgwidth/2, imgheight/2, 0, imgheight/2), Fraction(3,2),
                                  0, 0, AM_FISHEYE_ORIENT_WEST,
                                  Point(), PanTiltAngle(), Rect());
        transform[3].SetTransformParameters(3, Rect(imgwidth/2, imgheight/2, imgwidth/2, imgheight/2), Fraction(3,2),
                                  0, 0, AM_FISHEYE_ORIENT_SOUTH,
                                  Point(), PanTiltAngle(), Rect());
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect());
        encodesize[1].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect(0,0,imgwidth/2,0));
        encodesize[2].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect(0,0,0,imgheight/2));
        encodesize[3].SetEncodeSize(imgwidth/2, imgheight/2, 0, 0, Rect(0,0,imgwidth/2,imgheight/2));

        break;

      case CEILING_PANORAMA360:
        trans_number = 4;
        stream_number = 1;
        transformmode[0] = AM_TRANSFORM_MODE_PANORAMA;
        transformmode[1] = AM_TRANSFORM_MODE_PANORAMA;
        transformmode[2] = AM_TRANSFORM_MODE_PANORAMA;
        transformmode[3] = AM_TRANSFORM_MODE_PANORAMA;
        memset(transform, 0, sizeof(TransformParameters)*4);
        transform[0].SetTransformParameters(0, Rect(imgwidth/4, imgheight), Fraction(3,2),
                                  90, 90, AM_FISHEYE_ORIENT_WEST,
                                  Point(), PanTiltAngle(), Rect());
        transform[1].SetTransformParameters(1, Rect(imgwidth/4, imgheight, imgwidth/4, 0), Fraction(3,2),
                                  90, 90, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect());
        transform[2].SetTransformParameters(2, Rect(imgwidth/4, imgheight, imgwidth/2, 0), Fraction(3,2),
                                  90, 90, AM_FISHEYE_ORIENT_EAST,
                                  Point(), PanTiltAngle(), Rect());
        transform[3].SetTransformParameters(3, Rect(imgwidth/4, imgheight, imgwidth*3/4, 0), Fraction(3,2),
                                  90, 90, AM_FISHEYE_ORIENT_SOUTH,
                                  Point(), PanTiltAngle(), Rect());
        memset(encodesize, 0, sizeof(EncodeSize)*4);
        encodesize[0].SetEncodeSize(imgwidth, imgheight, 0, 0, Rect());
        break;

      default:
        ERROR("NOT SUPPORTED LAYOUT!");
        return -5;
    }

     do{
        if (transform_mode_setting(trans_number, transformmode) < 0) {
            ret_code = -1;
            break;
        }
        if (transform_region_setting(trans_number, transform) < 0) {
            ret_code = -2;
            break;
        }
        if (stream_type_fish_setting(stream_number, streamid, encodetype) < 0) {
            ret_code = -3;
            break;
        }
        if (stream_size_fish_setting(stream_number, encodesize) < 0) {
            ret_code = -4;
            break;
        }

     }while(0);


     return ret_code;

}

static void Set_fisheye_parameters(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE FISHEYE PARAMETERS SETTING REQUEST!==========");
  FisheyeParameters *pFisheyeParameters = (FisheyeParameters *)(msg_in->data);
  FISHEYE_LAYOUT *layout = (FISHEYE_LAYOUT *)(msg_in->data + sizeof(FisheyeParameters));
  layout_saved = *layout;
  if (fishcam->stop_encode() && fisheye_config->set(*pFisheyeParameters)){
    //INFO("ADD PM successfully!");
    fishcam->init();
    fishcam->start(fisheye_config);
    //fishcam->start_encode();
    fisheyeparameters_saved = *pFisheyeParameters;
    INFO("==========FISHEYE CAMERA START ENCODE!==========");

    if (Set_fisheye_layout(pFisheyeParameters->layout.warp.width, pFisheyeParameters->layout.warp.height, layout->layout) < 0) {
        msg_out->cmd_id = SET_FISHEYE_PARAMETERS;
        msg_out->status = STATUS_FAILURE;
        INFO("Set_fisheye_parameters Fail!");
    } else {
        msg_out->cmd_id = SET_FISHEYE_PARAMETERS;
        msg_out->status = STATUS_SUCCESS;
        INFO("Set_fisheye_parameters successful!");
    }
  }
  else {
    //INFO("fail to ADD PM");
    msg_out->cmd_id = SET_FISHEYE_PARAMETERS;
    msg_out->status = STATUS_FAILURE;
    INFO("Set_fisheye_parameters Fail!");
  }
}

static void Get_fisheye_parameters(MESSAGE *msg_in, MESSAGE *msg_out)
{
  INFO("==========RECEIVE FISHEYE PARAMETERS GETTING REQUEST!==========");
  FisheyeParameters fisheyeparameters, *pfisheyeparameters;
  if (fisheye_config->get(&fisheyeparameters)){
    msg_out->cmd_id = GET_FISHEYE_PARAMETERS;
    msg_out->status = STATUS_SUCCESS;
    pfisheyeparameters = (FisheyeParameters *)msg_out->data;
    *pfisheyeparameters = fisheyeparameters;
    FISHEYE_LAYOUT *layout = (FISHEYE_LAYOUT *)(msg_out->data + sizeof(FisheyeParameters));
    *layout = layout_saved;
    INFO("Get_fisheye_parameters successful!");
  }
  else {
    msg_out->cmd_id = GET_FISHEYE_PARAMETERS;
    msg_out->status = STATUS_FAILURE;
    INFO("Get_fisheye_parameters Fail!");
  }
}

static void Multi_Set(MESSAGE *in, MESSAGE *out)
{
  INFO("==========RECEIVE MULTI SETTING REQUEST!==========");
  MULTI_MESSAGES *msg_in = (MULTI_MESSAGES*)in;
  MULTI_MESSAGES *msg_out = (MULTI_MESSAGES*)out;
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
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(PRIVACY_MASK);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_OSD_TIME:
        Set_osd_time((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + 2*sizeof(uint32_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_OSD_BMP:
        Set_osd_bmp((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + 2*sizeof(uint32_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_OSD_TEXT:
        Set_osd_text((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) +  sizeof(OSDITEM) + 2*sizeof(uint32_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_DPTZ:
        Set_dptz((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(DPTZParam);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_STREAM_TYPE:
        Set_stream_type((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeType);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

     case SET_H264_N:
        Set_stream_n((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

     case SET_IDR_INTERVAL:
        Set_stream_idr((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

     case SET_PROFILE:
        Set_stream_profile((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

     case SET_CBR_BITRATE:
        Set_cbr_bitrate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

     case SET_MJPEG_Q:
        Set_mjpeg_quality((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

      case SET_STREAM_FRAMERATE:
        Set_stream_framerate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;

       case SET_STREAM_SIZE:
        {
        Set_stream_size((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(EncodeSize);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;
        }
       case SET_TRANSFORM_MODE:
        {
        Set_transform_mode((MESSAGE *)data, (MESSAGE *)data_output);
        uint32_t *pnum = (uint32_t *)(data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t));
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(TransformMode) * (*pnum);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;
        }

       case SET_TRANSFORM_REGION:
        {
        Set_transform_region((MESSAGE *)data, (MESSAGE *)data_output);
        uint32_t *pnum = (uint32_t *)(data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t));
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(TransformParameters) * (*pnum);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        msg_out->count++;
        break;
        }

       case SET_FISHEYE_PARAMETERS:
        {
        Set_fisheye_parameters((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(FisheyeParameters) + sizeof(FISHEYE_LAYOUT);
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
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
  uint32_t *status = (uint32_t *)(data_out + sizeof(ENC_COMMAND_ID));
  return *status;
}
static void Multi_Get(MESSAGE *in, MESSAGE *out)
{
  INFO("==========RECEIVE MULTI GET REQUEST!==========");
  MULTI_MESSAGES *msg_in = (MULTI_MESSAGES*)in;
  MULTI_MESSAGES *msg_out = (MULTI_MESSAGES*)out;
  uint32_t msg_count = msg_in->count;
  uint32_t i = 0, flag_unknown_cmd = 0;
  char *data = msg_in->data;
  char *data_output = msg_out->data;
  msg_out->cmd_id = MULTI_GET;
  msg_out->count = 0;
  for (i = 0;i < msg_count;i++){
    uint32_t cmd_id = *(uint32_t *)data;
    switch(cmd_id){
      case GET_CAM_TYPE:
        Get_cam_type((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        msg_out->count++;
        break;
      case GET_PRVACY_MASK:
        Get_prvacy_mask((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        msg_out->count++;
        break;
      case GET_DPTZ_ZPT_ENABLE:
        Get_dptz_zpt_enable((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        msg_out->count++;
        break;
      case GET_OSD:
        Get_osd((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(OSDText);
        }
        msg_out->count++;
        break;
      case GET_DPTZ:
        Get_dptz((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(DPTZParam);
        }
        msg_out->count++;
        break;

      case GET_STREAM_TYPE:
        Get_stream_type((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(EncodeType);
        }
        msg_out->count++;
        break;

     case GET_H264_N:
        Get_stream_n((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint16_t);
        }
        msg_out->count++;
        break;

      case GET_IDR_INTERVAL:
        Get_stream_idr((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint8_t);
        }
        msg_out->count++;
        break;

      case GET_PROFILE:
        Get_stream_profile((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint8_t);
        }
        msg_out->count++;
        break;

      case GET_CBR_BITRATE:
        Get_cbr_bitrate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        }
        msg_out->count++;
        break;

      case GET_MJPEG_Q:
        Get_mjpeg_quality((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint8_t);
        }
        msg_out->count++;
        break;

      case GET_STREAM_FRAMERATE:
        Get_stream_framerate((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) +sizeof(uint32_t);
        }
        msg_out->count++;
        break;

      case GET_STREAM_SIZE:
        Get_stream_size((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(EncodeSize);
        }
        msg_out->count++;
        break;

      case GET_FISHEYE_PARAMETERS:
        Get_fisheye_parameters((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(FisheyeParameters) + sizeof(FISHEYE_LAYOUT);
        }
        msg_out->count++;
        break;

      case GET_TRANSFORM_REGION:
        Get_transform_region((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + \
          transform_parameter_num * sizeof(TransformParameters);
        }
        msg_out->count++;
        break;

      case GET_TRANSFORM_MODE:
        Get_transform_mode((MESSAGE *)data, (MESSAGE *)data_output);
        data = data + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        if (get_status(data_output)){
          data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t);
        }
        else {
        data_output = data_output + sizeof(ENC_COMMAND_ID) + sizeof(uint32_t) + sizeof(uint32_t) + \
          transform_mode_num * sizeof(TransformMode);
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

static void process_cmd(int cmd_id) {
    void (*processlist[CMD_COUNT]) (MESSAGE*, MESSAGE*);
    processlist[SET_PRVACY_MASK] = Set_prvacy_mask;
    processlist[GET_PRVACY_MASK] = Get_prvacy_mask;
    processlist[GET_CAM_TYPE] = Get_cam_type;
    processlist[GET_DPTZ_ZPT_ENABLE] = Get_dptz_zpt_enable;
    processlist[SET_DPTZ] = Set_dptz;
    processlist[GET_DPTZ] = Get_dptz;
    processlist[SET_STREAM_TYPE] = Set_stream_type;
    processlist[GET_STREAM_TYPE] = Get_stream_type;
    processlist[SET_CBR_BITRATE] = Set_cbr_bitrate;
    processlist[GET_CBR_BITRATE] = Get_cbr_bitrate;
    processlist[SET_STREAM_FRAMERATE] = Set_stream_framerate;
    processlist[GET_STREAM_FRAMERATE] = Get_stream_framerate;
    processlist[SET_STREAM_SIZE] = Set_stream_size;
    processlist[GET_STREAM_SIZE] = Get_stream_size;
    processlist[SET_H264_N] = Set_stream_n;
    processlist[GET_H264_N] = Get_stream_n;
    processlist[SET_IDR_INTERVAL] = Set_stream_idr;
    processlist[GET_IDR_INTERVAL] = Get_stream_idr;
    processlist[SET_PROFILE] = Set_stream_profile;
    processlist[GET_PROFILE] = Get_stream_profile;
    processlist[SET_MJPEG_Q] = Set_mjpeg_quality;
    processlist[GET_MJPEG_Q] = Get_mjpeg_quality;
    processlist[SET_OSD_TIME] = Set_osd_time;
    processlist[SET_OSD_BMP] = Set_osd_bmp;
    processlist[SET_OSD_TEXT] = Set_osd_text;
    processlist[GET_OSD] = Get_osd;
    processlist[MULTI_SET] = Multi_Set;
    processlist[MULTI_GET] = Multi_Get;
    processlist[SET_TRANSFORM_MODE] = Set_transform_mode;
    processlist[GET_TRANSFORM_MODE] = Get_transform_mode;
    processlist[SET_TRANSFORM_REGION] = Set_transform_region;
    processlist[SET_TRANSFORM_ZPT] = Set_transform_zpt;
    processlist[GET_TRANSFORM_REGION] = Get_transform_region;
    processlist[SET_FISHEYE_PARAMETERS] = Set_fisheye_parameters;
    processlist[GET_FISHEYE_PARAMETERS] = Get_fisheye_parameters;
    (*processlist[cmd_id])((MESSAGE *)receive_buffer, (MESSAGE *)send_buffer);
}

static void  receive_cb_cam(union sigval sv)
{
  while (mq_receive(*(mqd_t *)sv.sival_ptr, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) >= 0){
    //INFO("RECEIVE MESSAGE:%s", receive_buffer);
    uint32_t i = 0;
    while(i < CMD_COUNT) {
      if(i == receive_buffer->cmd_id) {
        process_cmd(i);
        break;
      }
      i++;
    }
    if(i == CMD_COUNT) {
      send_buffer->cmd_id = UNSUPPORTED_ID;
      sprintf(send_buffer->data, "UNSUPPORTED_ID!");
      ERROR("NOT SUPPORTED COMMAND!");
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
  ERROR("Usage: webenc_server "
            "[cli/daemon]"
            "| [fullfr/dewarp/highmega]\
                   \n\t\t\t\tfullfr: full frame rate mode; dewarp: multi region warp mode;\
			\n\t\t\t\thighmega: high mega pixel mode;");
}

static void sighandler(int sig_no)
{
  /* Save stream configuration */

  if (camera_mode != WARP_MODE) {
    if (!error) {
      for (uint32_t i = 0; i < (config->vdevice_config())->stream_number; ++i) {
        config->set_stream_config(config->stream_config(i), i);
      }
    }

    if (webcam->stop_encode()) {
      INFO("Stop all streams successfully!");
    } else {
      ERROR("Failed to stop all streams!");
      error = true;
    }
    camtype = NULL;
    delete webcam;
  }else {
    if (fishcam->stop_encode()) {
      INFO("Stop all streams successfully!");
    } else {
      ERROR("Failed to stop all streams!");
      error = true;
    }
    camtype = NULL;
    delete fishcam;
  }
  delete config;

  //delete encode_device;
  exit(1);
}

int create_message_queuqe( mq_attr* attr) {
  mq_unlink(ENC_MQ_SEND);
  mq_unlink(ENC_MQ_RECEIVE);
  msg_queue_send = mq_open(ENC_MQ_SEND, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG, attr);
  if (msg_queue_send < 0)  {
      perror("Error Opening MQ: ");
      return -1;
  }
  msg_queue_receive = mq_open(ENC_MQ_RECEIVE, O_RDONLY | O_CREAT |O_NONBLOCK, S_IRWXU | S_IRWXG, attr);
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
  return 0;
}

int main(int argc, char *argv[])
{
  if ((argc != 3)
      || (!is_str_equal("cli", argv[1])
          && !is_str_equal("daemon", argv[1]))) {
    usage();
    exit(0);
  }

  int i;
  for(i = 0; i < MODE_TOTAL_NUM; i++) {
    if(is_str_equal(mode_list[i], argv[2])) {
      camera_mode = i;
      break;
    }
  }
  if(camera_mode < 0) {
    usage();
    exit(0);
  }

  if (is_str_equal("daemon", argv[1])) {
    daemon(1, 0);
  }

  config = new AmConfig();
  EncodeSize encodesize;
  memset(&encodesize, 0, sizeof(EncodeSize));
  switch(camera_mode) {
    case FULL_FRAMERATE_MODE:
      config->set_vin_config_path(BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf");
      webcam = new AmWebCam(config);
      camtype = webcam;
      if (!webcam->init()) {
        ERROR("Failed to init webcam.");
        return -1;
      }
      if (webcam->start_encode()) {
        INFO("Start all streams successfully!");
      } else {
        ERROR("Failed to start all streams!");
        error = true;
      }
      break;
   case HIGH_MEGA_MODE:
      config->set_vin_config_path(BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf");
      config->set_vdevice_config_path (BUILD_AMBARELLA_CAMERA_CONF_DIR"/highmega_vdevice.conf");
      webcam = new AmWebCam(config);
      camtype = webcam;
      if (!webcam->init()) {
        ERROR("Failed to init webcam.");
        return -1;
      }
      if (webcam->start_encode()) {
        INFO("Start all streams successfully!");
      } else {
        ERROR("Failed to start all streams!");
        error = true;
      }
      break;
   case WARP_MODE:
      config->set_vin_config_path(BUILD_AMBARELLA_CAMERA_CONF_DIR"/warp_vin.conf");
      config->set_vdevice_config_path (BUILD_AMBARELLA_CAMERA_CONF_DIR"/warp_vdevice.conf");
      fishcam = new AmFisheyeCam(config);
      camtype = fishcam;
      fisheye_config = config->fisheye_config();
      if (!fishcam->init()) {
        ERROR("Failed to init fishcam.");
        return -1;
      }
      memset(&layout_saved, 0, sizeof(FISHEYE_LAYOUT));
      encodesize.height = fisheye_config->layout.warp.height;
      encodesize.width= fisheye_config->layout.warp.width;

      if (fishcam->start(fisheye_config) && \
        fishcam->set_stream_type(AM_STREAM_ID_0, AM_ENCODE_TYPE_H264) && \
        fishcam->set_stream_size_all(1,&encodesize)&& fishcam->start_encode()) {
        INFO("Start all streams successfully!");
      } else {
        ERROR("Failed to start all streams!");
        error = true;
      }
      break;

    default:
      break;
  }

  //create Message Queue

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MESSAGES_SIZE;
  attr.mq_curmsgs = 0;
  if (create_message_queuqe( &attr) < 0) {
    ERROR("Failed to create mq.");
    return -1;
  }


  //start encode

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

//Set this flag to test IDR interval,N,Profile
#if 0
uint8_t idr_interval;
uint16_t n;
uint8_t profile;

getchar();
webcam->mEncodeDevice->set_stream_idr(0, 2);
webcam->mEncodeDevice->set_stream_n(0, 15);
webcam->mEncodeDevice->set_stream_profile(0, 1);

webcam->mEncodeDevice->get_stream_idr(0, &idr_interval);
webcam->mEncodeDevice->get_stream_n(0, &n);
webcam->mEncodeDevice->get_stream_profile(0, &profile);
ERROR("IDR:%d,N:%d,Profile:%d", idr_interval, n, profile);

getchar();
webcam->mEncodeDevice->set_stream_idr(0, 4);
webcam->mEncodeDevice->set_stream_n(0, 28);
webcam->mEncodeDevice->set_stream_profile(0, 2);

webcam->mEncodeDevice->get_stream_idr(0, &idr_interval);
webcam->mEncodeDevice->get_stream_n(0, &n);
webcam->mEncodeDevice->get_stream_profile(0, &profile);
ERROR("IDR:%d,N:%d,Profile:%d", idr_interval, n, profile);

getchar();
webcam->mEncodeDevice->set_stream_idr(0, 1);
webcam->mEncodeDevice->set_stream_n(0, 30);
webcam->mEncodeDevice->set_stream_profile(0, 0);

webcam->mEncodeDevice->get_stream_idr(0, &idr_interval);
webcam->mEncodeDevice->get_stream_n(0, &n);
webcam->mEncodeDevice->get_stream_profile(0, &profile);
ERROR("IDR:%d,N:%d,Profile:%d", idr_interval, n, profile);

#endif

//Set this flag to test MJPEG Quality
#if 0
uint8_t quality;

getchar();
webcam->mEncodeDevice->set_mjpeg_quality(0, 30);
webcam->mEncodeDevice->get_mjpeg_quality(0,&quality);
ERROR("MJPEG Quality:%d",quality);

getchar();
webcam->mEncodeDevice->set_mjpeg_quality(0, 40);
webcam->mEncodeDevice->get_mjpeg_quality(0,&quality);
ERROR("MJPEG Quality:%d",quality);

getchar();
webcam->mEncodeDevice->set_mjpeg_quality(0, 50);
webcam->mEncodeDevice->get_mjpeg_quality(0,&quality);
ERROR("MJPEG Quality:%d",quality);

#endif

  while (1){
    sleep(1);
  }

  return 0;
}
