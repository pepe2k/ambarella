/*
 * mp4_builder.cpp
 *
 * History:
 *	2011/8/1 - [Jay Zhang] created file
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <sys/ioctl.h> //for ioctl
#include <fcntl.h>     //for open O_* flags
#include <unistd.h>    //for read/write/lseek
#include <stdlib.h>    //for malloc/free
#include <string.h>    //for strlen/memset
#include <stdio.h>     //for printf
#include <time.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_muxer_info.h"
#include "am_mw_packet.h"
#include "am_media_info.h"
#include "am_utils.h"
#include "output_record_if.h"


#include "mp4_builder.h"

CMp4Builder::CMp4Builder(IMp4DataWriter *file):
mpMuxedFile(file),
mVideoDuration(0),
mAudioDuration(0),
mVideoCnt(0),
mAudioCnt(0),
mStssCnt(0),
_last_ctts(0),
_create_time(0),
_decimal_pts(0),
mCurPos(0),
_v_stts_pos(0),
_v_ctts_pos(0),
_v_stsz_pos(0),
_v_stco_pos(0),
_v_stss_pos(0),
_a_stsz_pos(0),
_a_stco_pos(0),
_mdat_begin_pos(0),
_mdat_end_pos(0),
_last_video_pts(0),
_audio_spec_config(0xffff),
_last_idr_num(0),
_last_p_num(0),
_last_i_num(0),
_last_pic_order_cnt(0),
_last_idr_pts(0),
_pps(NULL),
_sps(NULL),
_pps_size(0),
_sps_size(0),
mCurrAdtsIndex(0),
mFrameCount(0)
{
  memset(&mH264Info, 0, sizeof(mH264Info));
  memset(&_audio_info, 0, sizeof(_audio_info));
  memset(&_record_info, 0, sizeof(_record_info));
  memset(&mSpsInfo, 0, sizeof(mSpsInfo));

  _record_info.max_filesize = 1<<31;  // 2G
  _record_info.max_videocnt = -1;
}

CMp4Builder::~CMp4Builder()
{
  delete _sps;
  delete _pps;
}

void CMp4Builder::Delete()
{
  delete this;
}

void CMp4Builder::InitH264(AM_VIDEO_INFO* h264_info)
{
  if (h264_info) {
    memcpy(&mH264Info, h264_info, sizeof(AM_VIDEO_INFO));
  } else {
    mH264Info.fps = 0;
    mH264Info.width = 1280;
    mH264Info.height = 720;
    mH264Info.M = 3;
    mH264Info.N = 30;
    mH264Info.rate = 1001;
    mH264Info.scale = 30000;
  }
}

void CMp4Builder::InitAudio(AM_AUDIO_INFO* audio_info)
{
  if (audio_info) {
    memcpy(&_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
  } else { //default value
    _audio_info.sampleRate = 48000;
    _audio_info.chunkSize = 1024;
    _audio_info.sampleSize = 16;
    _audio_info.channels = 2;
  }
}

AM_ERR CMp4Builder::InitProcess()
{
  AM_ENSURE_OK_(put_FileTypeBox());
  AM_ENSURE_OK_(put_MediaDataBox());

  return ME_OK;
}

AM_ERR CMp4Builder::FinishProcess()
{
  timeval start, end, diff;
  gettimeofday(&start, NULL);
  _mdat_end_pos = mCurPos;

  AM_ENSURE_OK_(put_MovieBox());

  /* reset internal varibles, except sps/pps related */
  mCurPos = 0;
  mVideoDuration = 0;
  mAudioDuration = 0;
  mVideoCnt = 0;
  mAudioCnt = 0;
  mStssCnt = 0;
  _last_ctts = 0;

  _decimal_pts = 0;
  _v_stts_pos = 0;
  _v_ctts_pos = 0;
  _v_stsz_pos = 0;
  _v_stco_pos = 0;
  _v_stss_pos = 0;
  _a_stsz_pos = 0;
  _a_stco_pos = 0;

  _mdat_begin_pos = 0;
  _mdat_end_pos = 0;

  _last_video_pts = 0;
  _last_idr_num = 0;
  _last_p_num = 0;
  _last_i_num = 0;
  _last_pic_order_cnt = 0;
  _last_idr_pts = 0;

  gettimeofday(&end, NULL);
  timersub(&end, &start, &diff);
  INFO("Mp4 Finish Process takes %ld ms\n",
       (diff.tv_sec * 1000000 + diff.tv_usec) / 1000);
  return ME_OK;
}

AM_ERR CMp4Builder::get_time(AM_UINT* time_since1904, char * time_str,  int len)
{
  time_t  t;
  struct tm * utc;
  t= time(NULL);
  utc = localtime(&t);
  if (strftime(time_str, len, "%Y%m%d%H%M%S",utc) == 0) {
    AM_ERROR("date string format error \n");
    return ME_ERROR;
  }
  t = mktime(utc);//seconds since 1970-01-01 00:00:00 UTC
  //1904-01-01 00:00:00 UTC -> 1970-01-01 00:00:00 UTC
  //66 years plus 17 days of the 17 leap years [1904, 1908, ..., 1968]
  *time_since1904 = t+66*365*24*60*60+17*24*60*60;
  return ME_OK;
}

//--------------------------------------------------
//big-endian format
inline AM_ERR CMp4Builder::put_byte(AM_UINT data)
{
  AM_U8 w[1];
  w[0] = data;      //(data&0xFF);

  return put_buffer(w, 1);
}

inline AM_ERR CMp4Builder::put_be16(AM_UINT data)
{
  AM_U8 w[2];
  w[1] = data;      //(data&0x00FF);
  w[0] = data>>8;   //(data&0xFF00)>>8;

  return put_buffer(w, 2);
}

inline AM_ERR CMp4Builder::put_be24(AM_UINT data)
{
  AM_U8 w[3];
  w[2] = data;     //(data&0x0000FF);
  w[1] = data>>8;  //(data&0x00FF00)>>8;
  w[0] = data>>16; //(data&0xFF0000)>>16;

  return put_buffer(w, 3);
}

inline AM_ERR CMp4Builder::put_be32(AM_UINT data)
{
  AM_UINT dataBe = LeToBe32(data);
  return put_buffer((AM_U8*)&dataBe, sizeof(data));
}

inline AM_ERR CMp4Builder::put_buffer(AM_U8 *pdata, AM_UINT size)
{
  mCurPos += size;
  return mpMuxedFile->WriteData(pdata, size);
}

AM_ERR CMp4Builder::put_boxtype(const char* pdata)
{
  mCurPos += 4;
  return mpMuxedFile->WriteData((AM_U8 *)pdata, 4);
}

// get the type and length of a nal unit
AM_UINT CMp4Builder::GetOneNalUnit(AM_U8 *pNaluType,
                                   AM_U8 *pBuffer,
                                   AM_UINT size)
{
  AM_UINT code, tmp, pos;
  for (code=0xffffffff, pos = 0; pos <4; pos++) {
    tmp = pBuffer[pos];
    code = (code<<8)|tmp;
  }
  AM_ASSERT(code == 0x00000001); // check start code 0x00000001 (BE)

  *pNaluType = pBuffer[pos++] & 0x1F;
  for (code=0xffffffff; pos < size; pos++) {
    tmp = pBuffer[pos];
    if ((code=(code<<8)|tmp) == 0x00000001) {
      break; //next start code is found
    }
  }
  if (pos == size ) {
    // next start code is not found, this must be the last nalu
    return size;
  } else {
    return pos-4+1;
  }
}

