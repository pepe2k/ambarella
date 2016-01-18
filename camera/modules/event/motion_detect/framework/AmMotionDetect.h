/*******************************************************************************
 * AmMotionDetect.h
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

#ifndef _MOTINO_DETECT_H_
#define _MOTINO_DETECT_H_

#include "AmMdAlgo.h"

class AmMotionDetect {
private:
  static u32 seq;

  AmMdAlgo* md_algo[AM_MD_ALGO_NUM];
  void* libhandles[AM_MD_ALGO_NUM];
  get_instance_t get_algo_instance[AM_MD_ALGO_NUM];
  free_instance_t free_algo_instance[AM_MD_ALGO_NUM];

  static AmMotionDetect* md_instance;
  pthread_t tid;
  bool running;
  int stop_flag;

  am_md_event_cb_t md_cb;

private:
  AmMotionDetect(am_md_event_cb_t cb = NULL);
public:
  virtual ~AmMotionDetect();

  static AmMotionDetect* get_instance(am_md_event_cb_t cb = NULL);

  int start(void);
  int stop(void);

  int set_config(std::string key, void* value);
  int get_config(std::string key, void* value);

  friend void* md_loop(void*);

//some class helpers
private:
  inline void assemble_md_msg(am_md_event_msg_t*, int);
  inline int do_config_algo(AmMdAlgo*, am_md_config_algo_t*);

};

#endif //_MOTINO_DETECT_H_

