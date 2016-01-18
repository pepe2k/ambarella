/*******************************************************************************
 * audio_codec_aac.cpp
 *
 * Histroy:
 *   2012-9-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_types.h"
#include "utilities/am_define.h"
#include "am_media_info.h"

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_aac.h"

#define AAC_LIB_ENC_BUFFER_SIZE  106000
#define AAC_LIB_DEC_BUFFER_SIZE  25000

struct AacStatusMsg {
    AM_U32      error;
    const char* message;
    AacStatusMsg(AM_U32 err, const char* msg) :
      error(err)
    {
      message = amstrdup(msg);
    }
    virtual ~AacStatusMsg()
    {
      delete[] message;
    }
};

static AacStatusMsg encoder_status[] =
{
 AacStatusMsg(ENCODE_OK, "OK"),
 AacStatusMsg(ENCODE_INVALID_POINTER, "Invalid Pointer"),
 AacStatusMsg(ENCODE_FAILED, "Encode Failed"),
 AacStatusMsg(ENCODE_UNSUPPORTED_SAMPLE_RATE, "Unsupported Sample Rate"),
 AacStatusMsg(ENCODE_UNSUPPORTED_CH_CFG, "Unsupported Channel Configuration"),
 AacStatusMsg(ENCODE_UNSUPPORTED_BIT_RATE, "Unsupported Bitrate"),
 AacStatusMsg(ENCODE_UNSUPPORTED_MODE, "Unsupported Mode")
};

static AacStatusMsg decoder_status[] =
{
 AacStatusMsg(AAC_DEC_OK, "OK"),
 AacStatusMsg(MAIN_UCI_OUT_OF_MEMORY,
              "MAIN_UCI_OUT_OF_MEMORY"),
 AacStatusMsg(MAIN_UCI_HELP_MODE_ACTIV,
              "MAIN_UCI_HELP_MODE_ACTIV"),
 AacStatusMsg(MAIN_OPEN_BITSTREAM_FILE_FAILED,
              "MAIN_OPEN_BITSTREAM_FILE_FAILED"),
 AacStatusMsg(MAIN_OPEN_16_BIT_PCM_FILE_FAILED,
              "MAIN_OPEN_16_BIT_PCM_FILE_FAILED"),
 AacStatusMsg(MAIN_FRAME_COUNTER_REACHED_STOP_FRAME,
              "MAIN_FRAME_COUNTER_REACHED_STOP_FRAME"),
 AacStatusMsg(MAIN_TERMINATED_BY_ESC,      "MAIN_TERMINATED_BY_ESC"),
 AacStatusMsg(AAC_DEC_ADTS_SYNC_ERROR,     "AAC_DEC_ADTS_SYNC_ERROR"),
 AacStatusMsg(AAC_DEC_LOAS_SYNC_ERROR,     "AAC_DEC_LOAS_SYNC_ERROR"),
 AacStatusMsg(AAC_DEC_ADTS_SYNCWORD_ERROR, "AAC_DEC_ADTS_SYNCWORD_ERROR"),
 AacStatusMsg(AAC_DEC_LOAS_SYNCWORD_ERROR, "AAC_DEC_LOAS_SYNCWORD_ERROR"),
 AacStatusMsg(AAC_DEC_ADIF_SYNCWORD_ERROR, "AAC_DEC_ADIF_SYNCWORD_ERROR"),
 AacStatusMsg(AAC_DEC_UNSUPPORTED_FORMAT,  "AAC_DEC_UNSUPPORTED_FORMAT"),
 AacStatusMsg(AAC_DEC_DECODE_FRAME_ERROR,  "AAC_DEC_DECODE_FRAME_ERROR"),
 AacStatusMsg(AAC_DEC_CRC_CHECK_ERROR,     "AAC_DEC_CRC_CHECK_ERROR"),
 AacStatusMsg(AAC_DEC_INVALID_CODE_BOOK,   "AAC_DEC_INVALID_CODE_BOOK"),
 AacStatusMsg(AAC_DEC_UNSUPPORTED_WINOW_SHAPE,
              "AAC_DEC_UNSUPPORTED_WINOW_SHAPE"),
 AacStatusMsg(AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC,
              "AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC"),
 AacStatusMsg(AAC_DEC_UNIMPLEMENTED_CCE, "AAC_DEC_UNIMPLEMENTED_CCE"),
 AacStatusMsg(AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA,
              "AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA"),
 AacStatusMsg(AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE,
              "AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE"),
 AacStatusMsg(AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE,
              "AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE"),
 AacStatusMsg(AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE,
              "AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE"),
 AacStatusMsg(AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER,
              "AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER"),
 AacStatusMsg(AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS,
              "AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS"),
 AacStatusMsg(AAC_DEC_TNS_RANGE_ERROR,
              "AAC_DEC_TNS_RANGE_ERROR"),
 AacStatusMsg(AAC_DEC_NEED_MORE_DATA,
              "AAC_DEC_NEED_MORE_DATA"),
 AacStatusMsg(AAC_DEC_INSUFFICIENT_BACKUP_MEMORY,
              "AAC_DEC_INSUFFICIENT_BACKUP_MEMORY"),
};

static const char* am_aac_enc_strerror(AM_U32 err)
{
  AM_UINT length = sizeof(encoder_status) / sizeof(AacStatusMsg);
  AM_UINT i = 0;
  for (i = 0; i < length; ++ i) {
    if (AM_LIKELY(encoder_status[i].error == err)) {
      break;
    }
  }

  return (i >= length) ? "Unknown Error" : encoder_status[i].message;
}

static const char* am_aac_dec_strerror(AM_U32 errcode)
{
  AM_UINT length = sizeof(decoder_status) / sizeof(AacStatusMsg);
  AM_UINT i = 0;
  for (i = 0; i < length; ++ i) {
    if (AM_LIKELY(decoder_status[i].error == errcode)) {
      break;
    }
  }

  return (i >= length) ? "Unknown Error" : decoder_status[i].message;
}

/**
 * format convert pcm 32bit to 16bit
 * The function convert 32bit to 16bit interleave PCM format.
 */
