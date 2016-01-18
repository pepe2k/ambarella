/*******************************************************************************
 * AmEvent.h
 *
 * Histroy:
 *  2012-8-9 2012 - [Zhikan Yang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMEVENT_FRAMEWORK_H_
#define AMEVENT_FRAMEWORK_H_

typedef enum{
  MOTION_DECT = 0,
  AUDIO_DECT,
  FACE_DECT,
  ALL_MODULE_NUM,
}MODULE_ID;

typedef struct{
  char *key;
  void *value;
}MODULE_CONFIG;

typedef int (*MODULE_START_FUNC)();
typedef int (*MODULE_STOP_FUNC)();
typedef int (*MODULE_DESTROY_FUNC)();
typedef int (*MODULE_SET_CONFIG_FUNC)(MODULE_CONFIG *);
typedef int (*MODULE_GET_CONFIG_FUNC)(MODULE_CONFIG *);
typedef MODULE_ID (*MODULE_GET_ID_FUNC)();


typedef struct {
  MODULE_START_FUNC module_start_func;
  MODULE_STOP_FUNC module_stop_func;
  MODULE_DESTROY_FUNC module_destroy_func;
  MODULE_SET_CONFIG_FUNC module_set_config_func;
  MODULE_GET_CONFIG_FUNC module_get_config_func;
}MODULE_FUNCS;

class AmEvent
{
  public:
    int register_module(MODULE_ID module_id, const char *module_path);
    bool is_module_registered(MODULE_ID module_id);
    int start_event_monitor(MODULE_ID module_id);
    int stop_event_monitor(MODULE_ID module_id);
    int destroy_event_monitor(MODULE_ID module_id);

    int start_all_event_monitor();
    int stop_all_event_monitor();
    int destroy_all_event_monitor();

    int set_monitor_config(MODULE_ID module_id, MODULE_CONFIG *config);
    int get_monitor_config(MODULE_ID module_id, MODULE_CONFIG *config);

    /* Constructor && Destructor */
  public:
    virtual ~AmEvent();

  public:
    static AmEvent* get_instance(void);

  private:
    AmEvent();
    int init();

  private:
    static AmEvent* this_instance;
};

#endif
