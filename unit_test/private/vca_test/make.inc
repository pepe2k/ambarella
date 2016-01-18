##
## unit_test/private/make.inc
##
## History:
##    2013/10/08 - [Zhenwu Xue] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_PRIVATE_LINUX_UNIT_TESTS),y)
include $(call all-subdir-makefiles)
endif

