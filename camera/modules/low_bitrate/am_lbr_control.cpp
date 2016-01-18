/**
 * am_lbr_control.cpp
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
#include "am_include.h"
#include "am_utility.h"
#include "am_lbrcontrol.h"
#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "mw_struct.h"
#include "mw_api.h"

#define DIV_ROUND(divident, divider)   (((divident)+((divider)>>1)) / (divider))

int AmLBRControl::check_state(void)
{
    int state;
    if (ioctl(fd_iav, IAV_IOC_GET_STATE, &state) < 0) {
        ERROR("IAV_IOC_GET_STATE");
        return -1;
    }

    if ((state != IAV_STATE_ENCODING) && (state != IAV_STATE_PREVIEW )) {
        ERROR("IAV is not in encoding or preview state, cannot control LBR!\n");
        return -1;
    }

    return 0;
}

int AmLBRControl::check_stream_format(iav_encode_format_ex_t *format)
{
    if (ioctl(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, format) < 0) {
        return -1;
    }
    //printf("check stream%d format is %d!\n", format->id, format->encode_type);

    return 0;
}

void AmLBRControl::print_lbr_config (void)
{
    uint32_t i = 0;
    printf("MSEMotionLowThreshold   = %d\n", mse_motion_low_threshold);
    printf("MSEMotionHighThreshold  = %d\n", mse_motion_high_threshold);
    printf("MOG2MotionLowThreshold  = %d\n", mog2_motion_low_threshold);
    printf("MOG2MotionHighThreshold = %d\n", mog2_motion_high_threshold);
    printf("NoiseLowThreshold       = %d\n", noise_low_threshold);
    printf("NoiseHighThreshold      = %d\n", noise_high_threshold);
    for (i = 0; i < MAX_LBR_STREAM_NUM; i++) {
        printf("EnableLBR%d              = %d\n", i,
               stream_params[i].enable_lbr);
        printf("MotionControl%d          = %d\n", i,
               stream_params[i].motion_control);
        printf("LowLightControl%d        = %d\n", i,
               stream_params[i].low_light_control);
        printf("AutoBitrateTarget%d      = %d\n", i,
               stream_params[i].bitrate_target.auto_target);
        printf("BitrateCeiling%d         = %d\n", i,
               stream_params[i].bitrate_target.bitrate_ceiling);
    }
}

int AmLBRControl::init_lbr(void)
{
    uint32_t i = 0;
    lbr_init_t lbr;
    iav_encode_format_ex_t format;
    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        return -1;
    }

    if (check_state() < 0) {
        return -1;
    }

    if (lbr_open() < 0) {
        ERROR("lbr open failed \n");
        return -1;
    }

    memset(&lbr, 0, sizeof(lbr));
    lbr.fd_iav = fd_iav;
    if (lbr_init(&lbr) < 0) {
        ERROR("lbr init failed \n");
        return -1;
    }
    //print_lbr_config ();
    for (i = 0; i < MAX_LBR_STREAM_NUM ; i++) {
        format.id = (1 << i);
        check_stream_format(&format);
        if (format.encode_type != IAV_ENCODE_H264) {
            stream_state[i] = 0;
            WARN("stream %d is not H.264 \n", i);
        } else {
            stream_state[i] = 1;
            /*transfer bitrate from bps/MB to normal format*/
            stream_params[i].bitrate_target.bitrate_ceiling =
                    ((format.encode_width * format.encode_height) >> 8) *
                    stream_params[i].bitrate_target.bitrate_ceiling;
            /*printf("stream%d: resolution is %dx%d, bitrate ceiling is %d\n\n",
                   i, format.encode_width, format.encode_height ,
                   stream_params[i].bitrate_target.bitrate_ceiling);*/

            lbr_motion_level[i] = LBR_MOTION_LOW;
            lbr_noise_level[i] = LBR_NOISE_NONE;

            if (stream_params[i].enable_lbr == 0) {
                lbr_style[i] = LBR_STYLE_FPS_KEEP_CBR_ALIKE;
                if (lbr_set_style(lbr_style[i], i) < 0) {
                    ERROR("lbr set style %d for stream %d failed\n",
                         lbr_style[i], i);
                }
            } else {
                if (stream_params[i].frame_drop == 1) {
                     lbr_style[i] = LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP;
                } else {
                     lbr_style[i] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
                }
                if(lbr_set_style(lbr_style[i], i) < 0) {
                    ERROR("lbr set style %d for stream %d failed\n",
                         lbr_style[i], i);
                }

                if (stream_params[i].motion_control) {
                    if( lbr_set_motion_level(lbr_motion_level[i], i) < 0) {
                        ERROR("lbr set motion level %d for stream %d failed\n",
                             lbr_motion_level[i], i);
                    }
                }

                if (stream_params[i].low_light_control) {
                    if( lbr_set_noise_level(lbr_noise_level[i], i) < 0) {
                        ERROR("lbr set noise level %d for stream %d failed\n",
                             lbr_noise_level[i], i);
                    }
                }
            }
            if(lbr_set_bitrate_ceiling(&stream_params[i].bitrate_target, i)
                    < 0) {
                ERROR("lbr set bitrate ceiling %d for stream %d failed\n",
                     stream_params[i].bitrate_target.bitrate_ceiling, i);
            }
        }
    }

    return 0;
}

