#include <stdio.h>
#include <string.h>
#include "rtmp_get_h264_info.h"

/*copy from amba_create_mp4.cpp and modified.
*/

/*
67 42 e0 0a 89 95 42 c1 2c 80     （67为sps头）
68 ce 05 8b 72                              （68为pps头）

0100 0010 1110 0000 0000 1010 1000 1001 1001 0101 0100 0010 1100 0001 0010 1100 1000 0000 sps
FIELD                                                                No. of BITS                   VALUE                   CodeNum                 描述符
profile_idc                                                                     8                         01000010                   66                     u(8)
constraint_set0_flag                                                     1                          1                                                          u(1)
constraint_set1_flag                                                     1                          1                                                          u(1)
constraint_set2_flag                                                     1                          1                                                          u(1)
constraint_set3_flag                                                     1                           0                                                         u(1)
reserved_zero_4bits                                                     4                          0000                                                    u(4)
level_idc                                                                        8                          00001010                 10                       u(8)
seq_parameter_set_id                                                  1                          1                                 0                        ue(v)
log2_max_frame_num_minus4                                      7                          0001001                     8                       ue(v)
pic_order_cnt_type                                                        1                          1                                 0                       ue(v)
log2_max_pic_order_cnt_lsb_minus4                            5                          00101                         4                       ue(v)
num_ref_frames                                                             3                          010                                                      ue(v) 
gaps_in_frame_num_value_allowed_flag                       1                          1                                                          u(1)
pic_width_in_mbs_minus1                                               9                        000010110                  20                     ue(v)
pic_height_in_map_units_minus1                                    9                        000010010                 16                      ue(v)
frame_mbs_only_flag                                                       1                        1                                  0                       u(1)
mb_adaptive_frame_field_flag                                         1                        1                                   0                      u(1)
direct_8x8_inference_flag                                                1                         0                                                          u(1)
frame_cropping_flag                                                        1                         0                                                          u(1)
vui_parameters_present_flag                                          1                         1                                  0                       u(1)



1100 1110 0000 0101 1000 1011 0111 0010  pps
FIELD                                                                   No. of BITS                 VALUE                   CodeNum                 描述符
pic_parameter_set_id                                                      1                           1                            0                          ue(v)
seq_parameter_set_id                                                     1                           1                            0                          ue(v)
entropy_coding_mode_flag                                              1                           0                                                        ue(1)
pic_order_present_flag                                                    1                            0                                                       ue(1)
num_slice_groups_minus1                                               1                            1                           0                          ue(v)       
num_ref_idx_l0_active_minus1                                         1                           1                            0                         ue(v)
num_ref_idx_l1_active_minus1                                         1                           1                            0                         ue(v)
weighted_pred_flag                                                           1                           0                                                       ue(1)
weighted_bipred_idc                                                          2                           00                                                     ue(2)
pic_init_qp_minus26                                                          7                           0001011               10(-5)                   se(v)
pic_init_qs_minus26                                                          7                           0001011               10(-5)                   se(v)
chroma_qp_index_offset                                                   3                           011                        2(-1)                    se(v)
deblocking_filter_control_present_flag                             1                            1                                                       ue(1)
constrained_intra_pred_flag                                             1                            0                                                       ue(1)
redundant_pic_cnt_present_flag                                      1                            0                                                       ue(1) 
*/

static unsigned int read_bit(unsigned char* pBuffer, int* value, unsigned char * bit_pos = NULL, unsigned int  num = 1){
    *value = 0;
    unsigned int  i=0;
    for (unsigned int  j =0 ; j<num; j++){
        if (*bit_pos == 8){
            *bit_pos = 0;
            i++;
        }
        if (*bit_pos == 0){
            if (pBuffer[i] == 0x3 &&
               *(pBuffer+i-1) == 0 &&
               *(pBuffer+i-2) == 0)
                i++;
        }
        *value  <<= 1;
        *value  += pBuffer[i] >> (7 -(*bit_pos)++) & 0x1;
    }
    return i;
};

static unsigned int parse_exp_codes(unsigned char *pBuffer, int* value,  unsigned char * bit_pos = NULL,unsigned char type = 0){
    int leadingZeroBits = -1;
    unsigned int  i=0;
    unsigned char j=*bit_pos;
    for (unsigned char b = 0; !b; leadingZeroBits++,j++){
        if(j == 8){
            i++;
            j=0;
        }
        if (j == 0){
            if (pBuffer[i] == 0x3 &&
                *(pBuffer+i-1) == 0 &&
                *(pBuffer+i-2) == 0)
                i++;
        }
        b = pBuffer[i] >> (7 -j) & 0x1;
    }
    int codeNum = 0;
    i += read_bit(pBuffer+i,  &codeNum, &j, leadingZeroBits);
    codeNum += (1 << leadingZeroBits) -1;
    if (type == 0){
         //ue(v)
        *value = codeNum;
    }else if (type == 1) {
        //se(v)
        *value = (codeNum/2+1);
        if (codeNum %2 == 0){
            *value *= -1;
        }
    }
    *bit_pos = j;
    return i;
}

