#include "amba_dec_dsputil.h"

#ifndef __AMBA_DSP_DEFINE__
#define __AMBA_DSP_DEFINE__

#define VOP_CMD_DADDR 0x03c00000
#define VOP_MV_DADDR   0x03d00000
#define VOP_COEF_DADDR 0x04000000

//mpeg4/h263
//idct coef
typedef struct blk8x8_coef_s
{
    short coef[8][8];
}blk8x8_coef_t;

typedef struct mb_idctcoef_s
{
    blk8x8_coef_t y[4];
    blk8x8_coef_t cb;
    blk8x8_coef_t cr;
}mb_idctcoef_t;

#define forward_ref_num 0x0
#define backward_ref_num 0x1
#define not_valid_mv 0x6
#define not_valid_mv_value 0x60000000

//set mv[2] mv[3] as mv[0] mv[1] in field prediction
//#define __config_fill_field_mv23__
//fill not valid
//#define __config_fill_field_mv23_notvalid__

//mb mc setting
typedef struct mv_fmt_s
{
    int mv_x:14;
    int mv_y:13;
    int bottom:1;
    unsigned int ref_frame_num:3;//0:forward, 1:backward, 6: not valid mv
    unsigned int valid:1;//not used now
}mv_fmt_t;

//mv_type
#define mv_type_16x16 0
#define mv_type_16x8   1
#define mv_type_8x8     2
#define mv_type_16x16_gmc 3

typedef struct mb_setting_s
{
    unsigned int field_prediction:2;
    unsigned int reserved_1:14;
    unsigned int mv_type:5;
    unsigned int reserved_2:5;
    unsigned int dct_type:1;
    unsigned int intra:1;
    unsigned int reserved_3:1;
    unsigned int slice_start:1;
    unsigned int frame_left:1;
    unsigned int frame_top:1;
}mb_setting_t;

typedef struct mv_pair_s
{
    mv_fmt_t fw,bw;
}mv_pair_t;

typedef struct mb_mv_B_s
{
    mb_setting_t mb_setting;
    mv_pair_t      mvp[6];
}mb_mv_B_t;

typedef struct mb_mv_P_s
{
    mb_setting_t mb_setting;
    mv_fmt_t      mv[12];
}mb_mv_P_t;


//rv40 related
typedef struct RVDEC_MV_FMT
{
    int mv_x:14;
    int mv_y:12;
    unsigned int ref_idx:5;
    unsigned int valid:1;
}RVDEC_MV_FMT_t;

#define intratypemask 0xf
#define is16x16 0x4
#define usefw 0x1
#define usebw 0x2
#define mbisinter 0x3

#define usetopleft 0x20

typedef struct RVDEC_MV_B
{
    RVDEC_MV_FMT_t fwd;
    RVDEC_MV_FMT_t bwd;
}RVDEC_MV_B_t;

typedef struct RVDEC_MBMV_B
{
    RVDEC_MV_B_t mv[4];
}RVDEC_MBMV_B_t;

typedef struct RVDEC_MBMV_P
{
    RVDEC_MV_FMT_t fwd[8];
}RVDEC_MBMV_P_t;

typedef union RVDEC_MBINFO
{
    RVDEC_MBMV_P_t mv_p;
    RVDEC_MBMV_B_t mv_b;
}RVDEC_MBINFO_t;


//caution: must sync up with iav driver's header file

typedef struct udec_decode_s {
    unsigned char udec_type;
    unsigned char decoder_id;
    unsigned short num_pics;
    union {
        struct fifo {
            unsigned char *start_addr;
            unsigned char *end_addr;
        } fifo;

        struct mpeg4s_s { // hybrid MPEG4 - obsolete
            unsigned char *vop_coef_daddr;
            unsigned int vop_coef_size;

            unsigned char *mv_coef_daddr;
            unsigned int mv_coef_size;

            unsigned int vop_width;
            unsigned int vop_height;
            unsigned int vop_time_incre_res;
            unsigned int vop_pts_high;
            unsigned int vop_pts_low;
            unsigned char vop_code_type;
            unsigned char vop_chroma;
            unsigned char vop_round_type;
            unsigned char vop_interlaced;
            unsigned char vop_top_field_first;
            unsigned char end_of_sequence;
        } mpeg4s;

        struct mp4s2_s {// hybrid MPEG4
            unsigned char *vop_coef_start_addr;
            unsigned char *vop_coef_end_addr;
            unsigned char *vop_mv_start_addr;
            unsigned char *vop_mv_end_addr;
	} mp4s2;

        struct rv40_s { // RV40
            unsigned char *residual_daddr_start;
            unsigned char *residual_daddr_end;

            unsigned char *mv_daddr_start;
            unsigned char *mv_daddr_end;

            unsigned short fwd_ref_fb_id;
            unsigned short bwd_ref_fb_id;
            unsigned short target_fb_id;

            unsigned short pic_width;
            unsigned short pic_height;
            //unsigned int pts_hight;
            //unsigned int pts_low;
            unsigned char coding_type;
            unsigned char tiled_mode;

            unsigned char clean_fwd_ref_fb;	// clean cache for fwd_ref_fb_id
            unsigned char clean_bwd_ref_fb;	// clean cache for bwd_ref_fb_id
        } rv40;
    } uu;
} udec_decode_t;

