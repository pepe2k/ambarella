/*******************************************************************************
 * am_pm_dptz.cpp
 *
 * History:
 *  Nov 28, 2012 2012 - [qianshen] created file
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
#include "am_utility.h"
#include "am_vdevice.h"
#ifdef CONFIG_ARCH_S2
#include "lib_vproc.h"
#endif

AmPrivacyMaskDPTZ::AmPrivacyMaskDPTZ(int iav_hd, VinParameters **vin) :
  pm_buffer_max_num(0),pm_buffer_id(0),pm_color_index(0),g_pm_enable(0),pm_buffer_size(0),
  g_vin_width(0),g_vin_height(0),g_pm_color(0),num_block_x(0),num_block_y(0),mIsPrivacyMaskMapped(false)
{
  mIav = iav_hd;
  mVinParamList = vin;

  if (ioctl(mIav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &G_dptz_I) < 0) {
    ERROR("IAV_IOC_SET_DIGITAL_ZOOM_EX");
  }

  // black
  pm_color[0].y = 16;
  pm_color[0].u = 128;
  pm_color[0].v = 128;
  // red
  pm_color[1].y = 82;
  pm_color[1].u = 90;
  pm_color[1].v = 240;
  // blue
  pm_color[2].y = 41;
  pm_color[2].u = 240;
  pm_color[2].v = 110;
  // green
  pm_color[3].y = 145;
  pm_color[3].u = 54;
  pm_color[3].v = 34;
  // yellow
  pm_color[4].y = 210;
  pm_color[4].u = 16;
  pm_color[4].v = 146;
  // magenta
  pm_color[5].y = 107;
  pm_color[5].u = 202;
  pm_color[5].v = 222;
  // cyan
  pm_color[6].y = 170;
  pm_color[6].u = 166;
  pm_color[6].v = 16;
  // white
  pm_color[7].y = 235;
  pm_color[7].u = 128;
  pm_color[7].v = 128;

  g_pm_head = NULL;
  g_pm_tail = NULL;

  g_dptz_info.source_buffer = BUFFER_2;
  g_dptz_info.zoom_factor = 1;
  g_dptz_info.offset_x = 0;
  g_dptz_info.offset_y = 0;

}

AmPrivacyMaskDPTZ::~AmPrivacyMaskDPTZ()
{
  unmap_pm();
  DEBUG("AmPrivacyMaskDPTZ deleted!");
}

int AmPrivacyMaskDPTZ::find_nearest_block_rect(PRIVACY_MASK_RECT *block_rect,
  PRIVACY_MASK_RECT *input_rect)
{
  int end_x, end_y;
  block_rect->start_x = (input_rect->start_x)/16;
  block_rect->start_y = (input_rect->start_y)/16;
  end_x = round_up((input_rect->start_x + input_rect->width), 16);
  end_y = round_up((input_rect->start_y + input_rect->height), 16);

  block_rect->width = end_x/16 - block_rect->start_x;
  block_rect->height= end_y/16 - block_rect->start_y;
  return 0;
}

inline int AmPrivacyMaskDPTZ::is_masked(int block_x, int block_y,
  PRIVACY_MASK_RECT * block_rect)
{
  return  ((block_x >= block_rect->start_x) &&
    (block_x < block_rect->start_x + block_rect->width) &&
    (block_y >= block_rect->start_y) &&
    (block_y < block_rect->start_y + block_rect->height));
}

#ifdef CONFIG_ARCH_S2
#ifndef NONE_MASK
#define NONE_MASK (0x8)
#endif

int AmPrivacyMaskDPTZ::disable_pm()
{
  if (vproc_exit() < 0) {
    return -1;
  }
  return 0;
}

int AmPrivacyMaskDPTZ::pm_reset_all(RECT* main)
{
  PrivacyMaskNode* pMaskUnit = g_pm_head;
  while (pMaskUnit) {
    pm_param_t pmrect;
    pmrect.rect.height =  round_up(pMaskUnit->pm_data.height*main->height/100, 16);
    pmrect.rect.width = round_up(pMaskUnit->pm_data.width*main->width/100, 16);
    pmrect.rect.x =  round_up(pMaskUnit->pm_data.left*main->width/100, 16);
    pmrect.rect.y = round_up(pMaskUnit->pm_data.top*main->height/100, 16);
    pmrect.id = pMaskUnit->group_index;
    pmrect.enable = 1;
    if (vproc_pm(&pmrect)) {
      return -1;
    }
    pMaskUnit = pMaskUnit->pNext;
  }
  return 0;
}

int AmPrivacyMaskDPTZ::get_pm_param(uint32_t * pm_in)
{
  /*PrivacyMaskNode* pOutput = (PrivacyMaskNode*) pm_in;
  PrivacyMaskNode* pOne = g_pm_head;

  while(pOne) {
    memcpy(pOutput, pOne, sizeof(PrivacyMaskNode));

  }*/
  if(g_pm_tail) {
    *pm_in = g_pm_tail->group_index + 1;
    ERROR("*pm_in1 is %d",*pm_in);
  } else {
    *pm_in = 0;
  }
  return 0;
}
int  AmPrivacyMaskDPTZ::del_one_node(int id) {
  if(g_pm_head->pNext) {
    PrivacyMaskNode* pToDel = g_pm_head;
    PrivacyMaskNode* pToDelPre = g_pm_head;
     if(pToDel->group_index != id) {
           pToDel = pToDel->pNext;
      }
      while(pToDel) {
        if(pToDel->group_index != id) {
             pToDel = pToDel->pNext;
             pToDelPre = pToDelPre->pNext;
        } else {
          break;
        }
      }
      if(pToDel == g_pm_tail) {
        pToDelPre->pNext = NULL;
      } else {
        pToDelPre->pNext = pToDel->pNext;
      }
      free(pToDel->pNext);
      g_pm_tail = pToDelPre;
   } else {
   g_pm_tail = NULL;
   free(g_pm_head);
   g_pm_head = NULL;
   }
  return 0;
}
int AmPrivacyMaskDPTZ::set_pm_param(PRIVACY_MASK * pm_in, RECT* mainbuffer)
{
  if(pm_in->action == PM_ADD_INC) {
    pm_param_t pmrect;
    pmrect.rect.height = round_up(pm_in->height*mainbuffer->height/100, 16);
    pmrect.rect.width = round_up(pm_in->width*mainbuffer->width/100, 16);
    pmrect.rect.x = round_up(pm_in->left*mainbuffer->width/100, 16);
    pmrect.rect.y = round_up(pm_in->top*mainbuffer->height/100, 16);
    pmrect.id = pm_in->id;
    pmrect.enable = 1;
    if(vproc_pm(&pmrect) < 0) {
      return -1;
    } else {
      PrivacyMaskNode * pNewOne = (PrivacyMaskNode *)malloc(sizeof(PrivacyMaskNode));
      if (pNewOne == NULL) {
        return -1;
      }
      pNewOne->pNext = NULL;
      if (g_pm_head == NULL) {
        pNewOne->group_index = 0;
        pNewOne->pm_data.id = pm_in->id;
        pNewOne->pm_data.width = pm_in->width;
        pNewOne->pm_data.height = pm_in->height;
        pNewOne->pm_data.left = pm_in->left;
        pNewOne->pm_data.top = pm_in->top;
        g_pm_head = g_pm_tail = pNewOne;
      } else {
        assert (g_pm_tail->pNext == NULL);
        pNewOne->group_index = g_pm_tail->group_index + 1;
        pNewOne->pm_data.id = pm_in->id;
        pNewOne->pm_data.width = pm_in->width;
        pNewOne->pm_data.height = pm_in->height;
        pNewOne->pm_data.left = pm_in->left;
        pNewOne->pm_data.top = pm_in->top;
        g_pm_tail->pNext = pNewOne;
        g_pm_tail = pNewOne;
      }
    }
  }
  if(pm_in->action == PM_ADD_EXC) {
    pm_param_t pmrect;
    pmrect.id = pm_in->id;
    pmrect.enable = 0;
    if(vproc_pm(&pmrect) < 0) {
      return -1;
    } else {
      del_one_node(pm_in->id);
    }
  }
  if(pm_in->action == PM_REMOVE_ALL) {
    if (vproc_exit() < 0) {
      return -1;
    } else {
      PrivacyMaskNode* pToDel = g_pm_head;
      while (g_pm_head) {
        g_pm_head = pToDel->pNext;
        free(pToDel);
        pToDel = g_pm_head;
      }
      g_pm_tail = NULL;
    }
  }
  return 0;
}

