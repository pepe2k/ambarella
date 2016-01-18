/*******************************************************************************
 * rtp_struct.h
 *
 * History:
 *   2012-11-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTP_STRUCT_H_
#define RTP_STRUCT_H_

#include <arpa/inet.h>
/* RTP Header
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|   CC  |M|      PT     |         sequence number       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            synchronization source (SSRC) identifier           |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

struct RtpHeader {
    uint8_t csrccount   :4;
    uint8_t extension   :1;
    uint8_t padding     :1;
    uint8_t version     :2;

    uint8_t payloadtype :7;
    uint8_t marker      :1;
    uint16_t sequencenumber;
    uint32_t timestamp;
    uint32_t ssrc;

    void set_mark(bool m)
    {
      marker = (m ? 1 : 0);
    }

    void set_type(uint8_t pt)
    {
      payloadtype = pt;
    }

    void set_version(uint8_t ver)
    {
      version = ver;
    }

    void set_seq_num(uint16_t seqnum)
    {
      sequencenumber = htons(seqnum);
    }

    void set_time_stamp(uint32_t ts)
    {
      timestamp = htonl(ts);
    }

    void set_ssrc(uint32_t num)
    {
      ssrc = htonl(num);
    }
};

struct RtspTcpHeader {
    uint8_t  magic_num;
    uint8_t  channel;
    uint16_t data_len;

    void set_magic_num()
    {
      magic_num = 0x24; /* '$' */
    }

    void set_channel_id(uint8_t id)
    {
      channel = id;
    }

    void set_data_length(uint16_t len)
    {
      data_len = htons(len);
    }
};

#define RTP_HEADER_SIZE       12
#define RTP_PAYLOAD_TYPE_JPEG 26
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_OPUS 97
#define RTP_PAYLOAD_TYPE_AAC  98
#define RTP_PAYLOAD_TYPE_G726 99

#endif /* RTP_STRUCT_H_ */
