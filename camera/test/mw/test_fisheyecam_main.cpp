/*******************************************************************************
 * test_webcam_main.cpp
 *
 * History:
 *  Dec 26, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_middleware.h"

#define VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/warp_vin.conf"
#define VDEVICE_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/warp_vdevice.conf"
#define VIN0_VSYNC          "/proc/ambarella/vin0_vsync"

enum DewarpMode {
  DEWARP_MODE_NONE = 0,
  DEWARP_MODE_WALL_NORMAL,
  DEWARP_MODE_WALL_NONE_SUB_PANOR,
  DEWARP_MODE_CEIL_NORMAL_NS,
  DEWARP_MODE_CEIL_NORMAL_EW,
  DEWARP_MODE_CEIL_PANOR,
  DEWARP_MODE_CEIL_SUB,
  DEWARP_MODE_CEIL_360,
};

struct DewarpParam {
    DewarpMode          mode;
    uint32_t            trans_number;
    TransformMode       trans_modes[MAX_WARP_AREA_NUM];
    TransformParameters trans_param[MAX_WARP_AREA_NUM];
    uint32_t            stream_number;
    EncodeSize          stream_size[MAX_ENCODE_STREAM_NUM];
    DewarpParam(DewarpMode m,
                uint32_t trans_num,
                TransformMode* modes,
                TransformParameters* param,
                uint32_t stream_num,
                EncodeSize* size) :
        mode(m),
        trans_number(trans_num),
        stream_number(stream_num)
    {
      memcpy(trans_modes, modes, trans_number * sizeof(TransformMode));
      memcpy(trans_param, param, trans_number * sizeof(TransformParameters));
      memcpy(stream_size, size, stream_number * sizeof(EncodeSize));
    }
};

// Wall Normal
static TransformMode      wall_1_mode[] = {
    AM_TRANSFORM_MODE_NORMAL };
static TransformParameters wall_1_param[] = {
    TransformParameters(0, Rect(2048, 2048), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(), Rect())
};
static EncodeSize wall_1_stream[] = {
    EncodeSize(2048, 2048)
};

// Wall Sub + Panor
static TransformMode      wall_2_mode[] = {
    AM_TRANSFORM_MODE_NONE, AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_PANORAMA};
static TransformParameters wall_2_param[] = {
    TransformParameters(0, Rect(1024, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(), Rect(2048, 2048)),
    TransformParameters(1, Rect(1024, 1024, 1024, 0), Fraction(),
                      0, 0, AM_FISHEYE_ORIENT_NORTH,
                      Point(), PanTiltAngle(), Rect()),
    TransformParameters(2, Rect(2048, 1024, 0, 1024), Fraction(),
                       180, 0, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(), Rect()),
};
static EncodeSize wall_2_stream[] = {
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
    EncodeSize(1024, 1024),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 1024, 0))
};

// Ceiling Normal N+S
static TransformMode      ceil_1_mode[] = {
    AM_TRANSFORM_MODE_NORMAL,
    AM_TRANSFORM_MODE_NORMAL};
static TransformParameters ceil_1_param[] = {
    TransformParameters(0, Rect(2048, 1024), Fraction(),
                       0, 90, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(2048, 1024, 0, 1024), Fraction(),
                      0, 90, AM_FISHEYE_ORIENT_SOUTH,
                      Point(), PanTiltAngle(), Rect()),
};
static EncodeSize ceil_1_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Ceiling Normal W+E
static TransformMode      ceil_2_mode[] = {
    AM_TRANSFORM_MODE_NORMAL,
    AM_TRANSFORM_MODE_NORMAL};
static TransformParameters ceil_2_param[] = {
    TransformParameters(0, Rect(2048, 1024), Fraction(),
                       0, 90, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(2048, 1024, 0, 1024), Fraction(),
                      0, 90, AM_FISHEYE_ORIENT_EAST,
                      Point(), PanTiltAngle(), Rect()),
};
static EncodeSize ceil_2_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Ceiling Panorama
static TransformMode      ceil_3_mode[] = {
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA
};
static TransformParameters ceil_3_param[] = {
    TransformParameters(0, Rect(1024, 1024), Fraction(),
                       90, 90, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(1024, 1024, 1024, 0), Fraction(),
                      90, 90, AM_FISHEYE_ORIENT_NORTH,
                      Point(), PanTiltAngle(), Rect()),
    TransformParameters(2, Rect(1024, 1024, 0, 1024), Fraction(),
                       90, 90, AM_FISHEYE_ORIENT_EAST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(3, Rect(1024, 1024, 1024, 1024), Fraction(),
                      90, 90, AM_FISHEYE_ORIENT_SOUTH,
                      Point(), PanTiltAngle(), Rect()),

};
static EncodeSize ceil_3_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Ceiling Subregion
static TransformMode      ceil_4_mode[] = {
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION
};
static TransformParameters ceil_4_param[] = {
    TransformParameters(0, Rect(1024, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(-90, -40), Rect()),
    TransformParameters(1, Rect(1024, 1024, 1024, 0), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(0, -40), Rect()),
    TransformParameters(2, Rect(1024, 1024, 0, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_EAST,
                       Point(), PanTiltAngle(90, -40), Rect()),
    TransformParameters(3, Rect(1024, 1024, 1024, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_SOUTH,
                       Point(), PanTiltAngle(-180, -40), Rect()),

};
static EncodeSize ceil_4_stream[] = {
    EncodeSize(1024, 1024),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 1024, 0)),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 0, 1024)),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 1024, 1024)),
};

// Desktop Normal N+S
static TransformMode      desktop_1_mode[] = {
    AM_TRANSFORM_MODE_NORMAL,
    AM_TRANSFORM_MODE_NORMAL};
static TransformParameters desktop_1_param[] = {
    TransformParameters(0, Rect(2048, 1024), Fraction(),
                       0, 90, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(2048, 1024, 0, 1024), Fraction(),
                      0, 90, AM_FISHEYE_ORIENT_SOUTH,
                      Point(), PanTiltAngle(), Rect()),
};
static EncodeSize desktop_1_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Desktop Normal W+E
static TransformMode      desktop_2_mode[] = {
    AM_TRANSFORM_MODE_NORMAL,
    AM_TRANSFORM_MODE_NORMAL};
static TransformParameters desktop_2_param[] = {
    TransformParameters(0, Rect(2048, 1024), Fraction(),
                       0, 90, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(2048, 1024, 0, 1024), Fraction(),
                      0, 90, AM_FISHEYE_ORIENT_EAST,
                      Point(), PanTiltAngle(), Rect()),
};
static EncodeSize desktop_2_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Desktop Panorama
static TransformMode      desktop_3_mode[] = {
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA
};
static TransformParameters desktop_3_param[] = {
    TransformParameters(0, Rect(1024, 1024), Fraction(),
                       90, 90, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(1024, 1024, 1024, 0), Fraction(),
                      90, 90, AM_FISHEYE_ORIENT_SOUTH,
                      Point(), PanTiltAngle(), Rect()),
    TransformParameters(2, Rect(1024, 1024, 0, 1024), Fraction(),
                       90, 90, AM_FISHEYE_ORIENT_EAST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(3, Rect(1024, 1024, 1024, 1024), Fraction(),
                      90, 90, AM_FISHEYE_ORIENT_NORTH,
                      Point(), PanTiltAngle(), Rect()),

};
static EncodeSize desktop_3_stream[] = {
    EncodeSize(2048, 1024),
    EncodeSize(2048, 1024, 0, 0, Rect(0, 0, 0, 1024)),
};

// Desktop Subregion
static TransformMode      desktop_4_mode[] = {
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION,
    AM_TRANSFORM_MODE_SUBREGION
};
static TransformParameters desktop_4_param[] = {
    TransformParameters(0, Rect(1024, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(90, 40), Rect()),
    TransformParameters(1, Rect(1024, 1024, 1024, 0), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_NORTH,
                       Point(), PanTiltAngle(0, 40), Rect()),
    TransformParameters(2, Rect(1024, 1024, 0, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_EAST,
                       Point(), PanTiltAngle(-90, 40), Rect()),
    TransformParameters(3, Rect(1024, 1024, 1024, 1024), Fraction(),
                       0, 0, AM_FISHEYE_ORIENT_SOUTH,
                       Point(), PanTiltAngle(180, 40), Rect()),

};
static EncodeSize desktop_4_stream[] = {
    EncodeSize(1024, 1024),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 1024, 0)),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 0, 1024)),
    EncodeSize(1024, 1024, 0, 0, Rect(0, 0, 1024, 1024)),
};


// Ceiling 360 + Subregion
static TransformMode      ceil_360_mode[] = {
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_PANORAMA,
    AM_TRANSFORM_MODE_SUBREGION
};
static TransformParameters ceil_360_param[] = {
    TransformParameters(0, Rect(800, 608), Fraction(3, 2),
                       90, 90, AM_FISHEYE_ORIENT_WEST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(1, Rect(800, 608, 800, 0), Fraction(3, 2),
                      90, 90, AM_FISHEYE_ORIENT_NORTH,
                      Point(), PanTiltAngle(), Rect()),
    TransformParameters(2, Rect(800, 608, 1600, 0), Fraction(3, 2),
                       90, 90, AM_FISHEYE_ORIENT_EAST,
                       Point(), PanTiltAngle(), Rect()),
    TransformParameters(3, Rect(800, 608, 2400, 0), Fraction(3, 2),
                      90, 90, AM_FISHEYE_ORIENT_SOUTH,
                      Point(), PanTiltAngle(), Rect()),
    TransformParameters(4, Rect(800, 608, 3200, 0), Fraction(3, 2),
                       0, 0, AM_FISHEYE_ORIENT_SOUTH,
                       Point(), PanTiltAngle(-180, -40), Rect()),
};
static EncodeSize ceil_360_stream[] = {
    /*EncodeSize(3200, 800),
    EncodeSize(1280, 800, 0, 0, Rect(0, 0, 3200, 0)),
    EncodeSize(1280, 1280, 0, 1, Rect()),*/
    EncodeSize(3200, 608),
    EncodeSize(800, 600, 0, 0, Rect(0, 0, 3200, 0)),
    EncodeSize(1280, 1280, 0, 1, Rect()),
};

