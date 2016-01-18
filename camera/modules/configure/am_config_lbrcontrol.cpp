/**
 * am_config_lbrcontrol.cpp
 *
 *  History:
 *		Mar 13, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif
#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_lbrcontrol.h"

LBRParameters* AmConfigLBR::get_lbr_config()
{
    LBRParameters *ret = NULL;
    uint32_t i = 0;
    char *string = (char *)malloc(sizeof(char) * 128);
    if (init()) {
        if (!mLBRParameters) {
            mLBRParameters = new LBRParameters ();
        }
        if (mLBRParameters) {
            uint32_t mse_motion_low_threshold = get_int("MotionStatistics:"
                    "MSEMotionLowThreshold", 10);
            if (mse_motion_low_threshold > 0) {
                mLBRParameters->mse_motion_low_threshold =
                        mse_motion_low_threshold;
            } else {
               WARN("Invalid MSE motion low threshold: %d, reset to 10!",
                    mse_motion_low_threshold);
               mLBRParameters->mse_motion_low_threshold = 10;
               mLBRParameters->config_changed = 1;
            }

            uint32_t mse_motion_high_threshold = get_int("MotionStatistics:"
                    "MSEMotionHighThreshold", 1000);
            if (mse_motion_high_threshold > 0) {
                mLBRParameters->mse_motion_high_threshold =
                        mse_motion_high_threshold;
            } else {
               WARN("Invalid MSE motion high threshold: %d, reset to 1000!",
                    mse_motion_high_threshold);
               mLBRParameters->mse_motion_high_threshold = 1000;
               mLBRParameters->config_changed = 1;
            }

            uint32_t mog2_motion_low_threshold = get_int("MotionStatistics:"
                    "MOG2MotionLowThreshold", 10);
            if (mog2_motion_low_threshold > 0) {
                mLBRParameters->mog2_motion_low_threshold =
                        mog2_motion_low_threshold;
            } else {
               WARN("Invalid MOG2 motion low threshold: %d, reset to 10!",
                    mog2_motion_low_threshold);
               mLBRParameters->mog2_motion_low_threshold = 10;
               mLBRParameters->config_changed = 1;
            }

            uint32_t mog2_motion_high_threshold = get_int("MotionStatistics:"
                    "MOG2MotionHighThreshold", 1000);
            if (mog2_motion_high_threshold > 0) {
                mLBRParameters->mog2_motion_high_threshold =
                        mog2_motion_high_threshold;
            } else {
               WARN("Invalid MOG2 motion high threshold: %d, reset to 1000!",
                    mog2_motion_high_threshold);
               mLBRParameters->mog2_motion_high_threshold = 1000;
               mLBRParameters->config_changed = 1;
            }

            uint32_t noise_low_threshold = get_int("NoiseStatistics:"
                    "NoiseLowThreshold", 1);
            if (noise_low_threshold > 0) {
                mLBRParameters->noise_low_threshold = noise_low_threshold;
            } else {
               WARN("Invalid noise low threshold: %d, reset to 1!",
                    noise_low_threshold);
               mLBRParameters->noise_low_threshold = 10;
               mLBRParameters->config_changed = 1;
            }

            uint32_t noise_high_threshold = get_int("NoiseStatistics:"
                    "NoiseHighThreshold", 6);
            if (noise_high_threshold > 0) {
                mLBRParameters->noise_high_threshold = noise_high_threshold;
            } else {
               WARN("Invalid noise high threshold: %d, reset to 6!",
                    noise_high_threshold);
               mLBRParameters->noise_high_threshold = 6;
               mLBRParameters->config_changed = 1;
            }

            uint32_t enable_lbr = 0, motion_control = 0, low_light_control = 0,
                    frame_drop = 0, auto_bitrate_target = 0,
                    bitrate_ceiling = 0;
            for (i = 0; i < AM_STREAM_ID_MAX; i++) {
                sprintf(string, "%s%d", "StreamConfig:EnableLBR", i);
                enable_lbr = get_int(string, i);
                if (enable_lbr == 0 || enable_lbr == 1) {
                    mLBRParameters->stream_params[i].enable_lbr = enable_lbr;
                } else {
                    WARN("Invalid enable lbr stream%d: %d, reset to 0!",
                         i, enable_lbr);
                    mLBRParameters->stream_params[i].enable_lbr = 0;
                    mLBRParameters->config_changed = 1;
                }

                sprintf(string, "%s%d", "StreamConfig:MotionControl", i);
                motion_control = get_int(string, i);
                if (motion_control == 0 || motion_control == 1) {
                    mLBRParameters->stream_params[i].motion_control =
                            motion_control;
                } else {
                    WARN("Invalid motion control stream%d: %d, reset to 1!",
                         i, motion_control);
                    mLBRParameters->stream_params[i].motion_control = 1;
                    mLBRParameters->config_changed = 1;
                }

                sprintf(string, "%s%d", "StreamConfig:LowLightControl", i);
                low_light_control = get_int(string, i);
                if (low_light_control == 0 || low_light_control == 1) {
                    mLBRParameters->stream_params[i].low_light_control =
                            low_light_control;
                } else {
                    WARN("Invalid low light control stream%d: %d, reset to 1!",
                         i, low_light_control);
                    mLBRParameters->stream_params[i].low_light_control = 1;
                    mLBRParameters->config_changed = 1;
                }

                sprintf(string, "%s%d", "StreamConfig:FrameDrop", i);
                frame_drop = get_int(string, i);
                if (frame_drop == 0 || frame_drop == 1) {
                    mLBRParameters->stream_params[i].frame_drop = frame_drop;
                } else {
                    WARN("Invalid frame drop stream%d: %d, reset to 1!",
                         i, frame_drop);
                    mLBRParameters->stream_params[i].frame_drop = 1;
                    mLBRParameters->config_changed = 1;
                }

                sprintf(string, "%s%d", "StreamConfig:AutoBitrateTarget", i);
                auto_bitrate_target = get_int(string, i);
                if (auto_bitrate_target == 0 || auto_bitrate_target == 1) {
                    mLBRParameters->stream_params[i].auto_target =
                            auto_bitrate_target;
                } else {
                    WARN("Invalid auto auto_bitrate_target stream%d: %d, "
                            "reset to 1!", i, auto_bitrate_target);
                    mLBRParameters->stream_params[i].auto_target = 1;
                    mLBRParameters->config_changed = 1;
                }

                sprintf(string, "%s%d", "StreamConfig:BitrateCeiling", i);
                bitrate_ceiling = get_int(string, i);
                if (bitrate_ceiling > 0) {
                    mLBRParameters->stream_params[i].bitrate_ceiling =
                            bitrate_ceiling;
                } else {
                    WARN("Invalid auto bitrate ceiling stream%d: %d, "
                            "reset to 1M!", i, bitrate_ceiling);
                    mLBRParameters->stream_params[i].bitrate_ceiling = 142;
                    mLBRParameters->config_changed = 1;
                }

            }
            free(string);
        }
        ret = mLBRParameters;
    }
    return ret;
}

void AmConfigLBR::set_lbr_config(LBRParameters *config)
{
    uint32_t i;
    char* string = (char *)malloc(sizeof(char) * 128);
    if (AM_LIKELY(config)) {
        if (init()) {
            set_value("MotionStatistics:MSEMotionLowThreshold",
                      config->mse_motion_low_threshold);
            set_value("MotionStatistics:MSEMotionHighThreshold",
                      config->mse_motion_high_threshold);
            set_value("MotionStatistics:MOG2MotionLowThreshold",
                      config->mog2_motion_low_threshold);
            set_value("MotionStatistics:MOG2MotionHighThreshold",
                      config->mog2_motion_high_threshold);
            set_value("NoiseStatistics:NoiseLowThreshold",
                      config->noise_low_threshold);
            set_value("NoiseStatistics:NoiseHighThreshold",
                      config->noise_high_threshold);

            for (i = 0; i < AM_STREAM_ID_MAX; i++) {
                sprintf(string, "%s%d", "StreamConfig:EnableLBR", i);
                set_value(string, config->stream_params[i].enable_lbr);
                sprintf(string, "%s%d", "StreamConfig:MotionControl", i);
                set_value(string, config->stream_params[i].motion_control);
                sprintf(string, "%s%d", "StreamConfig:LowLightControl", i);
                set_value(string, config->stream_params[i].low_light_control);
                sprintf(string, "%s%d", "StreamConfig:FrameDrop", i);
                set_value(string, config->stream_params[i].frame_drop);
                sprintf(string, "%s%d", "StreamConfig:AutoBitrateTarget", i);
                set_value(string, config->stream_params[i].auto_target);
                sprintf(string, "%s%d", "StreamConfig:BitrateCeiling", i);
                set_value(string, config->stream_params[i].bitrate_ceiling);
            }
            config->config_changed = 0;
            save_config();
        } else {
            WARN("Failed openint %s, LBR configuration NOT saved!",
                 mConfigFile);
        }
    }
    free(string);

}
