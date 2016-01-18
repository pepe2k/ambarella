/*******************************************************************************
 * am_motion_detect.h
 *
 * Histroy:
 *  2014-1-2  [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef _AM_MOTION_DETECT_H_
#define _AM_MOTION_DETECT_H_

#include <string>

#define AM_MD_MAX_ROI_NUM     (4)
#define AM_MD_MAX_THRESHOLD   (200)
#define AM_MD_MAX_SENSITIVITY (100)

#define __AM_MD_LOW_OF(max)       (max/10)
#define __AM_MD_MEDIUM_OF(max)    (max/2)
#define __AM_MD_HIGH_OF(max)      (max/1 - 5)

/* mse specific */
#define MD_AE_TILE_MAX_ROW              8
#define MD_AE_TILE_MAX_COLUMN           12
#define MD_AE_TILE_MAX_NUM   (MD_AE_TILE_MAX_ROW * MD_AE_TILE_MAX_COLUMN)

/* mog2 specific */
#define MD_MOG2_MAX_SCALE               15

enum {
  AM_MD_SENSITIVITY_LOW     = __AM_MD_LOW_OF(AM_MD_MAX_SENSITIVITY),
  AM_MD_SENSITIVITY_MEDIUM  = __AM_MD_MEDIUM_OF(AM_MD_MAX_SENSITIVITY),
  AM_MD_SENSITIVITY_HIGH    = __AM_MD_HIGH_OF(AM_MD_MAX_SENSITIVITY),
};

enum {
  AM_MD_THRESHOLD_LOW       = __AM_MD_LOW_OF(AM_MD_MAX_THRESHOLD),
  AM_MD_THRESHOLD_MEDIUM    = __AM_MD_MEDIUM_OF(AM_MD_MAX_THRESHOLD),
  AM_MD_THRESHOLD_HIGH      = __AM_MD_HIGH_OF(AM_MD_MAX_THRESHOLD),
};

typedef enum {
  AM_MD_EVENT_NO_MOTION     = (0x01 << 0),
  AM_MD_EVENT_MOTION_START  = (0x01 << 1),
  AM_MD_EVENT_MOTION_END    = (0x01 << 2),
  AM_MD_EVENT_MOTION_DIFF   = (0x01 << 3),
  AM_MD_EVENT_MOTION_ALL    = (AM_MD_EVENT_MOTION_START |
                               AM_MD_EVENT_MOTION_END   |
                               AM_MD_EVENT_MOTION_DIFF),
} am_md_event_t;

typedef enum {
  AM_MD_ALGO_MSE    = (0x01 << 0),
  AM_MD_ALGO_MOG2   = (0x01 << 1),
  AM_MD_ALGO_HYBRID = AM_MD_ALGO_MSE | AM_MD_ALGO_MOG2,
  AM_MD_ALGO_NUM    = 2,
} am_md_algo_t;

typedef enum {
  AM_MD_ALGO_RUNNING,
  AM_MD_ALGO_STOPPED,
} am_md_algo_status_t;

typedef enum {
  AM_MD_ALGO_START,
  AM_MD_ALGO_STOP,
  AM_MD_ALGO_STATUS,
} am_md_algo_op_t;

typedef struct {
  uint32_t mog2_motion_output_enable;
} am_md_mog2_cfg_t;

typedef struct {
  uint32_t mse_diff_output_enable;
} am_md_mse_cfg_t;

typedef struct {
  uint32_t left;
  uint32_t right;
  uint32_t top;
  uint32_t bottom;
  uint32_t threshold;
  uint32_t sensitivity;
  uint32_t valid;
  union {
    am_md_mse_cfg_t  mse_cfg;
    am_md_mog2_cfg_t mog2_cfg;
  };
} am_md_roi_info_t;

typedef enum {
  //TODO algo specific config
  AM_MD_MODULE_CONFIG_ALGO        = (0x01 << 0),
  AM_MD_MODULE_CONFIG_ROI         = (0x01 << 1),
  AM_MD_MODULE_CONFIG_SENSITIVITY = (0x01 << 2),
  AM_MD_MODULE_CONFIG_THRESHOLD   = (0x01 << 3),
  AM_MD_MODULE_CONFIG_CB          = (0X01 << 4),
  AM_MD_MODULE_CONFIG_TOTAL_NUM   = (        5),
} am_md_module_config_item_t;


typedef struct {
  am_md_algo_t algo_type;
  uint8_t roi_idx;
  uint8_t motion_type;
  union {
    uint32_t diff;
    uint32_t motion;
  };
  uint32_t seq;
  time_t timestamp;
} am_md_event_msg_t;

typedef int (*am_md_event_cb_t)(am_md_event_msg_t*);

typedef struct {
  am_md_algo_t algo;
  am_md_algo_op_t op;
  am_md_algo_status_t status;
} am_md_config_algo_t;
typedef struct {
  am_md_algo_t algo;
  am_md_roi_info_t roi_info;
  uint8_t roi_idx;
  uint8_t reserved[3];
} am_md_config_roi_t;
typedef struct {
  am_md_algo_t algo;
  uint32_t sensitivity;
  uint8_t roi_idx;
  uint8_t reserved[3];
} am_md_config_sensitivity_t;
typedef struct {
  am_md_algo_t algo;
  uint32_t threshold;
  uint8_t roi_idx;
  uint8_t reserved[3];
} am_md_config_threshold_t;
typedef struct {
  am_md_algo_t algo;
  am_md_event_cb_t cb;
} am_md_config_cb_t;

typedef struct {
  std::string key;
  std::string value_type;
  am_md_module_config_item_t item;
} am_md_module_config_map_t;

#ifdef __cplusplus
extern "C" {
#endif

#include "../framework/amevent.h"

#ifndef DLL_PUBLIC
# define DLL_PUBLIC __attribute__ ((visibility ("default")))
#endif
#ifndef DLL_LOCAL
# define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#endif

DLL_PUBLIC int module_start(void);
DLL_PUBLIC int module_stop(void);
DLL_PUBLIC int module_destroy(void);

DLL_PUBLIC int module_set_config(MODULE_CONFIG* config);
DLL_PUBLIC int module_get_config(MODULE_CONFIG* config);
DLL_PUBLIC MODULE_ID module_get_id(void);

#ifdef __cplusplus
}
#endif

#endif //_AM_MOTION_DETECT_H_