void AmLBRControl::receive_motion_info_from_ipc_lbr(void *msg_data)
{
    event_msg_t *msg = (event_msg_t *)msg_data;
    event_type = msg->event_type;
    motion_value = msg->diff;
    motion_algo = msg->algo_type;
}

void AmLBRControl::lbr_main (void)
{
    uint32_t i = 0;
    uint32_t lbr_stream = 0;
    for (i = 0; i < MAX_LBR_STREAM_NUM ; i++) {
        if (stream_params[i].enable_lbr == 1) {
            lbr_stream++;
        }
        if (lbr_stream == 0) {
            return;//if no stream use LBR, return
        }
    }
    u8 motion_or_not = 0;
    uint32_t motion_low_threshold = 0, motion_high_threshold = 0;
    uint32_t motion = 0, motion_algo_type = 1;
    int sensor_gain = 0;
    uint32_t i0_motion = 0, i1_motion = 0, i2_motion = 0;
    uint32_t i0_light = 0, i1_light = 0, i2_light = 0;
    LBR_MOTION_LEVEL motion_level = LBR_MOTION_LOW,
            motion_level_backup = LBR_MOTION_NONE;
    LBR_NOISE_LEVEL noise_level = LBR_NOISE_LOW,
            noise_level_backup = LBR_NOISE_NONE;
    iav_encode_format_ex_t format;
    if (init_lbr() < 0) {
        ERROR("Init LBR failed!!\n");
        return;
    }
    while (1) {
        usleep(200000);
        /*get motion statistics*/
        memcpy(&motion_or_not, &event_type, sizeof(event_type));
        if (motion_or_not == 1) {
            memcpy(&motion_algo_type, &motion_algo, sizeof(motion_algo));
            memcpy(&motion, &motion_value, sizeof(motion_value));
        }
        if (motion_algo_type == 1) {
            motion_low_threshold = mse_motion_low_threshold;
            motion_high_threshold = mse_motion_high_threshold;
        } else if (motion_algo_type == 2) {
            motion_low_threshold = mog2_motion_low_threshold;
            motion_high_threshold = mog2_motion_high_threshold;
        } else {
            ERROR("Unknown motion statistic type!\n");
        }
        /*printf("LBR: motion_algo = %d (1:MSE,2:MOG2), motion value = %d\n\n",
               motion_algo_type, motion);
        */
        if (motion < motion_low_threshold) {
            i0_motion ++;
            i1_motion = 0;
            i2_motion = 0;
        } else if (motion < motion_high_threshold) {
            i0_motion = 0;
            i1_motion ++;
            i2_motion = 0;
        } else {
            i0_motion = 0;
            i1_motion = 0;
            i2_motion ++;
        }
        if (i0_motion == LBR_MOTION_NONE_DELAY) {
            motion_level = LBR_MOTION_NONE;
            i0_motion = 0;
        }
        if (i1_motion == LBR_MOTION_LOW_DELAY) {
            motion_level = LBR_MOTION_LOW;
            i1_motion = 0;
        }
        if (i2_motion == LBR_MOTION_HIGH_DELAY) {
            motion_level = LBR_MOTION_HIGH;
            i2_motion = 0;
        }
        /*get noise statistics*/
        if (mw_get_sensor_gain(fd_iav, &sensor_gain) < 0) {
            ERROR("get ae sensor gain failed!\n");
        } else {
            noise_value = sensor_gain >> 24;
        }
        //printf("LBR: sensor gain = %ddB\n\n", noise_value);
        if (noise_value > noise_low_threshold) {
            i0_light ++;
            i1_light = 0;
            i2_light = 0;
        } else if (noise_value > noise_high_threshold) {
            i0_light = 0;
            i1_light ++;
            i2_light = 0;
        } else {
            i0_light = 0;
            i1_light = 0;
            i2_light ++;
        }
        if (i0_light == LBR_LIGHT_LOW_DELAY) {
            noise_level = LBR_NOISE_HIGH;
            i0_light = 0;
        }
        if (i1_light == LBR_LIGHT_MIDDLE_DELAY) {
            noise_level = LBR_NOISE_LOW;
            i1_light = 0;
        }
        if (i2_light == LBR_LIGHT_HIGH_DELAY) {
            noise_level = LBR_NOISE_NONE;
            i2_light = 0;
        }
        for (i = 0; i < MAX_LBR_STREAM_NUM ; i++) {
            if (stream_format_changed == 1) {
                format.id = (1 << i);
                check_stream_format(&format);
                if (format.encode_type != IAV_ENCODE_H264) {
                    stream_state[i] = 0;
                    continue;
                } else {
                    stream_state[i] = 1;
                }
            }
            if ((stream_params[i].enable_lbr == 1) && stream_state[i] == 1) {
                if (motion_level != motion_level_backup) {
                    if (stream_params[i].motion_control == 1) {
                        lbr_set_motion_level(motion_level, i);
                    }
                }
                if (noise_level != noise_level_backup) {
                    if (stream_params[i].low_light_control == 1) {
                        lbr_set_noise_level(noise_level, i);
                    }
                }
            }
        }
        motion_level_backup = motion_level;
        noise_level_backup = noise_level;
        stream_format_changed = 0;
    }
    close(fd_iav);
}