AM_UINT CMp4Builder::read_bit(AM_U8* pBuffer, AM_INT* value,
                              AM_U8* bit_pos, AM_UINT num)
{
  *value = 0;
  AM_UINT  i=0;
  for (AM_UINT j =0 ; j<num; j++) {
    if (*bit_pos == 8) {
      *bit_pos = 0;
      i++;
    }
    if (*bit_pos == 0) {
      if ((pBuffer[i] == 0x3) &&
          (*(pBuffer+i-1) == 0) &&
          (*(pBuffer+i-2) == 0)) {
        i++;
      }
    }
    *value  <<= 1;
    *value  += pBuffer[i] >> (7 -(*bit_pos)++) & 0x1;
  }
  return i;
};

AM_UINT CMp4Builder::parse_exp_codes(AM_U8* pBuffer, AM_INT* value,
                                     AM_U8* bit_pos=0, AM_U8 type=0)
{
  int leadingZeroBits = -1;
  AM_UINT i=0;
  AM_U8 j=*bit_pos;
  for (AM_U8 b = 0; !b; leadingZeroBits ++, j ++) {
    if(j == 8) {
      i++;
      j=0;
    }
    if (j == 0) {
      if ((pBuffer[i] == 0x3) &&
          (*(pBuffer+i-1) == 0) &&
          (*(pBuffer+i-2) == 0)) {
        i++;
      }
    }
    b = pBuffer[i] >> (7 -j) & 0x1;
  }
  AM_INT codeNum = 0;
  i += read_bit(pBuffer+i,  &codeNum, &j, leadingZeroBits);
  codeNum += (1 << leadingZeroBits) -1;
  if (type == 0) { //ue(v)
    *value = codeNum;
  } else if (type == 1) {//se(v)
    *value = (codeNum/2+1);
    if (codeNum %2 == 0) {
      *value *= -1;
    }
  }
  *bit_pos = j;
  return i;
}

#define parse_ue(x,y,z) parse_exp_codes((x),(y),(z),0)
#define parse_se(x,y,z) parse_exp_codes((x),(y),(z),1)

AM_INT CMp4Builder::parse_scaling_list(AM_U8* pBuffer,
                                       AM_UINT sizeofScalingList,
                                       AM_U8* bit_pos)
{
  AM_INT* scalingList = new AM_INT[sizeofScalingList];
  //AM_U8 useDefaultScalingMatrixFlag;
  AM_INT lastScale = 8;
  AM_INT nextScale = 8;
  AM_INT delta_scale;
  AM_INT i = 0;
  for (AM_UINT j = 0; j < sizeofScalingList; ++ j) {
    if (nextScale != 0) {
      i += parse_se(pBuffer, &delta_scale,bit_pos);
      nextScale = (lastScale+ delta_scale + 256)%256;
      //useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
    }
    scalingList[j] = ( nextScale == 0 ) ? lastScale : nextScale;
    lastScale = scalingList[j];
  }
  delete[] scalingList;
  return i;
};

