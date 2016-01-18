/*******************************************************************************
 * am_overlay_time_task.h
 *
 * History:
 *  Jan 6, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_OVERLAY_TIME_TASK_H_
#define AM_OVERLAY_TIME_TASK_H_

class AmVideoDevice;

class AmOverlayTimeTask {
  public:
    AmOverlayTimeTask(uint32_t max_stream_num, AmVideoDevice* dev, struct timespec* period);
    virtual ~AmOverlayTimeTask();
  public:
    bool run();
    bool stop();
    bool add(uint32_t streamId, uint32_t areaId, Point offset, TextBox *pBox);
    void remove(uint32_t streamId, uint32_t areaId);

  private:
    static void* mThreadMain(void* self);
    void  update();

  private:
    uint32_t             mStreamNumber;
    AmVideoDevice*       mVDev;
    AmOverlayGenerator** mTimeOverlayList;
    uint32_t             mTimeOverlayNumber;
    struct timespec      mPeriod;
    struct timespec      mNextTime;
    pthread_t            mThreadId;
    pthread_mutex_t      mListLock;
    pthread_mutex_t      mLock;
    pthread_cond_t       mCond;
    bool                 mIsRunning;
};


#endif /* AM_OVERLAY_TIME_TASK_H_ */
