##
## Makefile
##
## Copyright (C) 2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MW_CE_TOPDIR	?= $(PWD)/../../..
include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk

RTSPVOD_INCLUDE   = -I$(MW_CE_TOPDIR)/amf/rtspvod/include
RTSPVOD_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/librtspvod
RTSPVOD_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/BasicUsageEnvironment/include
RTSPVOD_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/UsageEnvironment/include
RTSPVOD_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/groupsock/include
RTSPVOD_INCLUDE += -I$(MW_CE_TOPDIR)/amf/rtspvod/live555/liveMedia/include

RTSPVOD_CFLAG = -DLINUX $(RTSPVOD_INCLUDE) $(AMF_CFLAG)
RTSPVOD_CPPFLAG = -DLINUX $(RTSPVOD_INCLUDE) $(AMF_CFLAG)

objects_cxx := DynamicRTSPServer.o H264FramedLiveSource.o H264LiveVideoServerMediaSubsession.o \
	AACLiveSource.o AACLiveServerMediaSubsession.o \
	G711LiveSource.o G711LiveServerMediaSubsession.o \
	G726LiveSource.o G726LiveServerMediaSubsession.o \
	rtsp_active_object.o rtsp_queue.o rtsp_service_impl.o rtsp_vod.o

objects_c :=

all: cxxobj ccobj ldlib

cxxobj: $(objects_cxx)
$(objects_cxx): %.o: %.cpp
	$(CXX) $(RTSPVOD_CFLAG) -c $< -o $@

ccobj: $(objects_c)
$(objects_c): %.o: %.c
	$(CC) $(RTSPVOD_CPPFLAG) -c $< -o $@

ldlib:
	@echo "    gcc librtspvod.a:"
	$(AMBA_MAKEFILE_V) $(AR) rucs ../lib/librtspvod.a $(objects_cxx) $(objects_c)
	@ranlib ../lib/librtspvod.a

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile librtspvod]:"
	-rm *.o -rf
	-rm ../lib/librtspvod.a -rf
