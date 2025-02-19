/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQgYAryPbE4w1z7tx39CnajerL14+es+s0FHpIB
vtZ3TTehErRcel4h3B+cP3mXhpHdO6ke6ny5X0KL8IScnDjkn0DnsUE560LD5hWaEyUQ0z4e
CBk+ULfsA5LFAeCORHRtitwFIJBfkt/YxIXSXv/XkjdQL1Gzp0wcLuAgaIi7+Q==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * Floating point AAN DCT
 * this implementation is based upon the IJG integer AAN DCT (see jfdctfst.c)
 *
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2003 Roman Shaposhnik
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file libavcodec/faandct.c
 * @brief
 *     Floating point AAN DCT
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "dsputil.h"
#include "faandct.h"

#define FLOAT float
#ifdef FAAN_POSTSCALE
#    define SCALE(x) postscale[x]
#else
#    define SCALE(x) 1
#endif

#define B0 1.00000000000000000000
#define B1 0.72095982200694791383 // (cos(pi*1/16)sqrt(2))^-1
#define B2 0.76536686473017954350 // (cos(pi*2/16)sqrt(2))^-1
#define B3 0.85043009476725644878 // (cos(pi*3/16)sqrt(2))^-1
#define B4 1.00000000000000000000 // (cos(pi*4/16)sqrt(2))^-1
#define B5 1.27275858057283393842 // (cos(pi*5/16)sqrt(2))^-1
#define B6 1.84775906502257351242 // (cos(pi*6/16)sqrt(2))^-1
#define B7 3.62450978541155137218 // (cos(pi*7/16)sqrt(2))^-1


#define A1 0.70710678118654752438 // cos(pi*4/16)
#define A2 0.54119610014619698435 // cos(pi*6/16)sqrt(2)
#define A5 0.38268343236508977170 // cos(pi*6/16)
#define A4 1.30656296487637652774 // cos(pi*2/16)sqrt(2)

static const double postscale[64] =
{
    B0 * B0, B0 * B1, B0 * B2, B0 * B3, B0 * B4, B0 * B5, B0 * B6, B0 * B7,
    B1 * B0, B1 * B1, B1 * B2, B1 * B3, B1 * B4, B1 * B5, B1 * B6, B1 * B7,
    B2 * B0, B2 * B1, B2 * B2, B2 * B3, B2 * B4, B2 * B5, B2 * B6, B2 * B7,
    B3 * B0, B3 * B1, B3 * B2, B3 * B3, B3 * B4, B3 * B5, B3 * B6, B3 * B7,
    B4 * B0, B4 * B1, B4 * B2, B4 * B3, B4 * B4, B4 * B5, B4 * B6, B4 * B7,
    B5 * B0, B5 * B1, B5 * B2, B5 * B3, B5 * B4, B5 * B5, B5 * B6, B5 * B7,
    B6 * B0, B6 * B1, B6 * B2, B6 * B3, B6 * B4, B6 * B5, B6 * B6, B6 * B7,
    B7 * B0, B7 * B1, B7 * B2, B7 * B3, B7 * B4, B7 * B5, B7 * B6, B7 * B7,
};

static av_always_inline void row_fdct(FLOAT temp[64], DCTELEM *data)
{
    FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    FLOAT tmp10, tmp11, tmp12, tmp13;
    FLOAT z2, z4, z11, z13;
    //    FLOAT av_unused z5;
    int i;

    for (i = 0; i < 8 * 8; i += 8)
    {
        tmp0 = (float)(data[0 + i] + data[7 + i]);
        tmp7 = (float)(data[0 + i] - data[7 + i]);
        tmp1 = (float)(data[1 + i] + data[6 + i]);
        tmp6 = (float)(data[1 + i] - data[6 + i]);
        tmp2 = (float)(data[2 + i] + data[5 + i]);
        tmp5 = (float)(data[2 + i] - data[5 + i]);
        tmp3 = (float)(data[3 + i] + data[4 + i]);
        tmp4 = (float)(data[3 + i] - data[4 + i]);

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        temp[0 + i] = tmp10 + tmp11;
        temp[4 + i] = tmp10 - tmp11;

        tmp12 += tmp13;
        tmp12 = (float)(tmp12 * A1);
        temp[2 + i] = tmp13 + tmp12;
        temp[6 + i] = tmp13 - tmp12;

        tmp4 += tmp5;
        tmp5 += tmp6;
        tmp6 += tmp7;

#if 0
        z5 = (tmp4 - tmp6) * A5;
        z2 = tmp4 * A2 + z5;
        z4 = tmp6 * A4 + z5;
#else
        z2 = (float)(tmp4 * (A2 + A5) - tmp6 * A5);
        z4 = (float)(tmp6 * (A4 - A5) + tmp4 * A5);
#endif
        tmp5 = (float)(tmp5 * A1);

        z11 = tmp7 + tmp5;
        z13 = tmp7 - tmp5;

        temp[5 + i] = z13 + z2;
        temp[3 + i] = z13 - z2;
        temp[1 + i] = z11 + z4;
        temp[7 + i] = z11 - z4;
    }
}