AM_ERR CMp4Builder::get_h264_info(unsigned char* pBuffer,
                                  int size,
                                  AM_VIDEO_INFO* pH264Info)
{
  AM_U8* pSPS = pBuffer;
  AM_U8 bit_pos = 0;
  AM_INT profile_idc; // u(8)
  AM_INT constraint_set; //u(8)
  AM_INT level_idc; //u(8)
  AM_INT seq_paramter_set_id = 0;
  AM_INT chroma_format_idc = 0;
  AM_BOOL residual_colour_tramsform_flag;
  AM_INT bit_depth_luma_minus8 = 0;
  AM_INT bit_depth_chroma_minus8 = 0;
  AM_BOOL qpprime_y_zero_transform_bypass_flag;
  AM_BOOL seq_scaling_matrix_present_flag;
  AM_BOOL seq_scaling_list_present_flag[8] ;
  //AM_INT log2_max_frame_num_minus4;
  //AM_INT pic_order_cnt_type;
  //AM_INT log2_max_pic_order_cnt_lsb_minus4;
  AM_INT num_ref_frames;
  AM_BOOL gaps_in_frame_num_value_allowed_flag;
  AM_INT pic_width_in_mbs_minus1;
  AM_INT pic_height_in_map_units_minus1;
  //AM_BOOL frame_mbs_only_flag;
  AM_BOOL direct_8x8_inference_flag;

  pSPS += read_bit(pSPS, &profile_idc,&bit_pos,8);
  pSPS += read_bit(pSPS, &constraint_set,&bit_pos,8);
  pSPS += read_bit(pSPS, &level_idc, &bit_pos,8);
  pSPS += parse_ue(pSPS,&seq_paramter_set_id, &bit_pos);
  if (profile_idc == 100 ||profile_idc == 110 ||
      profile_idc == 122 ||profile_idc == 144 )
  {
    pSPS += parse_ue(pSPS,&chroma_format_idc,&bit_pos);
    if (chroma_format_idc == 3) {
      pSPS += read_bit(pSPS,
                       (AM_INT *)&residual_colour_tramsform_flag,
                       &bit_pos);
    }
    pSPS += parse_ue(pSPS, &bit_depth_luma_minus8,&bit_pos);
    pSPS += parse_ue(pSPS, &bit_depth_chroma_minus8,&bit_pos);
    pSPS += read_bit(pSPS,
                     (AM_INT *)&qpprime_y_zero_transform_bypass_flag,
                     &bit_pos);
    pSPS += read_bit(pSPS,
                     (AM_INT *)&seq_scaling_matrix_present_flag,
                     &bit_pos);
    if (seq_scaling_matrix_present_flag ) {
      for (AM_UINT i = 0; i < 8; ++ i) {
        pSPS += read_bit(pSPS,
                         (AM_INT *)&seq_scaling_list_present_flag[i],
                         &bit_pos);
        if (seq_scaling_list_present_flag[i]) {
          if (i < 6) {
            pSPS += parse_scaling_list(pSPS,16,&bit_pos);
          } else {
            pSPS += parse_scaling_list(pSPS,64,&bit_pos);
          }
        }
      }
    }
  }
  pSPS += parse_ue(pSPS, &mSpsInfo.log2_max_frame_num_minus4,&bit_pos);
  pSPS += parse_ue(pSPS, &mSpsInfo.pic_order_cnt_type,&bit_pos);
  if (mSpsInfo.pic_order_cnt_type == 0) {
    pSPS += parse_ue(pSPS,&mSpsInfo.log2_max_pic_order_cnt_lsb_minus4,&bit_pos);
  } else if (mSpsInfo.pic_order_cnt_type == 1) {
    AM_BOOL delta_pic_order_always_zero_flag;
    pSPS += read_bit(pSPS,
                     (AM_INT *)&delta_pic_order_always_zero_flag, &bit_pos);
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;
    pSPS += parse_se(pSPS,
                     &offset_for_non_ref_pic, &bit_pos);
    pSPS += parse_se(pSPS, &offset_for_top_to_bottom_field, &bit_pos);
    pSPS += parse_ue(pSPS, &num_ref_frames_in_pic_order_cnt_cycle, &bit_pos);
    AM_INT* offset_for_ref_frame =
        new AM_INT[num_ref_frames_in_pic_order_cnt_cycle];
    for (AM_INT i =0;i < num_ref_frames_in_pic_order_cnt_cycle; i++ ) {
      pSPS += parse_se(pSPS, offset_for_ref_frame + i, &bit_pos);
    }
    delete[] offset_for_ref_frame;
  }
  pSPS += parse_ue(pSPS,&num_ref_frames, &bit_pos);
  pSPS += read_bit(pSPS,
                   (AM_INT *)&gaps_in_frame_num_value_allowed_flag, &bit_pos);
  pSPS += parse_ue(pSPS,&pic_width_in_mbs_minus1,&bit_pos);
  pSPS += parse_ue(pSPS,&pic_height_in_map_units_minus1, &bit_pos);

  pH264Info->width  = (short)(pic_width_in_mbs_minus1 + 1) << 4;
  pH264Info->height = (short)(pic_height_in_map_units_minus1 + 1) <<4;

  pSPS += read_bit(pSPS, &mSpsInfo.frame_mbs_only_flag, &bit_pos);
  if (!mSpsInfo.frame_mbs_only_flag) {
    AM_BOOL mb_adaptive_frame_field_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&mb_adaptive_frame_field_flag, &bit_pos);
    pH264Info->height *= 2;
  }

  pSPS += read_bit(pSPS, (AM_INT *)&direct_8x8_inference_flag, &bit_pos);
  AM_BOOL frame_cropping_flag;
  AM_BOOL vui_parameters_present_flag;
  pSPS += read_bit(pSPS, (AM_INT *)&frame_cropping_flag, &bit_pos);
  if (frame_cropping_flag) {
    AM_INT frame_crop_left_offset;
    AM_INT frame_crop_right_offset;
    AM_INT frame_crop_top_offset;
    AM_INT frame_crop_bottom_offset;
    pSPS += parse_ue(pSPS,&frame_crop_left_offset, &bit_pos);
    pSPS += parse_ue(pSPS,&frame_crop_right_offset, &bit_pos);
    pSPS += parse_ue(pSPS,&frame_crop_top_offset, &bit_pos);
    pSPS += parse_ue(pSPS,&frame_crop_bottom_offset, &bit_pos);
  }
  pSPS += read_bit(pSPS, (AM_INT *)&vui_parameters_present_flag, &bit_pos);

  if (vui_parameters_present_flag) {
    AM_BOOL aspect_ratio_info_present_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&aspect_ratio_info_present_flag, &bit_pos);
    if (aspect_ratio_info_present_flag) {
      AM_INT aspect_ratio_idc;
      pSPS += read_bit(pSPS, &aspect_ratio_idc,&bit_pos,8);
      if (aspect_ratio_idc == 255) {// Extended_SAR
        AM_INT sar_width;
        AM_INT sar_height;
        pSPS += read_bit(pSPS, &sar_width, &bit_pos, 16);
        pSPS += read_bit(pSPS, &sar_height, &bit_pos, 16);
      }
    }
    AM_BOOL overscan_info_present_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&overscan_info_present_flag, &bit_pos);
    if (overscan_info_present_flag) {
      AM_BOOL overscan_appropriate_flag;
      pSPS += read_bit(pSPS, (AM_INT *)&overscan_appropriate_flag, &bit_pos);
    }
    AM_BOOL video_signal_type_present_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&video_signal_type_present_flag, &bit_pos);
    if (video_signal_type_present_flag) {
      AM_INT video_format;
      pSPS += read_bit(pSPS, &video_format, &bit_pos,3);
      AM_BOOL video_full_range_flag;
      pSPS += read_bit(pSPS, (AM_INT *)&video_full_range_flag, &bit_pos);
      AM_BOOL colour_description_present_flag;
      pSPS += read_bit(pSPS,
                       (AM_INT *)&colour_description_present_flag, &bit_pos);
      if (colour_description_present_flag) {
        AM_INT colour_primaries, transfer_characteristics,matrix_coefficients;
        pSPS += read_bit(pSPS, &colour_primaries, &bit_pos, 8);
        pSPS += read_bit(pSPS, &transfer_characteristics, &bit_pos, 8);
        pSPS += read_bit(pSPS, &matrix_coefficients, &bit_pos, 8);
      }
    }

    AM_BOOL chroma_loc_info_present_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&chroma_loc_info_present_flag, &bit_pos);
    if( chroma_loc_info_present_flag ) {
      AM_INT chroma_sample_loc_type_top_field;
      AM_INT chroma_sample_loc_type_bottom_field;
      pSPS += parse_ue(pSPS,&chroma_sample_loc_type_top_field, &bit_pos);
      pSPS += parse_ue(pSPS,&chroma_sample_loc_type_bottom_field, &bit_pos);
    }
    AM_BOOL timing_info_present_flag;
    pSPS += read_bit(pSPS, (AM_INT *)&timing_info_present_flag, &bit_pos);
    if (timing_info_present_flag) {
      //AM_INT num_units_in_tick,time_scale;
      AM_BOOL fixed_frame_rate_flag;
      pSPS += read_bit(pSPS, &mSpsInfo.num_units_in_tick, &bit_pos, 32);
      pSPS += read_bit(pSPS, &mSpsInfo.time_scale, &bit_pos, 32);
      pSPS += read_bit(pSPS, (AM_INT *)&fixed_frame_rate_flag, &bit_pos);
      if (fixed_frame_rate_flag) {
        AM_U8 divisor; //when pic_struct_present_flag == 1 && pic_struct == 0
        //pH264Info->fps = (float)time_scale/num_units_in_tick/divisor;
        if (mSpsInfo.frame_mbs_only_flag) {
          divisor = 2;
        } else {
          divisor = 1; // interlaced
        }
        pH264Info->fps = divisor * mSpsInfo.num_units_in_tick;
        if (mH264Info.rate == 0 && mH264Info.scale ==0) {
          mH264Info.rate = mSpsInfo.num_units_in_tick;
          mH264Info.scale = mSpsInfo.time_scale/2;
        }
      } else { //default value
        pH264Info->fps = 3003;
      }
    }
  } else { //default value
    pH264Info->fps = 3003;
  }

  NOTICE("h264_info %d %d\n", mH264Info.rate, mH264Info.scale);

  if ((pH264Info->width == 0) ||
      (pH264Info->height == 0)  ||
      (pH264Info->fps == 0)) {
    return ME_ERROR;
  } else {
    return ME_OK;
  }
};

AM_ERR CMp4Builder::get_pic_order(unsigned char* pBuffer, int size, int nal_unit_type, int* pic_order_cnt_lsb) {
  AM_U8* pSlice_header = pBuffer;
  AM_U8 bit_pos = 0;
  AM_INT first_mb_in_slice;
  AM_INT slice_type;
  AM_INT pic_parameter_set_id;
  AM_INT frame_num;
  pSlice_header += parse_ue(pSlice_header,&first_mb_in_slice,&bit_pos);
  pSlice_header += parse_ue(pSlice_header,&slice_type,&bit_pos);
  switch (slice_type) {
    case SLICE_P_0:
    case SLICE_P_1:
      if (_last_p_num == -1) {
        _last_p_num = mVideoCnt;
      } else {
        if (mH264Info.M == 0) {
          mH264Info.M = mVideoCnt - _last_p_num;
        }
      }
      break;
    case SLICE_I_0:
    case SLICE_I_1:
      if (_last_i_num == -1) {
        _last_i_num = mVideoCnt;
      } else {
        if (mH264Info.N == 0) {
          mH264Info.N = mVideoCnt - _last_i_num;
        }
      }
      break;
    default:
      break;
  }

  pSlice_header += parse_ue(pSlice_header,&pic_parameter_set_id,&bit_pos);
  pSlice_header += read_bit(pSlice_header,&frame_num,&bit_pos,
                            mSpsInfo.log2_max_frame_num_minus4+4);

  if (!mSpsInfo.frame_mbs_only_flag) {
    AM_BOOL field_pic_flag;
    AM_BOOL bottom_field_flag;
    pSlice_header += read_bit(pSlice_header, (AM_INT *)&field_pic_flag,&bit_pos);
    if (field_pic_flag)
      pSlice_header += read_bit(pSlice_header, (AM_INT *)&bottom_field_flag,&bit_pos);
  }
  if (nal_unit_type == NAL_IDR) {
    AM_INT idr_pic_id;
    pSlice_header += parse_ue(pSlice_header,&idr_pic_id,&bit_pos);
  }
  if( mSpsInfo.pic_order_cnt_type == 0 ) {
    pSlice_header += read_bit(pSlice_header,pic_order_cnt_lsb,&bit_pos,
                              mSpsInfo.log2_max_pic_order_cnt_lsb_minus4+4);
    int max_pic_order_cnt_lsb = 1<<(mSpsInfo.log2_max_pic_order_cnt_lsb_minus4+4);
    if (*pic_order_cnt_lsb > max_pic_order_cnt_lsb/2) {
      *pic_order_cnt_lsb -= max_pic_order_cnt_lsb;
    }
  }
  return ME_OK;
}

