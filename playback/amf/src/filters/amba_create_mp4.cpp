/*
 * create_mp4.cpp
 *
 * History:
 *	2009/5/26 - [Jacky Zhang] created file
 *	2011/2/23 - [ Yi Zhu] modified
 *  2011/5/12 - [Hanbo Xiao] modified
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
//#define LOG_NDEBUG 0
//#define LOG_TAG "amba_create_mp4"
//#define AMDROID_DEBUG
 
#include <sys/ioctl.h> //for ioctl
#include <fcntl.h>     //for open O_* flags
#include <unistd.h>    //for read/write/lseek
#include <stdlib.h>    //for malloc/free
#include <string.h>    //for strlen/memset
#include <stdio.h>     //for printf
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "am_record_if.h"
#include "am_sink.h"

#include <basetypes.h>
#include "iav_drv.h"
#include "amba_create_mp4.h"

#if ENABLE_AMBA_AAC==true
	#define AMBA_AAC 1
#else
	#define AMBA_AAC 0
#endif

/*
 * Three macroes are defined here using for debug.
 * 
 * RECORD_VIDEO_FILE is used to decide whether video info recorded 
 * from sensor need to be written to a file or not.
 *
 * Usage of RECORD_AUDIO_FILE is similar to RECORD_VIDEO_FILE.
 *
 * ZERO_NAL_UNIT_TYPE is used to decide whether a video frame 
 * whose nal unit's type is zero needs to be written to a file
 * or not.
 */
#define RECORD_VIDEO_FILE
#define RECORD_AUDIO_FILE
#define ZERO_NAL_UNIT_TYPE

iav_h264_config_t *CCreateMp4::_p_h264_config = NULL;

CCreateMp4::CCreateMp4(char* name, AM_INT stream_id):_init(AM_FALSE)
	,_video_cnt(0)
	,_audio_cnt(0)
	,_pps(NULL)
	,_sps(NULL)
	,_decimal_pts(0) {
	//,_pFinishThread(NULL){
	char stream_name = 'A'+ stream_id;
	sprintf(_fileName, "%s_%c",name,stream_name);
	_pSink = new CFileSink();
	_pIdxSink = new CFileSink();

#ifdef RECORD_VIDEO_FILE
	_pVSink = new CFileSink ();
#endif

#ifdef RECORD_AUDIO_FILE
	_pASink = new CFileSink ();
#endif
	
#ifdef ZERO_NAL_UNIT_TYPE
	_pNSink = new CFileSink ();
#endif
}

CCreateMp4::~CCreateMp4() {
	Finish();
	if (_p_fd > 0) {
		::close (_p_fd);
	}
	delete _pSink;
	delete _pIdxSink;

#ifdef RECORD_VIDEO_FILE
	delete _pVSink;
#endif

#ifdef RECORD_VIDEO_FILE
	delete _pASink;
#endif

#ifdef ZERO_NAL_UNIT_TYPE
	delete _pNSink;
#endif
}

void CCreateMp4::Delete()
{
	delete this;
}

AM_ERR CCreateMp4::set_h264_config(iav_h264_config_t *_p_h264)
{
	if (_p_h264_config == NULL) {
		_p_h264_config = (iav_h264_config_t *)malloc (sizeof (iav_h264_config_t));
		if (_p_h264_config == NULL) {
			return ME_NO_MEMORY;
		}
		memcpy (_p_h264_config, _p_h264, sizeof (iav_h264_config_t));
	}
	return ME_OK;
}

void CCreateMp4::destory_h264_config()
{
	if (_p_h264_config != NULL) {
		free (_p_h264_config);
	}
}

AM_ERR CCreateMp4::Init(CMP4MUX_H264_INFO* h264_info,
	CMP4MUX_AUDIO_INFO* audio_info,
	CMP4MUX_RECORD_INFO* record_info) {
	AM_ERR ret = ME_OK;
	_video_cnt = 0;
	_video_duration = 0;
	_audio_cnt = 0;
	_audio_duration = 0;
	_stss_cnt = 0;
	_pps_size = 0;
	_sps_size = 0;
	_v_stts_pos = 0;
	_v_ctts_pos = 0;
	_v_stsz_pos = 0;
	_v_stco_pos = 0;
	_v_stss_pos = 0;
	_curPos = 0;
	_decimal_pts = 0;

	_a_stsz_pos = 0;
	_a_stco_pos = 0;
	_last_pts = 0;
	_h264_info.fps = 0;
	_h264_info.width = 0;
	_h264_info.height = 0;

	if (audio_info) {
		_audio_info.sample_freq = audio_info->sample_freq;
		_audio_info.chunk_size = audio_info->chunk_size;
		_audio_info.sample_size = audio_info->sample_size;
		_audio_info.channels = audio_info->channels;
	} else { //default value
		_audio_info.sample_freq = 48000;
		_audio_info.chunk_size = 1024;
		_audio_info.sample_size = 16;
		_audio_info.channels = 2;
	}

	_h264_info.fps = 0;
	_h264_info.width = 0;
	_h264_info.height = 0;
	if (h264_info) {
		_h264_info.fps = h264_info->fps;
		_h264_info.width = h264_info->width;
		_h264_info.height = h264_info->height;
	}

	_record_info.max_filesize = 1<<31;  // 2G
	_record_info.max_videocnt = -1;
	if (record_info) {
		if (record_info->max_filesize) {
			_record_info.max_filesize = record_info->max_filesize;
		}
		if (record_info->max_videocnt) {
			_record_info.max_videocnt = record_info->max_videocnt;
		}
	}

	char cur_time[64];
	if (ME_OK != get_time(&_create_time,cur_time, sizeof(cur_time))) {
		AM_ERROR("get current time error\n");
	}
	char mp4_fileName[256];
	sprintf(mp4_fileName, "%s_%s.mp4", _fileName, cur_time);

	char tmp_idx_file[256];
	//sprintf(tmp_idx_file,"%s.idx", _fileName);
	// ret = _pIdxSink->Open(NULL);
	strcpy(tmp_idx_file, mp4_fileName);

	int str_len = strlen (tmp_idx_file);
	tmp_idx_file[str_len - 1] = 'x';
	tmp_idx_file[str_len - 2] = 'd';
	tmp_idx_file[str_len - 3] = 'i';
	AM_INFO("idx file name: %s\n", tmp_idx_file);
	
#ifdef RECORD_VIDEO_FILE
	char tmp_video_file[256];
	strcpy (tmp_video_file, mp4_fileName);
	tmp_video_file[str_len - 2] = '6';
	tmp_video_file[str_len - 3] = '2';
	AM_INFO("264 file name: %s\n", tmp_video_file);
	if ((ret = _pVSink->Open (tmp_video_file)) != ME_OK) {
		return ret;
	}
#endif

#ifdef RECORD_AUDIO_FILE
	char tmp_audio_file[256];
	strcpy (tmp_audio_file, mp4_fileName);
	tmp_audio_file[str_len - 1] = 'c';
	tmp_audio_file[str_len - 2] = 'a';
	tmp_audio_file[str_len - 3] = 'a';
	AM_INFO("aac file name: %s\n", tmp_audio_file);
	if ((ret = _pASink->Open (tmp_audio_file)) != ME_OK) {
		return ret;
	}
#endif

#ifdef ZERO_NAL_UNIT_TYPE 
	char tmp_nal_file[256];
	strcpy (tmp_nal_file, mp4_fileName);
	tmp_nal_file[str_len - 1] = 'l';
	tmp_nal_file[str_len - 2] = 'a';
	tmp_nal_file[str_len - 3] = 'n';
	AM_INFO("zero_nal file name: %s\n", tmp_nal_file);
	if ((ret = _pNSink->Open (tmp_nal_file)) != ME_OK) {
		return ret;
	}
#endif

	ret = _pIdxSink->Open(tmp_idx_file);
	if (ret != ME_OK) {
		return ret;
	}

	ret = _pSink->Open(mp4_fileName);
	if (ret != ME_OK) {
		return ret;
	}

	put_FileTypeBox();
	if (ret != ME_OK) {
		return ret;
	}
	put_MediaDataBox();
	_init = AM_TRUE;
	return ret;
}

