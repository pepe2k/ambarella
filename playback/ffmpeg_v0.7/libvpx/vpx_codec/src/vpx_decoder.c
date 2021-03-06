/*
 *  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license and patent
 *  grant that can be found in the LICENSE file in the root of the source
 *  tree. All contributing project authors may be found in the AUTHORS
 *  file in the root of the source tree.
 */


/*!\file vpx_decoder.c
 * \brief Provides the high level interface to wrap decoder algorithms.
 *
 */
#include <stdlib.h>
#include "vpx_codec/internal/vpx_codec_internal.h"

#define SAVE_STATUS(ctx,var) (ctx?(ctx->err = var):var)

vpx_codec_err_t vpx_codec_peek_stream_info(vpx_codec_iface_t       *iface,
        const uint8_t         *data,
        unsigned int           data_sz,
        vpx_codec_stream_info_t *si)
{
    vpx_codec_err_t res;

    if (!iface || !data || !data_sz || !si
        || si->sz < sizeof(vpx_codec_stream_info_t))
        res = VPX_CODEC_INVALID_PARAM;
    else
    {
        /* Set default/unknown values */
        si->w = 0;
        si->h = 0;

        res = iface->dec.peek_si(data, data_sz, si);
    }

    return res;
}


vpx_codec_err_t vpx_codec_get_stream_info(vpx_codec_ctx_t         *ctx,
        vpx_codec_stream_info_t *si)
{
    vpx_codec_err_t res;

    if (!ctx || !si || si->sz < sizeof(vpx_codec_stream_info_t))
        res = VPX_CODEC_INVALID_PARAM;
    else if (!ctx->iface || !ctx->priv)
        res = VPX_CODEC_ERROR;
    else
    {
        /* Set default/unknown values */
        si->w = 0;
        si->h = 0;

        res = ctx->iface->dec.get_si(ctx->priv->alg_priv, si);
    }

    return SAVE_STATUS(ctx, res);
}


vpx_codec_err_t vpx_codec_decode(vpx_codec_ctx_t    *ctx,
                                 const uint8_t        *data,
                                 unsigned int    data_sz,
                                 void       *user_priv,
                                 long        deadline)
{
    vpx_codec_err_t res;

    if (!ctx || !data || !data_sz)
        res = VPX_CODEC_INVALID_PARAM;
    else if (!ctx->iface || !ctx->priv)
        res = VPX_CODEC_ERROR;

#if CONFIG_EVAL_LIMIT
    else if (ctx->priv->eval_counter >= 500)
    {
        ctx->priv->err_detail = "Evaluation limit exceeded.";
        res = VPX_CODEC_ERROR;
    }

#endif
    else
    {
        res = ctx->iface->dec.decode(ctx->priv->alg_priv, data, data_sz,
                                     user_priv, deadline);
#if CONFIG_EVAL_LIMIT
        ctx->priv->eval_counter++;
#endif
    }

    return SAVE_STATUS(ctx, res);
}

vpx_image_t *vpx_codec_get_frame(vpx_codec_ctx_t  *ctx,
                                 vpx_codec_iter_t *iter)
{
    vpx_image_t *img;

    if (!ctx || !iter || !ctx->iface || !ctx->priv)
        img = NULL;
    else
        img = ctx->iface->dec.get_frame(ctx->priv->alg_priv, iter);

    return img;
}


vpx_codec_err_t vpx_codec_register_put_frame_cb(vpx_codec_ctx_t             *ctx,
        vpx_codec_put_frame_cb_fn_t  cb,
        void                      *user_priv)
{
    vpx_codec_err_t res;

    if (!ctx || !cb)
        res = VPX_CODEC_INVALID_PARAM;
    else if (!ctx->iface || !ctx->priv
             || !(ctx->iface->caps & VPX_CODEC_CAP_PUT_FRAME))
        res = VPX_CODEC_ERROR;
    else
    {
        ctx->priv->dec.put_frame_cb.put_frame = cb;
        ctx->priv->dec.put_frame_cb.user_priv = user_priv;
        res = VPX_CODEC_OK;
    }

    return SAVE_STATUS(ctx, res);
}


vpx_codec_err_t vpx_codec_register_put_slice_cb(vpx_codec_ctx_t             *ctx,
        vpx_codec_put_slice_cb_fn_t  cb,
        void                      *user_priv)
{
    vpx_codec_err_t res;

    if (!ctx || !cb)
        res = VPX_CODEC_INVALID_PARAM;
    else if (!ctx->iface || !ctx->priv
             || !(ctx->iface->caps & VPX_CODEC_CAP_PUT_FRAME))
        res = VPX_CODEC_ERROR;
    else
    {
        ctx->priv->dec.put_slice_cb.put_slice = cb;
        ctx->priv->dec.put_slice_cb.user_priv = user_priv;
        res = VPX_CODEC_OK;
    }

    return SAVE_STATUS(ctx, res);
}


vpx_codec_err_t vpx_codec_get_mem_map(vpx_codec_ctx_t                *ctx,
                                      vpx_codec_mmap_t               *mmap,
                                      vpx_codec_iter_t               *iter)
{
    vpx_codec_err_t res = VPX_CODEC_OK;

    if (!ctx || !mmap || !iter || !ctx->iface)
        res = VPX_CODEC_INVALID_PARAM;
    else if (!(ctx->iface->caps & VPX_CODEC_CAP_XMA))
        res = VPX_CODEC_ERROR;
    else
        res = ctx->iface->get_mmap(ctx, mmap, iter);

    return SAVE_STATUS(ctx, res);
}


vpx_codec_err_t vpx_codec_set_mem_map(vpx_codec_ctx_t   *ctx,
                                      vpx_codec_mmap_t  *mmap,
                                      unsigned int     num_maps)
{
    vpx_codec_err_t res = VPX_CODEC_MEM_ERROR;

    if (!ctx || !mmap || !ctx->iface)
        res = VPX_CODEC_INVALID_PARAM;
    else if (!(ctx->iface->caps & VPX_CODEC_CAP_XMA))
        res = VPX_CODEC_ERROR;
    else
    {
        unsigned int i;

        for (i = 0; i < num_maps; i++, mmap++)
        {
            if (!mmap->base)
                break;

            /* Everything look ok, set the mmap in the decoder */
            res = ctx->iface->set_mmap(ctx, mmap);

            if (res)
                break;
        }
    }

    return SAVE_STATUS(ctx, res);
}
