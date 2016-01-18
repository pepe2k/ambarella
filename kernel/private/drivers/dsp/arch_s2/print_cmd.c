
/*
 * print_cmd.c
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#define cast_cmd(type)	type *dsp_cmd = (type *)cmd

#define PRINTK_FLAG	KERN_DEBUG

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#define cprint(_cmd) \
	do { \
		if (default_cmd) \
			DRV_PRINT(PRINTK_FLAG "\ndsp_default_cmd[%d] " #_cmd " (0x%x)\n", counter++, _cmd); \
		else \
			DRV_PRINT(PRINTK_FLAG "\ndsp_cmd [%d] " #_cmd " (0x%x)\n", counter++, _cmd); \
	} while (0)

#define mprint(_m)	DRV_PRINT(PRINTK_FLAG "\t" #_m " = %d\n", (u32)dsp_cmd->_m)
#define mprintx(_m)	DRV_PRINT(PRINTK_FLAG "\t" #_m " = 0x%x\n", (u32)dsp_cmd->_m)
#define mprinta(_m, _i)		DRV_PRINT(PRINTK_FLAG "\t" #_m"[%d] = %d\n", _i, (u32)dsp_cmd->_m[_i])
#define mmprint(_m, _mm)	DRV_PRINT(PRINTK_FLAG "\t" #_m"." # _mm" = %d\n", (u32)dsp_cmd->_m._mm)

#define aprint(_arr) \
	do { \
		int i; \
		for (i = 0; i < sizeof(dsp_cmd->_arr)/sizeof(dsp_cmd->_arr[0]); i++) \
			mprinta(_arr, i); \
	} while (0)

#define asprint_begin(_arr, _max) \
	do { \
		unsigned int i; \
		for (i = 0; i < dsp_cmd->_max; i++) { \
			DRV_PRINT(PRINTK_FLAG "\t" #_arr "[%d]:\n", i) \

#define asprint_end(_arr) \
		} \
	} while (0)

void print_cmd(void *cmd, u32 size, int default_cmd)
{
	static int counter = 0;
	u32 cmd_code = *(u32*)cmd;

	switch (cmd_code) {
		case CMD_DSP_HEADER: {
				cast_cmd(DSP_HEADER_CMD);
				cprint(CMD_DSP_HEADER);
				mprintx(cmd_code);
				mprint(cmd_seq_num);
				mprint(num_cmds);
			}
			break;

		case CMD_DSP_SET_OPERATION_MODE: {
				cast_cmd(DSP_SET_OP_MODE_CMD);
				cprint(CMD_DSP_SET_OPERATION_MODE);
				mprintx(cmd_code);
				mprint(dsp_op_mode);
			}
			break;

		case CMD_DSP_ACTIVATE_OPERATION_MODE: {
				cast_cmd(DSP_ACTIVATE_OP_MODE_CMD);
				cprint(CMD_DSP_ACTIVATE_OPERATION_MODE);
				mprintx(cmd_code);
			}
			break;

		case CMD_DSP_SUSPEND_OPERATION_MODE: {
				cast_cmd(DSP_SUSPEND_OP_MODE_CMD);
				cprint(CMD_DSP_SUSPEND_OPERATION_MODE);
				mprintx(cmd_code);
			}
			break;

		case CMD_DSP_SET_DEBUG_LEVEL: {
				cast_cmd(DSP_SET_DEBUG_LEVEL_CMD);
				cprint(CMD_DSP_SET_DEBUG_LEVEL);
				mprintx(cmd_code);
				mprint(module);
				mprint(add_or_set);
				mprint(debug_mask);
			}
			break;

		case CMD_H264ENC_SETUP: {
				cast_cmd(ENCODER_SETUP_CMD);
				cprint(CMD_H264ENC_SETUP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(profile_idc);
				mprint(level_idc);
				mprint(coding_type);
				mprint(scalelist_opt);
				mprint(force_intlc_tb_iframe);
				mprint(encode_w_sz);
				mprint(encode_h_sz);
				mprint(encode_w_ofs);
				mprint(encode_h_ofs);
				mprint(aff_mode);
				mprint(M);
				mprint(N);
				mprint(gop_structure);
				mprint(numRef_P);
				mprint(numRef_B);
				mprint(use_cabac);
				mprint(quality_level);
				mprint(average_bitrate);
				mprint(vbr_cntl);
				mprint(vbr_setting);
				mprint(allow_I_adv);
				mprint(cpb_buf_idc);
				mprint(en_panic_rc);
				mprint(cpb_cmp_idc);
				mprint(fast_rc_idc);
				mprint(target_storage_space);
				mprintx(bits_fifo_base);
				mprintx(bits_fifo_limit);
				mprintx(info_fifo_base);
				mprintx(info_fifo_limit);
				mprint(audio_in_freq);
				mprint(encode_frame_rate);
				mprint(frame_sync);
				mprint(initial_fade_in_gain);
				mprint(final_fade_out_gain);
				mprint(idr_interval);
				mprint(cpb_user_size);
				mprint(stat_fifo_base);
				mprint(stat_fifo_limit);
				mprint(follow_gop);
				mprint(fgop_max_M);
				mprint(fgop_max_N);
				mprint(fgop_min_N);
				mprint(me_config);
				mprint(share_smem_option);
				mprint(share_smem_max_width);
			}
			break;

		case CMD_H264ENC_START: {
				cast_cmd(H264ENC_START_CMD);
				cprint(CMD_H264ENC_START);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(bits_fifo_next);
				mprint(info_fifo_next);
				mprint(start_encode_frame_no);
				mprint(encode_duration);
				mprint(is_flush);
				mprint(enable_slow_shutter);
				mprint(res_rate_min);
				mprint(alpha);
				mprint(beta);
				mprint(en_loop_filter);
				mprint(max_upsampling_rate);
				mprint(slow_shutter_upsampling_rate);
				mprint(firstGOPstartB);
				mprint(I_IDR_sp_rc_handle_mask);
				mprint(IDR_QP_adj_str);
				mprint(IDR_class_adj_limit);
				mprint(I_QP_adj_str);
				mprint(I_class_adj_limit);
				mprint(frame_cropping_flag);
				mprint(frame_crop_left_offset);
				mprint(frame_crop_right_offset);
				mprint(frame_crop_top_offset);
				mprint(frame_crop_bottom_offset);
				mprint(max_bytes_per_pic_denom);
				mprint(max_bits_per_mb_denom);
				mprint(sony_avc);
				mprint(au_type);
				//mprint(h264_vui_par);
			}
			break;

		case CMD_H264ENC_STOP: {
				cast_cmd(ENCODER_STOP_CMD);
				cprint(CMD_H264ENC_STOP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(stop_method);
			}
			break;

		case CMD_H264ENC_UPDATE_ON_DEMAND_IDR: {
				cast_cmd(ENCODER_UPDATE_ON_DEMAND_IDR_CMD);
				cprint(CMD_H264ENC_UPDATE_ON_DEMAND_IDR);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(on_demand_IDR);
				mprint(PTS_to_change_to_IDR);
			}
			break;

		case CMD_H264ENC_UPDATE_BITRATE_CHANGE: {
				cast_cmd(ENCODER_UPDATE_BITRATE_CHANGE_CMD);
				cprint(CMD_H264ENC_UPDATE_BITRATE_CHANGE);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(average_bitrate);
				mprint(PTS_to_change_bitrate);
			}
			break;

		case CMD_H264ENC_UPDATE_GOP_STRUCTURE: {
				cast_cmd(ENCODER_UPDATE_GOP_STRUCTURE_CMD);
				cprint(CMD_H264ENC_UPDATE_GOP_STRUCTURE);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(change_gop_option);
				mprint(follow_gop);
				mprint(fgop_max_N);
				mprint(fgop_min_N);
				mprint(M);
				mprint(N);
				mprint(gop_structure);
				mprint(idr_interval);
				mprint(PTS_to_change_gop);
			}
			break;

		case CMD_H264DEC_SETUP: {
				cast_cmd(H264DEC_SETUP_CMD);
				cprint(CMD_H264DEC_SETUP);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(decode_type);
				mprint(enable_pic_info);
				mprint(use_tiled_dram);
				mprint(bits_fifo_base);
				mprint(bits_fifo_limit);
				mprint(rbuf_smem_size);
				mprint(fbuf_dram_size);
				mprint(pjpeg_buf_size);
				mprint(cabac_2_recon_delay);
				mprint(force_fld_tiled);
				mprint(ec_mode);
			}
			break;

		case CMD_H264DEC_DECODE: {
				cast_cmd(H264DEC_DECODE_CMD);
				cprint(CMD_H264DEC_DECODE);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprintx(bits_fifo_start);
				mprintx(bits_fifo_end);
				mprint(num_pics);
				mprint(num_frame_decode);
				mprint(first_frame_display);
			}
			break;

		case CMD_H264DEC_STOP: {
				cast_cmd(H264DEC_STOP_CMD);
				cprint(CMD_H264DEC_STOP);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(stop_flag);
			}
			break;

		case CMD_H264DEC_BITSFIFO_UPDATE: {
				cast_cmd(H264DEC_BITS_FIFO_UPDATE_CMD);
				cprint(CMD_H264DEC_BITSFIFO_UPDATE);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(bits_fifo_start);
				mprint(bits_fifo_end);
				mprint(num_pics);
			}
			break;

		case CMD_H264DEC_SPEED: {
				cast_cmd(H264DEC_PLAYBACK_SPEED_CMD);
				cprint(CMD_H264DEC_SPEED);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(speed);
				mprint(scan_mode);
				mprint(direction);
			}
			break;

		case CMD_H264DEC_TRICKPLAY: {
				cast_cmd(H264DEC_TRICKPLAY_CMD);
				cprint(CMD_H264DEC_TRICKPLAY);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(mode);
			}
			break;

		case CMD_H264DEC_STAT_ENABLE: {
				cast_cmd(H264DEC_STAT_ENABLE_CMD);
				cprint(CMD_H264DEC_STAT_ENABLE);
				mprintx(cmd_code);
				mprint(decoder_id);
				mprint(enable_pic_info_msg);
				mprint(enable_coded_pic_dadder_in_bs);
				mprint(enable_sps_daddr_in_bs);
				mprint(enable_pic_stat_daddr);
				mprint(enable_mb_info);
			}
			break;

		case CMD_VOUT_MIXER_SETUP: {
				cast_cmd(VOUT_MIXER_SETUP_CMD);
				cprint(CMD_VOUT_MIXER_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(interlaced);
				mprint(frm_rate);
				mprint(act_win_width);
				mprint(act_win_height);
				mprint(back_ground_v);
				mprint(back_ground_u);
				mprint(back_ground_y);
				mprint(mixer_444);
				mprint(highlight_v);
				mprint(highlight_u);
				mprint(highlight_y);
				mprint(highlight_thresh);
				mprint(reverse_en);
				mprint(csc_en);
				aprint(csc_parms);
			}
			break;

		case CMD_VOUT_VIDEO_SETUP: {
				cast_cmd(VOUT_VIDEO_SETUP_CMD);
				cprint(CMD_VOUT_VIDEO_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(en);
				mprint(src);
				mprint(flip);
				mprint(rotate);
				mprint(win_offset_x);
				mprint(win_offset_y);
				mprint(win_width);
				mprint(win_height);
				mprint(default_img_y_addr);
				mprint(default_img_uv_addr);
				mprint(default_img_pitch);
				mprint(default_img_repeat_field);
			}
			break;

		case CMD_VOUT_DEFAULT_IMG_SETUP: {
				cast_cmd(VOUT_DEFAULT_IMG_SETUP_CMD);
				cprint(CMD_VOUT_DEFAULT_IMG_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(default_img_y_addr);
				mprint(default_img_uv_addr);
				mprint(default_img_pitch);
				mprint(default_img_repeat_field);
			}
			break;

		case CMD_VOUT_OSD_SETUP: {
				cast_cmd(VOUT_OSD_SETUP_CMD);
				cprint(CMD_VOUT_OSD_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(en);
				mprint(src);
				mprint(flip);
				mprint(rescaler_en);
				mprint(premultiplied);
				mprint(global_blend);
				mprint(win_offset_x);
				mprint(win_offset_y);
				mprint(win_width);
				mprint(win_height);
				mprint(rescaler_input_width);
				mprint(rescaler_input_height);
				mprint(osd_buf_dram_addr);
				mprint(osd_buf_pitch);
				mprint(osd_buf_repeat_field);
				mprint(osd_direct_mode);
				mprint(osd_transparent_color);
				mprint(osd_transparent_color_en);
				mprint(osd_swap_bytes);
				mprintx(osd_buf_info_dram_addr);
			}
			break;

		case CMD_VOUT_OSD_BUFFER_SETUP: {
				cast_cmd(VOUT_OSD_BUF_SETUP_CMD);
				cprint(CMD_VOUT_OSD_BUFFER_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(osd_buf_dram_addr);
				mprint(osd_buf_pitch);
				mprint(osd_buf_repeat_field);
			}
			break;

		case CMD_VOUT_OSD_CLUT_SETUP: {
				cast_cmd(VOUT_OSD_CLUT_SETUP_CMD);
				cprint(CMD_VOUT_OSD_CLUT_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(clut_dram_addr);
			}
			break;

		case CMD_VOUT_DISPLAY_SETUP: {
				cast_cmd(VOUT_DISPLAY_SETUP_CMD);
				cprint(CMD_VOUT_DISPLAY_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprintx(disp_config_dram_addr);
			}
			break;

		case CMD_VOUT_DVE_SETUP: {
				cast_cmd(VOUT_DVE_SETUP_CMD);
				cprint(CMD_VOUT_DVE_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(dve_config_dram_addr);
			}
			break;

		case CMD_VOUT_RESET: {
				cast_cmd(VOUT_RESET_CMD);
				cprint(CMD_VOUT_RESET);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(reset_mixer);
				mprint(reset_disp);
			}
			break;

		case CMD_VOUT_DISPLAY_CSC_SETUP: {
				cast_cmd(VOUT_DISPLAY_CSC_SETUP_CMD);
				cprint(CMD_VOUT_DISPLAY_CSC_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(csc_type);
				aprint(csc_parms);
			}
			break;

		case CMD_VOUT_DIGITAL_OUTPUT_MODE_SETUP: {
				cast_cmd(VOUT_DIGITAL_OUTPUT_MODE_SETUP_CMD);
				cprint(CMD_VOUT_DIGITAL_OUTPUT_MODE_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(output_mode);
			}
			break;

		case CMD_VOUT_GAMMA_SETUP: {
				cast_cmd(VOUT_GAMMA_SETUP_CMD);
				cprint(CMD_VOUT_GAMMA_SETUP);
				mprintx(cmd_code);
				mprint(vout_id);
				mprint(enable);
				mprint(setup_gamma_table);
				mprint(gamma_dram_addr);
			}
			break;

		case CMD_VCAP_SETUP: {
				cast_cmd(VCAP_SETUP_CMD);
				cprint(CMD_VCAP_SETUP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(vid_skip);
				mprint(input_format);
				mprint(sensor_id);
				mprint(keep_states);
				mprint(interlaced_output);
				mprint(vidcap_w);
				mprint(vidcap_h);
				mprint(main_w);
				mprint(main_h);
				mprint(preview_w_A);
				mprint(preview_h_A);
				mprint(input_center_x);
				mprint(input_center_y);
				mprint(zoom_factor_x);
				mprint(zoom_factor_y);
				mprint(CmdReadDly);
				mprint(sensor_readout_mode);
				mprint(noise_filter_strength);
				mprint(mctf_chan);
				mprint(sharpen_b_chan);
				mprint(cc_en);
				mprint(cmpr_en);
				mprint(cmpr_dither);
				mprint(mode);
				mprint(image_stabilize_strength);
				mprint(bit_resolution);
				mprint(bayer_pattern);
				mprint(preview_format_A);
				mprint(preview_format_B);
				mprint(no_pipelineflush);
				mprint(preview_frame_rate_A);
				mprint(preview_w_B);
				mprint(preview_h_B);
				mprint(preview_frame_rate_B);
				mprint(preview_A_src);
				mprint(preview_B_src);
				mprint(nbr_prevs_x);
				mprint(nbr_prevs_y);
				mprint(vin_frame_rate);
				mprint(idsp_out_frame_rate);
				mprint(pip_w);
				mprint(pip_h);
				mprint(vcap_cmd_msg_dec_rate);
			}
			break;

		case CMD_VCAP_SET_ZOOM: {
				cast_cmd(VCAP_SET_ZOOM_CMD);
				cprint(CMD_VCAP_SET_ZOOM);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(zoom_x);
				mprint(zoom_y);
				mprint(x_center_offset);
				mprint(y_center_offset);
			}
			break;

		case CMD_VCAP_PREV_SETUP: {
				cast_cmd(VCAP_PREV_SETUP_CMD);
				cprint(CMD_VCAP_PREV_SETUP);
				mprintx(cmd_code);
				mprint(preview_id);
				mprint(preview_format);
				mprint(preview_w);
				mprint(preview_h);
				mprint(preview_frame_rate);
				mprint(preview_src);
			}
			break;

		case CMD_VCAP_MCTF_MV_STAB: {
				cast_cmd(VCAP_MCTF_MV_STAB_CMD);
				cprint(CMD_VCAP_MCTF_MV_STAB);
				mprintx(cmd_code);
				mprint(noise_filter_strength);
				mprint(mctf_chan);
				mprint(sharpen_b_chan);
				mprint(cc_en);
				mprint(cmpr_en);
				mprint(cmpr_dither);
				mprint(mode);
				mprint(image_stabilize_strength);
				mprint(bitrate_y);
				mprint(bitrate_uv);
				mprint(use_zmv_as_predictor);
				mprint(mctf_cfg_dbase);
				mprint(cc_cfg_dbase);
				mprint(cmpr_cfg_dbase);
				//mprint(cmds);
				//mprint(word);
			}
			break;

		case CMD_VCAP_TMR_MODE: {
				cast_cmd(VCAP_TMR_MODE_CMD);
				cprint(CMD_VCAP_TMR_MODE);
				mprintx(cmd_code);
			}
			break;

		case CMD_VCAP_CTRL: {
				cast_cmd(VCAP_CTRL_CMD);
				cprint(CMD_VCAP_CTRL);
				mprintx(cmd_code);
				mprint(ctrl_code);
				mprint(par_1);
				mprint(par_2);
				mprint(par_3);
				mprint(par_4);
				mprint(par_5);
				mprint(par_6);
				mprint(par_7);
				mprint(par_8);
			}
			break;

		case CMD_VCAP_STILL_CAP: {
				cast_cmd(VCAP_STILL_CAP_CMD);
				cprint(CMD_VCAP_STILL_CAP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(still_process_mode);
				mprint(output_select);
				mprint(input_format);
				mprint(vsync_skip);
				mprint(number_frames_to_capture);
				mprint(vidcap_w);
				mprint(vidcap_h);
				mprint(main_w);
				mprint(main_h);
				mprint(encode_w);
				mprint(encode_h);
				mprint(preview_w);
				mprint(preview_h);
				mprint(thumbnail_w_active);
				mprint(thumbnail_h_active);
				mprint(thumbnail_w_dram);
				mprint(thumbnail_h_dram);
				mprint(jpeg_bit_fifo_start);
				mprint(jpeg_bit_info_fifo_start);
				mprint(sensor_readout_mode);
				mprint(sensor_id);
				mprint(sensor_resolution);
				mprint(sensor_pattern);
				mprint(preview_w_B);
				mprint(preview_h_B);
				mprint(raw_cap_cntl);
				mprint(raw2yuv_proc_mode);
				mprint(jpeg_encode_cntl);
				mprint(raw_capture_resource_ptr);
				mprint(hiLowISO_proc_cfg_ptr);
				mprint(preview_control);
				mprint(screen_thumbnail_active_w);
				mprint(screen_thumbnail_active_h);
				mprint(screen_thumbnail_dram_w);
				mprint(screen_thumbnail_dram_h);
			}
			break;

		case CMD_VCAP_STILL_CAP_IN_REC: {
				cast_cmd(VCAP_STILL_CAP_IN_REC_CMD);
				cprint(CMD_VCAP_STILL_CAP_IN_REC);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(main_jpg_w);
				mprint(main_jpg_h);
				mprint(encode_jpg_w);
				mprint(encode_jpg_h);
				mprint(encode_x_ctr_offset);
				mprint(encode_y_ctr_offset);
				mprint(thumbnail_w_active);
				mprint(thumbnail_h_active);
				mprint(thumbnail_w_dram);
				mprint(thumbnail_h_dram);
				mprint(blank_period_duration);
				mprint(is_use_compaction);
				mprint(screen_thumbnail_active_w);
				mprint(screen_thumbnail_active_h);
				mprint(screen_thumbnail_dram_w);
				mprint(screen_thumbnail_dram_h);
			}
			break;

		case CMD_VCAP_STILL_PROC_MEM: {
				cast_cmd(VCAP_STILL_PROC_MEM_CMD);
				cprint(CMD_VCAP_STILL_PROC_MEM);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(still_process_mode);
				mprint(output_select);
				mprint(input_format);
				mprint(bayer_pattern);
				mprint(resolution);
				mprint(input_address);
				mprint(input_chroma_address);
				mprint(input_pitch);
				mprint(input_chroma_pitch);
				mprint(input_w);
				mprint(input_h);
				mprint(main_w);
				mprint(main_h);
				mprint(encode_w);
				mprint(encode_h);
				mprint(encode_x_ctr_offset);
				mprint(encode_y_ctr_offset);
				mprint(thumbnail_w_active);
				mprint(thumbnail_h_active);
				mprint(thumbnail_w_dram);
				mprint(thumbnail_h_dram);
				mprint(preview_w_A);
				mprint(preview_h_A);
				mprint(preview_w_B);
				mprint(preview_h_B);
				mprint(hiLowISO_proc_cfg_ptr);
				mprint(preview_control);
				mprint(screen_thumbnail_active_w);
				mprint(screen_thumbnail_active_h);
				mprint(screen_thumbnail_dram_w);
				mprint(screen_thumbnail_dram_h);
			}
			break;

		case CMD_VCAP_INTERVAL_CAP: {
				cast_cmd(VCAP_INTERVAL_CAP_CMD);
				cprint(CMD_VCAP_INTERVAL_CAP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(mode);
				mprint(interval_cap_act);
				mprint(frames_to_cap);
			}
			break;

		case CMD_VCAP_MCTF_GMV: {
				cast_cmd(VCAP_MCTF_GMV_CMD);
				cprint(CMD_VCAP_MCTF_GMV);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(enable);
				mprint(gmv);
			}
			break;

		case CMD_VCAP_TRANSCODE_SETUP: {
				cast_cmd(VCAP_TRANSCODE_SETUP_CMD);
				cprint(CMD_VCAP_TRANSCODE_SETUP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(bm_noise_filter_strength);
				mprint(bm_look_ahead_buffer_depth);
				mprint(bm_flow_ctrl_option);
				mprint(bm_underflow_action_option);
				mprint(noise_filter_strength);
				mprint(look_ahead_buffer_depth);
				mprint(flow_ctrl_option);
				mprint(underflow_action_option);
			}
			break;

		case CMD_VCAP_TRANSCODE_CHAN_SW: {
				cast_cmd(VCAP_TRANSCODE_CHAN_SW_CMD);
				cprint(CMD_VCAP_TRANSCODE_CHAN_SW);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(new_input_channel);
				mprint(new_input_stream_type);
				mprint(switch_mode);
				mprint(switch_params_0);
				mprint(switch_params_1);
				mprint(switch_params_2);
				mprint(switch_params_3);
			}
			break;

		case CMD_VCAP_TRANSCODE_CHAN_STOP: {
				cast_cmd(VCAP_TRANSCODE_CHAN_STOP_CMD);
				cprint(CMD_VCAP_TRANSCODE_CHAN_STOP);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
			}
			break;

		case CMD_VCAP_TRANSCODE_PREV_SETUP: {
				cast_cmd(VCAP_TRANSCODE_PREV_SETUP_CMD);
				cprint(CMD_VCAP_TRANSCODE_PREV_SETUP);
				mprintx(cmd_code);
				mprint(is_set_prev_a);
				mprint(is_set_prev_b);
				mprint(is_prev_a_interlaced);
				mprint(is_prev_b_interlaced);
				mprint(is_prev_a_vout_th_mode);
				mprint(is_prev_b_vout_th_mode);
				mprint(prev_a_src);
				mprint(prev_b_src);
				mprint(prev_a_chan_id);
				mprint(prev_a_strm_tp);
				mprint(prev_b_chan_id);
				mprint(prev_b_strm_tp);
				mprint(preview_w_A);
				mprint(preview_h_A);
				mprint(preview_w_B);
				mprint(preview_h_B);
			}
			break;

		case CMD_POSTPROC: {
				cast_cmd(POSTPROC_CMD);
				cprint(CMD_POSTPROC);
				mprintx(cmd_code);
				mprint(decode_cat_id);
				mprint(decode_id);
				mprint(voutA_enable);
				mprint(voutB_enable);
				mprint(pip_enable);
				mprint(input_center_x);
				mprint(input_center_y);
				mprint(voutA_target_win_offset_x);
				mprint(voutA_target_win_offset_y);
				mprint(voutA_target_win_width);
				mprint(voutA_target_win_height);
				mprint(voutA_zoom_factor_x);
				mprint(voutA_zoom_factor_y);
				mprint(voutB_target_win_offset_x);
				mprint(voutB_target_win_offset_y);
				mprint(voutB_target_win_width);
				mprint(voutB_target_win_height);
				mprint(voutB_zoom_factor_x);
				mprint(voutB_zoom_factor_y);
				mprint(pip_target_offset_x);
				mprint(pip_target_offset_y);
				mprint(pip_target_x_size);
				mprint(pip_target_y_size);
				mprint(pip_zoom_factor_x);
				mprint(pip_zoom_factor_y);
			}
			break;

		case CMD_POSTP_YUV2YUV: {
				cast_cmd(POSTP_YUV2YUV_CMD);
				cprint(CMD_POSTP_YUV2YUV);
				mprintx(cmd_code);
				mprint(decode_cat_id);
				mprint(decode_id);
				mprint(fir_pts_low);
				mprint(fir_pts_high);
				aprint(matrix_values);
				mprint(y_offset);
				mprint(u_offset);
				mprint(v_offset);
			}
			break;

		case CMD_STILL_MULTIS_SETUP: {
				cast_cmd(STILL_MULTIS_SETUP_CMD);
				cprint(CMD_STILL_MULTIS_SETUP);
				mprintx(cmd_code);
				mprint(if_save_thumbnail);
				mprint(total_thumbnail);
				mprint(saving_thumbnail_width);
				mprint(saving_thumbnail_height);
				mprint(if_save_large_thumbnail);
				mprint(total_large_thumbnail);
				mprint(saving_large_thumbnail_width);
				mprint(saving_large_thumbnail_height);
				mprint(if_capture_large_size_thumb);
				mprint(large_thumbnail_pic_width);
				mprint(large_thumbnail_pic_height);
				mprint(visual_effect_type);
				//mprint(extra_total_thumbnail);
			}
			break;

		case CMD_STILL_MULTIS_DECODE: {
				cast_cmd(STILL_MULTIS_DECODE_CMD);
				cprint(CMD_STILL_MULTIS_DECODE);
				mprintx(cmd_code);
				mprint(total_scenes);
				mprint(start_scene_num);
				mprint(scene_num);
				mprint(end);
				mprint(update_lcd_only);
				mprint(buffer_source_only);
				mprint(fast_mode);
				mprint(use_single_prev_filter);
				//aprint(scene);
			}
			break;

		case CMD_STILL_CAPTURE_STILL: {
				cast_cmd(VCAP_STILL_CAP_CMD);
				cprint(CMD_STILL_CAPTURE_STILL);
				mprintx(cmd_code);
				mprint(channel_id);
				mprint(stream_type);
				mprint(still_process_mode);
				mprint(output_select);
				mprint(input_format);
				mprint(vsync_skip);
				mprint(number_frames_to_capture);
				mprint(vidcap_w);
				mprint(vidcap_h);
				mprint(main_w);
				mprint(main_h);
				mprint(encode_w);
				mprint(encode_h);
				mprint(preview_w);
				mprint(preview_h);
				mprint(thumbnail_w_active);
				mprint(thumbnail_h_active);
				mprint(thumbnail_w_dram);
				mprint(thumbnail_h_dram);
				mprint(jpeg_bit_fifo_start);
				mprint(jpeg_bit_info_fifo_start);
				mprint(sensor_readout_mode);
				mprint(sensor_id);
				mprint(sensor_resolution);
				mprint(sensor_pattern);
				mprint(preview_w_B);
				mprint(preview_h_B);
				mprint(raw_cap_cntl);
				mprint(raw2yuv_proc_mode);
				mprint(jpeg_encode_cntl);
				mprint(raw_capture_resource_ptr);
				mprint(hiLowISO_proc_cfg_ptr);
				mprint(preview_control);
				mprint(screen_thumbnail_active_w);
				mprint(screen_thumbnail_active_h);
				mprint(screen_thumbnail_dram_w);
				mprint(screen_thumbnail_dram_h);
			}
			break;

		case SENSOR_INPUT_SETUP: {
				cast_cmd(sensor_input_setup_t);
				cprint(SENSOR_INPUT_SETUP);
				mprint(sensor_id);
				mprint(sensor_resolution);
				mprint(sensor_pattern);
				mprint(first_line_field_0);
				mprint(first_line_field_1);
				mprint(first_line_field_2);
				mprint(first_line_field_3);
				mprint(first_line_field_4);
				mprint(first_line_field_5);
				mprint(first_line_field_6);
				mprint(first_line_field_7);
			}
			break;

		case SET_VIN_CAPTURE_WIN: {
				cast_cmd(vin_cap_win_t);
				cprint(SET_VIN_CAPTURE_WIN);
				mprintx(s_ctrl_reg);
				mprintx(s_inp_cfg_reg);
				mprintx(s_v_width_reg);
				mprintx(s_h_width_reg);
				mprintx(s_h_offset_top_reg);
				mprintx(s_h_offset_bot_reg);
				mprintx(s_v_reg);
				mprintx(s_h_reg);
				mprintx(s_min_v_reg);
				mprintx(s_min_h_reg);
				mprintx(s_trigger_0_start_reg);
				mprintx(s_trigger_0_end_reg);
				mprintx(s_trigger_1_start_reg);
				mprintx(s_trigger_1_end_reg);
				mprintx(s_vout_start_0_reg);
				mprintx(s_vout_start_1_reg);
				mprintx(s_cap_start_v_reg);
				mprintx(s_cap_start_h_reg);
				mprintx(s_cap_end_v_reg);
				mprintx(s_cap_end_h_reg);
				mprintx(s_blank_leng_h_reg);
				mprintx(vsync_timeout);
				mprintx(hsync_timeout);
			}
			break;

		case SET_WARP_CONTROL: {
				cast_cmd(set_warp_control_t);
				cprint(SET_WARP_CONTROL);
				mprint(warp_control);
				mprintx(warp_horizontal_table_address);
				mprintx(warp_vertical_table_address);
				mprint(actual_left_top_x);
				mprint(actual_left_top_y);
				mprint(actual_right_bot_x);
				mprint(actual_right_bot_y);
				mprintx(zoom_x);
				mprintx(zoom_y);
				mprint(x_center_offset);
				mprint(y_center_offset);
				mprint(grid_array_width);
				mprint(grid_array_height);
				mprint(horz_grid_spacing_exponent);
				mprint(vert_grid_spacing_exponent);
				mprint(vert_warp_enable);
				mprint(vert_warp_grid_array_width);
				mprint(vert_warp_grid_array_height);
				mprint(vert_warp_horz_grid_spacing_exponent);
				mprint(vert_warp_vert_grid_spacing_exponent);
			}
			break;

		case SET_VIN_CONFIG: {
				cast_cmd(set_vin_config_t);
				cprint(SET_VIN_CONFIG);
				mprint(vin_width);
				mprint(vin_height);
				mprintx(vin_config_dram_addr);
				mprintx(config_data_size);
				mprint(sensor_resolution);
				mprint(sensor_bayer_pattern);
			}
			break;

		case CMD_DSP_SET_DEBUG_THREAD: {
				cast_cmd(DSP_SET_DEBUG_THREAD_CMD);
				cprint(CMD_DSP_SET_DEBUG_THREAD);
				mprintx(thread_mask);
			}
			break;

		case CMD_UDEC_INIT: {
				cast_cmd(UDEC_INIT_CMD);
				cprint(CMD_UDEC_INIT);
				mprint(decoder_id);
				mprint(dsp_dram_sp_id);
				mprint(tiled_mode);
				mprint(frm_chroma_fmt_max);
				mprint(dec_types);
				mprint(max_frm_num);
				mprint(max_frm_width);
				mprint(max_frm_height);
				mprintx(max_fifo_size);
				//mprint(no_fmo);
//				mprint(dec_dram_size);
			}
			break;

		case CMD_UDEC_SETUP: {
				cast_cmd(UDEC_SETUP_BASE_CMD);
				cprint(CMD_UDEC_SETUP);
				mprint(decoder_id);
				mprint(decoder_type);
				mprint(enable_pp);
				mprint(enable_deint);
				mprint(enable_err_handle);
				mprint(validation_only);
				mprint(force_decode);
				mprint(concealment_mode);
				mprint(concealment_ref_frm_buf_id);
				mprint(bits_fifo_size);
				mprint(ref_cache_size);

				switch (dsp_cmd->decoder_type) {
				case UDEC_TYPE_H264: {
						cast_cmd(UDEC_SETUP_H264_CMD);
						mprint(pjpeg_buf_size);
					}
					break;

				case UDEC_TYPE_MP12:
				case UDEC_TYPE_MP4H: {
						cast_cmd(UDEC_SETUP_MPEG24_CMD);
						mprint(deblocking_flag);
					}
					break;

				case UDEC_TYPE_MP4S: {
						cast_cmd(UDEC_SETUP_MPEG4S_CMD);
						mprint(mv_fifo_size);
						mprint(deblocking_flag);
					}
					break;

				case UDEC_TYPE_VC1:
					break;

				case UDEC_TYPE_STILL: {
						cast_cmd(UDEC_SETUP_SDEC_CMD);
						mprint(still_bits_circular);
						mprint(still_max_decode_width);
						mprint(still_max_decode_height);
					}
					break;
				}
			}
			break;

		case CMD_POSTPROC_UDEC: {
				cast_cmd(POSTPROC_UDEC_CMD);
				cprint(CMD_POSTPROC_UDEC);

				mprint(decode_cat_id);
				mprint(decode_id);
				mprint(voutA_enable);
				mprint(voutB_enable);
				mprint(pipA_enable);
				mprint(pipB_enable);
				mprint(fir_pts_low);
				mprint(fir_pts_high);
				mprint(input_center_x);
				mprint(input_center_y);
				mprint(voutA_target_win_offset_x);
				mprint(voutA_target_win_offset_y);
				mprint(voutA_target_win_width);
				mprint(voutA_target_win_height);
				mprint(voutA_zoom_factor_x);
				mprint(voutA_zoom_factor_y);
				mprint(voutB_target_win_offset_x);
				mprint(voutB_target_win_offset_y);
				mprint(voutB_target_win_width);
				mprint(voutB_target_win_height);
				mprint(voutB_zoom_factor_x);
				mprint(voutB_zoom_factor_y);
				mprint(pipA_target_offset_x);
				mprint(pipA_target_offset_y);
				mprint(pipA_target_x_size);
				mprint(pipA_target_y_size);
				mprint(pipA_zoom_factor_x);
				mprint(pipA_zoom_factor_y);
				mprint(pipB_target_offset_x);
				mprint(pipB_target_offset_y);
				mprint(pipB_target_x_size);
				mprint(pipB_target_y_size);
				mprint(pipB_zoom_factor_x);
				mprint(pipB_zoom_factor_y);
				mprint(vout0_flip);
				mprint(vout0_rotate);
				mprint(vout0_win_offset_x);
				mprint(vout0_win_offset_y);
				mprint(vout0_win_width);
				mprint(vout0_win_height);
				mprint(vout1_flip);
				mprint(vout1_rotate);
				mprint(vout1_win_offset_x);
				mprint(vout1_win_offset_y);
				mprint(vout1_win_width);
				mprint(vout1_win_height);

				mprint(horz_warp_enable);
				mprintx(warp_horizontal_table_address);
				mprint(grid_array_width);
				mprint(grid_array_height);
				mprint(horz_grid_spacing_exponent);
				mprint(vert_grid_spacing_exponent);
			}
			break;

		case CMD_POSTP_SET_AUDIO_CLK_OFFSET: {
				cast_cmd(POSTP_SET_AUDIO_CLK_OFFSET_CMD);
				cprint(CMD_POSTP_SET_AUDIO_CLK_OFFSET);
				mprint(decoder_id);
				mprint(audio_clk_offset);
			}
			break;

		case CMD_POSTP_WAKE_VOUT: {
				cast_cmd(POSTP_WAKE_VOUT_CMD);
				cprint(CMD_POSTP_WAKE_VOUT);
				mprint(decoder_id);
			}
			break;

		case CMD_UDEC_TRICKPLAY: {
				cast_cmd(UDEC_TRICKPLAY_CMD);
				cprint(CMD_UDEC_TRICKPLAY);
				mprint(decoder_id);
				mprint(mode);
			}
			break;

		case CMD_UDEC_PLAYBACK_SPEED: {
				cast_cmd(UDEC_PLAYBACK_SPEED_CMD_t);
				cprint(CMD_UDEC_PLAYBACK_SPEED);
				mprintx(speed);
				mprint(decoder_id);
				//mprint(scan_mode);
				mprint(direction);
			}
			break;

		case CMD_UDEC_DECODE:
			{
				cast_cmd(UDEC_DEC_FIFO_CMD);
				cprint(CMD_UDEC_DECODE);
				mmprint(base_cmd, decoder_id);
				mmprint(base_cmd, decoder_type);
				mmprint(base_cmd, num_of_pics);
				mprint(bits_fifo_start);
				mprint(bits_fifo_end);
			}
			break;

		case CMD_UDEC_STOP: {
				cast_cmd(UDEC_STOP_CMD);
				cprint(CMD_UDEC_STOP);
				mprint(decoder_id);
				mprint(stop_flag);
			}
			break;

		case CMD_UDEC_EXIT: {
				cast_cmd(UDEC_EXIT_CMD);
				cprint(CMD_UDEC_EXIT);
				mprint(decoder_id);
				mprint(stop_flag);
			}
			break;

		case CMD_UDEC_CREATE: {
				cast_cmd(UDEC_CREATE_CMD);
				cprint(CMD_UDEC_CREATE);
				mprint(callback_id);
			}
			break;

//multiple window udec related
#if 0
		case CMD_UDEC_STILL_CAP: {
				cast_cmd(UDEC_STILL_CAP_CMD);
				cprint(CMD_UDEC_STILL_CAP);
				mprint(decoder_id);

				mprintx(coded_pic_base);
				mprintx(coded_pic_limit);
				mprintx(thumbnail_pic_base);
				mprintx(thumbnail_pic_limit);

				mprint(thumbnail_width);
				mprint(thumbnail_height);

				mprint(thumbnail_letterbox_strip_width);
				mprint(thumbnail_letterbox_strip_height);

				mprint(thumbnail_letterbox_strip_y);
				mprint(thumbnail_letterbox_strip_cb);
				mprint(thumbnail_letterbox_strip_cr);

				mprintx(quant_matrix_addr);

				mprint(target_pic_width);
				mprint(target_pic_height);

				mprint(pic_structure);

				mprintx(screennail_pic_base);
				mprintx(screennail_pic_limit);

				mprint(screennail_width);
				mprint(screennail_height);

				mprint(screennail_letterbox_strip_width);
				mprint(screennail_letterbox_strip_height);

				mprint(screennail_letterbox_strip_y);
				mprint(screennail_letterbox_strip_cb);
				mprint(screennail_letterbox_strip_cr);

				mprintx(quant_matrix_addr_thumbnail);
				mprintx(quant_matrix_addr_screennail);
			}
			break;

		case CMD_POSTPROC_MW_GLOBAL_CONFIG: {
				cast_cmd(POSTPROC_MW_GLOBAL_CONFIG_CMD);
				cprint(CMD_POSTPROC_MW_GLOBAL_CONFIG);
				mprint(total_num_win_configs);
				mprint(out_to_hdmi);
				mprint(av_sync_enabled);
				mprint(audio_on_win_id);
				mprint(pre_buffer_len);
				mprint(enable_buffering_ctrl);
				mprint(video_win_width);
				mprint(video_win_height);
			}
			break;

		case CMD_POSTPROC_MW_WIN_CONFIG: {
				cast_cmd(POSTPROC_MW_WIN_CONFIG_CMD);
				cprint(CMD_POSTPROC_MW_WIN_CONFIG);
				mprint(num_config);

				asprint_begin(configs, num_config);
				asprint_element(configs, win_config_id, i);
				asprint_element(configs, input_offset_x, i);
				asprint_element(configs, input_offset_y, i);
				asprint_element(configs, input_width, i);
				asprint_element(configs, input_height, i);
				asprint_element(configs, target_win_offset_x, i);
				asprint_element(configs, target_win_offset_y, i);
				asprint_element(configs, target_win_width, i);
				asprint_element(configs, target_win_height, i);
				asprint_end();
			}
			break;

		case CMD_POSTPROC_MW_RENDER_CONFIG: {
				cast_cmd(POSTPROC_MW_RENDER_CONFIG_CMD);
				cprint(CMD_POSTPROC_MW_RENDER_CONFIG);
				mprint(total_num_win_to_render);
				mprint(num_config);

				asprint_begin(configs, num_config);
				asprint_element(configs, render_id, i);
				asprint_element(configs, win_config_id, i);
				asprint_element(configs, win_config_id_2nd, i);
				asprint_element(configs, stream_id, i);
				//asprint_element(configs, first_pts_low, i);
				//asprint_element(configs, first_pts_high, i);
				asprint_element(configs, input_source_type, i);
				asprint_end();
			}
			break;

		case CMD_POSTPROC_MW_STREAM_SWITCH: {
				cast_cmd(POSTPROC_MW_STREAM_SWITCH_CMD);
				cprint(CMD_POSTPROC_MW_STREAM_SWITCH);
				mprint(num_config);

				asprint_begin(configs, num_config);
				asprint_element(configs, render_id, i);
				asprint_element(configs, new_stream_id, i);
				asprint_end();
			}
			break;

		case CMD_POSTPROC_MW_BUFFERING_CONTROL: {
				cast_cmd(POSTPROC_MW_BUFFERING_CONTROL_CMD);
				cprint(CMD_POSTPROC_MW_BUFFERING_CONTROL);
				mprint(stream_id);
				mprint(control_direction);
				mprint(frame_time);
			}
			break;

		case CMD_POSTPROC_MW_SCREEN_SHOT: {
				cast_cmd(POSTPROC_MW_SCREEN_SHOT_CMD);
				cprint(CMD_POSTPROC_MW_SCREEN_SHOT);
				mprint(udec_id);

				mprintx(coded_pic_base);
				mprintx(coded_pic_limit);
				mprintx(thumbnail_pic_base);
				mprintx(thumbnail_pic_limit);

				mprint(thumbnail_width);
				mprint(thumbnail_height);

				mprint(thumbnail_letterbox_strip_width);
				mprint(thumbnail_letterbox_strip_height);

				mprint(thumbnail_letterbox_strip_y);
				mprint(thumbnail_letterbox_strip_cb);
				mprint(thumbnail_letterbox_strip_cr);

				mprintx(quant_matrix_addr);

				mprint(target_pic_width);
				mprint(target_pic_height);

				mprintx(screennail_pic_base);
				mprintx(screennail_pic_limit);

				mprint(screennail_width);
				mprint(screennail_height);

				mprint(screennail_letterbox_strip_width);
				mprint(screennail_letterbox_strip_height);

				mprint(screennail_letterbox_strip_y);
				mprint(screennail_letterbox_strip_cb);
				mprint(screennail_letterbox_strip_cr);

				mprintx(quant_matrix_addr_thumbnail);
				mprintx(quant_matrix_addr_screennail);
			}
			break;

		case CMD_POSTPROC_MW_PLAYBACK_ZOOM: {
				cast_cmd(POSTPROC_MW_PLAYBACK_ZOOM_CMD);
				cprint(CMD_POSTPROC_MW_PLAYBACK_ZOOM);
				mprint(render_id);
				mprint(input_center_x);
				mprint(input_center_y);
				mprintx(zoom_factor_x);
				mprintx(zoom_factor_y);
				mprint(input_width);
				mprint(input_height);
			}
			break;
#endif

		case BLACK_LEVEL_GLOBAL_OFFSET :
			DRV_PRINT(KERN_DEBUG "BLACK LEVEL GLOBAL OFFSET \n");
			break;

		case BLACK_LEVEL_CORRECTION_CONFIG:
			DRV_PRINT(KERN_DEBUG "BLACK LEVEL CORRECTION CONFIG \n");
			break;

		case BLACK_LEVEL_STATE_TABLES:
			DRV_PRINT(KERN_DEBUG "BLACK LEVEL STATE TABLES \n");
			break;

		case BLACK_LEVEL_DETECTION_WINDOW:
			DRV_PRINT(KERN_DEBUG "BLACK_LEVEL_DETECTION_WINDOW \n");
			break;

		case RGB_GAIN_ADJUSTMENT:
			DRV_PRINT(KERN_DEBUG "RGB GAIN ADJUSTMENT \n");
			break;

		case DIGITAL_GAIN_SATURATION_LEVEL:
			DRV_PRINT(KERN_DEBUG "DIGITAL GAIN SATURATION LEVEL \n");
			break;

		case VIGNETTE_COMPENSATION:
			DRV_PRINT(KERN_DEBUG "VIGNETTE COMPENSATION \n");
			break;

		case LOCAL_EXPOSURE:
			DRV_PRINT(KERN_DEBUG "LOCAL EXPOSURE \n");
			break;

		case COLOR_CORRECTION: {
				cast_cmd(color_correction_t);
				cprint(COLOR_CORRECTION);
				DRV_PRINT(KERN_DEBUG "COLOR_CORRECTION \n");
				mprint(multi_red);
				mprint(multi_green);
				mprint(multi_blue);
			}
			break;

		case RGB_TO_YUV_SETUP:
			DRV_PRINT(KERN_DEBUG "RGB TO YUV SETUP \n");
			break;

		case CHROMA_SCALE:
			DRV_PRINT(KERN_DEBUG "CHROMA SCALE \n");
			break;

		case BAD_PIXEL_CORRECT_SETUP:
			DRV_PRINT(KERN_DEBUG "BAD PIXEL CORRECT SETUP \n");
			break;

		case CFA_NOISE_FILTER:
			DRV_PRINT(KERN_DEBUG "CFA NOISE FILTER \n");
			break;

		case DEMOASIC_FILTER:
			DRV_PRINT(KERN_DEBUG "DEMOSAIC FILTER \n");
			break;

#if 0
		case RGB_NOISE_FILTER:
			DRV_PRINT(KERN_DEBUG "RGB NOISE FILTER, NOT SUPPORTED on A7\n");
			break;
#endif

		//case RGB_DIRECTIONAL_FILTER:
		//	DRV_PRINT(KERN_DEBUG "RGB DIRECTIONAL FILTER, NOT SUPPORTED on A7\n");
		//	break;

		case CHROMA_MEDIAN_FILTER:
			DRV_PRINT(KERN_DEBUG "CHROMA MEDIAN FILTER \n");
			break;

		case LUMA_SHARPENING:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING FILTER \n");
			break;

		case LUMA_SHARPENING_LINEARIZATION:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING LINEARIZATION, NOT SUPPORTED on A7 \n");
			break;

		case LUMA_SHARPENING_FIR_CONFIG:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING FIR CONFIG \n");
			break;

		case LUMA_SHARPENING_LNL: {
				cast_cmd(luma_sharpening_LNL_t);
				cprint(LUMA_SHARPENING_LNL);
				mprint(input_offset_red);
				mprint(input_offset_green);
				mprint(input_offset_blue);
			}
			break;

		case LUMA_SHARPENING_TONE:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING TONE \n");
			break;

		case LUMA_SHARPENING_BLEND_CONTROL:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING BLEND CONTROL \n");
			break;

		case LUMA_SHARPENING_LEVEL_CONTROL:
			DRV_PRINT(KERN_DEBUG "LUMA SHARPENING LEVEL CONTROL \n");
			break;

		case AAA_STATISTICS_SETUP:
			DRV_PRINT(KERN_DEBUG "AAA STATISTICS SETUP \n");
			break;

		case AAA_PSEUDO_Y_SETUP:
			DRV_PRINT(KERN_DEBUG "AAA PSEUDO Y SETUP \n");
			break;

		case AAA_HISTORGRAM_SETUP:
			DRV_PRINT(KERN_DEBUG "AAA HISTOGRAM SETUP \n");
			break;

		case AAA_STATISTICS_SETUP1:
			DRV_PRINT(KERN_DEBUG "AAA STATISTICS SETUP1\n");
			break;

		case AAA_STATISTICS_SETUP2:
			DRV_PRINT(KERN_DEBUG "AAA STATISTICS SETUP2\n");
			break;

		case AAA_STATISTICS_SETUP3:
			DRV_PRINT(KERN_DEBUG "AAA STATISTICS SETUP3\n");
			break;

		case AAA_EARLY_WB_GAIN:
			DRV_PRINT(KERN_DEBUG "AAA EARLY WB GAIN \n");
			break;

		case AAA_FLOATING_TILE_CONFIG:
			DRV_PRINT(KERN_DEBUG "AAA FLOATING TILE CONFIG \n");
			break;

		case NOISE_FILTER_SETUP:
			DRV_PRINT(KERN_DEBUG "NOISE FILTER SETUP \n");
			break;

		case RAW_COMPRESSION:
			DRV_PRINT(KERN_DEBUG "RAW COMPRESSION \n");
			break;

		case RAW_DECOMPRESSION:
			DRV_PRINT(KERN_DEBUG "RAW DECOMPRESSION\n");
			break;

		case ROLLING_SHUTTER_COMPENSATION:
			DRV_PRINT(KERN_DEBUG "ROLLING SHUTTER COMPENSATION \n");
			break;

		case FPN_CALIBRATION:
			DRV_PRINT(KERN_DEBUG "FIXED PATTERN NOISE CALIBRATION \n");
			break;

		case HDR_MIXER:
			DRV_PRINT(KERN_DEBUG "HDR MIXER\n");
			break;

		case EARLY_WB_GAIN:
			DRV_PRINT(KERN_DEBUG "EARLY WB GAIN\n");
			break;

		case CHROMATIC_ABERRATION_WARP_CONTROL:
			DRV_PRINT(KERN_DEBUG "CHROMATIC ABERRATION WARP CONTROL \n");
			break;

		case DUMP_IDSP_CONFIG:
			DRV_PRINT(KERN_DEBUG "DUMP IDSP CONFIG \n");
			break;

		case SEND_IDSP_DEBUG_CMD:
			DRV_PRINT(KERN_DEBUG "SEND IDSP DEBUG CMD \n");
			break;

		case UPDATE_IDSP_CONFIG:
			DRV_PRINT(KERN_DEBUG "UPDATE IDSP CONFIG \n");
			break;

		/*case PROCESS_IDSP_CMD_BATCH:
			DRV_PRINT(KERN_DEBUG "PROCESS IDSP CMD BATCH\n");
			break;*/

		case CFA_DOMAIN_LEAKAGE_FILTER:
			DRV_PRINT(KERN_DEBUG "CFA_DOMAIN_LEAKAGE_FILTER \n");
			break;
		case ANTI_ALIASING:
			DRV_PRINT(KERN_DEBUG "ANTI_ALIASING \n");
			break;
		case CHROMA_NOISE_FILTER:
			DRV_PRINT(KERN_DEBUG "CHROMA_NOISE_FILTER \n");
			break;
		case STRONG_GRGB_MISMATCH_FILTER:
			DRV_PRINT(KERN_DEBUG "STRONG_GRGB_MISMATCH_FILTER \n");
			break;
		default:
				DRV_PRINT("PRINT_CMD: cmd code is 0x%x \n", cmd_code);
			break;

	}
}
EXPORT_SYMBOL(print_cmd);