int AmPrivacyMaskDPTZ::fill_pm_mem(PRIVACY_MASK_RECT * rect)
{
  int each_row_bytes, num_rows;
  iav_mctf_filter_strength_t *pm_pixel;
  int i, j, k;
  int row_gap_count;
  PRIVACY_MASK_RECT nearest_block_rect;

  if ((num_block_x == 0) || (num_block_y == 0)) {
    INFO("privacy mask block number error \n");
    return -1;
  }

  if (find_nearest_block_rect(&nearest_block_rect, rect) < 0) {
    INFO("input rect error \n");
    return -1;
  }

  pm_color_index = rect->color;

  //privacy mask dsp mem uses 4 bytes to represent one block,
  //and width needs to be 32 bytes aligned
#ifdef CONFIG_ARCH_S2
  each_row_bytes = pm_buffer_pitch;
#else
  each_row_bytes = round_up((num_block_x * 4), 32);
#endif
  num_rows = num_block_y;
  pm_pixel = (iav_mctf_filter_strength_t *)pm_start_addr[pm_buffer_id];
  row_gap_count = (each_row_bytes - num_block_x*4)/4;

  for(i = 0; i < num_rows; i++) {
    for (j = 0; j < num_block_x; j++) {
      k = is_masked(j, i, &nearest_block_rect);
      pm_pixel->u |= NONE_MASK;
      pm_pixel->v |= NONE_MASK;
      pm_pixel->y |= NONE_MASK;
      switch (rect->action) {
      case PM_ADD_INC:
        pm_pixel->privacy_mask |= k;
        break;
      case PM_ADD_EXC:
        pm_pixel->privacy_mask &= ~k;
        break;
      case PM_REPLACE:
        pm_pixel->privacy_mask = k;
        break;
      case PM_REMOVE_ALL:
        pm_pixel->privacy_mask = 0;
        break;
      default :
        pm_pixel->privacy_mask = 0;
        break;
      }
      pm_pixel++;
    }
    pm_pixel += row_gap_count;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::calc_dptz_I_param(DPTZParam * pDPTZ,
  iav_digital_zoom_ex_t *dptz)
{
  const int MAX_ZOOM_FACTOR_Y = (4 << 16);
  const int CENTER_OFFSET_Y_MARGIN = 4;

  int vin_w, vin_h;
  int x, y, max_x, max_y;
  int max_window_factor, max_zm_factor, zm_factor_x, zm_factor_y;
  int zm_out_width, zm_out_height;

  if (!g_vin_width || !g_vin_height){
    if (parse_vin_mode_resolution((**mVinParamList).video_mode, &g_vin_width, &g_vin_height) < 0)
      return -1;
  }

  vin_w = g_vin_width;
  vin_h = g_vin_height;
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  struct amba_video_info video_info;
  if (ioctl(mIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
    ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    return -1;
  }

  if (pDPTZ->zoom_factor == 0) {
    // restore to 1X center view
    zm_factor_y = (buffers.main_height << 16) / video_info.height;
    zm_factor_x = (buffers.main_width << 16) / video_info.width;
    x = y = 0;
  } else {
    max_window_factor = vin_h * 100 / buffers.main_height;
    max_zm_factor = MAX_ZOOM_IN_MAIN_BUFFER;
    zm_factor_y = (pDPTZ->zoom_factor - 1) * (max_zm_factor - 100) / MAX_ZOOM_STEPS + 100;
    zm_factor_y = max_window_factor * 100 / zm_factor_y;
    zm_out_width = round_up(zm_factor_y * buffers.main_width / 100, 4);
    zm_out_height = round_up(zm_factor_y * buffers.main_height / 100, 4);
    zm_factor_x = (buffers.main_width << 16) / zm_out_width;
    zm_factor_y = (buffers.main_height << 16) / zm_out_height;
    x = ((int)(G_dptz_I.input.x - video_info.width /2 + G_dptz_I.input.width /2)) + pDPTZ->offset_x * zm_out_width / 1000;
    y = ((int)(G_dptz_I.input.y - video_info.height /2 + G_dptz_I.input.height /2)) + pDPTZ->offset_y * zm_out_height / 1000;
    //x,y can not be odd in S2!
    x = x & (~0x01);
    y = y & (~0x01);

    max_x = vin_w / 2 - zm_out_width / 2;
    max_y = vin_h / 2 - zm_out_height / 2;
    if (zm_factor_y >= MAX_ZOOM_FACTOR_Y)
      max_y -= CENTER_OFFSET_Y_MARGIN;
    if (x < -max_x)
      x = -max_x;
    if (x > max_x)
      x = max_x;
    //x <<= 16;
    if (y < -max_y)
      y = -max_y;
    if (y > max_y)
      y = max_y;
    //y <<= 16;
  }

  dptz->input.width = (buffers.main_width << 16)/zm_factor_x;
  dptz->input.height = (buffers.main_height << 16)/zm_factor_y;
  dptz->input.x = x + (video_info.width - dptz->input.width) / 2;
  dptz->input.y = y + (video_info.height - dptz->input.height) / 2;

  DEBUG("DPTZ Type I : input_width 0x%x, input_height 0x%x, input_offset_x 0x%x, input_offset_y 0x%x.\n",
    dptz->input.width, dptz->input.height, dptz->input.x, dptz->input.y);

  return 0;
}

int AmPrivacyMaskDPTZ::get_dptz_org_param(DPTZOrgParam * pDPTZ, DPTZParam * dptz)
{
  int source = 0;
  iav_digital_zoom_ex_t digital_zoom;
  if (pDPTZ == NULL) {
    INFO("mw_get_dptz_param: DPTZ pointer is NULL!\n");
    return -1;
  }
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  struct amba_video_info video_info;
  if (ioctl(mIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
    ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    return -1;
  }

  memset(&digital_zoom, 0, sizeof(digital_zoom));
  if (dptz == 0) {
    source = pDPTZ->source_buffer;
    if (source == BUFFER_1) {
      if (ioctl(mIav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
        ERROR("IAV_IOC_GET_DIGITAL_ZOOM_EX");
        return -1;
      }
    }
  } else {
    if (calc_dptz_I_param(dptz, &digital_zoom) < 0) {
      ERROR("Calculate DPTZ I parameters failed!\n");
      return -1;
    }
  }
  pDPTZ->zoom_factor_x = (buffers.main_width << 16 ) / digital_zoom.input.width;
  pDPTZ->zoom_factor_y = (buffers.main_height << 16 ) / digital_zoom.input.height;
  pDPTZ->offset_x = (digital_zoom.input.x - video_info.width /2 + digital_zoom.input.width /2) << 16;
  pDPTZ->offset_y = (digital_zoom.input.y - video_info.height /2 + digital_zoom.input.height /2) << 16;

  return 0;
}

int AmPrivacyMaskDPTZ::calc_dptz_II_param(DPTZParam * pDPTZ, int source,
  iav_digital_zoom_ex_t * dptz)
{
  int x, y;
  int max_zm_factor, max_window_factor, zm_factor;
  int zm_out_width, zm_out_height;
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  switch (source){
    case BUFFER_2:
      all_buffer_format[source].height = buffers.second_height;
      all_buffer_format[source].width = buffers.second_width;
      break;
    case BUFFER_3:
      all_buffer_format[source].height = buffers.third_height;
      all_buffer_format[source].width = buffers.third_width;
      break;
    case BUFFER_4:
      all_buffer_format[source].height = buffers.fourth_height;
      all_buffer_format[source].width = buffers.fourth_width;
      break;
    default:
      return -1;
  }

  int max_height_factor = (buffers.main_height * 100) / all_buffer_format[source].height;
  int max_width_factor = (buffers.main_width* 100) / all_buffer_format[source].width;

  if (pDPTZ->zoom_factor == 0) {
    // restore to 1X center view
    if (max_height_factor <= max_width_factor) {
      zm_factor = max_height_factor;
      zm_out_width = zm_factor * all_buffer_format[source].width / 100;
      zm_out_height = buffers.main_height;
      x = (buffers.main_width - zm_out_width) / 2;
      y = 0;
    } else {
      zm_factor = max_width_factor;
      zm_out_width = buffers.main_width;
      zm_out_height = zm_factor * all_buffer_format[source].height / 100;
      x = 0;
      y = (buffers.main_height - zm_out_height) / 2;
    }
  } else {
    max_window_factor = max_height_factor > max_width_factor ? max_width_factor:max_height_factor;
    if (source == BUFFER_2) {
      max_zm_factor = max_window_factor;
    } else {
      max_zm_factor = MAX_ZOOM_IN_PREVIEW_A_B;
    }
    zm_factor = (pDPTZ->zoom_factor - 1) * (max_zm_factor - 100) / MAX_ZOOM_STEPS + 100;
    zm_factor = max_window_factor * 100 / zm_factor;
    zm_out_width = zm_factor * all_buffer_format[source].width / 100;
    zm_out_height = zm_factor * all_buffer_format[source].height / 100;
    x = G_dptz[source].input.x + pDPTZ->offset_x * zm_out_width / 1000;
    y = G_dptz[source].input.y + pDPTZ->offset_y * zm_out_height / 1000;
    if (x + zm_out_width > buffers.main_width)
      x = buffers.main_width - zm_out_width;
    x = (x > 0) ? x : 0;
    if (y + zm_out_height > buffers.main_height)
      y = buffers.main_height - zm_out_height;
    y = (y > 0) ? y : 0;
  }

  dptz->source = source;
  dptz->input.width = zm_out_width & (~0x1);
  dptz->input.height = zm_out_height & (~0x3);
  dptz->input.x = x & (~0x1);
  dptz->input.y = y & (~0x3);
  DEBUG("DPTZ Type II : source %d, input window %dx%d, offset %dx%d.\n",
    dptz->source, dptz->input.width, dptz->input.height,
    dptz->input.x, dptz->input.y);

  return 0;
}

int AmPrivacyMaskDPTZ::set_dptz_control_param(DPTZParam *pDPTZ)
{
  int source = pDPTZ->source_buffer;
  iav_digital_zoom_ex_t * dptz = &G_dptz[source];
  if (source == BUFFER_1) {
    if (calc_dptz_I_param(pDPTZ, &G_dptz_I) < 0) {
      ERROR("Calculate DPTZ I parameters failed!\n");
      return -1;
    }
    if (ioctl(mIav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &G_dptz_I) < 0) {
      ERROR("IAV_IOC_SET_DIGITAL_ZOOM_EX");
      return -1;
    }
  } else {
    if (calc_dptz_II_param(pDPTZ, source, dptz) < 0) {
      ERROR("Calculate DPTZ II parameters failed!\n");
      return -1;
    }
    if (ioctl(mIav, IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX");
      return -1;
    }
  }

  return 0;
}

int AmPrivacyMaskDPTZ::mw_get_dptz_param(DPTZParam * pDPTZ)
{
  if (pDPTZ == NULL) {
    INFO("mw_get_dptz_param: DPTZ pointer is NULL!\n");
    return -1;
  }

  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  struct amba_video_info video_info;
  if (ioctl(mIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
    ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    return -1;
  }

  int source = pDPTZ->source_buffer;
  u32 zoom_factor;
  if (source == BUFFER_1) {
    iav_digital_zoom_ex_t * dptz = &G_dptz_I;
    if (ioctl(mIav, IAV_IOC_GET_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_GET_DIGITAL_ZOOM_EX");
      return -1;
    }
    zoom_factor = (g_vin_height * ((buffers.main_width << 16 ) / dptz->input.width))
      / buffers.main_height * 100 / 65536 + 1;
    convert_dptz_zoom_factor(&pDPTZ->zoom_factor, zoom_factor, source);
    pDPTZ->offset_x = (dptz->input.x - video_info.width /2 + dptz->input.width /2) / 65536;
    pDPTZ->offset_y = (dptz->input.y - video_info.height /2 + dptz->input.height /2) / 65536;
  } else {
    iav_digital_zoom_ex_t * dptz = &G_dptz[source];
    dptz->source = source;
    if (ioctl(mIav, IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX");
      return -1;
    }
    zoom_factor = buffers.main_height * 100 / dptz->input.height;
    convert_dptz_zoom_factor(&pDPTZ->zoom_factor, zoom_factor, source);
    pDPTZ->offset_x = dptz->input.x;
    pDPTZ->offset_y = dptz->input.y;
  }

  return 0;
}
int AmPrivacyMaskDPTZ::set_dptz_param(DPTZParam	 *dptz_set)
{
  if (mw_set_dptz_param(dptz_set) < 0) {
    ERROR("set_dptz_param");
    return -1;
  }
  return 0;
}

int AmPrivacyMaskDPTZ::set_dptz_param_mainbuffer(DPTZParam *dptz_set)
{
  if (dptz_set == NULL) {
    INFO("set_dptz_param_mainbuffer pointer is NULL!\n");
    return -1;
  }

  if (calc_dptz_I_param(dptz_set, &G_dptz_I) < 0) {
    ERROR("Calculate DPTZ I parameters failed!\n");
    return -1;
  }
  //ERROR("dptz in set_dptz_param_mainbuffer source:%d w:%d h:%d x:%d y:%d",G_dptz_I.source, G_dptz_I.input.width,G_dptz_I.input.height,G_dptz_I.input.x,G_dptz_I.input.y);
  return vproc_dptz(&G_dptz_I);
}
#else
int AmPrivacyMaskDPTZ::pm_reset_all(int enable_pm, DPTZParam *dptz)
{
  if (map_pm() < 0) {
    ERROR("Map privacy mask failed !\n");
  }

  PRIVACY_MASK pm;

  memset(&pm, 0, sizeof(pm));
  pm.action = PM_REMOVE_ALL;
  if (add_privacy_mask(&pm) < 0) {
    ERROR("Cannot remove privacy mask!\n");
    return -1;
  }
  if (get_transfer_info(dptz) < 0) {
    ERROR("get_transfer_info");
    return -1;
  }

  PrivacyMaskNode* pMaskUnit = g_pm_head;
  while (pMaskUnit) {
    if (trans_cord_vin_to_main(&pm,&(pMaskUnit->pm_data)))
      return -1;
    if (pm.width && pm.height) {
      pm.color = g_pm_color;
      if (add_privacy_mask(&pm) < 0) {
        ERROR("Cannot refresh privacy mask!\n");
        return -1;
      }
    }
    pMaskUnit = pMaskUnit->pNext;
  }
  if (enable_pm) {
    if (enable_privacy_mask(1) < 0) {
      ERROR("Draw privacy mask failed!\n");
      return -1;
    }
  }
  return 0;
}
int AmPrivacyMaskDPTZ::fill_pm_mem(PRIVACY_MASK_RECT * rect)
{
  int each_row_bytes, num_rows;
  uint32_t *pm_pixel;
  int i, j, k;
  int row_gap_count;
  PRIVACY_MASK_RECT nearest_block_rect;

  if ((num_block_x == 0) || (num_block_y == 0)) {
    INFO("privacy mask block number error \n");
    return -1;
  }

  if (find_nearest_block_rect(&nearest_block_rect, rect) < 0) {
    INFO("input rect error \n");
    return -1;
  }

  pm_color_index = rect->color;

  //privacy mask dsp mem uses 4 bytes to represent one block,
  //and width needs to be 32 bytes aligned
  each_row_bytes = round_up((num_block_x * 4), 32);
  num_rows = num_block_y;
  pm_pixel = (uint32_t *)pm_start_addr[pm_buffer_id];
  row_gap_count = (each_row_bytes - num_block_x*4)/4;

  for(i = 0; i < num_rows; i++) {
    for (j = 0; j < num_block_x; j++) {
      k = is_masked(j, i, &nearest_block_rect);
      switch (rect->action) {
      case PM_ADD_INC:
        *pm_pixel |= k;
        break;
      case PM_ADD_EXC:
        *pm_pixel &= ~k;
        break;
      case PM_REPLACE:
        *pm_pixel = k;
        break;
      case PM_REMOVE_ALL:
        *pm_pixel = 0;
        break;
      default :
        *pm_pixel = 0;
        break;
      }
      pm_pixel++;
    }
    pm_pixel += row_gap_count;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::calc_dptz_I_param(DPTZParam * pDPTZ,
  iav_digital_zoom_ex_t *dptz)
{
  const int MAX_ZOOM_FACTOR_Y = (4 << 16);
  const int CENTER_OFFSET_Y_MARGIN = 4;

  int vin_w, vin_h;
  int x, y, max_x, max_y;
  int max_window_factor, max_zm_factor, zm_factor_x, zm_factor_y;
  int zm_out_width, zm_out_height;

  if (!g_vin_width || !g_vin_height){
    if (parse_vin_mode_resolution((**mVinParamList).video_mode, &g_vin_width, &g_vin_height) < 0)
      return -1;
  }

  vin_w = g_vin_width;
  vin_h = g_vin_height;
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }

  if (pDPTZ->zoom_factor == 0) {
    // restore to 1X center view
    zm_factor_y = (buffers.main_height << 16) / round_up(vin_h-3,4);
    zm_factor_x = zm_factor_y;
    x = y = 0;
  } else {
    max_window_factor = vin_h * 100 / buffers.main_height;
    max_zm_factor = MAX_ZOOM_IN_MAIN_BUFFER;
    zm_factor_y = (pDPTZ->zoom_factor - 1) * (max_zm_factor - 100) / MAX_ZOOM_STEPS + 100;
    zm_factor_y = max_window_factor * 100 / zm_factor_y;
    zm_out_width = round_up(zm_factor_y * buffers.main_width / 100, 4);
    zm_out_height = round_up(zm_factor_y * buffers.main_height / 100, 4);
    zm_factor_x = (buffers.main_width << 16) / zm_out_width;
    zm_factor_y = (buffers.main_height << 16) / zm_out_height;
    x = ((int)G_dptz_I.center_offset_x / 65536) + pDPTZ->offset_x * zm_out_width / 1000;
    y = ((int)G_dptz_I.center_offset_y / 65536) + pDPTZ->offset_y * zm_out_height / 1000;
    max_x = vin_w / 2 - zm_out_width / 2;
    max_y = vin_h / 2 - zm_out_height / 2;
    if (zm_factor_y >= MAX_ZOOM_FACTOR_Y)
      max_y -= CENTER_OFFSET_Y_MARGIN;
    if (x < -max_x)
      x = -max_x;
    if (x > max_x)
      x = max_x;
    x <<= 16;
    if (y < -max_y)
      y = -max_y;
    if (y > max_y)
      y = max_y;
    y <<= 16;
  }

  dptz->zoom_factor_x = zm_factor_x;
  dptz->zoom_factor_y = zm_factor_y;
  dptz->center_offset_x = x;
  dptz->center_offset_y = y;

  DEBUG("DPTZ Type I : zoom_factor_x 0x%x, zoom_factor_y 0x%x, offset_x 0x%x, offset_y 0x%x.\n",
    dptz->zoom_factor_x, dptz->zoom_factor_y, dptz->center_offset_x, dptz->center_offset_y);

  return 0;
}

int AmPrivacyMaskDPTZ::get_dptz_org_param(DPTZOrgParam * pDPTZ, DPTZParam * dptz)
{
  int source = 0;
  iav_digital_zoom_ex_t digital_zoom;
  if (pDPTZ == NULL) {
    INFO("mw_get_dptz_param: DPTZ pointer is NULL!\n");
    return -1;
  }

  memset(&digital_zoom, 0, sizeof(digital_zoom));
  if (dptz == 0) {
    source = pDPTZ->source_buffer;
    if (source == BUFFER_1) {
      if (ioctl(mIav, IAV_IOC_GET_DIGITAL_ZOOM_EX, &digital_zoom) < 0) {
        ERROR("IAV_IOC_GET_DIGITAL_ZOOM_EX");
        return -1;
      }
    }
  } else {
    if (calc_dptz_I_param(dptz, &digital_zoom) < 0) {
      ERROR("Calculate DPTZ I parameters failed!\n");
      return -1;
    }
  }
  pDPTZ->zoom_factor_x = digital_zoom.zoom_factor_x;
  pDPTZ->zoom_factor_y = digital_zoom.zoom_factor_y;
  pDPTZ->offset_x = digital_zoom.center_offset_x;
  pDPTZ->offset_y = digital_zoom.center_offset_y;

  return 0;
}


int AmPrivacyMaskDPTZ::calc_dptz_II_param(DPTZParam * pDPTZ, int source,
  iav_digital_zoom2_ex_t * dptz)
{
  int x, y;
  int max_zm_factor, max_window_factor, zm_factor;
  int zm_out_width, zm_out_height;
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  switch (source){
    case BUFFER_2:
      all_buffer_format[source].height = buffers.second_height;
      all_buffer_format[source].width = buffers.second_width;
      break;
    case BUFFER_3:
      all_buffer_format[source].height = buffers.third_height;
      all_buffer_format[source].width = buffers.third_width;
      break;
    case BUFFER_4:
      all_buffer_format[source].height = buffers.fourth_height;
      all_buffer_format[source].width = buffers.fourth_width;
      break;
    default:
      return -1;
  }

  int max_height_factor = (buffers.main_height * 100) / all_buffer_format[source].height;
  int max_width_factor = (buffers.main_width* 100) / all_buffer_format[source].width;

  if (pDPTZ->zoom_factor == 0) {
    // restore to 1X center view
    if (max_height_factor <= max_width_factor) {
      zm_factor = max_height_factor;
      zm_out_width = zm_factor * all_buffer_format[source].width / 100;
      zm_out_height = buffers.main_height;
      x = (buffers.main_width - zm_out_width) / 2;
      y = 0;
    } else {
      zm_factor = max_width_factor;
      zm_out_width = buffers.main_width;
      zm_out_height = zm_factor * all_buffer_format[source].height / 100;
      x = 0;
      y = (buffers.main_height - zm_out_height) / 2;
    }
  } else {
    max_window_factor = max_height_factor > max_width_factor ? max_width_factor:max_height_factor;
    if (source == BUFFER_2) {
      max_zm_factor = max_window_factor;
    } else {
      max_zm_factor = MAX_ZOOM_IN_PREVIEW_A_B;
    }
    zm_factor = (pDPTZ->zoom_factor - 1) * (max_zm_factor - 100) / MAX_ZOOM_STEPS + 100;
    zm_factor = max_window_factor * 100 / zm_factor;
    zm_out_width = zm_factor * all_buffer_format[source].width / 100;
    zm_out_height = zm_factor * all_buffer_format[source].height / 100;
    x = G_dptz[source].input_offset_x + pDPTZ->offset_x * zm_out_width / 1000;
    y = G_dptz[source].input_offset_y + pDPTZ->offset_y * zm_out_height / 1000;
    if (x + zm_out_width > buffers.main_width)
      x = buffers.main_width - zm_out_width;
    x = (x > 0) ? x : 0;
    if (y + zm_out_height > buffers.main_height)
      y = buffers.main_height - zm_out_height;
    y = (y > 0) ? y : 0;
  }

  dptz->source = source;
  dptz->input_width = zm_out_width & (~0x1);
  dptz->input_height = zm_out_height & (~0x3);
  dptz->input_offset_x = x & (~0x1);
  dptz->input_offset_y = y & (~0x3);
  DEBUG("DPTZ Type II : source %d, input window %dx%d, offset %dx%d.\n",
    dptz->source, dptz->input_width, dptz->input_height,
    dptz->input_offset_x, dptz->input_offset_y);

  return 0;
}

int AmPrivacyMaskDPTZ::set_dptz_control_param(DPTZParam *pDPTZ)
{
  int source = pDPTZ->source_buffer;
  iav_digital_zoom2_ex_t * dptz = &G_dptz[source];
  if (source == BUFFER_1) {
    if (calc_dptz_I_param(pDPTZ, &G_dptz_I) < 0) {
      ERROR("Calculate DPTZ I parameters failed!\n");
      return -1;
    }
    if (ioctl(mIav, IAV_IOC_SET_DIGITAL_ZOOM_EX, &G_dptz_I) < 0) {
      ERROR("IAV_IOC_SET_DIGITAL_ZOOM_EX");
      return -1;
    }
  } else {
    if (calc_dptz_II_param(pDPTZ, source, dptz) < 0) {
      ERROR("Calculate DPTZ II parameters failed!\n");
      return -1;
    }
    if (ioctl(mIav, IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX");
      return -1;
    }
  }

  return 0;
}

int AmPrivacyMaskDPTZ::mw_get_dptz_param(DPTZParam * pDPTZ)
{
  if (pDPTZ == NULL) {
    INFO("mw_get_dptz_param: DPTZ pointer is NULL!\n");
    return -1;
  }

  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }

  int source = pDPTZ->source_buffer;
  u32 zoom_factor;
  if (source == BUFFER_1) {
    iav_digital_zoom_ex_t * dptz = &G_dptz_I;
    if (ioctl(mIav, IAV_IOC_GET_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_GET_DIGITAL_ZOOM_EX");
      return -1;
    }
    zoom_factor = (g_vin_height * dptz->zoom_factor_x)
      / buffers.main_height * 100 / 65536 + 1;
    convert_dptz_zoom_factor(&pDPTZ->zoom_factor, zoom_factor, source);
    pDPTZ->offset_x = dptz->center_offset_x / 65536;
    pDPTZ->offset_y = dptz->center_offset_y / 65536;
  } else {
    iav_digital_zoom2_ex_t * dptz = &G_dptz[source];
    dptz->source = source;
    if (ioctl(mIav, IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX, dptz) < 0) {
      ERROR("IAV_IOC_GET_2ND_DIGITAL_ZOOM_EX");
      return -1;
    }
    zoom_factor = buffers.main_height * 100 / dptz->input_height;
    convert_dptz_zoom_factor(&pDPTZ->zoom_factor, zoom_factor, source);
    pDPTZ->offset_x = dptz->input_offset_x;
    pDPTZ->offset_y = dptz->input_offset_y;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::get_pm_rect(PRIVACY_MASK *pPrivacyMask, PRIVACY_MASK_RECT *rect)
{
  if (pPrivacyMask->unit == PM_UNIT_PERCENT) {
    iav_source_buffer_format_all_ex_t buf_format;

    if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
      ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
      return -1;
    }

    rect->start_x = pPrivacyMask->left * buf_format.main_width / 100;
    rect->start_y = pPrivacyMask->top * buf_format.main_height / 100;
    rect->width = pPrivacyMask->width * buf_format.main_width / 100;
    rect->height = pPrivacyMask->height * buf_format.main_height / 100;
    rect->color = pPrivacyMask->color;
    rect->action = pPrivacyMask->action;
  } else {
    rect->start_x = pPrivacyMask->left;
    rect->start_y = pPrivacyMask->top;
    rect->width = pPrivacyMask->width;
    rect->height = pPrivacyMask->height;
    rect->color = pPrivacyMask->color;
    rect->action = pPrivacyMask->action;
  }
  DEBUG("Rect : offset %dx%d, size %dx%d, color %d, action %d.\n",
    rect->start_x, rect->start_y, rect->width, rect->height,
    rect->color, rect->action);
  return 0;
}

int AmPrivacyMaskDPTZ::add_privacy_mask(PRIVACY_MASK *pPrivacyMask)
{
  static PRIVACY_MASK_RECT pm_rect;

  if (pPrivacyMask == NULL) {
    INFO("[add_privacy_mask] Pointer is NULL !");
    return -1;
  }

  get_pm_rect(pPrivacyMask, &pm_rect);
  if (pPrivacyMask->unit == PM_UNIT_PERCENT) {
    pPrivacyMask->width = pm_rect.width;
    pPrivacyMask->height = pm_rect.height;
    pPrivacyMask->left = pm_rect.start_x;
    pPrivacyMask->top = pm_rect.start_y;
    pPrivacyMask->unit = PM_UNIT_PIXEL;
  }

  if (calc_pm_size() < 0) {
    INFO("Calc privacy mask size failed !\n");
    return -1;
  }
  if (check_for_pm(&pm_rect) < 0) {
    INFO("Check privacy mask parameters error!\n");
    return -1;
  }

  if (fill_pm_mem(&pm_rect) < 0) {
    INFO("Fill privacy mask mem failed !\n");
    return -1;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::trans_cord_vin_to_main(PRIVACY_MASK* main_pm, const PRIVACY_MASK* vin_pm)
{
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }

  int cap_window_width =
    1LL * buffers.main_width * 65536 / g_dptzI_param.zoom_factor_x;
  int cap_window_height =
    1LL * buffers.main_height * 65536 / g_dptzI_param.zoom_factor_y;

  main_pm->width =
    1LL * vin_pm->width * g_dptzI_param.zoom_factor_x / 65536 + 1;

  main_pm->height =
    1LL * vin_pm->height * g_dptzI_param.zoom_factor_y / 65536 + 1;

  main_pm->left = 1LL * (vin_pm->left -
    ((int)g_vin_width /2 - cap_window_width /2) -
     g_dptzI_param.offset_x / 65536) *
     g_dptzI_param.zoom_factor_x  / 65536 + 1;
  if (main_pm->left < 0) {
    main_pm->width += main_pm->left;
    if (main_pm->width < 0)
      main_pm->width = 0;
    main_pm->left = 0;
  } else if (main_pm->left > buffers.main_width) {
    main_pm->left = buffers.main_height;
  }

  if (main_pm->left + main_pm->width > buffers.main_width)
    main_pm->width = buffers.main_width - main_pm->left;

  main_pm->top = 1LL * (vin_pm->top -
    ((int)g_vin_height/2 - cap_window_height /2) -
    g_dptzI_param.offset_y / 65536) *
    g_dptzI_param.zoom_factor_y / 65536 + 1;

  if (main_pm->top <0) {
    main_pm->height += main_pm->top;
    if (main_pm->height < 0)
      main_pm->height = 0;
    main_pm->top = 0;
  } else if (main_pm->top > buffers.main_height ) {
    main_pm->top = buffers.main_height;
  }

  if (main_pm->top + main_pm->height > buffers.main_height)
    main_pm->height = buffers.main_height - main_pm->top;

  main_pm->unit = vin_pm->unit;
  main_pm->color = vin_pm->color;
  main_pm->action = vin_pm->action;

  // adjust privacy mask of translation process from VIN to Main
  adjust_pm_in_main(main_pm);

  INFO("vin->main [vin]: width = %d, height = %d, left = %d, top =%d \n",
    vin_pm->width, vin_pm->height, vin_pm->left, vin_pm->top);
  INFO("vin->main [main]: width = %d, height = %d, left = %d, top =%d \n",
    main_pm->width, main_pm->height, main_pm->left,   main_pm->top);
  return 0;
}

int AmPrivacyMaskDPTZ::trans_cord_main_to_vin(PRIVACY_MASK* vin_pm, const PRIVACY_MASK* main_pm)
{
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }

  int cap_window_width =
    1LL * buffers.main_width * 65536 / g_dptzI_param.zoom_factor_x;
  int cap_window_height =
    1LL * buffers.main_height * 65536 / g_dptzI_param.zoom_factor_y;

  vin_pm->width =
    1LL * main_pm->width  * 65536 / g_dptzI_param.zoom_factor_x;
  vin_pm->height =
    1LL * main_pm->height * 65536 / g_dptzI_param.zoom_factor_y;
  vin_pm->left =
    1LL * main_pm->left * 65536 / g_dptzI_param.zoom_factor_x +
    (int)(g_vin_width /2 - cap_window_width /2 ) +
    g_dptzI_param.offset_x / 65536;

  vin_pm->top =
    1LL * main_pm->top * 65536 / g_dptzI_param.zoom_factor_y +
    (int)(g_vin_height /2 - cap_window_height /2 ) +
    g_dptzI_param.offset_y / 65536;

  vin_pm->unit = main_pm->unit;
  vin_pm->color = main_pm->color;
  vin_pm->action = main_pm->action;

  INFO("main->vin [main]: width = %d, height = %d, left = %d, top =%d \n",
    main_pm->width, main_pm->height, main_pm->left,   main_pm->top);
  INFO("main->vin [vin]: width = %d, height = %d, left = %d, top =%d \n",
    vin_pm->width, vin_pm->height, vin_pm->left, vin_pm->top);
  return 0;
}

int AmPrivacyMaskDPTZ::pm_add_one_mask(PRIVACY_MASK* pm)
{
  PrivacyMaskNode * pNewOne = (PrivacyMaskNode *)malloc(sizeof(PrivacyMaskNode));
  if (pNewOne == NULL)
    return -1;
  if (get_transfer_info(0) < 0) {
    ERROR("get_transfer_info");
    free(pNewOne);
    return -1;
  }
  if (trans_cord_main_to_vin(&(pNewOne->pm_data),pm)) {
    free(pNewOne);
    return -1;
  }
  pNewOne->pNext = NULL;
  if (g_pm_head == NULL) {
    pNewOne->group_index = 0;
    g_pm_head = g_pm_tail = pNewOne;
  } else {
    assert (g_pm_tail->pNext == NULL);
    pNewOne->group_index = g_pm_tail->group_index + 1;
    g_pm_tail->pNext = pNewOne;
    g_pm_tail = pNewOne;
  }
  g_pm_color = pm->color;
  return 0;
}

int AmPrivacyMaskDPTZ::set_pm_param(PRIVACY_MASK * pm_in)
{
  if (map_pm() < 0) {
    ERROR("Map privacy mask failed !\n");
  }

  PRIVACY_MASK pm;
  pm.unit = 0;
  pm.width = pm_in->width;
  pm.height = pm_in->height;
  pm.left = pm_in->left;
  pm.top = pm_in->top;
  pm.action = pm_in->action;
  pm.color = pm_in->color;
  if (add_privacy_mask(&pm) < 0) {
    ERROR("add_privacy_mask");
    return -1;
  } else {
    if (pm_in->action == PM_REMOVE_ALL) {
      if (pm_remove_all_mask() < 0){
        ERROR("pm_remove_all_mask");
        return -1;
      }
      if (enable_privacy_mask(0) < 0) {
        ERROR("Failed to clear privacy mask!\n");
        return -1;
      }
    } else {
      if (pm_add_one_mask(&pm) < 0) {
        ERROR("pm_add_one_mask");
        return -1;
      }
      if (enable_privacy_mask(1) < 0) {
        ERROR("Failed to draw privacy mask!\n");
        return -1;
      }
    }
  }
  g_pm_enable = (pm_in->action != PM_REMOVE_ALL);

  return 0;
}

int AmPrivacyMaskDPTZ::set_dptz_param(DPTZParam	 *dptz_set)
{
  if (map_pm() < 0) {
    ERROR("Map privacy mask failed !\n");
  }

  if ((dptz_set->source_buffer == BUFFER_1) &&
    g_pm_enable) {
    if (pm_reset_all(0, dptz_set) < 0) {
      ERROR("pm_reset_all");
      return -1;
    }
    g_dptz_pm.dptz = *dptz_set;
    if (set_dptz_privacy_mask(&g_dptz_pm) < 0) {
      ERROR("set_dptz_privacy_mask");
      return -1;
    }
  } else {
    if (mw_set_dptz_param(dptz_set) < 0) {
      ERROR("set_dptz_param");
      return -1;
    }
  }
  memcpy(&g_dptz_info, dptz_set, sizeof(g_dptz_info));
  return 0;
}

#endif

int AmPrivacyMaskDPTZ::check_for_pm(PRIVACY_MASK_RECT *input_mask_pixel_rect)
{
  struct amba_video_info video_info;
  iav_state_info_t info;
  iav_source_buffer_format_all_ex_t buf_format;

  if (ioctl(mIav, IAV_IOC_GET_STATE_INFO, &info) < 0) {
    ERROR("IAV_IOC_GET_STATE_INFO");
    return -1;
  }

  if ((info.state != IAV_STATE_PREVIEW) &&
    (info.state != IAV_STATE_ENCODING)) {
    ERROR("privacymask need iav to be in preview or encoding, cur state is %d\n", info.state);
    return -1;
  }

  if (ioctl(mIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
    ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    return -1;
  }
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }
  if ((video_info.format == AMBA_VIDEO_FORMAT_INTERLACE) &&
    (buf_format.main_deintlc_for_intlc_vin != DEINTLC_BOB_MODE)) {
    input_mask_pixel_rect->start_y /= 2;
    input_mask_pixel_rect->height /= 2;
  }
  INFO("start_x : %d, start_y : %d, width : %d, height : %d\n",
    input_mask_pixel_rect->start_x, input_mask_pixel_rect->start_y,
    input_mask_pixel_rect->width, input_mask_pixel_rect->height);

  //check for input mask pixel rect
  if ((input_mask_pixel_rect->start_x + input_mask_pixel_rect->width > num_block_x *16) ||
    (input_mask_pixel_rect->start_y + input_mask_pixel_rect->height > num_block_y *16)) {
    INFO("input mask rect error, start_x (%d) + width (%d) > (%d),"
      " start_y (%d) + height (%d) > (%d)\n",
      input_mask_pixel_rect->start_x, input_mask_pixel_rect->width,
      num_block_x * 16,
      input_mask_pixel_rect->start_y, input_mask_pixel_rect->height,
      num_block_y * 16);
    return -1;
  }

  if (input_mask_pixel_rect->action >= PM_ACTIONS_NUM) {
    INFO("Invalid privacy mask action [%d]. Valid action range is [0~%d].\n",
      input_mask_pixel_rect->action, (PM_ACTIONS_NUM - 1));
    return -1;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::calc_pm_size(void)
{
  iav_source_buffer_format_all_ex_t buf_format;

  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
    return -1;
  }
  num_block_x = round_up(buf_format.main_width, 16) / 16;
  num_block_y = round_up(buf_format.main_height, 16) / 16;

  return 0;
}


int AmPrivacyMaskDPTZ::get_pm_buffer_info(void)
{
  iav_privacy_mask_info_ex_t mask_info;

  // use multiple buffers to add privacy mask
  memset(&mask_info, 0, sizeof(mask_info));
  if (ioctl(mIav, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &mask_info) < 0) {
    ERROR("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
    return -1;
  }
#ifdef CONFIG_ARCH_A5S
  pm_buffer_id = mask_info.next_buffer_id;
  pm_buffer_max_num = mask_info.total_buffer_num;
#endif

#ifdef CONFIG_ARCH_S2
  pm_buffer_pitch = mask_info.buffer_pitch;
#endif
  return 0;
}

int AmPrivacyMaskDPTZ::map_pm(void)
{
  iav_mmap_info_t info;
  int i;

  if (mIsPrivacyMaskMapped)
    return 0;

  memset(&info, 0, sizeof(info));
  if (ioctl(mIav, IAV_IOC_MAP_PRIVACY_MASK_EX, &info) < 0) {
    ERROR("IAV_IOC_MAP_PRIVACY_MASK_EX");
    return -1;
  }
  pm_info.start = info.addr;
  pm_info.size = info.length;
  pm_info.end = info.addr + info.length;
  memset(info.addr, 0, info.length);
  DEBUG("privacy mask : start = %p, total size = 0x%x.\n",
    info.addr, info.length);
  mIsPrivacyMaskMapped = true;

  if (get_pm_buffer_info() < 0) {
    ERROR("Failed to get privacy mask info!");
    return -1;
  }
  pm_buffer_size = info.length / pm_buffer_max_num;
  for (i = 0; i < pm_buffer_max_num; ++i) {
    pm_start_addr[i] = info.addr + i * pm_buffer_size;
  }

  return 0;
}

bool AmPrivacyMaskDPTZ::unmap_pm(void)
{
  if (mIsPrivacyMaskMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_UNMAP_PRIVACY_MASK_EX) < 0)) {
      PERROR("IAV_IOC_UNMAP_PRIVACY_MASK_EX");
    } else {
      mIsPrivacyMaskMapped = false;
    }
  }

  return !mIsPrivacyMaskMapped;
}


int AmPrivacyMaskDPTZ::prepare_pm(int enable, iav_privacy_mask_setup_ex_t *privacy_mask_setup)
{
  privacy_mask_setup->enable = enable;
  privacy_mask_setup->y = pm_color[pm_color_index].y;
  privacy_mask_setup->u = pm_color[pm_color_index].u;
  privacy_mask_setup->v = pm_color[pm_color_index].v;
#ifdef CONFIG_ARCH_S2
  privacy_mask_setup->buffer_pitch = pm_buffer_pitch;
  privacy_mask_setup->buffer_height = num_block_y;
#else
  privacy_mask_setup->block_count_x = num_block_x;
  privacy_mask_setup->block_count_y = num_block_y;
  privacy_mask_setup->buffer_id = pm_buffer_id;
#endif

  return 0;
}

int AmPrivacyMaskDPTZ::copy_pm_to_next_buffer(void)
{
  int current_buffer_id = pm_buffer_id;

  if (get_pm_buffer_info() < 0) {
    ERROR("Failed to get privacy mask info!");
    return -1;
  }

  DEBUG("Current privacy mask buffer id [%d], next buffer id [%d].\n",
    current_buffer_id, pm_buffer_id);
  memcpy(pm_start_addr[pm_buffer_id],
    pm_start_addr[current_buffer_id],
    pm_buffer_size);

  return 0;
}

int AmPrivacyMaskDPTZ::enable_privacy_mask(int enable)
{
  iav_privacy_mask_setup_ex_t privacy_mask_setup;

  memset(&privacy_mask_setup, 0, sizeof(privacy_mask_setup));
  if (get_pm_buffer_info() < 0) {
    ERROR("Failed to get privacy mask info!");
    return -1;
  }

  if (prepare_pm(enable, &privacy_mask_setup) < 0) {
    ERROR("Set privacy mask failed !");
    return -1;
  }
  if (ioctl(mIav, IAV_IOC_SET_PRIVACY_MASK_EX, &privacy_mask_setup) < 0) {
    ERROR("IAV_IOC_SET_PRIVACY_MASK_EX");
    return -1;
  }
  if (copy_pm_to_next_buffer() < 0) {
    ERROR("Copy privacy mask to next buffer!");
    return -1;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::parse_vin_mode_resolution(u32 vin_mode, u32 *vin_width, u32 *vin_height)
{
  uint32_t i;
  for (i = 0;i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));i++){
    if (gVideoModeList[i].ambaMode == vin_mode){
      *vin_width = gVideoModeList[i].width;
      *vin_height = gVideoModeList[i].height;
      if (*vin_width && *vin_height){
        return 0;
      }
      else {
        struct amba_video_info vin_info;
        if (ioctl(mIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info) < 0) {
          ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
          return -1;
        }
        *vin_width = vin_info.width;
        *vin_height = vin_info.height;
        return 0;
      }
    }
  }

  return -1;
}

int AmPrivacyMaskDPTZ::get_transfer_info(DPTZParam *dptz)
{
  if (parse_vin_mode_resolution((**mVinParamList).video_mode, &g_vin_width, &g_vin_height) < 0)
    return -1;

  g_dptzI_param.source_buffer = BUFFER_1;
  if (get_dptz_org_param(&g_dptzI_param, dptz) < 0)
    return -1;

  INFO("vin_width=%d, vin_height = %d, dptz_zoom_x=%d, "
    "dptz_zoom_y= %d,dptz_off_x= %d,dptz_off_y=%d\n",
    g_vin_width, g_vin_height,
    g_dptzI_param.zoom_factor_x, g_dptzI_param.zoom_factor_y,
    g_dptzI_param.offset_x, g_dptzI_param.offset_y);
  return 0;
}

int AmPrivacyMaskDPTZ::adjust_pm_in_main(PRIVACY_MASK *main_pm)
{
  if (main_pm->action == PM_ADD_EXC) {
    main_pm->left = round_up(main_pm->left, 16);
    main_pm->top = round_up(main_pm->top, 16);
    main_pm->width = round_down(main_pm->width, 16);
    main_pm->height = round_down(main_pm->height, 16);
  } else {
    main_pm->left = round_down(main_pm->left, 16);
    main_pm->top = round_down(main_pm->top, 16);
    main_pm->width = round_up(main_pm->width, 16);
    main_pm->height = round_up(main_pm->height, 16);
  }
  return 0;
}






int AmPrivacyMaskDPTZ::pm_remove_all_mask()
{
  PrivacyMaskNode* pToDel = g_pm_head;
  while (g_pm_head) {
    g_pm_head = pToDel->pNext;
    free(pToDel);
    pToDel = g_pm_head;
  }
  g_pm_tail = NULL;
  return 0;
}



//DPTZ start

int AmPrivacyMaskDPTZ::mw_set_dptz_param(DPTZParam * pDPTZ)
{
  if (pDPTZ == NULL) {
    INFO("mw_set_dptz_param: DPTZ pointer is NULL!\n");
    return -1;
  }

  if (set_dptz_control_param(pDPTZ) < 0) {
    ERROR("set_dptz_control_param");
    return -1;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::set_dptz_privacy_mask(DPTZPrivacyMaskParam * pDZPM)
{
  iav_digital_zoom_privacy_mask_ex_t dz_pm;
  memset(&dz_pm, 0, sizeof(dz_pm));

  if (pDZPM == NULL) {
    INFO("mw_set_dptz_privacy_mask pointer is NULL!\n");
    return -1;
  }

  if (calc_dptz_I_param(&pDZPM->dptz, &G_dptz_I) < 0) {
    ERROR("Calculate DPTZ I parameters failed!\n");
    return -1;
  }
  dz_pm.zoom = G_dptz_I;

  if (prepare_pm(1, &dz_pm.privacy_mask) < 0) {
    ERROR("Prepare privacy mask data failed!");
    return -1;
  }
  if (ioctl(mIav, IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX, &dz_pm) < 0) {
    if (errno != EAGAIN) {
      ERROR("IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK_EX");
      return -1;
    }
  }
  if (copy_pm_to_next_buffer() < 0) {
    ERROR("Copy privacy mask to next buffer!");
    return -1;
  }

  return 0;
}

int AmPrivacyMaskDPTZ::convert_dptz_zoom_factor(u32 *new_factor, u32 old_factor, int source)
{
  u32 max_zm_factor;
  iav_source_buffer_format_all_ex_t buffers;
  if (ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buffers) < 0) {
    ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
    return -1;
  }

  switch (source){
    case BUFFER_2:
      all_buffer_format[source].height = buffers.second_height;
      all_buffer_format[source].width = buffers.second_width;
      break;
    case BUFFER_3:
      all_buffer_format[source].height = buffers.third_height;
      all_buffer_format[source].width = buffers.third_width;
      break;
    case BUFFER_4:
      all_buffer_format[source].height = buffers.fourth_height;
      all_buffer_format[source].width = buffers.fourth_width;
      break;
    default:
      return -1;
  }

  if (source == BUFFER_1)
    max_zm_factor = MAX_ZOOM_IN_MAIN_BUFFER;
  else if (source == BUFFER_2)
    max_zm_factor = (buffers.main_height * 100) / all_buffer_format[source].height;
  else
    max_zm_factor = MAX_ZOOM_IN_PREVIEW_A_B;

  *new_factor = (old_factor - 100) * MAX_ZOOM_STEPS / (max_zm_factor - 100) + 1;

  return 0;
}

int AmPrivacyMaskDPTZ::get_dptz_param(uint32_t stream_id, DPTZParam *dptz_get)
{
  if (map_pm() < 0) {
    ERROR("Map privacy mask failed !\n");
  }

  iav_encode_format_ex_t encode_info;
  encode_info.id = 1 << stream_id;

  if (ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_info) < 0) {
    ERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
    return -1;
  }

  dptz_get->source_buffer = encode_info.source;

  if (mw_get_dptz_param(dptz_get) < 0) {
    ERROR("mw_get_dptz_param");
    return -1;
  }

  INFO("    source buffer = %d\n", dptz_get->source_buffer);
  INFO("      zoom factor = %d\n", dptz_get->zoom_factor);
  INFO("         offset x = %d\n", dptz_get->offset_x);
  INFO("         offset y = %d\n", dptz_get->offset_y);
  INFO("------------------------------------------\n");

  return 0;
}

//DPTZ end