#define VideoMediaHeaderBox_SIZE          20
#define DataReferenceBox_SIZE             28
#define DataInformationBox_SIZE           (8+DataReferenceBox_SIZE)
#define AvcConfigurationBox_SIZE          (19+_sps_size+_pps_size)
#define BitrateBox_SIZE                   0// 20
#define VisualSampleEntry_SIZE \
  (86+AvcConfigurationBox_SIZE+BitrateBox_SIZE)
#define VideoSampleDescriptionBox_SIZE    (16+VisualSampleEntry_SIZE)
#define VideoDecodingTimeToSampleBox_SIZE (_decimal_pts? 16+(mVideoCnt<<3):24)
#define CompositionTimeToSampleBox_SIZE   (16+(mVideoCnt<<3))
#define SampleToChunkBox_SIZE             28
#define VideoSampleSizeBox_SIZE           (20+(mVideoCnt<<2))
#define VideoChunkOffsetBox_SIZE          (16+(mVideoCnt<<2))
#define SyncSampleBox_SIZE                (16+(mStssCnt<<2))
#define VideoSampleTableBox_SIZE \
  (8+VideoSampleDescriptionBox_SIZE  + \
   VideoDecodingTimeToSampleBox_SIZE + \
   CompositionTimeToSampleBox_SIZE   + \
   SampleToChunkBox_SIZE             + \
   VideoSampleSizeBox_SIZE           + \
   VideoChunkOffsetBox_SIZE          + \
   SyncSampleBox_SIZE)
#define VideoMediaInformationBox_SIZE \
  (8+VideoMediaHeaderBox_SIZE + \
      DataInformationBox_SIZE + \
      VideoSampleTableBox_SIZE)

//--------------------------------------------------

#define VIDEO_HANDLER_NAME            ("Ambarella AVC")
#define VIDEO_HANDLER_NAME_LEN        strlen(VIDEO_HANDLER_NAME)+1
#define MediaHeaderBox_SIZE           32
#define VideoHandlerReferenceBox_SIZE (32+VIDEO_HANDLER_NAME_LEN) // 46
#define VideoMediaBox_SIZE \
  (8+MediaHeaderBox_SIZE            + \
      VideoHandlerReferenceBox_SIZE + \
      VideoMediaInformationBox_SIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
//--------------------------------------------------

#define ElementaryStreamDescriptorBox_SIZE  50
#define AudioSampleEntry_SIZE     (36+ElementaryStreamDescriptorBox_SIZE) // 86
#define AudioSampleDescriptionBox_SIZE        (16+AudioSampleEntry_SIZE) // 102
#define AudioDecodingTimeToSampleBox_SIZE   24
#define AudioSampleSizeBox_SIZE             (20+(mAudioCnt<<2))
#define AudioChunkOffsetBox_SIZE            (16+(mAudioCnt<<2))
#define AudioSampleTableBox_SIZE            (8+AudioSampleDescriptionBox_SIZE+\
    AudioDecodingTimeToSampleBox_SIZE+\
    SampleToChunkBox_SIZE+\
    AudioSampleSizeBox_SIZE+\
    AudioChunkOffsetBox_SIZE) // 198+A*8
#define SoundMediaHeaderBox_SIZE            16
#define AudioMediaInformationBox_SIZE       (8+SoundMediaHeaderBox_SIZE+\
    DataInformationBox_SIZE+\
    AudioSampleTableBox_SIZE) // 258+A*8

#define AUDIO_HANDLER_NAME             ("Ambarella AAC")
#define AUDIO_HANDLER_NAME_LEN         strlen(AUDIO_HANDLER_NAME)+1
#define AudioHandlerReferenceBox_SIZE  (32+AUDIO_HANDLER_NAME_LEN) // 46
#define AudioMediaBox_SIZE             (8+MediaHeaderBox_SIZE+\
    AudioHandlerReferenceBox_SIZE+\
    AudioMediaInformationBox_SIZE) // 344+A*8

//--------------------------------------------------
#define MovieHeaderBox_SIZE 108
//#define ObjDescrpBox_SIZE 24//33
#define AMBABox_SIZE        32
#define UserDataBox_SIZE    (8+AMBABox_SIZE) // 48

#define TrackHeaderBox_SIZE 92//+12
#define VideoTrackBox_SIZE (8+TrackHeaderBox_SIZE+VideoMediaBox_SIZE)
#define AudioTrackBox_SIZE (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE) // 444+A*8
#define MovieBox_SIZE (8+MovieHeaderBox_SIZE + \
                       UserDataBox_SIZE      + \
                       VideoTrackBox_SIZE    + \
                       ((mAudioDuration == 0) ? 0 : AudioTrackBox_SIZE))

AM_ERR CMp4Builder::UpdateVideoIdx(AM_INT deltaPts, AM_UINT sampleSize,
                                   AM_UINT chunk_offset, AM_U8 sync_point)
{

  //AM_U16 start_code[2];
  //start_code[0] = 0x1;

  _v_stsz[mVideoCnt] = LeToBe32(sampleSize);	//sample size
  _v_stco[mVideoCnt] = LeToBe32(chunk_offset);	//chunk_offset

  AM_INT ctts = 0;
  if (AM_UNLIKELY(mVideoCnt == 0)) {
    ctts = 0;
  } else {
    ctts = _last_ctts + (deltaPts - mH264Info.rate); // delta to dts
    if (AM_UNLIKELY(ctts < 0)) {
      for (AM_INT i = mVideoCnt - 1; i >= 0; i--) { // adjust all previous ctts
        _ctts[2*i + 1] = LeToBe32((BeToLe32(_ctts[2*i + 1]) - ctts));
      }
      ctts = 0;
      mVideoDuration += ctts;
    }
  }
  //DEBUG("delta PTS %d, ctts %d -> %d, mVideoDuration %d\n",
  //      deltaPts, _last_ctts, ctts, mVideoDuration);
  _ctts[2*mVideoCnt] = LeToBe32(1); // sample_count = 1
  _ctts[2*mVideoCnt + 1] = LeToBe32(ctts);
  _last_ctts = ctts;

  mVideoCnt++;
  mVideoDuration += deltaPts;

  if (sync_point) {
    _stss[mStssCnt] = LeToBe32(mVideoCnt);
    mStssCnt++;
  }
  // DEBUG("mVideoDuration %d, cnt %d\n", mVideoDuration, mVideoCnt);
  return ME_OK;
}

inline AM_ERR CMp4Builder::UpdateAudioIdx(AM_UINT sampleSize,
                                          AM_UINT chunk_offset)
{
  _a_stsz[mAudioCnt] = LeToBe32(sampleSize);   //sample size
  _a_stco[mAudioCnt] = LeToBe32(chunk_offset); //chunk_offset

  mAudioDuration += (mAudioCnt != 0 ? _audio_info.pktPtsIncr : _audio_info.pktPtsIncr * 2);
  mAudioCnt ++;
  return ME_OK;
}

#define FileTypeBox_SIZE 24
AM_ERR CMp4Builder::put_FileTypeBox()
{
  PRINT_FUNCTION_NAME;

  AM_ENSURE_OK_(put_be32(FileTypeBox_SIZE)); //uint32 size
  AM_ENSURE_OK_(put_boxtype("ftyp"));
  AM_ENSURE_OK_(put_boxtype("mp42"));        //uint32 major_brand
  AM_ENSURE_OK_(put_be32(0));                //uint32 minor_version
  AM_ENSURE_OK_(put_boxtype("mp42"));        //uint32 compatible_brands[]
  AM_ENSURE_OK_(put_boxtype("isom"));

  return ME_OK;
}

//mp4 file size < 2G, no B frames, 1 chunk has only one sample
AM_ERR CMp4Builder::put_MediaDataBox()
{
  PRINT_FUNCTION_NAME;
  int ret = 0;
  _mdat_begin_pos = mCurPos;
  ret += put_be32(0);            //uint32 size, will be revised in the end
  ret += put_boxtype("mdat");

  return ((ret == 0) ? ME_OK : ME_ERROR);
}

AM_ERR CMp4Builder::put_VideoData(AM_U8* pData, AM_UINT size,
                                  AM_PTS pts,   CPacket* pUsrSEI)
{
  AM_UINT nal_unit_length;
  AM_U8 nal_unit_type;
  AM_UINT cursor = 0;
  AM_UINT chunk_offset = mCurPos;
  AM_U8 sync_point = 0;

  do {
    nal_unit_length = GetOneNalUnit(&nal_unit_type, pData+cursor, size-cursor);
    AM_ASSERT(nal_unit_length > 0 && nal_unit_type > 0);

    if (nal_unit_type == NAL_SPS) { // write sps
      if (_sps == NULL) {
        _sps_size = nal_unit_length -4;
        _sps = new AM_U8[_sps_size];
        memcpy(_sps, pData+cursor+4, _sps_size); //exclude start code 0x00000001
      }
      if ((mH264Info.width == 0) || (mH264Info.height == 0)
          || (mH264Info.fps == 0)) {
        AM_ENSURE_OK_( get_h264_info(pData+cursor+5,
                                     nal_unit_length-5, &mH264Info) );
      }
    } else if (nal_unit_type == NAL_PPS) { // write pps
      if (_pps == NULL) {
        _pps_size = nal_unit_length -4;
        _pps = new AM_U8[_pps_size];
        memcpy(_pps, pData+cursor+4, _pps_size); //exclude start code 0x00000001
      }
    } else {
      /*
        if (pts ==0 &&
          mVideoCnt &&
          (nal_unit_type == NAL_NON_IDR || nal_unit_type == NAL_IDR)) {
        int pic_order_cnt = 0;
        if ( ME_OK != get_pic_order(pData+cursor+5,
                                    nal_unit_length-5,
                                    nal_unit_type, &pic_order_cnt)) {
          return ME_ERROR;
        }
        AM_ASSERT(mSpsInfo.num_units_in_tick && mSpsInfo.time_scale);
        AM_U64 divisor = 1;
        if (mSpsInfo.frame_mbs_only_flag) {
          divisor = 2;
        }
        if (pic_order_cnt == 0) {
          printf("_last_idr_pts %lld, cnt %d, %d\n",
                 _last_idr_pts, mVideoCnt, _last_idr_num);
          pts = _last_idr_pts +
              (mVideoCnt -_last_idr_num) * mSpsInfo.num_units_in_tick;
          _last_idr_pts = pts;
          _last_idr_num = mVideoCnt;
        } else {
          pts = _last_idr_pts +
                1LL * pic_order_cnt * mSpsInfo.num_units_in_tick /divisor ;
        }
        printf("pts %d -> %d\n", org_pts, pts);
      }
      */

      if (nal_unit_type == NAL_IDR) {
        sync_point = 1;
      }

      put_be32(nal_unit_length - 4); // exclude start code length
      put_buffer(pData + cursor + 4, nal_unit_length - 4);// save ES data

      if (nal_unit_type == NAL_AUD && pUsrSEI) {
        AM_UINT length = pUsrSEI->GetDataSize() - 4;// exclude start code length
        put_be32(length);
        put_buffer(pUsrSEI->GetDataPtr() + 4, length);// save ES data
        NOTICE("Get usr SEI %d\n", length);
      }
    }
    cursor += nal_unit_length;
  } while (cursor < size);

  AM_INT delta_pts = ((mVideoCnt == 0) ?
      (90000*mH264Info.rate/mH264Info.scale) : (pts - _last_video_pts));
  _last_video_pts = pts;
  return UpdateVideoIdx(delta_pts, (mCurPos - chunk_offset),
                        chunk_offset, sync_point);
}

AM_ERR CMp4Builder::put_AudioData(AM_U8* pData,
                                  AM_UINT dataSize,
                                  AM_U32 frameCount)
{
  AM_U8 *pFrameStart;
  AM_U32 frameSize;

  FeedStreamData(pData, dataSize, frameCount);

  while (GetOneFrame (&pFrameStart, &frameSize)) {
    AM_UINT   chunk_offset = mCurPos;
    AdtsHeader *adtsHeader = (AdtsHeader*)pFrameStart;
    AM_UINT  header_length = sizeof(AdtsHeader);

    if (AM_UNLIKELY((mCurPos + frameSize + MovieBox_SIZE) >
                     _record_info.max_filesize)) {
      return ME_TOO_MANY;
    }

    if (AM_UNLIKELY(frameSize < 7)) {
      return ME_ERROR;
    }
    if (AM_UNLIKELY(false == adtsHeader->IsSyncWordOk())) {
      return ME_ERROR;
    }
    if (AM_LIKELY(_audio_spec_config == 0xffff)) {
      _audio_spec_config = (((adtsHeader->AacAudioObjectType() << 11) |
                             (adtsHeader->AacFrequencyIndex() << 7)   |
                             (adtsHeader->AacChannelConf() << 3)) & 0xFFF8);
      INFO("Audio Spec Info: 0x%04x", _audio_spec_config);
    }
    if (AM_LIKELY(adtsHeader->AacFrameNumber() == 0)) {
      if (adtsHeader->ProtectionAbsent() == 0) { //adts_error_check
        header_length += 2;
      }
      //adts_fixed_header + adts_variable_header
      put_buffer(pFrameStart + header_length, frameSize - header_length);
    } else {
      //Todo
    }
    UpdateAudioIdx((frameSize - header_length), chunk_offset);
  }

  return ME_OK;
}

