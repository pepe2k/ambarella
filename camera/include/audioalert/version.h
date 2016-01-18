/*
 * version.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 23/11/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_AUDIOALERT_VERSION_H__
#define __AM_AUDIOALERT_VERSION_H__

#define AUDIOALERT_LIB_MAJOR 0
#define AUDIOALERT_LIB_MINOR 1
#define AUDIOALERT_LIB_PATCH 0
#define AUDIOALERT_LIB_VERSION ((AUDIOALERT_LIB_MAJOR << 16) | \
                             (AUDIOALERT_LIB_MINOR << 8)  | \
                             AUDIOALERT_LIB_PATCH)

#define AUDIOALERT_VERSION_STR "0.1.0"

#endif