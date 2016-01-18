
#ifndef AVCODEC_AMBA_RV12_H
#define AVCODEC_AMBA_RV12_H
#include "amba_dec_util.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"

#include "h264pred.h"
#include "log_dump.h"

//decoding config
//use dsp format
//old
//#define _d_use_DSP_format_


//new
#define _d_new_parallel_

//special settings: intra MB need set valid=1 for rv
#define _tmp_rv_dsp_setting_

typedef struct RVDEC_MV_FMT
{
    int32_t mv_x:14;
    int32_t mv_y:12;
    uint32_t ref_idx:5;
    uint32_t valid:1;
}RVDEC_MV_FMT_t;

#ifdef _tmp_rv_dsp_setting_
#define intratypemask 0xf
#define is16x16 0x4
#define usefw 0x1
#define usebw 0x2
#define mbisinter 0x3

#define usetopleft 0x20

//#define topleft_mask 0x2f 
#else
typedef struct RVDEC_INTRAPRED_FMT
{
    uint8_t intratype:4;
    uint8_t is16x16:1;
    uint8_t use_topleft:1;
    uint8_t reserved:2;
}RVDEC_INTRAPRED_FMT_t;
#define intratype_mask 0xf
#define is16x16_mask 0x10
#define use_topleft_mask 0x20
#endif


#ifdef _d_use_DSP_format_
typedef struct RVDEC_MB_MV_B
{
    RVDEC_MV_FMT_t fwd0;
    RVDEC_MV_FMT_t bwd0;
    RVDEC_MV_FMT_t fwd1;
    RVDEC_MV_FMT_t bwd1;
    RVDEC_MV_FMT_t fwd2;
    RVDEC_MV_FMT_t bwd2;
    RVDEC_MV_FMT_t fwd3;
    RVDEC_MV_FMT_t bwd3;    
}RVDEC_MB_MV_B_t;

typedef struct RVDEC_MB_MV_P
{
    RVDEC_MV_FMT_t fwd0;
    RVDEC_MV_FMT_t fwd1;
    RVDEC_MV_FMT_t fwd2;
    RVDEC_MV_FMT_t fwd3;
    uint32_t reserved[4];
}RVDEC_MB_MV_P_t;

typedef struct RVDEC_PIC_MV
{
    void* pbase;
    RVDEC_MB_MV_P_t* p;
    RVDEC_MB_MV_B_t* b;
}RVDEC_PIC_MV_t;

#endif

typedef struct RVDEC_PIC_INFO
{
    uint32_t residual_daddr;
    uint32_t mv_daddr;
    uint32_t ref_y_daddr;
    uint32_t ref_uv_daddr;
    uint32_t pic_width;
    uint32_t pic_height;
    uint32_t pts_high;
    uint32_t pts_low;
    uint32_t pic_coding_type;
    uint32_t mb_num;
    uint32_t reserved[22];
}RVDEC_PIC_INFO_t;

#ifdef _d_new_parallel_
typedef struct RVDEC_MV_B
{
    RVDEC_MV_FMT_t fwd;
    RVDEC_MV_FMT_t bwd;
}RVDEC_MV_B_t;

typedef struct RVDEC_MBMV_B
{
    RVDEC_MV_B_t mv[4];
}RVDEC_MBMV_B_t;

#ifdef _tmp_rv_dsp_setting_
typedef struct RVDEC_INTRAPRED
{
    uint8_t intratype[20];
}RVDEC_INTRAPRED_t;
#else
typedef struct RVDEC_INTRAPRED
{
    RVDEC_INTRAPRED_FMT_t intratype[32];
}RVDEC_INTRAPRED_t;
#endif

typedef struct RVDEC_MBMV_P
{
    RVDEC_MV_FMT_t fwd[8];
}RVDEC_MBMV_P_t;

//these three struct should have same size
//DSP only use MC related
//RVDEC_MV_FMT_t::valid should not be used for RVDEC_INTRAPRED_t
#ifdef _tmp_rv_dsp_setting_
typedef union RVDEC_MBINFO
{
    RVDEC_MBMV_P_t mv_p;
    RVDEC_MBMV_B_t mv_b;
}RVDEC_MBINFO_t;
#else
typedef union RVDEC_MBINFO
{
    RVDEC_MBMV_P_t mv_p;
    RVDEC_MBMV_B_t mv_b;
    RVDEC_INTRAPRED_t intra;
}RVDEC_MBINFO_t;
#endif

