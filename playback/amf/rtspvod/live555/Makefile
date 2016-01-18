##
## Makefile
##
## Copyright (C) 2007-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MW_CE_TOPDIR	?= $(PWD)/../..

include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk

LIVE555_INCLUDE := -I./
LIVE555_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/BasicUsageEnvironment/include
LIVE555_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/UsageEnvironment/include
LIVE555_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/groupsock/include
LIVE555_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/liveMedia/include

LIVE555_CFLAG = -O2 -DLINUX $(LIVE555_INCLUDE) -DALLOW_RTSP_SERVER_PORT_REUSE
LIVE555_CPPFLAG = -O2 -DLINUX $(LIVE555_INCLUDE) -DALLOW_RTSP_SERVER_PORT_REUSE

prefix_1 := ./BasicUsageEnvironment
objects_cxx_1 := $(prefix_1)/BasicHashTable.o \
	$(prefix_1)/BasicTaskScheduler.o \
	$(prefix_1)/BasicTaskScheduler0.o \
	$(prefix_1)//BasicUsageEnvironment.o \
	$(prefix_1)//BasicUsageEnvironment0.o \
	$(prefix_1)//DelayQueue.o
objects_c_1 :=

prefix_2 := ./groupsock
objects_cxx_2 := $(prefix_2)/GroupEId.o \
	$(prefix_2)/Groupsock.o \
	$(prefix_2)/GroupsockHelper.o \
	$(prefix_2)/IOHandlers.o \
	$(prefix_2)/NetAddress.o \
	$(prefix_2)/NetInterface.o
objects_c_2 := $(prefix_2)/inet.o

