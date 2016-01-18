RTSP_CLIENT_DIR = ../../Windows/Filters/rtspClient
INCLUDES = -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include -I../BasicUsageEnvironment/include
MODULENAME = testProgs
!include    <ntwin32.mak>
COMPILE_OPTS =		$(INCLUDES) $(cdebug) $(cflags) $(cvarsdll) /Gy /GX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\"
C =	c
CPP =			cpp
C_FLAGS =	$(COMPILE_OPTS)
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS)
OBJ =			obj
LINK =			$(link) -out:
LIBRARY_LINK =		lib -out:
LIB_SUFFIX =		lib
CONSOLE_UI_OPTS =		$(conlflags) $(conlibsdll)
LINK_OPTS_0 =		$(linkdebug)
CONSOLE_LINK_OPTS =	$(LINK_OPTS_0) $(CONSOLE_UI_OPTS)
EXE = .exe
# change default OUTDIR : XP32_DEBUG/XP32_RETAIL
OUTDIR = $(RTSP_CLIENT_DIR)/$(MODULENAME)

ALL = $(OUTDIR)  $(MULTICAST_APPS) $(UNICAST_APPS) $(MISC_APPS)
all:	$(ALL)


MULTICAST_STREAMER_APPS = $(OUTDIR)/testMP3Streamer$(EXE) $(OUTDIR)/testMPEG1or2VideoStreamer$(EXE) \
	$(OUTDIR)/testMPEG1or2AudioVideoStreamer$(EXE) $(OUTDIR)/testMPEG2TransportStreamer$(EXE) \
	$(OUTDIR)/testMPEG4VideoStreamer$(EXE) $(OUTDIR)/testDVVideoStreamer$(EXE) \
	$(OUTDIR)/testWAVAudioStreamer$(EXE) $(OUTDIR)/testAMRAudioStreamer$(EXE) \
	$(OUTDIR)/vobStreamer$(EXE)
MULTICAST_RECEIVER_APPS = $(OUTDIR)/testMP3Receiver$(EXE) $(OUTDIR)/testMPEG1or2VideoReceiver$(EXE) \
	$(OUTDIR)/sapWatch$(EXE)
MULTICAST_MISC_APPS = $(OUTDIR)/testRelay$(EXE)
MULTICAST_APPS = $(MULTICAST_STREAMER_APPS) $(MULTICAST_RECEIVER_APPS) $(MULTICAST_MISC_APPS)

UNICAST_STREAMER_APPS = $(OUTDIR)/testOnDemandRTSPServer$(EXE) \
	$(OUTDIR)/testMPEG1or2AudioVideoToDarwin$(EXE) \
	$(OUTDIR)/testMPEG4VideoToDarwin$(EXE)
UNICAST_RECEIVER_APPS = $(OUTDIR)/openRTSP$(EXE) $(OUTDIR)/playSIP$(EXE)
UNICAST_APPS = $(UNICAST_STREAMER_APPS) $(UNICAST_RECEIVER_APPS)

MISC_APPS = $(OUTDIR)/testMPEG1or2Splitter$(EXE) $(OUTDIR)/testMPEG1or2ProgramToTransportStream$(EXE) \
	$(OUTDIR)/MPEG2TransportStreamIndexer$(EXE) $(OUTDIR)/testMPEG2TransportStreamTrickPlay$(EXE)

ALL_APPS = $(MULTICAST_APPS) $(UNICAST_APPS) $(MISC_APPS)
all: $(OUTDIR) $(ALL_APPS)

extra:	$(OUTDIR)/testGSMStreamer$(EXE)