AM_ERR CCreateMp4::Finish() {
	return FinishProcess();
}

AM_ERR  CCreateMp4::FinishThreadEntry(void *p){
	if ( ME_OK == ((CCreateMp4*)p)->FinishProcess()) {
		((CCreateMp4*)p)->Delete();
		return ME_OK;
	}
	return ME_ERROR;
}

AM_ERR CCreateMp4::FinishProcess() {
	if (_init == AM_FALSE) { 
		AM_INFO("FinishProcess have been called befored.\n");
		return ME_OK;
	}
	AM_INFO("Enter to FinishProcess.\n");
	_mdat_end_pos = _curPos;
	AM_INFO("begin to call put_MovieBox(). \n");
	put_MovieBox();
	AM_INFO("put_MovieBox is called completely.\n ");

	AM_INFO("Free memory allocated fro _sps and _pps.\n ");
	if (_sps) {
		delete _sps;
		_sps = NULL;
	}

	if (_pps) {
		delete _pps;
		_pps = NULL;
	}

	AM_INFO("Begin to close two file descriptor: _pSink and _pIdxSink.\n");
	_pSink->Close();
	_pIdxSink->Close();
	AM_INFO("Close two file descritpor is successful.\n");

#ifdef RECORD_VIDEO_FILE
	_pVSink->Close ();
#endif

#ifdef RECORD_AUDIO_FILE
	_pASink->Close ();
#endif

#ifdef ZERO_NAL_UNIT_TYPE
	_pNSink->Close ();
#endif

	_init = AM_FALSE;
	return ME_OK;
}


