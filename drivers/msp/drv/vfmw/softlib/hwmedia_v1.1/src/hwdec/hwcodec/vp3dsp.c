/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQnrOvXqrMLPutyY70C/YiLiDRftuGTCTSM/M8D
U4z4mGWFUONqfK9yJYJVk0i7y6qlkyXvIj1ywn/34eu7EU3zeC82LHA2CFUq9OLQTsQaxIgZ
0YQrOq/toURJE5tszwgf0zxTr2JYYDZGE534jFhmn7bZGJGXmibSSgvhiAM+HQ==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * Copyright (C) 2004 the ffmpeg project
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
 * @file libavcodec/vp3dsp.c
 * Standard C DSP-oriented functions cribbed from the original VP3
 * source code.
 */

#include "avcodec.h"
#include "dsputil.h"

#define IdctAdjustBeforeShift 8
#define xC1S7 64277
#define xC2S6 60547
#define xC3S5 54491
#define xC4S4 46341
#define xC5S3 36410
#define xC6S2 25080
#define xC7S1 12785

#define M(a,b) (((a) * (b))>>16)

void idct_slow1_put(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int E, F;

    int i;

    E = M(xC4S4, (ip[0] ));
    F =  M(xC4S4, E) + 8 + 16 * 128;

    for ( i = 0; i < 8; i++)
    {
        dst[0] =
            dst[1] =
                dst[2] =
                    dst[3] =
                        dst[4] =
                            dst[5] =
                                dst[6] =
                                    dst[7] = cm[F >> 4];
        dst += stride;
    }
}


void idct_slow3_put(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, Ad, Bd, Cd, Dd, E, F;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 2; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] )
        {
            A = M(xC1S7, ip[1]);
            B = M(xC7S1, ip[1]);

            Ad = M(xC4S4, A);
            Bd = M(xC4S4, B);

            Cd = A ;
            Dd = B ;

            E = F = M(xC4S4, (ip[0] ));

            Ed = E;
            Gd = E;

            Add = F + Ad;
            Bdd = Bd;

            Fd = F - Ad;
            Hd = Bd;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[1 * 8] | ip[0 * 8]
           )
        {

            A = M(xC1S7, ip[1 * 8]);
            B = M(xC7S1, ip[1 * 8]);

            Ad = M(xC4S4, A);
            Bd = M(xC4S4, B);

            Cd = A;
            Dd = B;

            E = M(xC4S4, (ip[0 * 8])) + 8;
            F = M(xC4S4, (ip[0 * 8])) + 8;

            E += 16 * 128;
            F += 16 * 128;

            Ed = E ;
            Gd = E ;

            Add = F + Ad;
            Bdd = Bd;

            Fd = F - Ad;
            Hd = Bd;

            dst[0 * stride] = cm[(Gd + Cd )  >> 4];
            dst[7 * stride] = cm[(Gd - Cd )  >> 4];

            dst[1 * stride] = cm[(Add + Hd ) >> 4];
            dst[2 * stride] = cm[(Add - Hd ) >> 4];

            dst[3 * stride] = cm[(Ed + Dd )  >> 4];
            dst[4 * stride] = cm[(Ed - Dd )  >> 4];

            dst[5 * stride] = cm[(Fd + Bdd ) >> 4];
            dst[6 * stride] = cm[(Fd - Bdd ) >> 4];

        }
        else
        {

            dst[0 * stride] =
                dst[1 * stride] =
                    dst[2 * stride] =
                        dst[3 * stride] =
                            dst[4 * stride] =
                                dst[5 * stride] =
                                    dst[6 * stride] =
                                        dst[7 * stride] = 128;
        }

        ip++;            /* next column */
        dst++;
    }
}

