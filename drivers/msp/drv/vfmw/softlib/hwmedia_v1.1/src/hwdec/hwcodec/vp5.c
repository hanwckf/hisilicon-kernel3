/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQnrOvXqrMLPutyY70C/YiLiDRftuGTCTSM/M8D
U4z4mFT03aGnfhIeBmL8e4R1Lhcwfpm8dBNHkbvee9Cf1JUMr5FJwOTqb4rCvfKG0cQZBMcc
h/pOrowrDohMllrEP0LntCa0889W1XzEpes/6kyvD4woeJIPi5qplsTIgClFbQ==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/**
 * @file libavcodec/vp5.c
 * VP5 compatible video decoder
 *
 * Copyright (C) 2006  Aurelien Jacobs <aurel@gnuage.org>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
//#include <string.h>

#include "avcodec.h"
#include "dsputil.h"
#include "bitstream.h"

#include "vp56.h"
#include "vp56data.h"
#include "vp5data.h"


static int vp5_parse_header(VP56Context *s, const uint8_t *buf, int buf_size,
                            int *golden_frame, int *key_frame)
{
    VP56RangeCoder *c = &s->c;
    int rows, cols;

    vp56_init_range_decoder(&s->c, buf, buf_size);
    s->framep[VP56_FRAME_CURRENT]->key_frame = !vp56_rac_get(c);
    *key_frame =  s->framep[VP56_FRAME_CURRENT]->key_frame;
    vp56_rac_get(c);
    vp56_init_dequant(s, vp56_rac_gets(c, 6));

    if (s->framep[VP56_FRAME_CURRENT]->key_frame)
    {
        vp56_rac_gets(c, 8);

        if (vp56_rac_gets(c, 5) > 5)
        { return 0; }

        vp56_rac_gets(c, 2);

        if (vp56_rac_get(c))
        {
            av_log(s->avctx, AV_LOG_ERROR, "interlacing not supported\n");
            return 0;
        }

        rows = vp56_rac_gets(c, 8);  /* number of stored macroblock rows */
        cols = vp56_rac_gets(c, 8);  /* number of stored macroblock cols */
        vp56_rac_gets(c, 8);  /* number of displayed macroblock rows */
        vp56_rac_gets(c, 8);  /* number of displayed macroblock cols */
        vp56_rac_gets(c, 2);

        if (!s->macroblocks || /* first frame */
            16 * cols != s->avctx->coded_width ||
            16 * rows != s->avctx->coded_height)
        {
            avcodec_set_dimensions(s->avctx, 16 * cols, 16 * rows);
            return 2;
        }
    }
    else if (!s->macroblocks)
    { return 0; }

    return 1;
}

#if 0
/* Gives very similar result than the vp6 version except in a few cases */
static int vp5_adjust(int v, int t)
{
    int s2, s1 = v >> 31;
    v ^= s1;
    v -= s1;
    v *= v < 2 * t;
    v -= t;
    s2 = v >> 31;
    v ^= s2;
    v -= s2;
    v = t - v;
    v += s1;
    v ^= s1;
    return v;
}
#else
static int vp5_adjust( int32_t FiltVal, uint32_t FLimit)
{
    int32_t Clamp;
    int32_t FiltSign;
    int32_t NewSign;

    Clamp = 2 * FLimit;

    // Next 3 lines are fast way to find abs...
    FiltSign = (FiltVal >> 31);         // Sign extension makes FiltSign all 0's or all 1's
    FiltVal ^= FiltSign;                // FiltVal is then 1's complement of value if -ve
    FiltVal -= FiltSign;                // Filtval = abs Filtval

    FiltVal *= (FiltVal < Clamp);       // clamp filter value to 2 times limit

    FiltVal -= FLimit;                  // subtract limit value

    // Next 3 lines are fast way to find abs...
    NewSign = (FiltVal >> 31);          // Sign extension makes NewSign all 0's or all 1's
    FiltVal ^= NewSign;                 // FiltVal is then 1's complement of value if -ve
    FiltVal -= NewSign;                 // FiltVal = abs FiltVal

    FiltVal = FLimit - FiltVal;         // flimit - abs (filtVal - flimit)

    FiltVal += FiltSign;                // convert back to signed value
    FiltVal ^= FiltSign;

    return FiltVal;
}
#endif

