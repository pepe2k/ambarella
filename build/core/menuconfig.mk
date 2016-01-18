##
## History:
##    2011/12/13 - [Cao Rongrong] Create
##    2013/03/26 - [Jian Tang] Modified
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

$(DOT_CONFIG):
	@echo "System not configured!"
	@echo "Please run 'make menuconfig'"
	@exit 1

$(AMB_TOPDIR)/build/kconfig/conf: $(AMB_TOPDIR)/build/kconfig/Makefile
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/kconfig conf

$(AMB_TOPDIR)/build/kconfig/mconf: $(AMB_TOPDIR)/build/kconfig/Makefile
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/kconfig ncurses conf mconf

.PHONY: menuconfig show_configs

show_configs:
	@ls $(AMB_BOARD_DIR)/config/

menuconfig: $(AMB_TOPDIR)/build/kconfig/mconf
	@AMBABUILD_SRCTREE=$(AMB_TOPDIR) $(AMB_TOPDIR)/build/kconfig/mconf $(AMB_TOPDIR)/AmbaConfig

%_config: $(AMB_BOARD_DIR)/config/%_config prepare_public_linux $(AMB_TOPDIR)/build/kconfig/conf
	@cp -f $< $(DOT_CONFIG)
	@AMBABUILD_SRCTREE=$(AMB_TOPDIR) $(AMB_TOPDIR)/build/kconfig/conf --defconfig=$< $(AMB_TOPDIR)/AmbaConfig

.PHONY: create_boards
create_boards: $(AMB_TOPDIR)/build/bin/create_board_mkcfg
ifeq ($(shell [ -d $(AMB_TOPDIR)/boards ] && echo y), y)
	@$(AMB_TOPDIR)/build/bin/create_board_mkcfg $(AMB_TOPDIR)/boards;
endif

.PHONY: sync_build_mkcfg
sync_build_mkcfg: create_boards $(AMB_TOPDIR)/build/bin/create_root_mkcfg
	@$(AMB_TOPDIR)/build/bin/create_mkcfg $(AMB_TOPDIR)/build/cfg $(AMB_TOPDIR)
	@$(AMB_TOPDIR)/build/bin/create_root_mkcfg $(AMB_TOPDIR)


.PHONY: prepare_public_linux
prepare_public_linux:
ifneq ($(shell [ -L ../../kernel/linux ] && echo y), y)
	$(AMBA_MAKEFILE_V)echo "start to extract linux"
	tar xjvf ../../kernel/linux-3.8.tar.bz2 -C ../../kernel/.
	ln -s linux-3.8 ../../kernel/linux
	patch -d ../../kernel/linux -p1 < ../../kernel/linux-3.8-ambarella.patch
	$(AMBA_MAKEFILE_V)echo "linux patched"
endif

