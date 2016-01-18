/*******************************************************************************
 * am_media_info.h
 *
 * Histroy:
 *  2012-7-16 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_MEDIA_INFO_H_
#define AM_MEDIA_INFO_H_

enum MM_MEDIA_FORMAT
{
  MF_NULL,
  MF_TEST,

  MF_H264,
  MF_MJPEG,

  MF_PCM,
  MF_G711,
  MF_G726_40,
  MF_G726_32,
  MF_G726_24,
  MF_G726_16,
  MF_AAC,
  MF_OPUS,
  MF_BPCM,

  MF_TS,
  MF_MP4,
};

struct AM_VIDEO_INFO {
    AM_U32 needsync; /* Indicate if A/V sync is needed in AVQueue */
    AM_U32 type;  /* Video encode type: 1: H.264, 2: MJPEG */
    AM_U32 fps;   /* framerate = 512000000 / fps */
    AM_U32 rate;  /* Video rate */
    AM_U32 scale; /* Video scale, framerate = scale/rate, 29.97 = 30000/1001 */
    AM_U16 mul;   /* Video framerate numerator */
    AM_U16 div;   /* Video framerate demoninator */
    AM_U16 devidor;
    AM_U16 width;
    AM_U16 height;
    AM_U16 M;
    AM_U16 N;
};

struct AM_AUDIO_INFO {
    AM_UINT needsync;       /* Indicate if A/V sync is needed in AVQueue */
    AM_UINT sampleRate;
    AM_UINT channels;
    AM_UINT pktPtsIncr;
    AM_UINT sampleSize;
    AM_UINT chunkSize;
    MM_MEDIA_FORMAT format;
};

#endif /* AM_MEDIA_INFO_H_ */