AM_ERR CMp4Builder::put_MovieBox()
{
  PRINT_FUNCTION_NAME;
  //MovieBox
  int ret = 0;
  ret += put_be32(MovieBox_SIZE); //uint32 size
  ret += put_boxtype("moov");     //'moov'

  //MovieHeaderBox
  ret += put_be32(MovieHeaderBox_SIZE); //uint32 size
  ret += put_boxtype("mvhd");           //'mvhd'
  ret += put_byte(0);                   //uint8 version
  put_be24(0);                          //bits24 flags
  //uint32 creation_time [version==0] uint64 creation_time [version==1]
  put_be32(_create_time);
  //uint32 modification_time [version==0] uint64 modification_time [version==1]
  put_be32(_create_time);
  put_be32(mH264Info.scale);            //uint32 timescale
  //uint32 duration [version==0] uint64 duration [version==1]
  put_be32(mVideoDuration);
  put_be32(0x00010000);                 //int32 rate
  put_be16(0x0100);                     //int16 volume
  put_be16(0);                          //bits16 reserved
  put_be32(0);                          //uint32 reserved[2]
  put_be32(0);
  put_be32(0x00010000);                 //int32 matrix[9]
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x00010000);
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x40000000);
  put_be32(0);                          //bits32 pre_defined[6]
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(3);                          //uint32 next_track_ID
  /*
  put_be32(ObjDescrpBox_SIZE);          //uint32 size
  put_boxtype("iods");  //'iods'
  put_byte(0);                          //uint8 version
  put_be24(0);                          //bits24 flags
  put_be32(0x10808080);
  put_be32(0x07004FFF );
  put_be32(0xFF0F7FFF);
   */
  //UserDataBox
  put_be32(UserDataBox_SIZE);            //uint32 size
  put_boxtype("udta");                   //'udta'
  //AMBABox
  put_be32(AMBABox_SIZE);                //uint32 size
  put_boxtype("Amba");                   //'Amba'
  put_be16(mH264Info.M);                 //uint16 M
  put_be16(mH264Info.N);                 //uint16 N
  put_be32(mH264Info.scale);             //uint32 scale
  put_be32(mH264Info.rate);              //uint32 rate
  put_be32(_audio_info.sampleRate);      //uint32 audio's samplefreq
  put_be32(_audio_info.channels);        //unit32 audio's channel number
  put_be32(0);                           //reseverd

  if (mVideoDuration) {
    put_VideoTrackBox(1, mVideoDuration);
  }
  if (mAudioDuration) {
    put_AudioTrackBox(2, mAudioDuration);
  }
  UpdateIdxBox();
  return ME_OK;
}

void CMp4Builder::put_VideoTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
  PRINT_FUNCTION_NAME;
  //TrackBox
  put_be32(VideoTrackBox_SIZE);//uint32 size
  put_boxtype("trak");         //'trak'

  //TrackHeaderBox
  put_be32(TrackHeaderBox_SIZE);//uint32 size
  put_boxtype("tkhd");          //'tkhd'
  put_byte(0);                  //uint8 version
  //0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
  put_be24(0x07);               //bits24 flags
  //uint32 creation_time [version==0] uint64 creation_time [version==1]
  put_be32(_create_time);
  //uint32 modification_time [version==0] uint64 modification_time [version==1]
  put_be32(_create_time);
  put_be32(TrackId);            //uint32 track_ID
  put_be32(0);                  //uint32 reserved
  //uint32 duration [version==0] uint64 duration [version==1]
  put_be32(Duration);
  put_be32(0);                  //uint32 reserved[2]
  put_be32(0);
  put_be16(0);                  //int16 layer
  put_be16(0);                  //int16 alternate_group
  put_be16(0x0000);             //int16 volume
  put_be16(0);                  //uint16 reserved
  put_be32(0x00010000);         //int32 matrix[9]
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x00010000);
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x40000000);
  put_be32(mH264Info.width<<16); //uint32 width  //16.16 fixed-point
  put_be32(mH264Info.height<<16);//uint32 height //16.16 fixed-point

  put_VideoMediaBox(Duration);
}

void CMp4Builder::put_VideoMediaBox(AM_UINT Duration)
{
  PRINT_FUNCTION_NAME;
  INFO("Video duration is %lu", mVideoDuration);
  //MediaBox
  put_be32(VideoMediaBox_SIZE); //uint32 size
  put_boxtype("mdia");          //'mdia'

  //MediaHeaderBox
  put_be32(MediaHeaderBox_SIZE); //uint32 size
  put_boxtype("mdhd");           //'mdhd'
  put_byte(0);                   //uint8 version
  put_be24(0);                   //bits24 flags
  //uint32 creation_time [version==0] uint64 creation_time [version==1]
  put_be32(_create_time);
  //uint32 modification_time [version==0] uint64 modification_time [version==1]
  put_be32(_create_time);
  put_be32(mH264Info.scale);     //uint32 timescale
  //uint32 duration [version==0] uint64 duration [version==1]
  put_be32(Duration);
  put_be16(0);                  //bits5 language[3]  //ISO-639-2/T language code
  put_be16(0);                  //uint16 pre_defined

  //HandlerReferenceBox
  put_be32(VideoHandlerReferenceBox_SIZE); //uint32 size
  put_boxtype("hdlr");                     //'hdlr'
  put_byte(0);                             //uint8 version
  put_be24(0);                             //bits24 flags
  put_be32(0);                             //uint32 pre_defined
  put_boxtype("vide");                     //'vide'
  put_be32(0);                             //uint32 reserved[3]
  put_be32(0);
  put_be32(0);
  put_byte(VIDEO_HANDLER_NAME_LEN);   //char name[], name[0] is actual length
  put_buffer((AM_U8 *)VIDEO_HANDLER_NAME, VIDEO_HANDLER_NAME_LEN-1);

  put_VideoMediaInformationBox();
}

