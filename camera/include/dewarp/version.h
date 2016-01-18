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
#ifndef __AM_DEWARP_VERSION_H__
#define __AM_DEWARP_VERSION_H__

#define DEWARP_LIB_MAJOR 0
#define DEWARP_LIB_MINOR 1
#define DEWARP_LIB_PATCH 0
#define DEWARP_LIB_VERSION ((DEWARP_LIB_MAJOR << 16) | \
                             (DEWARP_LIB_MINOR << 8)  | \
                             DEWARP_LIB_PATCH)

#define DEWARP_VERSION_STR "0.1.0"

#define FISHEYE_TRANSFORM_LIB_MAJOR 0
#define FISHEYE_TRANSFORM_LIB_MINOR 1
#define FISHEYE_TRANSFORM_LIB_PATCH 0
#define FISHEYE_TRANSFORM_LIB_VERSION ((FISHEYE_TRANSFORM_LIB_MAJOR << 16) | \
                             (FISHEYE_TRANSFORM_LIB_MINOR << 8)  | \
                             FISHEYE_TRANSFORM_LIB_PATCH)

#define FISHEYE_TRANSFORM_VERSION_STR "0.1.0"

#endif