static TransformMode      notrans_mode = AM_TRANSFORM_MODE_NONE;
static TransformParameters notrans_param(0, Rect(2048, 2048), Fraction(),
                                  0, 0, AM_FISHEYE_ORIENT_NORTH,
                                  Point(), PanTiltAngle(), Rect(2048, 2048));
static EncodeSize         notrans_size(2048, 2048);

DewarpParam dewarp_none[] = {
        DewarpParam(DEWARP_MODE_NONE,
                    1,
                    &notrans_mode,
                    &notrans_param,
                    1,
                    &notrans_size)
};

DewarpParam dewarp_wall[] = {
    DewarpParam(DEWARP_MODE_WALL_NORMAL,
                sizeof(wall_1_mode) / sizeof(TransformMode),
                wall_1_mode,
                wall_1_param,
                sizeof(wall_1_stream) / sizeof(EncodeSize),
                wall_1_stream),
    DewarpParam(DEWARP_MODE_WALL_NONE_SUB_PANOR,
                sizeof(wall_2_mode) / sizeof(TransformMode),
                wall_2_mode,
                wall_2_param,
                sizeof(wall_2_stream) / sizeof(EncodeSize),
                wall_2_stream),
};

DewarpParam dewarp_ceil[] = {
    DewarpParam(DEWARP_MODE_CEIL_NORMAL_NS,
                sizeof(ceil_1_mode) / sizeof(TransformMode),
                ceil_1_mode,
                ceil_1_param,
                sizeof(ceil_1_stream) / sizeof(EncodeSize),
                ceil_1_stream),
    DewarpParam(DEWARP_MODE_CEIL_NORMAL_EW,
                sizeof(ceil_2_mode) / sizeof(TransformMode),
                ceil_2_mode,
                ceil_2_param,
                sizeof(ceil_2_stream) / sizeof(EncodeSize),
                ceil_2_stream),
    DewarpParam(DEWARP_MODE_CEIL_PANOR,
                sizeof(ceil_3_mode) / sizeof(TransformMode),
                ceil_3_mode,
                ceil_3_param,
                sizeof(ceil_3_stream) / sizeof(EncodeSize),
                ceil_3_stream),
    DewarpParam(DEWARP_MODE_CEIL_SUB,
                sizeof(ceil_4_mode) / sizeof(TransformMode),
                ceil_4_mode,
                ceil_4_param,
                sizeof(ceil_4_stream) / sizeof(EncodeSize),
                ceil_4_stream),
};