static void vp5_parse_vector_adjustment(VP56Context *s, VP56mv *vect)
{
    VP56RangeCoder *c = &s->c;
    VP56Model *model = s->modelp;
    int comp, di;

    for (comp = 0; comp < 2; comp++)
    {
        int delta = 0;

        if (vp56_rac_get_prob(c, model->vector_dct[comp]))
        {
            int sign = vp56_rac_get_prob(c, model->vector_sig[comp]);
            di  = vp56_rac_get_prob(c, model->vector_pdi[comp][0]);
            di |= vp56_rac_get_prob(c, model->vector_pdi[comp][1]) << 1;
            delta = vp56_rac_get_tree(c, vp56_pva_tree,
                                      model->vector_pdv[comp]);
            delta = di | (delta << 2);
            delta = (delta ^ -sign) + sign;
        }

        if (!comp)
        { vect->x = delta; }
        else
        { vect->y = delta; }
    }
}

static void vp5_parse_vector_models(VP56Context *s)
{
    VP56RangeCoder *c = &s->c;
    VP56Model *model = s->modelp;
    int comp, node;

    for (comp = 0; comp < 2; comp++)
    {
        if (vp56_rac_get_prob(c, vp5_vmc_pct[comp][0]))
        { model->vector_dct[comp] = vp56_rac_gets_nn(c, 7); }

        if (vp56_rac_get_prob(c, vp5_vmc_pct[comp][1]))
        { model->vector_sig[comp] = vp56_rac_gets_nn(c, 7); }

        if (vp56_rac_get_prob(c, vp5_vmc_pct[comp][2]))
        { model->vector_pdi[comp][0] = vp56_rac_gets_nn(c, 7); }

        if (vp56_rac_get_prob(c, vp5_vmc_pct[comp][3]))
        { model->vector_pdi[comp][1] = vp56_rac_gets_nn(c, 7); }
    }

    for (comp = 0; comp < 2; comp++)
        for (node = 0; node < 7; node++)
            if (vp56_rac_get_prob(c, vp5_vmc_pct[comp][4 + node]))
            { model->vector_pdv[comp][node] = vp56_rac_gets_nn(c, 7); }
}

static int vp5_parse_coeff_models(VP56Context *s)
{
    VP56RangeCoder *c = &s->c;
    VP56Model *model = s->modelp;
    uint8_t def_prob[11];
    int node, cg, ctx;
    int ct;    /* code type */
    int pt;    /* plane type (0 for Y, 1 for U or V) */

    memset(def_prob, 0x80, sizeof(def_prob));

    for (pt = 0; pt < 2; pt++)
        for (node = 0; node < 11; node++)
            if (vp56_rac_get_prob(c, vp5_dccv_pct[pt][node]))
            {
                def_prob[node] = vp56_rac_gets_nn(c, 7);
                model->coeff_dccv[pt][node] = def_prob[node];
            }
            else if (s->framep[VP56_FRAME_CURRENT]->key_frame)
            {
                model->coeff_dccv[pt][node] = def_prob[node];
            }

    for (ct = 0; ct < 3; ct++)
        for (pt = 0; pt < 2; pt++)
            for (cg = 0; cg < 6; cg++)
                for (node = 0; node < 11; node++)
                    if (vp56_rac_get_prob(c, vp5_ract_pct[ct][pt][cg][node]))
                    {
                        def_prob[node] = vp56_rac_gets_nn(c, 7);
                        model->coeff_ract[pt][ct][cg][node] = def_prob[node];
                    }
                    else if (s->framep[VP56_FRAME_CURRENT]->key_frame)
                    {
                        model->coeff_ract[pt][ct][cg][node] = def_prob[node];
                    }

    /* coeff_dcct is a linear combination of coeff_dccv */
    for (pt = 0; pt < 2; pt++)
        for (ctx = 0; ctx < 36; ctx++)
            for (node = 0; node < 5; node++)
            { model->coeff_dcct[pt][ctx][node] = av_clip(((model->coeff_dccv[pt][node] * vp5_dccv_lc[node][ctx][0] + 128) >> 8) + vp5_dccv_lc[node][ctx][1], 1, 254); }

    /* coeff_acct is a linear combination of coeff_ract */
    for (ct = 0; ct < 3; ct++)
        for (pt = 0; pt < 2; pt++)
            for (cg = 0; cg < 3; cg++)
                for (ctx = 0; ctx < 6; ctx++)
                    for (node = 0; node < 5; node++)
                    { model->coeff_acct[pt][ct][cg][ctx][node] = av_clip(((model->coeff_ract[pt][ct][cg][node] * vp5_ract_lc[ct][cg][node][ctx][0] + 128) >> 8) + vp5_ract_lc[ct][cg][node][ctx][1], 1, 254); }

    return 0;
}

