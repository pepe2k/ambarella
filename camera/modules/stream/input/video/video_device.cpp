/*******************************************************************************
 * video_device.cpp
 *
 * Histroy:
 *   2012-10-3 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include "video_device.h"
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
#include "frame_stat.h"
#endif

#define DSP_MAX_PTS 0x3FFFFFFF

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
void CVideoDevice::SetFrameStatisticsCallback(void (*callback)(void *))
{
  frame_stat_set_callback(callback);
}
#endif

bool CVideoDevice::Init()
{
  if (AM_LIKELY(mIavFd < 0)) {
    if (AM_UNLIKELY((mIavFd = open("/dev/iav", O_RDWR)) < 0)) {
      PERROR("open");
    }
  }
  if (AM_LIKELY((mIavFd >= 0) && (false == mIsMemMapped))) {
    if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_MAP_BSB2, &mMemMapInfo) < 0)) {
      PERROR("IAV_IOC_MAP_BSB2");
      close(mIavFd);
      mIavFd = -1;
    } else if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_MAP_DSP, &mMemMapInfo) < 0)) {
      PERROR("IAV_IOC_MAP_DSP");
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_UNMAP_BSB, 0) < 0)) {
        PERROR("IAV_IOC_UNMAP_BSB");
      }
      close(mIavFd);
      mIavFd = -1;
    } else {
      mIsMemMapped = true;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
      mIsFrameStatInit = frame_stat_init(mIavFd);
#endif
    }
  }


  return ((mIavFd >= 0) && mIsMemMapped
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
          && mIsFrameStatInit
#endif
         );
}

void CVideoDevice::Fini()
{
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
  if (AM_LIKELY(mIsFrameStatInit)) {
    frame_stat_destroy();
    mIsFrameStatInit = false;
  }
#endif
  if (AM_LIKELY((mIavFd >= 0) && mIsMemMapped)) {
    if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_UNMAP_DSP, 0) < 0)) {
      PERROR("IAV_IOC_UNMAP_DSP");
    }
    if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_UNMAP_BSB, 0) < 0)) {
      PERROR("IAV_IOC_UNMAP_BSB");
    }
    close(mIavFd);
    mIavFd = -1;
    mIsMemMapped = false;
  }
}

AM_INT CVideoDevice::GetIavStatus()
{
  AM_INT ret = IAV_STATE_INIT;
  if (AM_LIKELY(Init())) {
    iav_state_info_t status;
    if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &status) < 0)) {
      PERROR("IAV_IOC_GET_STATE_INFO");
    } else {
      ret = status.state;
    }
  } else {
    ERROR("Failed to initialized IAV device!");
  }

  return ret;
}

/*******************************************************************************
 * CVideoDeviceA5s
 ******************************************************************************/
CVideoDeviceA5s::CVideoDeviceA5s() :
    CVideoDevice()
{
  memset(mLastPts, 0, sizeof(AM_PTS)*MAX_ENCODE_STREAM_NUM);
  memset(mLastDspPts, 0, sizeof(AM_U32)*MAX_ENCODE_STREAM_NUM);
  memset(mEncodeFormat, 0,
         sizeof(iav_encode_format_ex_t*)*MAX_ENCODE_STREAM_NUM);
  memset(mH264Config, 0, sizeof(iav_h264_config_ex_t*)*MAX_ENCODE_STREAM_NUM);
  memset(mJpegConfig, 0, sizeof(iav_jpeg_config_ex_t*)*MAX_ENCODE_STREAM_NUM);
  memset(mVideoInfo,  0, sizeof(AM_VIDEO_INFO*)*MAX_ENCODE_STREAM_NUM);
}

CVideoDeviceA5s::~CVideoDeviceA5s()
{
  for (AM_UINT i = 0; i < GetMaxEncodeNumber(); ++ i) {
    delete mEncodeFormat[i];
    delete mH264Config[i];
    delete mJpegConfig[i];
    delete mVideoInfo[i];
  }
}

