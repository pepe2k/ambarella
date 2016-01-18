/*******************************************************************************
 * test_motion_detect.cpp
 *
 * Histroy:
 *  2014-1-3  [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/


#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <dlfcn.h>

#include "basetypes.h"
#include "am_include.h"
#include "am_utility.h"
#include "am_structure.h"
#include "am_configure.h"
#include "am_motion_detect.h"

#define KEY_LENTH  20
#define MENU_LEN   20
#define MENU_PREFIX     " $$ "
#define CHOICE_PREFIX   " $$ -> "
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define MOTION_DETECT_MSE_CONFIG_PATH \
    BUILD_AMBARELLA_CAMERA_CONF_DIR"/motiondetect_mse.conf"
#define MOTION_DETECT_MOG2_CONFIG_PATH \
    BUILD_AMBARELLA_CAMERA_CONF_DIR"/motiondetect_mog2.conf"


typedef struct {
  char name[MENU_LEN];
  char  flag;
} menu_t;

static menu_t top[] =  {
  {"Start", 's'},
  {"sTop",  't'},
  {"Destroy", 'd'},
  {"Config",  'c'},
  {"Quit",    'q'},
};
static menu_t config[] = {
  {"Algo",  'a'},
  {"Roi",   'r'},
  {"Sensitivity", 's'},
  {"Threshold",   't'},
  {"Display",     'd'},
  {"Up",          'u'},
};

static AmEvent* test_instance;

static void sigstop(int arg)
{
  if (test_instance) {
    test_instance->destroy_event_monitor(MOTION_DECT);
  }
  printf("\n\nStop Motion Detection.\n\n");
  exit(1);
}

static void start_md(void);
static void do_config_algo(void);
static void do_config_roi(void);
static void do_config_sensitivity(void);
static void do_config_threshold(void);
static void do_display_motion(void);
static int  motion_display_cb(am_md_event_msg_t*);
static int  ignore_cb(am_md_event_msg_t*);
static void config_menu(void);
static int load_algo_configs(am_md_algo_t algo);

static void top_menu(void)
{
  char c = 0;

  printf("\n\tMotion Detection\n");
  for (u32 i = 0; i < ARRAY_SIZE(top); i++) {
    printf(MENU_PREFIX "%s\n", top[i].name);
  }
  printf(CHOICE_PREFIX "Your choice:");

  scanf(" %c", &c);
  c = tolower(c);

  switch (c) {
  case 's':
    start_md();
    break;
  case 't':
    if (test_instance->stop_event_monitor(MOTION_DECT) < 0) {
      printf("module stop failed\n");
    }
    break;
  case 'd':
    if (test_instance->destroy_event_monitor(MOTION_DECT) < 0) {
      printf("module destroy failed\n");
    }
    break;
  case 'c':
    config_menu();
    break;
  case 'q':
    exit(0);
  default:
    printf("unknown choice: %c\n", c);
    break;
  }
}

static void config_menu(void)
{
  char c = 0;

  while (1) {
    printf("\n\tConfig\n");
    for (u32 i = 0; i < ARRAY_SIZE(config); i++) {
      printf(MENU_PREFIX "%s\n", config[i].name);
    }
    printf(CHOICE_PREFIX "Your choice:");

    scanf(" %c", &c);
    c = tolower(c);

    switch (c) {
    case 'a':
      do_config_algo();
      break;
    case 'r':
      do_config_roi();
      break;
    case 's':
      do_config_sensitivity();
      break;
    case 't':
      do_config_threshold();
      break;
    case 'd':
      do_display_motion();
      break;
    case 'u':
      return;
    default:
      printf("unkown choice: %c\n", c);
      break;
    }
  }
}

int main(int argc, char** argv)
{
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  test_instance = AmEvent::get_instance();
  if (test_instance == NULL) {
    printf(MENU_PREFIX "failed to get AmEvent instance\n");
    return -1;
  }

  while (1) {
    top_menu();
  }

  test_instance->stop_event_monitor(MOTION_DECT);
  test_instance->destroy_event_monitor(MOTION_DECT);
  return 0;
}


static void start_md(void)
{
  if (test_instance->start_event_monitor(MOTION_DECT) < 0) {
    printf("module start failed\n");
    exit(-1);
  }

  if (access(MOTION_DETECT_MSE_CONFIG_PATH, R_OK) == 0) {
    load_algo_configs(AM_MD_ALGO_MSE);
  }
  if (access(MOTION_DETECT_MOG2_CONFIG_PATH, R_OK) == 0) {
    load_algo_configs(AM_MD_ALGO_MOG2);
  }
}

static void do_config_algo(void)
{
    u32 algo = 0, op = 0;
    am_md_config_algo_t value;
    MODULE_CONFIG config;
    char key[KEY_LENTH] = "algo";

    config.key = key;
    config.value = &value;

    printf("algo(0=mse or 1=mog2), operation(0=start, 1=stop, 2=status)\n");
    printf(CHOICE_PREFIX "Your choice:");

    scanf(" %u , %u", &algo, &op);

    if (algo == 0) {
      algo = AM_MD_ALGO_MSE;
    }
    else if (algo == 1) {
      algo = AM_MD_ALGO_MOG2;
    }
    else {
      printf("invalid algo type\n");
      return;
    }
    value.algo = (am_md_algo_t ) algo;

    if (op == 0) {
      op = AM_MD_ALGO_START;
    }
    else if (op == 1) {
      op = AM_MD_ALGO_STOP;
    }
    else if (op == 2) {
      op = AM_MD_ALGO_STATUS;
    }
    else {
      printf("invalid algo op\n");
      return;
    }
    value.op = (am_md_algo_op_t ) op;

    if (value.op == AM_MD_ALGO_STATUS) {
      test_instance->get_monitor_config(MOTION_DECT, &config);
      printf("algo %s is %s\n",
             (value.algo == AM_MD_ALGO_MSE? "mse":"mog2"),
             (value.status == AM_MD_ALGO_RUNNING? "running":"stopped"));
    }
    else {
      test_instance->set_monitor_config(MOTION_DECT, &config);
    }
}

static inline int choose_algo_idx_dir(u32* algo, u32* roi_idx, u32* dir)
{
  printf("algo(0=mse | 1=mog2), roi_idx(0~%d), direction(0=get | 1=set)\n",
         AM_MD_MAX_ROI_NUM-1);
  printf(CHOICE_PREFIX "Your choice:");
  scanf(" %u , %u , %u", algo, roi_idx, dir);

  if (*algo == 0) {
    *algo = AM_MD_ALGO_MSE;
  }
  else if (*algo == 1) {
    *algo = AM_MD_ALGO_MOG2;
  }
  else {
    printf("invalid algo type %d\n", *algo);
    return -1;
  }
  if (*roi_idx > (AM_MD_MAX_ROI_NUM-1)) {
    printf("invalid roi_idx\n");
    return -1;
  }
  if (*dir != 0 && *dir != 1) {
    printf("invalid direction\n");
    return -1;
  }
  return 0;
}

static void do_config_roi(void)
{
  u32 algo = 0, roi_idx = 0, dir = 0;
  MODULE_CONFIG config;
  char key[KEY_LENTH] = "roi";
  am_md_config_roi_t value;

  config.key = key;
  config.value = &value;
  memset(&value, 0, sizeof(am_md_config_roi_t));

  if (choose_algo_idx_dir(&algo, &roi_idx, &dir) < 0) {
    return;
  }
  value.algo = (am_md_algo_t) algo;
  value.roi_idx = roi_idx;

  am_md_roi_info_t* roi_info = &value.roi_info;

  if (dir == 1) {
    if (algo == AM_MD_ALGO_MSE) {
      printf("left(0~%d), right(0~%d), top(0~%d), bottom(0~%d), "
             "threshold(0~%d), sensitivity(0~%d), valid(0|1), "
             "mse_diff_output_enable(0|1)\n",
           (MD_AE_TILE_MAX_COLUMN-1), (MD_AE_TILE_MAX_COLUMN-1),
           (MD_AE_TILE_MAX_ROW-1),    (MD_AE_TILE_MAX_ROW-1),
           AM_MD_MAX_THRESHOLD, AM_MD_MAX_SENSITIVITY);
    }
    else {
      printf("left(0~%d), right(0~%d), top(0~%d), bottom(0~%d), "
             "threshold(0~%d), sensitivity(0~%d), valid(0|1), "
             "motion_output_enable(0|1)\n",
           MD_MOG2_MAX_SCALE-1, MD_MOG2_MAX_SCALE-1, MD_MOG2_MAX_SCALE-1,
           MD_MOG2_MAX_SCALE-1, AM_MD_MAX_THRESHOLD, AM_MD_MAX_SENSITIVITY);
    }

    printf(CHOICE_PREFIX "Your choice:");

    scanf(" %u , %u , %u , %u , %u , %u , %u ,", &roi_info->left, &roi_info->right,
          &roi_info->top, &roi_info->bottom, &roi_info->threshold, &roi_info->sensitivity,
          &roi_info->valid);
    if (algo == AM_MD_ALGO_MSE) {
      scanf(" %u", &roi_info->mse_cfg.mse_diff_output_enable);
    }
    else {
      scanf(" %u", &roi_info->mog2_cfg.mog2_motion_output_enable);
    }

    test_instance->set_monitor_config(MOTION_DECT, &config);
  }
  else {
    test_instance->get_monitor_config(MOTION_DECT, &config);
    printf("get_config: left %d right %d top %d bottom %d sensitivity %d "
           "threshold %d valid %d ",
           roi_info->left, roi_info->right, roi_info->top, roi_info->bottom,
           roi_info->sensitivity, roi_info->threshold, roi_info->valid);

    if (algo == AM_MD_ALGO_MSE) {
      printf("mse_diff_output_enable %d\n",
             roi_info->mse_cfg.mse_diff_output_enable);
    }
    else {
      printf("mog2_motion_output_enable %d\n",
             roi_info->mog2_cfg.mog2_motion_output_enable);
    }
  }
}

static void do_config_sensitivity(void)
{
  u32 algo = 0, roi_idx = 0, dir = 0;
  MODULE_CONFIG config;
  char key[KEY_LENTH] = "sensitivity";
  am_md_config_sensitivity_t value;

  config.key = key;
  config.value = &value;

  memset(&value, 0, sizeof(am_md_config_sensitivity_t));

  if (choose_algo_idx_dir(&algo, &roi_idx, &dir) < 0) {
    return;
  }

  value.algo = (am_md_algo_t) algo;
  value.roi_idx = roi_idx;

  if (dir == 1) {
    printf("sensitivity(0~%d)\n", AM_MD_MAX_SENSITIVITY);
    printf(CHOICE_PREFIX "Your choice:");
    scanf(" %u", &value.sensitivity);
    test_instance->set_monitor_config(MOTION_DECT, &config);
  }
  else {
    test_instance->get_monitor_config(MOTION_DECT, &config);
    printf("get_config: sensitivity %d\n", value.sensitivity);
  }
}

static void do_config_threshold(void)
{
  u32 algo = 0, roi_idx = 0, dir = 0;
  MODULE_CONFIG config;
  char key[KEY_LENTH] = "threshold";
  am_md_config_threshold_t value;

  config.key = key;
  config.value = &value;

  memset(&value, 0, sizeof(am_md_config_threshold_t));

  if (choose_algo_idx_dir(&algo, &roi_idx, &dir) < 0) {
    return;
  }
  value.algo = (am_md_algo_t) algo;
  value.roi_idx = roi_idx;

  if (dir == 1) {
    printf("threshold(0~%d)\n", AM_MD_MAX_THRESHOLD);
    printf(CHOICE_PREFIX "Your choice:");
    scanf(" %u", &value.threshold);
    test_instance->set_monitor_config(MOTION_DECT, &config);
  }
  else {
    test_instance->get_monitor_config(MOTION_DECT, &config);
    printf("get_config: threshold %d\n", value.threshold);
  }
}

static char mog2_motion_str[512] =
"          |          |          |          |          |          |\n"
"          |   MOG2   |          |          |          |          |\n"
"           __________ __________ __________ __________ __________\n";

static char mse_motion_str[512] =
"          |          |          |          |          |          |\n"
"          |   MSE    |          |          |          |          |\n"
"           __________ __________ __________ __________ __________\n";

#define SAVE_CURSOR_POS "\e[s"
#define REST_CURSOR_POS "\e[u"
#define CURSOR_UP       "\e[%dA"
#define CURSOR_DOWN     "\e[%dB"
#define CURSOR_FW       "\e[%dC"
#define CURSOR_BW       "\e[%dD"
static int mog2_cursor_up_cnts = 5;
static int mse_cursor_up_cnts  = 2;
static int roi_cursor_fw_cnts[AM_MD_MAX_ROI_NUM];

static void do_display_motion(void)
{
  MODULE_CONFIG config;
  char key[KEY_LENTH] = "callback";
  am_md_config_cb_t value;

  int prefix_len = sizeof("          |    XXX   |") - 1;
  int block_len  = sizeof("          |") - 1;

  for (int i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    roi_cursor_fw_cnts[i] = prefix_len + i*block_len;
  }

  fprintf(stdout, "Press s to stop motion display\n");

  fprintf(stdout,
   "\n"
   "           __________ __________ __________ __________ __________\n"
   "          |          |          |          |          |          |\n"
   "          |          |  ROI #0  |  ROI #1  |  ROI #2  |  ROI #3  |\n"
   "           __________ __________ __________ __________ __________\n");
  fprintf(stdout, "%s", mog2_motion_str);
  fprintf(stdout, "%s", mse_motion_str);

  config.key = key;
  config.value = &value;

  value.cb = motion_display_cb;

  test_instance->set_monitor_config(MOTION_DECT, &config);

  while (getc(stdin) != 's')
  {
    sleep(1);
  }

  value.cb = ignore_cb;
  test_instance->set_monitor_config(MOTION_DECT, &config);
}

static int motion_display_cb(am_md_event_msg_t* msg)
{
  char motion_str[20];
  int cursor_up = 0;
  int cursor_fw = 0;

  if (msg) {
    if (msg->roi_idx >= AM_MD_MAX_ROI_NUM) {
      fprintf(stderr, "Invalid roi_idx %d\n", msg->roi_idx);
      return -1;
    }

    cursor_up = (msg->algo_type ==
                 AM_MD_ALGO_MOG2 ? mog2_cursor_up_cnts : mse_cursor_up_cnts);
    cursor_fw = roi_cursor_fw_cnts[msg->roi_idx];
    switch (msg->motion_type) {
    case AM_MD_EVENT_MOTION_START:
      strncpy(motion_str, "  START   ", 20);
      break;
    case AM_MD_EVENT_MOTION_END:
      strncpy(motion_str,  "   END    ", 20);
      break;
    default:
      return 0;
    }

    fprintf(stdout, SAVE_CURSOR_POS);
    fprintf(stdout, CURSOR_UP, cursor_up);
    fprintf(stdout, CURSOR_FW, cursor_fw);
    fprintf(stdout, "%s", motion_str);
    fprintf(stdout, REST_CURSOR_POS);
    fflush(stdout);
  }

  return 0;
}

static int ignore_cb(am_md_event_msg_t* msg)
{
  return 0;
}

static int load_algo_configs(am_md_algo_t algo)
{

  MODULE_CONFIG module_config;
  char key[20];
  am_md_config_roi_t md_roi_config;

  MotionDetectParameters *mdConfigs = NULL;
  AmConfig *config = NULL;

  do {
    config = new AmConfig();
    if (AM_UNLIKELY(!config)) {
      ERROR("Failed to create AmConfig object!");
      break;
    }

    if (algo == AM_MD_ALGO_MSE) {
      config->set_motiondetect_config_path(MOTION_DETECT_MSE_CONFIG_PATH);
    } else if (algo == AM_MD_ALGO_MOG2) {
      config->set_motiondetect_config_path(MOTION_DETECT_MOG2_CONFIG_PATH);
    } else {
      break;
    }

    if (AM_UNLIKELY(!config->load_motiondetect_config())) {
      ERROR("Failed to load motion detect configurations!");
      break;
    } else {
      if (AM_UNLIKELY(NULL == (mdConfigs = config->motiondetect_config()))) {
        ERROR("Failed to get motion detect configurations!");
        break;
      }
    }
    memset(&md_roi_config, 0, sizeof(md_roi_config));
    memset(key, 0, sizeof(key));
    strncpy(key, "roi", sizeof(key));
    module_config.key   = key;
    module_config.value = &md_roi_config;

    if (strcmp(mdConfigs->algorithm, "MOG2") == 0) {
      md_roi_config.algo = AM_MD_ALGO_MOG2;
    } else {
      md_roi_config.algo = AM_MD_ALGO_MSE;
    }
    for (int j = 0; j < MAX_MOTION_DETECT_ROI_NUM; j++) {
      md_roi_config.roi_idx = j;
      md_roi_config.roi_info.left        = mdConfigs->value[j][MD_ROI_LEFT];
      md_roi_config.roi_info.right       = mdConfigs->value[j][MD_ROI_RIGHT];
      md_roi_config.roi_info.top         = mdConfigs->value[j][MD_ROI_TOP];
      md_roi_config.roi_info.bottom      = mdConfigs->value[j][MD_ROI_BOTTOM];
      md_roi_config.roi_info.threshold   = mdConfigs->value[j][MD_ROI_THRESHOLD];
      md_roi_config.roi_info.sensitivity = mdConfigs->value[j][MD_ROI_SENSITIVITY];
      md_roi_config.roi_info.valid       = mdConfigs->value[j][MD_ROI_VALID];
      if (md_roi_config.algo == AM_MD_ALGO_MOG2) {
        md_roi_config.roi_info.mog2_cfg.mog2_motion_output_enable =
            mdConfigs->value[j][MD_ROI_OUTPUT_MOTION];
      } else {
        md_roi_config.roi_info.mse_cfg.mse_diff_output_enable =
            mdConfigs->value[j][MD_ROI_OUTPUT_MOTION];
      }
      if (test_instance->set_monitor_config(MOTION_DECT,
                                               &module_config) < 0) {
            ERROR("Failed to set roi config for motion detecti!");
            break;
      }
    }

  } while (0);

  if (config) {
    delete config;
  }

  return 0;
}
