/*
 * am_muxer_flag.h
 *
 * @Author: Hanbo Xiao
 * @Time  : 10/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_MUXER_INFO_H__
#define __AM_MUXER_INFO_H__

enum AmStreamNumber {
  AM_STREAM_NUMBER_1 = 0,
  AM_SRREAM_NUMBER_2,
  AM_STREAM_NUMBER_3,
  AM_STREAM_NUMBER_4,
  AM_STREAM_NUMBER_MAX
};

enum AmStreamType {
   AUDIO_STREAM = 0x1 << 0,
   VIDEO_STREAM = 0x1 << 1,
};

enum AmMuxerType {
   AM_MUXER_TYPE_NONE    = 0,
   AM_MUXER_TYPE_RAW     = 1 << 0,
   AM_MUXER_TYPE_TS_FILE = 1 << 1,
   AM_MUXER_TYPE_TS_HTTP = 1 << 2,
   AM_MUXER_TYPE_TS_HLS  = 1 << 3,
   AM_MUXER_TYPE_TS_IPTS = 1 << 4,
   AM_MUXER_TYPE_RTSP    = 1 << 5,
   AM_MUXER_TYPE_MP4     = 1 << 6,
   AM_MUXER_TYPE_JPEG    = 1 << 7,
   AM_MUXER_TYPE_MOV     = 1 << 8,
   AM_MUXER_TYPE_MKV     = 1 << 9,
   AM_MUXER_TYPE_AVI     = 1 << 10,
};

/* Update this macro when new muxer is added. */
#define MUXER_TYPE_AMOUNT 11

enum AmSinkType {
   AM_SINK_TYPE_NONE    = 0,
   AM_SINK_TYPE_RAW     = 1 << 0,
   AM_SINK_TYPE_TS_FILE = 1 << 1,
   AM_SINK_TYPE_TS_HTTP = 1 << 2,
   AM_SINK_TYPE_TS_HLS  = 1 << 3,
   AM_SINK_TYPE_TS_IPTS = 1 << 4,
   AM_SINK_TYPE_RTSP    = 1 << 5,
   AM_SINK_TYPE_MP4     = 1 << 6,
   AM_SINK_TYPE_JPEG    = 1 << 7,
   AM_SINK_TYPE_MOV     = 1 << 8,
   AM_SINK_TYPE_MKV     = 1 << 9,
   AM_SINK_TYPE_AVI     = 1 << 10,
};

#endif
