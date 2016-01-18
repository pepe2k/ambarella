/*******************************************************************************
 * AmMdAlgoMse.h
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

#ifndef _MD_MSE_H_
#define _MD_MSE_H_

#include "AmMdAlgo.h"

typedef struct region_s {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} region_t;

typedef struct roi_s {
    uint32_t mse_diff_output_enable;
    uint32_t luma_mse_threshold;
    region_t area;
    uint16_t tiles[MD_AE_TILE_MAX_ROW][MD_AE_TILE_MAX_COLUMN];
    uint16_t tiles_num;
    uint16_t luma_diff_threshold;
    uint16_t sensitivity;
    uint16_t update_freq;
    uint16_t update_cnt;
    uint8_t  valid;
    uint8_t  reserved;
} roi_t;

typedef struct motion_statistics_s {
    uint32_t pts;
    uint32_t indicator[AM_MD_MAX_ROI_NUM];
} motion_statistics_t;

typedef struct aaa_ae_data_s {
    uint32_t lin_y;
    uint32_t non_lin_y;
} aaa_ae_data_t;


class AmMdAlgoMse : public AmMdAlgo {
private:
  //config
  roi_t roi[AM_MD_MAX_ROI_NUM];
  //input
  aaa_ae_data_t ae_luma_buf[2][MD_AE_TILE_MAX_NUM];
  //output
  int motion_status[AM_MD_MAX_ROI_NUM];
  int buf_id;

public:
  AmMdAlgoMse();
  virtual ~AmMdAlgoMse();

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
  //SysV
  int shmid;
  char* shm;
  struct shmid_ds shmid_ds;
  int start_mse(void);
  int stop_mse(void);
  int mse_read(void*);
};

#define SHM_DATA_SZ (sizeof(aaa_ae_data_t) * MD_AE_TILE_MAX_NUM)

extern "C" AmMdAlgo* get_instance(void);
extern "C" void free_instance(AmMdAlgo*);

#endif //_MD_MSE_H_