#define mpeg4_hybird 4
#define rv40_hybird 6
/*
typedef struct iav_mpeg4_idctmc_decode_s
{
    unsigned int decode_id:         8;
    unsigned int decode_type:     8;//=4, mpeg4, dsp do idct+mc
    unsigned int num_of_pics:      16;
    unsigned int vop_coef_daddr;
    unsigned int vop_mv_daddr;
    unsigned short vop_width;
    unsigned short vop_height;
    unsigned int vop_time_increment_resolution;
    unsigned int vop_PTS_high;
    unsigned int vop_PTS_low;
    unsigned int vop_coding_type        :2; //I/P/B  0/1/2
    unsigned int vop_chroma_format   :2;
    unsigned int vop_rounding_type    :1;
    unsigned int vop_interlaced            :1 ;
    unsigned int vop_top_field_first     :1;
    unsigned int vop_reserved              :25 ;
} iav_mpeg4_idctmc_decode_t;

typedef struct iav_rv40_mc_decode_s
{
    unsigned int decode_id:         8;
    unsigned int decode_type:     8;//=6 rv40, dsp do mc
    unsigned int num_of_pics:      16;
    unsigned int vop_residual_daddr;
    unsigned int vop_mv_daddr;
    unsigned int fwd_ref_buffer_id;
    unsigned int bwd_ref_buffer_id;
    unsigned int target_buffer_id;
    unsigned short pic_width;
    unsigned short pic_height;
    unsigned int PTS_high;
    unsigned int PTS_low;
    unsigned int coding_type;
    unsigned int tiled_mode;
} iav_rv40_mc_decode_t;
*/

//vop info added at mv fifo chunk's head
typedef struct vop_info_s{
    unsigned int vop_width;
    unsigned int vop_height;
    unsigned int vop_time_incre_res;

    unsigned int vop_pts_high;
    unsigned int vop_pts_low;

    unsigned int vop_code_type : 2;
    unsigned int vop_chroma : 2;
    unsigned int vop_round_type : 1;
    unsigned int vop_interlaced : 1;
    unsigned int vop_top_field_first : 1;
    unsigned int end_of_sequence : 1;

    unsigned int reserved : 24;
    unsigned int reserved1;
    unsigned int reserved2; // aligned to 32 bytes
} vop_info_t;
#define CHUNKHEAD_VOPINFO_SIZE sizeof(vop_info_t)


struct amba_decoding_accelerator_s;
typedef void (*rv40_change_resolution_t)(struct amba_decoding_accelerator_s *pAcc);

typedef struct amba_decoding_accelerator_s
{
    int decode_id;
    int decode_type;

    int amba_iav_fd;
    int amba_buffer_number;
    int amba_picture_real_width;//may be not 16 byte align
    int amba_picture_real_height;//may be not 16 byte align
    int amba_picture_width;//must be 16 byte align, assert here
    int amba_picture_height;//must be 16 byte align, assert here
    unsigned char* p_amba_idct_ringbuffer;
    unsigned int amba_idct_buffer_size;
    unsigned char* p_amba_mv_ringbuffer;
    unsigned int amba_mv_buffer_size;

    unsigned int video_buffer_number;//attached video buffer pool, if zero, means ucode will manage it decoding buffer pool, mw should not call get_frame
    unsigned int has_video_output;//if has no out pic, will get only pts
    unsigned int video_ticks;//for bug917/921, hy pts issue

    //max to 4, need check
    unsigned char* p_mcresult_buffer[4];
    unsigned int mcresult_buffer_size[4];
    unsigned int  mcresult_buffer_id[4];
    unsigned int mcresult_buffer_real_id[4];

    //shared memory, for direct communication with dsp
    unsigned int* p_sync_counter;

    //for IAV_WAIT_OUTPIC debugging with ucode in ppmode=1
    unsigned int sent_decode_pic_cnt;
    unsigned int got_decoded_pic_cnt;
    //for different video render methord specified by ppmod
    unsigned int does_render_by_filter;
    //debug
    unsigned int req_cnt;
    unsigned int dec_cnt;

    //opaque
    void* p_opaque;
    struct rv40_neon_t rv40_accel;
    //added by liujian
    rv40_change_resolution_t rv40_change_resolution;
} amba_decoding_accelerator_t;



#endif