static void vp5_parse_coeff(VP56Context *s)
{
    VP56RangeCoder *c = &s->c;
    VP56Model *model = s->modelp;
    uint8_t *permute = s->scantable.permutated;
    uint8_t *model1, *model2;
    int coeff, sign, coeff_idx;
    int b, i, cg, idx, ctx, ctx_last;
    int pt = 0;    /* plane type (0 for Y, 1 for U or V) */

    for (b = 0; b < 6; b++)
    {
        int ct = 1;    /* code type */

        if (b > 3) { pt = 1; }

        ctx = 6 * s->coeff_ctx[vp56_b6to4[b]][0]
              + s->above_blocks[s->above_block_idx[b]].not_null_dc;
        model1 = model->coeff_dccv[pt];
        model2 = model->coeff_dcct[pt][ctx];

        for (coeff_idx = 0; coeff_idx < 64; )
        {
            if (vp56_rac_get_prob(c, model2[0]))
            {
                if (vp56_rac_get_prob(c, model2[2]))
                {
                    if (vp56_rac_get_prob(c, model2[3]))
                    {
                        s->coeff_ctx[vp56_b6to4[b]][coeff_idx] = 4;
                        idx = vp56_rac_get_tree(c, vp56_pc_tree, model1);
                        sign = vp56_rac_get(c);
                        coeff = vp56_coeff_bias[idx + 5];

                        for (i = vp56_coeff_bit_length[idx]; i >= 0; i--)
                        { coeff += vp56_rac_get_prob(c, vp56_coeff_parse_table[idx][i]) << i; }
                    }
                    else
                    {
                        if (vp56_rac_get_prob(c, model2[4]))
                        {
                            coeff = 3 + vp56_rac_get_prob(c, model1[5]);
                            s->coeff_ctx[vp56_b6to4[b]][coeff_idx] = 3;
                        }
                        else
                        {
                            coeff = 2;
                            s->coeff_ctx[vp56_b6to4[b]][coeff_idx] = 2;
                        }

                        sign = vp56_rac_get(c);
                    }

                    ct = 2;
                }
                else
                {
                    ct = 1;
                    s->coeff_ctx[vp56_b6to4[b]][coeff_idx] = 1;
                    sign = vp56_rac_get(c);
                    coeff = 1;
                }

                coeff = (coeff ^ -sign) + sign;

                if (coeff_idx)
                { coeff *= s->dequant_ac; }

                s->block_coeff[b][permute[coeff_idx]] = coeff;
            }
            else
            {
                if (ct && !vp56_rac_get_prob(c, model2[1]))
                { break; }

                ct = 0;
                s->coeff_ctx[vp56_b6to4[b]][coeff_idx] = 0;
            }

            cg = vp5_coeff_groups[++coeff_idx];
            ctx = s->coeff_ctx[vp56_b6to4[b]][coeff_idx];
            model1 = model->coeff_ract[pt][ct][cg];
            model2 = cg > 2 ? model1 : model->coeff_acct[pt][ct][cg][ctx];
        }

        ctx_last = FFMIN(s->coeff_ctx_last[vp56_b6to4[b]], 24);
        s->coeff_ctx_last[vp56_b6to4[b]] = coeff_idx;

        if (coeff_idx < ctx_last)
            for (i = coeff_idx; i <= ctx_last; i++)
            { s->coeff_ctx[vp56_b6to4[b]][i] = 5; }

        s->above_blocks[s->above_block_idx[b]].not_null_dc = s->coeff_ctx[vp56_b6to4[b]][0];
    }
}

static void vp5_default_models_init(VP56Context *s)
{
    VP56Model *model = s->modelp;
    int i;

    for (i = 0; i < 2; i++)
    {
        model->vector_sig[i] = 0x80;
        model->vector_dct[i] = 0x80;
        model->vector_pdi[i][0] = 0x55;
        model->vector_pdi[i][1] = 0x80;
    }

    memcpy(model->mb_types_stats, vp56_def_mb_types_stats, sizeof(model->mb_types_stats));
    memset(model->vector_pdv, 0x80, sizeof(model->vector_pdv));
}

static av_cold int vp5_decode_init(AVCodecContext *avctx)
{
    VP56Context *s = avctx->priv_data;

    vp56_init(avctx, 1, 0);
    s->vp56_coord_div = vp5_coord_div;
    s->parse_vector_adjustment = vp5_parse_vector_adjustment;
    s->adjust = vp5_adjust;
    s->parse_coeff = vp5_parse_coeff;
    s->default_models_init = vp5_default_models_init;
    s->parse_vector_models = vp5_parse_vector_models;
    s->parse_coeff_models = vp5_parse_coeff_models;
    s->parse_header = vp5_parse_header;

    return 0;
}

AVCodec vp5_decoder =
{
    "vp5",
    CODEC_TYPE_VIDEO,
    CODEC_ID_VP5,
    sizeof(VP56Context),
    vp5_decode_init,
    NULL,
    vp56_free,
    vp56_decode_frame,
    CODEC_CAP_DR1,
    //    .long_name = NULL_IF_CONFIG_SMALL("On2 VP5"),
};
