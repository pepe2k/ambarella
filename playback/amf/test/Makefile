##
## Makefile
##
## History:
##    2011/01/07 -  [Zhi He]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

include $(MW_CE_TOPDIR)/config.mk

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

TEST_CFLAG :=
TEST_CFLAG += $(AMBARELLA_CFLAGS) -I$(MW_CE_TOPDIR)/amf/include -I$(FFMPEG_TOP)
TEST_CFLAG += $(V_COMPILE_CFLAGS)
TEST_CFLAG += $(MWDV_CFLAG)
TEST_CFLAG += $(OPT_CFLAG)
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/android
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/filters/general
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/engines
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/framework
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/audio
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/filters/general/misc
TEST_CFLAG += -I$(MW_CE_TOPDIR)/cloudService/aliyunoss/include
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/extern/g711
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/utils
TEST_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspvod/include
TEST_LDFLAG := $(AMBARELLA_LDFLAGS) -lpthread -lrt -ldl -lamf_linux -lavformat -lavcodec -lavutil -L$(MW_CE_TOPDIR)/amf/build/linux/ -L$(FFMPEG_TOP)/build/linux/
THIRD_PARTY_DIR := $(MW_CE_TOPDIR)/../prebuild/third-party
STORAGE_LDFLAG := -lcurl -lz -lcrypto -lssh2 -lidn -lssl -ljson -L$(THIRD_PARTY_DIR)/libcurl/lib -L$(THIRD_PARTY_DIR)/json-c/lib -L$(THIRD_PARTY_DIR)/openssl/lib -L$(THIRD_PARTY_DIR)/libidn/lib -L$(THIRD_PARTY_DIR)/libssh2/lib -L$(THIRD_PARTY_DIR)/zlib/lib
TEST_LDFLAG += $(STORAGE_LDFLAG)
AUDIOREC_LDFLAG := -lg711 -L$(MW_CE_TOPDIR)/amf/extern/g711
TEST_LDFLAG += $(AUDIOREC_LDFLAG)

ifeq ($(AM_PBTEST_DEBUG), true)
TEST_LDFLAG += -L$(MW_CE_TOPDIR)/../../ambarella/prebuild/imgproc -limg_algo
endif

MTEST_FLAG := -lpthread -lrt -lamf_linux -lavformat -lavcodec -lavutil -L$(MW_CE_TOPDIR)/amf/build/linux/ -L$(FFMPEG_TOP)/build/linux/
MTEST_FLAG += -L$(MW_CE_TOPDIR)/amf/separate -ljpegenc
MTEST_FLAG += -L$(MW_CE_TOPDIR)/amf/libaac -laacdec
MTEST_FLAG += -L$(MW_CE_TOPDIR)/external/jpeg-6b -ljpegdec -I$(MW_CE_TOPDIR)/external/jpeg-6b -I$(MW_CE_TOPDIR)/external/jpeg-6b/test
MTEST_FLAG += -L$(MW_CE_TOPDIR)/amf/injector/lib -linjector -lrtmp
MTEST_FLAG += -L$(MW_CE_TOPDIR)/amf/rtspvod/lib -lrtspvod -llive555
MTEST_FLAG += -L$(MW_CE_TOPDIR)/cloudService/aliyunoss/lib -laliyunoss -L$(MW_CE_TOPDIR)/cloudService/storage/lib -lstorage
#MTEST_FLAG += -L$(MW_CE_TOPDIR)/../prebuild/third-party/coreavc/lib -lcoreavc_sdk

A5SAUDIO_CFLAG := -I$(MW_CE_TOPDIR)/../prebuild/third-party/alsa/include \
		-I$(MW_CE_TOPDIR)/../prebuild/third-party/misc/include
A5SAUDIO_CDEP := -L$(MW_CE_TOPDIR)/../prebuild/third-party/alsa/lib -lasound -lpthread -ldl

TEST_LDFLAG += $(A5SAUDIO_CDEP)


TEST_LDFLAG += -L$(MW_CE_TOPDIR)/../prebuild/third-party/misc/lib -laacdec -laacenc
TEST_LDFLAG += -L$(MW_CE_TOPDIR)/amf/injector/lib -linjector -lrtmp
TEST_LDFLAG += -L$(MW_CE_TOPDIR)/amf/rtspvod/lib -lrtspvod -llive555

IMGPROC_CFLAG :=	-I$(MW_CE_TOPDIR)/amf/imgproc \
					-I$(MW_CE_TOPDIR)/amf/imgproc/vin -I$(MW_CE_TOPDIR)/amf/imgproc/vout
