/*******************************************************************************
 * mjpeg_structs.h
 *
 * History:
 *   2013-4-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef MJPEG_STRUCTS_H_
#define MJPEG_STRUCTS_H_

#define MAX_QUANTIZATION_TABLE_LEN (4 * 128)

struct JpegHdr {
  uint8_t typeSpecific;
  uint8_t fregOffset3;
  uint8_t fregOffset2;
  uint8_t fregOffset1;
  uint8_t type;
  uint8_t qfactor;
  uint8_t width;
  uint8_t height;
};

struct JpegQuantizationHdr {
    uint8_t mbz;
    uint8_t precision;
    uint8_t length2;
    uint8_t length1;
};

struct JpegParams {
    uint8_t *data;
    uint32_t len;
    uint32_t offset;
    uint16_t width;
    uint16_t height;
    uint16_t qTableLen;
    uint8_t  precision;
    uint8_t  type;
    uint8_t  qTable[MAX_QUANTIZATION_TABLE_LEN];
};


#endif /* MJPEG_STRUCTS_H_ */
