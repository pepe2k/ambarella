#include "avcodec_export.h"
#include "h264.h"
#include "vc1.h"
#include "mpegvideo.h"

// this function may be exposed someday.
static int getSeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader);

int InitSeqHeader(enum CodecID codecID, SeqHeader **ppSeqHeader)
{
    int size = 0;
    switch (codecID) {
        case CODEC_ID_H264:
            size = sizeof(H264SeqHeader);
            break;
        case CODEC_ID_VC1:
        case CODEC_ID_WMV3:
            size = sizeof(VC1SeqHeader);
            break;
        case CODEC_ID_MPEG4:
            size = sizeof(MPEG4SeqHeader);
            break;
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            size = sizeof(MPEG12SeqHeader);
            break;
        default:{
            av_log(NULL, AV_LOG_ERROR, "InitSeqHeader failure - codecID:%d\n", codecID);
            return -1;
            }
    }

    *ppSeqHeader = (SeqHeader*) malloc(size);
    if (*ppSeqHeader == NULL) {
        av_log(NULL, AV_LOG_ERROR, "InitSeqHeader failure - Insufficient memory\n");
        return -1;
    }

    // init
    (*ppSeqHeader)->codecID = codecID;
    (*ppSeqHeader)->framerate.den = 0;
    (*ppSeqHeader)->framerate.num = 0;
    (*ppSeqHeader)->width = 0;
    (*ppSeqHeader)->height = 0;
    (*ppSeqHeader)->pix_fmt = PIX_FMT_NONE;
    (*ppSeqHeader)->is_interlaced = 0;
    (*ppSeqHeader)->interlaced_mode = INTERLACED_MODE_INVALID;

    switch (codecID) {
        case CODEC_ID_H264: {
            H264SeqHeader *pSeqHeader = (H264SeqHeader*)*ppSeqHeader;
            pSeqHeader->bit_depth_luma = 0;
            pSeqHeader->bit_depth_chroma = 0;
            }
            break;
        case CODEC_ID_VC1:
        case CODEC_ID_WMV3: {
            VC1SeqHeader *pSeqHeader = (VC1SeqHeader*)*ppSeqHeader;
            pSeqHeader->profile = 0;
            pSeqHeader->broadcast = 0;
            pSeqHeader->interlace = 0;
            pSeqHeader->res_y411 = 0;
            pSeqHeader->res_sprite = 0;
            pSeqHeader->res_rtm_flag = 1;
            }
            break;
        case CODEC_ID_MPEG4: {
            MPEG4SeqHeader *pSeqHeader = (MPEG4SeqHeader*)*ppSeqHeader;
            pSeqHeader->num_sprite_warping_points = 0;
            pSeqHeader->real_sprite_warping_points = 0;
            pSeqHeader->scalability = 0;
            pSeqHeader->divx_packed = 0;
            }
            break;
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO: {
//            MPEG12SeqHeader *pSeqHeader = (MPEG12SeqHeader*)*ppSeqHeader;
            }
            break;
        default:
            assert(0);
    }
    return 0;
}

void DeinitSeqHeader(SeqHeader **ppSeqHeader)
{
  if (*ppSeqHeader) {
    free(*ppSeqHeader);
    *ppSeqHeader = NULL;
  }
}

