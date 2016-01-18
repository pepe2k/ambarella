/*******************************************************************************
 * AmEvent.cpp
 *
 * Histroy:
 *  2012-8-9 2012 - [zkyang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "am_include.h"
#include "am_utility.h"
#include "amevent.h"

#define PATH_LEN 256

#ifdef BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR
#define CAMERA_EVENT_PLUGIN_DIR  BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR
#else
#define CAMERA_EVENT_PLUGIN_DIR  "/usr/lib/camera/event"
#endif

#define PLUGIN_PATH(s) (CAMERA_EVENT_PLUGIN_DIR s)

static const char *module_files[ALL_MODULE_NUM] = {
  [MOTION_DECT] = NULL,
  [AUDIO_DECT] = NULL,
  [FACE_DECT] = NULL,
};

static void *module_handles[ALL_MODULE_NUM] = {NULL};

static MODULE_FUNCS module_funcs[ALL_MODULE_NUM] = {NULL};

static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Constructor && Destructor */
AmEvent::AmEvent()
{

}

AmEvent::~AmEvent()
{
  int i = 0;
  for (i = 0; i < ALL_MODULE_NUM; i++){
    if (module_files[i]){
      free((void *)module_files[i]);
    }
  }
}

int AmEvent::init()
{
  int ret = 0;
  memset(module_handles, 0, sizeof(void *) * ALL_MODULE_NUM);
  memset(module_funcs, 0, sizeof(MODULE_FUNCS) * ALL_MODULE_NUM);

  //scan plugin modules in CAMERA_EVENT_PLUGIN_DIR
  DIR *module_dir = NULL;
  struct dirent *file = NULL;
  struct stat file_info;
  void *handle = NULL;
  MODULE_GET_ID_FUNC get_id_func = NULL;
  MODULE_ID module_id;
  char *module_path = NULL;

  module_dir = opendir(CAMERA_EVENT_PLUGIN_DIR);
  if (!module_dir){
    ERROR("Can not open dir %s\n", CAMERA_EVENT_PLUGIN_DIR);
    ret = -1;
  }

  while ((file = readdir(module_dir)) != NULL){
    if(strncmp(file->d_name, ".", 1) == 0)
      continue;

    module_path = (char *)calloc(PATH_LEN, 1);
    sprintf(module_path, "%s/%s",CAMERA_EVENT_PLUGIN_DIR, file->d_name);
    if(stat(module_path, &file_info) >= 0 && !S_ISDIR(file_info.st_mode)){
      handle = dlopen(module_path, RTLD_NOW);

      if (handle == NULL){
        ERROR("dlopen: can not open %s\n", file->d_name);
        ERROR("%s\n", dlerror());
        ret = -1;
        free(module_path);
        break;
      }

      get_id_func = (MODULE_GET_ID_FUNC)dlsym(handle, "module_get_id");
      if (get_id_func == NULL){
        ERROR("dlsym: can not load symbol module_get_id");
        ret = -1;
        free(module_path);
        break;
      }

      module_id = get_id_func();
      register_module(module_id, module_path);
      dlclose(handle);
      get_id_func = NULL;
      handle = NULL;
    }
  }

  if (handle)
    dlclose(handle);
  closedir(module_dir);
  return ret;
}

AmEvent * AmEvent::this_instance = NULL;
AmEvent * AmEvent::get_instance(void)
{
  if (this_instance == NULL) {
    this_instance = new AmEvent();
    if (this_instance && (this_instance->init() < 0)) {
      delete this_instance;
      this_instance = NULL;
    }
  }
  return this_instance;
}

int AmEvent::register_module(MODULE_ID module_id, const char *module_path)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);
  module_files[module_id] = module_path;
  pthread_mutex_unlock(&event_mutex);
  return ret;
}

bool AmEvent::is_module_registered(MODULE_ID module_id)
{
  return (module_files[module_id] != NULL);
}

