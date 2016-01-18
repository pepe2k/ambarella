/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2012-3-8 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_MW_VERSION_H_
#define AM_MW_VERSION_H_

#define CAM_LIB_MAJOR 0
#define CAM_LIB_MINOR 1
#define CAM_LIB_PATCH 0
#define CAM_LIB_VERSION ((CAM_LIB_MAJOR << 16) | \
                             (CAM_LIB_MINOR << 8)  | \
                             CAM_LIB_PATCH)
#define CAM_VERSION_STR "0.1.0"

#define WEBCAM_LIB_MAJOR 0
#define WEBCAM_LIB_MINOR 1
#define WEBCAM_LIB_PATCH 0
#define WEBCAM_LIB_VERSION ((WEBCAM_LIB_MAJOR << 16) | \
                               (WEBCAM_LIB_MINOR << 8)  | \
                               WEBCAM_LIB_PATCH)
#define WEBCAM_VERSION_STR "0.1.0"

#define FISHEYECAM_LIB_MAJOR 0
#define FISHEYECAM_LIB_MINOR 1
#define FISHEYECAM_LIB_PATCH 0
#define FISHEYECAM_LIB_VERSION ((FISHEYECAM_LIB_MAJOR << 16) | \
                               (FISHEYECAM_LIB_MINOR << 8)  | \
                               FISHEYECAM_LIB_PATCH)
#define FISHEYECAM_VERSION_STR "0.1.0"

#endif /* AM_MW_VERSION_H_ */
