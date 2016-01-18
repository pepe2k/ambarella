/*
 * amba_create_mp4.h
 *
 * History:
 *  2009/5/26 - [Jacky Zhang] created file
 *	2011/2/24 - [Yi Zhu] modified
 *  2011/5/12 - [Hanbo Xiao] modified
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_CREATE_MP4_H__
#define __AMBA_CREATE_MP4_H__

class ISeekableSink;
class CThread;

#define	IDX_DUMP_SIZE	256
#define	CLOCK	90000

class CCreateMp4
{
public:
	CCreateMp4(char* name, AM_INT stream_id);
	~CCreateMp4();

	AM_ERR Init(CMP4MUX_H264_INFO* h264_info,
		CMP4MUX_AUDIO_INFO* audio_info,
		CMP4MUX_RECORD_INFO* record_info);
	AM_ERR put_VideoData(AM_U8* pData, AM_UINT size, AM_PTS pts);
	AM_ERR put_AudioData(AM_U8* pData, AM_UINT size);
	//AM_ERR Finish(bool directly);
	AM_ERR Finish();
	void Delete();

public:
	static AM_ERR set_h264_config(iav_h264_config_t *);
	static void destory_h264_config();

protected:
	AM_ERR put_byte(AM_UINT data);
	AM_ERR put_be16(AM_UINT data);
	AM_ERR put_be24(AM_UINT data);
	AM_ERR put_be32(AM_UINT data);
	AM_ERR put_be64(AM_U64 data);
	AM_ERR put_buffer(AM_U8 *pdata, AM_UINT size);
	AM_ERR put_boxtype(const char* pdata);
	AM_UINT get_byte();
	AM_UINT get_be16();
	AM_UINT get_be32();
	void get_buffer(AM_U8 *pdata, AM_UINT size);
	AM_UINT le_to_be32(AM_UINT data);

	AM_UINT read_bit(AM_U8* pBuffer,  AM_INT* value, AM_U8* bit_pos, AM_UINT num);
	AM_INT parse_scaling_list(AM_U8* pBuffer,AM_UINT sizeofScalingList,AM_U8* bit_pos);
	AM_UINT parse_exp_codes(AM_U8* pBuffer, AM_INT* value, AM_U8* bit_pos, AM_U8 type);

	AM_ERR get_time(AM_UINT* time_since1904, char * time_str,  int len);
	AM_UINT	get_nal_unit(AM_U8* nal_unit_type, AM_U8* pBuffer, AM_UINT size);
	AM_ERR get_h264_info(unsigned char* pBuffer, int size, CMP4MUX_H264_INFO* pH264Info);
	AM_UINT get_aac_info(AM_UINT samplefreq, AM_UINT channel);

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

	static AM_ERR FinishThreadEntry(void *p);
	AM_ERR FinishProcess();

private:
	AM_BOOL _init;
	AM_U32 _curPos;
	char _fileName[256];
	//AM_U32 _dts_delta;
	AM_U32 _video_duration;
	AM_U32 _audio_duration;

	AM_UINT	_v_stsz[IDX_DUMP_SIZE];	//sample size
	AM_UINT	_v_stco[IDX_DUMP_SIZE];	//chunk_offset
	AM_UINT	_a_stsz[IDX_DUMP_SIZE];	//sample size
	AM_UINT	_a_stco[IDX_DUMP_SIZE];	//chunk_offset
	AM_UINT	_ctts[2 * IDX_DUMP_SIZE];	//composition time
	AM_UINT	_stts[2 * IDX_DUMP_SIZE];	//decoding time

	AM_UINT _last_ctts;
	AM_U32	_video_cnt;
	AM_U32	_audio_cnt;
	AM_U32	_stss[IDX_DUMP_SIZE]; //sync sample
	AM_U32	_stss_cnt;

	AM_U8* _pps;
	AM_U32 _pps_size;
	AM_U8* _sps;
	AM_U32 _sps_size;
	AM_U32 _create_time;
	AM_U32 _decimal_pts;

	ISeekableSink *_pSink;
	ISeekableSink *_pIdxSink;
	
	ISeekableSink *_pVSink;
	ISeekableSink *_pASink;
	ISeekableSink *_pNSink;
	int _p_fd;

	AM_U32 _v_stts_pos;
	AM_U32 _v_ctts_pos;
	AM_U32 _v_stsz_pos;
	AM_U32 _v_stco_pos;
	AM_U32 _v_stss_pos;

	AM_U32 _a_stsz_pos;
	AM_U32 _a_stco_pos;

	AM_U32 _mdat_begin_pos;
	AM_U32 _mdat_end_pos;

	AM_PTS _last_pts;
	CMP4MUX_H264_INFO _h264_info;
	CMP4MUX_AUDIO_INFO _audio_info;
	CMP4MUX_RECORD_INFO _record_info;

	// static AM_U16 _video_width;
	// static AM_U16 _video_height;
	static iav_h264_config_t *_p_h264_config;
	//CThread* _pFinishThread;
};

#endif