AM_ERR CVideoDeviceA5s::Start(AM_UINT streamid)
{
  AM_ERR ret = ME_ERROR;
  AM_INT status = GetIavStatus();
  if (AM_LIKELY((status == IAV_STATE_PREVIEW) ||
                (status == IAV_STATE_ENCODING))) {
    if (AM_LIKELY(streamid < GetMaxEncodeNumber())) {
      /* Start Single Stream */
      iav_encode_stream_info_ex_t encode;
      encode.id = 1 << streamid;
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
                            &encode) < 0)) {
        PERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
      } else if (AM_UNLIKELY(encode.state == IAV_STREAM_STATE_ENCODING)) {
        ret = ME_OK;
        NOTICE("Stream%u: Encoding already started!", streamid);
      } else {
        if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_START_ENCODE_EX,
                              encode.id) < 0)) {
          ERROR("Stream%u: IAV_IOC_START_ENCODE_EX: %s",
                streamid, strerror(errno));
        } else {
          NOTICE("Start encoding for stream%u successfully!", streamid);
          ret = ME_OK;
        }
      }
    } else if (AM_LIKELY(streamid == GetMaxEncodeNumber())) {
      /* Start All Available Stream */
      AM_UINT id = 0;
      iav_encode_format_ex_t info;
      for (AM_UINT i = 0; i < GetMaxEncodeNumber(); ++ i) {
        info.id = 1 << i;
        if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_ENCODE_FORMAT_EX,
                              &info) < 0)) {
          PERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
        } else if (AM_LIKELY(((info.encode_type == 1) ||
                              (info.encode_type == 2)) && /* H.264 */
                             (mVideoStreamMap & info.id))) { /*Stream enabled*/
          id |= info.id;
        } else {
          NOTICE("Stream %u is not enabled!", i);
        }
      }
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_START_ENCODE_EX, id) < 0)) {
        PERROR("IAV_IOC_START_ENCODE_EX");
      } else {
        ret = ME_OK;
      }
    } else {
      ERROR("Stream %u is not supported!", streamid);
    }
  } else {
    ERROR("Video device should be in preview state or encoding state!");
  }

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
  if (AM_LIKELY(ME_OK == ret)) {
    ret = frame_stat_start() ? ME_OK : ME_ERROR;
  }
#endif

  return ret;
}

AM_ERR CVideoDeviceA5s::Stop(AM_UINT streamid)
{
  AM_ERR ret = ME_ERROR;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
  frame_stat_stop();
#endif
  if (AM_LIKELY(streamid < GetMaxEncodeNumber())) {
    iav_encode_stream_info_ex_t encode;
    encode.id = 1 << streamid;
    if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
                          &encode) < 0)) {
      PERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
    } else if (encode.state == IAV_STREAM_STATE_ENCODING) {
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_STOP_ENCODE_EX, encode.id) < 0)) {
        PERROR("IAV_IOC_STOP_ENCODE_EX");
      } else {
        ret = ME_OK;
      }
    } else {
      NOTICE("Stream%u is already stopped!", streamid);
    }
  } else if (AM_LIKELY(streamid == GetMaxEncodeNumber())) { /* Stop All */
    iav_encode_stream_info_ex_t encode;
    iav_stream_id_t id = 0;
    INFO("Stop all streams!");
    for (AM_UINT i = 0; i < GetMaxEncodeNumber(); ++ i) {
      encode.id = (1 << i);
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
                            &encode) < 0)) {
        PERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
      } else if (encode.state == IAV_STREAM_STATE_ENCODING) {
        id |= encode.id;
        INFO("Stream%u needs to be stopped!", i);
      }
    }
    if (AM_LIKELY(id > 0)) {
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_STOP_ENCODE_EX, id) < 0)) {
        PERROR("IAV_IOC_STOP_ENCODE_EX");
      } else {
        INFO("Streams stopped successfully!");
        ret = ME_OK;
      }
    } else {
      NOTICE("All streams are already stopped!");
      ret = ME_OK;
    }
  } else {
    ERROR("Stream %u is not supported!", streamid);
  }

  return ret;
}

