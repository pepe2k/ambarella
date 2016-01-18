/*!\file
   \brief VP8 decoder support via libvpx
*/

#include "avcodec.h"

#ifndef HAVE_STDINT_H
# define HAVE_STDINT_H 1
#endif
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx_decoder.h"
#include "vp8dx.h"

#include <assert.h>

typedef struct VP8DecoderContext {
  vpx_codec_ctx_t decoder;
} vp8dctx_t;

static av_cold int vp8_init(AVCodecContext* avctx)
{
  vp8dctx_t* const ctx = avctx->priv_data;
  vpx_codec_iface_t* const iface = &vpx_codec_vp8_dx_algo;
  vpx_codec_dec_cfg_t deccfg = { /*token partitions+1 would be a decent choice*/
                                 .threads= FFMIN(avctx->thread_count,16) };

  av_log(avctx,AV_LOG_ERROR,"thread_count:%d\n",avctx->thread_count);

  const vpx_codec_flags_t flags = 0;

  //av_log(avctx,AV_LOG_INFO,"%s\n",vpx_codec_version_str());
  //av_log(avctx,AV_LOG_VERBOSE,"%s\n",vpx_codec_build_config());

  if(vpx_codec_dec_init(&ctx->decoder,iface,&deccfg,flags)!=VPX_CODEC_OK) {
    const char* error = vpx_codec_error(&ctx->decoder);
    av_log(avctx,AV_LOG_ERROR,"Failed to initialize decoder: %s\n",error);
    return -1;
  }

  avctx->pix_fmt = PIX_FMT_YUV420P;
  return 0;
}

static int vp8_decode(AVCodecContext* avctx,
                      void* data, int* data_size,
                      AVPacket *avpkt)
{
  const uint8_t* const buf = avpkt->data;
  const int buf_size = avpkt->size;
  vp8dctx_t* const ctx = avctx->priv_data;
  AVFrame* const picture = data;
  vpx_codec_iter_t iter = NULL;
  vpx_image_t* img;

  if(vpx_codec_decode(&ctx->decoder,buf,buf_size,NULL,0)!=VPX_CODEC_OK) {
    const char* error = vpx_codec_error(&ctx->decoder);
    const char* detail = vpx_codec_error_detail(&ctx->decoder);

    av_log(avctx,AV_LOG_ERROR,"Failed to decode frame: %s\n",error);
    if(detail) av_log(avctx,AV_LOG_ERROR,"  Additional information: %s\n",detail);
    return -1;
  }

  if( (img= vpx_codec_get_frame(&ctx->decoder,&iter)) ) {
    assert(img->fmt==IMG_FMT_I420);

    if((int)img->d_w!=avctx->width || (int)img->d_h!=avctx->height) {
      av_log(avctx,AV_LOG_INFO,"dimension change! %dx%d -> %dx%d\n",
        avctx->width,avctx->height,img->d_w,img->d_h);
      if (av_image_check_size(img->d_w, img->d_h, 0, avctx))
        return -1;
      avcodec_set_dimensions(avctx,img->d_w,img->d_h);
    }
    picture->data[0] = img->planes[0];
    picture->data[1] = img->planes[1];
    picture->data[2] = img->planes[2];
    picture->data[3] = img->planes[3];
    picture->linesize[0] = img->stride[0];
    picture->linesize[1] = img->stride[1];
    picture->linesize[2] = img->stride[2];
    picture->linesize[3] = img->stride[3];
    *data_size = sizeof(AVPicture);
  }
  return buf_size;
}

static av_cold int vp8_free(AVCodecContext *avctx)
{
  vp8dctx_t* const ctx = avctx->priv_data;
  vpx_codec_destroy(&ctx->decoder);
  return 0;
}


AVCodec ff_libvpx_decoder = {
  "libvpx_vp8",
  AVMEDIA_TYPE_VIDEO,
  CODEC_ID_VP8,
  sizeof(vp8dctx_t),
  vp8_init,
  NULL, /*encode*/
  vp8_free,
  vp8_decode,
  0, /*capabilities*/
  .long_name = NULL_IF_CONFIG_SMALL("libvpx VP8"),
};