#define parse_ue(x,y,z)		parse_exp_codes((x),(y),(z),0)
#define parse_se(x,y,z)		parse_exp_codes((x),(y),(z),1)

static int parse_scaling_list(unsigned char* pBuffer,unsigned int sizeofScalingList,unsigned char* bit_pos){
    int* scalingList = new int[sizeofScalingList];
    unsigned char useDefaultScalingMatrixFlag;
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;
    int i = 0;
    for (unsigned int j =0; j<sizeofScalingList; j++ ){
        if (nextScale != 0){
            i += parse_se(pBuffer, &delta_scale,bit_pos);
            nextScale = (lastScale+ delta_scale + 256)%256;
            useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
            useDefaultScalingMatrixFlag = useDefaultScalingMatrixFlag;
        }
        scalingList[j] = ( nextScale == 0 ) ? lastScale : nextScale;
        lastScale = scalingList[j];
    }
    delete[] scalingList;
    return i;
};

int H264_Helper::get_h264_info(unsigned char* pBuffer, H264_Helper::H264_INFO* pH264Info)
{
    memset(pH264Info,0,sizeof(H264_INFO));
    unsigned char* pSPS = pBuffer;
    unsigned char bit_pos = 0;
    int profile_idc; // u(8)
    int constraint_set; //u(8)
    int level_idc; //u(8)
    int seq_paramter_set_id = 0;
    int chroma_format_idc = 0;
    int residual_colour_tramsform_flag;
    int bit_depth_luma_minus8 = 0;
    int bit_depth_chroma_minus8 = 0;
    int qpprime_y_zero_transform_bypass_flag;
    int seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag[8] ;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int num_ref_frames;
    int gaps_in_frame_num_value_allowed_flag;
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int direct_8x8_inference_flag;

    pSPS += read_bit(pSPS, &profile_idc,&bit_pos,8);
    pSPS += read_bit(pSPS, &constraint_set,&bit_pos,8);
    pSPS += read_bit(pSPS, &level_idc, &bit_pos,8);
    pSPS += parse_ue(pSPS,&seq_paramter_set_id, &bit_pos);
    if (profile_idc == 100 ||profile_idc == 110 ||profile_idc == 122 ||profile_idc == 144 ){
        pSPS += parse_ue(pSPS,&chroma_format_idc,&bit_pos);
        if (chroma_format_idc == 3)
            pSPS += read_bit(pSPS, &residual_colour_tramsform_flag, &bit_pos);
        pSPS += parse_ue(pSPS,&bit_depth_luma_minus8,&bit_pos);
        pSPS += parse_ue(pSPS,&bit_depth_chroma_minus8,&bit_pos);
        pSPS += read_bit(pSPS,&qpprime_y_zero_transform_bypass_flag,&bit_pos);
        pSPS += read_bit(pSPS,&seq_scaling_matrix_present_flag,&bit_pos);
        if (seq_scaling_matrix_present_flag ){
            for (unsigned int i = 0; i<8; i++){
                pSPS += read_bit(pSPS,&seq_scaling_list_present_flag[i],&bit_pos);
                if (seq_scaling_list_present_flag[i]){
                    if (i < 6){
                        pSPS += parse_scaling_list(pSPS,16,&bit_pos);
                    }else{
                        pSPS += parse_scaling_list(pSPS,64,&bit_pos);
                    }
                }
            }
        }
    }
    pSPS += parse_ue(pSPS,&log2_max_frame_num_minus4,&bit_pos);
    pSPS += parse_ue(pSPS,&pic_order_cnt_type,&bit_pos);
    if (pic_order_cnt_type == 0){
        pSPS += parse_ue(pSPS,&log2_max_pic_order_cnt_lsb_minus4,&bit_pos);
    }else if (pic_order_cnt_type == 1){
        int delta_pic_order_always_zero_flag;
        pSPS += read_bit(pSPS, &delta_pic_order_always_zero_flag, &bit_pos);
        int offset_for_non_ref_pic;
        int offset_for_top_to_bottom_field;
        int num_ref_frames_in_pic_order_cnt_cycle;
        pSPS += parse_se(pSPS,&offset_for_non_ref_pic, &bit_pos);
        pSPS += parse_se(pSPS,&offset_for_top_to_bottom_field, &bit_pos);
        pSPS += parse_ue(pSPS,&num_ref_frames_in_pic_order_cnt_cycle, &bit_pos);
        int* offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
        for (int i =0;i < num_ref_frames_in_pic_order_cnt_cycle; i++ ){
            pSPS += parse_se(pSPS,offset_for_ref_frame+i, &bit_pos);
        }
        delete[] offset_for_ref_frame;
    }
    pSPS += parse_ue(pSPS,&num_ref_frames, &bit_pos);
    pSPS += read_bit(pSPS, &gaps_in_frame_num_value_allowed_flag, &bit_pos);
    pSPS += parse_ue(pSPS,&pic_width_in_mbs_minus1,&bit_pos);
    pSPS += parse_ue(pSPS,&pic_height_in_map_units_minus1, &bit_pos);

    pH264Info->width = (short)(pic_width_in_mbs_minus1 + 1) << 4;
    pH264Info->height = (short)(pic_height_in_map_units_minus1 + 1) <<4;

    //printf("pH264Info->width = %d\n", pH264Info->width);
    //printf("pH264Info->height = %d\n", pH264Info->height);

    pSPS += read_bit(pSPS, &frame_mbs_only_flag, &bit_pos);
    if (!frame_mbs_only_flag){
        int mb_adaptive_frame_field_flag;
        pSPS += read_bit(pSPS, &mb_adaptive_frame_field_flag, &bit_pos);
        pH264Info->height *= 2;
    }

    pSPS += read_bit(pSPS, &direct_8x8_inference_flag, &bit_pos);
    int frame_cropping_flag;
    int vui_parameters_present_flag;
    pSPS += read_bit(pSPS, &frame_cropping_flag, &bit_pos);
    if (frame_cropping_flag){
        int frame_crop_left_offset;
        int frame_crop_right_offset;
        int frame_crop_top_offset;
        int frame_crop_bottom_offset;
        pSPS += parse_ue(pSPS,&frame_crop_left_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_right_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_top_offset, &bit_pos);
        pSPS += parse_ue(pSPS,&frame_crop_bottom_offset, &bit_pos);
    }
    pSPS += read_bit(pSPS, &vui_parameters_present_flag, &bit_pos);

    pH264Info->time_base_num = 90000;
    pH264Info->time_base_den = 3003;
    if (vui_parameters_present_flag){
        //printf("Begin to set pH264Info->fps.\n");
        int aspect_ratio_info_present_flag;
        pSPS += read_bit(pSPS, &aspect_ratio_info_present_flag, &bit_pos);
        if (aspect_ratio_info_present_flag){
            int aspect_ratio_idc;
            pSPS += read_bit(pSPS, &aspect_ratio_idc,&bit_pos,8);
            if (aspect_ratio_idc == 255) // Extended_SAR
            {
                int sar_width;
                int sar_height;
                pSPS += read_bit(pSPS, &sar_width, &bit_pos, 16);
                pSPS += read_bit(pSPS, &sar_height, &bit_pos, 16);
            }
        }
        int overscan_info_present_flag;
        pSPS += read_bit(pSPS, &overscan_info_present_flag, &bit_pos);
        if (overscan_info_present_flag){
            int overscan_appropriate_flag;
            pSPS += read_bit(pSPS, &overscan_appropriate_flag, &bit_pos);
        }
        int video_signal_type_present_flag;
        pSPS += read_bit(pSPS, &video_signal_type_present_flag, &bit_pos);
        if (video_signal_type_present_flag){
            int video_format;
            pSPS += read_bit(pSPS, &video_format, &bit_pos,3);
            int video_full_range_flag;
            pSPS += read_bit(pSPS, &video_full_range_flag, &bit_pos);
            int colour_description_present_flag;
            pSPS += read_bit(pSPS, &colour_description_present_flag, &bit_pos);
            if (colour_description_present_flag){
                int colour_primaries, transfer_characteristics,matrix_coefficients;
                pSPS += read_bit(pSPS, &colour_primaries, &bit_pos, 8);
                pSPS += read_bit(pSPS, &transfer_characteristics, &bit_pos, 8);
                pSPS += read_bit(pSPS, &matrix_coefficients, &bit_pos, 8);
            }
        }

        int chroma_loc_info_present_flag;
        pSPS += read_bit(pSPS, &chroma_loc_info_present_flag, &bit_pos);
        if( chroma_loc_info_present_flag ){
            int chroma_sample_loc_type_top_field;
            int chroma_sample_loc_type_bottom_field;
            pSPS += parse_ue(pSPS,&chroma_sample_loc_type_top_field, &bit_pos);
            pSPS += parse_ue(pSPS,&chroma_sample_loc_type_bottom_field, &bit_pos);
        }
        int timing_info_present_flag;
        pSPS += read_bit(pSPS, &timing_info_present_flag, &bit_pos);
        if (timing_info_present_flag){
            int num_units_in_tick,time_scale;
            int fixed_frame_rate_flag;
            pSPS += read_bit(pSPS, &num_units_in_tick, &bit_pos, 32);
            pSPS += read_bit(pSPS, &time_scale, &bit_pos, 32);
            pSPS += read_bit(pSPS, &fixed_frame_rate_flag, &bit_pos);
            if (fixed_frame_rate_flag){
                unsigned char divisor; //when pic_struct_present_flag == 1 && pic_struct == 0
                //pH264Info->fps = (float)time_scale/num_units_in_tick/divisor;
                if (frame_mbs_only_flag) {
                    divisor = 2;
                } else {
                    divisor = 1; // interlaced
                }
                pH264Info->time_base_num = time_scale;
                pH264Info->time_base_den = divisor * num_units_in_tick;
                //printf("pH264Info->fps = [%d][%d]\n", pH264Info->time_base_num,pH264Info->time_base_den);
            }
        }
    }

    if ((pH264Info->width == 0) ||(pH264Info->height == 0) ||(pH264Info->time_base_num == 0) || (pH264Info->time_base_den == 0)) {
        return -1;
    }
    return 0;
};
