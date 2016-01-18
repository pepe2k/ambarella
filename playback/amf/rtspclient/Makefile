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

include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

LOCAL_C_INCLUDES += $(AMF_TOP)/../../../../ambarella/build/include

RTSP_CFLAG :=	-I$(MW_CE_TOPDIR)/amf/rtspclient
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/utils
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_rtp
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_manager
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/ffmpeg_v0.7/
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/ffmpeg_v0.7/libavformat

objects = rtspclient.o \
	amf_rtspclient.o
	
all: $(objects)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/utils $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_rtp $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_manager $@
	$(AMBA_MAKEFILE_V) echo "  [compile amf/rtspclient/]:"

$(objects): %.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $(RTSP_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/rtspclient]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/utils $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_rtp $@	
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspclient/rtsp_manager $@
	-rm *.o -rf