int GetSeqHeader(AVStream *stream, SeqHeader *pSeqHeader)
{
    int ret = -1;
    //av_log(NULL, AV_LOG_ERROR, "~~~~~~ParseExtradata begin~~~~~~\n");

    AVCodecParserContext *s = av_parser_init(stream->codec->codec_id);
    if (!s) {
        av_log(NULL, AV_LOG_ERROR, "Init parser failure - codec_id:%d\n", stream->codec->codec_id);
        return -1;
    }
    uint8_t *pOutBuf = NULL;
    int iOutButSize = 0;
    av_parser_parse2(s, stream->codec, &pOutBuf, &iOutButSize, NULL, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

    getSeqHeader(s, stream->codec, pSeqHeader);

    av_parser_close(s);

    //av_log(NULL, AV_LOG_ERROR, "~~~~~~ParseExtradata end~~~~~~\n");
    return ret;
}

#if TARGET_USE_AMBARELLA_S2_DSP
#define MAX_WIDTH_SUPPORTED     4096
#define MAX_HEIGHT_SUPPORTED    2160
#else
#define MAX_WIDTH_SUPPORTED     1920
#define MAX_HEIGHT_SUPPORTED    1088
#endif

enum SEQ_HEADER_RET CheckSeqHeader(const SeqHeader *pSeqHeader)
{
    // check pixel format
    switch (pSeqHeader->pix_fmt) {
        case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:
            break;
        default:
            return SEQ_HEADER_RET_DSP_UNSUPPORTED;
    }

    // check resolution
    int max_width = (pSeqHeader->height > pSeqHeader->width) ? pSeqHeader->height : pSeqHeader->width;
    int max_height = (pSeqHeader->height > pSeqHeader->width) ? pSeqHeader->width : pSeqHeader->height;
    if ((max_width > MAX_WIDTH_SUPPORTED || max_height > MAX_HEIGHT_SUPPORTED)) {
        av_log(NULL, AV_LOG_ERROR, "common: unsupported height=[%d] width=[%d]\n", pSeqHeader->height, pSeqHeader->width);
        return SEQ_HEADER_RET_PLAYER_UNSUPPORTED;
    }

    // check specific codec info
    enum SEQ_HEADER_RET ret = SEQ_HEADER_RET_OK;
    switch (pSeqHeader->codecID) {
        case CODEC_ID_H264: {
                H264SeqHeader *pH264SeqHeader = (H264SeqHeader*)pSeqHeader;
                if (pH264SeqHeader->bit_depth_luma > 8) {
                    av_log(NULL, AV_LOG_ERROR, "h264: unsupported bit_depth_luma=[%d]\n", pH264SeqHeader->bit_depth_luma);
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }
            }
            break;
        case CODEC_ID_VC1:
        case CODEC_ID_WMV3: {
                VC1SeqHeader *pVC1SeqHeader = (VC1SeqHeader*)pSeqHeader;

                if(pVC1SeqHeader->res_y411 == 1){
                    av_log(NULL, AV_LOG_ERROR,"VC1: res_y411 is not supported.\n");
                    ret = SEQ_HEADER_RET_PLAYER_UNSUPPORTED;
                    break;
                }
                if(pVC1SeqHeader->res_sprite == 1){
                    av_log(NULL, AV_LOG_ERROR,"VC1: MVP2 is not supported.\n");
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }
                //VC1 complex profile is not supported
                if(pVC1SeqHeader->profile == 2){//PROFILE_COMPLEX
                    av_log(NULL, AV_LOG_ERROR,"VC1: complex profile is not supported by DSP, use software decoder!\n");
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }
                if(!pVC1SeqHeader->res_rtm_flag){
                    av_log(NULL, AV_LOG_ERROR,"VC1:  Old WMV9 is not supported by DSP, use software decoder!.\n");
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }
                if(pSeqHeader->codecID == CODEC_ID_VC1){//CODEC_ID_VC1
                    if(pVC1SeqHeader->profile != 3){
                        av_log(NULL, AV_LOG_ERROR,"VC1: has a wrong profile, ignore video! profile = %d.\n", pVC1SeqHeader->profile);
                        ret = SEQ_HEADER_RET_PLAYER_UNSUPPORTED;
                        break;
                    }
                }
                if(pSeqHeader->codecID == CODEC_ID_WMV3){//CODEC_ID_WMV3
                    if(pVC1SeqHeader->profile != 0 && pVC1SeqHeader->profile != 1){
                        av_log(NULL, AV_LOG_ERROR,"VC1: VC1(WMV3) has a wrong profile, ignore video! profile = %d.\n",  pVC1SeqHeader->profile);
                        ret = SEQ_HEADER_RET_PLAYER_UNSUPPORTED;
                        break;
                    }
                }
                //The minimum picture dimension DSP support is 64x48 for vc1.
                if(pVC1SeqHeader->avctx->height < 48 ||
                   pVC1SeqHeader->avctx->width < 64){
                    av_log(NULL, AV_LOG_ERROR,"VC1: video[VC1] size[%dx%d] is too small, DSP not supported, use software decoder.\n",
                       pVC1SeqHeader->avctx->width, pVC1SeqHeader->avctx->height);
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }
            }
            break;
        case CODEC_ID_MPEG4: {
                MPEG4SeqHeader *pMPEG4SeqHeader = (MPEG4SeqHeader*)pSeqHeader;
//                uint8_t* StartCode_ptr = NULL;
                if (pMPEG4SeqHeader->scalability) {
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    av_log(NULL, AV_LOG_ERROR, "mpeg4: unsupported scalability=[%d]\n", pMPEG4SeqHeader->scalability);
                    break;
                }

                if (pMPEG4SeqHeader->num_sprite_warping_points > 1) {

                    av_log(NULL, AV_LOG_ERROR, "mpeg4: hw unsupported num_sprite_warping_points=[%d]\n", pMPEG4SeqHeader->num_sprite_warping_points);

                    if (0 != pMPEG4SeqHeader->quarter_sample)//for bug917/921 hy issue
                    {
                        av_log(NULL, AV_LOG_ERROR, "mpeg4: hy unsupported quarter_sample=[%d]\n", pMPEG4SeqHeader->quarter_sample);//quarter sample DSP not support in hy
                        ret = SEQ_HEADER_RET_HYBRID_UNSUPPORTED;
                        break;
                    }

                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    break;
                }

                if(pMPEG4SeqHeader->avctx->codec_tag == AV_RL32("SMP4")){
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    av_log(NULL, AV_LOG_ERROR, "mpeg4: unsupported FourCC SMP4.\n");
                    break;
                }
                if(pMPEG4SeqHeader->avctx->codec_tag == AV_RL32("SEDG")){
                    ret = SEQ_HEADER_RET_DSP_UNSUPPORTED;
                    av_log(NULL, AV_LOG_ERROR, "mpeg4: unsupported FourCC SEDG.\n");
                    break;
                }
            }
            break;
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO: {
//                MPEG12SeqHeader *pMPEG12SeqHeader = (MPEG12SeqHeader*)pSeqHeader;
            }
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "Unsupported codec: id=%d\n", pSeqHeader->codecID);
            ret = SEQ_HEADER_RET_INVALID;
    }
    return ret;
}

