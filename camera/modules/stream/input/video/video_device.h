/*******************************************************************************
 * video_device.h
 *
 * Histroy:
 *   2012-9-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef VIDEO_DEVICE_H_
#define VIDEO_DEVICE_H_

class CVideoDevice
{
  public:
    CVideoDevice() :
      mIavFd(-1),
      mIsMemMapped(false),
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
      mIsFrameStatInit(false),
#endif
      mVideoStreamMap(0),
      mAvSyncMap(0) {
      memset(mErrorCount, 0, sizeof(mErrorCount));
    }
    virtual ~CVideoDevice() {
      Fini();
    }

  public:
    int GetIavFd() {return mIavFd;}
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    void SetFrameStatisticsCallback(void (*callback)(void *));
#endif

  public:
    virtual AM_ERR Start(AM_UINT streamid) = 0;
    virtual AM_ERR Stop(AM_UINT streamid)  = 0;
    virtual AM_ERR GetVideoParameter(AM_VIDEO_INFO *info, AM_UINT streamid) = 0;
    virtual AM_UINT GetMaxEncodeNumber() = 0;
    virtual AM_ERR GetVideoData(CPacket* &packet) = 0;
    virtual void GetVideoEos(CPacket* &packet,
                             AM_UINT streamid,
                             AM_UINT type) = 0;
    virtual void SetVideoStreamMap(AM_UINT map) = 0;
    virtual void SetAvSyncMap(AM_UINT avSyncMap) = 0;

  public:
    virtual bool Init();
    virtual void Fini();

  protected:
    virtual AM_INT GetIavStatus();

  protected:
    int                    mIavFd;
    bool                   mIsMemMapped;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    bool                   mIsFrameStatInit;
#endif
    iav_mmap_info_t        mMemMapInfo;
    AM_UINT                mVideoStreamMap;
    AM_UINT                mAvSyncMap;
    AM_UINT                mErrorCount[MAX_ENCODE_STREAM_NUM];
};

class CVideoDeviceA5s: public CVideoDevice
{
  public:
    CVideoDeviceA5s();
    virtual ~CVideoDeviceA5s();

  public:
    virtual AM_ERR Start(AM_UINT streamid = MAX_ENCODE_STREAM_NUM);
    virtual AM_ERR Stop(AM_UINT streamid = MAX_ENCODE_STREAM_NUM);
    virtual AM_ERR GetVideoParameter(AM_VIDEO_INFO *info, AM_UINT streamid);
    virtual AM_UINT GetMaxEncodeNumber(){return MAX_ENCODE_STREAM_NUM;}
    virtual AM_ERR GetVideoData(CPacket* &packet);
    virtual void GetVideoEos(CPacket* &packet, AM_UINT streamid, AM_UINT type);
    virtual void SetVideoStreamMap(AM_UINT map);
    virtual void SetAvSyncMap(AM_UINT avSyncMap);

  private:
    bool FixPts(bits_info_ex_t& bisInfo, AM_PTS& monopts);

  private:
    AM_PTS mLastPts[MAX_ENCODE_STREAM_NUM];
    AM_U32 mLastDspPts[MAX_ENCODE_STREAM_NUM];
    iav_encode_format_ex_t *mEncodeFormat[MAX_ENCODE_STREAM_NUM];
    iav_h264_config_ex_t   *mH264Config[MAX_ENCODE_STREAM_NUM];
    iav_jpeg_config_ex_t   *mJpegConfig[MAX_ENCODE_STREAM_NUM];
    AM_VIDEO_INFO          *mVideoInfo[MAX_ENCODE_STREAM_NUM];
};


#endif /* VIDEO_DEVICE_H_ */
