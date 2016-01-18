/*
 * am_data_transfer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 23/07/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, oi transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_PACKET_TRANSFER_H__
#define __AM_PACKET_TRANSFER_H__

#ifndef TRANSFER_COMMUNICATE_PATH
#define TRANSFER_COMMUNICATE_PATH "/tmp/am_datatransfer"
#endif

#ifndef BLOCK_SIZE_MAX
#define BLOCK_SIZE_MAX 512 * 1024
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX  108
#endif

enum AmTransferPacketType {
   AM_TRANSFER_PACKET_TYPE_NULL     = 0,
   AM_TRANSFER_PACKET_TYPE_H264     = 1,
   AM_TRANSFER_PACKET_TYPE_MJPEG    = 2,
   AM_TRANSFER_PACKET_TYPE_PCM      = 3,
   AM_TRANSFER_PACKET_TYPE_G711     = 4,
   AM_TRANSFER_PACKET_TYPE_G726_40  = 5,
   AM_TRANSFER_PACKET_TYPE_G726_32  = 6,
   AM_TRANSFER_PACKET_TYPE_G726_24  = 7,
   AM_TRANSFER_PACKET_TYPE_G726_16  = 8,
   AM_TRANSFER_PACKET_TYPE_AAC      = 9,
   AM_TRANSFER_PACKET_TYPE_OPUS     = 10,
   AM_TRANSFER_PACKET_TYPE_BPCM     = 11,
};

enum AmTransferFrameType {
   AM_TRANSFER_FRAME_NULL           = 0,
   AM_TRANSFER_FRAME_IDR_VIDEO      = 1,
   AM_TRANSFER_FRAME_I_VIDEO        = 2,
   AM_TRANSFER_FRAME_P_VIDEO        = 3,
   AM_TRANSFER_FRAME_B_VIDEO        = 4,
   AM_TRANSFER_FRAME_JPEG_STREAM    = 5,
   AM_TRANSFER_FRAME_JPEG_THUMBNAIL = 6,
};

struct AmTransferPacketHeader {
   unsigned long long   dataPTS;
   unsigned long long   seqNum;
   int                  streamId;
   int                  dataLen;
   AmTransferPacketType dataType;
   AmTransferFrameType  frameType;
};

struct AmTransferPacket {
   AmTransferPacketHeader header;
   unsigned char          data[BLOCK_SIZE_MAX];
};

#endif
