##
## ambarella/playback/amf/src/make.inc
##
## History:
##    2013/04/01 - [Roy Su] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

MODULE_SRCS	:= 
MODULE_INCLUDES_AMF	:= $(AMF_CFLAG)

#src/audio
MODULE_SRCS += $(LOCAL_PATH)/audio/audio_if.cpp $(LOCAL_PATH)/audio/audio_alsa.cpp
MODULE_INCLUDES_AMF += -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include

#src/dsp
MODULE_SRCS += $(LOCAL_PATH)/dsp/amba_dsp_common.cpp $(LOCAL_PATH)/dsp/amba_dsp_ione.cpp
MODULE_INCLUDES_AMF += -I$(PLAYBACK_FFMPEG_TOP)/libavcodec \
			-I$(LOCAL_PATH)/filters/general \
			-I$(LOCAL_PATH)/engines \
			-I$(LOCAL_PATH)/framework \
			-I$(LOCAL_PATH)/audio \
			-I$(AMF_TOP)/rtspvod/include \
			-I$(AMF_TOP)/injector/include \
            -I$(AMF_TOP)/rtspclient \
            -I$(AMF_TOP)/rtspclient/utils \
            -I$(AMF_TOP)/rtspclient/rtsp_manager \
            -I$(AMF_TOP)/rtspclient/rtsp_message \
            -I$(AMF_TOP)/rtspclient/rtsp_tcp \
            -I$(AMF_TOP)/rtspclient/rtsp_rtp

#/src/engine
MODULE_SRCS += $(LOCAL_PATH)/engines/engine_guids.cpp \
		$(LOCAL_PATH)/engines/filter_list.cpp \
		$(LOCAL_PATH)/engines/filter_registry.cpp \
		$(LOCAL_PATH)/engines/pb_engine.cpp \
		$(LOCAL_PATH)/engines/active_pb_engine.cpp \
		$(LOCAL_PATH)/engines/pbif.cpp \
		$(LOCAL_PATH)/engines/record_if.cpp \
		$(LOCAL_PATH)/engines/active_ve_engine.cpp \
		$(LOCAL_PATH)/engines/ve_if.cpp \
		$(LOCAL_PATH)/engines/active_mdec_engine.cpp \
		$(LOCAL_PATH)/engines/mdec_if.cpp \
		$(LOCAL_PATH)/engines/streamming_if.cpp \
		$(LOCAL_PATH)/engines/streamming_server.cpp \
		$(LOCAL_PATH)/engines/random.cpp \
		$(LOCAL_PATH)/engines/ws_discovery.cpp \
		$(LOCAL_PATH)/engines/motiondetectreceiver.cpp \
		$(LOCAL_PATH)/engines/ws_discovery_impl.cpp
MODULE_INCLUDES_AMF += -I$(LOCAL_PATH)/filters/general/misc

#src/filters/general/misc
MODULE_SRCS += $(LOCAL_PATH)/filters/general/misc/general_muxer_save.cpp \
		$(LOCAL_PATH)/filters/general/misc/gmf_osd_shower.cpp \
		$(LOCAL_PATH)/filters/general/misc/g_muxer_ffmpeg.cpp \
		$(LOCAL_PATH)/filters/general/misc/g_simple_save.cpp \
		$(LOCAL_PATH)/filters/general/misc/general_layout_manager.cpp \
		$(LOCAL_PATH)/filters/general/misc/general_pipeline.cpp \
		$(LOCAL_PATH)/filters/general/misc/g_injector_rtmp.cpp