AM_ERR CVideoDeviceA5s::GetVideoParameter(AM_VIDEO_INFO *info, AM_UINT streamid)
{
  AM_ERR ret = ME_ERROR;

  if (AM_LIKELY(info && (streamid < GetMaxEncodeNumber()))) {
    if (AM_LIKELY(mEncodeFormat[streamid] == NULL)) {
      mEncodeFormat[streamid] = new iav_encode_format_ex_t;
    }

    if (AM_LIKELY(mEncodeFormat[streamid])) {
      AM_U32 fps = 0;
      iav_change_framerate_factor_ex_t framefactor;

      mEncodeFormat[streamid]->id = 1 << streamid;
      framefactor.id = 1 << streamid;

      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_FRAMERATE_FACTOR_EX,
                            &framefactor))) {
        PERROR("IAV_IOC_GET_FRAMRATE_FACTOR_EX");
      } else if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_FRAME_RATE,
                                   &fps) < 0)) {
        PERROR("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
      } else if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_ENCODE_FORMAT_EX,
                                   mEncodeFormat[streamid]) < 0)) {
        PERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
      } else if (mEncodeFormat[streamid]->encode_type == 1) { /* H.264 */
        if (AM_LIKELY(mH264Config[streamid] == NULL)) {
          mH264Config[streamid] = new iav_h264_config_ex_t;
        }

        if (AM_LIKELY(mH264Config[streamid])) {
          mH264Config[streamid]->id = 1 << streamid;

          if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_H264_CONFIG_EX,
                                mH264Config[streamid]) < 0)) {
            PERROR("IAV_IOC_GET_H264_CONFIG_EX");
          } else {
            if (AM_LIKELY(mVideoInfo[streamid] == NULL)) {
              mVideoInfo[streamid] = new AM_VIDEO_INFO;
            }
            info->needsync = mAvSyncMap & (1 << streamid);
            info->type     = 1;
            info->width    = mEncodeFormat[streamid]->encode_width;
            info->height   = mEncodeFormat[streamid]->encode_height;
            info->rate     = mH264Config[streamid]->pic_info.rate;
            info->scale    = mH264Config[streamid]->pic_info.scale;
            info->mul      = framefactor.ratio_numerator;
            info->div      = framefactor.ratio_denominator;
            info->M        = mH264Config[streamid]->M;
            info->N        = mH264Config[streamid]->N;
            info->fps      = fps;
            if (AM_UNLIKELY((info->mul == 0) || (info->div == 0))) {
              info->mul = 1;
              info->div = 1;
            }
            memcpy(mVideoInfo[streamid], info, sizeof(AM_VIDEO_INFO));
            ret = ME_OK;
          }
        } else {
          ERROR("Memory not available!");
          ERROR("Failed to get H.264 configuration for stream%u", streamid);
        }
      } else if (mEncodeFormat[streamid]->encode_type == 2) { /* MJpeg */
        if (AM_LIKELY(mJpegConfig[streamid] == NULL)) {
          mJpegConfig[streamid] = new iav_jpeg_config_ex_t;
        }

        if (AM_LIKELY(mJpegConfig[streamid])) {
          mJpegConfig[streamid]->id = 1 << streamid;

          if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_GET_JPEG_CONFIG_EX,
                                mJpegConfig[streamid]) < 0)) {
            PERROR("IAV_IOC_GET_JPEG_CONFIG_EX");
          } else {
            if (AM_LIKELY(mVideoInfo[streamid] == NULL)) {
              mVideoInfo[streamid] = new AM_VIDEO_INFO;
            }
            info->needsync = mAvSyncMap & (1 << streamid);
            info->type     = 2;
            info->width    = mEncodeFormat[streamid]->encode_width;
            info->height   = mEncodeFormat[streamid]->encode_height;
            info->rate     = 0;
            info->scale    = 0;
            info->mul      = framefactor.ratio_numerator;
            info->div      = framefactor.ratio_denominator;
            info->M        = 0;
            info->N        = 0;
            info->fps      = fps;
            if (AM_UNLIKELY((info->mul == 0) || (info->div == 0))) {
              info->mul = 1;
              info->div = 1;
            }
            memcpy(mVideoInfo[streamid], info, sizeof(AM_VIDEO_INFO));
            ret = ME_OK;
          }
        } else {
          ERROR("Memory not available!");
          ERROR("Failed to get MJPEG configuration for stream%u", streamid);
        }
      } else {
        ERROR("Stream%u is not enabled!", streamid);
      }
    } else {
      ERROR("Memory not available!");
    }
  } else if (!info) {
    ERROR("Null pointer of AM_VIDEO_INFO!");
  } else {
    ERROR("Stream %u is not supported!", streamid);
  }

  return ret;
}

