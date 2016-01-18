/*******************************************************************************
 * motion_detect.h
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

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "am_include.h"
#include "am_utility.h"

#include "am_motion_detect.h"
#include "AmMotionDetect.h"

#ifdef BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR
# define CAMERA_EVENT_PLUGIN_DIR  BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR
#else
# define CAMERA_EVENT_PLUGIN_DIR  "/usr/lib/camera/event"
#endif

#define MD_MSE_PATH CAMERA_EVENT_PLUGIN_DIR "/md_algos/md_mse.so"
#define MD_MOG2_PATH CAMERA_EVENT_PLUGIN_DIR "/md_algos/md_mog2.so"

static inline int get_item(std::string& key);
static inline am_md_algo_t get_algo(void* v);
static int default_cb(am_md_event_msg_t* msg);

#define STR(w) #w
am_md_module_config_map_t am_md_config_map[] = {
  {"algo",        STR(am_md_config_algo_t),         AM_MD_MODULE_CONFIG_ALGO},
  {"roi",         STR(am_md_config_roi_t),          AM_MD_MODULE_CONFIG_ROI},
  {"sensitivity", STR(am_md_config_sensitivity_t),  AM_MD_MODULE_CONFIG_SENSITIVITY},
  {"threshold",   STR(am_md_config_threshold_t),    AM_MD_MODULE_CONFIG_THRESHOLD},
  {"callback",    STR(am_md_config_cb_t),           AM_MD_MODULE_CONFIG_CB},
};

AmMotionDetect* AmMotionDetect::md_instance = NULL;
uint32_t AmMotionDetect::seq = 0;

AmMotionDetect::AmMotionDetect(am_md_event_cb_t cb)
{
  if (!md_instance) {
    for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
      libhandles[i] = NULL;
      md_algo[i] = NULL;
      get_algo_instance[i] = NULL;
      free_algo_instance[i] = NULL;
    }

    const char *md_algo_lib_path[AM_MD_ALGO_NUM] = {
      MD_MSE_PATH,
      MD_MOG2_PATH,
    };

    for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
      if (access(md_algo_lib_path[i], R_OK) == 0) {
        libhandles[i] = dlopen(md_algo_lib_path[i], RTLD_NOW);
        if (libhandles[i] == NULL) {
          ERROR("failed to load algo %d: %s\n", i, dlerror());
        }
        get_algo_instance[i] = (get_instance_t) dlsym(libhandles[i],
                                                   "get_instance");
        free_algo_instance[i] = (free_instance_t) dlsym(libhandles[i],
                                                   "free_instance");
        if (get_algo_instance[i] && free_algo_instance[i]) {
          if ((md_algo[i] = get_algo_instance[i]()) == NULL) {
            ERROR("failed to get algo instance for %d\n", i);
            dlclose(libhandles[i]);
            libhandles[i] = NULL;
          }
        } else {
          ERROR("failed to get algo lib function: %s\n", dlerror());
          dlclose(libhandles[i]);
          libhandles[i] = NULL;
        }
      }
    }

    if (cb != NULL) {
      md_cb = cb;
    }
    else {
      md_cb = default_cb;
    }

    running = false;
    stop_flag = 0;
    tid = -1;

    md_instance = this;
  }
}

AmMotionDetect::~AmMotionDetect()
{
  stop();

  for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
    if (libhandles[i]) {
      free_algo_instance[i](md_algo[i]);
      dlclose(libhandles[i]);
    }
  }
}

AmMotionDetect* AmMotionDetect::get_instance(am_md_event_cb_t cb)
{
  if (!md_instance) {
    md_instance = new AmMotionDetect(cb);
  }
  return md_instance;
}

inline void AmMotionDetect::assemble_md_msg(am_md_event_msg_t* msgs, int msg_num)
{
  for (int i = 0; i < msg_num; i++) {
    msgs[i].seq = seq;
    seq++;
  }
}
//use only one lock, simple
static pthread_mutex_t md_mutex = PTHREAD_MUTEX_INITIALIZER;

void* md_loop(void* arg)
{
  DEBUG("MD::md_loop enter\n");

  AmMotionDetect* instance = (AmMotionDetect* ) arg;
  if (instance == NULL) {
    ERROR("null instance\n");
    return NULL;
  }

  int stop = 0;

  while (stop != 1) {
    int ret = 0;
    int msg_num = 0;
    am_md_event_msg_t* msgs = NULL;

    if (pthread_mutex_trylock(&md_mutex) != 0) {
      usleep(50 * 1000);
      continue;
    }

    stop = instance->stop_flag;
    for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
      if (instance->md_algo[i]) {
        if (instance->md_algo[i]->is_running()) {
          msgs = instance->md_algo[i]->check_motion(&msg_num);
          instance->assemble_md_msg(msgs, msg_num);

          for (int j = 0; j < msg_num; j++) {
            if ((ret = instance->md_cb(&msgs[j])) < 0)
              WARN("MD::callback returned %d\n", ret);
          }
        }

        msgs = NULL;
        msg_num = 0;
      }
    }

    pthread_mutex_unlock(&md_mutex);

    usleep(50 * 1000);
  }

  DEBUG("MD::md_loop exit\n");
  return NULL;
}

int AmMotionDetect::start(void)
{
  int ret = 0;

  pthread_mutex_lock(&md_mutex);

  do {
    if (running) {
      WARN("MD is running\n");
      ret = -1;
      break;
    }

    for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
      if (md_algo[i]) {
        if (!md_algo[i]->is_running()) {
          if (md_algo[i]->start() == 0) {
            NOTICE("MD::start algo %d success\n", i);
          }
          else {
            WARN("MD::start algo %d failed\n", i);
            ret = -1;
            break;
          }
        }
      }
    }

    stop_flag = 0;

    if (pthread_create(&tid, NULL, md_loop, this) != 0) {
      PERROR("failed to create md_loop");
      ret = -1;
      break;
    }

    running = true;

    DEBUG("MD::start md_loop\n");
  } while(0);

  pthread_mutex_unlock(&md_mutex);
  return ret;
}

int AmMotionDetect::stop(void)
{
  int ret = 0;

  pthread_mutex_lock(&md_mutex);

  do {
    if (!running) {
      WARN("MD is not running\n");
      ret = -1;
      break;
    }
    stop_flag = 1;

    //release lock for md_loop
    pthread_mutex_unlock(&md_mutex);

    //this will block untill md_loop exited
    if (pthread_join(tid, NULL) != 0) {
      ERROR("failed to join md_loop, tid = %d\n", tid);
      pthread_mutex_lock(&md_mutex);
      ret = -1;
      break;
    }

    pthread_mutex_lock(&md_mutex);

    for (int i = 0; i < AM_MD_ALGO_NUM; i++) {
      if (md_algo[i] && md_algo[i]->is_running()) {
        md_algo[i]->stop();
      }
    }

    running = false;
    stop_flag = false;

    DEBUG("MD::stop md_loop\n");
  } while (0);

  pthread_mutex_unlock(&md_mutex);
  return ret;
}

inline int AmMotionDetect::do_config_algo(AmMdAlgo* algo, am_md_config_algo_t* config)
{
  switch (config->op) {
  case AM_MD_ALGO_START:
    return algo->start();
  case AM_MD_ALGO_STOP:
    return algo->stop();
  case AM_MD_ALGO_STATUS:
    if (algo->is_running()) {
      config->status = AM_MD_ALGO_RUNNING;
      DEBUG("algo is running\n");
    }
    else {
      config->status = AM_MD_ALGO_STOPPED;
      DEBUG("algo is stopped\n");
    }
    return 0;
  default:
    return -1;
  }
}

int AmMotionDetect::set_config(std::string key, void* value)
{
  if (check_pointer(value) < 0) {
    return -1;
  }

  int item = get_item(key);
  am_md_algo_t a = get_algo(value);
  AmMdAlgo* algo = NULL;

  if (item != AM_MD_MODULE_CONFIG_CB) {
    switch (a) {
    case AM_MD_ALGO_MSE:
      if ((algo = md_algo[0]) == NULL) {
        WARN("mse algo is not loaded, maybe you didn't build it?\n");
        return -1;
      }
      break;
    case AM_MD_ALGO_MOG2:
      if ((algo = md_algo[1]) == NULL) {
        WARN("mog2 algo is not loaded, maybe you didn't build it?\n");
        return -1;
      }
      break;
    default:
      ERROR("unknown algo id=%d\n", a);
      return -1;
    }
  }

  int ret = 0;

  pthread_mutex_lock(&md_mutex);
  switch (item) {
  case AM_MD_MODULE_CONFIG_ALGO:
    {
      if (do_config_algo(algo, (am_md_config_algo_t* ) value) < 0) {
        ret = -1;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_CB:
    {
      am_md_config_cb_t* config_cb = (am_md_config_cb_t* ) value;
      md_cb = config_cb->cb;
      break;
    }
  case AM_MD_MODULE_CONFIG_ROI:
    {
      am_md_config_roi_t* config_roi = (am_md_config_roi_t*) value;
      if (algo->set_roi_info(config_roi->roi_idx, &config_roi->roi_info) < 0) {
        ret = -1;
        break;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_SENSITIVITY:
    {
      am_md_config_sensitivity_t* config_sens =
        (am_md_config_sensitivity_t* ) value;
      if (algo->set_sensitivity(config_sens->roi_idx,
                                config_sens->sensitivity) < 0) {
        ret = -1;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_THRESHOLD:
    {
      am_md_config_threshold_t* config_thres = (am_md_config_threshold_t* ) value;
      if (algo->set_threshold(config_thres->roi_idx,
                              config_thres->threshold) < 0) {
        ret = -1;
      }
      break;
    }
  default:
    {
      ERROR("unknown key: %s\n", key.data());
      ret = -1;
      break;
    }
  }
  pthread_mutex_unlock(&md_mutex);

  if (ret < 0) {
    ERROR("failed to set config for key:%s\n", key.data());
  }
  return ret;
}

int AmMotionDetect::get_config(std::string key, void* value)
{
  if (check_pointer(value) < 0) {
    return -1;
  }

  int item = get_item(key);
  am_md_algo_t a = get_algo(value);
  AmMdAlgo* algo = NULL;

  if (item != AM_MD_MODULE_CONFIG_CB)
    switch (a) {
    case AM_MD_ALGO_MSE:
      if ((algo = md_algo[0]) == NULL) {
        WARN("algo mse is not loaded, maybe you didn't build it?\n");
        return -1;
      }
      break;
    case AM_MD_ALGO_MOG2:
      if ((algo = md_algo[1]) == NULL) {
        WARN("algo mog2 is not loaded, maybe you didn't build it?\n");
        return -1;
      }
      break;
    default:
      ERROR("unknown algo, id=%d\n", a);
      return -1;
    }

  int ret = 0;

  pthread_mutex_lock(&md_mutex);
  switch (item) {
  case AM_MD_MODULE_CONFIG_ALGO:
    {
      if (do_config_algo(algo, (am_md_config_algo_t* ) value) < 0) {
        ret = -1;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_CB:
    {
      am_md_config_cb_t* config_cb = (am_md_config_cb_t* ) value;
      config_cb->cb = md_cb;
      break;
    }
  case AM_MD_MODULE_CONFIG_ROI:
    {
      am_md_config_roi_t* config_roi = (am_md_config_roi_t* ) value;
      if (algo->get_roi_info(config_roi->roi_idx, &config_roi->roi_info) < 0) {
        ret = -1;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_SENSITIVITY:
    {
      am_md_config_sensitivity_t* config_sens = (am_md_config_sensitivity_t* ) value;
      if (algo->get_sensitivity(config_sens->roi_idx,
                                &config_sens->sensitivity) < 0) {
        ret = -1;
      }
      break;
    }
  case AM_MD_MODULE_CONFIG_THRESHOLD:
    {
      am_md_config_threshold_t* config_thres = (am_md_config_threshold_t* ) value;
      if (algo->get_threshold(config_thres->roi_idx,
                              &config_thres->threshold) < 0) {
        ret = -1;
      }
      break;
    }
  default:
    {
      ERROR("unknown key: %s\n", key.data());
      ret = -1;
      break;
    }
  }

  if (ret < 0) {
    ERROR("failed to get config for key:%s\n", key.data());
  }
  pthread_mutex_unlock(&md_mutex);
  return ret;
}

static inline int get_item(std::string& key)
{
  for (int i = 0; i < AM_MD_MODULE_CONFIG_TOTAL_NUM; i++) {
    if (key == am_md_config_map[i].key) {
      return am_md_config_map[i].item;
    }
  }

  return -1;
}

static inline am_md_algo_t get_algo(void* v)
{
  return *((am_md_algo_t* )v);
}

static int default_cb(am_md_event_msg_t* msg)
{
  debug_md_msg_filter(msg, AM_MD_ALGO_MSE | AM_MD_ALGO_MOG2,
                      AM_MD_MAX_ROI_NUM,
                      //AM_MD_EVENT_MOTION_START | AM_MD_EVENT_MOTION_END);
                      AM_MD_EVENT_MOTION_ALL);
  return 0;
}

