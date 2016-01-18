/*******************************************************************************
 * md_mse.cpp
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


#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <unistd.h>

#include "am_utility.h"

#include "am_motion_detect.h"
#include "AmMdAlgoMse.h"

static inline void debug_mse_roi_info(roi_t*);
static inline void debug_md_roi_mse_cfg(am_md_mse_cfg_t*);
static inline int check_roi_info(am_md_roi_info_t*);

static am_md_roi_info_t initial_roi =
{0, MD_AE_TILE_MAX_COLUMN-1,
 0, MD_AE_TILE_MAX_ROW-1,
 AM_MD_THRESHOLD_LOW,
 AM_MD_SENSITIVITY_HIGH,
 0,
 {0,},
};
//0: MD_NO_MOTION
static int initial_motion_status[AM_MD_MAX_ROI_NUM];

#define MSE_SKIP_CNT 15

AmMdAlgoMse::AmMdAlgoMse(): AmMdAlgo(AM_MD_ALGO_MSE)
{
  memset(ae_luma_buf, 0, 2 * MD_AE_TILE_MAX_NUM * sizeof(aaa_ae_data_t));
  memset(roi, 0, AM_MD_MAX_ROI_NUM * sizeof(roi_t));

  set_roi_info(0, &initial_roi);
  memcpy(motion_status, initial_motion_status, AM_MD_MAX_ROI_NUM * sizeof(int));
  buf_id = 0;

  shmid = 0;
  shm = NULL;
  memset(&shmid_ds, 0, sizeof(struct shmid_ds));

  status = AM_MD_ALGO_STOPPED;
}
AmMdAlgoMse::~AmMdAlgoMse()
{
  if (status == AM_MD_ALGO_RUNNING) {
    stop();
  }
}

int AmMdAlgoMse::start(void)
{
  if (status == AM_MD_ALGO_RUNNING) {
    NOTICE("MSE is already running\n");
    return 0;
  }

  if (start_mse() == 0) {
    status = AM_MD_ALGO_RUNNING;
    NOTICE("MSE is created\n");
    return 0;
  }
  else {
    status = AM_MD_ALGO_STOPPED;
    NOTICE("MSE creation failed\n");
    return -1;
  }
}

int AmMdAlgoMse::stop(void)
{
  if (status == AM_MD_ALGO_STOPPED) {
    NOTICE("MSE is already stopped\n");
    return 0;
  }

  stop_mse();

  status = AM_MD_ALGO_STOPPED;
  NOTICE("MSE is stopped\n");

  return 0;
}

am_md_event_msg_t* AmMdAlgoMse::check_motion(int* msg_num)
{
  if (check_pointer(msg_num) < 0) {
    return NULL;
  }

  static int skip_cnt = MSE_SKIP_CNT;
  if (skip_cnt-- > 0) {
    *msg_num = 0;
    return NULL;
  } else {
    skip_cnt = MSE_SKIP_CNT;
  }

  if (mse_read((void *) &ae_luma_buf[buf_id][0]) < 0) {
    ERROR("failed to read luma data of AE tiles\n");
    *msg_num = 0;
    return NULL;
  }

  buf_id = 1 - buf_id;

  uint32_t diff_tile;
  uint32_t index = 0;
  int32_t diff_y_total;
  int32_t diff_y_avg;
  uint32_t mse_diff_y;

  int32_t diff_y[MD_AE_TILE_MAX_NUM];
  int16_t inc_num = 0;
  int motion_event_num = 0;
  int diff_event_num = 0;

  motion_statistics_t motion_stat;

  int i, j, k;

  int motion_event[AM_MD_MAX_ROI_NUM];
  int diff_event[AM_MD_MAX_ROI_NUM];

  memset(&motion_stat, 0, sizeof(motion_stat));
  motion_stat.pts = 0;  // reserved
  for (i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    motion_event[i] = AM_MD_EVENT_NO_MOTION;
    diff_event[i] = AM_MD_EVENT_NO_MOTION;
    if (roi[i].valid == 0) {
      //   memcpy(roi[i].ptr_buf[], &ae_cur_y[0], sizeof(roi[i].pre_y));
      continue;
    }

    //roi valid && mse_diff_output_enable, send AM_MD_EVENT_MOTION_DIFF
    if (roi[i].mse_diff_output_enable) {
      diff_event[i] = AM_MD_EVENT_MOTION_DIFF;
      diff_event_num++;
    }

    diff_y_total = 0;
    diff_y_avg = 0;
    mse_diff_y = 0;
    inc_num = 0;

    if (roi[i].update_cnt) {
      roi[i].update_cnt --;
      continue;
    } else {
      roi[i].update_cnt = roi[i].update_freq - 1;
    }

    /* Calculate MSE of luma diff in each ROI */
    for (j = 0; j < MD_AE_TILE_MAX_ROW; j++) {
      for (k = 0; k < MD_AE_TILE_MAX_COLUMN; k++) {
        if (roi[i].tiles[j][k] == 1) {
          index = j * MD_AE_TILE_MAX_COLUMN + k;
          diff_tile = AM_ABS(ae_luma_buf[buf_id][index].lin_y - \
                             ae_luma_buf[1 - buf_id][index].lin_y);
          if (diff_tile >= (ae_luma_buf[1 - buf_id][index].lin_y * \
              roi[i].sensitivity / AM_MD_MAX_SENSITIVITY))
          {
            inc_num++;
          }
          diff_y[index] = diff_tile;
          diff_y_total += diff_tile;
        }
      }
      //DEBUG("\n");
    }

    inc_num = (inc_num > 0 ? inc_num : roi[i].tiles_num);
    diff_y_avg = diff_y_total / roi[i].tiles_num;
    for (j = 0; j < MD_AE_TILE_MAX_ROW; j++) {
      for (k = 0; k < MD_AE_TILE_MAX_COLUMN; k++) {
        if (roi[i].tiles[j][k] == 1) {
          index = j * MD_AE_TILE_MAX_COLUMN + k;
          //DEBUG("%d\t", diff_y[index]);
          if (diff_y[index]) {
            mse_diff_y += ((diff_y[index] - diff_y_avg) *
                           (diff_y[index] - diff_y_avg));
          }
        }
      }
      //DEBUG("\n");
    }
    mse_diff_y /= inc_num;
    mse_diff_y /= inc_num;
    /*
    DEBUG("ROI#%d: diff_y_avg = %d,\tinc = %d,\tmse_diff_y = %d,"
         "\tthreshold = %d\n",
         i, diff_y_avg, inc_num, mse_diff_y, roi[i].luma_mse_threshold);
    */

    if (mse_diff_y > MAX_MOTION_INDICATOR) {
      mse_diff_y = MAX_MOTION_INDICATOR;
    }

    motion_stat.indicator[i] = mse_diff_y;

    if (motion_status[i] == MD_NO_MOTION) {
      if (mse_diff_y > roi[i].luma_mse_threshold) {
        motion_status[i] = MD_IN_MOTION;
        motion_event[i] = AM_MD_EVENT_MOTION_START;
        motion_event_num++;
      }
    } else if (motion_status[i] == MD_IN_MOTION) {
      if (mse_diff_y < roi[i].luma_mse_threshold) {
        motion_status[i] = MD_NO_MOTION;
        motion_event[i] = AM_MD_EVENT_MOTION_END;
        motion_event_num++;
      }
    }
  }

  *msg_num = diff_event_num + motion_event_num;

  am_md_event_msg_t* p = &md_msgs[0];

  for (int i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    if (diff_event[i] == AM_MD_EVENT_MOTION_DIFF) {
      p->algo_type    = AM_MD_ALGO_MSE;
      p->roi_idx      = i;
      p->motion_type  = AM_MD_EVENT_MOTION_DIFF;
      p->diff         = motion_stat.indicator[i];
      p->timestamp    = time(NULL);
      //debug_md_msg(p);
      p++;
    }

    if (motion_event[i] == AM_MD_EVENT_MOTION_START ||
        motion_event[i] == AM_MD_EVENT_MOTION_END) {
      p->algo_type    = AM_MD_ALGO_MSE;
      p->roi_idx      = i;
      p->motion_type  = motion_event[i];
      p->diff         = motion_stat.indicator[i];
      p->timestamp    = time(NULL);
      //debug_md_msg(p);
      p++;
    }
  }

  return &md_msgs[0];
}

