/*******************************************************************************
 * rtcp_structs.h
 *
 * History:
 *   2013-3-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTCP_STRUCTS_H_
#define RTCP_STRUCTS_H_

#include <arpa/inet.h>
/* RTCP Sender Report
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    RC   |   PT=SR=200   |             length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         SSRC of sender                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |              NTP timestamp, most significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             NTP timestamp, least significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         RTP timestamp                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     sender's packet count                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      sender's octet count                     |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

struct RtcpHeader {
    uint8_t  rrcount    :5; /* 0 */
    uint8_t  padding    :1; /* 0 */
    uint8_t  version    :2; /* 2 */
    uint8_t  packettype;    /* SR = 200 */
    uint16_t length;        /* 28 bytes */
    uint32_t ssrc;
    RtcpHeader(uint8_t type = 200) :
      rrcount(0),
      padding(0),
      version(2),
      packettype(type),
      length(htons(sizeof(RtcpHeader))),
      ssrc(0){}
    uint8_t get_rr_count()
    {
      return rrcount;
    }

    void set_packet_type(uint8_t type)
    {
      packettype = type;
    }

    void set_packet_length(uint16_t len)
    {
      length = htons(len);
    }

    void set_ssrc(uint32_t _ssrc)
    {
      ssrc = htonl(_ssrc);
    }

    uint32_t get_ssrc()
    {
      return ntohl(ssrc);
    }
};

struct RtcpSRPayload {
    uint32_t   ntptimeh32;
    uint32_t   ntptimel32;
    uint32_t   rtptimestamp;
    uint32_t   packetcount;
    uint32_t   packetbytes;

    RtcpSRPayload() :
      ntptimeh32(0),
      ntptimel32(0),
      rtptimestamp(0),
      packetcount(0),
      packetbytes(0){}

    void set_ntp_time_h32(uint32_t h32)
    {
      ntptimeh32 = htonl(h32);
    }

    void set_ntp_time_l32(uint32_t l32)
    {
      ntptimel32 = htonl(l32);
    }

    void set_time_stamp(uint32_t ts)
    {
      rtptimestamp = htonl(ts);
    }

    void set_packet_count(uint32_t pc)
    {
      packetcount = htonl(pc);
    }

    void set_packet_bytes(uint32_t pb)
    {
      packetbytes = htonl(pb);
    }

};

/* RTCP Receiver Report
 * |       0       |       1       |       2       |       3       |
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    RC   |   PT=SR=200   |             length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         SSRC of sender                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                 SSRC_1 (SSRC of first source)                 | report
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
 * | fraction lost |       cumulative number of packets lost       |   1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           extended highest sequence number received           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      interarrival jitter                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         last SR (LSR)                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   delay since last SR (DLSR)                  |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

struct RtcpRRPayload {
    uint32_t sourcessrc;
    uint32_t fractionlost   :8;
    uint32_t packetslost    :24;
    uint32_t sequencenum;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;

    uint32_t get_source_ssrc()
    {
      return ntohl(sourcessrc);
    }

    uint8_t get_fraction_lost()
    {
      return (uint8_t)fractionlost;
    }

    uint32_t get_packet_lost()
    {
      return ntohl(packetslost);
    }

    uint32_t get_sequence_number()
    {
      return ntohl(sequencenum);
    }

    uint32_t get_jitter()
    {
      return ntohl(jitter);
    }

    uint32_t get_lsr()
    {
      return ntohl(lsr);
    }

    uint32_t get_dlsr()
    {
      return ntohl(dlsr);
    }
};
#endif /* RTCP_STRUCTS_H_ */