static int getH264SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader);
static int getVC1SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader);
static int getMPEG12SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader);
static int getMPEG4SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader);
static int cfgInterlacedParameters(AVCodecContext *avctx, SeqHeader *pSeqHeader);

int getSeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    int ret = -1;
    assert(avctx);


    pSeqHeader->height = avctx->height;
    pSeqHeader->width = avctx->width;
    pSeqHeader->pix_fmt = avctx->pix_fmt;

    switch (avctx->codec_id) {
        case CODEC_ID_H264:
            ret = getH264SeqHeader(s, avctx, pSeqHeader);
            break;
        case CODEC_ID_VC1:
        case CODEC_ID_WMV3:
            ret = getVC1SeqHeader(s, avctx, pSeqHeader);
            break;
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            ret = getMPEG12SeqHeader(s, avctx, pSeqHeader);
            break;
        case CODEC_ID_MPEG4:
            ret = getMPEG4SeqHeader(s, avctx, pSeqHeader);
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "Not supported codec_id:%d\n", avctx->codec_id);
    }

    av_log(NULL, AV_LOG_ERROR, "framerate: den=[%d] num=[%d] height=[%d] width=[%d]\n",
        pSeqHeader->framerate.den, pSeqHeader->framerate.num,
        pSeqHeader->height, pSeqHeader->width);
    return ret;
}

int getH264SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    H264Context *h = (H264Context*)s->priv_data;
    assert(h);

    H264SeqHeader *pH264SeqHeader = (H264SeqHeader*)pSeqHeader;
    assert(pH264SeqHeader);

    /*
     * For h264, it can be extracted SPS & PPS, sometimes SEI from extradata
     * we can access these info by h->sps, h->pps, h->sei_xxx
    */
    if (h->sps.vui_parameters_present_flag) {
        if (h->sps.timing_info_present_flag) {
            pSeqHeader->framerate.den = (int)h->sps.num_units_in_tick * 2;
            pSeqHeader->framerate.num = (int)h->sps.time_scale;
        }
    }

    pH264SeqHeader->bit_depth_luma = h->sps.bit_depth_luma;
    pH264SeqHeader->bit_depth_chroma = h->sps.bit_depth_chroma;

    if(avctx->h264_interlaced) {
        cfgInterlacedParameters(avctx, pSeqHeader);
    }

    return 0;
}

typedef struct {
    ParseContext pc;
    VC1Context v;
} VC1ParseContext;

