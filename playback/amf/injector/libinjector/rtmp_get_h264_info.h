#ifndef _GET_H264_INFO_H__
#define _GET_H264_INFO_H__

class H264_Helper{
public:
    struct H264_INFO{
        int width;
        int height;
        int time_base_num;
        int time_base_den;
    };
    static int get_h264_info(unsigned char *sps,H264_INFO*info);
};

#endif //_GET_H264_INFO_H__