//for picture pipe line decoding, there would be multipile picture decoding at the same time,
//so move data to picture from dec context
typedef struct rv40_pic_data
{
    void* pbase;
    
    #ifdef _tmp_rv_dsp_setting_
//    void* pintrabase;//intra pred
    int* pmbtype;// record the MB types(inter or intra) for whole picture
    #endif
    
    //mc intrapred
    RVDEC_MBINFO_t* pinfo;//mc
    RVDEC_INTRAPRED_t* pintra;//intrapred
    
    //inverse transform residue
    //Y16x16 U8x8 V8x8
    void* pdctbase;
    DCTELEM* pdct;
//    DCTELEM* pdctu,*pdctv;

    RVDEC_PIC_INFO_t picinfo;
        
    //deblock 
    uint16_t *cbp_luma;      ///< CBP values for luma subblocks
    uint8_t  *cbp_chroma;    ///< CBP values for chroma subblocks
    int      *deblock_coefs; ///< deblock coefficients for each macroblock

    int width, height;///< picture size. must be a multiple of 16
    int mb_width, mb_height;   ///< number of MBs horizontally & vertically
    int mb_stride;             ///< mb_width+1 used for some arrays to allow simple addressing of left & top MBs without sig11
    
    //picture information
    Picture *last_picture_ptr;     ///< pointer to the previous picture.
    Picture *next_picture_ptr;     ///< pointer to the next picture (for bidir pred)
    Picture *current_picture_ptr;  ///< pointer to the current picture

//debug use
    int frame_cnt;

}rv40_pic_data_t;

typedef struct vld_data_s
{
//    AVCodecContext *avctx;
//    AVPacket *avpkt;
    int slice_cnt;
    int* slice_offset; 
    uint8_t* pbuf;
    int size;

    enum AVDiscard skip_frame;
    int hurry_up;
}vld_data_t;

typedef struct process_data_s
{
    rv40_pic_data_t* p_picdata;
    int start_row;//>=
    int end_row;//<
}process_data_t;

#endif

typedef struct RVDEC_MB_Y_REF
{
    uint8_t y[16][16];
}RVDEC_MB_Y_REF_t;

typedef struct RVDEC_MB_UV_REF
{
    uint8_t uv[8][16];
}RVDEC_MB_UV_REF_t;

typedef struct RVDEC_MB_RESULT
{
    uint8_t y[16][16];
    uint8_t u[8][8];
    uint8_t v[8][8];
}RVDEC_MB_RESULT_t;

typedef struct RVDEC_MB_RESIDUAL
{
    int16_t y[16][16];
    int16_t u[8][8];
    int16_t v[8][8];
}RVDEC_MB_RESIDUAL_t;


#define MB_TYPE_SEPARATE_DC 0x01000000
#define IS_SEPARATE_DC(a)   ((a) & MB_TYPE_SEPARATE_DC)

/**
 * RV30 and RV40 Macroblock types
 */
enum RV40BlockTypes{
    RV34_MB_TYPE_INTRA,      ///< Intra macroblock
    RV34_MB_TYPE_INTRA16x16, ///< Intra macroblock with DCs in a separate 4x4 block
    RV34_MB_P_16x16,         ///< P-frame macroblock, one motion frame
    RV34_MB_P_8x8,           ///< P-frame macroblock, 8x8 motion compensation partitions
    RV34_MB_B_FORWARD,       ///< B-frame macroblock, forward prediction
    RV34_MB_B_BACKWARD,      ///< B-frame macroblock, backward prediction
    RV34_MB_SKIP,            ///< Skipped block
    RV34_MB_B_DIRECT,        ///< Bidirectionally predicted B-frame macroblock, no motion vectors
    RV34_MB_P_16x8,          ///< P-frame macroblock, 16x8 motion compensation partitions
    RV34_MB_P_8x16,          ///< P-frame macroblock, 8x16 motion compensation partitions
    RV34_MB_B_BIDIR,         ///< Bidirectionally predicted B-frame macroblock, two motion vectors
    RV34_MB_P_MIX16x16,      ///< P-frame macroblock with DCs in a separate 4x4 block, one motion vector
    RV34_MB_TYPES
};