AM_ERR CVideoDeviceA5s::GetVideoData(CPacket* &packet)
{
  AM_ERR ret = ME_OK;
  bits_info_ex_t bitsInfo;
  if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_READ_BITSTREAM_EX, &bitsInfo) < 0)) {
    PERROR("IAV_IOC_READ_BITSTREAM_EX");
    ret = ME_ERROR;
  } else {
    if (AM_UNLIKELY(bitsInfo.stream_end)) {
      /* Fixme:
       * This is a work around for pic_type,
       * This value should be read from bitsInfo.pic_type */
      AM_UINT pic_type = 0;
      if (AM_LIKELY(mEncodeFormat[bitsInfo.stream_id])) {
        if (mEncodeFormat[bitsInfo.stream_id]->encode_type == 1) {
          pic_type = P_FRAME; /* Fake a pic type */
        } else if (mEncodeFormat[bitsInfo.stream_id]->encode_type == 2) {
          pic_type = JPEG_STREAM; /* Fake a pic type */
        }
      } else {
        ERROR("Unknown stream, ID %u", bitsInfo.stream_id);
        ret = ME_ERROR;
      }
      NOTICE("Stream%u type %u reached its end!",
             bitsInfo.stream_id, pic_type);
      GetVideoEos(packet, bitsInfo.stream_id, pic_type);
    } else {
      CPacket::Payload *videoData =
          packet->GetPayload(CPacket::AM_PAYLOAD_TYPE_DATA,
                             CPacket::AM_PAYLOAD_ATTR_VIDEO);
      videoData->mData.mBuffer     = ((AM_U8*)bitsInfo.start_addr);
      videoData->mData.mPayloadPts = bitsInfo.monotonic_pts;
      videoData->mData.mStreamId   = bitsInfo.stream_id;
      videoData->mData.mFrameType  = bitsInfo.pic_type;
      videoData->mData.mFrameAttr  = bitsInfo.jpeg_quality;
      videoData->mData.mFrameCount = 1;
      videoData->mData.mSize       = bitsInfo.pic_size;
      ret = FixPts(bitsInfo, videoData->mData.mPayloadPts) ? ME_OK : ME_ERROR;

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
      frame_stat_feed_bsinfo(&bitsInfo);
#endif
    }
  }

  return ret;
}