bool AmLBRControl::get_lbr_enable_lbr(u32 stream_id, u32 *enable_lbr)
{
    enable_lbr = &stream_params[stream_id].enable_lbr;
    return true;
}

bool AmLBRControl::set_lbr_enable_lbr(u32 stream_id, u32 enable_lbr)
{
    iav_encode_format_ex_t format;
    format.id = (1 << stream_id);
    check_stream_format(&format);
    if (format.encode_type != IAV_ENCODE_H264) {
        ERROR("stream %d is not H.264 \n", stream_id);
        return false;
    }
    stream_params[stream_id].enable_lbr = enable_lbr;
    if (stream_params[stream_id].enable_lbr == 0) {
        if (lbr_set_style(LBR_STYLE_FPS_KEEP_CBR_ALIKE, stream_id) < 0) {
            ERROR("lbr set style %d for stream %d failed\n",
                 lbr_style[stream_id], stream_id);
        }
    } else {
        if( lbr_set_style(LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP,
                          stream_id) < 0) {
            ERROR("lbr set style %d for stream %d failed\n",
                 lbr_style[stream_id], stream_id);
        }
    }

    return true;
}

bool AmLBRControl::get_lbr_bitrate_target(u32 stream_id,
                                          lbr_bitrate_target_t *bitrate_target)
{
    iav_encode_format_ex_t format;
    format.id = (1 << stream_id);
    check_stream_format(&format);
    if (format.encode_type != IAV_ENCODE_H264) {
        ERROR("stream %d is not H.264 \n", stream_id);
        return false;
    }
    if (lbr_get_bitrate_ceiling(bitrate_target, stream_id) < 0) {
        ERROR("Get lbr bitrate target failed!\n");
        return false;
    }

    /*transfer bitrate from  normal format to bps/MB */
    bitrate_target->bitrate_ceiling = bitrate_target->bitrate_ceiling /
            ((format.encode_width * format.encode_height) >> 8);

    return true;
}

bool AmLBRControl::set_lbr_bitrate_target(u32 stream_id,
                                          lbr_bitrate_target_t bitrate_target)
{
    iav_encode_format_ex_t format;
    format.id = (1 << stream_id);
    check_stream_format(&format);
    if (format.encode_type != IAV_ENCODE_H264) {
        ERROR("stream %d is not H.264 \n", stream_id);
        return false;
    }

    /*transfer bitrate from bps/MB to normal format*/
    bitrate_target.bitrate_ceiling =
            ((format.encode_width * format.encode_height) >> 8) *
            bitrate_target.bitrate_ceiling;

    /*printf("stream%d: resolution is %dx%d, bitrate ceiling is %d bps\n\n",
           stream_id, format.encode_width, format.encode_height,
           bitrate_target.bitrate_ceiling);*/

    if (lbr_set_bitrate_ceiling(&bitrate_target,
                                stream_id) < 0) {
        ERROR("lbr set bitrate target for stream %d failed\n", stream_id);
        return false;
    }

    return true;
}

bool AmLBRControl::get_lbr_drop_frame(u32 stream_id, u32 *enable)
{
    enable = &stream_params[stream_id].frame_drop;
    return true;
}

bool AmLBRControl::set_lbr_drop_frame(u32 stream_id, u32 enable)
{
    iav_encode_format_ex_t format;
    format.id = (1 << stream_id);
    check_stream_format(&format);
    if (format.encode_type != IAV_ENCODE_H264) {
        ERROR("stream %d is not H.264 \n", stream_id);
        return false;
    }
    stream_params[stream_id].frame_drop = enable;
    if (stream_params[stream_id].enable_lbr == 1) {
        if (stream_params[stream_id].frame_drop == 1) {
            lbr_style[stream_id] = LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP;
        } else {
            lbr_style[stream_id] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
        }
        if( lbr_set_style(lbr_style[stream_id], stream_id) < 0) {
            ERROR("lbr set style %d for stream %d failed\n",
                 lbr_style[stream_id], stream_id);
        }
    }

    return true;
}