void ff_faandct(DCTELEM *data)
{
    FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    FLOAT tmp10, tmp11, tmp12, tmp13;
    FLOAT z2, z4, z11, z13;
    //    FLOAT av_unused z5;
    FLOAT temp[64];
    int i;

    emms_c();

    row_fdct(temp, data);

    for (i = 0; i < 8; i++)
    {
        tmp0 = temp[8 * 0 + i] + temp[8 * 7 + i];
        tmp7 = temp[8 * 0 + i] - temp[8 * 7 + i];
        tmp1 = temp[8 * 1 + i] + temp[8 * 6 + i];
        tmp6 = temp[8 * 1 + i] - temp[8 * 6 + i];
        tmp2 = temp[8 * 2 + i] + temp[8 * 5 + i];
        tmp5 = temp[8 * 2 + i] - temp[8 * 5 + i];
        tmp3 = temp[8 * 3 + i] + temp[8 * 4 + i];
        tmp4 = temp[8 * 3 + i] - temp[8 * 4 + i];

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        data[8 * 0 + i] = lrintf(SCALE(8 * 0 + i) * (tmp10 + tmp11));
        data[8 * 4 + i] = lrintf(SCALE(8 * 4 + i) * (tmp10 - tmp11));

        tmp12 += tmp13;
        tmp12 = (float)(tmp12 * A1);
        data[8 * 2 + i] = lrintf(SCALE(8 * 2 + i) * (tmp13 + tmp12));
        data[8 * 6 + i] = lrintf(SCALE(8 * 6 + i) * (tmp13 - tmp12));

        tmp4 += tmp5;
        tmp5 += tmp6;
        tmp6 += tmp7;

#if 0
        z5 = (tmp4 - tmp6) * A5;
        z2 = tmp4 * A2 + z5;
        z4 = tmp6 * A4 + z5;
#else
        z2 = (float)(tmp4 * (A2 + A5) - tmp6 * A5);
        z4 = (float)(tmp6 * (A4 - A5) + tmp4 * A5);
#endif
        tmp5 = (float)(tmp5 * A1);

        z11 = tmp7 + tmp5;
        z13 = tmp7 - tmp5;

        data[8 * 5 + i] = lrintf(SCALE(8 * 5 + i) * (z13 + z2));
        data[8 * 3 + i] = lrintf(SCALE(8 * 3 + i) * (z13 - z2));
        data[8 * 1 + i] = lrintf(SCALE(8 * 1 + i) * (z11 + z4));
        data[8 * 7 + i] = lrintf(SCALE(8 * 7 + i) * (z11 - z4));
    }
}

void ff_faandct248(DCTELEM *data)
{
    FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    FLOAT tmp10, tmp11, tmp12, tmp13;
    FLOAT temp[64];
    int i;

    emms_c();

    row_fdct(temp, data);

    for (i = 0; i < 8; i++)
    {
        tmp0 = temp[8 * 0 + i] + temp[8 * 1 + i];
        tmp1 = temp[8 * 2 + i] + temp[8 * 3 + i];
        tmp2 = temp[8 * 4 + i] + temp[8 * 5 + i];
        tmp3 = temp[8 * 6 + i] + temp[8 * 7 + i];
        tmp4 = temp[8 * 0 + i] - temp[8 * 1 + i];
        tmp5 = temp[8 * 2 + i] - temp[8 * 3 + i];
        tmp6 = temp[8 * 4 + i] - temp[8 * 5 + i];
        tmp7 = temp[8 * 6 + i] - temp[8 * 7 + i];

        tmp10 = tmp0 + tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        tmp13 = tmp0 - tmp3;

        data[8 * 0 + i] = lrintf(SCALE(8 * 0 + i) * (tmp10 + tmp11));
        data[8 * 4 + i] = lrintf(SCALE(8 * 4 + i) * (tmp10 - tmp11));

        tmp12 += tmp13;
        tmp12 = (float)(tmp12 * A1);
        data[8 * 2 + i] = lrintf(SCALE(8 * 2 + i) * (tmp13 + tmp12));
        data[8 * 6 + i] = lrintf(SCALE(8 * 6 + i) * (tmp13 - tmp12));

        tmp10 = tmp4 + tmp7;
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp5 - tmp6;
        tmp13 = tmp4 - tmp7;

        data[8 * 1 + i] = lrintf(SCALE(8 * 0 + i) * (tmp10 + tmp11));
        data[8 * 5 + i] = lrintf(SCALE(8 * 4 + i) * (tmp10 - tmp11));

        tmp12 += tmp13;
        tmp12 *= (float)(tmp12 * A1);
        data[8 * 3 + i] = lrintf(SCALE(8 * 2 + i) * (tmp13 + tmp12));
        data[8 * 7 + i] = lrintf(SCALE(8 * 6 + i) * (tmp13 - tmp12));
    }
}
