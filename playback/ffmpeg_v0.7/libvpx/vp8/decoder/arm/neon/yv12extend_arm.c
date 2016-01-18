#include "vpx_scale/yv12config.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/vpxscale.h"

void vp8_yv12_copy_frame_func_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

void
vp8_yv12_copy_frame_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc)
{
    vp8_yv12_copy_frame_func_neon(src_ybc, dst_ybc);
    //printf("Border:%d; plane_stride:%d; plane_height:%d; plane_width:%d\n",dst_ybc->border,dst_ybc->y_stride,dst_ybc->y_height,dst_ybc->y_width);

    vp8_yv12_extend_frame_borders_ptr(dst_ybc);
}