int AmMdAlgoMse::set_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi_info)
{
  if (check_pointer(roi_info) < 0) {
    return -1;
  }

  uint32_t i, j;
  int threshold;
  int width, height;

  debug_md_roi_info(roi_info);
  debug_md_roi_mse_cfg(&roi_info->mse_cfg);

  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  if (check_roi_info(roi_info) < 0) {
    return -1;
  }

  width   = roi_info->right - roi_info->left + 1;
  height  = roi_info->bottom - roi_info->top + 1;

  roi[roi_idx].valid = !!roi_info->valid;
  roi[roi_idx].area.x = roi_info->left;
  roi[roi_idx].area.y = roi_info->top;
  roi[roi_idx].area.width = width;
  roi[roi_idx].area.height = height;
  roi[roi_idx].update_freq = 1;
  roi[roi_idx].update_cnt = roi[roi_idx].update_freq - 1;
  roi[roi_idx].tiles_num=0;

  check_threshold(&roi_info->threshold);
  threshold = roi_info->threshold;
  roi[roi_idx].luma_diff_threshold = threshold;
  roi[roi_idx].luma_mse_threshold = threshold * threshold;
  check_sensitivity(&roi_info->sensitivity);
  roi[roi_idx].sensitivity        = roi_info->sensitivity;

  /* Fill the tiles to be detected */
  for (i = roi_info->left; i <= roi_info->right; i++) {
    for (j = roi_info->top; j <= roi_info->bottom; j++) {
      roi[roi_idx].tiles[j][i] = 1;
      roi[roi_idx].tiles_num++;
    }
  }

  roi[roi_idx].mse_diff_output_enable =
    roi_info->mse_cfg.mse_diff_output_enable != 0? 1:0;

  debug_mse_roi_info(&roi[roi_idx]);
  return 0;
}