int getVC1SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    VC1ParseContext *vpc = (VC1ParseContext*)s->priv_data;
    VC1Context *v = &vpc->v;
    assert(avctx);

    VC1SeqHeader *pVC1SeqHeader = (VC1SeqHeader*)pSeqHeader;
    assert(pVC1SeqHeader);

    //av_log(NULL, AV_LOG_ERROR, "profile=%d\n", v->profile);

    pVC1SeqHeader->profile = v->profile;

    /*
     * For vc1, it can be extracted lots of info, such as profile, framerate from extradata
     * we can access these info by vc1->v
    */

    if (v->profile == PROFILE_ADVANCED) {   // Advanced Profile
        pSeqHeader->framerate.den = avctx->time_base.num;
        pSeqHeader->framerate.num = avctx->time_base.den;

        pVC1SeqHeader->broadcast = v->broadcast;
        pVC1SeqHeader->interlace = v->interlace;

        if (avctx->vc1_interlaced) {
            cfgInterlacedParameters(avctx, pSeqHeader);
        }

        /*
        if (v->broadcast) {
            pSeqHeader->framerate.den *= 30;
            pSeqHeader->framerate.num *= 24;
            if (pVC1SeqHeader->interlace) {
                pSeqHeader->framerate.den *= 2;
            }
        }
        */
    } else {                                // Simple/Main Profile
        pVC1SeqHeader->res_y411 = v->res_y411;
        pVC1SeqHeader->res_rtm_flag = v->res_rtm_flag; //old WMV9

        if (v->res_y411) {
            av_log(NULL, AV_LOG_ERROR, "getVC1SeqHeader: Old interlaced mode is not supported\n");
        }

        if (v->frmrtq_postproc == 7) {
            pSeqHeader->framerate.den = 1;
            pSeqHeader->framerate.num = 30;
        } else {
            pSeqHeader->framerate.den = 1;
            pSeqHeader->framerate.num = (2 + v->frmrtq_postproc * 4);
        }
    }
    pVC1SeqHeader->res_sprite = v->res_sprite;//for simple/main/advanced profile,mdf for bug737, roy 2011.12.15.
    pVC1SeqHeader->avctx = avctx;
    return 0;
}

int getMPEG12SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    ParseContext1 *pc = (ParseContext1*)s->priv_data;
    assert(pc);

/*    MPEG12SeqHeader *pMPEG2SeqHeader = (MPEG12SeqHeader*)pSeqHeader;
    assert(pMPEG2SeqHeader);*/

    /*
     * For mpeg12 video, it can be extracted lots of info, such bitrate, framerate from extradata
     * we can access these info by pc & avctx
    */
    pSeqHeader->framerate.den = avctx->time_base.num;
    pSeqHeader->framerate.num = avctx->time_base.den;

    // MPEG-1 was designed to code progressively scanned video at bit rates up to about 1.5 Mbit/s for applications
    //av_log(NULL, AV_LOG_ERROR, "getMPEG12SeqHeader: progressive_sequence=%d enc=%p\n", pc->progressive_sequence, pc->enc);
    if (avctx->codec_id == CODEC_ID_MPEG2VIDEO && !pc->progressive_sequence) {
        cfgInterlacedParameters(avctx, pSeqHeader);
    }

    return 0;
}

int getMPEG4SeqHeader(AVCodecParserContext *s, AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    ParseContext1 *pc = (ParseContext1*)s->priv_data;
    MpegEncContext *enc = pc->enc;
    assert(enc);

    MPEG4SeqHeader *pMPEG4SeqHeader = (MPEG4SeqHeader*)pSeqHeader;
    assert(pMPEG4SeqHeader);

    /*
     * For mpeg4, it can be extracted vop from extradata
     * we can access these info by pc
    */
    pSeqHeader->framerate.den = avctx->time_base.num;
    pSeqHeader->framerate.num = avctx->time_base.den;

    if(avctx->mpeg4_interlaced) {
        cfgInterlacedParameters(avctx, pSeqHeader);
    }

    pMPEG4SeqHeader->num_sprite_warping_points = enc->num_sprite_warping_points;
    pMPEG4SeqHeader->real_sprite_warping_points = enc->real_sprite_warping_points;
    pMPEG4SeqHeader->scalability = enc->scalability_real;
    pMPEG4SeqHeader->quarter_sample = enc->quarter_sample;
    pMPEG4SeqHeader->avctx = avctx;

    pMPEG4SeqHeader->divx_packed = enc->divx_packed;
    return 0;
}

static int cfgInterlacedParameters(AVCodecContext *avctx, SeqHeader *pSeqHeader)
{
    /*
     *   Deinterlace Mode Condition From Tao@ucode-team:
     *   1. width in (0 720],      full mode
     *   2. width in (720, 1920],  simple mode
     */
    if (avctx->width > 0 && avctx->width <= 720) {
        pSeqHeader->is_interlaced = 1;
        pSeqHeader->interlaced_mode = INTERLACED_MODE_FULL_MODE;
    } else if (avctx->width > 720 && avctx->width <= 1920) {
        pSeqHeader->is_interlaced = 1;
        pSeqHeader->interlaced_mode = INTERLACED_MODE_SIMPLE_MODE;
    } else {
        av_log(NULL, AV_LOG_ERROR, "cfgInterlacedParameters: incorrect codec_id=%d codec_name=%s width=%d\n", avctx->codec_id, avctx->codec_name, avctx->width);
    }

    return 0;
}
