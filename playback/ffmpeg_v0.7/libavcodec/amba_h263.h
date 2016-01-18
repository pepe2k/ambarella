
#ifndef AVCODEC_AMBA_H263
#define AVCODEC_AMBA_H263
#include "mpegvideo.h"
#include "amba_dec_util.h"
#include "amba_dsp_define.h"

typedef struct h263_vld_data_s
{
//    int slice_cnt;
//    int* slice_offset; 
    uint8_t* pbuf;
    int size;

    int coded_width, coded_height;

    enum AVDiscard skip_frame;
    int hurry_up;
    int64_t pts;
}h263_vld_data_t;

typedef struct h263_pic_swinfo_s
{
    int block_last_index[6];//dct block info
    int qscale,chroma_qscale;//for deblock
}h263_pic_swinfo_t;

typedef struct h263_pic_data_s
{
    //mv
    void* pbase;
    mb_mv_B_t* pmvb;
    mb_mv_P_t* pmvp;
    unsigned char* pvopinfo;
        
    //idct coef/result
    //y 4x8x8,cb 8x8,cr 8x8 
    void* pdctbase;
    short* pdct;// 32 byte align

    //vop info
    udec_decode_t vopinfo;
        
    //deblock 
    
    //picture information
    int width, height;///< picture size. must be a multiple of 16
    int mb_width, mb_height;   ///< number of MBs horizontally & vertically
    int mb_stride;
    
    Picture *last_picture_ptr;     ///< pointer to the previous picture.
    Picture *next_picture_ptr;     ///< pointer to the next picture (for bidir pred)
    Picture *current_picture_ptr;  ///< pointer to the current picture

    //from s
    int f_code,b_code;
    int no_rounding;
    int progressive_sequence;
    int quarter_sample;
    int mpeg_quant;
    int alternate_scan;
    int h_edge_pos, v_edge_pos;///< horizontal / vertical position of the right/bottom edge (pixel replication)

    //sprite related
    int sprite_warping_accuracy;
    int vol_sprite_usage;
    int sprite_width;
    int sprite_height;
    int sprite_left;
    int sprite_top;
    int sprite_brightness_change;
    int num_sprite_warping_points;
    int real_sprite_warping_points;
    uint16_t sprite_traj[4][2];      ///< sprite trajectory points
    int sprite_offset[2][2];         ///< sprite offset[isChroma][isMVY]
    int sprite_delta[2][2];          ///< sprite_delta [isY][isMVY]
    int sprite_shift[2];             ///< sprite shift [isChroma]

    //sw decoding needed info
    h263_pic_swinfo_t* pswinfo; 

    //used for using dsp->not using dsp or vice vesa
    int use_dsp;//sw:0, amba Ione: 1, 
    int use_permutated;//use permutated matrix

    //debug use
    int frame_cnt;
}h263_pic_data_t;


typedef struct H263AmbaDecContext_s
{
    MpegEncContext s;

    //use dsp?
    int use_dsp;//sw:0, amba Ione: 1, 
    int use_permutated;//use permutated matrix, 0: 1: -1

    //amba dsp related
    amba_decoding_accelerator_t* pAcc;
    
    //parallel related
    
    //config
    int parallel_method;// 1: use way 1, 2: use way 2
//    int use_dsp;

    //thread control 
    pthread_t tid_vld,tid_mc_idct,tid_idct,tid_mc,tid_deblock;
    int vld_loop, mc_idct_loop, idct_loop, mc_loop, deblock_loop;
    
    //communicate queue
    ambadec_triqueue_t* p_vld_dataq;
    //way 1 vld->mc_idct
    ambadec_triqueue_t* p_mc_idct_dataq;
    //way 2 vld->idct->mc
    ambadec_triqueue_t* p_idct_dataq;
    ambadec_triqueue_t* p_mc_dataq;
    
    ambadec_triqueue_t* p_deblock_dataq;//optional
    ambadec_triqueue_t* p_frame_pool;//output pool

    //data queue
    ambadec_triqueue_t* p_pic_dataq;
    int pic_finished;//current picture vld decoding success, initial value 1; 
//    int vld_sent_row,mc_sent_row;
//    int previous_pic_is_b;//previous picture is BVOP(not reference), initial value 1;
    ambadec_pool_t* pic_pool;//parallel decoder used picture pool
    h263_pic_data_t  picdata[10];//extend to multiple pic data for dsp pipeline

    //store state and indicate next to do..
    int need_restart_pipeline; //should restart
    int pipe_line_started; //pipe line status

    h263_pic_data_t* p_pic;

    //pthread_mutex_t mutex;	//no use now
    //unsigned long  decoding_frame_cnt;//already in decoding frames //no use now

    //flush related
    int vld_need_flush;
    int idct_mc_need_flush;//parallel_method == 1
    int idct_need_flush;// parallel_method == 2
    int mc_need_flush; // parallel_method == 2
    int deblock_need_flush;
    int iswaiting4flush;
}H263AmbaDecContext_t;


void ff_h263_update_motion_val_amba_new(MpegEncContext * s,mb_mv_P_t* pmvp);
void ff_h263_loop_filter_amba(MpegEncContext * s, Picture* curpic, h263_pic_swinfo_t* pinfo, int mb_width, int mb_height);

int ff_h263_decode_mba(MpegEncContext *s);
void ff_h263_update_motion_val_amba(MpegEncContext * s);
int ff_mpeg4_get_video_packet_prefix_length(MpegEncContext *s);
int ff_h263_resync_amba(MpegEncContext *s);
void mpeg4_pred_ac_amba(MpegEncContext * s, DCTELEM *block, int n, int dir);
void ff_mpeg4_init_direct_mv(MpegEncContext *s);
int ff_mpeg4_decode_picture_header_amba(MpegEncContext * s, GetBitContext *gb);

void* thread_h263_idct_addresidue(void* p);
void* thread_h263_mc(void* p);
void* thread_h263_deblock(void* p);
void* thread_h263_vld(void* p);
void* thread_h263_mc_idct_addresidue(void* p);

int ff_intel_h263_decode_picture_header(MpegEncContext *s);
int ff_flv_decode_picture_header(MpegEncContext *s);
int h263_decode_picture_header(MpegEncContext *s);

av_cold int ff_h263_decode_end_amba(AVCodecContext *avctx);
void ff_er_frame_end_nv12(MpegEncContext *s);
av_cold int ff_h263_decode_init_amba(AVCodecContext *avctx);
void MPV_decode_mb_internal_amba(MpegEncContext *s);

void h263_mpeg4_p_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
void h263_mpeg4_b_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
void h263_mpeg4_i_process_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
int ff_h263_get_gob_height(MpegEncContext *s);
int ff_h263_decode_frame_amba(AVCodecContext *avctx,void *data, int *data_size,AVPacket *avpkt);
int ff_mpeg4_decode_partitions(MpegEncContext *s);

#endif

