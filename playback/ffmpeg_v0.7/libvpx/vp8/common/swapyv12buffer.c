/*
 *  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license and patent
 *  grant that can be found in the LICENSE file in the root of the source
 *  tree. All contributing project authors may be found in the AUTHORS
 *  file in the root of the source tree.
 */


#include "swapyv12buffer.h"

void vp8_swap_yv12_buffer(YV12_BUFFER_CONFIG *new_frame, YV12_BUFFER_CONFIG *last_frame)
{
    unsigned char *temp;

    temp = last_frame->buffer_alloc;
    last_frame->buffer_alloc = new_frame->buffer_alloc;
    new_frame->buffer_alloc = temp;

    temp = last_frame->y_buffer;
    last_frame->y_buffer = new_frame->y_buffer;
    new_frame->y_buffer = temp;

    temp = last_frame->u_buffer;
    last_frame->u_buffer = new_frame->u_buffer;
    new_frame->u_buffer = temp;

    temp = last_frame->v_buffer;
    last_frame->v_buffer = new_frame->v_buffer;
    new_frame->v_buffer = temp;

}
