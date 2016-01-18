/**
 * ain_mic.h
 *
 * History:
 *    2008/3/12 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

 
#ifndef __AUDIO_CONTROL_H__
#define __AUDIO_CONTROL_H__

#include <alsa/asoundlib.h>

#define VERBOSE 	0

#define FORMAT_DEFAULT      -1
#define FORMAT_RAW		    0
#define FORMAT_VOC		    1
#define FORMAT_WAVE		    2
#define FORMAT_AU		        3
#define FORMAT_ADPCM        4

/* Definitions for Microsoft WAVE format */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	    ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)		        (v)
#define LE_INT(v)		        (v)
#define BE_SHORT(v)	      	    bswap_16(v)
#define BE_INT(v)		        bswap_32(v)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	    ((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)	    	    bswap_16(v)
#define LE_INT(v)	    	    bswap_32(v)
#define BE_SHORT(v)	    	    (v)
#define BE_INT(v)	    	    (v)
#else
#error "Wrong endian"
#endif

#define check_wavefile_space(buffer, len, blimit) \
	if (len > blimit) { \
		blimit = len; \
		if ((buffer = realloc(buffer, blimit)) == NULL) { \
			AM_ERROR("not enough memory");		  \
			return ME_ERROR; \
		} \
	}

#define WAV_RIFF		        COMPOSE_ID('R','I','F','F')
#define WAV_WAVE		        COMPOSE_ID('W','A','V','E')
#define WAV_FMT			    COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		        COMPOSE_ID('d','a','t','a')
#define WAV_PCM_CODE		    1


/* it's in chunks like .voc and AMIGA iff, but my source say there
   are in only in this combination, so I combined them in one header;
   it works on all WAVE-file I have
 */
typedef struct {
    AM_U32 magic;		/* 'RIFF' */
    AM_U32 length;		/* filelen */
    AM_U32 type;		/* 'WAVE' */
} WaveHeader;

typedef struct {
    u_short format;		/* should be 1 for PCM-code */
    u_short modus;		/* 1 Mono, 2 Stereo */
    u_int sample_fq;	/* frequence of sample */
    u_int byte_p_sec;
    u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
    u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
    u_int type;		/* 'data' */
    u_int length;		/* samplecount */
} WaveChunkHeader;

#define AU_MAGIC		    COMPOSE_ID('.','s','n','d')
#define AU_FMT_ULAW		1
#define AU_FMT_LIN8		2
#define AU_FMT_LIN16		3

typedef struct au_header {
    u_int magic;		/* '.snd' */
    u_int hdr_size;		/* size of header (min 24) */
    u_int data_size;	/* size of data */
    u_int encoding;		/* see to AU_FMT_XXXX */
    u_int sample_rate;	/* sample rate */
    u_int channels;		/* number of channels (voices) */
} AuHeader;

//class CamSimpleBufferPool;
class CamAudio;

class CamAudio
{
public:
    CamAudio(): 
        mpHandle(NULL),
        mStream(SND_PCM_STREAM_CAPTURE),
        mPcmFormat(SND_PCM_FORMAT_S16_LE),
        mSampleRate(48000),
        mNumOfChannels(1),
        mChunkSize(0),
        mChunkBytes(0),
        _file_type(FORMAT_DEFAULT),
        _fd(-1),
        _log(NULL)
    {}
    //AM_ERR Construct();
    virtual ~CamAudio();

    AM_UINT GetChunkSize() {
        return (AM_UINT)mChunkSize;
    }

    AM_UINT GetBitsPerFrame() {
        return mBitsPerFrame;
    }

public:
    AM_ERR SetParams(snd_pcm_format_t pcmFormat, AM_UINT sampleRate, AM_UINT numOfChannels);
    AM_ERR ShowHeader(snd_pcm_stream_t stream);
    AM_ERR Wave_Header(AM_U32 cnt);
    AM_ERR Wave_End(void);
    AM_ERR Au_Header(AM_U32 cnt);
    AM_ERR Au_End(void);
    AM_UINT PcmRead(AM_U8*data, AM_INT timeout);

    AM_ERR PcmInit(snd_pcm_stream_t stream);
    AM_ERR PcmDeinit();
    AM_ERR Suspend(void);
    AM_ERR Xrun(snd_pcm_stream_t stream);

private:
    snd_pcm_t *mpHandle;
    snd_pcm_stream_t mStream;
    snd_pcm_format_t mPcmFormat;
    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    snd_pcm_uframes_t mChunkSize;
    AM_UINT  mChunkBytes;
    AM_UINT mBitsPerSample;
    AM_UINT mBitsPerFrame;
    AM_INT  _file_type;
    AM_INT  _fd;
    AM_U64 _fdcount;
    snd_output_t *_log;
};

#endif