/*FIXME: We assume that bufout is big enough to store data in bufin*/
inline void CAudioCodecAac::fc32ito16i(AM_S32 *bufin,
                                       AM_S16 *bufout,
                                       AM_S32 ch,
                                       AM_S32 proc_size)
{
  AM_S32 *bufin_ptr  = bufin;
  AM_S16 *bufout_ptr = bufout;

  for (AM_S32 i = 0; i < proc_size; ++ i) {
    for (AM_S32 j = 0; j < ch; ++ j) {
      *bufout_ptr = (*bufin_ptr) >> 16;
      ++ bufin_ptr;
      ++ bufout_ptr;
    }
  }
}

void CAudioCodecAac::deinterleave(AM_S16 *data, AM_UINT *size, AM_UINT channel)
{
  AM_S16 *wtptr = (channel == 0) ? data + 1 : data;
  AM_S16 *rdptr = (channel == 0) ? data + 2 : data + 1;
  while ((AM_UINT)(rdptr - data) < *size) {
    *wtptr = *rdptr;
    ++ wtptr;
    rdptr += 2;
  }
  *size = wtptr - data;
}

bool CAudioCodecAac::InitCodec(AudioCodecInfo   &codecInfo,
                               AmAudioCodecMode mode)
{
  mAudioInfo = &codecInfo.audio_info;
  if (AM_LIKELY(false == mIsInitialized)) {
    codecInfo.audio_info.format = MF_AAC;
    switch(mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        if (AM_LIKELY(!mEncBuffer)) {
          mEncBuffer = new AM_U8[AAC_LIB_ENC_BUFFER_SIZE];
        }
        if (AM_LIKELY(mEncBuffer)) {
          if (AM_LIKELY(!mConfigEnc)) {
            mConfigEnc = new au_aacenc_config_t;
          }
          if (AM_LIKELY(mConfigEnc)) {
            memset(mConfigEnc, 0, sizeof(au_aacenc_config_t));
            mConfigEnc->sample_freq = mAudioInfo->sampleRate;
            mConfigEnc->Src_numCh = mAudioInfo->channels;
            mConfigEnc->enc_mode = codecInfo.codec_aac.enc_mode;
            mConfigEnc->Out_numCh = codecInfo.codec_aac.out_channel_num;
            mConfigEnc->tns = codecInfo.codec_aac.tns;
            mConfigEnc->ffType = codecInfo.codec_aac.ff_type;
            mConfigEnc->bitRate = codecInfo.codec_aac.bitrate;
            mConfigEnc->quantizerQuality =
                codecInfo.codec_aac.quantizer_quality;
            mConfigEnc->codec_lib_mem_adr = (AM_U32*)mEncBuffer;
            aacenc_setup(mConfigEnc);
            aacenc_open(mConfigEnc);
            mIsInitialized = true;
          } else {
            ERROR("Failed to allocate AAC encode config structure!");
            delete[] mEncBuffer;
            mEncBuffer = NULL;
          }
        } else {
          ERROR("Failed to allocate AAC codec encode buffer!");
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        if (AM_LIKELY(!mDecBuffer)) {
          mDecBuffer = new AM_U32[AAC_LIB_DEC_BUFFER_SIZE];
        }
        if (AM_LIKELY(mDecBuffer)) {
          if (AM_LIKELY(!mConfigDec)) {
            mConfigDec = new au_aacdec_config_t;
          }
          if (AM_LIKELY(mConfigDec)) {
            memset(mConfigDec, 0, sizeof(au_aacdec_config_t));
            if (AM_UNLIKELY(!mDecOutBuf)) {
              mDecOutBuf = new AM_S32[AAC_DEC_OUT_BUF_SIZE];
            }
            if (AM_LIKELY(mDecOutBuf)) {
              mConfigDec->bsFormat = ADTS_BSFORMAT;
              mConfigDec->srcNumCh = mAudioInfo->channels;
              mConfigDec->outNumCh = 2;
              mConfigDec->codec_lib_mem_addr   = mDecBuffer;
              mConfigDec->externalSamplingRate = mAudioInfo->sampleRate;
              /* Reset audio info channels to 2 */
              mAudioInfo->channels = 2;
              mAudioInfo->sampleSize = 2;
              aacdec_setup(mConfigDec);
              aacdec_open(mConfigDec);
              mIsInitialized = true;
            } else {
              ERROR("Failed to allocate AAC decode output buffer!");
              delete[] mConfigDec;
              delete[] mDecBuffer;
              mConfigDec = NULL;
              mDecBuffer = NULL;
            }
          } else {
            ERROR("Failed to allocate AAC decode config structure!");
            delete[] mDecBuffer;
            mDecBuffer = NULL;
          }
        } else {
          ERROR("Failed to allocate AAC codec decode buffer!");
        }
      }break;
      default: {
        ERROR("Invalid AAC codec mode!");
      }break;
    }
  }

  return mIsInitialized;
}