/**
 * VLC tables used by the decoder
 *
 * Intra frame VLC sets do not contain some of those tables.
 */
typedef struct RV34VLC{
    VLC cbppattern[2];     ///< VLCs used for pattern of coded block patterns decoding
    VLC cbp[2][4];         ///< VLCs used for coded block patterns decoding
    VLC first_pattern[4];  ///< VLCs used for decoding coefficients in the first subblock
    VLC second_pattern[2]; ///< VLCs used for decoding coefficients in the subblocks 2 and 3
    VLC third_pattern[2];  ///< VLCs used for decoding coefficients in the last subblock
    VLC coefficient;       ///< VLCs used for decoding big coefficients
}RV34VLC;

/** essential slice information */
typedef struct SliceInfo{
    int type;              ///< slice type (intra, inter)
    int quant;             ///< quantizer used for this slice
    int vlc_set;           ///< VLCs used for this slice
    int start, end;        ///< start and end macroblocks of the slice
    int width;             ///< coded width
    int height;            ///< coded height
    int pts;               ///< frame timestamp
}SliceInfo;


#ifndef _d_new_parallel_

//intrapredmc_type in MBInfoMCIntraPred

//mc or intrapred
#define useIntrapred 0x01
#define useMC 0x02
//mc :0

//intrapred type
#define IntraPred16x16 0x04
// 4x4: 0

//MC tppe
#define MC4MV 0x04
// 1mv: 0

#define MCForward 0x10
#define MCBackward 0x20
#define MCBidirectional 0x30

//addtional informaition

typedef struct sMBInfoMC
{
    int16_t mvx[8];
    int16_t mvy[8];
}MBInfoMC;

typedef struct sMBInfoIntraPred
{   
    int8_t intratype[32];
}MBInfoIntraPred;

typedef struct uMBInfoMCIntraPred
{
    union
    {
        MBInfoMC mc;
        MBInfoIntraPred intrapred;
    }info;
    long intrapredmc_type;   
}MBInfoMCIntraPred;

typedef struct sMBLineMCIntraPred
{
    MBInfoMCIntraPred* pMBInfo;
    DCTELEM* p_idct_result_base, *p_idct_result_y, *p_idct_result_uv;
    int mbwidth,mbheight;
    int mbrow,stride;
}MBLineMCIntraPred;


typedef struct sMBLineDeblockInfo
{
    int mbrow;
    
    //luma_cbp, chroma_cbp,deblock_coefs is in rv34 context
    //current picture in s    
}MBLineDeblock;
#endif