prefix_3 := ./liveMedia
OBJ = o
MP3_SOURCE_OBJS = $(prefix_3)/MP3FileSource.$(OBJ) $(prefix_3)/MP3Transcoder.$(OBJ) $(prefix_3)/MP3ADU.$(OBJ) $(prefix_3)/MP3ADUdescriptor.$(OBJ) $(prefix_3)/MP3ADUinterleaving.$(OBJ) $(prefix_3)/MP3ADUTranscoder.$(OBJ) $(prefix_3)/MP3StreamState.$(OBJ) $(prefix_3)/MP3Internals.$(OBJ) $(prefix_3)/MP3InternalsHuffman.$(OBJ) $(prefix_3)/MP3InternalsHuffmanTable.$(OBJ) $(prefix_3)/MP3ADURTPSource.$(OBJ)
MPEG_SOURCE_OBJS = $(prefix_3)/MPEG1or2Demux.$(OBJ) $(prefix_3)/MPEG1or2DemuxedElementaryStream.$(OBJ) $(prefix_3)/MPEGVideoStreamFramer.$(OBJ) $(prefix_3)/MPEG1or2VideoStreamFramer.$(OBJ) $(prefix_3)/MPEG1or2VideoStreamDiscreteFramer.$(OBJ) $(prefix_3)/MPEG4VideoStreamFramer.$(OBJ) $(prefix_3)/MPEG4VideoStreamDiscreteFramer.$(OBJ) $(prefix_3)/H264VideoStreamFramer.$(OBJ) $(prefix_3)/H264VideoStreamDiscreteFramer.$(OBJ) $(prefix_3)/MPEGVideoStreamParser.$(OBJ) $(prefix_3)/MPEG1or2AudioStreamFramer.$(OBJ) $(prefix_3)/MPEG1or2AudioRTPSource.$(OBJ) $(prefix_3)/MPEG4LATMAudioRTPSource.$(OBJ) $(prefix_3)/MPEG4ESVideoRTPSource.$(OBJ) $(prefix_3)/MPEG4GenericRTPSource.$(OBJ) $(MP3_SOURCE_OBJS) $(prefix_3)/MPEG1or2VideoRTPSource.$(OBJ) $(prefix_3)/MPEG2TransportStreamMultiplexor.$(OBJ) $(prefix_3)/MPEG2TransportStreamFromPESSource.$(OBJ) $(prefix_3)/MPEG2TransportStreamFromESSource.$(OBJ) $(prefix_3)/MPEG2TransportStreamFramer.$(OBJ) $(prefix_3)/ADTSAudioFileSource.$(OBJ)
H263_SOURCE_OBJS = $(prefix_3)/H263plusVideoRTPSource.$(OBJ) $(prefix_3)/H263plusVideoStreamFramer.$(OBJ) $(prefix_3)/H263plusVideoStreamParser.$(OBJ)
AC3_SOURCE_OBJS = $(prefix_3)/AC3AudioStreamFramer.$(OBJ) $(prefix_3)/AC3AudioRTPSource.$(OBJ)
DV_SOURCE_OBJS = $(prefix_3)/DVVideoStreamFramer.$(OBJ) $(prefix_3)/DVVideoRTPSource.$(OBJ)
MP3_SINK_OBJS = $(prefix_3)/MP3ADURTPSink.$(OBJ)
MPEG_SINK_OBJS = $(prefix_3)/MPEG1or2AudioRTPSink.$(OBJ) $(MP3_SINK_OBJS) $(prefix_3)/MPEG1or2VideoRTPSink.$(OBJ) $(prefix_3)/MPEG4LATMAudioRTPSink.$(OBJ) $(prefix_3)/MPEG4GenericRTPSink.$(OBJ) $(prefix_3)/MPEG4ESVideoRTPSink.$(OBJ)
H263_SINK_OBJS = $(prefix_3)/H263plusVideoRTPSink.$(OBJ)
H264_SINK_OBJS = $(prefix_3)/H264VideoRTPSink.$(OBJ)
DV_SINK_OBJS = $(prefix_3)/DVVideoRTPSink.$(OBJ)
AC3_SINK_OBJS = $(prefix_3)/AC3AudioRTPSink.$(OBJ)
MISC_SOURCE_OBJS = $(prefix_3)/MediaSource.$(OBJ) $(prefix_3)/FramedSource.$(OBJ) $(prefix_3)/FramedFileSource.$(OBJ) $(prefix_3)/FramedFilter.$(OBJ) $(prefix_3)/ByteStreamFileSource.$(OBJ) $(prefix_3)/ByteStreamMultiFileSource.$(OBJ) $(prefix_3)/ByteStreamMemoryBufferSource.$(OBJ) $(prefix_3)/BasicUDPSource.$(OBJ) $(prefix_3)/DeviceSource.$(OBJ) $(prefix_3)/AudioInputDevice.$(OBJ) $(prefix_3)/WAVAudioFileSource.$(OBJ) $(MPEG_SOURCE_OBJS) $(H263_SOURCE_OBJS) $(AC3_SOURCE_OBJS) $(DV_SOURCE_OBJS) $(prefix_3)/JPEGVideoSource.$(OBJ) $(prefix_3)/AMRAudioSource.$(OBJ) $(prefix_3)/AMRAudioFileSource.$(OBJ) $(prefix_3)/InputFile.$(OBJ) $(prefix_3)/StreamReplicator.$(OBJ)
MISC_SINK_OBJS = $(prefix_3)/MediaSink.$(OBJ) $(prefix_3)/FileSink.$(OBJ) $(prefix_3)/BasicUDPSink.$(OBJ) $(prefix_3)/AMRAudioFileSink.$(OBJ) $(prefix_3)/H264VideoFileSink.$(OBJ) $(MPEG_SINK_OBJS) $(H263_SINK_OBJS) $(H264_SINK_OBJS) $(DV_SINK_OBJS) $(AC3_SINK_OBJS) $(prefix_3)/VorbisAudioRTPSink.$(OBJ) $(prefix_3)/VP8VideoRTPSink.$(OBJ) $(prefix_3)/GSMAudioRTPSink.$(OBJ) $(prefix_3)/JPEGVideoRTPSink.$(OBJ) $(prefix_3)/SimpleRTPSink.$(OBJ) $(prefix_3)/AMRAudioRTPSink.$(OBJ) $(prefix_3)/T140TextRTPSink.$(OBJ) $(prefix_3)/TCPStreamSink.$(OBJ) $(prefix_3)/OutputFile.$(OBJ)
MISC_FILTER_OBJS = $(prefix_3)/uLawAudioFilter.$(OBJ)
TRANSPORT_STREAM_TRICK_PLAY_OBJS = $(prefix_3)/MPEG2IndexFromTransportStream.$(OBJ) $(prefix_3)/MPEG2TransportStreamIndexFile.$(OBJ) $(prefix_3)/MPEG2TransportStreamTrickModeFilter.$(OBJ)
RTP_SOURCE_OBJS = $(prefix_3)/RTPSource.$(OBJ) $(prefix_3)/MultiFramedRTPSource.$(OBJ) $(prefix_3)/SimpleRTPSource.$(OBJ) $(prefix_3)/H261VideoRTPSource.$(OBJ) $(prefix_3)/H264VideoRTPSource.$(OBJ) $(prefix_3)/QCELPAudioRTPSource.$(OBJ) $(prefix_3)/AMRAudioRTPSource.$(OBJ) $(prefix_3)/JPEGVideoRTPSource.$(OBJ) $(prefix_3)/VorbisAudioRTPSource.$(OBJ) $(prefix_3)/VP8VideoRTPSource.$(OBJ)
RTP_SINK_OBJS = $(prefix_3)/RTPSink.$(OBJ) $(prefix_3)/MultiFramedRTPSink.$(OBJ) $(prefix_3)/AudioRTPSink.$(OBJ) $(prefix_3)/VideoRTPSink.$(OBJ) $(prefix_3)/TextRTPSink.$(OBJ)
RTP_INTERFACE_OBJS = $(prefix_3)/RTPInterface.$(OBJ)
RTP_OBJS = $(RTP_SOURCE_OBJS) $(RTP_SINK_OBJS) $(RTP_INTERFACE_OBJS)
#RTCP_OBJS = RTCP.$(OBJ) rtcp_from_spec.$(OBJ)
RTCP_OBJS = $(prefix_3)/RTCP.$(OBJ)
RTSP_OBJS = $(prefix_3)/RTSPServer.$(OBJ) $(prefix_3)/RTSPClient.$(OBJ) $(prefix_3)/RTSPCommon.$(OBJ) $(prefix_3)/RTSPServerSupportingHTTPStreaming.$(OBJ) $(prefix_3)/RTSPRegisterSender.$(OBJ)
SIP_OBJS = $(prefix_3)/SIPClient.$(OBJ)
SESSION_OBJS = $(prefix_3)/MediaSession.$(OBJ) $(prefix_3)/ServerMediaSession.$(OBJ) $(prefix_3)/PassiveServerMediaSubsession.$(OBJ) $(prefix_3)/OnDemandServerMediaSubsession.$(OBJ) $(prefix_3)/FileServerMediaSubsession.$(OBJ) $(prefix_3)/MPEG4VideoFileServerMediaSubsession.$(OBJ) $(prefix_3)/H264VideoFileServerMediaSubsession.$(OBJ) $(prefix_3)/H263plusVideoFileServerMediaSubsession.$(OBJ) $(prefix_3)/WAVAudioFileServerMediaSubsession.$(OBJ) $(prefix_3)/AMRAudioFileServerMediaSubsession.$(OBJ) $(prefix_3)/MP3AudioFileServerMediaSubsession.$(OBJ) $(prefix_3)/MPEG1or2VideoFileServerMediaSubsession.$(OBJ) $(prefix_3)/MPEG1or2FileServerDemux.$(OBJ) $(prefix_3)/MPEG1or2DemuxedServerMediaSubsession.$(OBJ) $(prefix_3)/MPEG2TransportFileServerMediaSubsession.$(OBJ) $(prefix_3)/ADTSAudioFileServerMediaSubsession.$(OBJ) $(prefix_3)/DVVideoFileServerMediaSubsession.$(OBJ) $(prefix_3)/AC3AudioFileServerMediaSubsession.$(OBJ) $(prefix_3)/MPEG2TransportUDPServerMediaSubsession.$(OBJ) $(prefix_3)/ProxyServerMediaSession.$(OBJ)
QUICKTIME_OBJS = $(prefix_3)/QuickTimeFileSink.$(OBJ) $(prefix_3)/QuickTimeGenericRTPSource.$(OBJ)
AVI_OBJS = $(prefix_3)/AVIFileSink.$(OBJ)
MATROSKA_FILE_OBJS = $(prefix_3)/MatroskaFile.$(OBJ) $(prefix_3)/MatroskaFileParser.$(OBJ) $(prefix_3)/EBMLNumber.$(OBJ) $(prefix_3)/MatroskaDemuxedTrack.$(OBJ)
MATROSKA_SERVER_MEDIA_SUBSESSION_VIDEO_OBJS = $(prefix_3)/H264VideoMatroskaFileServerMediaSubsession.$(OBJ) $(prefix_3)/VP8VideoMatroskaFileServerMediaSubsession.$(OBJ)
MATROSKA_SERVER_MEDIA_SUBSESSION_AUDIO_OBJS = $(prefix_3)/AACAudioMatroskaFileServerMediaSubsession.$(OBJ) $(prefix_3)/AC3AudioMatroskaFileServerMediaSubsession.$(OBJ) $(prefix_3)/MP3AudioMatroskaFileServerMediaSubsession.$(OBJ) $(prefix_3)/VorbisAudioMatroskaFileServerMediaSubsession.$(OBJ)
MATROSKA_SERVER_MEDIA_SUBSESSION_TEXT_OBJS = $(prefix_3)/T140TextMatroskaFileServerMediaSubsession.$(OBJ)
MATROSKA_SERVER_MEDIA_SUBSESSION_OBJS = $(MATROSKA_SERVER_MEDIA_SUBSESSION_VIDEO_OBJS) $(MATROSKA_SERVER_MEDIA_SUBSESSION_AUDIO_OBJS) $(MATROSKA_SERVER_MEDIA_SUBSESSION_TEXT_OBJS)
MATROSKA_RTSP_SERVER_OBJS = $(prefix_3)/MatroskaFileServerDemux.$(OBJ) $(MATROSKA_SERVER_MEDIA_SUBSESSION_OBJS)
MATROSKA_OBJS = $(MATROSKA_FILE_OBJS) $(MATROSKA_RTSP_SERVER_OBJS)
#MISC_OBJS = DarwinInjector.$(OBJ) BitVector.$(OBJ) StreamParser.$(OBJ) DigestAuthentication.$(OBJ) our_md5.$(OBJ) our_md5hl.$(OBJ) Base64.$(OBJ) Locale.$(OBJ)
MISC_OBJS = $(prefix_3)/DarwinInjector.$(OBJ) $(prefix_3)/BitVector.$(OBJ) $(prefix_3)/StreamParser.$(OBJ) $(prefix_3)/DigestAuthentication.$(OBJ) $(prefix_3)/Base64.$(OBJ) $(prefix_3)/Locale.$(OBJ)

