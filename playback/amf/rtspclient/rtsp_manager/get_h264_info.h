#ifndef __AMBA_GET_H264_INFO_H__
#define __AMBA_GET_H264_INFO_H__

#if NEW_RTSP_CLIENT

struct H264_INFO{
    int width;
    int height;
    int time_base_num;
    int time_base_den;
};
int get_h264_info(unsigned char* sps,H264_INFO*info);

#endif //NEW_RTSP_CLIENT

#endif //__AMBA_GET_H264_INFO_H__

