/**
 * h264Structs.h
 *
 * History:
 *	2009/12/17 - [Yupeng Chang] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef H264_STRUCTS_H_
#define H264_STRUCTS_H_

/* Generic Nal header
 *
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */

struct header_octet
{
#ifdef BIGENDIAN
    uint8_t f    :1; /* Bit 0     */
    uint8_t nri  :2; /* Bit 1 ~ 2 */
    uint8_t type :5; /* Bit 3 ~ 7 */
#else
    uint8_t type :5; /* Bit 3 ~ 7 */
    uint8_t nri  :2; /* Bit 1 ~ 2 */
    uint8_t f    :1; /* Bit 0     */
#endif
};
typedef struct header_octet nal_header_type;
typedef struct header_octet fu_indicator_type;
typedef struct header_octet stap_header_type;
typedef struct header_octet mtap_header_type;

/*  FU header
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |S|E|R|  Type   |
 *  +---------------+
 */

struct header_fu
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 0     */
    uint8_t e    :1; /* Bit 1     */
    uint8_t r    :1; /* Bit 2     */
    uint8_t type :5; /* Bit 3 ~ 7 */
#else
    uint8_t type :5; /* Bit 3 ~ 7 */
    uint8_t r    :1; /* Bit 2     */
    uint8_t e    :1; /* Bit 1     */
    uint8_t s    :1; /* Bit 0     */
#endif
};
typedef struct header_fu fu_header_type;

struct NALU
{
    uint8_t *addr;
    uint32_t nalu_type;
    uint32_t size;
    uint32_t packetsNum;
};
typedef struct NALU NALU;

enum NALU_TYPE {
  NALUHEAD = 0x01,
  IDRHEAD  = 0x05,
  SEIHEAD  = 0x06,
  SPSHEAD  = 0x07,
  PPSHEAD  = 0x08,
  AUDHEAD  = 0x09,
};

#endif //H264_STRUCTS_H_