int AmEvent::start_event_monitor(MODULE_ID module_id)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);

  if (module_files[module_id] == NULL){
    ERROR("This module[id:%d] is not registered !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }

  if (!module_handles[module_id]){
    module_handles[module_id] = dlopen(module_files[module_id], RTLD_NOW);
    if (module_handles[module_id] == NULL){
      ERROR("dlerror: %s\n", dlerror());
      ERROR("dlopen:Can not load %s !\n",module_files[module_id]);
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_start
  if (!module_funcs[module_id].module_start_func){
    module_funcs[module_id].module_start_func = (MODULE_START_FUNC)dlsym(module_handles[module_id], "module_start");
    if (module_funcs[module_id].module_start_func == NULL){
      ERROR("dlsym:Can not load symbol module_start !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_stop
  if (!module_funcs[module_id].module_stop_func){
    module_funcs[module_id].module_stop_func = (MODULE_STOP_FUNC)dlsym(module_handles[module_id], "module_stop");
    if (module_funcs[module_id].module_stop_func == NULL){
      ERROR("dlsym:Can not load symbol module_stop !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_destroy
  if (!module_funcs[module_id].module_destroy_func){
    module_funcs[module_id].module_destroy_func = (MODULE_DESTROY_FUNC)dlsym(module_handles[module_id], "module_destroy");
    if (module_funcs[module_id].module_destroy_func == NULL){
      ERROR("dlsym:Can not load symbol module_destroy !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_set_config
  if (!module_funcs[module_id].module_set_config_func){
    module_funcs[module_id].module_set_config_func = (MODULE_SET_CONFIG_FUNC)dlsym(module_handles[module_id], "module_set_config");
    if (module_funcs[module_id].module_set_config_func == NULL){
      ERROR("dlsym:Can not load symbol module_set_config !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_get_config
  if (!module_funcs[module_id].module_get_config_func){
    module_funcs[module_id].module_get_config_func = (MODULE_GET_CONFIG_FUNC)dlsym(module_handles[module_id], "module_get_config");
    if (module_funcs[module_id].module_get_config_func == NULL){
      ERROR("dlsym:Can not load symbol module_get_config !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  ret = module_funcs[module_id].module_start_func();

  pthread_mutex_unlock(&event_mutex);
  return ret;
}

int AmEvent::stop_event_monitor(MODULE_ID module_id)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);

  if (module_funcs[module_id].module_stop_func == NULL){
    ERROR("This module[id:%d] is not started !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }

  ret = module_funcs[module_id].module_stop_func();

  pthread_mutex_unlock(&event_mutex);
  return ret;
}

int AmEvent::destroy_event_monitor(MODULE_ID module_id)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);

  if (module_files[module_id] == NULL){
    ERROR("This module[id:%d] is not registered !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }

  if (module_funcs[module_id].module_destroy_func == NULL){
    ERROR("This module[id:%d] is not started !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }


  ret = module_funcs[module_id].module_destroy_func();

  module_funcs[module_id].module_start_func = NULL;
  module_funcs[module_id].module_stop_func = NULL;
  module_funcs[module_id].module_destroy_func = NULL;
  module_funcs[module_id].module_set_config_func = NULL;
  module_funcs[module_id].module_get_config_func = NULL;
  dlclose(module_handles[module_id]);
  module_handles[module_id] = NULL;

  pthread_mutex_unlock(&event_mutex);
  return ret;

}

int AmEvent::start_all_event_monitor()
{
  int ret = 0;
  int module_id = 0;

  for (module_id = 0; module_id < ALL_MODULE_NUM; module_id++){
    if (module_files[module_id]){
      ret = start_event_monitor((MODULE_ID)module_id);
    }
    if (ret < 0){
      ERROR("Fail to start module[%d]!\n", module_id);
      return ret;
    }
  }

  return ret;
}

int AmEvent::stop_all_event_monitor()
{
  int ret = 0;
  int module_id = 0;

  for (module_id = 0; module_id < ALL_MODULE_NUM; module_id++){
    if (module_files[module_id]){
      ret = stop_event_monitor((MODULE_ID)module_id);
    }
    if (ret < 0){
      ERROR("Fail to stop module[%d]!\n", module_id);
      return ret;
    }
  }

  return ret;
}

int AmEvent::destroy_all_event_monitor()
{
  int ret = 0;
  int module_id = 0;

  for (module_id = 0; module_id < ALL_MODULE_NUM; module_id++){
    if (module_files[module_id]){
      ret = destroy_event_monitor((MODULE_ID)module_id);
    }
    if (ret < 0){
      ERROR("Fail to destroy module[%d]!\n", module_id);
      return ret;
    }
  }

  return ret;
}

int AmEvent::set_monitor_config(MODULE_ID module_id, MODULE_CONFIG *config)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);

  if (module_files[module_id] == NULL){
    ERROR("This module[id:%d] is not registered !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }

  if (!module_handles[module_id]){
    module_handles[module_id] = dlopen(module_files[module_id], RTLD_NOW);
    if (module_handles[module_id] == NULL){
      ERROR("dlerror: %s\n", dlerror());
      ERROR("dlopen:Can not load %s !\n",module_files[module_id]);
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  //register module_set_config
  if (!module_funcs[module_id].module_set_config_func){
    module_funcs[module_id].module_set_config_func = (MODULE_SET_CONFIG_FUNC)dlsym(module_handles[module_id], "module_set_config");
    if (module_funcs[module_id].module_set_config_func == NULL){
      ERROR("dlsym:Can not load symbol module_set_config !\n");
      pthread_mutex_unlock(&event_mutex);
      return -1;
    }
  }

  ret = module_funcs[module_id].module_set_config_func(config);

  pthread_mutex_unlock(&event_mutex);
  return ret;
}

int AmEvent::get_monitor_config(MODULE_ID module_id, MODULE_CONFIG *config)
{
  int ret = 0;
  pthread_mutex_lock(&event_mutex);

  if (module_funcs[module_id].module_get_config_func == NULL){
    ERROR("This module[id:%d] is not started !\n", module_id);
    pthread_mutex_unlock(&event_mutex);
    return -1;
  }

  ret = module_funcs[module_id].module_get_config_func(config);

  pthread_mutex_unlock(&event_mutex);
  return ret;
}

