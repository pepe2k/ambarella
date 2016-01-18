##
## Makefile
##
## Copyright (C) 2004-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

include $(MW_CE_TOPDIR)/config.mk

RTSP_CFLAG := -I$(MW_CE_TOPDIR)/amf/rtspclient/utils
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp
RTSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects = RtspTcpService.o \
	RtspTcpAgent.o \
	RtspTcpConnection.o

all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/rtspclient/rtsp_tcp]:"
$(objects): %.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $(RTSP_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/rtspclient/rtsp_tcp]:"
	-rm *.o -rf

