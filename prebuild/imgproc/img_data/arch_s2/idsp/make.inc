##
## prebuild/imgproc/img_data/arch_s2/idsp/make.inc
##
## History:
##    2012/11/06 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###

include $(CLEAR_VARS)

.PHONY: prebuild_idsp_bin

prebuild_idsp_bin: PRIVATE_PATH:=$(LOCAL_PATH)
prebuild_idsp_bin:
	@mkdir -p $(UNIT_TEST_IMGPROC_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/*.bin $(UNIT_TEST_IMGPROC_PATH)/
	@mkdir -p $(UNIT_TEST_IMGPROC_PATH)/sensors/
	@mv -f $(UNIT_TEST_IMGPROC_PATH)/*_3D.bin $(UNIT_TEST_IMGPROC_PATH)/sensors/

$(call add-target-into-build, prebuild_idsp_bin)