void idct_slow10_put(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, C, D, Ad, Bd, Cd, Dd, E, F, G, H;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 4; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] | ip[2] | ip[3] )
        {
            A = M(xC1S7, ip[1]);
            B = M(xC7S1, ip[1]);
            C = M(xC3S5, ip[3]);
            D = M(xC5S3, ip[3]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B + D));

            Cd = A + C;
            Dd = B - D;

            E = F = M(xC4S4, ip[0]);

            G = M(xC2S6, ip[2]);
            H = M(xC6S2, ip[2]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[1 * 8] | ip[2 * 8] | ip[3 * 8] | ip[0 * 8]
           )
        {

            A = M(xC1S7, ip[1 * 8]);
            B = M(xC7S1, ip[1 * 8]);
            C = M(xC3S5, ip[3 * 8]);
            D = M(xC5S3, ip[3 * 8]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B + D));

            Cd = A + C;
            Dd = B - D;

            E = F = M(xC4S4, ip[0 * 8]) + 8;

            E += 16 * 128;
            F += 16 * 128;


            G = M(xC2S6, ip[2 * 8]);
            H = M(xC6S2, ip[2 * 8]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            dst[0 * stride] = cm[(Gd + Cd )  >> 4];
            dst[7 * stride] = cm[(Gd - Cd )  >> 4];

            dst[1 * stride] = cm[(Add + Hd ) >> 4];
            dst[2 * stride] = cm[(Add - Hd ) >> 4];

            dst[3 * stride] = cm[(Ed + Dd )  >> 4];
            dst[4 * stride] = cm[(Ed - Dd )  >> 4];

            dst[5 * stride] = cm[(Fd + Bdd ) >> 4];
            dst[6 * stride] = cm[(Fd - Bdd ) >> 4];

        }
        else
        {

            dst[0 * stride] =
                dst[1 * stride] =
                    dst[2 * stride] =
                        dst[3 * stride] =
                            dst[4 * stride] =
                                dst[5 * stride] =
                                    dst[6 * stride] =
                                        dst[7 * stride] = 128;
        }

        ip++;            /* next column */
        dst++;
    }
}

void idct_slow_put(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, C, D, Ad, Bd, Cd, Dd, E, F, G, H;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 8; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] | ip[2] | ip[3] | ip[4] | ip[5] | ip[6] | ip[7] )
        {
            A = M(xC1S7, ip[1]) + M(xC7S1, ip[7]);
            B = M(xC7S1, ip[1]) - M(xC1S7, ip[7]);
            C = M(xC3S5, ip[3]) + M(xC5S3, ip[5]);
            D = M(xC3S5, ip[5]) - M(xC5S3, ip[3]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0] + ip[4]));
            F = M(xC4S4, (ip[0] - ip[4]));

            G = M(xC2S6, ip[2]) + M(xC6S2, ip[6]);
            H = M(xC6S2, ip[2]) - M(xC2S6, ip[6]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[0 * 8] | ip[1 * 8] | ip[2 * 8] | ip[3 * 8] |
             ip[4 * 8] | ip[5 * 8] | ip[6 * 8] | ip[7 * 8] )
        {

            A = M(xC1S7, ip[1 * 8]) + M(xC7S1, ip[7 * 8]);
            B = M(xC7S1, ip[1 * 8]) - M(xC1S7, ip[7 * 8]);
            C = M(xC3S5, ip[3 * 8]) + M(xC5S3, ip[5 * 8]);
            D = M(xC3S5, ip[5 * 8]) - M(xC5S3, ip[3 * 8]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0 * 8] + ip[4 * 8])) + 8;
            F = M(xC4S4, (ip[0 * 8] - ip[4 * 8])) + 8;

            E += 16 * 128;
            F += 16 * 128;


            G = M(xC2S6, ip[2 * 8]) + M(xC6S2, ip[6 * 8]);
            H = M(xC6S2, ip[2 * 8]) - M(xC2S6, ip[6 * 8]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            dst[0 * stride] = cm[(Gd + Cd )  >> 4];
            dst[7 * stride] = cm[(Gd - Cd )  >> 4];

            dst[1 * stride] = cm[(Add + Hd ) >> 4];
            dst[2 * stride] = cm[(Add - Hd ) >> 4];

            dst[3 * stride] = cm[(Ed + Dd )  >> 4];
            dst[4 * stride] = cm[(Ed - Dd )  >> 4];

            dst[5 * stride] = cm[(Fd + Bdd ) >> 4];
            dst[6 * stride] = cm[(Fd - Bdd ) >> 4];

        }
        else
        {

            dst[0 * stride] =
                dst[1 * stride] =
                    dst[2 * stride] =
                        dst[3 * stride] =
                            dst[4 * stride] =
                                dst[5 * stride] =
                                    dst[6 * stride] =
                                        dst[7 * stride] = 128;
        }

        ip++;            /* next column */
        dst++;
    }
}

