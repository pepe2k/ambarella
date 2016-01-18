/*******************************************************************************
 * AmMdAlgo.h
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

#ifndef _MD_ALGO_H_
#define _MD_ALGO_H_

#define MD_MSGS_MAX_NUM     AM_MD_MAX_ROI_NUM*2
/* md internals */
class AmMdAlgo {
protected:
  //AM_MD_ALGO_RUNNING/STOPPED
  int status;
  am_md_algo_t type;

public:
  AmMdAlgo(am_md_algo_t algo_type):status(AM_MD_ALGO_STOPPED), type(algo_type)
  {
    memset(md_msgs, 0, sizeof(am_md_event_msg_t) * MD_MSGS_MAX_NUM);
  }
  virtual ~AmMdAlgo()
  {}

  virtual am_md_event_msg_t* check_motion(int* msg_num)     { return 0; }
  virtual int start(void) { status = AM_MD_ALGO_RUNNING; return 0; }
  virtual int stop(void)  { status = AM_MD_ALGO_STOPPED; return 0; }
  bool is_running(void)   { return status == AM_MD_ALGO_RUNNING; }
  am_md_algo_t algo_type(void) { return type; }

  virtual int set_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi) { return 0; }
  virtual int get_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi) { return 0; }
  virtual int set_sensitivity(uint8_t roi_idx, uint32_t sensitivity)    { return 0; }
  virtual int get_sensitivity(uint8_t roi_idx, uint32_t* sensitivity)   { return 0; }
  virtual int set_threshold(uint8_t roi_idx, uint32_t threshold)        { return 0; }
  virtual int get_threshold(uint8_t roi_idx, uint32_t* threshold)       { return 0; }

protected:
  am_md_event_msg_t md_msgs[MD_MSGS_MAX_NUM];

};

typedef AmMdAlgo* (*get_instance_t)(void);
typedef void (*free_instance_t)(AmMdAlgo*);

enum {
  MD_NO_MOTION = 0,
  MD_IN_MOTION,
};

#define MAX_MOTION_INDICATOR            1000000
#define DEFAULT_KEY                     5678 // share memory key number, don't modify it

static inline int check_pointer(void* p)
{
  if (AM_UNLIKELY(!p)) {
    ERROR("null pointer\n");
    return -1;
  }
  return 0;
}


/* some helper functions */
inline void debug_md_msg(am_md_event_msg_t* p)
{
  if (AM_UNLIKELY(!p)) {
    return;
  }

  if (p->algo_type == AM_MD_ALGO_MSE) {
    DEBUG("MD::event msg {algo %d roi %d m-type %d diff %d ts %d}\n",
        p->algo_type, p->roi_idx, p->motion_type, p->diff, p->timestamp);
  }
  else if (p->algo_type == AM_MD_ALGO_MOG2) {
    DEBUG("MD::event msg {algo %d roi %d m-type %d motion %d ts %d}\n",
        p->algo_type, p->roi_idx, p->motion_type, p->motion, p->timestamp);
  }
  else {
    ERROR("MD::unknown algo type\n");
  }
}
inline void debug_md_msg_filter(am_md_event_msg_t* p,
                                int algo, int roi, int motion_type)
{
  if (AM_UNLIKELY(!p)) {
    return;
  }

  if ((p->algo_type & algo)                               &&
      (p->roi_idx == roi || roi == AM_MD_MAX_ROI_NUM)     &&
      (p->motion_type & motion_type)) {
    debug_md_msg(p);
  }
}
inline void debug_md_roi_info(am_md_roi_info_t* p)
{
  if (AM_UNLIKELY(!p)) {
    return;
  }

  DEBUG("MD:: roi: {left %d right %d top %d bottom %d thres %d sens %d "
        "valid %d}\n",
        p->left, p->right, p->top, p->bottom, p->threshold, p->sensitivity,
        p->valid);
}

inline int check_roi_idx(uint8_t roi_idx)
{
  if (roi_idx >= AM_MD_MAX_ROI_NUM) {
    ERROR("ROI index %d out of range(0~%d)\n", roi_idx, AM_MD_MAX_ROI_NUM-1);
    return -1;
  }
  return 0;
}
inline void check_sensitivity(uint32_t* sens)
{
  if (AM_UNLIKELY(!sens)) {
    return;
  }

  if (*sens > AM_MD_MAX_SENSITIVITY) {
    WARN("ROI sensitivity %d is out of range(0~%d), cutting to %d\n",
         *sens, AM_MD_MAX_SENSITIVITY, AM_MD_MAX_SENSITIVITY);
    *sens = AM_MD_MAX_SENSITIVITY;
  }
}
inline void check_threshold(uint32_t* thres)
{
  if (AM_UNLIKELY(!thres)) {
    return;
  }

  if (*thres > AM_MD_MAX_THRESHOLD) {
    WARN("ROI thresitivity %d is out of range(0~%d), cutting to %d\n",
         *thres, AM_MD_MAX_THRESHOLD, AM_MD_MAX_THRESHOLD);
    *thres = AM_MD_MAX_THRESHOLD;
  }
}
#endif //_MD_ALGO_H_
