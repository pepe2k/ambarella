#ifndef __LOG_DUMP__H__
#define __LOG_DUMP__H__

#include "config.h"

extern int log_cur_frame;
extern int log_start_frame;
extern int log_end_frame;
extern int log_mask[16];
extern int log_start_mb_x;
extern int log_start_mb_y;
extern int log_end_mb_x;
extern int log_end_mb_y;
extern int log_config_start_frame;

extern int dump_config_extedge;//dump with extended edge

#define ambadec_assert_ffmpeg(expr) \
    do{ \
        if (!(expr)) { \
		av_log(NULL,AV_LOG_ERROR,"assert failed: file %s, %d line. \n", __FILE__, __LINE__); \
        } \
    } while (0)
  
//#define ambadec_assert_ffmpeg(expr)			

//#define __log_decoding_config__
//config bit
#define decoding_config_idct_deq     0
#define decoding_config_add_idct     1
#define decoding_config_mc              2
#define decoding_config_intrapred    3
#define decoding_config_deblock      4

#define decoding_config_pre_interpret_gmc      5
#define decoding_config_use_dsp_permutated      6

#define decoding_sub_config_offset  7

//enable dump decoder input data
//#define __log_dump_decocer_input_data__

//enable dump frame data
//#define __log_dump_data__

//dump yuv420p
//#define __dump_yuv420p__
//#define __dump_yuv420_nv12__
//#define __dump_yuv420_mb__

int log_openfile(int index,char* str);
int log_openfile_with_num(int index,char* str,int num);
int log_closefile(int index);
int log_dump(int index,char* data,int len);
int log_dump_rect(int index,char* data,int width,int height,int stride);

//dump inverse transform result
//#define __log_dump_idct_data__
int log_openfile_text(int index,char* str);
int log_text(int index,char* str);
int log_idct_txt(int index,void* pdata,int size);
int log_idct_txt_uv(int index,void* pdata,int size);

//log intrapred
//#define __log_intrapred__

//log mc type
//#define __log_mc__

//log mc type
//#define __log_mv_new__

//log mv value
//#define __log_mv__

//check mb
//#define __dump_mb_data__

#define log_fd_frame_data   0
#define log_fd_idct_y   2
#define log_fd_idct_uv   3
#define log_fd_mb_data   4

#define log_fd_residual  5
#define log_fd_mvdsp  6
#define log_fd_reffyraw  7
#define log_fd_reffuvraw  8
#define log_fd_reffytiled  9
#define log_fd_reffuvtiled  10
#define log_fd_refbyraw  11
#define log_fd_refbuvraw  12
#define log_fd_refbytiled  13
#define log_fd_refbuvtiled  14
#define log_fd_result  15
#define log_fd_curyraw  16
#define log_fd_curuvraw  17
#define log_fd_picinfo  18
#define log_fd_mbtype  19

#define log_fd_intrapred      20
#define log_fd_mc      21
#define log_fd_mv      22
#define log_fd_idct_text   23
#define log_fd_picinfo_text   24
#define log_fd_dsp_text 25
#define log_fd_mv_new      26

#define log_fd_intrapred_binary      27
#define log_fd_chroma_mv_calculate      28
#define log_fd_chroma_mv_infact      29

#define log_fd_result_2  30
#define log_fd_result_3  31
#define log_fd_residual_2 32
#define log_fd_residual_3 38

#define log_fd_general_txt 33
#define log_fd_dsp_deblock_text 34
#define log_fd_dsp_mc_dct_text 35

//separate log file to parts
#define log_offset_vld 1
#define log_offset_dct 2
#define log_offset_mc 3

//dump pic info
//#define __log_picinfo__

//#define __dump_temp__
#define log_fd_temp   36
#define log_fd_temp_2   37

//DSP special case
//interpret gmc to normal mc, spirit vop to p vop
//#define __dsp_interpret_gmc__

