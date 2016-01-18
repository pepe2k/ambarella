##
## env/check_env.mk
##
## History:
##    2008/02/18 - [Anthony Ginger] Create
##
## Copyright (C) 2004-2008, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

.PHONY: check_env

check_env:
ifndef ARM_ELF_TOOLCHAIN_DIR
	$(error Can not find arm-elf toolchain, please set ARM_ELF_TOOLCHAIN_DIR)
endif
ifndef ARM_LINUX_TOOLCHAIN_DIR
	$(error Can not find arm-linux toolchain, please set ARM_LINUX_TOOLCHAIN_DIR)
endif