AM_ERR CCreateMp4::get_time(AM_UINT* time_since1904, char * time_str,  int len)
{
	time_t  t;
	struct tm * utc;
	t= time(NULL);
	utc = localtime(&t);
	if (strftime(time_str, len, "%m%d%H%M%S",utc) == 0) {
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
AM_ERR CCreateMp4::put_byte(AM_UINT data)
{
	AM_U8 w[1];
	w[0] = data;      //(data&0xFF);
	_curPos++;
	if (1 == _pSink->Write((AM_U8 *)&w, 1))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_ERR CCreateMp4::put_be16(AM_UINT data)
{
	AM_U8 w[2];
	w[1] = data;      //(data&0x00FF);
	w[0] = data>>8;   //(data&0xFF00)>>8;
	_curPos += 2;
	if (2 == _pSink->Write((AM_U8 *)&w, 2))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_ERR CCreateMp4::put_be24(AM_UINT data)
{
	AM_U8 w[3];
	w[2] = data;     //(data&0x0000FF);
	w[1] = data>>8;  //(data&0x00FF00)>>8;
	w[0] = data>>16; //(data&0xFF0000)>>16;
	_curPos += 3;
	if (3 == _pSink->Write((AM_U8 *)&w, 3))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_ERR CCreateMp4::put_be32(AM_UINT data)
{
	AM_U8 w[4];
	w[3] = data;     //(data&0x000000FF);
	w[2] = data>>8;  //(data&0x0000FF00)>>8;
	w[1] = data>>16; //(data&0x00FF0000)>>16;
	w[0] = data>>24; //(data&0xFF000000)>>24;
	_curPos += 4;
	if (4 == _pSink->Write((AM_U8 *)&w, 4))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_ERR CCreateMp4::put_buffer(AM_U8 *pdata, AM_UINT size)
{
	_curPos += size;
	if (size == (AM_UINT)_pSink->Write(pdata, size))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_ERR CCreateMp4::put_boxtype(const char* pdata)
{
	_curPos += 4;
	if (4 == _pSink->Write((AM_U8 *)pdata, 4))
		return ME_OK;
	else
		return ME_ERROR;
}

AM_UINT CCreateMp4::get_byte()
{
	AM_U8 data[1];
	_pSink->Read(data, 1);
	return data[0];
}

AM_UINT CCreateMp4::get_be16()
{
	AM_U8 data[2];
	_pSink->Read(data, 2);
	return (data[1] | (data[0]<<8));
}

AM_UINT CCreateMp4::get_be32()
{
	AM_U8 data[4];
	_pSink->Read(data, 4);
	return (data[3] | (data[2]<<8) | (data[1]<<16) | (data[0]<<24));
}

void CCreateMp4::get_buffer(AM_U8 *pdata, AM_UINT size)
{
	_pSink->Read(pdata, size);
}

AM_UINT CCreateMp4::le_to_be32(AM_UINT data)
{
	AM_UINT rval;
	rval  = (data&0x000000FF)<<24;
	rval += (data&0x0000FF00)<<8;
	rval += (data&0x00FF0000)>>8;
	rval += (data&0xFF000000)>>24;
	return rval;
}

// get the type and length of a nal unit
AM_UINT CCreateMp4::get_nal_unit(AM_U8* nal_unit_type, AM_U8* pBuffer , AM_UINT size)
{
	if (size < 5) {
		return 0;
	}

	AM_UINT code, tmp, pos;
	for (code=0xffffffff, pos = 0; pos <4; pos++) {
		tmp = pBuffer[pos];
		code = (code<<8)|tmp;
	}

	AM_INFO("In get_nal_unit, code = 0x%08x\n", code);
	if (code != 0x00000001) // check start code 0x00000001 
	{
		return 0;
	}

	*nal_unit_type = pBuffer[pos++] & 0x1F;
	AM_INFO("Get nal_unit_type, nal_unit_type = %d\n", *nal_unit_type);
	for (code=0xffffffff; pos < size; pos++) {
		tmp = pBuffer[pos];
		if ((code=(code<<8)|tmp) == 0x00000001) {
				break;//found next start code
		}
	}
	AM_INFO("pos = %d, code = 0x%08x\n", pos, code);
	
	if (pos == size ) {
		return size;
	} else {
		return pos - 4 + 1;
	}
}

AM_UINT CCreateMp4::read_bit(AM_U8* pBuffer,  AM_INT* value, AM_U8* bit_pos=0, AM_UINT num = 1)
{
	*value = 0;
	AM_UINT  i=0;
	for (AM_UINT j =0 ; j<num; j++)
	{
		if (*bit_pos == 8)
		{
			*bit_pos = 0;
			i++;
		}
		if (*bit_pos == 0)
		{
			if (pBuffer[i] == 0x3 &&
				*(pBuffer+i-1) == 0 &&
				*(pBuffer+i-2) == 0)
				i++;
		}
		*value  <<= 1;
		*value  += pBuffer[i] >> (7 -(*bit_pos)++) & 0x1;
	}
	return i;
};

AM_UINT CCreateMp4::parse_exp_codes(AM_U8* pBuffer, AM_INT* value,  AM_U8* bit_pos=0,AM_U8 type=0)
{
	int leadingZeroBits = -1;
	AM_UINT i=0;
	AM_U8 j=*bit_pos;
	for (AM_U8 b = 0; !b; leadingZeroBits++,j++)
	{
		if(j == 8)
		{
			i++;
			j=0;
		}
		if (j == 0)
		{
			if (pBuffer[i] == 0x3 &&
				*(pBuffer+i-1) == 0 &&
				*(pBuffer+i-2) == 0)
				i++;
		}
		b = pBuffer[i] >> (7 -j) & 0x1;
	}
	AM_INT codeNum = 0;
	i += read_bit(pBuffer+i,  &codeNum, &j, leadingZeroBits);
	codeNum += (1 << leadingZeroBits) -1;
	if (type == 0) //ue(v)
	{
		*value = codeNum;
	}
	else if (type == 1) //se(v)
	{
		*value = (codeNum/2+1);
		if (codeNum %2 == 0)
		{
			*value *= -1;
		}
	}
	*bit_pos = j;
	return i;
}

#define parse_ue(x,y,z)		parse_exp_codes((x),(y),(z),0)
#define parse_se(x,y,z)		parse_exp_codes((x),(y),(z),1)

AM_INT CCreateMp4::parse_scaling_list(AM_U8* pBuffer,AM_UINT sizeofScalingList,AM_U8* bit_pos)
{
	AM_INT* scalingList = new AM_INT[sizeofScalingList];
	AM_U8 useDefaultScalingMatrixFlag;
	AM_INT lastScale = 8;
	AM_INT nextScale = 8;
	AM_INT delta_scale;
	AM_INT i = 0;
	for (AM_UINT j =0; j<sizeofScalingList; j++ )
	{
		if (nextScale != 0)
		{
			i += parse_se(pBuffer, &delta_scale,bit_pos);
			nextScale = (lastScale+ delta_scale + 256)%256;
			useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
		}
		scalingList[j] = ( nextScale == 0 ) ? lastScale : nextScale;
		lastScale = scalingList[j];
	}
	delete[] scalingList;
	return i;
};

AM_ERR CCreateMp4::get_h264_info(unsigned char* pBuffer, int size,
	CMP4MUX_H264_INFO* pH264Info)
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
	AM_INT log2_max_frame_num_minus4;
	AM_INT pic_order_cnt_type;
	AM_INT log2_max_pic_order_cnt_lsb_minus4;
	AM_INT num_ref_frames;
	AM_BOOL gaps_in_frame_num_value_allowed_flag;
	AM_INT pic_width_in_mbs_minus1;
	AM_INT pic_height_in_map_units_minus1;
	AM_BOOL frame_mbs_only_flag;
	AM_BOOL direct_8x8_inference_flag;

	pSPS += read_bit(pSPS, &profile_idc,&bit_pos,8);
	pSPS += read_bit(pSPS, &constraint_set,&bit_pos,8);
	pSPS += read_bit(pSPS, &level_idc, &bit_pos,8);
	pSPS += parse_ue(pSPS,&seq_paramter_set_id, &bit_pos);
	if (profile_idc == 100 ||profile_idc == 110 ||
		profile_idc == 122 ||profile_idc == 144 )
	{
		pSPS += parse_ue(pSPS,&chroma_format_idc,&bit_pos);
		if (chroma_format_idc == 3)
			pSPS += read_bit(pSPS, &residual_colour_tramsform_flag, &bit_pos);
		pSPS += parse_ue(pSPS,&bit_depth_luma_minus8,&bit_pos);
		pSPS += parse_ue(pSPS,&bit_depth_chroma_minus8,&bit_pos);
		pSPS += read_bit(pSPS,&qpprime_y_zero_transform_bypass_flag,&bit_pos);
		pSPS += read_bit(pSPS,&seq_scaling_matrix_present_flag,&bit_pos);
		if (seq_scaling_matrix_present_flag )
		{
			for (AM_UINT i = 0; i<8; i++)
			{
				pSPS += read_bit(pSPS,&seq_scaling_list_present_flag[i],&bit_pos);
				if (seq_scaling_list_present_flag[i])
				{
					if (i < 6)
					{
						pSPS += parse_scaling_list(pSPS,16,&bit_pos);
					}
					else
					{
						pSPS += parse_scaling_list(pSPS,64,&bit_pos);
					}
				}
			}
		}
	}
	pSPS += parse_ue(pSPS,&log2_max_frame_num_minus4,&bit_pos);
	pSPS += parse_ue(pSPS,&pic_order_cnt_type,&bit_pos);
	if (pic_order_cnt_type == 0)
	{
		pSPS += parse_ue(pSPS,&log2_max_pic_order_cnt_lsb_minus4,&bit_pos);
	}
	else if (pic_order_cnt_type == 1)
	{
		AM_BOOL delta_pic_order_always_zero_flag;
		pSPS += read_bit(pSPS, &delta_pic_order_always_zero_flag, &bit_pos);
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int num_ref_frames_in_pic_order_cnt_cycle;
		pSPS += parse_se(pSPS,&offset_for_non_ref_pic, &bit_pos);
		pSPS += parse_se(pSPS,&offset_for_top_to_bottom_field, &bit_pos);
		pSPS += parse_ue(pSPS,&num_ref_frames_in_pic_order_cnt_cycle, &bit_pos);
		AM_INT* offset_for_ref_frame = new AM_INT[num_ref_frames_in_pic_order_cnt_cycle];
		for (AM_INT i =0;i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
		{
			pSPS += parse_se(pSPS,offset_for_ref_frame+i, &bit_pos);
		}
		delete[] offset_for_ref_frame;
	}
	pSPS += parse_ue(pSPS,&num_ref_frames, &bit_pos);
	pSPS += read_bit(pSPS, &gaps_in_frame_num_value_allowed_flag, &bit_pos);
	pSPS += parse_ue(pSPS,&pic_width_in_mbs_minus1,&bit_pos);
	pSPS += parse_ue(pSPS,&pic_height_in_map_units_minus1, &bit_pos);

	pH264Info->width = (short)(pic_width_in_mbs_minus1 + 1) << 4;
	pH264Info->height = (short)(pic_height_in_map_units_minus1 + 1) <<4;

	AM_INFO("pH264Info->width = %d\n", pH264Info->width);
	AM_INFO("pH264Info->height = %d\n", pH264Info->height);

	pSPS += read_bit(pSPS, &frame_mbs_only_flag, &bit_pos);
	if (!frame_mbs_only_flag)
	{
		AM_BOOL mb_adaptive_frame_field_flag;
		pSPS += read_bit(pSPS, &mb_adaptive_frame_field_flag, &bit_pos);
		pH264Info->height *= 2;
	}

	pSPS += read_bit(pSPS, &direct_8x8_inference_flag, &bit_pos);
	AM_BOOL frame_cropping_flag;
	AM_BOOL vui_parameters_present_flag;
	pSPS += read_bit(pSPS, &frame_cropping_flag, &bit_pos);
	if (frame_cropping_flag)
	{
		AM_INT frame_crop_left_offset;
		AM_INT frame_crop_right_offset;
		AM_INT frame_crop_top_offset;
		AM_INT frame_crop_bottom_offset;
		pSPS += parse_ue(pSPS,&frame_crop_left_offset, &bit_pos);
		pSPS += parse_ue(pSPS,&frame_crop_right_offset, &bit_pos);
		pSPS += parse_ue(pSPS,&frame_crop_top_offset, &bit_pos);
		pSPS += parse_ue(pSPS,&frame_crop_bottom_offset, &bit_pos);
	}
	pSPS += read_bit(pSPS, &vui_parameters_present_flag, &bit_pos);

	if (vui_parameters_present_flag)
	{
		AM_INFO("Begin to set pH264Info->fps.\n");
		AM_BOOL aspect_ratio_info_present_flag;
		pSPS += read_bit(pSPS, &aspect_ratio_info_present_flag, &bit_pos);
		if (aspect_ratio_info_present_flag)
		{
			AM_INT aspect_ratio_idc;
			pSPS += read_bit(pSPS, &aspect_ratio_idc,&bit_pos,8);
			if (aspect_ratio_idc == 255) // Extended_SAR
			{
				AM_INT sar_width;
				AM_INT sar_height;
				pSPS += read_bit(pSPS, &sar_width, &bit_pos, 16);
				pSPS += read_bit(pSPS, &sar_height, &bit_pos, 16);
			}
		}
		AM_BOOL overscan_info_present_flag;
		pSPS += read_bit(pSPS, &overscan_info_present_flag, &bit_pos);
		if (overscan_info_present_flag)
		{
			AM_BOOL overscan_appropriate_flag;
			pSPS += read_bit(pSPS, &overscan_appropriate_flag, &bit_pos);
		}
		AM_BOOL video_signal_type_present_flag;
		pSPS += read_bit(pSPS, &video_signal_type_present_flag, &bit_pos);
		if (video_signal_type_present_flag)
		{
			AM_INT video_format;
			pSPS += read_bit(pSPS, &video_format, &bit_pos,3);
			AM_BOOL video_full_range_flag;
			pSPS += read_bit(pSPS, &video_full_range_flag, &bit_pos);
			AM_BOOL colour_description_present_flag;
			pSPS += read_bit(pSPS, &colour_description_present_flag, &bit_pos);
			if (colour_description_present_flag)
			{
				AM_INT colour_primaries, transfer_characteristics,matrix_coefficients;
				pSPS += read_bit(pSPS, &colour_primaries, &bit_pos, 8);
				pSPS += read_bit(pSPS, &transfer_characteristics, &bit_pos, 8);
				pSPS += read_bit(pSPS, &matrix_coefficients, &bit_pos, 8);
			}
		}

		AM_BOOL chroma_loc_info_present_flag;
		pSPS += read_bit(pSPS, &chroma_loc_info_present_flag, &bit_pos);
		if( chroma_loc_info_present_flag )
		{
			AM_INT chroma_sample_loc_type_top_field;
			AM_INT chroma_sample_loc_type_bottom_field;
			pSPS += parse_ue(pSPS,&chroma_sample_loc_type_top_field, &bit_pos);
			pSPS += parse_ue(pSPS,&chroma_sample_loc_type_bottom_field, &bit_pos);
		}
		AM_BOOL timing_info_present_flag;
		pSPS += read_bit(pSPS, &timing_info_present_flag, &bit_pos);
		if (timing_info_present_flag)
		{
			AM_INT num_units_in_tick,time_scale;
			AM_BOOL fixed_frame_rate_flag;
			pSPS += read_bit(pSPS, &num_units_in_tick, &bit_pos, 32);
			pSPS += read_bit(pSPS, &time_scale, &bit_pos, 32);
			pSPS += read_bit(pSPS, &fixed_frame_rate_flag, &bit_pos);
			if (fixed_frame_rate_flag)
			{
				AM_U8 divisor; //when pic_struct_present_flag == 1 && pic_struct == 0
				//pH264Info->fps = (float)time_scale/num_units_in_tick/divisor;
				if (frame_mbs_only_flag) {
					divisor = 2;
				} else {
					divisor = 1; // interlaced
				}
				pH264Info->fps = divisor * num_units_in_tick * CLOCK / time_scale;
				AM_INFO("pH264Info->fps = %d\n", pH264Info->fps);
				if (divisor * num_units_in_tick * CLOCK % time_scale) {
					if (pH264Info->fps == 1501)
						_decimal_pts = 2;	//  1501,1502,1501,1502
					else
						_decimal_pts = 1;
				}
			}
		}
	}

	pH264Info->fps = 1501;
	_decimal_pts = 2;
	
	if ((pH264Info->width == 0) ||
		(pH264Info->height == 0) ||
		(pH264Info->fps == 0)) {
		AM_INFO("Return ME_ERROR.\n");
		return ME_ERROR;
	} else {
		AM_INFO("Return ME_OK.\n");
		return ME_OK;
	}

};

//Generate AAC decoder information
AM_UINT CCreateMp4::get_aac_info(AM_UINT samplefreq, AM_UINT channel)
{
	//bits5: Audio Object Type  AAC Main=1, AAC LC (low complexity)=2, AAC SSR=3, AAC LTP=4
	//bits4: Sample Frequency Index
	//	 bits24: Sampling Frequency [if Sample Frequency Index == 0xf]
	//bits4: Channel Configuration
	//bits1: FrameLength 0:1024, 1:960
	//bits1: DependsOnCoreCoder
	//	 bits14: CoreCoderDelay [if DependsOnCoreCoder==1]
	//bits1: ExtensionFlag
	AM_UINT info = 0x1000;//AAC LC
	AM_UINT sfi = 0x0f;
	//ISO/IEC 14496-3 Table 1.16 Sampling Frequency Index
	switch (samplefreq)
	{
	case 96000:
		sfi = 0;
		break;
	case 88200:
		sfi = 1;
		break;
	case 64000:
		sfi = 2;
		break;
	case 48000:
		sfi = 3;
		break;
	case 44100:
		sfi = 4;
		break;
	case 32000:
		sfi = 5;
		break;
	case 24000:
		sfi = 6;
		break;
	case 22050:
		sfi = 7;
		break;
	case 16000:
		sfi = 8;
		break;
	case 12000:
		sfi = 9;
		break;
	case 11025:
		sfi = 10;
		break;
	case 8000:
		sfi = 11;
		break;
	}
	info |= (sfi << 7);
	info |= (channel << 3);
	return info;
}


//--------------------------------------------------
#define VideoMediaHeaderBox_SIZE			20
#define DataReferenceBox_SIZE			   28
#define DataInformationBox_SIZE			 (8+DataReferenceBox_SIZE)
#define AvcConfigurationBox_SIZE			(19+_sps_size+_pps_size)
#define BitrateBox_SIZE					0// 20
#define VisualSampleEntry_SIZE			  (86+AvcConfigurationBox_SIZE+BitrateBox_SIZE)
#define VideoSampleDescriptionBox_SIZE	  (16+VisualSampleEntry_SIZE)
#define VideoDecodingTimeToSampleBox_SIZE		(_decimal_pts? 16+(_video_cnt<<3):24)
#define CompositionTimeToSampleBox_SIZE	 (16+(_video_cnt<<3))
#define SampleToChunkBox_SIZE			   28
#define VideoSampleSizeBox_SIZE			 (20+(_video_cnt<<2))
#define VideoChunkOffsetBox_SIZE			(16+(_video_cnt<<2))
#define SyncSampleBox_SIZE				  (16+(_stss_cnt<<2))
#define VideoSampleTableBox_SIZE			(8+VideoSampleDescriptionBox_SIZE+\
											 VideoDecodingTimeToSampleBox_SIZE+\
											 CompositionTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 VideoSampleSizeBox_SIZE+\
											 VideoChunkOffsetBox_SIZE+\
											 SyncSampleBox_SIZE)
#define VideoMediaInformationBox_SIZE	   (8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_SIZE)

//--------------------------------------------------

#define VIDEO_HANDLER_NAME		("Ambarella AVC")
#define VIDEO_HANDLER_NAME_LEN	strlen(VIDEO_HANDLER_NAME)+1
#define MediaHeaderBox_SIZE			32
#define VideoHandlerReferenceBox_SIZE  (32+VIDEO_HANDLER_NAME_LEN) // 46
#define VideoMediaBox_SIZE			 (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_SIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
//--------------------------------------------------

#define ElementaryStreamDescriptorBox_SIZE  50
#define AudioSampleEntry_SIZE               (36+ElementaryStreamDescriptorBox_SIZE) // 86
#define AudioSampleDescriptionBox_SIZE      (16+AudioSampleEntry_SIZE) // 102
#define AudioDecodingTimeToSampleBox_SIZE		24
#define AudioSampleSizeBox_SIZE             (20+(_audio_cnt<<2))
#define AudioChunkOffsetBox_SIZE            (16+(_audio_cnt<<2))
#define AudioSampleTableBox_SIZE            (8+AudioSampleDescriptionBox_SIZE+\
											AudioDecodingTimeToSampleBox_SIZE+\
											SampleToChunkBox_SIZE+\
											AudioSampleSizeBox_SIZE+\
											AudioChunkOffsetBox_SIZE) // 198+A*8
#define SoundMediaHeaderBox_SIZE            16
#define AudioMediaInformationBox_SIZE       (8+SoundMediaHeaderBox_SIZE+\
											DataInformationBox_SIZE+\
											AudioSampleTableBox_SIZE) // 258+A*8

#define AUDIO_HANDLER_NAME		("Ambarella AAC")
#define AUDIO_HANDLER_NAME_LEN         strlen(AUDIO_HANDLER_NAME)+1
#define AudioHandlerReferenceBox_SIZE  (32+AUDIO_HANDLER_NAME_LEN) // 46
#define AudioMediaBox_SIZE             (8+MediaHeaderBox_SIZE+\
									AudioHandlerReferenceBox_SIZE+\
									AudioMediaInformationBox_SIZE) // 344+A*8

//--------------------------------------------------
#define MovieHeaderBox_SIZE  108
//#define ObjDescrpBox_SIZE		24//33
//#define UserDataBox_SIZE	 (8+AMBABox_SIZE) // 48

#define TrackHeaderBox_SIZE	 92//+12
#define VideoTrackBox_SIZE	  (8+TrackHeaderBox_SIZE+VideoMediaBox_SIZE)
#define AudioTrackBox_SIZE       (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE) // 444+A*8
#define MovieBox_SIZE		(8+MovieHeaderBox_SIZE+\
							  VideoTrackBox_SIZE + AudioTrackBox_SIZE)

AM_ERR CCreateMp4::UpdateVideoIdx(AM_INT pts, AM_UINT sample_size,
	AM_UINT chunk_offset, AM_U8 sync_point) {
	AM_U32 cur_video_duration = _video_duration + pts;
	if ( (cur_video_duration < _video_duration)
		&& (pts > 0)) {
		return ME_TOO_MANY;
	}

	AM_INT idx = _video_cnt % IDX_DUMP_SIZE;
	AM_U16 start_code[2];
	start_code[0] = 0x1;
	if (idx == 0 && _video_cnt > 0) {
		start_code[1] = sizeof(_v_stsz);
		_pIdxSink->Write((AM_U8 *)start_code,4);
		_pIdxSink->Write((AM_U8 *)"stsz",4);
		_pIdxSink->Write((AM_U8*)_v_stsz,start_code[1]);

		start_code[1] = sizeof(_v_stco);
		_pIdxSink->Write((AM_U8*)start_code,4);
		_pIdxSink->Write((AM_U8 *)"stco",4);
		_pIdxSink->Write((AM_U8*)_v_stco,start_code[1]);

		start_code[1] = sizeof(_ctts);
		_pIdxSink->Write((AM_U8 *)start_code,4);
		_pIdxSink->Write((AM_U8 *)"ctts",4);
		_pIdxSink->Write((AM_U8 *)_ctts,start_code[1]);

		if (_decimal_pts) {
			start_code[1] = sizeof(_stts);
			_pIdxSink->Write((AM_U8 *)start_code,4);
			_pIdxSink->Write((AM_U8 *)"stts",4);
			_pIdxSink->Write((AM_U8 *)_stts,start_code[1]);
		}
	}
	_v_stsz[idx] = le_to_be32(sample_size);	//sample size
	_v_stco[idx] = le_to_be32(chunk_offset);	//chunk_offset

	AM_UINT ctts = 0;
	AM_UINT stts = 0;
	if (_video_cnt == 0) {
		if (_decimal_pts) {
			stts = _h264_info.fps;
			ctts = _h264_info.fps + 1;
		} else
			ctts = _h264_info.fps;
	} else {
		if (_decimal_pts) {
			stts = (_video_cnt % _decimal_pts == (_decimal_pts -1))?
					(_h264_info.fps +1) : _h264_info.fps;
			ctts = _last_ctts + pts - stts;
		}
		else
			ctts = _last_ctts + pts -_h264_info.fps;
	}
	_ctts[2*idx] = le_to_be32(1);
	_ctts[2*idx+1] = le_to_be32(ctts);
	_last_ctts = ctts;

	if (_decimal_pts) {
		_stts[2*idx] = le_to_be32(1);
		_stts[2*idx+1] = le_to_be32(stts);
	}

	_video_cnt++;
	_video_duration = cur_video_duration;

	if (sync_point) {
		idx = _stss_cnt % IDX_DUMP_SIZE;
		if (idx == 0 && _stss_cnt > 0) {
			start_code[1] = sizeof(_stss);
			_pIdxSink->Write((AM_U8 *)start_code,4);
			_pIdxSink->Write((AM_U8 *)"stss",4);
			_pIdxSink->Write((AM_U8 *)_stss, start_code[1]);
		}
		_stss[idx] = le_to_be32(_video_cnt);
		_stss_cnt++;
	}
	return ME_OK;
}

AM_ERR CCreateMp4::UpdateAudioIdx(AM_UINT sample_size,
	AM_UINT chunk_offset) {
	AM_U32 cur_audio_duration = _audio_duration + _audio_info.chunk_size;
	if (cur_audio_duration < _audio_duration) { //over flow
		return ME_TOO_MANY;
	}

	AM_INT idx = _audio_cnt % IDX_DUMP_SIZE;
	AM_U16 start_code[2];
	start_code[0] = 0x2;
	if (idx == 0 && _audio_cnt > 0) {
		start_code[1] = sizeof(_a_stsz);
		_pIdxSink->Write((AM_U8 *)start_code,4);
		_pIdxSink->Write((AM_U8 *)"stsz",4);
		_pIdxSink->Write((AM_U8*)_a_stsz,start_code[1]);

		start_code[1] = sizeof(_a_stco);
		_pIdxSink->Write((AM_U8*)start_code,4);
		_pIdxSink->Write((AM_U8 *)"stco",4);
		_pIdxSink->Write((AM_U8*)_a_stco,start_code[1]);
	}
	_a_stsz[idx] = le_to_be32(sample_size);	//sample size
	_a_stco[idx] = le_to_be32(chunk_offset);	//chunk_offset

	_audio_cnt ++;
	_audio_duration = cur_audio_duration;
	return ME_OK;
}

#define	FileTypeBox_SIZE	24
AM_ERR CCreateMp4::put_FileTypeBox()
{
	int ret = 0;
	ret += put_be32(FileTypeBox_SIZE);	//uint32 size
	ret += put_boxtype("ftyp");
	ret += put_boxtype("mp42");  //uint32 major_brand
	ret += put_be32(0);                           //uint32 minor_version
	ret += put_boxtype("mp42");  //uint32 compatible_brands[]
       ret += put_boxtype("isom");
	if (ret == 0) {
		return ME_OK;
	} else {
		return ME_ERROR;
	}
}

//mp4 file size < 2G, no B frames, 1 chunk has only one sample
AM_ERR CCreateMp4::put_MediaDataBox()
{
	int ret = 0;
	_mdat_begin_pos = _curPos;
	ret += put_be32(0);            //uint32 size, will be revised in the end
	ret += put_boxtype("mdat");
	if (ret == 0) {
		return ME_OK;
	} else {
		return ME_ERROR;
	}
}

AM_ERR CCreateMp4::put_VideoData(AM_U8* pData, AM_UINT size, AM_PTS pts) {
	//AM_INFO("In put_VideoData: size = %d\n", size);
	AM_UINT estimated_file_size = _curPos +  size + MovieBox_SIZE;
	if (/*estimated_file_size < _curPos ||*/
		estimated_file_size > _record_info.max_filesize ||
		_video_cnt == _record_info.max_videocnt) {
		return ME_TOO_MANY;
	}

	/*
	 * A static variable is defined here for debugging.
	 * Theoretically, when the start code of a nal unit
	 * is not equal to 0x00000001, muxer will stop to 
	 * mux immediately.
	 */
	static int zero_nal_unit_type = 0;
		
	AM_UINT nal_unit_length;
	AM_U8 nal_unit_type;
	AM_UINT cursor = 0;
	AM_UINT chunk_offset = _curPos;
	AM_U8 sync_point = 0;
	do {
		nal_unit_length = get_nal_unit(&nal_unit_type, pData+cursor, size-cursor);
		AM_INFO("Return value from get_nal_unit is %d\n", nal_unit_length);
		AM_INFO("nal_unit_type is %d\n", nal_unit_type);

#ifdef ZERO_NAL_UNIT_TYPE
		// AM_UINT code = *((AM_UINT *)pData);
		// if (code == 0) {
		//	_pNSink->Write (pData, size);
		// }
#endif

		if (nal_unit_length > 0 && nal_unit_type > 0 ) {
			if (nal_unit_type == 7) //not write sps
			{
				if (_sps == NULL) {
					_sps_size = nal_unit_length - 4;
					_sps = new AM_U8[_sps_size];
					memcpy(_sps, pData+cursor+4, _sps_size) ;//exclude start code 0x00000001
					_pNSink->Write(pData, size);
				}
				if ((_h264_info.width == 0) && (_h264_info.height == 0)
					&& (_h264_info.fps == 0)) {
					/* if ( ME_OK != get_h264_info(pData+cursor+5, nal_unit_length-5, &_h264_info)) {
						return ME_ERROR;
					} */
					_h264_info.width = 1280;//_p_h264_config->pic_info.width;
					_h264_info.height = 720;//_p_h264_config->pic_info.height;

					_h264_info.fps = 1501;
					_decimal_pts = 2;
					_last_ctts = _h264_info.fps;
				}
			} else if (nal_unit_type == 8) {//not write pps
				if (_pps == NULL) {
					_pps_size = nal_unit_length -4;
					_pps = new AM_U8[_pps_size];
					memcpy(_pps, pData+cursor+4, _pps_size); //exclude start code 0x00000001
				}
			} else {
				if (nal_unit_type == 5) {
					sync_point = 1;
				}
				AM_UINT length = nal_unit_length -4;
				put_be32(length);
				put_buffer(pData+cursor+4, length);
			}
			cursor += nal_unit_length;
		} else {
			AM_ERROR("nal unit parse error\n");
			zero_nal_unit_type++;
			if (zero_nal_unit_type <= 3) {
				break;
			} else {
				return ME_ERROR;
			}
			// return ME_ERROR;
		}
	}while (cursor < size);
	if (_video_cnt == 0) {
		_last_pts = pts;
	}

#ifdef RECORD_VIDEO_FILE
	_pVSink->Write (pData, size);
#endif

	AM_INT delta_pts = pts - _last_pts;
	_last_pts = pts;
	return UpdateVideoIdx(delta_pts, (_curPos - chunk_offset), chunk_offset, sync_point);
}

AM_ERR CCreateMp4::put_AudioData(AM_U8* pData, AM_UINT size) {
	AM_UINT estimated_file_size = _curPos +  size + MovieBox_SIZE;
	if (/*estimated_file_size < _curPos ||*/
		estimated_file_size > _record_info.max_filesize) {
		return ME_TOO_MANY;
	}

	AM_UINT header_length = 0;
	AM_UINT chunk_offset = _curPos;
	AM_U8* pAdts_header = pData;
	AM_U8 bit_pos = 0;

	if (size < 7) {
		return ME_ERROR;
	}

	AM_INFO("AMBA_AAC = %d\n", AMBA_AAC);
	if (AMBA_AAC == 1) {
		AM_INT syncword;
		pAdts_header += read_bit(pAdts_header,  &syncword, &bit_pos, 12);
		if (syncword != 0xFFF) {
			return ME_ERROR;
		}
		bit_pos += 3;
		AM_INT protection_absent;
		pAdts_header += read_bit(pAdts_header,  &protection_absent, &bit_pos);
		pAdts_header = pData + 6;
		bit_pos = 6;
		AM_INT number_of_raw_data_blocks_in_frame;
		pAdts_header += read_bit(pAdts_header,  &number_of_raw_data_blocks_in_frame, &bit_pos, 2);
		header_length= 7; //adts_fixed_header + adts_variable_header
		if (number_of_raw_data_blocks_in_frame == 0) {
			if (protection_absent == 0) { //adts_error_check
				header_length += 2;
			}
		} else {
			//To do
		}
	}

	AM_INFO("header_length = %d\n", header_length);
	put_buffer(pData+header_length, size - header_length);

#ifdef RECORD_AUDIO_FILE
	_pASink->Write (pData, size);
#endif

	return UpdateAudioIdx((_curPos - chunk_offset), chunk_offset);
}

AM_ERR CCreateMp4::put_MovieBox()
{
	//MovieBox
	int ret = 0;
	ret += put_be32(MovieBox_SIZE);	//uint32 size
	ret += put_boxtype("moov"); //'moov'

	//MovieHeaderBox
	ret += put_be32(MovieHeaderBox_SIZE);	//uint32 size
	ret += put_boxtype("mvhd");	//'mvhd'
	ret += put_byte(0);	//uint8 version
	put_be24(0);	//bits24 flags
	put_be32(_create_time);	//uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_create_time);	//uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(CLOCK);		//uint32 timescale
	put_be32(_video_duration);//uint32 duration [version==0] uint64 duration [version==1]
	put_be32(0x00010000);	//int32 rate
	put_be16(0x0100);	//int16 volume
	put_be16(0);	//bits16 reserved
	put_be32(0);	//uint32 reserved[2]
	put_be32(0);
	put_be32(0x00010000);	//int32 matrix[9]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x00010000);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x40000000);
	put_be32(0);	//bits32 pre_defined[6]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(3);	//uint32 next_track_ID
/*
	put_be32(ObjDescrpBox_SIZE); 	 //uint32 size
	put_boxtype("iods");  //'iods'
	put_byte(0);                           //uint8 version
	put_be24(0);                           //bits24 flags
	put_be32(0x10808080);
	put_be32(0x07004FFF );
	put_be32(0xFF0F7FFF);

	//UserDataBox
	put_be32(UserDataBox_SIZE);			//uint32 size
	put_be32(FOURCC('u', 'd', 't', 'a'));  //'udta'
*/

	put_VideoTrackBox(1, _video_duration);
	put_AudioTrackBox(2,  (AM_UINT)(1.0L * _audio_duration * CLOCK /_audio_info.sample_freq));
	UpdateIdxBox();
	return ME_OK;
}

void CCreateMp4::put_VideoTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
	//TrackBox
	put_be32(VideoTrackBox_SIZE);		  //uint32 size
	put_boxtype("trak");  //'trak'

	//TrackHeaderBox
	put_be32(TrackHeaderBox_SIZE);		 //uint32 size
	put_boxtype("tkhd");  //'tkhd'
	put_byte(0);						   //uint8 version
	//0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
	put_be24(0x07);						//bits24 flags
	put_be32(_create_time);			   //uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_create_time);		   //uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(TrackId);					 //uint32 track_ID
	put_be32(0);						   //uint32 reserved
	put_be32(Duration);					//uint32 duration [version==0] uint64 duration [version==1]
	put_be32(0);						   //uint32 reserved[2]
	put_be32(0);
	put_be16(0);						   //int16 layer
	put_be16(0);						   //int16 alternate_group
	put_be16(0x0000);					  //int16 volume
	put_be16(0);						   //uint16 reserved
	put_be32(0x00010000);				  //int32 matrix[9]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x00010000);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x40000000);
	put_be32(_h264_info.width<<16);		  //uint32 width  //16.16 fixed-point
	put_be32(_h264_info.height<<16);		 //uint32 height //16.16 fixed-point

	put_VideoMediaBox(Duration);
}

void CCreateMp4::put_VideoMediaBox(AM_UINT Duration)
{
	//MediaBox
	put_be32(VideoMediaBox_SIZE);			   //uint32 size
	put_boxtype("mdia");	   //'mdia'

	//MediaHeaderBox
	put_be32(MediaHeaderBox_SIZE);			  //uint32 size
	put_boxtype("mdhd");	   //'mdhd'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(_create_time);					//uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_create_time);				//uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(CLOCK);					 //uint32 timescale
	put_be32(Duration);						 //uint32 duration [version==0] uint64 duration [version==1]
	put_be16(0);								//bits5 language[3]  //ISO-639-2/T language code
	put_be16(0);								//uint16 pre_defined

	//HandlerReferenceBox
	put_be32(VideoHandlerReferenceBox_SIZE);	//uint32 size
	put_boxtype("hdlr");	//'hdlr'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(0);								//uint32 pre_defined
	put_boxtype("vide");	//'vide'
	put_be32(0);								//uint32 reserved[3]
	put_be32(0);
	put_be32(0);
	put_byte(VIDEO_HANDLER_NAME_LEN);		   //char name[], name[0] is actual length
	put_buffer((AM_U8 *)VIDEO_HANDLER_NAME, VIDEO_HANDLER_NAME_LEN-1);

	put_VideoMediaInformationBox();
}

void CCreateMp4::put_VideoMediaInformationBox()
{
	//MediaInformationBox
	put_be32(VideoMediaInformationBox_SIZE);	 //uint32 size
	put_boxtype("minf");		//'minf'

	//VideoMediaHeaderBox
	put_be32(VideoMediaHeaderBox_SIZE);		  //uint32 size
	put_boxtype("vmhd");		//'vmhd'

	put_byte(0);								 //uint8 version
	//This is a compatibility flag that allows QuickTime to distinguish between movies created with QuickTime
	//1.0 and newer movies. You should always set this flag to 1, unless you are creating a movie intended
	//for playback using version 1.0 of QuickTime
	put_be24(1);								 //bits24 flags
	put_be16(0);								 //uint16 graphicsmode  //0=copy over the existing image
	put_be16(0);								 //uint16 opcolor[3]	  //(red, green, blue)
	put_be16(0);
	put_be16(0);

	//DataInformationBox
	put_be32(DataInformationBox_SIZE);		   //uint32 size
	put_boxtype("dinf");		//'dinf'

	//DataReferenceBox
	put_be32(DataReferenceBox_SIZE);			 //uint32 size
	put_boxtype("dref");	//'dref'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(12);								//uint32 size
	put_boxtype("url");		//'url '
	put_byte(0);								 //uint8 version
	put_be24(1);								 //bits24 flags	//1=media data is in the same file as the MediaBox

	//SampleTableBox
	put_be32(VideoSampleTableBox_SIZE);		  //uint32 size
	put_boxtype("stbl");	//'stbl'

	//SampleDescriptionBox
	put_be32(VideoSampleDescriptionBox_SIZE);	//uint32 size
	put_boxtype("stsd");	//'stsd'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	//VisualSampleEntry
	put_be32(VisualSampleEntry_SIZE);			//uint32 size
	put_boxtype("avc1");	//'avc1'
	put_byte(0);								 //uint8 reserved[6]
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_be16(1);								 //uint16 data_reference_index
	put_be16(0);								 //uint16 pre_defined
	put_be16(0);								 //uint16 reserved
	put_be32(0);								 //uint32 pre_defined[3]
	put_be32(0);
	put_be32(0);
	put_be16(_h264_info.width);					//uint16 width
	put_be16(_h264_info.height);				   //uint16 height
	put_be32(0x00480000);						//uint32 horizresolution  72dpi
	put_be32(0x00480000);						//uint32 vertresolution   72dpi
	put_be32(0);								 //uint32 reserved
	put_be16(1);								 //uint16 frame_count
	AM_U8 EncoderName[32]="\012AVC Coding"; //Compressorname
	put_buffer(EncoderName,32);
	put_be16(0x0018);							//uint16 depth   //0x0018=images are in colour with no alpha
	put_be16(-1);								//int16 pre_defined
	//AvcConfigurationBox
	put_be32(AvcConfigurationBox_SIZE);		  //uint32 size
	put_boxtype("avcC");	//'avcC'
	put_byte(1);								 //uint8 configurationVersion
	put_byte(_sps[1]);						   //uint8 AVCProfileIndication
	put_byte(_sps[2]);						   //uint8 profile_compatibility
	put_byte(_sps[3]);						   //uint8 level
	put_byte(0xFF);							  //uint8 nal_length  //(nal_length&0x03)+1 [reserved:6, lengthSizeMinusOne:2]
	put_byte(0xE1);							  //uint8 sps_count  //sps_count&0x1f [reserved:3, numOfSequenceParameterSets:5]
	put_be16(_sps_size);						  //uint16 sps_size	  //sequenceParameterSetLength
	put_buffer(_sps, _sps_size);				  //uint8 sps[sps_size] //sequenceParameterSetNALUnit
	put_byte(1);								 //uint8 pps_count	  //umOfPictureParameterSets
	put_be16(_pps_size);						  //uint16 pps_size	  //pictureParameterSetLength

	put_buffer(_pps, _pps_size);				  //uint8 pps[pps_size] //pictureParameterSetNALUnit

/*
	//BitrateBox
	put_be32(BitrateBox_SIZE);				   //uint32 size
	put_boxtype("btrt");		//'btrt'
	put_be32(0);								 //uint32 buffer_size
	put_be32(0);								 //uint32 max_bitrate
	put_be32(0);								 //uint32 avg_bitrate
*/
	AM_UINT skip_length = 0;
	AM_UINT current_idx_cnt = 0;
	AM_UINT dump_cnt = 0;

	current_idx_cnt = (_video_cnt -1)%IDX_DUMP_SIZE + 1;
	dump_cnt = (_video_cnt -1)/IDX_DUMP_SIZE;

	//DecodingTimeToSampleBox						 //bits24 flags
	put_be32(VideoDecodingTimeToSampleBox_SIZE);	  //uint32 size
	put_boxtype("stts");		//'stts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags

	if (!_decimal_pts) {
		put_be32(1);								 //uint32 entry_count
		put_be32(_video_cnt);
		put_be32(_h264_info.fps);
	} else {
		put_be32(_video_cnt);					   //uint32 entry_count

		if (dump_cnt > 0) {
			_v_stts_pos = _curPos;
			skip_length = dump_cnt* IDX_DUMP_SIZE*2*sizeof(_stts[0]);
			_pSink->Seek(skip_length, SEEK_CUR);
			_curPos += skip_length;
		}
		put_buffer((AM_U8 *)_stts, current_idx_cnt*2*sizeof(_stts[0]));
	}

	//CompositionTimeToSampleBox
	put_be32(CompositionTimeToSampleBox_SIZE);   //uint32 size
	put_boxtype("ctts");			//'ctts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_video_cnt);					   //uint32 entry_count

	if (dump_cnt > 0) {
		_v_ctts_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*2*sizeof(_ctts[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8 *)_ctts, current_idx_cnt*2*sizeof(_ctts[0]));

	//SampleToChunkBox
	put_be32(SampleToChunkBox_SIZE);			 //uint32 size
	put_boxtype("stsc");		//'stsc'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(1);								 //uint32 first_chunk
	put_be32(1);								 //uint32 samples_per_chunk
	put_be32(1);								 //uint32 sample_description_index

	//SampleSizeBox
	put_be32(VideoSampleSizeBox_SIZE);		   //uint32 size
	put_boxtype("stsz");		//'stsz'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(0);								 //uint32 sample_size
	put_be32(_video_cnt);					   //uint32 sample_count

	if (dump_cnt > 0) {
		_v_stsz_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*sizeof(_v_stsz[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8 *)_v_stsz, current_idx_cnt * sizeof(_v_stsz[0]));

	//ChunkOffsetBox
	put_be32(VideoChunkOffsetBox_SIZE);		  //uint32 size
	put_boxtype("stco");		//'stco'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_video_cnt);					   //uint32 entry_count

	if (dump_cnt > 0) {
		_v_stco_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*sizeof(_v_stco[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8 *)_v_stco, current_idx_cnt * sizeof(_v_stco[0]));

	//SyncSampleBox
	put_be32(SyncSampleBox_SIZE);				//uint32 size
	put_boxtype("stss");		//'stss'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_stss_cnt);					 //uint32 entry_count

	current_idx_cnt = (_stss_cnt -1)%IDX_DUMP_SIZE + 1;
	dump_cnt = (_stss_cnt -1)/IDX_DUMP_SIZE;
	if (dump_cnt > 0) {
		_v_stss_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*sizeof(_stss[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8*)_stss, current_idx_cnt*sizeof(_stss[0]));

}

AM_ERR CCreateMp4::UpdateIdxBox()
{
	_pSink->Seek(_mdat_begin_pos,SEEK_SET);
	_curPos = _mdat_begin_pos;
	put_be32(_mdat_end_pos - _mdat_begin_pos);
	_pIdxSink->Seek(0,SEEK_SET);
	AM_U8 head[8];
	AM_U32 length = 0;
	AM_U8* pTempBuf = NULL;
	while (8 == _pIdxSink->Read((AM_U8 *)head,8))
	{
		AM_U16* start_code = (AM_U16*)head;
		if ((start_code[0] != 0x1) &&
			(start_code[0] != 0x2)) {
			return ME_ERROR;
		}
		length = start_code[1];
		if (pTempBuf) {
			delete[] pTempBuf;
			pTempBuf = NULL;
		}
		pTempBuf = new AM_U8[length];
		if (start_code[0] == 0x1) {	//video
			if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 't') && (head[7] == 's')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_v_stts_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_v_stts_pos += length;
				_curPos = _v_stts_pos;
			} else if ((head[4] =='c') && (head[5] == 't') &&
				(head[6] == 't') && (head[7] == 's')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_v_ctts_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_v_ctts_pos += length;
				_curPos = _v_ctts_pos;
			} else if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 's') && (head[7] == 'z')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_v_stsz_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_v_stsz_pos += length;
				_curPos = _v_stsz_pos;
			} else if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 'c') && (head[7] == 'o')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_v_stco_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_v_stco_pos += length;
				_curPos = _v_stco_pos;
			} else if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 's') && (head[7] == 's')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_v_stss_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_v_stss_pos += length;
				_curPos = _v_stss_pos;
			}
		} else { //audio
			 if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 's') && (head[7] == 'z')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_a_stsz_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_a_stsz_pos += length;
				_curPos = _a_stsz_pos;
			} else if ((head[4] =='s') && (head[5] == 't') &&
				(head[6] == 'c') && (head[7] == 'o')) {
				_pIdxSink->Read(pTempBuf, length);
				_pSink->Seek(_a_stco_pos,SEEK_SET);
				_pSink->Write(pTempBuf,length);
				_a_stco_pos += length;
				_curPos = _a_stco_pos;
			}
		}
		if (pTempBuf) {
			delete[] pTempBuf;
			pTempBuf = NULL;
		}
	}
	return ME_OK;
}