objects_cxx_3 = $(prefix_3)/Media.$(OBJ) $(MISC_SOURCE_OBJS) $(MISC_SINK_OBJS) $(MISC_FILTER_OBJS) $(RTP_OBJS) $(RTCP_OBJS) $(RTSP_OBJS) $(SIP_OBJS) $(SESSION_OBJS) $(QUICKTIME_OBJS) $(AVI_OBJS) $(TRANSPORT_STREAM_TRICK_PLAY_OBJS) $(MATROSKA_OBJS) $(MISC_OBJS)
objects_c_3 = $(prefix_3)/our_md5.$(OBJ) $(prefix_3)/our_md5hl.$(OBJ) $(prefix_3)/rtcp_from_spec.$(OBJ)


prefix_4 := ./UsageEnvironment
objects_cxx_4 := $(prefix_4)/HashTable.o $(prefix_4)/strDup.o $(prefix_4)/UsageEnvironment.o
objects_c_4 := 
 
objects_cxx := $(objects_cxx_1) $(objects_cxx_2) $(objects_cxx_3) $(objects_cxx_4)
objects_c := $(objects_c_1) $(objects_c_2) $(objects_c_3) $(objects_c_4)

.PHONY: all clean

all: cxxobj ccobj ldlib

cxxobj: $(objects_cxx)
$(objects_cxx): %.o: %.cpp
	$(CXX) $(LIVE555_CPPFLAG) -c $< -o $@

ccobj: $(objects_c)
$(objects_c): %.o: %.c
	$(CC) $(LIVE555_CFLAG) -c $< -o $@

ldlib:
	@echo "    gcc liblive555.a:"
	$(AMBA_MAKEFILE_V) $(AR) rucs ../lib/liblive555.a $(objects_cxx) $(objects_c)
	@ranlib ../lib/liblive555.a

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile live555]:"
	-rm $(prefix_1)/*.o -rf
	-rm $(prefix_2)/*.o -rf
	-rm $(prefix_3)/*.o -rf
	-rm $(prefix_4)/*.o -rf
	-rm ../lib/liblive555.a -rf