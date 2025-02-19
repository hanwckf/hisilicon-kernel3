/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQnrOvXqrMLPutyY70C/YiLiDRftuGTCTSM/M8D
U4z4mMIJKj3E60iv7epdOPDMuIZRg0sPV7st8ca+GTIDeAfm3zT+h6PBVg5v2gR4CHaYC3Qj
YqXnWTo5uFgVNLyuRoT2QhM7pxstX09a8VcaeYHv7xNBpIrya3zRjXQ5wJtaAg==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * Copyright (c) 2009 Mans Rullgard <mans@mansr.com>
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

#include <stdint.h>

#include "avcodec.h"
#include "dsputil.h"
#include "dsputil_arm.h"

//guoshan+00101841 20100820 增加h263滤波器的arm代码优化
extern void vp6_filter_hv4asm_armv6(uint8_t *dst, uint8_t *src, int stride,
                                    const int16_t *h_weights, const int16_t *v_weights);
extern void vp6_filter_hv4asm_h(uint8_t *dst, uint8_t *src, int stride, int delta, const int16_t *weights);
extern void vp6_filter_hv4asm_v(uint8_t *dst, uint8_t *src, int stride, int delta, const int16_t *weights);
#ifdef ARMV8
void ff_put_pixels16_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_x2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_y2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_xy2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_x2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_y2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_xy2_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_x2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_y2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_xy2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_x2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_y2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_xy2_no_rnd_neon(uint8_t *, const uint8_t *, int, int);
void ff_avg_pixels16_neon(uint8_t *, const uint8_t *, int, int);
void  ff_avg_pixels16_x2_neon(uint8_t *block, const uint8_t *pixels, int line_size, int h);
void  ff_avg_pixels16_x2_neon(uint8_t *block, const uint8_t *pixels, int line_size, int h);
void  ff_avg_pixels16_y2_neon(uint8_t *block, const uint8_t *pixels,
                              int line_size, int h);
void ff_avg_pixels16_xy2_neon(uint8_t *block, const uint8_t *pixels,
                              int line_size, int h);
void ff_avg_pixels8_neon(uint8_t *block, const uint8_t *pixels,
                         int line_size, int h);
void   ff_avg_pixels8_x2_neon(uint8_t *block, const uint8_t *pixels,
                              int line_size, int h);
void   ff_avg_pixels8_y2_neon(uint8_t *block, const uint8_t *pixels,
                              int line_size, int h);
void  ff_avg_pixels8_xy2_neon(uint8_t *block, const uint8_t *pixels,
                              int line_size, int h);
void  ff_avg_pixels16_x2_no_rnd_neon(uint8_t *block, const uint8_t *pixels,
                                     int line_size, int h);
void  ff_avg_pixels16_y2_no_rnd_neon(uint8_t *block, const uint8_t *pixels,
                                     int line_size, int h);
void ff_avg_pixels16_xy2_no_rnd_neon(uint8_t *block, const uint8_t *pixels,
                                     int line_size, int h);