void idct_slow1_add(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int E, F;

    int i;

    E = M(xC4S4, (ip[0] ));
    F =  M(xC4S4, E) + 8;

    for ( i = 0; i < 8; i++)
    {
        dst[0] = cm[dst[0] + (F >> 4)];
        dst[1] = cm[dst[1] + (F >> 4)];
        dst[2] = cm[dst[2] + (F >> 4)];
        dst[3] = cm[dst[3] + (F >> 4)];
        dst[4] = cm[dst[4] + (F >> 4)];
        dst[5] = cm[dst[5] + (F >> 4)];
        dst[6] = cm[dst[6] + (F >> 4)];
        dst[7] = cm[dst[7] + (F >> 4)];
        dst += stride;
    }
}

void idct_slow3_add(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, Ad, Bd, Cd, Dd, E, F;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 2; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] )
        {
            A = M(xC1S7, ip[1]);
            B = M(xC7S1, ip[1]);

            Ad = M(xC4S4, A);
            Bd = M(xC4S4, B);

            Cd = A;
            Dd = B;

            E = M(xC4S4, ip[0]);
            F = M(xC4S4, ip[0]);

            Ed = E;
            Gd = E;

            Add = F + Ad;
            Bdd = Bd;

            Fd = F - Ad;
            Hd = Bd;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[1 * 8] | ip[0 * 8])
        {

            A = M(xC1S7, ip[1 * 8]);
            B = M(xC7S1, ip[1 * 8]);

            Ad = M(xC4S4, A);
            Bd = M(xC4S4, B);

            Cd = A ;
            Dd = B ;

            E = M(xC4S4, ip[0 * 8]) + 8;
            F = M(xC4S4, ip[0 * 8]) + 8;

            Ed = E;
            Gd = E;

            Add = F + Ad;
            Bdd = Bd;

            Fd = F - Ad;
            Hd = Bd;

            dst[0 * stride] = cm[dst[0 * stride] + ((Gd + Cd )  >> 4)];
            dst[7 * stride] = cm[dst[7 * stride] + ((Gd - Cd )  >> 4)];

            dst[1 * stride] = cm[dst[1 * stride] + ((Add + Hd ) >> 4)];
            dst[2 * stride] = cm[dst[2 * stride] + ((Add - Hd ) >> 4)];

            dst[3 * stride] = cm[dst[3 * stride] + ((Ed + Dd )  >> 4)];
            dst[4 * stride] = cm[dst[4 * stride] + ((Ed - Dd )  >> 4)];

            dst[5 * stride] = cm[dst[5 * stride] + ((Fd + Bdd ) >> 4)];
            dst[6 * stride] = cm[dst[6 * stride] + ((Fd - Bdd ) >> 4)];

        }

        ip++;            /* next column */
        dst++;
    }
}

void idct_slow10_add(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, C, D, Ad, Bd, Cd, Dd, E, F, G, H;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 4; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] | ip[2] | ip[3] )
        {
            A = M(xC1S7, ip[1]);
            B = M(xC7S1, ip[1]);
            C = M(xC3S5, ip[3]);
            D = M(xC5S3, ip[3]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B + D));

            Cd = A + C;
            Dd = B - D;

            E = F = M(xC4S4, ip[0]);

            G = M(xC2S6, ip[2]);
            H = M(xC6S2, ip[2]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[1 * 8] | ip[2 * 8] | ip[3 * 8] | ip[0 * 8])
        {

            A = M(xC1S7, ip[1 * 8]);
            B = M(xC7S1, ip[1 * 8]);
            C = M(xC3S5, ip[3 * 8]);
            D = M(xC5S3, ip[3 * 8]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B + D));

            Cd = A + C;
            Dd = B - D;

            E = F = M(xC4S4, ip[0 * 8]) + 8;

            G = M(xC2S6, ip[2 * 8]);
            H = M(xC6S2, ip[2 * 8]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            dst[0 * stride] = cm[dst[0 * stride] + ((Gd + Cd )  >> 4)];
            dst[7 * stride] = cm[dst[7 * stride] + ((Gd - Cd )  >> 4)];

            dst[1 * stride] = cm[dst[1 * stride] + ((Add + Hd ) >> 4)];
            dst[2 * stride] = cm[dst[2 * stride] + ((Add - Hd ) >> 4)];

            dst[3 * stride] = cm[dst[3 * stride] + ((Ed + Dd )  >> 4)];
            dst[4 * stride] = cm[dst[4 * stride] + ((Ed - Dd )  >> 4)];

            dst[5 * stride] = cm[dst[5 * stride] + ((Fd + Bdd ) >> 4)];
            dst[6 * stride] = cm[dst[6 * stride] + ((Fd - Bdd ) >> 4)];

        }

        ip++;            /* next column */
        dst++;
    }
}