void CCreateMp4::put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration)
 {
	//TrackBox
	put_be32(AudioTrackBox_SIZE);          //uint32 size
	put_boxtype("trak");  //'trak'

        //TrackHeaderBox
	put_be32(TrackHeaderBox_SIZE);         //uint32 size
	put_boxtype("tkhd");
	put_byte(0);                           //uint8 version
	//0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
	put_be24(0x07);                        //bits24 flags
	put_be32(_create_time);               //uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_create_time);           //uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(TrackId);                     //uint32 track_ID
	put_be32(0);                           //uint32 reserved
	put_be32(Duration);                    //uint32 duration [version==0] uint64 duration [version==1]
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

void CCreateMp4::put_AudioMediaBox()
{
	//MediaBox
	put_be32(AudioMediaBox_SIZE);               //uint32 size
	put_boxtype("mdia");       //'mdia'

        //MediaHeaderBox
        put_be32(MediaHeaderBox_SIZE);              //uint32 size
        put_boxtype("mdhd");	//'mdhd'
        put_byte(0);                                //uint8 version
        put_be24(0);                                //bits24 flags
        put_be32(_create_time);                    //uint32 creation_time [version==0] uint64 creation_time [version==1]
        put_be32(_create_time);                //uint32 modification_time [version==0] uint64 modification_time [version==1]
        put_be32(_audio_info.sample_freq);              //uint32 timescale
        put_be32(_audio_info.chunk_size*_audio_cnt);                 //uint32 duration [version==0] uint64 duration [version==1]
        put_be16(0);                                //bits5 language[3]  //ISO-639-2/T language code
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
        put_byte(AUDIO_HANDLER_NAME_LEN);           //char name[], name[0] is actual length
        put_buffer((AM_U8 *)AUDIO_HANDLER_NAME, AUDIO_HANDLER_NAME_LEN-1);

        put_AudioMediaInformationBox();
}

void CCreateMp4::put_AudioMediaInformationBox()
{
	//MediaInformationBox
	put_be32(AudioMediaInformationBox_SIZE);     //uint32 size
	put_boxtype("minf");	//'minf'

	//SoundMediaHeaderBox
	put_be32(SoundMediaHeaderBox_SIZE);          //uint32 size
	put_boxtype("smhd");        //'smhd'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be16(0);                                 //int16 balance
	put_be16(0);                                 //uint16 reserved

        //DataInformationBox
	put_be32(DataInformationBox_SIZE);           //uint32 size
	put_boxtype("dinf");    //'dinf'
	//DataReferenceBox
	put_be32(DataReferenceBox_SIZE);             //uint32 size
	put_boxtype("dref");        //'dref'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(1);                                 //uint32 entry_count
	put_be32(12);                                //uint32 size
	put_boxtype("url");       //'url '
	put_byte(0);                                 //uint8 version
	put_be24(1);                                 //bits24 flags    //1=media data is in the same file as the MediaBox

	//SampleTableBox
	put_be32(AudioSampleTableBox_SIZE);          //uint32 size
	put_boxtype("stbl");        //'stbl'

	//SampleDescriptionBox
	put_be32(AudioSampleDescriptionBox_SIZE);    //uint32 size
	put_boxtype("stsd");           //uint32 type
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(1);                                 //uint32 entry_count
	//AudioSampleEntry
	put_be32(AudioSampleEntry_SIZE);            //uint32 size
	put_boxtype("mp4a");        //'mp4a'
	put_byte(0);                                 //uint8 reserved[6]
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_be16(1);                                 //uint16 data_reference_index
	put_be32(0);                                 //uint32 reserved[2]
	put_be32(0);
	put_be16(_audio_info.channels);                 //uint16 channelcount
	put_be16(_audio_info.sample_size);               //uint16 samplesize
	put_be16(0xfffe);                            //uint16 pre_defined   //for QT sound
	put_be16(0);                                 //uint16 reserved
	put_be32(_audio_info.sample_freq<<16);           //uint32 samplerate   //= (timescale of media << 16)

	//ElementaryStreamDescriptorBox
	put_be32(ElementaryStreamDescriptorBox_SIZE);//uint32 size
	put_boxtype("esds");	 //'esds'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	//ES descriptor takes 38 bytes
	put_byte(3);                                 //ES descriptor type tag
	put_be16(0x8080);
	put_byte(34);                                //descriptor type length
	put_be16(0);                                 //ES ID
	put_byte(0);                                 //stream priority
	//Decoder config descriptor takes 26 bytes (include decoder specific info)
                    put_byte(4);                                 //decoder config descriptor type tag
                    put_be16(0x8080);
                    put_byte(22);                                //descriptor type length
                    put_byte(0x40);                              //object type ID MPEG-4 audio=64 AAC
                    put_byte(0x15);                              //stream type:6, upstream flag:1, reserved flag:1 (audio=5)    Audio stream
                    put_be24(8192);                              // buffer size
                    put_be32(128000);                            // max bitrate
                    put_be32(128000);                            // avg bitrate
                    //Decoder specific info descriptor takes 9 bytes
                    put_byte(5);                                 //decoder specific descriptor type tag
                    put_be16(0x8080);
                    put_byte(5);                                 //descriptor type length
                    put_be16(get_aac_info(_audio_info.sample_freq, _audio_info.channels));
                    put_be16(0x0000);
                    put_byte(0x00);
                    //SL descriptor takes 5 bytes
                    put_byte(6);                                 //SL config descriptor type tag
                    put_be16(0x8080);
                    put_byte(1);                                 //descriptor type length
                    put_byte(2);                                 //SL value

	//DecodingTimeToSampleBox
	put_be32(AudioDecodingTimeToSampleBox_SIZE);      //uint32 size
	put_boxtype("stts");        //'stts'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(1);                                 //uint32 entry_count
	put_be32(_audio_cnt);                       //uint32 sample_count
	put_be32(_audio_info.chunk_size);                              //uint32 sample_delta

	//SampleToChunkBox
	put_be32(SampleToChunkBox_SIZE);             //uint32 size
	put_boxtype("stsc");       //'stsc'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(1);                                 //uint32 entry_count
	put_be32(1);                                 //uint32 first_chunk
	put_be32(1);                                 //uint32 samples_per_chunk
	put_be32(1);                                 //uint32 sample_description_index

	AM_UINT skip_length = 0;
	AM_UINT current_idx_cnt = 0;
	AM_UINT dump_cnt = 0;

	current_idx_cnt = (_audio_cnt -1)%IDX_DUMP_SIZE + 1;
	dump_cnt = (_audio_cnt -1)/IDX_DUMP_SIZE;

	//SampleSizeBox
	put_be32(AudioSampleSizeBox_SIZE);           //uint32 size
	put_boxtype("stsz");        //'stsz'
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(0);                                 //uint32 sample_size
	put_be32(_audio_cnt);                       //uint32 sample_count

	if (dump_cnt > 0) {
		_a_stsz_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*sizeof(_a_stsz[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8 *)_a_stsz, current_idx_cnt * sizeof(_a_stsz[0]));

	//ChunkOffsetBox
	put_be32(AudioChunkOffsetBox_SIZE);          //uint32 size
	put_boxtype("stco");
	put_byte(0);                                 //uint8 version
	put_be24(0);                                 //bits24 flags
	put_be32(_audio_cnt);                       //uint32 entry_count
	if (dump_cnt > 0) {
		_a_stco_pos = _curPos;
		skip_length = dump_cnt* IDX_DUMP_SIZE*sizeof(_a_stco[0]);
		_pSink->Seek(skip_length, SEEK_CUR);
		_curPos += skip_length;
	}
	put_buffer((AM_U8 *)_a_stco, current_idx_cnt * sizeof(_a_stco[0]));

}