void CVideoDeviceA5s::GetVideoEos(CPacket* &packet,
                                  AM_UINT streamid,
                                  AM_UINT type)
{
  CPacket::Payload *eos = packet->GetPayload(CPacket::AM_PAYLOAD_TYPE_EOS,
                                             CPacket::AM_PAYLOAD_ATTR_VIDEO);
  eos->mData.mSize = 0;
  eos->mData.mBuffer = NULL;
  eos->mData.mPayloadPts = mLastPts[streamid];
  eos->mData.mStreamId = streamid;
  eos->mData.mFrameType = type;
  if (mVideoInfo[streamid] && (mVideoInfo[streamid]->type == 1)) {
    if (AM_LIKELY((mLastPts[streamid] != 0) && mH264Config[streamid])) {
      eos->mData.mPayloadPts += mH264Config[streamid]->pic_info.rate;
    }
  } else if (mVideoInfo[streamid] && (mVideoInfo[streamid]->type == 2)) {
    if (AM_LIKELY(mLastPts[streamid] != 0)) {
      eos->mData.mPayloadPts += (mVideoInfo[streamid]->fps * 90000 / 512000000);
    }
  }
}

void CVideoDeviceA5s::SetVideoStreamMap(AM_UINT map)
{
  mVideoStreamMap = map;
}

void CVideoDeviceA5s::SetAvSyncMap(AM_UINT avSyncMap)
{
  mAvSyncMap = avSyncMap;
}