void idct_slow_add(uint8_t *dst, int stride, int16_t *input)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, C, D, Ad, Bd, Cd, Dd, E, F, G, H;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 8; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] | ip[2] | ip[3] | ip[4] | ip[5] | ip[6] | ip[7] )
        {
            A = M(xC1S7, ip[1]) + M(xC7S1, ip[7]);
            B = M(xC7S1, ip[1]) - M(xC1S7, ip[7]);
            C = M(xC3S5, ip[3]) + M(xC5S3, ip[5]);
            D = M(xC3S5, ip[5]) - M(xC5S3, ip[3]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0] + ip[4]));
            F = M(xC4S4, (ip[0] - ip[4]));

            G = M(xC2S6, ip[2]) + M(xC6S2, ip[6]);
            H = M(xC6S2, ip[2]) - M(xC2S6, ip[6]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if (ip[0 * 8] |  ip[1 * 8] | ip[2 * 8] | ip[3 * 8] |
            ip[4 * 8] | ip[5 * 8] | ip[6 * 8] | ip[7 * 8] )
        {

            A = M(xC1S7, ip[1 * 8]) + M(xC7S1, ip[7 * 8]);
            B = M(xC7S1, ip[1 * 8]) - M(xC1S7, ip[7 * 8]);
            C = M(xC3S5, ip[3 * 8]) + M(xC5S3, ip[5 * 8]);
            D = M(xC3S5, ip[5 * 8]) - M(xC5S3, ip[3 * 8]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0 * 8] + ip[4 * 8])) + 8;
            F = M(xC4S4, (ip[0 * 8] - ip[4 * 8])) + 8;

            G = M(xC2S6, ip[2 * 8]) + M(xC6S2, ip[6 * 8]);
            H = M(xC6S2, ip[2 * 8]) - M(xC2S6, ip[6 * 8]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;


            dst[0 * stride] = cm[dst[0 * stride] + ((Gd + Cd )  >> 4)];
            dst[7 * stride] = cm[dst[7 * stride] + ((Gd - Cd )  >> 4)];

            dst[1 * stride] = cm[dst[1 * stride] + ((Add + Hd ) >> 4)];
            dst[2 * stride] = cm[dst[2 * stride] + ((Add - Hd ) >> 4)];

            dst[3 * stride] = cm[dst[3 * stride] + ((Ed + Dd )  >> 4)];
            dst[4 * stride] = cm[dst[4 * stride] + ((Ed - Dd )  >> 4)];

            dst[5 * stride] = cm[dst[5 * stride] + ((Fd + Bdd ) >> 4)];
            dst[6 * stride] = cm[dst[6 * stride] + ((Fd - Bdd ) >> 4)];

        }

        ip++;            /* next column */
        dst++;
    }
}

