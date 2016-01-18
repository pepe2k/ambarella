#ifndef __AMBA_DEC_DSPUTIL__
#define __AMBA_DEC_DSPUTIL__

typedef void (*transform_function_t)(short* ptmp);

typedef void (*mb_addresidue_function_t)(unsigned char* pdesy,unsigned char* pdesuv ,short* dct,int stride);
typedef void (*addresidue_function_t)(unsigned char* pdes ,short* psrc,int stride);

typedef void (*pred816_function_t)(unsigned char *src, int stride);
typedef void (*pred4_function_t)(unsigned char *src, unsigned char *topright, int stride);
typedef void (*new_mc_result_function_t)(unsigned char* des,unsigned char* des2,unsigned char* src,int stride,int width_x,int height_y,unsigned char* src_2);
typedef void(*rv40_loop_filter)(unsigned char *src, int stride, int* argu);

typedef enum rv40_trans_type_e
{
    trans_type_8x8=0,
    trans_type_16x16,
    trans_type_dc,
    trans_type_num
}rv40_trans_type_t;


typedef enum rv40_addresidue_type_e
{
//    addresidue_type_16x16=0,
    addresidue_type_4x4y=0,
    addresidue_type_4x4uv,
    addresidue_type_num
}rv40_addresidue_type_t;


typedef enum rv40_intrapred816_type_e
{
    intrapred_type_8x8_128dc=0,
    intrapred_type_8x8_vertical,
    intrapred_type_8x8_horizontal,
    intrapred_type_8x8_topdc,
    intrapred_type_8x8_128dc_nv12,
    intrapred_type_8x8_vertical_nv12,
    intrapred_type_8x8_horizontal_nv12,
    intrapred_type_8x8_topdc_nv12,
    intrapred_type_16x16_128dc,
    intrapred_type_16x16_vertical,
    intrapred_type_16x16_horizontal,
    intrapred_type_16x16_topdc,
    intrapred816_type_num
}rv40_intrapred816_type_t;


typedef enum rv40_intrapred4_type_e
{
    intrapred_type_4x4_128dc_nv12=0,
    intrapred_type_4x4_vertical_nv12,
    intrapred_type_4x4_horizontal_nv12,
    intrapred_type_4x4_topdc_nv12,
    intrapred4_type_num
}rv40_intrapred4_type_t;

typedef enum rv40_loop_filter_type_e
{
    v_loop_filter_y=0,
    h_loop_filter_y,
    v_loop_filter_uv,
    h_loop_filter_uv,
    loop_filter_type_num
}rv40_loop_filter_type_t;

typedef struct rv40_neon_t
{
    transform_function_t trans[trans_type_num];
    mb_addresidue_function_t mb_addresidue;
    addresidue_function_t addresidue[addresidue_type_num];
    new_mc_result_function_t rv40_new_result;
    pred816_function_t pred816[intrapred816_type_num];
    pred4_function_t pred4[intrapred4_type_num];
    rv40_loop_filter deblock[loop_filter_type_num];
}rv40_neon_t;

int rv40_replace_pred(rv40_neon_t* p_neon,void* h);

#endif
