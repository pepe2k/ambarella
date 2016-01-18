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

INJECTOR_INCLUDE   = -I$(MW_CE_TOPDIR)/amf/injector/include
INJECTOR_INCLUDE += -I$(MW_CE_TOPDIR)/amf/injector/librtmp
INJECTOR_INCLUDE += -I$(MW_CE_TOPDIR)/amf/injector/libinjector

INJECTOR_CFLAG = -DLINUX $(INJECTOR_INCLUDE) $(AMF_CFLAG) 
INJECTOR_CPPFLAG = -DLINUX $(INJECTOR_INCLUDE) $(AMF_CFLAG)

objects_cxx := rtmp_queue.o rtmp_injector.o rtmp_buffer.o rtmp_get_h264_info.o rtmp_active_object.o rtmpclient.o rtmpclient_impl.o
objects_c := 

all: cxxobj ccobj ldlib

cxxobj: $(objects_cxx)
$(objects_cxx): %.o: %.cpp
	$(CXX) $(INJECTOR_CPPFLAG) -c $< -o $@
	
ccobj: $(objects_c)
$(objects_c): %.o: %.c
	$(CC) $(INJECTOR_CFLAG) -c $< -o $@
	
ldlib:
	@echo "    gcc libinjector.a:"
	$(AMBA_MAKEFILE_V) $(AR) rucs ../lib/libinjector.a $(objects_cxx) $(objects_c)
	@ranlib ../lib/libinjector.a

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile libinjector]:"
	-rm *.o -rf
	-rm ../lib/libinjector.a -rf