inline bool CVideoDeviceA5s::FixPts(bits_info_ex_t& bitsInfo, AM_PTS& monopts)
{
  bool ret = true;
  AM_S32 dspPtsDiff = 0;
  AM_S64 ptsDiff = 0;

  monopts = bitsInfo.monotonic_pts;
  if (AM_LIKELY(mLastDspPts[bitsInfo.stream_id])) {
    dspPtsDiff = (AM_S32)(bitsInfo.PTS - mLastDspPts[bitsInfo.stream_id]);
    if (AM_UNLIKELY(AM_ABS(dspPtsDiff) >= (DSP_MAX_PTS >> 1))) {
      if (dspPtsDiff < 0) {
        dspPtsDiff += DSP_MAX_PTS;
      } else if (dspPtsDiff > 0) {
        dspPtsDiff -= DSP_MAX_PTS;
      }
    }
  } else {
    switch(mEncodeFormat[bitsInfo.stream_id]->encode_type) {
      case 1: { /* H.264 */
        dspPtsDiff = (90000LLU * (AM_U64)mVideoInfo[bitsInfo.stream_id]->rate /
            (AM_U64)mVideoInfo[bitsInfo.stream_id]->scale);
      }break;
      case 2: { /* MJPEG */
        dspPtsDiff = (AM_S32)((AM_U64)mVideoInfo[bitsInfo.stream_id]->fps /
                              512000000LLU * 90000LLU);
      }break;
      default:break;
    }
  }

  if (AM_LIKELY(mLastPts[bitsInfo.stream_id])) {
    ptsDiff = (AM_S64)(bitsInfo.monotonic_pts - mLastPts[bitsInfo.stream_id]);
  } else {
    switch(mEncodeFormat[bitsInfo.stream_id]->encode_type) {
      case 1: { /* H.264 */
        ptsDiff = (90000LLU * (AM_U64)mVideoInfo[bitsInfo.stream_id]->rate /
            (AM_U64)mVideoInfo[bitsInfo.stream_id]->scale);
      }break;
      case 2: { /* MJPEG */
        ptsDiff = (90000LLU * (AM_U64)mVideoInfo[bitsInfo.stream_id]->fps /
                   512000000LLU);
      }break;
      default: break;
    }
  }

  if (AM_UNLIKELY(monopts && (AM_ABS(ptsDiff - dspPtsDiff) > 30000))) {
    AM_UINT maxErrorCount = 300;
    switch(mEncodeFormat[bitsInfo.stream_id]->encode_type) {
      case 1 : { /* H.264 */
        maxErrorCount = (AM_UINT)
            (mH264Config[bitsInfo.stream_id]->pic_info.scale*2 +
                mH264Config[bitsInfo.stream_id]->pic_info.rate) /
                (mH264Config[bitsInfo.stream_id]->pic_info.rate*2) * 5;
      }break;
      case 2 : { /* MJPEG */
        maxErrorCount = (AM_UINT)
            ((512000000 + mVideoInfo[bitsInfo.stream_id]->fps) /
                (2 * mVideoInfo[bitsInfo.stream_id]->fps)) * 5;
      }break;
      default: maxErrorCount = 300;
      break;
    }
    if (mErrorCount[bitsInfo.stream_id] < maxErrorCount) {
      ++ mErrorCount[bitsInfo.stream_id];
      PRINTF("Fatal Error: PTS value abnormal, trying to fix!"
             "\n    Frame Number: %u"
             "\nCurrent MONO PTS: %llu"
             "\n   Last MONO PTS: %llu"
             "\n   MONO PTS Diff: %lld"
             "\n Current DSP PTS: %u"
             "\n    Last DSP PTS: %u"
             "\n    DSP PTS Diff: %d"
             "\n       Stream ID: %hu"
             "\n      Frame Type: %hu",
           bitsInfo.frame_num,
           monopts,
           mLastPts[bitsInfo.stream_id],
           ptsDiff,
           bitsInfo.PTS,
           mLastDspPts[bitsInfo.stream_id],
           dspPtsDiff,
           bitsInfo.stream_id,
           bitsInfo.pic_type);
    } else {
      ERROR("Fatal Error: PTS value abnormal, more than %u times, abort!"
            "\n    Frame Number: %u"
            "\nCurrent MONO PTS: %llu"
            "\n   Last MONO PTS: %llu"
            "\n   MONO PTS Diff: %lld"
            "\n Current DSP PTS: %u"
            "\n    Last DSP PTS: %u"
            "\n    DSP PTS Diff: %d"
            "\n       Stream ID: %hu"
            "\n      Frame Type: %hu",
            mErrorCount[bitsInfo.stream_id],
            bitsInfo.frame_num,
            monopts,
            mLastPts[bitsInfo.stream_id],
            ptsDiff,
            bitsInfo.PTS,
            mLastDspPts[bitsInfo.stream_id],
            dspPtsDiff,
            bitsInfo.stream_id,
            bitsInfo.pic_type);
      mErrorCount[bitsInfo.stream_id] = 0;
      ret = false;
    }
    monopts = mLastPts[bitsInfo.stream_id] + dspPtsDiff;
  } else if (AM_UNLIKELY(monopts == 0)) {
    AM_UINT maxErrorCount = 300;
    switch(mEncodeFormat[bitsInfo.stream_id]->encode_type) {
      case 1 : { /* H.264 */
        maxErrorCount = (AM_UINT)
                      (mH264Config[bitsInfo.stream_id]->pic_info.scale*2 +
                          mH264Config[bitsInfo.stream_id]->pic_info.rate) /
                          (mH264Config[bitsInfo.stream_id]->pic_info.rate*2) * 5;
      }break;
      case 2 : { /* MJPEG */
        maxErrorCount = (AM_UINT)
                      ((512000000 + mVideoInfo[bitsInfo.stream_id]->fps) /
                          (2 * mVideoInfo[bitsInfo.stream_id]->fps)) * 5;
      }break;
      default: maxErrorCount = 300;
      break;
    }
    if (AM_LIKELY(mEncodeFormat[bitsInfo.stream_id]->encode_type == 1)) {
      if (mErrorCount[bitsInfo.stream_id] < maxErrorCount) {
        ++ mErrorCount[bitsInfo.stream_id];
        PRINTF("Fatal Error: PTS jumps to 0, trying to fix!"
               "\n    Frame Number: %u"
               "\nCurrent MONO PTS: %llu"
               "\n   Last MONO PTS: %llu"
               "\n Current DSP PTS: %u"
               "\n    Last DSP PTS: %u"
               "\n     Buffer Addr: %p"
               "\n       Stream ID: %hu"
               "\n      Frame Type: %hu"
               "\n      Frame Size: %u",
            bitsInfo.frame_num,
            monopts,
            mLastPts[bitsInfo.stream_id],
            bitsInfo.PTS,
            mLastDspPts[bitsInfo.stream_id],
            ((AM_U8*)bitsInfo.start_addr),
            bitsInfo.stream_id,
            bitsInfo.pic_type,
            bitsInfo.pic_size);
      } else {
        ERROR("Fatal Error: PTS jumps to 0 more than %u times, abort!"
              "\n    Frame Number: %u"
              "\nCurrent MONO PTS: %llu"
              "\n   Last MONO PTS: %llu"
              "\n Current DSP PTS: %u"
              "\n    Last DSP PTS: %u"
              "\n     Buffer Addr: %p"
              "\n       Stream ID: %hu"
              "\n      Frame Type: %hu"
              "\n      Frame Size: %u",
              mErrorCount[bitsInfo.stream_id],
              bitsInfo.frame_num,
              monopts,
              mLastPts[bitsInfo.stream_id],
              bitsInfo.PTS,
              mLastDspPts[bitsInfo.stream_id],
              ((AM_U8*)bitsInfo.start_addr),
              bitsInfo.stream_id,
              bitsInfo.pic_type,
              bitsInfo.pic_size);
        mErrorCount[bitsInfo.stream_id] = 0;
        ret = false;
      }
      monopts = mLastPts[bitsInfo.stream_id] + dspPtsDiff;
    } else if (AM_LIKELY(mEncodeFormat[bitsInfo.stream_id]->encode_type == 2)) {
      if (mErrorCount[bitsInfo.stream_id] < maxErrorCount) {
        ++ mErrorCount[bitsInfo.stream_id];
        PRINTF("Fatal Error: PTS jumps to 0, trying to fix!"
               "\n    Frame Number: %u"
               "\nCurrent MONO PTS: %llu"
               "\n   Last MONO PTS: %llu"
               "\n Current DSP PTS: %u"
               "\n    Last DSP PTS: %u"
               "\n     Buffer Addr: %p"
               "\n       Stream ID: %hu"
               "\n      Frame Type: %hu"
               "\n      Frame Size: %u",
             bitsInfo.frame_num,
             monopts,
             mLastPts[bitsInfo.stream_id],
             bitsInfo.PTS,
             mLastDspPts[bitsInfo.stream_id],
             ((AM_U8*)bitsInfo.start_addr),
             bitsInfo.stream_id,
             bitsInfo.pic_type,
             bitsInfo.pic_size);
      } else {
        ERROR("Fatal Error: PTS jumps to 0 more than %u times, abort!"
              "\n    Frame Number: %u"
              "\nCurrent MONO PTS: %llu"
              "\n   Last MONO PTS: %llu"
              "\n Current DSP PTS: %u"
              "\n    Last DSP PTS: %u"
              "\n     Buffer Addr: %p"
              "\n       Stream ID: %hu"
              "\n      Frame Type: %hu"
              "\n      Frame Size: %u",
              bitsInfo.frame_num,
              monopts,
              mLastPts[bitsInfo.stream_id],
              bitsInfo.PTS,
              mLastDspPts[bitsInfo.stream_id],
              ((AM_U8*)bitsInfo.start_addr),
              bitsInfo.stream_id,
              bitsInfo.pic_type,
              bitsInfo.pic_size);
        mErrorCount[bitsInfo.stream_id] = 0;
        ret = false;
      }
      monopts = mLastPts[bitsInfo.stream_id] + dspPtsDiff;
    }
  } else if (AM_UNLIKELY((bitsInfo.start_addr == 0) ||
                         (bitsInfo.pic_size == 0))) {
    ERROR("IAV read bitstream returned OK, but buffer is invalid!");
    ret = false;
  } else {
    mErrorCount[bitsInfo.stream_id] = 0;
  }
  mLastPts[bitsInfo.stream_id] = monopts;
  mLastDspPts[bitsInfo.stream_id] = bitsInfo.PTS;

  return ret;
}