all:
	#echo $(A5SAUDIO_CDEP)
	#echo $(TEST_LDFLAG)
	@$(AMBA_MAKEFILE_V)echo " compiling pbtest:"
	$(CXX) -o pbtest $(TEST_CFLAG) pbtest.cpp $(TEST_LDFLAG)
	cp pbtest $(MW_CE_TOPDIR)/amf/build/linux/
	$(STRIP) pbtest
	cp pbtest $(OUT_BINARY_DIR)
	#chmod 777 gdb
	#-cp gdb $(OUT_BINARY_DIR)
	-cp pb.config $(OUT_BINARY_DIR)
	-cp log.config $(OUT_BINARY_DIR)
	-cp rec.config $(OUT_BINARY_DIR)
	-cp -a pbtest_bmp/* $(OUT_BINARY_DIR)/
	-cp autoplay $(OUT_BINARY_DIR)

	@$(AMBA_MAKEFILE_V)echo " compiling audiorectest:"
	$(CXX) -o audiorectest $(TEST_CFLAG) audiorectest.cpp $(TEST_LDFLAG)
	-cp audiorectest $(MW_CE_TOPDIR)/amf/build/linux/
	$(STRIP) audiorectest
	-cp audiorectest $(OUT_BINARY_DIR)

#ifeq ($(CONFIG_ENABLE_RECTEST), y)
	@$(AMBA_MAKEFILE_V)echo " compiling rectest:"
ifeq ($(CONFIG_IMGPROC_IN_AMF_UT), y)
	$(CXX) -o rectest $(TEST_CFLAG) -DCONFIG_IMGPROC_IN_AMF_UT rectest.cpp $(TEST_LDFLAG) $(IMGPROC_CFLAG)
else
	$(CXX) -o rectest $(TEST_CFLAG) rectest.cpp $(TEST_LDFLAG)
endif
	cp rectest $(MW_CE_TOPDIR)/amf/build/linux/
	$(STRIP) rectest
	cp rectest $(OUT_BINARY_DIR)
#endif

	@$(AMBA_MAKEFILE_V)echo " compiling dutest:"
	$(CXX) -o dutest $(TEST_CFLAG) dutest.cpp $(TEST_LDFLAG)
	cp dutest $(MW_CE_TOPDIR)/amf/build/linux/
	$(STRIP) dutest
	cp dutest $(OUT_BINARY_DIR)

	@$(AMBA_MAKEFILE_V)echo " compiling socket_test:"
	$(CXX) -o socket_test $(TEST_CFLAG) simple_socket_test.cpp $(TEST_LDFLAG)
	cp socket_test $(MW_CE_TOPDIR)/amf/build/linux/
	$(STRIP) socket_test
	cp socket_test $(OUT_BINARY_DIR)

ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
	@$(AMBA_MAKEFILE_V)echo " compiling mdectest:"
	$(CXX) -o mdectest -g -O0 $(TEST_CFLAG) mdectest.cpp $(TEST_LDFLAG) $(MTEST_FLAG)
	#$(STRIP) mdectest
	cp mdectest $(MW_CE_TOPDIR)/amf/build/linux/
	cp mdectest $(OUT_BINARY_DIR)

ifeq ($(AM_VIDEOEDITING_CONFIG), true)
	@$(AMBA_MAKEFILE_V)echo " compiling test_2udec:"
	$(CXX) -o test_2udec $(TEST_CFLAG) test_2udec.cpp $(TEST_LDFLAG)
	$(STRIP) test_2udec
	cp test_2udec $(MW_CE_TOPDIR)/amf/build/linux/
	cp test_2udec $(OUT_BINARY_DIR)

	@$(AMBA_MAKEFILE_V)echo " compiling test_transcode:"
	$(CXX) -o test_transcode $(TEST_CFLAG) test_transcode.cpp $(TEST_LDFLAG)
	$(STRIP) test_transcode
	cp test_transcode $(MW_CE_TOPDIR)/amf/build/linux/
	cp test_transcode $(OUT_BINARY_DIR)
endif

	@$(AMBA_MAKEFILE_V)echo " compiling test_tscombine:"
	$(CXX) -o test_tscombine $(TEST_CFLAG) ../android/TSFileCombiner.cpp test_tscombine.cpp $(TEST_LDFLAG)
	$(STRIP) test_transcode
	cp test_tscombine $(MW_CE_TOPDIR)/amf/build/linux/
	cp test_tscombine $(OUT_BINARY_DIR)
endif

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/test]:"
	-rm $(MW_CE_TOPDIR)/amf/build/linux/*.o -rf
	-rm $(MW_CE_TOPDIR)/amf/build/linux/*.a -rf
	-rm pbtest -rf
	-rm rectest -rf
	-rm dutest -rf
	-rm mdectest -rf
	-rm socket_test -rf
	-rm test_tscombine -rf
	-rm test_2udec -rf
	-rm test_transcode -rf
