/*******************************************************************************
 * wav.h
 *
 * History:
 *   2013-6-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef WAV_H_
#define WAV_H_

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT  COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')

struct WavRiffHdr {
    uint32_t chunkId;       /* 'RIFF'       */
    uint32_t chunkSize;     /* FileSize - 8 */
    uint32_t type;          /* 'WAVE'       */
    WavRiffHdr() :
      chunkId(0),
      chunkSize(0),
      type(0){}
    bool isChunkOk() {
      return ((chunkId == WAV_RIFF) && (type == WAV_WAVE));
    }
};

struct WavChunkHdr {
    uint32_t chunkId;
    uint32_t chunkSize;
    WavChunkHdr() :
      chunkId(0),
      chunkSize(0){}
};

struct WavFmtBody {
    uint16_t audioFmt;      /* 1 indicates PCM    */
    uint16_t channels;      /* Mono: 1, Stereo: 2 */
    uint32_t sampleRate;    /* 44100, 48000, etc  */
    uint32_t byteRate;      /* sampleRate * channels * bitsPerSample / 8 */
    uint16_t blockAlign;    /* channels * bitsPerSample / 8 */
    uint16_t bitsPerSample;
};

struct WavFmtHdr: public WavChunkHdr {
    /* ChunkId 'fmt ' */
    WavFmtHdr() :
      WavChunkHdr(){}
    bool isChunkOk() {
      return ((chunkId == WAV_FMT) && (chunkSize >= sizeof(WavFmtBody)));
    }
};

struct WavFmt {
    WavFmtHdr  fmtHeader;
    WavFmtBody fmtBody;
};

struct WavDataHdr: public WavChunkHdr {
    /* ChunkId 'data' */
    WavDataHdr() :
      WavChunkHdr(){}
    bool isChunkOk() {
      return (chunkId == WAV_DATA);
    }
};

struct WavChunkData: public WavChunkHdr {
    char* chunkData;
    WavChunkData() :
      WavChunkHdr(),
      chunkData(NULL){}
    ~WavChunkData()
    {
      delete[] chunkData;
    }
    char* getChunkData(uint32_t size)
    {
      delete[] chunkData;
      if (AM_LIKELY(size)) {
        chunkData = new char[size];
      }
      return chunkData;
    }
    bool isFmtChunk()
    {
      return (chunkId == WAV_FMT);
    }
    bool isDataChunk()
    {
      return (chunkId == WAV_DATA);
    }
};
#endif /* WAV_H_ */