void CMp4Builder::put_VideoMediaInformationBox()
{
  PRINT_FUNCTION_NAME;
  //MediaInformationBox
  put_be32(VideoMediaInformationBox_SIZE); //uint32 size
  put_boxtype("minf");                     //'minf'

  //VideoMediaHeaderBox
  put_be32(VideoMediaHeaderBox_SIZE);      //uint32 size
  put_boxtype("vmhd");                     //'vmhd'

  put_byte(0);                             //uint8 version
  //This is a compatibility flag that allows QuickTime to distinguish
  // between movies created with QuickTime 1.0 and newer movies.
  // You should always set this flag to 1, unless you are creating a movie
  // intended for playback using version 1.0 of QuickTime
  put_be24(1); //bits24 flags
  put_be16(0); //uint16 graphicsmode  //0=copy over the existing image
  put_be16(0); //uint16 opcolor[3]	  //(red, green, blue)
  put_be16(0);
  put_be16(0);

  //DataInformationBox
  put_be32(DataInformationBox_SIZE); //uint32 size
  put_boxtype("dinf"); //'dinf'

  //DataReferenceBox
  put_be32(DataReferenceBox_SIZE);   //uint32 size
  put_boxtype("dref");               //'dref'
  put_byte(0); //uint8 version
  put_be24(0); //bits24 flags
  put_be32(1); //uint32 entry_count
  put_be32(12);//uint32 size
  put_boxtype("url");//'url '
  put_byte(0); //uint8 version
  put_be24(1);//bits24 flags 1=media data is in the same file as the MediaBox

  //SampleTableBox
  put_be32(VideoSampleTableBox_SIZE); //uint32 size
  put_boxtype("stbl"); //'stbl'

  //SampleDescriptionBox
  put_be32(VideoSampleDescriptionBox_SIZE); //uint32 size
  put_boxtype("stsd"); //'stsd'
  put_byte(0); //uint8 version
  put_be24(0); //bits24 flags
  put_be32(1); //uint32 entry_count
  //VisualSampleEntry
  put_be32(VisualSampleEntry_SIZE); //uint32 size
  put_boxtype("avc1"); //'avc1'
  put_byte(0); //uint8 reserved[6]
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_be16(1); //uint16 data_reference_index
  put_be16(0); //uint16 pre_defined
  put_be16(0); //uint16 reserved
  put_be32(0); //uint32 pre_defined[3]
  put_be32(0);
  put_be32(0);
  put_be16(mH264Info.width); //uint16 width
  put_be16(mH264Info.height);//uint16 height
  put_be32(0x00480000); //uint32 horizresolution  72dpi
  put_be32(0x00480000); //uint32 vertresolution   72dpi
  put_be32(0); //uint32 reserved
  put_be16(1); //uint16 frame_count
  AM_U8 EncoderName[32]="\012AVC Coding"; //Compressorname
  put_buffer(EncoderName,32);
  put_be16(0x0018); //uint16 depth   //0x0018=images are in colour with no alpha
  put_be16(-1); //int16 pre_defined
  //AvcConfigurationBox
  put_be32(AvcConfigurationBox_SIZE); //uint32 size
  put_boxtype("avcC"); //'avcC'
  put_byte(1); //uint8 configurationVersion
  put_byte(_sps[1]); //uint8 AVCProfileIndication
  put_byte(_sps[2]); //uint8 profile_compatibility
  put_byte(_sps[3]);  //uint8 level
  //uint8 nal_length  //(nal_length&0x03)+1 [reserved:6, lengthSizeMinusOne:2]
  put_byte(0xFF);
  //uint8 sps_count  //sps_count&0x1f [reserved:3, numOfSequenceParameterSets:5]
  put_byte(0xE1);
  //uint16 sps_size    //sequenceParameterSetLength
  put_be16(_sps_size);
  //uint8 sps[sps_size] //sequenceParameterSetNALUnit
  put_buffer(_sps, _sps_size);
  put_byte(1);                 //uint8 pps_count //umOfPictureParameterSets
  put_be16(_pps_size);         //uint16 pps_size //pictureParameterSetLength

  put_buffer(_pps, _pps_size);//uint8 pps[pps_size] //pictureParameterSetNALUnit

  /*
	//BitrateBox
	put_be32(BitrateBox_SIZE);				   //uint32 size
	put_boxtype("btrt");		//'btrt'
	put_be32(0);								 //uint32 buffer_size
	put_be32(0);								 //uint32 max_bitrate
	put_be32(0);								 //uint32 avg_bitrate
   */

  //DecodingTimeToSampleBox  //bits24 flags
  put_be32(VideoDecodingTimeToSampleBox_SIZE); //uint32 size
  put_boxtype("stts"); //'stts'
  put_byte(0); //uint8 version
  put_be24(0); //bits24 flags

  put_be32(1); //uint32 entry_count
  put_be32(mVideoCnt);
  put_be32(mH264Info.rate);

  //CompositionTimeToSampleBox
  put_be32(CompositionTimeToSampleBox_SIZE);   //uint32 size
  put_boxtype("ctts"); //'ctts'
  put_byte(0);         //uint8 version
  put_be24(0);         //bits24 flags
  put_be32(mVideoCnt); //uint32 entry_count
  put_buffer((AM_U8 *)_ctts, mVideoCnt * 2 *sizeof(_ctts[0]));

  //SampleToChunkBox
  put_be32(SampleToChunkBox_SIZE); //uint32 size
  put_boxtype("stsc");             //'stsc'
  put_byte(0); //uint8 version
  put_be24(0); //bits24 flags
  put_be32(1); //uint32 entry_count
  put_be32(1); //uint32 first_chunk
  put_be32(1); //uint32 samples_per_chunk
  put_be32(1); //uint32 sample_description_index

  //SampleSizeBox
  put_be32(VideoSampleSizeBox_SIZE); //uint32 size
  put_boxtype("stsz");               //'stsz'
  put_byte(0);                       //uint8 version
  put_be24(0);                       //bits24 flags
  put_be32(0);                       //uint32 sampleSize
  put_be32(mVideoCnt);               //uint32 sample_count
  put_buffer((AM_U8 *)_v_stsz, mVideoCnt * sizeof(_v_stsz[0]));

  //ChunkOffsetBox
  put_be32(VideoChunkOffsetBox_SIZE); //uint32 size
  put_boxtype("stco");                //'stco'
  put_byte(0);                        //uint8 version
  put_be24(0);                        //bits24 flags
  put_be32(mVideoCnt);                //uint32 entry_count
  put_buffer((AM_U8 *)_v_stco, mVideoCnt * sizeof(_v_stco[0]));

  //SyncSampleBox
  put_be32(SyncSampleBox_SIZE);       //uint32 size
  put_boxtype("stss");                //'stss'
  put_byte(0);                        //uint8 version
  put_be24(0);                        //bits24 flags
  put_be32(mStssCnt);                 //uint32 entry_count
  put_buffer((AM_U8*)_stss, mStssCnt*sizeof(_stss[0]));

}

AM_ERR CMp4Builder::UpdateIdxBox()
{
  PRINT_FUNCTION_NAME;
  mpMuxedFile->SeekData(_mdat_begin_pos, SEEK_SET);
  mCurPos = _mdat_begin_pos;
  put_be32(_mdat_end_pos - _mdat_begin_pos);
  return ME_OK;
}

void CMp4Builder::put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
  PRINT_FUNCTION_NAME;
  //TrackBox
  put_be32(AudioTrackBox_SIZE);          //uint32 size
  put_boxtype("trak");                   //'trak'

  //TrackHeaderBox
  put_be32(TrackHeaderBox_SIZE);         //uint32 size
  put_boxtype("tkhd");
  put_byte(0);                           //uint8 version
  //0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
  put_be24(0x07);                        //bits24 flags
  //uint32 creation_time [version==0] uint64 creation_time [version==1]
  put_be32(_create_time);
  //uint32 modification_time [version==0] uint64 modification_time [version==1]
  put_be32(_create_time);
  put_be32(TrackId);                     //uint32 track_ID
  put_be32(0);                           //uint32 reserved
  //uint32 duration [version==0] uint64 duration [version==1]
  put_be32(Duration);
  put_be32(0);                           //uint32 reserved[2]
  put_be32(0);
  put_be16(0);                           //int16 layer
  put_be16(0);                           //int16 alternate_group
  put_be16(0x0100);                      //int16 volume
  put_be16(0);                           //uint16 reserved
  put_be32(0x00010000);                  //int32 matrix[9]
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x00010000);
  put_be32(0);
  put_be32(0);
  put_be32(0);
  put_be32(0x40000000);
  put_be32(0);                           //uint32 width  //16.16 fixed-point
  put_be32(0);                           //uint32 height //16.16 fixed-point

  put_AudioMediaBox();
}

