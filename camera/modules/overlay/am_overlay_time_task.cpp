/*******************************************************************************
 * am_overlay_generatortime_task.cpp
 *
 * History:
 *  Jan 8, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_overlay.h"

AmOverlayTimeTask::AmOverlayTimeTask(uint32_t max_stream_num, AmVideoDevice* dev, struct timespec* period):
  mStreamNumber(max_stream_num),
  mVDev(dev),
  mTimeOverlayList(NULL),
  mTimeOverlayNumber(0),
  mThreadId(0),
  mIsRunning(false)
{
  pthread_mutex_init(&mListLock, NULL);
  pthread_mutex_init(&mLock, NULL);
  pthread_cond_init(&mCond, NULL);
  memset(&mNextTime, 0, sizeof(struct timespec));
  mTimeOverlayNumber = mStreamNumber * MAX_OVERLAY_AREA_NUM;
  mTimeOverlayList = new AmOverlayGenerator *[mTimeOverlayNumber];
  for (uint32_t i = 0; i < mTimeOverlayNumber; ++i) {
    mTimeOverlayList[i] = NULL;
  }
  if (!period) {
    mPeriod.tv_sec = 1;
    mPeriod.tv_nsec = 0;
  } else {
    memcpy(&mPeriod, period, sizeof(struct timespec));
  }

}

AmOverlayTimeTask::~AmOverlayTimeTask()
{
  stop();
  for (uint32_t i = 0; i < mTimeOverlayNumber; ++ i) {
    delete mTimeOverlayList[i];
  }
  delete[] mTimeOverlayList;
}

bool AmOverlayTimeTask::run()
{
  if (!mIsRunning) {
    if (clock_gettime(CLOCK_REALTIME, &mNextTime) != 0) {
      PERROR("clock_gettime");
    } else if (mVDev) {
      if (pthread_create(&mThreadId, NULL, &mThreadMain, this) == 0) {
        mIsRunning = true;
        DEBUG("Overlay Time task is running.");
      }
    }
  }
  return mIsRunning;
}

bool AmOverlayTimeTask:: stop()
{
  if (mIsRunning) {
    DEBUG("Try to stop time overlay task.");
    if (pthread_mutex_lock(&mListLock) != 0) {
      PERROR("pthread_mutex_lock");
    }
    if (pthread_cancel(mThreadId) != 0) {
      PERROR("pthread_cancel");
    }
    if (pthread_join(mThreadId, NULL) != 0) {
      PERROR("pthread_join");
    }
    if (pthread_mutex_unlock(&mListLock) != 0) {
      PERROR("pthread_mutex_unlock");
    }

    DEBUG("Thread time overlay task stopped.");
    DEBUG("Try to remove time overlay data.");
    for (uint32_t streamId = 0; streamId < mStreamNumber; ++streamId) {
      for (uint32_t areaId = 0; areaId < MAX_OVERLAY_AREA_NUM; ++areaId) {
        remove(streamId, areaId);
      }
    }
    mThreadId = 0;
    mIsRunning = false;

    DEBUG("stop time overlay task.");
  }

  return !mIsRunning;
}

bool AmOverlayTimeTask::add(uint32_t streamId, uint32_t areaId, Point offset,
                            TextBox *pBox)
{
  bool ret = false;
  if (mVDev) {
    EncodeSize size;
    uint32_t overlay_max_size =
        mVDev->get_stream_overlay_max_size(streamId, areaId);
    mVDev->get_stream_size(streamId, &size);
    AmOverlayGenerator *overlay_generator = new AmOverlayGenerator(overlay_max_size);
    Overlay *overlay = overlay_generator->create_time(size, offset, pBox);

    if (overlay) {
      if (pthread_mutex_lock(&mListLock) != 0) {
        PERROR("pthread_mutex_lock");
      }
      mTimeOverlayList[streamId * MAX_OVERLAY_AREA_NUM + areaId] = overlay_generator;
      if (pthread_mutex_unlock(&mListLock) != 0) {
        PERROR("pthread_mutex_unlock");
      }
      ret = true;
      DEBUG("add stream%u area%u in overlay time task.", streamId, areaId);
    } else {
      delete overlay_generator;
    }
  } else {
    ERROR("Encode Device not initialized.");
  }
  return ret;
}

void AmOverlayTimeTask::remove(uint32_t streamId, uint32_t areaId)
{
  if (pthread_mutex_lock(&mListLock) != 0) {
    PERROR("pthread_mutex_lock");
  }
  AmOverlayGenerator *overlay_generator =
      mTimeOverlayList[streamId * MAX_OVERLAY_AREA_NUM + areaId];
  mTimeOverlayList[streamId * MAX_OVERLAY_AREA_NUM + areaId] = NULL;
  delete overlay_generator;
  if (pthread_mutex_unlock(&mListLock) != 0) {
    PERROR("pthread_mutex_unlock");
  }
}

void AmOverlayTimeTask::update()
{
  if (mVDev) {
    AmOverlayGenerator *overlay_generator = NULL;
    for (uint32_t streamId = 0; streamId < mStreamNumber; ++streamId) {
      for (uint32_t areaId = 0; areaId < MAX_OVERLAY_AREA_NUM; ++areaId) {
        if (pthread_mutex_lock(&mListLock) != 0) {
          PERROR("pthread_mutex_lock");
        }
        overlay_generator = mTimeOverlayList[streamId * MAX_OVERLAY_AREA_NUM + areaId];
        if (overlay_generator) {
          mVDev->set_stream_overlay(streamId, areaId,
              overlay_generator->update_time());
        }
        if (pthread_mutex_unlock(&mListLock) != 0) {
          PERROR("pthread_mutex_unlock");
        }
      }
    }
  } else {
    ERROR("Encode Device not initialized.");
  }
}

void* AmOverlayTimeTask:: mThreadMain(void* arg)
{
  AmOverlayTimeTask* self = (AmOverlayTimeTask*)arg;
  struct timespec now;

  if (pthread_mutex_lock(&(self->mLock)) != 0) {
    PERROR("pthread_mutex_lock");
  }
  while (true) {
    int rc = pthread_cond_timedwait(&(self->mCond), &(self->mLock),
        &(self->mNextTime));
    if (rc != ETIMEDOUT && rc != 0) {
      PERROR("pthread_cond_timewait");
    }
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
      PERROR("clock_gettime");
    }
    if (self->mPeriod.tv_sec || self->mPeriod.tv_nsec) {
      self->mNextTime.tv_sec = now.tv_sec + self->mPeriod.tv_sec;
      self->mNextTime.tv_nsec = now.tv_nsec + self->mPeriod.tv_nsec;
    }
    self->update();
    if (pthread_mutex_unlock(&(self->mLock)) != 0) {
      PERROR("pthread_mutex_unlock");
    }
  }
  return (void*)NULL;
}




