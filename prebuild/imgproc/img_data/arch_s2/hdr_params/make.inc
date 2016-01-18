##
## prebuild/imgproc/img_data/arch_s2/hdr_params/make.inc
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

.PHONY: prebuild_hdr_cfg

prebuild_hdr_cfg: PRIVATE_PATH:=$(LOCAL_PATH)
prebuild_hdr_cfg:
	@mkdir -p $(UNIT_TEST_IMGPROC_PATH)/
	@mkdir -p $(UNIT_TEST_IMGPROC_PATH)/hdr/
	@cp -dpRf $(PRIVATE_PATH)/*.cfg $(UNIT_TEST_IMGPROC_PATH)/hdr/

$(call add-target-into-build, prebuild_hdr_cfg)

