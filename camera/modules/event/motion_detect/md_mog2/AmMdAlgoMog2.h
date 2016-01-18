/*******************************************************************************
 * AmMdAlgoMog2.h
 *
 * Histroy:
 *  2014-1-6  [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef _MD_MOG2_H_
#define _MD_MOG2_H_

#include "AmMdAlgo.h"

typedef struct {
  uint32_t  T;
} mog2_attr_t;

typedef struct {
  uint8_t  s_left;
  uint8_t  s_right;
  uint8_t  s_low;
  uint8_t  s_high;
} mog2_roi_scale_t;
typedef struct {
  uint32_t  mog2_motion_output_enable;
  uint16_t  sensitivity;
  uint16_t  threshold;
  uint16_t  mog2_thres;
  uint8_t   valid;
  uint8_t   reserved;
} mog2_roi_attr_t;

typedef struct {
  mog2_roi_scale_t  scale[AM_MD_MAX_ROI_NUM];
  mog2_roi_attr_t   attr[AM_MD_MAX_ROI_NUM];
} mog2_roi_info_t;

enum {
  ROI_AREA    = 0x01 << 0,
  ROI_ATTR    = 0x01 << 1,
  MOG2_ATTR   = 0x01 << 2,
  DIMENTION   = 0x01 << 3,
  REFRESH_ALL = ROI_AREA | ROI_ATTR | MOG2_ATTR | DIMENTION,
};

struct YRawFormat;
class AmMdCam;
class AmMdAlgoMog2 : public AmMdAlgo {
private:
  mog2_attr_t attr;
  mog2_roi_info_t mog2_roi;
  mdet_session_t mdet_session;
  uint8_t motion_status[AM_MD_MAX_ROI_NUM];
  AmMdCam* mdCam;
  bool createCam;
  YRawFormat* ybuf;
  int bufType;

public:
  AmMdAlgoMog2(AmMdCam* cam = NULL);
  virtual ~AmMdAlgoMog2();

  int start(void);
  int stop(void);

  am_md_event_msg_t* check_motion(int* msg_num);

  int set_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi);
  int get_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi);
  int set_sensitivity(uint8_t roi_idx, uint32_t sensitivity);
  int get_sensitivity(uint8_t roi_idx, uint32_t* sensitivity);
  int set_threshold(uint8_t roi_idx, uint32_t threshold);
  int get_threshold(uint8_t roi_idx, uint32_t* threshold);

private:
  inline int start_mog2(uint8_t ref = REFRESH_ALL,
                        uint8_t roi = AM_MD_MAX_ROI_NUM);
  inline int stop_mog2(void);
  int get_mdcam(void);
  inline int put_mdcam(void);
  /* return: 0: success; 1: need to restart mog2 */
  inline int get_ybuf(void);
  /*
    @scope: 1:roi_area, 2:roi_attr, 4:mog2_attr, 8:dim
    @roi:0~AM_MD_MAX_ROI_NUM, with AM_MD_MAX_ROI_NUM meaning all rois
    */
  int refresh(uint8_t refresh, uint8_t roi);
  inline int check_dimension(mdet_dimension_t*);
  inline int check_scale(mog2_roi_scale_t*);
  inline void calc_roi_area(mdet_dimension_t*, mog2_roi_scale_t*, mdet_roi_t*);
};

extern "C" AmMdAlgo* get_instance(void);
extern "C" void free_instance(AmMdAlgo*);
#endif //_MD_MOG2_H_
