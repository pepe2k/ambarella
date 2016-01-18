/**
 * lbr_control.h
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

#ifndef LBR_CONTROL_H_
#define LBR_CONTROL_H_
#include "lbr_api.h"

#define MAX_LBR_STREAM_NUM 4
#define LBR_MOTION_NONE_DELAY 12
#define LBR_MOTION_LOW_DELAY 6
#define LBR_MOTION_HIGH_DELAY 3
#define LBR_LIGHT_LOW_DELAY 3
#define LBR_LIGHT_MIDDLE_DELAY 6
#define LBR_LIGHT_HIGH_DELAY 12

struct LBRStream {
    u32 enable_lbr;
    u32 motion_control;
    u32 low_light_control;
    u32 frame_drop;
    lbr_bitrate_target_t bitrate_target;
};

struct event_msg_t {
    u8   event_type;
    u8   reserved[3];
    u32  sequence_num;
    u32  utc;
    u32  diff;
    u32  algo_type;/*algo_type: 1 : mse, 2: mog2*/
};/*Don't change the order !!!!*/

class AmLBRControl
{
  public:
    void lbr_main (void);
    void receive_motion_info_from_ipc_lbr(void *);
    bool get_lbr_enable_lbr(u32 stream_id, u32 *enable_lbr);
    bool set_lbr_enable_lbr(u32 stream_id, u32 enable_lbr);
    bool get_lbr_bitrate_target(u32 stream_id, lbr_bitrate_target_t *bitrate_target);
    bool set_lbr_bitrate_target(u32 stream_id, lbr_bitrate_target_t bitrate_target);
    bool get_lbr_drop_frame(u32 stream_id, u32 *enable);
    bool set_lbr_drop_frame(u32 stream_id, u32 enable);
  private:
    int check_state(void);
    int check_stream_format(iav_encode_format_ex_t *format);
    int init_lbr(void);
    void print_lbr_config (void);
  public:
    u32 config_changed;
    u32 mse_motion_low_threshold;
    u32 mse_motion_high_threshold;
    u32 mog2_motion_low_threshold;
    u32 mog2_motion_high_threshold;
    int noise_low_threshold;
    int noise_high_threshold;
    LBRStream stream_params[MAX_LBR_STREAM_NUM];
    u32 stream_format_changed;
  private:
    int32_t fd_iav;
    u8 event_type; /*1: motion */
    u32 motion_value;
    u32 motion_algo;
    int noise_value;
    LBR_STYLE lbr_style[MAX_LBR_STREAM_NUM];
    LBR_MOTION_LEVEL lbr_motion_level[MAX_LBR_STREAM_NUM];
    LBR_NOISE_LEVEL lbr_noise_level[MAX_LBR_STREAM_NUM];
    uint32_t stream_state[MAX_LBR_STREAM_NUM];
};

#endif /* LBR_CONTROL_H_ */