#src/filters/general
MODULE_SRCS += $(LOCAL_PATH)/filters/general/general_mw.cpp \
		$(LOCAL_PATH)/filters/general/general_header.cpp \
		$(LOCAL_PATH)/filters/general/general_parse.cpp \
		$(LOCAL_PATH)/filters/general/general_demuxer_filter.cpp \
		$(LOCAL_PATH)/filters/general/general_decoder_filter.cpp \
		$(LOCAL_PATH)/filters/general/general_renderer_filter.cpp \
		$(LOCAL_PATH)/filters/general/general_transcoder_filter.cpp \
		$(LOCAL_PATH)/filters/general/general_audio_manager.cpp \
		$(LOCAL_PATH)/filters/general/g_ffmpeg_demuxer_exqueue.cpp \
		$(LOCAL_PATH)/filters/general/g_ffmpeg_demuxer_exnet.cpp \
		$(LOCAL_PATH)/filters/general/gvideo_decoder_dsp_exqueue.cpp \
		$(LOCAL_PATH)/filters/general/g_ffmpeg_decoder.cpp \
		$(LOCAL_PATH)/filters/general/g_sync_renderer.cpp \
		$(LOCAL_PATH)/filters/general/g_render_out.cpp \
		$(LOCAL_PATH)/filters/general/misc/g_injector_rtsp.cpp \
		$(LOCAL_PATH)/filters/general/g_ffmpeg_video_decoder.cpp \
        $(LOCAL_PATH)/filters/general/general_transc_audiosink_filter.cpp
MODULE_INCLUDES_AMF += -I$(LOCAL_PATH)/filters

#src/filters
MODULE_SRCS += $(LOCAL_PATH)/filters/audio_renderer.cpp \
		$(LOCAL_PATH)/filters/amba_video_decoder.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_decoder.cpp \
		$(LOCAL_PATH)/filters/video_renderer.cpp \
		$(LOCAL_PATH)/filters/amdsp_common.cpp \
		$(LOCAL_PATH)/filters/amba_video_sink.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_demuxer.cpp \
		$(LOCAL_PATH)/filters/simple_muxer.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_muxer2.cpp \
		$(LOCAL_PATH)/filters/audio_input2.cpp \
		$(LOCAL_PATH)/filters/aac_encoder.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_muxer.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_encoder.cpp \
		$(LOCAL_PATH)/filters/audio_effecter.cpp \
		$(LOCAL_PATH)/filters/video_effecter_preview.cpp \
		$(LOCAL_PATH)/filters/video_transcoder.cpp \
		$(LOCAL_PATH)/filters/video_mem_encoder.cpp \
		$(LOCAL_PATH)/filters/pridata_composer.cpp \
		$(LOCAL_PATH)/filters/pridata_parser.cpp \
		$(LOCAL_PATH)/filters/am_ffmpeg.cpp \
		$(LOCAL_PATH)/filters/ffmpeg_util.cpp \
		$(LOCAL_PATH)/filters/video_decoder_dsp_ve.cpp \
		$(LOCAL_PATH)/filters/general_decode_filter_ve.cpp
MODULE_INCLUDES_AMF += -I$(LOCAL_PATH)/dsp \
			-I$(LOCAL_PATH)/src/audio \
			-I$(PREBUILD_3RD_PARTY_DIR)/aac/include \
			-I$(PREBUILD_3RD_PARTY_DIR)/alsa/include/alsa

#src/framework
MODULE_SRCS += $(LOCAL_PATH)/framework/am_base.cpp \
		$(LOCAL_PATH)/framework/am_futif.cpp \
		$(LOCAL_PATH)/framework/am_if.cpp \
		$(LOCAL_PATH)/framework/am_new.cpp \
		$(LOCAL_PATH)/framework/am_queue.cpp \
		$(LOCAL_PATH)/framework/msgsys.cpp \
		$(LOCAL_PATH)/framework/osal_linux.cpp \
		$(LOCAL_PATH)/framework/am_param.cpp

#src/misc
MODULE_SRCS += $(LOCAL_PATH)/misc/am_util.cpp \
		$(LOCAL_PATH)/misc/misc_guids.cpp \
		$(LOCAL_PATH)/misc/am_pridata.cpp


LOCAL_TARGET	:= libamf_linux.so
LOCAL_SRCS	:= $(MODULE_SRCS)
LOCAL_CFLAGS	:= $(MODULE_INCLUDES_AMF)
		

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