DewarpParam dewarp_desktop[] = {
    DewarpParam(DEWARP_MODE_CEIL_NORMAL_NS,
                sizeof(desktop_1_mode) / sizeof(TransformMode),
                desktop_1_mode,
                desktop_1_param,
                sizeof(desktop_1_stream) / sizeof(EncodeSize),
                desktop_1_stream),
    DewarpParam(DEWARP_MODE_CEIL_NORMAL_EW,
                sizeof(desktop_2_mode) / sizeof(TransformMode),
                desktop_2_mode,
                desktop_2_param,
                sizeof(desktop_2_stream) / sizeof(EncodeSize),
                desktop_2_stream),
    DewarpParam(DEWARP_MODE_CEIL_PANOR,
                sizeof(desktop_3_mode) / sizeof(TransformMode),
                desktop_3_mode,
                desktop_3_param,
                sizeof(desktop_3_stream) / sizeof(EncodeSize),
                desktop_3_stream),
    DewarpParam(DEWARP_MODE_CEIL_SUB,
                sizeof(desktop_4_mode) / sizeof(TransformMode),
                desktop_4_mode,
                desktop_4_param,
                sizeof(desktop_4_stream) / sizeof(EncodeSize),
                desktop_4_stream),
};



DewarpParam dewarp_360[] = {
        DewarpParam(DEWARP_MODE_CEIL_360,
                    sizeof(ceil_360_mode) / sizeof(TransformMode),
                    ceil_360_mode,
                    ceil_360_param,
                    sizeof(ceil_360_stream) / sizeof(EncodeSize),
                    ceil_360_stream)
};

