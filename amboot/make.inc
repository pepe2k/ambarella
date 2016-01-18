##
## amboot/make.inc
##
## History:
##    2012/06/07 - [Cao Rongrong] Create
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH:=$(call my-dir)

.PHONY: amboot

amboot: PRIVATE_PATH:=$(LOCAL_PATH)
amboot: $(DOT_CONFIG)
	@mkdir -p $(AMB_BOARD_OUT)/amboot
	@echo "Build Amboot ... "
	@cp -dpRf $(PRIVATE_PATH)/build/Makefile $(AMB_BOARD_OUT)/amboot
	@rm -rf $(IMAGES_OUT_DIR)/*
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_BOARD_OUT)/amboot clean
	$(AMBA_MAKEFILE_V)$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_BOARD_OUT)/amboot
	@mkdir -p $(IMAGES_OUT_DIR)/
	@mv -f $(AMB_BOARD_OUT)/amboot/memfwprog/* $(IMAGES_OUT_DIR)/