void av_cold ff_dsputil_init_armv8(DSPContext *c, AVCodecContext *avctx)
{
    c->put_pixels_tab[0][0] = ff_put_pixels16_neon;
    c->put_pixels_tab[0][1] = ff_put_pixels16_x2_neon;
    c->put_pixels_tab[0][2] = ff_put_pixels16_y2_neon;
    c->put_pixels_tab[0][3] = ff_put_pixels16_xy2_neon;
    c->put_pixels_tab[1][0] = ff_put_pixels8_neon;
    c->put_pixels_tab[1][1] = ff_put_pixels8_x2_neon;
    c->put_pixels_tab[1][2] = ff_put_pixels8_y2_neon;
    c->put_pixels_tab[1][3] = ff_put_pixels8_xy2_neon;
    c->put_no_rnd_pixels_tab[0][0] = ff_put_pixels16_neon;
    c->put_no_rnd_pixels_tab[0][1] = ff_put_pixels16_x2_neon;
    c->put_no_rnd_pixels_tab[0][2] = ff_put_pixels16_y2_neon;
    c->put_no_rnd_pixels_tab[0][3] = ff_put_pixels16_xy2_neon;
    c->put_no_rnd_pixels_tab[1][0] = ff_put_pixels8_neon;
    c->put_no_rnd_pixels_tab[1][1] = ff_put_pixels8_x2_neon;
    c->put_no_rnd_pixels_tab[1][2] = ff_put_pixels8_y2_neon;
    c->put_no_rnd_pixels_tab[1][3] = ff_put_pixels8_xy2_neon;
    c->avg_pixels_tab[0][0] = ff_avg_pixels16_neon;
    c->avg_pixels_tab[0][1] = ff_avg_pixels16_x2_neon;
    c->avg_pixels_tab[0][2] = ff_avg_pixels16_y2_neon;
    c->avg_pixels_tab[0][3] = ff_avg_pixels16_xy2_neon;
    c->avg_pixels_tab[1][0] = ff_avg_pixels8_neon;
    c->avg_pixels_tab[1][1] = ff_avg_pixels8_x2_neon;
    c->avg_pixels_tab[1][2] = ff_avg_pixels8_y2_neon;
    c->avg_pixels_tab[1][3] = ff_avg_pixels8_xy2_neon;
}
#else
extern const uint8_t ff_h263_loop_filter_strength[32];
extern void h263_v_lf_arm(uint8_t *src, int stride, int strength);
extern void h263_h_lf_arm(uint8_t *src, int stride, int strength);
static void h263_v_loop_filter_arm(uint8_t *src, int stride, int qscale);
static void h263_h_loop_filter_arm(uint8_t *src, int stride, int qscale);

void ff_simple_idct_armv6(DCTELEM *data);
void ff_simple_idct_put_armv6(uint8_t *dest, int line_size, DCTELEM *data);
void ff_simple_idct_add_armv6(uint8_t *dest, int line_size, DCTELEM *data);

void ff_put_pixels16_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_x2_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_y2_armv6(uint8_t *, const uint8_t *, int, int);

void ff_put_pixels16_x2_no_rnd_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels16_y2_no_rnd_armv6(uint8_t *, const uint8_t *, int, int);

void ff_avg_pixels16_armv6(uint8_t *, const uint8_t *, int, int);

void ff_put_pixels8_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_x2_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_y2_armv6(uint8_t *, const uint8_t *, int, int);

void ff_put_pixels8_x2_no_rnd_armv6(uint8_t *, const uint8_t *, int, int);
void ff_put_pixels8_y2_no_rnd_armv6(uint8_t *, const uint8_t *, int, int);

void ff_avg_pixels8_armv6(uint8_t *, const uint8_t *, int, int);

void ff_add_pixels_clamped_armv6(const DCTELEM *block,
                                 uint8_t *restrict pixels,
                                 int line_size);

void ff_get_pixels_armv6(DCTELEM *block, const uint8_t *pixels, int stride);
void ff_diff_pixels_armv6(DCTELEM *block, const uint8_t *s1,
                          const uint8_t *s2, int stride);

int ff_pix_abs16_armv6(void *s, uint8_t *blk1, uint8_t *blk2,
                       int line_size, int h);
int ff_pix_abs16_x2_armv6(void *s, uint8_t *blk1, uint8_t *blk2,
                          int line_size, int h);
int ff_pix_abs16_y2_armv6(void *s, uint8_t *blk1, uint8_t *blk2,
                          int line_size, int h);

int ff_pix_abs8_armv6(void *s, uint8_t *blk1, uint8_t *blk2,
                      int line_size, int h);

int ff_sse16_armv6(void *s, uint8_t *blk1, uint8_t *blk2,
                   int line_size, int h);

int ff_pix_norm1_armv6(uint8_t *pix, int line_size);
int ff_pix_sum_armv6(uint8_t *pix, int line_size);

/*x00141957 2010 1127 vp64抽头插值滤波 armv6代码*/