.$(CPP){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<
.$(C){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<

MP3_STREAMER_OBJS = $(OUTDIR)/testMP3Streamer.$(OBJ)
MP3_RECEIVER_OBJS = $(OUTDIR)/testMP3Receiver.$(OBJ)
RELAY_OBJS = $(OUTDIR)/testRelay.$(OBJ)
MPEG_1OR2_SPLITTER_OBJS = $(OUTDIR)/testMPEG1or2Splitter.$(OBJ)
MPEG_1OR2_VIDEO_STREAMER_OBJS = $(OUTDIR)/testMPEG1or2VideoStreamer.$(OBJ)
MPEG_1OR2_VIDEO_RECEIVER_OBJS = $(OUTDIR)/testMPEG1or2VideoReceiver.$(OBJ)
MPEG_1OR2_AUDIO_VIDEO_STREAMER_OBJS = $(OUTDIR)/testMPEG1or2AudioVideoStreamer.$(OBJ)
MPEG2_TRANSPORT_STREAMER_OBJS = $(OUTDIR)/testMPEG2TransportStreamer.$(OBJ)
MPEG4_VIDEO_STREAMER_OBJS = $(OUTDIR)/testMPEG4VideoStreamer.$(OBJ)
DV_VIDEO_STREAMER_OBJS = $(OUTDIR)/testDVVideoStreamer.$(OBJ)
WAV_AUDIO_STREAMER_OBJS = $(OUTDIR)/testWAVAudioStreamer.$(OBJ)
AMR_AUDIO_STREAMER_OBJS	= $(OUTDIR)/testAMRAudioStreamer.$(OBJ)
ON_DEMAND_RTSP_SERVER_OBJS	= $(OUTDIR)/testOnDemandRTSPServer.$(OBJ)
VOB_STREAMER_OBJS	= $(OUTDIR)/vobStreamer.$(OBJ)
OPEN_RTSP_OBJS    = $(OUTDIR)/openRTSP.$(OBJ) $(OUTDIR)/playCommon.$(OBJ)
PLAY_SIP_OBJS     = $(OUTDIR)/playSIP.$(OBJ) $(OUTDIR)/playCommon.$(OBJ)
SAP_WATCH_OBJS = $(OUTDIR)/sapWatch.$(OBJ)
MPEG_1OR2_AUDIO_VIDEO_TO_DARWIN_OBJS = $(OUTDIR)/testMPEG1or2AudioVideoToDarwin.$(OBJ)
MPEG_4_VIDEO_TO_DARWIN_OBJS = $(OUTDIR)/testMPEG4VideoToDarwin.$(OBJ)
MPEG_1OR2_PROGRAM_TO_TRANSPORT_STREAM_OBJS = $(OUTDIR)/testMPEG1or2ProgramToTransportStream.$(OBJ)
MPEG2_TRANSPORT_STREAM_INDEXER_OBJS = $(OUTDIR)/MPEG2TransportStreamIndexer.$(OBJ)
MPEG2_TRANSPORT_STREAM_TRICK_PLAY_OBJS = $(OUTDIR)/testMPEG2TransportStreamTrickPlay.$(OBJ)

GSM_STREAMER_OBJS = $(OUTDIR)/testGSMStreamer.$(OBJ) $(OUTDIR)/testGSMEncoder.$(OBJ)

openRTSP.$(CPP):	playCommon.hh
playCommon.$(CPP):	playCommon.hh
playSIP.$(CPP):		playCommon.hh

USAGE_ENVIRONMENT_DIR = $(RTSP_CLIENT_DIR)/UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = $(RTSP_CLIENT_DIR)/BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = $(RTSP_CLIENT_DIR)/liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = $(RTSP_CLIENT_DIR)/groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
LOCAL_LIBS =	$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
		$(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB)
LIBS =			$(LOCAL_LIBS)

$(OUTDIR)/testMP3Streamer$(EXE):	$(MP3_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MP3_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testMP3Receiver$(EXE):	$(MP3_RECEIVER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MP3_RECEIVER_OBJS) $(LIBS)
$(OUTDIR)/testRelay$(EXE):	$(RELAY_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(RELAY_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2Splitter$(EXE):	$(MPEG_1OR2_SPLITTER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_SPLITTER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2VideoStreamer$(EXE):	$(MPEG_1OR2_VIDEO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_VIDEO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2VideoReceiver$(EXE):	$(MPEG_1OR2_VIDEO_RECEIVER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_VIDEO_RECEIVER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2AudioVideoStreamer$(EXE):	$(MPEG_1OR2_AUDIO_VIDEO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_AUDIO_VIDEO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG2TransportStreamer$(EXE):	$(MPEG2_TRANSPORT_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG2_TRANSPORT_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG4VideoStreamer$(EXE):	$(MPEG4_VIDEO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG4_VIDEO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testDVVideoStreamer$(EXE):	$(DV_VIDEO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(DV_VIDEO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testWAVAudioStreamer$(EXE):	$(WAV_AUDIO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(WAV_AUDIO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testAMRAudioStreamer$(EXE):	$(AMR_AUDIO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(AMR_AUDIO_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/testOnDemandRTSPServer$(EXE):	$(ON_DEMAND_RTSP_SERVER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(ON_DEMAND_RTSP_SERVER_OBJS) $(LIBS)
$(OUTDIR)/vobStreamer$(EXE):	$(VOB_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(VOB_STREAMER_OBJS) $(LIBS)
$(OUTDIR)/openRTSP$(EXE):	$(OPEN_RTSP_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(OPEN_RTSP_OBJS) $(LIBS)
$(OUTDIR)/playSIP$(EXE):	$(PLAY_SIP_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(PLAY_SIP_OBJS) $(LIBS)
$(OUTDIR)/sapWatch$(EXE):	$(SAP_WATCH_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(SAP_WATCH_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2AudioVideoToDarwin$(EXE):	$(MPEG_1OR2_AUDIO_VIDEO_TO_DARWIN_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_AUDIO_VIDEO_TO_DARWIN_OBJS) $(LIBS)
$(OUTDIR)/testMPEG4VideoToDarwin$(EXE):	$(MPEG_4_VIDEO_TO_DARWIN_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_4_VIDEO_TO_DARWIN_OBJS) $(LIBS)
$(OUTDIR)/testMPEG1or2ProgramToTransportStream$(EXE):	$(MPEG_1OR2_PROGRAM_TO_TRANSPORT_STREAM_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG_1OR2_PROGRAM_TO_TRANSPORT_STREAM_OBJS) $(LIBS)
$(OUTDIR)/MPEG2TransportStreamIndexer$(EXE):	$(MPEG2_TRANSPORT_STREAM_INDEXER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG2_TRANSPORT_STREAM_INDEXER_OBJS) $(LIBS)
$(OUTDIR)/testMPEG2TransportStreamTrickPlay$(EXE):	$(MPEG2_TRANSPORT_STREAM_TRICK_PLAY_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MPEG2_TRANSPORT_STREAM_TRICK_PLAY_OBJS) $(LIBS)

$(OUTDIR)/testGSMStreamer$(EXE):	$(GSM_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(GSM_STREAMER_OBJS) $(LIBS)

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	if exist "$(OUTDIR)" rd /s /q "$(OUTDIR)"

##### Any additional, platform-specific rules come here:
