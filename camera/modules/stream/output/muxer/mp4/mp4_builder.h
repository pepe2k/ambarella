/*
 * mp4_builder.h
 *
 * History:
 *    2011/8/1 - [Jay Zhang] created file
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MP4_BUILDER_H__
#define __MP4_BUILDER_H__

#include "adts.h"

#define	MAX_FRAME_NUM_FOR_SINGLE_FILE	245760		// can buffer 8192 sec video data, 4GB for 0.5MBps

struct CMP4MUX_RECORD_INFO {
    char*	dest_name;
    AM_U32 max_filesize;
    AM_U32 max_videocnt;
};

class CMp4Builder
{
    typedef struct {
        AM_INT pic_order_cnt_type;
        AM_INT log2_max_frame_num_minus4;
        AM_INT log2_max_pic_order_cnt_lsb_minus4;
        AM_INT frame_mbs_only_flag;
        AM_INT num_units_in_tick;
        AM_INT time_scale;
    } sps_info_t;

    enum {
      NAL_UNSPECIFIED = 0,
      NAL_NON_IDR,
      NAL_IDR=5,
      NAL_SEI,
      NAL_SPS,
      NAL_PPS,
      NAL_AUD,
    };

    enum{
      SLICE_P_0,
      SLICE_B_0,
      SLICE_I_0,
      SLICE_SP_0,
      SLICE_SI_0,

      SLICE_P_1,
      SLICE_B_1,
      SLICE_I_1,
      SLICE_SP_1,
      SLICE_SI_1,
    };

    enum {max_adts = 8};

  public:
    CMp4Builder(IMp4DataWriter *file);
    virtual ~CMp4Builder();
    void Delete();

    AM_ERR InitProcess();
    void InitH264(AM_VIDEO_INFO *pH264Info);
    void InitAudio(AM_AUDIO_INFO *pAudioInfo);
    AM_ERR put_VideoData(AM_U8* pData, AM_UINT size,
                         AM_PTS pts, CPacket *pUsrSEI);
    AM_ERR put_AudioData(AM_U8* pData, AM_UINT size, AM_U32 frameCount);
    AM_ERR FinishProcess();

  private:
    AM_ERR get_time(AM_UINT* time_since1904, char * time_str,  int len);
    AM_ERR put_byte(AM_UINT data);
    AM_ERR put_be16(AM_UINT data);
    AM_ERR put_be24(AM_UINT data);
    AM_ERR put_be32(AM_UINT data);
    AM_ERR put_be64(AM_U64 data);
    AM_ERR put_buffer(AM_U8 *pdata, AM_UINT size);
    AM_ERR put_boxtype(const char* pdata);
    AM_UINT get_byte();
    void get_buffer(AM_U8 *pdata, AM_UINT size);
    void debug_sei();

    AM_UINT read_bit(AM_U8* pBuffer,  AM_INT* value,
                     AM_U8* bit_pos = NULL, AM_UINT num = 1);
    AM_INT parse_scaling_list(AM_U8* pBuffer,AM_UINT sizeofScalingList,
                              AM_U8* bit_pos);
    AM_UINT parse_exp_codes(AM_U8* pBuffer, AM_INT* value,
                            AM_U8* bit_pos, AM_U8 type);

    AM_UINT	GetOneNalUnit(AM_U8 *pNaluType, AM_U8 *pBuffer , AM_UINT size);
    AM_ERR get_h264_info(unsigned char* pBuffer, int size,
                         AM_VIDEO_INFO* pH264Info);
    AM_ERR get_pic_order(unsigned char* pBuffer, int size,
                         int nal_unit_type, int* pic_order_cnt_lsb);

    AM_ERR UpdateVideoIdx(AM_INT pts, AM_UINT sample_size,
                          AM_UINT chunk_offset, AM_U8 sync_point);
    AM_ERR UpdateAudioIdx(AM_UINT sample_size,
                          AM_UINT chunk_offset);

    AM_ERR put_FileTypeBox();
    AM_ERR put_MediaDataBox();
    AM_ERR put_MovieBox();
    void put_VideoTrackBox(AM_UINT TrackId, AM_UINT Duration);
    void put_VideoMediaBox(AM_UINT Duration);
    void put_VideoMediaInformationBox();
    void put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration);
    void put_AudioMediaBox();
    void put_AudioMediaInformationBox();
    AM_ERR UpdateIdxBox();

    virtual void FeedStreamData(AM_U8* inputBuf,
                                AM_U32 inputBufSize,
                                AM_U32 frameCount);
    virtual AM_BOOL GetOneFrame(AM_U8** ppFrameStart,
                                AM_U32* pFrameSize);
  private:
    void FindAdts (ADTS *adts, AM_U8 *bs);

  private:
    IMp4DataWriter *mpMuxedFile;
    AM_U32 mVideoDuration;
    AM_U32 mAudioDuration;
    AM_U32 mVideoCnt;
    AM_U32 mAudioCnt;
    AM_U32 mStssCnt;

    AM_UINT _v_stsz[MAX_FRAME_NUM_FOR_SINGLE_FILE]; //sample size
    AM_UINT _v_stco[MAX_FRAME_NUM_FOR_SINGLE_FILE]; //chunk_offset
    AM_UINT _a_stsz[MAX_FRAME_NUM_FOR_SINGLE_FILE]; //sample size
    AM_UINT _a_stco[MAX_FRAME_NUM_FOR_SINGLE_FILE]; //chunk_offset
    AM_UINT _ctts[2*MAX_FRAME_NUM_FOR_SINGLE_FILE]; //composition time
    AM_UINT _stts[2*MAX_FRAME_NUM_FOR_SINGLE_FILE]; //decoding time
    AM_U32  _stss[MAX_FRAME_NUM_FOR_SINGLE_FILE];   //sync sample

    AM_INT _last_ctts;

    AM_U32 _create_time;
    AM_U32 _decimal_pts;

    AM_U32 mCurPos;
    AM_U32 _v_stts_pos;
    AM_U32 _v_ctts_pos;
    AM_U32 _v_stsz_pos;
    AM_U32 _v_stco_pos;
    AM_U32 _v_stss_pos;
    AM_U32 _a_stsz_pos;
    AM_U32 _a_stco_pos;

    AM_U32 _mdat_begin_pos;
    AM_U32 _mdat_end_pos;

    AM_PTS _last_video_pts;
    AM_U16 _audio_spec_config;
    AM_VIDEO_INFO mH264Info;
    AM_AUDIO_INFO _audio_info;
    CMP4MUX_RECORD_INFO _record_info;

    AM_U32 _last_idr_num;
    AM_INT _last_p_num;
    AM_INT _last_i_num;
    AM_INT _last_pic_order_cnt;
    AM_U64 _last_idr_pts;

    AM_U8* _pps;
    AM_U8* _sps;
    AM_U32 _pps_size;
    AM_U32 _sps_size;
    sps_info_t mSpsInfo;

    ADTS mAdts[max_adts];
    AM_U32 mCurrAdtsIndex;
    AM_U32 mFrameCount;
};

#endif	//__MP4_BUILDER_H__