static EncodeType stream_type[MAX_ENCODE_STREAM_NUM];

static void usage()
{
  ERROR("Usage: test_fisheyecam "
            "[none] | [wall] [0~1] | [ceiling] [0~3]| | [desktop] [0~3] | [360] [m/h][w] [f][interval]");
}

int main(int argc, char *argv[])
{
    if (argc < 3 ||
      (!is_str_equal("none", argv[1])
          && !is_str_equal("wall", argv[1])
          && !is_str_equal("ceiling", argv[1])
          && !is_str_equal("desktop", argv[1])
          && !is_str_equal("360", argv[1]))) {
    usage();
    exit(0);
  }

  AmConfig *config = new AmConfig();
  config->set_vin_config_path(VIN_CONFIG);
  config->set_vdevice_config_path(VDEVICE_CONFIG);

  AmFisheyeCam fishcam(config);
  FisheyeParameters *fish_config = config->fisheye_config();
  DewarpParam *dewarp = dewarp_wall;
  uint32_t dewarp_num = 0, dewarp_id = 0;
  uint32_t interval = 1;
  memset(stream_type, 0, sizeof(stream_type));

  if (is_str_equal("wall", argv[1])) {
    fish_config->mount = AM_FISHEYE_MOUNT_WALL;
    fish_config->layout.warp.width = fish_config->layout.warp.height = 2048;
    fish_config->layout.unwarp.width = fish_config->layout.unwarp.height = 2048;
    dewarp = dewarp_wall;
    dewarp_num = sizeof(dewarp_wall) / sizeof(dewarp_wall[0]);
    dewarp_id = atoi(argv[2]);
    dewarp_id = (dewarp_id < dewarp_num ? dewarp_id : 0);
  } else if (is_str_equal("ceiling", argv[1])) {
    fish_config->mount = AM_FISHEYE_MOUNT_CEILING;
    fish_config->layout.warp.width = fish_config->layout.warp.height = 2048;
    fish_config->layout.unwarp.width = fish_config->layout.unwarp.height = 2048;
    dewarp = dewarp_ceil;
    dewarp_num = sizeof(dewarp_ceil) / sizeof(dewarp_ceil[0]);
    dewarp_id = atoi(argv[2]);
    dewarp_id = (dewarp_id < dewarp_num ? dewarp_id : 0);
  } else if (is_str_equal("none", argv[1])) {
    fish_config->mount = AM_FISHEYE_MOUNT_WALL;
    fish_config->layout.warp.width = fish_config->layout.warp.height = 2048;
    fish_config->layout.unwarp.width = fish_config->layout.unwarp.height = 2048;
    dewarp = dewarp_none;
    dewarp_num = sizeof(dewarp_none) / sizeof(dewarp_none[0]);
    dewarp_id = 0;
  } else if (is_str_equal("desktop", argv[1])) {
    fish_config->mount = AM_FISHEYE_MOUNT_DESKTOP;
    fish_config->layout.warp.width = fish_config->layout.warp.height = 2048;
    fish_config->layout.unwarp.width = fish_config->layout.unwarp.height = 2048;
    dewarp = dewarp_desktop;
    dewarp_num = sizeof(dewarp_desktop) / sizeof(dewarp_desktop[0]);
    dewarp_id = atoi(argv[2]);
    dewarp_id = (dewarp_id < dewarp_num ? dewarp_id : 0);
  } else if (is_str_equal("360", argv[1])) {
    fish_config->mount = AM_FISHEYE_MOUNT_CEILING;
    fish_config->layout.warp.width =
        dewarp_360[0].stream_size[0].width + dewarp_360[0].stream_size[1].width;
    fish_config->layout.warp.height =  dewarp_360[0].stream_size[0].height;
    fish_config->layout.unwarp.width = fish_config->layout.unwarp.height = 1600;
    dewarp = dewarp_360;
    dewarp_num = sizeof(dewarp_360) / sizeof(dewarp_360[0]);
    dewarp_id = 0;
    stream_type[2] = ((argv[2][0] == 'm' || argv[2][0] == 'M') ?
        AM_ENCODE_TYPE_MJPEG : AM_ENCODE_TYPE_H264);
    dewarp[dewarp_id].stream_size[2].width =
        dewarp[dewarp_id].stream_size[2].height = atoi(&argv[2][1]);
    if(argc > 3) {
      interval = argv[3][0] == 'f'? atoi(&argv[3][1]) : 1;
    }
  }
  if (fishcam.init() && fishcam.start(fish_config)) {
    for (uint32_t i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
      if (stream_type[i] == AM_ENCODE_TYPE_NONE) {
        stream_type[i] = (
            i < dewarp[dewarp_id].stream_number ? AM_ENCODE_TYPE_H264 :
                AM_ENCODE_TYPE_NONE);
      }
      fishcam.set_stream_type(i, stream_type[i]);
    }

    if (fishcam.set_transform_mode_all(dewarp[dewarp_id].trans_number,
                                    dewarp[dewarp_id].trans_modes)
        && fishcam.set_transform_region(dewarp[dewarp_id].trans_number,
                                      dewarp[dewarp_id].trans_param)
        && fishcam.set_stream_size_all(dewarp[dewarp_id].stream_number,
                                    dewarp[dewarp_id].stream_size)
        && fishcam.start_encode()) {
      INFO("Start to encode %s [%d] successfully.", argv[1], dewarp_id);
    } else {
      ERROR("Failed to encode %s [%d].", argv[1], dewarp_id);
    }

    if (dewarp[dewarp_id].mode == DEWARP_MODE_CEIL_360) {
      int vin_fd = open(VIN0_VSYNC, O_RDONLY);
      if (vin_fd >= 0) {
        TransformParameters *sub_param = &dewarp_360[0].trans_param[4];
        char vin_array[8];
        uint32_t i = 0;
        do {
          read(vin_fd, vin_array, 8);
          i++;
          if(i == interval) {
            sub_param->pantilt_angle.pan = (sub_param->pantilt_angle.pan >= 180 ?
                -180 : sub_param->pantilt_angle.pan + 1);
            fishcam.set_transform_region(1, sub_param);
            i = 0;
          }
        } while (true);
      }
    }
    else if (dewarp[dewarp_id].mode == DEWARP_MODE_WALL_NONE_SUB_PANOR) {
      int vin_fd = open(VIN0_VSYNC, O_RDONLY);
      if (vin_fd >= 0) {
        TransformParameters *sub_param = &dewarp_wall[1].trans_param[1];
        int dir = 1;
        char vin_array[8];
        do {
          read(vin_fd, vin_array, 8);
          if (sub_param->pantilt_angle.pan <= -50 || sub_param->pantilt_angle.pan >= 50) {
            dir = -dir;
          }
          sub_param->pantilt_angle.pan += dir;
          fishcam.set_transform_region(1, sub_param);
        } while (true);
      }
    }

  } else {
    ERROR("Failed to start fisheye cam.");
  }

  delete config;
  return 0;
}