static av_always_inline void idct(uint8_t *dst, int stride, int16_t *input, int type)
{
    int16_t *ip = input;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int A, B, C, D, Ad, Bd, Cd, Dd, E, F, G, H;
    int Ed, Gd, Add, Bdd, Fd, Hd;

    int i;

    /* Inverse DCT on the rows now */
    for (i = 0; i < 8; i++)
    {
        /* Check for non-zero values */
        if ( ip[0] | ip[1] | ip[2] | ip[3] | ip[4] | ip[5] | ip[6] | ip[7] )
        {
            A = M(xC1S7, ip[1]) + M(xC7S1, ip[7]);
            B = M(xC7S1, ip[1]) - M(xC1S7, ip[7]);
            C = M(xC3S5, ip[3]) + M(xC5S3, ip[5]);
            D = M(xC3S5, ip[5]) - M(xC5S3, ip[3]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0] + ip[4]));
            F = M(xC4S4, (ip[0] - ip[4]));

            G = M(xC2S6, ip[2]) + M(xC6S2, ip[6]);
            H = M(xC6S2, ip[2]) - M(xC2S6, ip[6]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /*  Final sequence of operations over-write original inputs. */
            ip[0] = Gd + Cd ;
            ip[7] = Gd - Cd ;

            ip[1] = Add + Hd;
            ip[2] = Add - Hd;

            ip[3] = Ed + Dd ;
            ip[4] = Ed - Dd ;

            ip[5] = Fd + Bdd;
            ip[6] = Fd - Bdd;
        }

        ip += 8;            /* next row */
    }

    ip = input;

    for ( i = 0; i < 8; i++)
    {
        /* Check for non-zero values (bitwise or faster than ||) */
        if ( ip[1 * 8] | ip[2 * 8] | ip[3 * 8] |
             ip[4 * 8] | ip[5 * 8] | ip[6 * 8] | ip[7 * 8] )
        {

            A = M(xC1S7, ip[1 * 8]) + M(xC7S1, ip[7 * 8]);
            B = M(xC7S1, ip[1 * 8]) - M(xC1S7, ip[7 * 8]);
            C = M(xC3S5, ip[3 * 8]) + M(xC5S3, ip[5 * 8]);
            D = M(xC3S5, ip[5 * 8]) - M(xC5S3, ip[3 * 8]);

            Ad = M(xC4S4, (A - C));
            Bd = M(xC4S4, (B - D));

            Cd = A + C;
            Dd = B + D;

            E = M(xC4S4, (ip[0 * 8] + ip[4 * 8])) + 8;
            F = M(xC4S4, (ip[0 * 8] - ip[4 * 8])) + 8;

            if (type == 1) //HACK
            {
                E += 16 * 128;
                F += 16 * 128;
            }

            G = M(xC2S6, ip[2 * 8]) + M(xC6S2, ip[6 * 8]);
            H = M(xC6S2, ip[2 * 8]) - M(xC2S6, ip[6 * 8]);

            Ed = E - G;
            Gd = E + G;

            Add = F + Ad;
            Bdd = Bd - H;

            Fd = F - Ad;
            Hd = Bd + H;

            /* Final sequence of operations over-write original inputs. */
            if (type == 0)
            {
                ip[0 * 8] = (Gd + Cd )  >> 4;
                ip[7 * 8] = (Gd - Cd )  >> 4;

                ip[1 * 8] = (Add + Hd ) >> 4;
                ip[2 * 8] = (Add - Hd ) >> 4;

                ip[3 * 8] = (Ed + Dd )  >> 4;
                ip[4 * 8] = (Ed - Dd )  >> 4;

                ip[5 * 8] = (Fd + Bdd ) >> 4;
                ip[6 * 8] = (Fd - Bdd ) >> 4;
            }
            else if (type == 1)
            {
                dst[0 * stride] = cm[(Gd + Cd )  >> 4];
                dst[7 * stride] = cm[(Gd - Cd )  >> 4];

                dst[1 * stride] = cm[(Add + Hd ) >> 4];
                dst[2 * stride] = cm[(Add - Hd ) >> 4];

                dst[3 * stride] = cm[(Ed + Dd )  >> 4];
                dst[4 * stride] = cm[(Ed - Dd )  >> 4];

                dst[5 * stride] = cm[(Fd + Bdd ) >> 4];
                dst[6 * stride] = cm[(Fd - Bdd ) >> 4];
            }
            else
            {
                dst[0 * stride] = cm[dst[0 * stride] + ((Gd + Cd )  >> 4)];
                dst[7 * stride] = cm[dst[7 * stride] + ((Gd - Cd )  >> 4)];

                dst[1 * stride] = cm[dst[1 * stride] + ((Add + Hd ) >> 4)];
                dst[2 * stride] = cm[dst[2 * stride] + ((Add - Hd ) >> 4)];

                dst[3 * stride] = cm[dst[3 * stride] + ((Ed + Dd )  >> 4)];
                dst[4 * stride] = cm[dst[4 * stride] + ((Ed - Dd )  >> 4)];

                dst[5 * stride] = cm[dst[5 * stride] + ((Fd + Bdd ) >> 4)];
                dst[6 * stride] = cm[dst[6 * stride] + ((Fd - Bdd ) >> 4)];
            }

        }
        else
        {
            if (type == 0)
            {
                ip[0 * 8] =
                    ip[1 * 8] =
                        ip[2 * 8] =
                            ip[3 * 8] =
                                ip[4 * 8] =
                                    ip[5 * 8] =
                                        ip[6 * 8] =
                                            ip[7 * 8] = ((xC4S4 * ip[0 * 8] + (IdctAdjustBeforeShift << 16)) >> 20);
            }
            else if (type == 1)
            {
                dst[0 * stride] =
                    dst[1 * stride] =
                        dst[2 * stride] =
                            dst[3 * stride] =
                                dst[4 * stride] =
                                    dst[5 * stride] =
                                        dst[6 * stride] =
                                            dst[7 * stride] = cm[128 + ((xC4S4 * ip[0 * 8] + (IdctAdjustBeforeShift << 16)) >> 20)];
            }
            else
            {
                if (ip[0 * 8])
                {
                    int v = ((xC4S4 * ip[0 * 8] + (IdctAdjustBeforeShift << 16)) >> 20);
                    dst[0 * stride] = cm[dst[0 * stride] + v];
                    dst[1 * stride] = cm[dst[1 * stride] + v];
                    dst[2 * stride] = cm[dst[2 * stride] + v];
                    dst[3 * stride] = cm[dst[3 * stride] + v];
                    dst[4 * stride] = cm[dst[4 * stride] + v];
                    dst[5 * stride] = cm[dst[5 * stride] + v];
                    dst[6 * stride] = cm[dst[6 * stride] + v];
                    dst[7 * stride] = cm[dst[7 * stride] + v];
                }
            }
        }

        ip++;            /* next column */
        dst++;
    }
}