int AmMdAlgoMse::get_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi_info)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  if (check_pointer(roi_info) < 0) {
    return -1;
  }

  roi_info->left        = roi[roi_idx].area.x;
  roi_info->top         = roi[roi_idx].area.y;
  roi_info->right       = roi[roi_idx].area.x + roi[roi_idx].area.width - 1;
  roi_info->bottom      = roi[roi_idx].area.y + roi[roi_idx].area.height - 1;
  roi_info->sensitivity = roi[roi_idx].sensitivity;
  roi_info->threshold   = roi[roi_idx].luma_diff_threshold;
  roi_info->valid       = roi[roi_idx].valid;

  roi_info->mse_cfg.mse_diff_output_enable = roi[roi_idx].mse_diff_output_enable;

  debug_md_roi_info(roi_info);
  debug_md_roi_mse_cfg(&roi_info->mse_cfg);

  return 0;
}

int AmMdAlgoMse::set_sensitivity(uint8_t roi_idx, uint32_t sensitivity)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  check_sensitivity(&sensitivity);

  roi[roi_idx].sensitivity = sensitivity;
  DEBUG("MSE:set ROI[%d] sensitivity: %u\n", roi_idx, sensitivity);
  return 0;
}

int AmMdAlgoMse::get_sensitivity(uint8_t roi_idx, uint32_t* sensitivity)
{
  if (check_pointer(sensitivity) < 0) {
    return -1;
  }

  if (check_roi_idx(roi_idx)) {
    return -1;
  }

  *sensitivity = roi[roi_idx].sensitivity;
  DEBUG("MSE:get ROI[%d] sensitivity: %u\n", roi_idx, *sensitivity);
  return 0;
}