void av_cold ff_dsputil_init_armv6(DSPContext *c, AVCodecContext *avctx)
{
    if (!avctx->lowres && (avctx->idct_algo == FF_IDCT_AUTO ||
                           avctx->idct_algo == FF_IDCT_SIMPLEARMV6))
    {
        c->idct_put              = ff_simple_idct_put_armv6;
        c->idct_add              = ff_simple_idct_add_armv6;
        c->idct                  = ff_simple_idct_armv6;
        c->idct_permutation_type = FF_LIBMPEG2_IDCT_PERM;
    }

    /*郭姗所优化h263 loop filter代码  x00141957 2010 1127*/
    c->h263_v_loop_filter = h263_v_loop_filter_arm;
    c->h263_h_loop_filter = h263_h_loop_filter_arm;

    c->put_pixels_tab[0][0] = ff_put_pixels16_armv6;
    c->put_pixels_tab[0][1] = ff_put_pixels16_x2_armv6;
    c->put_pixels_tab[0][2] = ff_put_pixels16_y2_armv6;
    /*     c->put_pixels_tab[0][3] = ff_put_pixels16_xy2_armv6; */
    c->put_pixels_tab[1][0] = ff_put_pixels8_armv6;
    c->put_pixels_tab[1][1] = ff_put_pixels8_x2_armv6;
    c->put_pixels_tab[1][2] = ff_put_pixels8_y2_armv6;
    /*     c->put_pixels_tab[1][3] = ff_put_pixels8_xy2_armv6; */

    c->put_no_rnd_pixels_tab[0][0] = ff_put_pixels16_armv6;
    c->put_no_rnd_pixels_tab[0][1] = ff_put_pixels16_x2_no_rnd_armv6;
    c->put_no_rnd_pixels_tab[0][2] = ff_put_pixels16_y2_no_rnd_armv6;
    /*     c->put_no_rnd_pixels_tab[0][3] = ff_put_pixels16_xy2_no_rnd_armv6; */
    c->put_no_rnd_pixels_tab[1][0] = ff_put_pixels8_armv6;
    c->put_no_rnd_pixels_tab[1][1] = ff_put_pixels8_x2_no_rnd_armv6;
    c->put_no_rnd_pixels_tab[1][2] = ff_put_pixels8_y2_no_rnd_armv6;
    /*     c->put_no_rnd_pixels_tab[1][3] = ff_put_pixels8_xy2_no_rnd_armv6; */

    c->avg_pixels_tab[0][0] = ff_avg_pixels16_armv6;
    c->avg_pixels_tab[1][0] = ff_avg_pixels8_armv6;

    c->add_pixels_clamped = ff_add_pixels_clamped_armv6;
    c->get_pixels = ff_get_pixels_armv6;
    c->diff_pixels = ff_diff_pixels_armv6;

    c->pix_abs[0][0] = ff_pix_abs16_armv6;
    c->pix_abs[0][1] = ff_pix_abs16_x2_armv6;
    c->pix_abs[0][2] = ff_pix_abs16_y2_armv6;

    c->pix_abs[1][0] = ff_pix_abs8_armv6;

    c->sad[0] = ff_pix_abs16_armv6;
    c->sad[1] = ff_pix_abs8_armv6;

    c->sse[0] = ff_sse16_armv6;

    c->pix_norm1 = ff_pix_norm1_armv6;
    c->pix_sum   = ff_pix_sum_armv6;
#if 0

    /*x00141957 为vp6解码所写armv6汇编 2010 1128*/
    if (CONFIG_VP6_DECODER)
    {
        c->vp6_filter_diag4 = vp6_filter_hv4asm_armv6;
        c->vp6_filter_diag4_h = vp6_filter_hv4asm_h;
        c->vp6_filter_diag4_v = vp6_filter_hv4asm_v;
        c->vp56_edge_filter = vp56_edge_filter_armv6;
        c->vp6_filter_diag2 = vp6_filter_diag2_asm;
        c->vp6_filter_diag2_h = twotap_vp6_asm_h;
        c->vp6_filter_diag2_v = twotap_vp6_asm_v;
    }

#endif
}


//guoshan+00101841 20100820 增加h263滤波器的arm代码优化
static void h263_v_loop_filter_arm(uint8_t *src, int stride, int qscale)
{
    const int strength = ff_h263_loop_filter_strength[qscale];

    h263_v_lf_arm(src, stride, strength);
}

static void h263_h_loop_filter_arm(uint8_t *src, int stride, int qscale)
{
    const int strength = ff_h263_loop_filter_strength[qscale];

    h263_h_lf_arm(src, stride, strength);
}
#endif