void ff_vp3_idct_c(DCTELEM *block/* align 16*/)
{
    idct(NULL, 0, block, 0);
}

void ff_vp3_idct_put_c(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/)
{
    idct(dest, line_size, block, 1);
}

void ff_vp3_idct_add_c(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/)
{
    idct(dest, line_size, block, 2);
}

void ff_vp3_v_loop_filter_c(uint8_t *first_pixel, int stride, int *bounding_values)
{
    unsigned char *end;
    int filter_value;
    const int nstride = -stride;

    for (end = first_pixel + 8; first_pixel < end; first_pixel++)
    {
        filter_value =
            (first_pixel[2 * nstride] - first_pixel[ stride])
            + 3 * (first_pixel[0          ] - first_pixel[nstride]);
        filter_value = bounding_values[(filter_value + 4) >> 3];
        first_pixel[nstride] = av_clip_uint8(first_pixel[nstride] + filter_value);
        first_pixel[0] = av_clip_uint8(first_pixel[0] - filter_value);
    }
}

void ff_vp3_h_loop_filter_c(uint8_t *first_pixel, int stride, int *bounding_values)
{
    unsigned char *end;
    int filter_value;

    for (end = first_pixel + 8 * stride; first_pixel != end; first_pixel += stride)
    {
        filter_value =
            (first_pixel[-2] - first_pixel[ 1])
            + 3 * (first_pixel[ 0] - first_pixel[-1]);
        filter_value = bounding_values[(filter_value + 4) >> 3];
        first_pixel[-1] = av_clip_uint8(first_pixel[-1] + filter_value);
        first_pixel[ 0] = av_clip_uint8(first_pixel[ 0] - filter_value);
    }
}