void CMp4Builder::put_AudioMediaBox()
{
  PRINT_FUNCTION_NAME;
  //MediaBox
  put_be32(AudioMediaBox_SIZE);               //uint32 size
  put_boxtype("mdia");                        //'mdia'

  //MediaHeaderBox
  put_be32(MediaHeaderBox_SIZE);              //uint32 size
  put_boxtype("mdhd");                        //'mdhd'
  put_byte(0);                                //uint8 version
  put_be24(0);                                //bits24 flags
  //uint32 creation_time [version==0] uint64 creation_time [version==1]
  put_be32(_create_time);
  //uint32 modification_time [version==0] uint64 modification_time [version==1]
  put_be32(_create_time);
  //Audio's timescale is the same as Video, 90000
  put_be32(mH264Info.scale);                  //uint32 timescale
  //uint32 duration [version==0] uint64 duration [version==1]
  put_be32(mAudioDuration);
  put_be16(0);                  //bits5 language[3]  //ISO-639-2/T language code
  put_be16(0);                                //uint16 pre_defined

  //HandlerReferenceBox
  put_be32(AudioHandlerReferenceBox_SIZE);    //uint32 size
  put_boxtype("hdlr");	//'hdlr'
  put_byte(0);                                //uint8 version
  put_be24(0);                                //bits24 flags
  put_be32(0);                                //uint32 pre_defined
  put_boxtype("soun");	//'soun':audio track
  put_be32(0);                                //uint32 reserved[3]
  put_be32(0);
  put_be32(0);
  //char name[], name[0] is actual length
  put_byte(AUDIO_HANDLER_NAME_LEN);
  put_buffer((AM_U8 *)AUDIO_HANDLER_NAME, AUDIO_HANDLER_NAME_LEN-1);

  put_AudioMediaInformationBox();
}

void CMp4Builder::put_AudioMediaInformationBox()
{
  PRINT_FUNCTION_NAME;
  //MediaInformationBox
  put_be32(AudioMediaInformationBox_SIZE);     //uint32 size
  put_boxtype("minf");                         //'minf'

  //SoundMediaHeaderBox
  put_be32(SoundMediaHeaderBox_SIZE);          //uint32 size
  put_boxtype("smhd");                         //'smhd'
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  put_be16(0);                                 //int16 balance
  put_be16(0);                                 //uint16 reserved

  //DataInformationBox
  put_be32(DataInformationBox_SIZE);           //uint32 size
  put_boxtype("dinf");                         //'dinf'
  //DataReferenceBox
  put_be32(DataReferenceBox_SIZE);             //uint32 size
  put_boxtype("dref");                         //'dref'
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  put_be32(1);                                 //uint32 entry_count
  put_be32(12);                                //uint32 size
  put_boxtype("url");                          //'url '
  put_byte(0);                                 //uint8 version
  //1=media data is in the same file as the MediaBox
  put_be24(1);                                 //bits24 flags

  //SampleTableBox
  put_be32(AudioSampleTableBox_SIZE);          //uint32 size
  put_boxtype("stbl");        //'stbl'

  //SampleDescriptionBox
  put_be32(AudioSampleDescriptionBox_SIZE);   //uint32 size
  put_boxtype("stsd");                        //uint32 type
  put_byte(0);                                //uint8 version
  put_be24(0);                                //bits24 flags
  put_be32(1);                                //uint32 entry_count
  //AudioSampleEntry
  put_be32(AudioSampleEntry_SIZE);            //uint32 size
  put_boxtype("mp4a");                        //'mp4a'
  put_byte(0);                                //uint8 reserved[6]
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_byte(0);
  put_be16(1);                                 //uint16 data_reference_index
  put_be32(0);                                 //uint32 reserved[2]
  put_be32(0);
  put_be16(_audio_info.channels);              //uint16 channelcount
  put_be16(_audio_info.sampleSize * 8);        //uint16 samplesize
  //for QT sound
  put_be16(0xfffe);                            //uint16 pre_defined
  put_be16(0);                                 //uint16 reserved
  //= (timescale of media << 16)
  put_be32(_audio_info.sampleRate<<16);        //uint32 samplerate

  //ElementaryStreamDescriptorBox
  put_be32(ElementaryStreamDescriptorBox_SIZE);//uint32 size
  put_boxtype("esds");                         //'esds'
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  //ES descriptor takes 38 bytes
  put_byte(3);                                 //ES descriptor type tag
  put_be16(0x8080);
  put_byte(34);                                //descriptor type length
  put_be16(0);                                 //ES ID
  put_byte(0);                                 //stream priority
  //Decoder config descriptor takes 26 bytes (include decoder specific info)
  put_byte(4);              //decoder config descriptor type tag
  put_be16(0x8080);
  put_byte(22);                                //descriptor type length
  put_byte(0x40); //object type ID MPEG-4 audio=64 AAC
  //stream type:6, upstream flag:1, reserved flag:1 (audio=5)    Audio stream
  put_byte(0x15);
  put_be24(8192);                              // buffer size
  put_be32(128000);                            // max bitrate
  put_be32(128000);                            // avg bitrate
  //Decoder specific info descriptor takes 9 bytes
  //decoder specific descriptor type tag
  put_byte(5);
  put_be16(0x8080);
  put_byte(5);                                 //descriptor type length
  put_be16((AM_UINT)_audio_spec_config);
  put_be16(0x0000);
  put_byte(0x00);
  //SL descriptor takes 5 bytes
  put_byte(6);                                 //SL config descriptor type tag
  put_be16(0x8080);
  put_byte(1);                                 //descriptor type length
  put_byte(2);                                 //SL value

  //DecodingTimeToSampleBox
  put_be32(AudioDecodingTimeToSampleBox_SIZE);//uint32 size
  put_boxtype("stts");                        //'stts'
  put_byte(0);                                //uint8 version
  put_be24(0);                                //bits24 flags
  put_be32(1);                                //uint32 entry_count
  put_be32(mAudioCnt);                        //uint32 sample_count
  put_be32(_audio_info.pktPtsIncr);           //uint32 sample_delta

  //SampleToChunkBox
  put_be32(SampleToChunkBox_SIZE);             //uint32 size
  put_boxtype("stsc");                         //'stsc'
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  put_be32(1);                                 //uint32 entry_count
  put_be32(1);                                 //uint32 first_chunk
  put_be32(1);                                 //uint32 samples_per_chunk
  put_be32(1);                                 //uint32 sample_description_index

  //SampleSizeBox
  put_be32(AudioSampleSizeBox_SIZE);           //uint32 size
  put_boxtype("stsz");                         //'stsz'
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  put_be32(0);                                 //uint32 sampleSize
  put_be32(mAudioCnt);                         //uint32 sample_count
  put_buffer((AM_U8 *)_a_stsz, mAudioCnt*sizeof(_a_stsz[0]));

  //ChunkOffsetBox
  put_be32(AudioChunkOffsetBox_SIZE);          //uint32 size
  put_boxtype("stco");
  put_byte(0);                                 //uint8 version
  put_be24(0);                                 //bits24 flags
  put_be32(mAudioCnt);                         //uint32 entry_count
  put_buffer((AM_U8 *)_a_stco, mAudioCnt * sizeof(_a_stco[0]));
}

void CMp4Builder::FindAdts (ADTS *adts, AM_U8 *bs)
{
  //AM_U8 *bs ;
  for (AM_UINT i = 0; i < mFrameCount; ++ i) {
    adts[i].addr = bs;
    adts[i].size = ((AdtsHeader*)bs)->FrameLength();
    bs += adts[i].size;
  }
}

void CMp4Builder::FeedStreamData(AM_U8 *inputBuf,
                                 AM_U32 inputBufSize,
                                 AM_U32 frameCount)
{
  mFrameCount =  frameCount;
  FindAdts(mAdts, inputBuf);
  mCurrAdtsIndex = 0;
}

AM_BOOL CMp4Builder::GetOneFrame(AM_U8 **ppFrameStart,
                                 AM_U32 *pFrameSize)
{
  if (mCurrAdtsIndex >= mFrameCount) {
    return false;
  }
  *ppFrameStart  = mAdts[mCurrAdtsIndex].addr;
  *pFrameSize    = mAdts[mCurrAdtsIndex].size;
  ++ mCurrAdtsIndex;

  return true;
}

