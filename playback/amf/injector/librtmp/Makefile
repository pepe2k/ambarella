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

RTMP_INCLUDE := -I$(MW_CE_TOPDIR)/amf/injector/librtmp

RTMP_CFLAG = -DLINUX -DNO_CRYPTO $(RTMP_INCLUDE) $(AMF_CFLAG) 
RTMP_CPPFLAG = -DLINUX -DNO_CRYPTO $(RTMP_INCLUDE) $(AMF_CFLAG)

objects_cxx := 
objects_c := amf.o hashswf.o log.o parseurl.o rtmp.o 

all: cxxobj ccobj ldlib

cxxobj: $(objects_cxx)
$(objects_cxx): %.o: %.cpp
	$(CXX) $(RTMP_CPPFLAG) -c $< -o $@

ccobj: $(objects_c)
$(objects_c): %.o: %.c
	$(CC) $(RTMP_CFLAG) -c $< -o $@

ldlib:
	@echo "    gcc librtmp.a:"
	$(AMBA_MAKEFILE_V) $(AR) rucs ../lib/librtmp.a $(objects_cxx) $(objects_c)
	@ranlib ../lib/librtmp.a

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile librtmp]:"
	-rm *.o -rf
	-rm ../lib/librtmp.a -rf