/** decoder context */
typedef struct RV34AmbaDecContext{
    MpegEncContext s;
    int8_t *intra_types_hist;///< old block types, used for prediction
    int8_t *intra_types;     ///< block types
    int    intra_types_stride;///< block types array stride
    const uint8_t *luma_dc_quant_i;///< luma subblock DC quantizer for intraframes
    const uint8_t *luma_dc_quant_p;///< luma subblock DC quantizer for interframes

    RV34VLC *cur_vlcs;       ///< VLC set used for current frame decoding
    int bits;                ///< slice size in bits
    H264PredContext h;       ///< functions for 4x4 and 16x16 intra block prediction
    SliceInfo si;            ///< current slice information

    int *mb_type;            ///< internal macroblock types
    int block_type;          ///< current block type
    int luma_vlc;            ///< which VLC set will be used for decoding of luma blocks
    int chroma_vlc;          ///< which VLC set will be used for decoding of chroma blocks
    int is16;                ///< current block has additional 16x16 specific features or not
    int dmv[4][2];           ///< differential motion vectors for the current macroblock

    int rv30;                ///< indicates which RV variasnt is currently decoded
    int rpr;                 ///< one field size in RV30 slice header

    int cur_pts, last_pts, next_pts;

#ifndef _d_new_parallel_
    uint16_t *cbp_luma;      ///< CBP values for luma subblocks
    uint8_t  *cbp_chroma;    ///< CBP values for chroma subblocks
    int      *deblock_coefs; ///< deblock coefficients for each macroblock
#endif

    /** 8x8 block available flags (for MV prediction) */
    DECLARE_ALIGNED(8, uint32_t, avail_cache[3*4]);

    int (*parse_slice_header)(struct RV34AmbaDecContext *r, GetBitContext *gb, SliceInfo *si);
    int (*decode_mb_info)(struct RV34AmbaDecContext *r);
    int (*decode_intra_types)(struct RV34AmbaDecContext *r, GetBitContext *gb, int8_t *dst);
#ifdef _d_new_parallel_
    void (*loop_filter)(struct RV34AmbaDecContext *r,rv40_pic_data_t* pic_data,int row);
#else
    void (*loop_filter)(struct RV34AmbaDecContext *r, int row);
#endif

    //parellel related
    
    //thread control 
    pthread_t tid_vld,tid_mc_intrapred,tid_deblock;

    int vld_loop;
    int mc_intrapred_loop;
    int deblock_loop;
    
#ifdef _d_new_parallel_
    //communicate queue
    ambadec_triqueue_t* p_vld_dataq;
    ambadec_triqueue_t* p_mcintrapred_dataq;
    ambadec_triqueue_t* p_deblock_dataq;
    ambadec_triqueue_t* p_frame_pool;//output pool

    //data queue
    ambadec_triqueue_t* p_pic_dataq;
    int pic_finished;//current picture vld decoding success, initial value 1; 
    int vld_sent_row,mc_sent_row;
    int previous_pic_is_b;//previous picture is BVOP(not reference), initial value 1;
    ambadec_pool_t* pic_pool;//parallel decoder used picture pool
    rv40_pic_data_t  picdata[2];

    rv40_pic_data_t* p_pic;

    pthread_mutex_t mutex;
    unsigned long  decoding_frame_cnt;//already in decoding frames
#else
//    ambadec_queue_t* p_thread_exit;

    ambadec_queue_t* p_mc_intrapred_queue;
    ambadec_queue_t* p_mc_intrapred_free_queue;
    MBLineMCIntraPred* p_mc_intrapred;//main thread use
    MBInfoMCIntraPred* p_mbinfo;//Mbinfo
//    DCTELEM* p_y,*p_uv;
    
    ambadec_queue_t* p_deblock_queue;
    ambadec_queue_t* p_deblock_free_queue;
//    MBLineDeblock* p_deblock;
    ambadec_queue_t* p_frame_done;

    #ifdef _d_use_DSP_format_
    RVDEC_PIC_MV_t pdspmv;
    RVDEC_MB_RESIDUAL_t residual;
    RVDEC_MB_Y_REF_t fy,by;
    RVDEC_MB_UV_REF_t fuv,buv;
    RVDEC_MB_RESULT_t result;
    RVDEC_PIC_INFO_t picinfo;//for dump
    #endif
#endif

//    int deblock_wait;
//    int intrapred_mc_wait;
}RV34AmbaDecContext;

/**
 * common decoding functions
 */
int ff_rv34_amba_get_start_offset(GetBitContext *gb, int blocks);
#ifdef _d_new_parallel_
int ffnew_rv34_amba_decode_end(AVCodecContext *avctx);
int ffnew_rv34_amba_decode_frame(AVCodecContext *avctx, void *data, int *data_size, AVPacket *avpkt);
#else
int ff_rv34_amba_decode_end(AVCodecContext *avctx);
int ff_rv34_amba_decode_frame(AVCodecContext *avctx, void *data, int *data_size, AVPacket *avpkt);
#endif
//int ff_rv34_amba_decode_end(AVCodecContext *avctx);

#if 0
MBLineMCIntraPred* new_mc_intrapred(int mbwidth, int mbheight);
void delete_mc_intrapred(MBLineMCIntraPred* p_mc_intrapred);
#endif

#endif /* AVCODEC_RV34_H */