//dump DSP test data
//#define __dump_DSP_test_data__
//#define __dump_binary__
//#define __dump_whole__
//#define __dump_DSP_result__
//#define __dump_DSP_result_with_display_order__
//#define __dump_DSP_TXT__
//#define __dump_deblock_DSP_TXT__
//#define __dump_mc_dct_DSP_TXT__
//separate txt logs
//#define __dump_separate__ 
//rv_residual_%d residual data, YUV format, tiled MB
//rv_mv_%d mv data,
//rv_reference_Y_raw_%d raw data for Y
//rv_reference_UV_raw_%d raw data for UV
//rv_reference_Y_tiled_%d raw data for Y
//rv_reference_UV_tiled_%d raw data for UV

//#define __dump_chroma_mv__

typedef enum log_mbtype_e
{
    intra=0,
    direct,
    skip16x16,
    skipgmc16x16,
    gmc16x16,
    field16x8,
    field16x8bw,
    field16x8bi,
    frame16x16,
    frame16x16bw,
    frame16x16bi,
    frame8x8,
    tot_mbtype_cnt,
}log_mbtype;

extern char log_mbtypestr[tot_mbtype_cnt][20];

//#define __debug_trap__
#ifdef __debug_trap__
#define debug_trap() _log_debug_trap()
#else
#define debug_trap()
#endif

//check bit-stream's specail case
#define __log_special_case__

//temp modification 
//use transposed permutation for amba dsp(mpeg4 IDCT+MC acceleration)
#define __tmp_use_amba_permutation__

//use neon accel in rv40 nv12 decoder
#if HAVE_NEON
#define _config_rv40_neon_
#endif

void set_decoding_config(int config,int start_frame);
void apply_decoding_config(void);
void set_dump_config(int start, int end, int start_x, int end_x, int start_y,int end_y, int dump_ext);
int log_openfile_p(int index,char* str,int fcnt);
int log_openfile_f(int index,char* str);
int log_openfile_f_p(int index,char* str,int fcnt);
int log_closefile_p(int index,int fcnt);
int log_dump_f(int index,char* data,int len);
int log_dump_p(int index,char* data,int len,int fcnt);
int log_openfile_text_p(int index,char* str,int fcnt);
int log_openfile_text_f(int index,char* str);
int log_text_f(int index,char* str);
int log_text_p(int index,char* str,int fcnt);
int log_sub_idct_txt(int index,void* pdata,int stride);
int log_idct_txt_uv_separated(int index,void* pdata);
int log_reference_txt(int index,unsigned char* pdata,int w,int h);
void log_text_rect_char(int index,char* data,int width,int height,int stride);
void log_text_rect_char_p(int index,char* data,int width,int height,int stride,int fcnt);
void log_text_rect_char_hex(int index,char* data,int width,int height,int stride);
void log_text_rect_char_hex_f(int index,char* data,int width,int height,int stride);
void log_text_rect_char_hex_p(int index,char* data,int width,int height,int stride,int fcnt);
void log_text_rect_char_chroma(int index,char* data,int width,int height,int stride);
void log_text_rect_char_chroma_p(int index,char* data,int width,int height,int stride,int fcnt);
void log_text_rect_char_hex_chroma(int index,char* data,int width,int height,int stride);
void log_text_rect_char_hex_chroma_p(int index,char* data,int width,int height,int stride,int fcnt);
void log_text_rect_short(int index,short* data,int width,int height,int stride);
void log_text_rect_short_p(int index,short* data,int width,int height,int stride,int fcnt);
void log_text_rect_short_hex(int index,short* data,int width,int height,int stride);
void log_text_rect_short_hex_p(int index,short* data,int width,int height,int stride,int fcnt);
void log_text_rect_short_chroma(int index,short* data,int width,int height,int stride);
void log_text_rect_short_chroma_p(int index,short* data,int width,int height,int stride,int fcnt);
void log_text_rect_short_hex_chroma(int index,short* data,int width,int height,int stride);
void log_text_rect_short_hex_chroma_p(int index,short* data,int width,int height,int stride,int fcnt);
void log_text_rect_int(int index,int* data,int width,int height,int stride);
void log_text_rect_int_p(int index,int* data,int width,int height,int stride,int fcnt);


//generic dump function
void dump_yuv_data(char* filename, void* p_pic, unsigned int width, unsigned int height, unsigned int informat, unsigned int outformat, unsigned int cnt);
void dump_yuv_data_mb(char* filename, void* p_pic, unsigned int width, unsigned int height, unsigned int informat, unsigned int cnt);

#endif
