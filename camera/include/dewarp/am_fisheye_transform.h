/*******************************************************************************
 * am_fisheye_transform.h
 *
 * History:
 *  Mar 21, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_FISHEYE_TRANSFORM_H_
#define AM_FISHEYE_TRANSFORM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "lib_dewarp.h"
#ifdef __cplusplus
}
#endif

class AmFisheyeTransform {
  public:
    AmFisheyeTransform();
    ~AmFisheyeTransform();

  public:
    void         set_transform_config(FisheyeParameters *config);
    bool         init();

    uint32_t     get_max_area_num(TransformMode mode);
    FisheyeMount get_mount_mode();
    uint32_t     create_warp_control(WarpControl *ctrl, const TransformMode mode,
                                   TransformParameters *trans);

  private:
    warp_region_t* get_lib_region(const Rect *region, const Fraction *zoom);
    warp_vector_t* get_lib_vector(WarpControl *ctrl, uint32_t areaNum);
    ORIENTATION    get_lib_orient(const FisheyeOrient orient);
    uint32_t       create_lib_notrans(WarpControl *ctrl, const TransformParameters *trans);
    uint32_t       create_lib_normal(WarpControl *ctrl, const TransformParameters *trans);
    uint32_t       create_lib_panorama(WarpControl *ctrl, const TransformParameters *trans);
    uint32_t       create_lib_subregion(WarpControl *ctrl, TransformParameters *trans);

  private:
    FisheyeParameters *mFisheyeParams;

    fisheye_config_t mConfig;
    warp_vector_t    mVector[MAX_WARP_AREA_NUM];
    warp_region_t    mRegion;

  private:
    bool    IsInited;
};


#endif /* AM_FISHEYE_TRANSFORM_H_ */