bool CAudioCodecAac::FiniCodec()
{
  if (AM_LIKELY(mIsInitialized)) {
    if (mCodecMode == AM_AUDIO_CODEC_MODE_ENCODE) {
      aacenc_close();
    } else if (mCodecMode == AM_AUDIO_CODEC_MODE_DECODE) {
      aacdec_close();
    }
    mIsInitialized = false;
  }
  return !mIsInitialized;
}

AM_UINT CAudioCodecAac::encode(AM_U8 *input,  AM_UINT inDataSize,
                               AM_U8 *output, AM_UINT *outDataSize)
{
  *outDataSize = 0;
  if (AM_LIKELY(mConfigEnc)) {
    mConfigEnc->enc_rptr = (AM_S32*)input;
    mConfigEnc->enc_wptr = output;
    aacenc_encode(mConfigEnc);
    if (AM_UNLIKELY(mConfigEnc->ErrorStatus)) {
      ERROR("AAC encoding error: %s, encode mode: %d!",
            am_aac_enc_strerror(mConfigEnc->ErrorStatus),
            mConfigEnc->enc_mode);
    } else {
      *outDataSize = (AM_UINT)((mConfigEnc->nBitsInRawDataBlock + 7) >> 3);
    }
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return *outDataSize;
}

AM_UINT CAudioCodecAac::decode(AM_U8 *input,  AM_UINT inDataSize,
                               AM_U8 *output, AM_UINT *outDataSize)
{
  AM_UINT ret = 0;
  if (AM_LIKELY(mConfigDec)) {
    mConfigDec->dec_rptr = input;
    mConfigDec->dec_wptr = mDecOutBuf;
    mConfigDec->interBufSize = inDataSize;
    mConfigDec->consumedByte = 0;
    *outDataSize = 0;
    aacdec_set_bitstream_rp(mConfigDec);
    aacdec_decode(mConfigDec);

    if (AM_UNLIKELY(mConfigDec->ErrorStatus)) {
      ERROR("AAC decoding error: %s, consumed %u bytes!",
            am_aac_dec_strerror((AM_U32)(mConfigDec->ErrorStatus)),
            mConfigDec->consumedByte);
      /* Skip broken data */
      mConfigDec->consumedByte = (mConfigDec->consumedByte == 0) ? inDataSize :
          mConfigDec->consumedByte;
    }
    if (AM_LIKELY(mConfigDec->frameCounter > 0)) {
      fc32ito16i(mConfigDec->dec_wptr,
                 (AM_S16*)output,
                 mConfigDec->outNumCh,
                 mConfigDec->frameSize);
      *outDataSize = mConfigDec->frameSize * sizeof(AM_S32);
      /* This is the sample rate of decoded PCM audio data */
      mAudioInfo->sampleRate = mConfigDec->fs_out;
      mAudioInfo->channels   = mConfigDec->outNumCh;
#if 0
      if (AM_LIKELY(mConfigDec->srcNumCh == 1)) {
        deinterleave((AM_S16*)output, outDataSize, 0);
      }
#endif
    }
    ret = mConfigDec->consumedByte;
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return ret;
}
