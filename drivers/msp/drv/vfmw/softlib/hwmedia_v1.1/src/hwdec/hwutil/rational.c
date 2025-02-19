/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQUPqbpJPz/c/KR2HuFneshZ7x5/nUrYcwVjZST
wdqlOy/ELyUAz5YUoYCuuS+lZMeoTH1F2U7DDu4h9BGxB+OQgDyKJ2L9KwScanaIYJg0T89z
7kogTxgIGbZ9p+R652Mlbsmh7tSmPoe5qU07pcJPTOQ1vWCJzBuObaUNa2BhkA==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * rational numbers
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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

/**
 * @file libavutil/rational.c
 * rational numbers
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include <assert.h>
#ifndef __KERNEL__
#include <math.h>
#include <limits.h>
#endif

#include "common.h"
#include "mathematics.h"
#include "rational.h"


#if 0
int av_reduce(int *dst_num, int *dst_den, int64_t num, int64_t den, int64_t max)
{
    AVRational a0 = {0, 1}, a1 = {1, 0};
    int sign = (num < 0) ^ (den < 0);
    int64_t gcd = av_gcd(FFABS(num), FFABS(den));

    if (gcd)
    {
        num = FFABS(num) / gcd;
        den = FFABS(den) / gcd;
    }

    if (num <= max && den <= max)
    {
        a1.num = (int)num;
        a1.den = (int)den;
        den = 0;
    }

    while (den)
    {
        uint64_t x      = num / den;
        int64_t next_den = num - den * x;
        int64_t a2n = x * a1.num + a0.num;
        int64_t a2d = x * a1.den + a0.den;

        if (a2n > max || a2d > max)
        {
            if (a1.num) { x = (max - a0.num) / a1.num; }

            if (a1.den) { x = FFMIN((int64_t)x, (max - a0.den) / a1.den); }

            if (den * (2 * (int64_t)x * a1.den + a0.den) > num * a1.den)
            { a1.num = (int)(x * a1.num + a0.num); }

            a1.den = (int)(x * a1.den + a0.den);
            break;
        }

        a0 = a1;
        a1.num = (int)a2n;
        a1.den = (int)a2d;
        num = den;
        den = next_den;
    }

    /*x00141957 20100506*/
    //    assert(av_gcd(a1.num, a1.den) <= 1U);

    *dst_num = sign ? -a1.num : a1.num;
    *dst_den = a1.den;

    return den == 0;
}

AVRational av_mul_q(AVRational b, AVRational c)
{
    av_reduce(&b.num, &b.den, b.num * (int64_t)c.num, b.den * (int64_t)c.den, INT_MAX);
    return b;
}

AVRational av_div_q(AVRational b, AVRational c)
{
    AVRational d = {c.den, c.num};
    return av_mul_q(b, d);
    //return av_mul_q(b, (AVRational){c.den, c.num});
}

AVRational av_add_q(AVRational b, AVRational c)
{
    av_reduce(&b.num, &b.den, b.num * (int64_t)c.den + c.num * (int64_t)b.den, b.den * (int64_t)c.den, INT_MAX);
    return b;
}

AVRational av_sub_q(AVRational b, AVRational c)
{
    AVRational d = { -c.num, c.den};
    return av_add_q(b, d);
    //return av_add_q(b, (AVRational){-c.num, c.den});
}


AVRational av_d2q(double d, int max)
{
    AVRational a;
#define LOG2  0.69314718055994530941723212145817656807550013436025
    int exponent = FFMAX( (int)(log(FFABS(d) + 1e-20) / LOG2), 0);
    int64_t den = 1LL << (61 - exponent);
    av_reduce(&a.num, &a.den, (int64_t)(d * den + 0.5), den, max);

    return a;
}


//#ifndef __KERNEL__
int av_nearer_q(AVRational q, AVRational q1, AVRational q2)
{
    /* n/d is q, a/b is the median between q1 and q2 */
    int64_t a = q1.num * (int64_t)q2.den + q2.num * (int64_t)q1.den;
    int64_t b = 2 * (int64_t)q1.den * q2.den;

    /* rnd_up(a*d/b) > n => a*d/b > n */
    int64_t x_up = av_rescale_rnd(a, q.den, b, AV_ROUND_UP);

    /* rnd_down(a*d/b) < n => a*d/b < n */
    int64_t x_down = av_rescale_rnd(a, q.den, b, AV_ROUND_DOWN);

    return ((x_up > q.num) - (x_down < q.num)) * av_cmp_q(q2, q1);
}

int av_find_nearest_q_idx(AVRational q, const AVRational *q_list)
{
    int i, nearest_q_idx = 0;

    for (i = 0; q_list[i].den; i++)
        if (av_nearer_q(q, q_list[i], q_list[nearest_q_idx]) > 0)
        { nearest_q_idx = i; }

    return nearest_q_idx;
}
#endif
//#endif