int AmMdAlgoMse::set_threshold(uint8_t roi_idx, uint32_t threshold)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  check_threshold(&threshold);

  roi[roi_idx].luma_mse_threshold   = threshold * threshold;
  roi[roi_idx].luma_diff_threshold  = threshold;
  DEBUG("MSE:set ROI[%d] threshold %u\n", roi_idx, threshold);

  return 0;
}

int AmMdAlgoMse::get_threshold(uint8_t roi_idx, uint32_t* threshold)
{
  if (check_pointer(threshold) < 0) {
    return -1;
  }

  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  *threshold = roi[roi_idx].luma_diff_threshold;
  DEBUG("MSE:get ROI[%d] threshold %u\n", roi_idx, *threshold);
  return 0;
}

int AmMdAlgoMse::start_mse(void)
{
  while ((shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666)) < 0) {
    NOTICE("shmget:3A is off, wait one second\n");
    //exit(1);
    sleep(1);
  }

  if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*) -1) {
    PERROR("shmat");
    return -1;
  }

  DEBUG("mse shared memory get\n");
  return 0;
}

int AmMdAlgoMse::stop_mse(void)
{
  if (shmdt(shm) < 0) {
    PERROR("shmdt");
    return -1;
  }

  DEBUG("mse shared memory deleted\n");
  return 0;
}

int AmMdAlgoMse::mse_read(void* ae_luma_buf)
{
  int retry_count = 100;
  do {
    if (retry_count == 0) {
      ERROR("can't get shm lock, tried 100 times\n");
      return -1;
    }

    if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
      PERROR("shmctl IPC_STAT");
      continue;
    }

    if (shmid_ds.shm_perm.mode & SHM_DEST) {
      ERROR("\n\n3A is off.\n");
      continue;
    }

    if (shmid_ds.shm_perm.mode & SHM_LOCKED) {
      NOTICE("shared memory is locked, retry in 10ms\n");
      usleep(10*1000);
      continue;
    }

    if (shmctl(shmid, SHM_LOCK, (struct shmid_ds *)NULL) == -1) {
      PERROR("shmctl SHM_LOCK");
      continue;
    }
    //DEBUG("SHM_LOCK, key=%d\n", DEFAULT_KEY);
    break;
  } while (retry_count--);

  //Note: fflush will cause dead lock here
  memcpy(ae_luma_buf, shm, SHM_DATA_SZ);

  if (shmctl(shmid, SHM_UNLOCK, (struct shmid_ds *)NULL) == -1) {
    PERROR("shmctl SHM_UNLOCK");
    return -1;
  }
  //DEBUG("SHM_UNLOCK, key=%d\n", DEFAULT_KEY);

  return 0;
}

static inline void debug_mse_roi_info(roi_t* p)
{
  DEBUG("MD::mse roi: {x %d w %d y %d h %d diff_thres %d mse_thres %d valid\
        %d\n",
        p->area.x, p->area.width, p->area.y, p->area.height,
        p->luma_diff_threshold, p->luma_mse_threshold, p->valid);
}
static inline void debug_md_roi_mse_cfg(am_md_mse_cfg_t* p)
{
  if (AM_LIKELY(p)) {
    DEBUG("MD::mse cfg {mse_diff_output_enable %d}\n",
          p->mse_diff_output_enable);
  }
}
static inline int check_roi_info(am_md_roi_info_t* roi_info)
{
  if (roi_info->left  > roi_info->right           ||
      roi_info->right > (MD_AE_TILE_MAX_COLUMN-1) ||
      roi_info->top   > roi_info->bottom          ||
      roi_info->bottom> (MD_AE_TILE_MAX_ROW-1)) {
    ERROR("ROI(left %d, right %d, top %d, bottom %d) is out of max ROI range\n",
          roi_info->left, roi_info->right, roi_info->top, roi_info->bottom);
    return -1;
  }
  return 0;
}

AmMdAlgo* get_instance(void)
{
  return new AmMdAlgoMse;
}
void free_instance(AmMdAlgo *algo)
{
  if (algo) {
    delete algo;
    algo = NULL;
  }
}
