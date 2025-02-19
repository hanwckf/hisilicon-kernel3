/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroSuky1G/o7YV+/j21G9XCUnh+APYIFyfBpvEmtC
cIXMaEqxH6ZsrJuD8Ud3gSa3/NWL0XNJh+dN9pk4X9IGuZ2VM+LQNZklMGfyNyoBa2JPRLA3
f54qlajRL62TwBw7VR54OKwWK4jN1tGD2hWrbSJ3UJNJuVQVqZb2vxR30hP6fA==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * VC-1 and WMV3 decoder
 * Copyright (c) 2006-2007 Konstantin Shishkov
 * Partly based on vc9.c (c) 2005 Anonymous, Alex Beregszaszi, Michael Niedermayer
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
 * @file libavcodec/vc1.c
 * VC-1 and WMV3 decoder
 *
 */
#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "vc1.h"
#include "vc1data.h"
#include "vc1acdata.h"
#include "msmpeg4data.h"
#include "unary.h"
#include "simple_idct.h"
#include "mathops.h"
#include "vdpau_internal.h"
#include "internal.h"

#undef NDEBUG
#include <assert.h>

#define MB_INTRA_VLC_BITS 9
#define DC_VLC_BITS 9
#define AC_VLC_BITS 9

/* 2010/06/18 19:00:00 liuxw+00139685 */
/* 新增加的宏 */
#define MAX_PIC_HEIGHT 1088
#define MIN_PIC_HEIGHT 16
#define MAX_PIC_WIDTH  1920
#define MIN_PIC_WIDTH  32

//#define LXW_DEBUG
#ifdef LXW_DEBUG
#define MB_X 117
#define MB_Y 24
#define MB_Y2 MB_Y
#define TARGET_FRAME 116
static int frame_num = 0;
static int output_num = 0;
#endif

static const uint16_t table_mb_intra[64][2];

/* 2010/05/31 17:00:00 liuxw+00139685 */
/* 将vc1_parser.c中新增的函数的声明加到这里，原因：没有vc1_parser.h头文件 */
int VC1_Frame_Parse(AVCodecContext *pstAVCodecContext, const uint8_t **pucOutBuf, unsigned int *puiOutBufSize, const uint8_t *pucInBuf, unsigned int uiInBufSize, int *piLength);
//int msmpeg4_vlc_table_free();
/**
 * Init VC-1 specific tables and VC1Context members
 * @param v The VC1Context to initialize
 * @return Status
 */
/* 2010/08/31 11:30:00 liuxw+00139685 [AZ1D02297] */
/* 将动态分配的空间全部转换为表态组 */
#if 0
static int vc1_init_common(VC1Context *v)
{
    /* 2010/08/13 11:30:00 liuxw+00139685 [AZ1D02265] */
    /* 由于在复位的时候有些初始化的函数的局部变量无法复位，所以修改为通道变量中的成员 */
    // static int done = 0;
    int i = 0;
    /* 2010/06/18 17:30:00 liuxw+00139685 */
    /* 增加返回码 */
    int iRet = 0;

    v->hrd_rate = v->hrd_buffer = NULL;

    /* VLC tables */
    /* 2010/08/13 11:30:00 liuxw+00139685 [AZ1D02265] */
    /* 由于在复位的时候有些初始化的函数的局部变量无法复位，所以修改为通道变量中的成员 */
    /*  if(!done)
      {
          done = 1; */
    if (!v->done)
    {
        v->done = 1;

        iRet = init_vlc(&ff_vc1_bfraction_vlc, VC1_BFRACTION_VLC_BITS, 23,
                        ff_vc1_bfraction_bits, 1, 1,
                        ff_vc1_bfraction_codes, 1, 1, 1);

        if (0 > iRet)
        {
            av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_bfraction_vlc table failed!\n");
            return -1;
        }

        iRet = init_vlc(&ff_vc1_norm2_vlc, VC1_NORM2_VLC_BITS, 4,
                        ff_vc1_norm2_bits, 1, 1,
                        ff_vc1_norm2_codes, 1, 1, 1);

        if (0 > iRet)
        {
            av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_norm2_vlc table failed!\n");
            return -1;
        }

        iRet = init_vlc(&ff_vc1_norm6_vlc, VC1_NORM6_VLC_BITS, 64,
                        ff_vc1_norm6_bits, 1, 1,
                        ff_vc1_norm6_codes, 2, 2, 1);

        if (0 > iRet)
        {
            av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_norm6_vlc table failed!\n");
            return -1;
        }

        iRet = init_vlc(&ff_vc1_imode_vlc, VC1_IMODE_VLC_BITS, 7,
                        ff_vc1_imode_bits, 1, 1,
                        ff_vc1_imode_codes, 1, 1, 1);

        if (0 > iRet)
        {
            av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_imode_vlc table failed!\n");
            return -1;
        }

        for (i = 0; i < 3; i++)
        {
            iRet = init_vlc(&ff_vc1_ttmb_vlc[i], VC1_TTMB_VLC_BITS, 16,
                            ff_vc1_ttmb_bits[i], 1, 1,
                            ff_vc1_ttmb_codes[i], 2, 2, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_ttmb_vlc[%d] table failed!\n", i);
                return -1;
            }

            iRet = init_vlc(&ff_vc1_ttblk_vlc[i], VC1_TTBLK_VLC_BITS, 8,
                            ff_vc1_ttblk_bits[i], 1, 1,
                            ff_vc1_ttblk_codes[i], 1, 1, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_ttblk_vlc[%d] table failed!\n", i);
                return -1;
            }

            iRet = init_vlc(&ff_vc1_subblkpat_vlc[i], VC1_SUBBLKPAT_VLC_BITS, 15,
                            ff_vc1_subblkpat_bits[i], 1, 1,
                            ff_vc1_subblkpat_codes[i], 1, 1, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_subblkpat_vlc[%d] table failed!\n", i);
                return -1;
            }
        }

        for (i = 0; i < 4; i++)
        {
            iRet = init_vlc(&ff_vc1_4mv_block_pattern_vlc[i], VC1_4MV_BLOCK_PATTERN_VLC_BITS, 16,
                            ff_vc1_4mv_block_pattern_bits[i], 1, 1,
                            ff_vc1_4mv_block_pattern_codes[i], 1, 1, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_4mv_block_pattern_vlc[%d] table failed!\n", i);
                return -1;
            }

            iRet = init_vlc(&ff_vc1_cbpcy_p_vlc[i], VC1_CBPCY_P_VLC_BITS, 64,
                            ff_vc1_cbpcy_p_bits[i], 1, 1,
                            ff_vc1_cbpcy_p_codes[i], 2, 2, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_cbpcy_p_vlc[%d] table failed!\n", i);
                return -1;
            }

            iRet = init_vlc(&ff_vc1_mv_diff_vlc[i], VC1_MV_DIFF_VLC_BITS, 73,
                            ff_vc1_mv_diff_bits[i], 1, 1,
                            ff_vc1_mv_diff_codes[i], 2, 2, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_mv_diff_vlc[%d] table failed!\n", i);
                return -1;
            }
        }

        for (i = 0; i < 8; i++)
        {
            iRet = init_vlc(&ff_vc1_ac_coeff_table[i], AC_VLC_BITS, vc1_ac_sizes[i], &vc1_ac_tables[i][0][1], 8, 4, &vc1_ac_tables[i][0][0], 8, 4, 1);

            if (0 > iRet)
            {
                av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_vc1_ac_coeff_table[%d] table failed!\n", i);
                return -1;
            }
        }

        iRet = init_vlc(&ff_msmp4_mb_i_vlc, MB_INTRA_VLC_BITS, 64, &ff_msmp4_mb_i_table[0][1], 4, 2, &ff_msmp4_mb_i_table[0][0], 4, 2, 1);

        if (0 > iRet)
        {
            av_log(v->s.avctx, AV_LOG_ERROR, "Malloc memory for ff_msmp4_mb_i_table table failed!\n");
            return -1;
        }
    }

    /* Other defaults */
    v->pq = -1;
    v->mvrange = 0; /* 7.1.1.18, p80 */

    return 0;
}
#else

static const uint16_t vlc_offs[] =
{
    0,   520,   552,   616,  1128,  1160, 1224, 1740, 1772, 1836, 1900, 2436,
    2986,  3050,  3610,  4154,  4218,  4746, 5326, 5390, 5902, 6554, 7658, 8620,
    9262, 10202, 10756, 11310, 12228, 15078
};

/**
 * Init VC-1 specific tables and VC1Context members
 * @param v The VC1Context to initialize
 * @return Status
 */
static int vc1_init_common(VC1Context *v)
{
    static int done = 0;
    int i = 0;
    static VLC_TYPE vlc_table[15078][2];

    v->hrd_rate = v->hrd_buffer = NULL;

    /* VLC tables */
    if (!done)
    {
        INIT_VLC_STATIC(&ff_vc1_bfraction_vlc, VC1_BFRACTION_VLC_BITS, 23,
                        ff_vc1_bfraction_bits, 1, 1,
                        ff_vc1_bfraction_codes, 1, 1, 1 << VC1_BFRACTION_VLC_BITS);
        INIT_VLC_STATIC(&ff_vc1_norm2_vlc, VC1_NORM2_VLC_BITS, 4,
                        ff_vc1_norm2_bits, 1, 1,
                        ff_vc1_norm2_codes, 1, 1, 1 << VC1_NORM2_VLC_BITS);
        INIT_VLC_STATIC(&ff_vc1_norm6_vlc, VC1_NORM6_VLC_BITS, 64,
                        ff_vc1_norm6_bits, 1, 1,
                        ff_vc1_norm6_codes, 2, 2, 556);
        INIT_VLC_STATIC(&ff_vc1_imode_vlc, VC1_IMODE_VLC_BITS, 7,
                        ff_vc1_imode_bits, 1, 1,
                        ff_vc1_imode_codes, 1, 1, 1 << VC1_IMODE_VLC_BITS);

        for (i = 0; i < 3; i++)
        {
            ff_vc1_ttmb_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 0]];
            ff_vc1_ttmb_vlc[i].table_allocated = vlc_offs[i * 3 + 1] - vlc_offs[i * 3 + 0];
            init_vlc(&ff_vc1_ttmb_vlc[i], VC1_TTMB_VLC_BITS, 16,
                     ff_vc1_ttmb_bits[i], 1, 1,
                     ff_vc1_ttmb_codes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
            ff_vc1_ttblk_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 1]];
            ff_vc1_ttblk_vlc[i].table_allocated = vlc_offs[i * 3 + 2] - vlc_offs[i * 3 + 1];
            init_vlc(&ff_vc1_ttblk_vlc[i], VC1_TTBLK_VLC_BITS, 8,
                     ff_vc1_ttblk_bits[i], 1, 1,
                     ff_vc1_ttblk_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
            ff_vc1_subblkpat_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 2]];
            ff_vc1_subblkpat_vlc[i].table_allocated = vlc_offs[i * 3 + 3] - vlc_offs[i * 3 + 2];
            init_vlc(&ff_vc1_subblkpat_vlc[i], VC1_SUBBLKPAT_VLC_BITS, 15,
                     ff_vc1_subblkpat_bits[i], 1, 1,
                     ff_vc1_subblkpat_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
        }

        for (i = 0; i < 4; i++)
        {
            ff_vc1_4mv_block_pattern_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 9]];
            ff_vc1_4mv_block_pattern_vlc[i].table_allocated = vlc_offs[i * 3 + 10] - vlc_offs[i * 3 + 9];
            init_vlc(&ff_vc1_4mv_block_pattern_vlc[i], VC1_4MV_BLOCK_PATTERN_VLC_BITS, 16,
                     ff_vc1_4mv_block_pattern_bits[i], 1, 1,
                     ff_vc1_4mv_block_pattern_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
            ff_vc1_cbpcy_p_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 10]];
            ff_vc1_cbpcy_p_vlc[i].table_allocated = vlc_offs[i * 3 + 11] - vlc_offs[i * 3 + 10];
            init_vlc(&ff_vc1_cbpcy_p_vlc[i], VC1_CBPCY_P_VLC_BITS, 64,
                     ff_vc1_cbpcy_p_bits[i], 1, 1,
                     ff_vc1_cbpcy_p_codes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
            ff_vc1_mv_diff_vlc[i].table = &vlc_table[vlc_offs[i * 3 + 11]];
            ff_vc1_mv_diff_vlc[i].table_allocated = vlc_offs[i * 3 + 12] - vlc_offs[i * 3 + 11];
            init_vlc(&ff_vc1_mv_diff_vlc[i], VC1_MV_DIFF_VLC_BITS, 73,
                     ff_vc1_mv_diff_bits[i], 1, 1,
                     ff_vc1_mv_diff_codes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
        }

        for (i = 0; i < 8; i++)
        {
            ff_vc1_ac_coeff_table[i].table = &vlc_table[vlc_offs[i + 21]];
            ff_vc1_ac_coeff_table[i].table_allocated = vlc_offs[i + 22] - vlc_offs[i + 21];
            init_vlc(&ff_vc1_ac_coeff_table[i], AC_VLC_BITS, vc1_ac_sizes[i],
                     &vc1_ac_tables[i][0][1], 8, 4,
                     &vc1_ac_tables[i][0][0], 8, 4, INIT_VLC_USE_NEW_STATIC);
        }

        done = 1;
    }

    /* Other defaults */
    v->pq = -1;
    v->mvrange = 0; /* 7.1.1.18, p80 */

    return 0;
}
#endif

/* 2010/08/31 11:30:00 liuxw+00139685 [AZ1D02297] */
/* 由于上面将动态分配的VLC空间全部转换为静态数组，所以无需内存释放 */
#if 0
/* 2010/06/18 15:30:00 liuxw+00139685 */
/* 新增函数：释放VC1所用到的VLC表所占用的内存 */
/* 2010/08/18 15:30:00 liuxw+00139685 [AZ1D02265] */
/* 对于每一个释放后的vlc_table，对管理其table的全局变量进行清0处理 */
static int vc1_vlc_talbe_free()
{
    int i;

    av_freep(&ff_vc1_bfraction_vlc.table);
    memset(&ff_vc1_bfraction_vlc, 0, sizeof(VLC));
    av_freep(&ff_vc1_norm2_vlc.table);
    memset(&ff_vc1_norm2_vlc, 0, sizeof(VLC));
    av_freep(&ff_vc1_norm6_vlc.table);
    memset(&ff_vc1_norm6_vlc, 0, sizeof(VLC));
    av_freep(&ff_vc1_imode_vlc.table);
    memset(&ff_vc1_imode_vlc, 0, sizeof(VLC));

    for (i = 0; i < 3; i++)
    {
        av_freep(&ff_vc1_ttmb_vlc[i].table);
        memset(&ff_vc1_ttmb_vlc[i], 0, sizeof(VLC));
        av_freep(&ff_vc1_ttblk_vlc[i].table);
        memset(&ff_vc1_ttblk_vlc[i], 0, sizeof(VLC));
        av_freep(&ff_vc1_subblkpat_vlc[i].table);
        memset(&ff_vc1_subblkpat_vlc[i], 0, sizeof(VLC));
    }

    for (i = 0; i < 4; i++)
    {
        av_freep(&ff_vc1_4mv_block_pattern_vlc[i].table);
        memset(&ff_vc1_4mv_block_pattern_vlc[i], 0, sizeof(VLC));
        av_freep(&ff_vc1_cbpcy_p_vlc[i].table);
        memset(&ff_vc1_cbpcy_p_vlc[i], 0, sizeof(VLC));
        av_freep(&ff_vc1_mv_diff_vlc[i].table);
        memset(&ff_vc1_mv_diff_vlc[i], 0, sizeof(VLC));
    }

    for (i = 0; i < 8; i++)
    {
        av_freep(&ff_vc1_ac_coeff_table[i].table);
        memset(&ff_vc1_ac_coeff_table[i], 0, sizeof(VLC));
    }

    av_freep(&ff_msmp4_mb_i_vlc.table);
    memset(&ff_msmp4_mb_i_vlc, 0, sizeof(VLC));

    return 0;
}
#endif

/***********************************************************************/
/**
 * @defgroup vc1bitplane VC-1 Bitplane decoding
 * @see 8.7, p56
 * @{
 */

/**
 * Imode types
 * @{
 */
enum Imode
{
    IMODE_RAW,
    IMODE_NORM2,
    IMODE_DIFF2,
    IMODE_NORM6,
    IMODE_DIFF6,
    IMODE_ROWSKIP,
    IMODE_COLSKIP
};
/** @} */ //imode defines

/** Decode rows by checking if they are skipped
 * @param plane Buffer to store decoded bits
 * @param[in] width Width of this buffer
 * @param[in] height Height of this buffer
 * @param[in] stride of this buffer
 */
static void decode_rowskip(uint8_t *plane, int width, int height, int stride, GetBitContext *gb)
{
    int x, y;

    for (y = 0; y < height; y++)
    {
        if (!get_bits1(gb)) //rowskip
        { memset(plane, 0, width); }
        else
            for (x = 0; x < width; x++)
            { plane[x] = get_bits1(gb); }

        plane += stride;
    }
}

/** Decode columns by checking if they are skipped
 * @param plane Buffer to store decoded bits
 * @param[in] width Width of this buffer
 * @param[in] height Height of this buffer
 * @param[in] stride of this buffer
 * @todo FIXME: Optimize
 */
static void decode_colskip(uint8_t *plane, int width, int height, int stride, GetBitContext *gb)
{
    int x, y;

    for (x = 0; x < width; x++)
    {
        if (!get_bits1(gb)) //colskip
            for (y = 0; y < height; y++)
            { plane[y * stride] = 0; }
        else
            for (y = 0; y < height; y++)
            { plane[y * stride] = get_bits1(gb); }

        plane ++;
    }
}

/** Decode a bitplane's bits
 * @param data bitplane where to store the decode bits
 * @param[out] raw_flag pointer to the flag indicating that this bitplane is not coded explicitly
 * @param v VC-1 context for bit reading and logging
 * @return Status
 * @todo FIXME: Optimize
 */
static int bitplane_decoding(uint8_t *data, int *raw_flag, VC1Context *v)
{
    GetBitContext *gb = &v->s.gb;

    int imode, x, y, code, offset;
    uint8_t invert, *planep = data;
    int width, height, stride;

    width = v->s.mb_width;
    height = v->s.mb_height;
    stride = v->s.mb_stride;
    invert = get_bits1(gb);
    imode = get_vlc2(gb, ff_vc1_imode_vlc.table, VC1_IMODE_VLC_BITS, 1);

    *raw_flag = 0;

    switch (imode)
    {
        case IMODE_RAW:
            //Data is actually read in the MB layer (same for all tests == "raw")
            *raw_flag = 1; //invert ignored
            return invert;

        case IMODE_DIFF2:
        case IMODE_NORM2:
            if ((height * width) & 1)
            {
                *planep++ = get_bits1(gb);
                offset = 1;
            }
            else { offset = 0; }

            // decode bitplane as one long line
            for (y = offset; y < height * width; y += 2)
            {
                code = get_vlc2(gb, ff_vc1_norm2_vlc.table, VC1_NORM2_VLC_BITS, 1);
                *planep++ = code & 1;
                offset++;

                if (offset == width)
                {
                    offset = 0;
                    planep += stride - width;
                }

                *planep++ = code >> 1;
                offset++;

                if (offset == width)
                {
                    offset = 0;
                    planep += stride - width;
                }
            }

            break;

        case IMODE_DIFF6:
        case IMODE_NORM6:
            if (!(height % 3) && (width % 3)) // use 2x3 decoding
            {
                for (y = 0; y < height; y += 3)
                {
                    for (x = width & 1; x < width; x += 2)
                    {
                        code = get_vlc2(gb, ff_vc1_norm6_vlc.table, VC1_NORM6_VLC_BITS, 2);

                        if (code < 0)
                        {
                            /* 2010/06/23 10:30:00 liuxw+00139685 */
                            /* 修改错误级别 */
                            //						av_log(v->s.avctx, AV_LOG_DEBUG, "invalid NORM-6 VLC\n");
                            av_log(v->s.avctx, AV_LOG_WARNING, "invalid NORM-6 VLC\n");
                            return -1;
                        }

                        planep[x + 0] = (code >> 0) & 1;
                        planep[x + 1] = (code >> 1) & 1;
                        planep[x + 0 + stride] = (code >> 2) & 1;
                        planep[x + 1 + stride] = (code >> 3) & 1;
                        planep[x + 0 + stride * 2] = (code >> 4) & 1;
                        planep[x + 1 + stride * 2] = (code >> 5) & 1;
                    }

                    planep += stride * 3;
                }

                if (width & 1)
                { decode_colskip(data, 1, height, stride, &v->s.gb); }
            }
            else // 3x2
            {
                planep += (height & 1) * stride;

                for (y = height & 1; y < height; y += 2)
                {
                    for (x = width % 3; x < width; x += 3)
                    {
                        code = get_vlc2(gb, ff_vc1_norm6_vlc.table, VC1_NORM6_VLC_BITS, 2);

                        if (code < 0)
                        {
                            /* 2010/06/23 10:30:00 liuxw+00139685 */
                            /* 修改错误级别 */
                            //						av_log(v->s.avctx, AV_LOG_DEBUG, "invalid NORM-6 VLC\n");
                            av_log(v->s.avctx, AV_LOG_WARNING, "invalid NORM-6 VLC\n");
                            return -1;
                        }

                        planep[x + 0] = (code >> 0) & 1;
                        planep[x + 1] = (code >> 1) & 1;
                        planep[x + 2] = (code >> 2) & 1;
                        planep[x + 0 + stride] = (code >> 3) & 1;
                        planep[x + 1 + stride] = (code >> 4) & 1;
                        planep[x + 2 + stride] = (code >> 5) & 1;
                    }

                    planep += stride * 2;
                }

                x = width % 3;

                if (x)
                { decode_colskip(data, x, height, stride, &v->s.gb); }

                if (height & 1)
                { decode_rowskip(data + x, width - x, 1, stride, &v->s.gb); }
            }

            break;

        case IMODE_ROWSKIP:
            decode_rowskip(data, width, height, stride, &v->s.gb);
            break;

        case IMODE_COLSKIP:
            decode_colskip(data, width, height, stride, &v->s.gb);
            break;

        default:
            /* 2010/06/23 10:30:00 liuxw+00139685 */
            /* 增加日志打印，并返回 */
            av_log(v->s.avctx, AV_LOG_WARNING, "Bitplane mode[%d] is invalid!\n", imode);
            return -1;
            //		break;
    }

    /* Applying diff operator */
    if (imode == IMODE_DIFF2 || imode == IMODE_DIFF6)
    {
        planep = data;
        planep[0] ^= invert;

        for (x = 1; x < width; x++)
        { planep[x] ^= planep[x - 1]; }

        for (y = 1; y < height; y++)
        {
            planep += stride;
            planep[0] ^= planep[-stride];

            for (x = 1; x < width; x++)
            {
                if (planep[x - 1] != planep[x - stride])
                { planep[x] ^= invert; }
                else
                { planep[x] ^= planep[x - 1]; }
            }
        }
    }
    else if (invert)
    {
        planep = data;

        for (x = 0; x < stride * height; x++)
        { planep[x] = !planep[x]; } //FIXME stride
    }

    return (imode << 1) + invert;
}

/** @} */ //Bitplane group

#define FILTSIGN(a) ((a) >= 0 ? 1 : -1)
/**
 * VC-1 in-loop deblocking filter for one line
 * @param src source block type
 * @param stride block stride
 * @param pq block quantizer
 * @return whether other 3 pairs should be filtered or not
 * @see 8.6
 */
static av_always_inline int vc1_filter_line(uint8_t *src, int stride, int pq)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    int a0 = (2 * (src[-2 * stride] - src[ 1 * stride]) - 5 * (src[-1 * stride] - src[ 0 * stride]) + 4) >> 3;
    int a0_sign = a0 >> 31;        /* Store sign */
    a0 = (a0 ^ a0_sign) - a0_sign; /* a0 = FFABS(a0); */

    if (a0 < pq)
    {
        int a1 = FFABS((2 * (src[-4 * stride] - src[-1 * stride]) - 5 * (src[-3 * stride] - src[-2 * stride]) + 4) >> 3);
        int a2 = FFABS((2 * (src[ 0 * stride] - src[ 3 * stride]) - 5 * (src[ 1 * stride] - src[ 2 * stride]) + 4) >> 3);

        if (a1 < a0 || a2 < a0)
        {
            int clip = src[-1 * stride] - src[ 0 * stride];
            int clip_sign = clip >> 31;
            clip = ((clip ^ clip_sign) - clip_sign) >> 1;

            if (clip)
            {
                int a3 = FFMIN(a1, a2);
                int d = 5 * (a3 - a0);
                int d_sign = (d >> 31);
                d = ((d ^ d_sign) - d_sign) >> 3;
                d_sign ^= a0_sign;

                if ( d_sign ^ clip_sign )
                { d = 0; }
                else
                {
                    d = FFMIN(d, clip);
                    d = (d ^ d_sign) - d_sign;          /* Restore sign */
                    src[-1 * stride] = cm[src[-1 * stride] - d];
                    src[ 0 * stride] = cm[src[ 0 * stride] + d];
                }

                return 1;
            }
        }
    }

    return 0;
}

/**
 * VC-1 in-loop deblocking filter
 * @param src source block type
 * @param step distance between horizontally adjacent elements
 * @param stride distance between vertically adjacent elements
 * @param len edge length to filter (4 or 8 pixels)
 * @param pq block quantizer
 * @see 8.6
 */
static void vc1_loop_filter(uint8_t *src, int step, int stride, int len, int pq)
{
    int i;
    int filt3;

    for (i = 0; i < len; i += 4)
    {
        filt3 = vc1_filter_line(src + 2 * step, stride, pq);

        if (filt3)
        {
            vc1_filter_line(src + 0 * step, stride, pq);
            vc1_filter_line(src + 1 * step, stride, pq);
            vc1_filter_line(src + 3 * step, stride, pq);
        }

        src += step * 4;
    }
}

static void vc1_loop_filter_iblk(MpegEncContext *s, int pq)
{
    /* 2010/06/28 16:30:00 liuxw+00139685 */
    /* 由于代码修改，所以变量i没有使用，为消除编译warning，注释掉变量i的声明  */
    //  int i, j;
    int j;

    if (!s->first_slice_line)
    { vc1_loop_filter(s->dest[0], 1, s->linesize, 16, pq); }

    vc1_loop_filter(s->dest[0] + 8 * s->linesize, 1, s->linesize, 16, pq);

    /* 2010/06/28 16:30:00 liuxw+00139685 */
    /* 去掉for的描述方式，将其改为if的方式  */
    //  for(i = !s->mb_x*8; i < 16; i += 8)
    //		vc1_loop_filter(s->dest[0] + i, s->linesize, 1, 16, pq);
    if (s->mb_x)
    {
        vc1_loop_filter(s->dest[0], s->linesize, 1, 16, pq);
    }

    vc1_loop_filter(s->dest[0] + 8, s->linesize, 1, 16, pq);

    for (j = 0; j < 2; j++)
    {
        if (!s->first_slice_line)
        { vc1_loop_filter(s->dest[j + 1], 1, s->uvlinesize, 8, pq); }

        if (s->mb_x)
        { vc1_loop_filter(s->dest[j + 1], s->uvlinesize, 1, 8, pq); }
    }
}

/***********************************************************************/
/** VOP Dquant decoding
 * @param v VC-1 Context
 */
static int vop_dquant_decoding(VC1Context *v)
{
    GetBitContext *gb = &v->s.gb;
    int pqdiff;

    //variable size
    if (v->dquant == 2)
    {
        pqdiff = get_bits(gb, 3);

        if (pqdiff == 7)
        { v->altpq = get_bits(gb, 5); }
        else
        { v->altpq = v->pq + pqdiff + 1; }
    }
    else
    {
        v->dquantfrm = get_bits1(gb);

        if ( v->dquantfrm )
        {
            v->dqprofile = get_bits(gb, 2);

            switch (v->dqprofile)
            {
                case DQPROFILE_SINGLE_EDGE:
                case DQPROFILE_DOUBLE_EDGES:
                    v->dqsbedge = get_bits(gb, 2);
                    break;

                case DQPROFILE_ALL_MBS:
                    v->dqbilevel = get_bits1(gb);

                    if (!v->dqbilevel)     /* 2010/06/23 10:30:00 liuxw+00139685 所有的宏块都不会用PQUANT,所以halfqp也不会使用 */
                    { v->halfpq = 0; }

                    break;

                default:
                    break; //Forbidden ?  /* 2010/06/23 10:30:00 liuxw+00139685 DQPROFILE_FOUR_EDGES */
            }

            if (v->dqbilevel || v->dqprofile != DQPROFILE_ALL_MBS)
            {
                pqdiff = get_bits(gb, 3);

                if (pqdiff == 7)
                { v->altpq = get_bits(gb, 5); }
                else
                { v->altpq = v->pq + pqdiff + 1; }
            }
        }
    }

    return 0;
}

/** Put block onto picture
 */
static void vc1_put_block(VC1Context *v, DCTELEM block[6][64])
{
    uint8_t *Y;
    int ys, us, vs;
    DSPContext *dsp = &v->s.dsp;
    /* 2010/08/26 10:00:00 liuxw+00139685 */
    /* rangeredfrm功能使用错误 */
    /*
        if(v->rangeredfrm)
    	{
            int i, j, k;
            for(k = 0; k < 6; k++)
                for(j = 0; j < 8; j++)
                    for(i = 0; i < 8; i++)
                        block[k][i + j*8] = ((block[k][i + j*8] - 128) << 1) + 128;

        } */
    ys = v->s.current_picture.linesize[0];
    us = v->s.current_picture.linesize[1];
    vs = v->s.current_picture.linesize[2];
    Y = v->s.dest[0];

    dsp->put_pixels_clamped(block[0], Y, ys);
    dsp->put_pixels_clamped(block[1], Y + 8, ys);
    Y += ys * 8;
    dsp->put_pixels_clamped(block[2], Y, ys);
    dsp->put_pixels_clamped(block[3], Y + 8, ys);

    if (!(v->s.flags & CODEC_FLAG_GRAY))
    {
        dsp->put_pixels_clamped(block[4], v->s.dest[1], us);
        dsp->put_pixels_clamped(block[5], v->s.dest[2], vs);
    }
}

/** Do motion compensation over 1 macroblock
 * Mostly adapted hpel_motion and qpel_motion from mpegvideo.c
 */
static void vc1_mc_1mv(VC1Context *v, int dir)
{
    MpegEncContext *s = &v->s;
    DSPContext *dsp = &v->s.dsp;
    uint8_t *srcY, *srcU, *srcV;
    int dxy, uvdxy, mx, my, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;

    if (!v->s.last_picture.data[0])
    { return; }

    mx = s->mv[dir][0][0];
    my = s->mv[dir][0][1];

    // store motion vectors for further use in B frames
    if (s->pict_type == FF_P_TYPE)
    {
        s->current_picture.motion_val[1][s->block_index[0]][0] = mx;
        s->current_picture.motion_val[1][s->block_index[0]][1] = my;
    }

    uvmx = (mx + ((mx & 3) == 3)) >> 1;
    uvmy = (my + ((my & 3) == 3)) >> 1;

    if (v->fastuvmc)
    {
        uvmx = uvmx + ((uvmx < 0) ? (uvmx & 1) : -(uvmx & 1));
        uvmy = uvmy + ((uvmy < 0) ? (uvmy & 1) : -(uvmy & 1));
    }

    if (!dir)
    {
        srcY = s->last_picture.data[0];
        srcU = s->last_picture.data[1];
        srcV = s->last_picture.data[2];
    }
    else
    {
        srcY = s->next_picture.data[0];
        srcU = s->next_picture.data[1];
        srcV = s->next_picture.data[2];
    }

    src_x = s->mb_x * 16 + (mx >> 2);
    src_y = s->mb_y * 16 + (my >> 2);
    uvsrc_x = s->mb_x * 8 + (uvmx >> 2);
    uvsrc_y = s->mb_y * 8 + (uvmy >> 2);

    if (v->profile != PROFILE_ADVANCED)
    {
        src_x   = av_clip(  src_x, -16, s->mb_width  * 16);
        src_y   = av_clip(  src_y, -16, s->mb_height * 16);
        uvsrc_x = av_clip(uvsrc_x,  -8, s->mb_width  *  8);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->mb_height *  8);
    }
    else
    {
        src_x   = av_clip(  src_x, -17, s->avctx->coded_width);
        src_y   = av_clip(  src_y, -18, s->avctx->coded_height + 1);
        uvsrc_x = av_clip(uvsrc_x,  -8, s->avctx->coded_width  >> 1);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->avctx->coded_height >> 1);
    }

    srcY += src_y * s->linesize + src_x;
    srcU += uvsrc_y * s->uvlinesize + uvsrc_x;
    srcV += uvsrc_y * s->uvlinesize + uvsrc_x;

    /* for grayscale we should not try to read from unknown area */
    if (s->flags & CODEC_FLAG_GRAY)
    {
        srcU = s->edge_emu_buffer + 18 * s->linesize;
        srcV = s->edge_emu_buffer + 18 * s->linesize;
    }

    /* 2010/05/27 10:00:00 liuxw+00139685 */
    /* 消除编译warning(在>的右边强制转化为unsigned) */
    if (v->rangeredfrm || (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        || (unsigned)(src_x - s->mspel) > (unsigned)(s->h_edge_pos - (mx & 3) - 16 - s->mspel * 3)
        || (unsigned)(src_y - s->mspel) > (unsigned)(s->v_edge_pos - (my & 3) - 16 - s->mspel * 3))
    {
        uint8_t *uvbuf = s->edge_emu_buffer + 19 * s->linesize;

        srcY -= s->mspel * (1 + s->linesize);
        ff_emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 17 + s->mspel * 2, 17 + s->mspel * 2,
                            src_x - s->mspel, src_y - s->mspel, s->h_edge_pos, s->v_edge_pos);
        srcY = s->edge_emu_buffer;
        ff_emulated_edge_mc(uvbuf     , srcU, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        ff_emulated_edge_mc(uvbuf + 16, srcV, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        srcU = uvbuf;
        srcV = uvbuf + 16;

        /* if we deal with range reduction we need to scale source blocks */
        /* 2010/08/26 10:00:00 liuxw+00139685 */
        /* rangeredfrm功能使用错误 */
        /*     if(v->rangeredfrm)
        	{
                 int i, j;
                 uint8_t *src, *src2;

                 src = srcY;
                 for(j = 0; j < 17 + s->mspel*2; j++)
        		{
                     for(i = 0; i < 17 + s->mspel*2; i++) src[i] = ((src[i] - 128) >> 1) + 128;
                     src += s->linesize;
                 }
                 src = srcU;
        		src2 = srcV;
                 for(j = 0; j < 9; j++)
        		{
                     for(i = 0; i < 9; i++)
        			{
                         src[i] = ((src[i] - 128) >> 1) + 128;
                         src2[i] = ((src2[i] - 128) >> 1) + 128;
                     }
                     src += s->uvlinesize;
                     src2 += s->uvlinesize;
                 }
             } */
        /* if we deal with intensity compensation we need to scale source blocks */
        if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        {
            int i, j;
            uint8_t *src, *src2;

            src = srcY;

            for (j = 0; j < 17 + s->mspel * 2; j++)
            {
                for (i = 0; i < 17 + s->mspel * 2; i++)
                { src[i] = v->luty[src[i]]; }

                src += s->linesize;
            }

            src = srcU;
            src2 = srcV;

            for (j = 0; j < 9; j++)
            {
                for (i = 0; i < 9; i++)
                {
                    src[i] = v->lutuv[src[i]];
                    src2[i] = v->lutuv[src2[i]];
                }

                src += s->uvlinesize;
                src2 += s->uvlinesize;
            }
        }

        srcY += s->mspel * (1 + s->linesize);
    }

    if (s->mspel)
    {
        dxy = ((my & 3) << 2) | (mx & 3);
        dsp->put_vc1_mspel_pixels_tab[dxy](s->dest[0]    , srcY    , s->linesize, v->rnd);
        dsp->put_vc1_mspel_pixels_tab[dxy](s->dest[0] + 8, srcY + 8, s->linesize, v->rnd);
        srcY += s->linesize * 8;
        dsp->put_vc1_mspel_pixels_tab[dxy](s->dest[0] + 8 * s->linesize    , srcY    , s->linesize, v->rnd);
        dsp->put_vc1_mspel_pixels_tab[dxy](s->dest[0] + 8 * s->linesize + 8, srcY + 8, s->linesize, v->rnd);
    }
    else
    {
        // hpel mc - always used for luma
        dxy = (my & 2) | ((mx & 2) >> 1);

        if (!v->rnd)
        { dsp->put_pixels_tab[0][dxy](s->dest[0], srcY, s->linesize, 16); }
        else
        { dsp->put_no_rnd_pixels_tab[0][dxy](s->dest[0], srcY, s->linesize, 16); }
    }

    if (s->flags & CODEC_FLAG_GRAY)
    { return; }

    /* Chroma MC always uses qpel bilinear */
    uvdxy = ((uvmy & 3) << 2) | (uvmx & 3);
    uvmx = (uvmx & 3) << 1;
    uvmy = (uvmy & 3) << 1;

    if (!v->rnd)
    {
        dsp->put_h264_chroma_pixels_tab[0](s->dest[1], srcU, s->uvlinesize, 8, uvmx, uvmy);
        dsp->put_h264_chroma_pixels_tab[0](s->dest[2], srcV, s->uvlinesize, 8, uvmx, uvmy);
    }
    else
    {
        dsp->put_no_rnd_h264_chroma_pixels_tab[0](s->dest[1], srcU, s->uvlinesize, 8, uvmx, uvmy);
        dsp->put_no_rnd_h264_chroma_pixels_tab[0](s->dest[2], srcV, s->uvlinesize, 8, uvmx, uvmy);
    }
}

/** Do motion compensation for 4-MV macroblock - luminance block
 */
static void vc1_mc_4mv_luma(VC1Context *v, int n)
{
    MpegEncContext *s = &v->s;
    DSPContext *dsp = &v->s.dsp;
    uint8_t *srcY;
    int dxy, mx, my, src_x, src_y;
    int off;
    //int tmpMvx,tmpMvy;
    //int tmpPosX,tmpPosY;

    if (!v->s.last_picture.data[0])
    { return; }

    mx = s->mv[0][n][0];
    my = s->mv[0][n][1];
    srcY = s->last_picture.data[0];

    off = s->linesize * 4 * (n & 2) + (n & 1) * 8;

    //tmpPosX = s->mb_x * 16 + (n&1) * 8;
    //tmpPosY = s->mb_y * 16 + (n&2) * 4;

    src_x = s->mb_x * 16 + (n & 1) * 8 + (mx >> 2);
    src_y = s->mb_y * 16 + (n & 2) * 4 + (my >> 2);
    /*
    	src_x = tmpPosX + (mx >> 2);
    	src_y = tmpPosY + (my >> 2); */

    if (v->profile != PROFILE_ADVANCED)
    {
        src_x   = av_clip(  src_x, -16, s->mb_width  * 16);
        src_y   = av_clip(  src_y, -16, s->mb_height * 16);
        /*		tmpMvx = mx;
        		tmpMvy = my;

        		if(src_x < -16)
        		{
        			tmpMvx = ((-16-tmpPosX)<<2) + (mx&3);
        		}
        		else if(src_x > s->mb_width*16)
        		{
        			tmpMvx = ((s->mb_width*16 - tmpPosX)<<2) + (mx&3);
        		}

        		if(src_y < -16)
        		{
        			tmpMvy = ((-16-tmpPosY)<<2) + (my&3);
        		}
        		else if(src_y > s->mb_height*16)
        		{
        			tmpMvy = ((s->mb_height*16 - tmpPosY)<<2) + (my&3);
        		}

        		src_x = tmpPosX + (tmpMvx >>2);
        		src_y = tmpPosY + (tmpMvy >>2); */

    }
    else
    {
        src_x   = av_clip(  src_x, -17, s->avctx->coded_width);
        src_y   = av_clip(  src_y, -18, s->avctx->coded_height + 1);
    }

    srcY += src_y * s->linesize + src_x;

    /* 2010/05/27 10:00:00 liuxw+00139685 */
    /* 消除编译warning(在>的右边强制转化为unsigned) */
    if (v->rangeredfrm || (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        || (unsigned)(src_x - s->mspel) > (unsigned)(s->h_edge_pos - (mx & 3) - 8 - s->mspel * 2)
        || (unsigned)(src_y - s->mspel) > (unsigned)(s->v_edge_pos - (my & 3) - 8 - s->mspel * 2))
    {
        srcY -= s->mspel * (1 + s->linesize);
        ff_emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 9 + s->mspel * 2, 9 + s->mspel * 2,
                            src_x - s->mspel, src_y - s->mspel, s->h_edge_pos, s->v_edge_pos);
        srcY = s->edge_emu_buffer;

        /* if we deal with range reduction we need to scale source blocks */
        /* 2010/08/26 10:00:00 liuxw+00139685 */
        /* rangeredfrm功能使用错误 */
        /*        if(v->rangeredfrm)
        		{
                    int i, j;
                    uint8_t *src;

                    src = srcY;
                    for(j = 0; j < 9 + s->mspel*2; j++)
        			{
                        for(i = 0; i < 9 + s->mspel*2; i++) src[i] = ((src[i] - 128) >> 1) + 128;
                        src += s->linesize;
                    }
                } */
        /* if we deal with intensity compensation we need to scale source blocks */
        if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        {
            int i, j;
            uint8_t *src;

            src = srcY;

            for (j = 0; j < 9 + s->mspel * 2; j++)
            {
                for (i = 0; i < 9 + s->mspel * 2; i++)
                { src[i] = v->luty[src[i]]; }

                src += s->linesize;
            }
        }

        srcY += s->mspel * (1 + s->linesize);
    }

    if (s->mspel)
    {
        dxy = ((my & 3) << 2) | (mx & 3);
        dsp->put_vc1_mspel_pixels_tab[dxy](s->dest[0] + off, srcY, s->linesize, v->rnd);
    }
    else
    {
        // hpel mc - always used for luma
        dxy = (my & 2) | ((mx & 2) >> 1);

        if (!v->rnd)
        { dsp->put_pixels_tab[1][dxy](s->dest[0] + off, srcY, s->linesize, 8); }
        else
        { dsp->put_no_rnd_pixels_tab[1][dxy](s->dest[0] + off, srcY, s->linesize, 8); }
    }
}

static inline int median4(int a, int b, int c, int d)
{
    if (a < b)
    {
        if (c < d)
        { return (FFMIN(b, d) + FFMAX(a, c)) / 2; }
        else
        { return (FFMIN(b, c) + FFMAX(a, d)) / 2; }
    }
    else
    {
        if (c < d)
        { return (FFMIN(a, d) + FFMAX(b, c)) / 2; }
        else
        { return (FFMIN(a, c) + FFMAX(b, d)) / 2; }
    }
}


/** Do motion compensation for 4-MV macroblock - both chroma blocks
 */
static void vc1_mc_4mv_chroma(VC1Context *v)
{
    MpegEncContext *s = &v->s;
    DSPContext *dsp = &v->s.dsp;
    uint8_t *srcU, *srcV;
    int uvdxy, uvmx, uvmy, uvsrc_x, uvsrc_y;
    int i, idx, tx = 0, ty = 0;
    int mvx[4], mvy[4], intra[4];
    static const int count[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

    if (!v->s.last_picture.data[0]) { return; }

    if (s->flags & CODEC_FLAG_GRAY) { return; }

    for (i = 0; i < 4; i++)
    {
        mvx[i] = s->mv[0][i][0];
        mvy[i] = s->mv[0][i][1];
        intra[i] = v->mb_type[0][s->block_index[i]];
    }

    /* calculate chroma MV vector from four luma MVs */
    idx = (intra[3] << 3) | (intra[2] << 2) | (intra[1] << 1) | intra[0];

    if (!idx)  // all blocks are inter
    {
        tx = median4(mvx[0], mvx[1], mvx[2], mvx[3]);
        ty = median4(mvy[0], mvy[1], mvy[2], mvy[3]);
    }
    else if (count[idx] == 1)    // 3 inter blocks
    {
        switch (idx)
        {
            case 0x1:
                tx = mid_pred(mvx[1], mvx[2], mvx[3]);
                ty = mid_pred(mvy[1], mvy[2], mvy[3]);
                break;

            case 0x2:
                tx = mid_pred(mvx[0], mvx[2], mvx[3]);
                ty = mid_pred(mvy[0], mvy[2], mvy[3]);
                break;

            case 0x4:
                tx = mid_pred(mvx[0], mvx[1], mvx[3]);
                ty = mid_pred(mvy[0], mvy[1], mvy[3]);
                break;

            case 0x8:
                tx = mid_pred(mvx[0], mvx[1], mvx[2]);
                ty = mid_pred(mvy[0], mvy[1], mvy[2]);
                break;
        }
    }
    else if (count[idx] == 2)
    {
        int t1 = 0, t2 = 0;

        for (i = 0; i < 3; i++) if (!intra[i]) {t1 = i; break;}

        for (i = t1 + 1; i < 4; i++)if (!intra[i]) {t2 = i; break;}

        tx = (mvx[t1] + mvx[t2]) / 2;
        ty = (mvy[t1] + mvy[t2]) / 2;
    }
    else
    {
        s->current_picture.motion_val[1][s->block_index[0]][0] = 0;
        s->current_picture.motion_val[1][s->block_index[0]][1] = 0;
        return; //no need to do MC for inter blocks
    }

    s->current_picture.motion_val[1][s->block_index[0]][0] = tx;
    s->current_picture.motion_val[1][s->block_index[0]][1] = ty;
    uvmx = (tx + ((tx & 3) == 3)) >> 1;
    uvmy = (ty + ((ty & 3) == 3)) >> 1;

    if (v->fastuvmc)
    {
        uvmx = uvmx + ((uvmx < 0) ? (uvmx & 1) : -(uvmx & 1));
        uvmy = uvmy + ((uvmy < 0) ? (uvmy & 1) : -(uvmy & 1));
    }

    uvsrc_x = s->mb_x * 8 + (uvmx >> 2);
    uvsrc_y = s->mb_y * 8 + (uvmy >> 2);

    if (v->profile != PROFILE_ADVANCED)
    {
        uvsrc_x = av_clip(uvsrc_x,  -8, s->mb_width  *  8);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->mb_height *  8);
    }
    else
    {
        uvsrc_x = av_clip(uvsrc_x,  -8, s->avctx->coded_width  >> 1);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->avctx->coded_height >> 1);
    }

    srcU = s->last_picture.data[1] + uvsrc_y * s->uvlinesize + uvsrc_x;
    srcV = s->last_picture.data[2] + uvsrc_y * s->uvlinesize + uvsrc_x;

    /* 2010/05/27 10:00:00 liuxw+00139685 */
    /* 消除编译warning(在>的右边强制转化为unsigned) */
    if (v->rangeredfrm || (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        || (unsigned)uvsrc_x > (unsigned)((s->h_edge_pos >> 1) - 9)
        || (unsigned)uvsrc_y > (unsigned)((s->v_edge_pos >> 1) - 9))
    {
        ff_emulated_edge_mc(s->edge_emu_buffer     , srcU, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        ff_emulated_edge_mc(s->edge_emu_buffer + 16, srcV, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        srcU = s->edge_emu_buffer;
        srcV = s->edge_emu_buffer + 16;

        /* if we deal with range reduction we need to scale source blocks */
        /* 2010/08/26 10:00:00 liuxw+00139685 */
        /* rangeredfrm功能使用错误 */
        /*        if(v->rangeredfrm)
        		{
                    int i, j;
                    uint8_t *src, *src2;

                    src = srcU; src2 = srcV;
                    for(j = 0; j < 9; j++)
        			{
                        for(i = 0; i < 9; i++)
        				{
                            src[i] = ((src[i] - 128) >> 1) + 128;
                            src2[i] = ((src2[i] - 128) >> 1) + 128;
                        }
                        src += s->uvlinesize;
                        src2 += s->uvlinesize;
                    }
                } */
        /* if we deal with intensity compensation we need to scale source blocks */
        if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        {
            int i, j;
            uint8_t *src, *src2;

            src = srcU;
            src2 = srcV;

            for (j = 0; j < 9; j++)
            {
                for (i = 0; i < 9; i++)
                {
                    src[i] = v->lutuv[src[i]];
                    src2[i] = v->lutuv[src2[i]];
                }

                src += s->uvlinesize;
                src2 += s->uvlinesize;
            }
        }
    }

    /* Chroma MC always uses qpel bilinear */
    uvdxy = ((uvmy & 3) << 2) | (uvmx & 3);
    uvmx = (uvmx & 3) << 1;
    uvmy = (uvmy & 3) << 1;

    if (!v->rnd)
    {
        dsp->put_h264_chroma_pixels_tab[0](s->dest[1], srcU, s->uvlinesize, 8, uvmx, uvmy);
        dsp->put_h264_chroma_pixels_tab[0](s->dest[2], srcV, s->uvlinesize, 8, uvmx, uvmy);
    }
    else
    {
        dsp->put_no_rnd_h264_chroma_pixels_tab[0](s->dest[1], srcU, s->uvlinesize, 8, uvmx, uvmy);
        dsp->put_no_rnd_h264_chroma_pixels_tab[0](s->dest[2], srcV, s->uvlinesize, 8, uvmx, uvmy);
    }
}

static int decode_sequence_header_adv(VC1Context *v, GetBitContext *gb);

/**
 * Decode Simple/Main Profiles sequence header
 * @see Figure 7-8, p16-17
 * @param avctx Codec context
 * @param gb GetBit context initialized from Codec context extra_data
 * @return Status
 */
/* 2010/06/17 17:00:00 liuxw+00139685 */
/* 将静态的属性去掉：在probeheader中会调用此函数 */
/*static*/ int decode_sequence_header(AVCodecContext *avctx, GetBitContext *gb)
{
    VC1Context *v = avctx->priv_data;
    int width, height;

    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 增加解析宽和高的语法元素(私自定义的) */
    width = get_bits(gb, 32) * 2;
    height = get_bits(gb, 32) * 2;

    /* 2010/06/18 19:00:00 liuxw+00139685 */
    /* 添加长和宽的合法性检查及返回 */
    if (avctx->usSourceWidth && avctx->usSourceHeight < height)
    {
        if (MIN_PIC_WIDTH > width || avctx->usSourceWidth < width || MIN_PIC_HEIGHT > height || avctx->usSourceHeight < height)
        {
            av_log(avctx, AV_LOG_WARNING, "width[%d] or height[%d] is out of range([%d-%d],[%d-%d])\n", width, height, MIN_PIC_WIDTH, avctx->usSourceWidth, MIN_PIC_HEIGHT, avctx->usSourceHeight);
            avctx->iTotalError++;
            IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_SIZE);
            return -1;
        }
    }
    else
    {
        if (MIN_PIC_WIDTH > width || MAX_PIC_WIDTH < width || MIN_PIC_HEIGHT > height || MAX_PIC_HEIGHT < height)
        {
            av_log(avctx, AV_LOG_WARNING, "width[%d] or height[%d] is out of range([%d-%d],[%d-%d])\n", width, height, MIN_PIC_WIDTH, MAX_PIC_WIDTH, MIN_PIC_HEIGHT, MAX_PIC_HEIGHT);
            avctx->iTotalError++;
            IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_SIZE);
            return -1;
        }
    }


    v->s.avctx->width  = v->s.avctx->coded_width  = width;
    v->s.avctx->height = v->s.avctx->coded_height = height;

    v->s.h_edge_pos = v->s.avctx->coded_width;
    v->s.v_edge_pos = v->s.avctx->coded_height;

    av_log(avctx, AV_LOG_DEBUG, "Header: %0X\n", show_bits(gb, 32));
    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 由协议附录J可知，profile语法元素占4bit */
    //	v->profile = get_bits(gb, 2);
    v->profile = get_bits(gb, 4);

    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 修改profile不正确的处理方式:报警告，然后继续向后解 */
    //if (v->profile == PROFILE_COMPLEX)
    //{
    //    av_log(avctx, AV_LOG_ERROR, "WMV3 Complex Profile is not fully supported\n");
    //}

    //   if (v->profile == PROFILE_ADVANCED)
    //  {
    //v->zz_8x4 = ff_vc1_adv_progressive_8x4_zz;
    //v->zz_4x8 = ff_vc1_adv_progressive_4x8_zz;
    //return decode_sequence_header_adv(v, gb);

    //  }
    if (v->profile != PROFILE_SIMPLE && v->profile != PROFILE_MAIN)
    {
        av_log(avctx, AV_LOG_WARNING, "Profile[%d] is invalid or not support!\n", v->profile);
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对profile进行修正 */
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_PROFILE_LEVEL);
        v->profile = PROFILE_MAIN;
    }

    //  else
    //  {
    v->zz_8x4 = wmv2_scantableA;
    v->zz_4x8 = wmv2_scantableB;
    //      v->res_sm = get_bits(gb, 2); //reserved
    //    if (v->res_sm)
    //    {
    //        av_log(avctx, AV_LOG_ERROR,
    //               "Reserved RES_SM=%i is forbidden\n", v->res_sm);
    //        return -1;
    //    }
    //}

    // (fps-2)/4 (->30)
    v->frmrtq_postproc = get_bits(gb, 3); //common
    // (bitrate-32kbps)/64kbps
    v->bitrtq_postproc = get_bits(gb, 5); //common
    v->s.loop_filter = get_bits1(gb); //common

    if (v->s.loop_filter == 1 && v->profile == PROFILE_SIMPLE)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对loop_filter进行修正 */
        //      av_log(avctx, AV_LOG_WARNING,"LOOPFILTER shell not be enabled in simple profile\n");
        av_log(avctx, AV_LOG_WARNING, "LOOPFILTER shell not be enabled in simple profile\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_PROFILE_LEVEL);
        v->s.loop_filter = 0;
    }

    if (v->s.avctx->skip_loop_filter >= AVDISCARD_ALL)
    { v->s.loop_filter = 0; }

    v->res_x8 = get_bits1(gb); //reserved

    /* 2010/06/19 16:30:00 liuxw+00139685 */
    /* 增加对保留位的合法性判断,并进行修正 */
    if (1 == v->res_x8)
    {
        av_log(avctx, AV_LOG_WARNING, "Reserved bit must be zero!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->res_x8 = 0;
    }

    v->multires = get_bits1(gb);

    /* 2010/06/19 16:30:00 liuxw+00139685 */
    /* 在simple profiel下不支持动态调整分辨率 */
    if (PROFILE_SIMPLE == v->profile && v->multires)
    {
        av_log(avctx, AV_LOG_WARNING, "Simple Profile not support resolution change!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->multires = 0;
    }

    v->res_fasttx = get_bits1(gb);

    if (!v->res_fasttx)
    {
        v->s.dsp.vc1_inv_trans_8x8 = ff_simple_idct;
        v->s.dsp.vc1_inv_trans_8x4 = ff_simple_idct84_add;
        v->s.dsp.vc1_inv_trans_4x8 = ff_simple_idct48_add;
        v->s.dsp.vc1_inv_trans_4x4 = ff_simple_idct44_add;
        /* 2010/06/19 16:30:00 liuxw+00139685 */
        /* 保留位必需为1 */
        av_log(avctx, AV_LOG_WARNING, "Simple Profile not support resolution change!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->res_fasttx = 0;
    }

    v->fastuvmc =  get_bits1(gb); //common

    if (!v->profile && !v->fastuvmc)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对fastuvmc进行修正 */
        //      av_log(avctx, AV_LOG_ERROR,"FASTUVMC unavailable in Simple Profile\n");
        av_log(avctx, AV_LOG_ERROR, "FASTUVMC unavailable in Simple Profile\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        //      return -1;
        v->fastuvmc = 1;
    }

    v->extended_mv =  get_bits1(gb); //common

    if (!v->profile && v->extended_mv)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对extended_mv进行修正 */
        //      av_log(avctx, AV_LOG_ERROR,"Extended MVs unavailable in Simple Profile\n");
        av_log(avctx, AV_LOG_WARNING, "Extended MVs unavailable in Simple Profile\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        //      return -1;
        v->extended_mv = 0;
    }

    v->dquant =  get_bits(gb, 2); //common

    /* 2010/06/19 17:00:00 liuxw+00139685 */
    /* 增加对dquant的合法性检测 */
    if (v->dquant && PROFILE_SIMPLE == v->profile ||  (PROFILE_MAIN == v->profile && v->multires))
    {
        av_log(avctx, AV_LOG_WARNING, "Simple Profile not support dquant[%d] or Main Profile not support dquant[%d] and resolution change simultaneity!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->dquant = 0;
    }

    v->vstransform =  get_bits1(gb); //common

    v->res_transtab = get_bits1(gb);

    if (v->res_transtab)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对extended_mv进行修正 */
        //		av_log(avctx, AV_LOG_ERROR,"1 for reserved RES_TRANSTAB is forbidden\n");
        av_log(avctx, AV_LOG_WARNING, "1 for reserved RES_TRANSTAB is forbidden!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        //      return -1;
        v->res_transtab = 0;
    }

    v->overlap = get_bits1(gb); //common

    v->s.resync_marker = get_bits1(gb);

    /* 2010/06/18 19:00:00 liuxw+00139685 */
    /* 添加对resync_marker的检测 */
    if (PROFILE_SIMPLE == v->profile && v->s.resync_marker)
    {
        av_log(avctx, AV_LOG_WARNING, "resync_marker should be set to 0 in simple profile\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->s.resync_marker = 0;
    }

    v->rangered = get_bits1(gb);

    if (v->rangered && PROFILE_SIMPLE == v->profile)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对rangered进行修正 */
        //		av_log(avctx, AV_LOG_INFO,"RANGERED should be set to 0 in simple profile\n");
        av_log(avctx, AV_LOG_WARNING, "RANGERED should be set to 0 in simple profile\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->rangered = 0;
    }

    v->s.max_b_frames = avctx->max_b_frames = get_bits(gb, 3); //common

    /* 2010/06/19 10:30:00 liuxw+00139685 */
    /* 增加对simple下不支持B帧的检测 */
    if (PROFILE_SIMPLE == v->profile && v->s.max_b_frames)
    {
        av_log(avctx, AV_LOG_WARNING, "Simple profile not support B frames!\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_MARKER);
        v->s.max_b_frames = 0;
    }

    v->quantizer_mode = get_bits(gb, 2); //common

    v->finterpflag = get_bits1(gb); //common
    v->res_rtm_flag = get_bits1(gb); //reserved

    if (!v->res_rtm_flag)
    {
        /* 2010/06/18 19:00:00 liuxw+00139685 */
        /* 添加错误码，并对res_rtm_flag进行修正 */
        //      av_log(avctx, AV_LOG_ERROR, "0 for reserved RES_RTM_FLAG is forbidden\n");
        //      av_log(avctx, AV_LOG_ERROR,"Old WMV3 version detected, only I-frames will be decoded\n");
        av_log(avctx, AV_LOG_WARNING, "Old WMV3 version detected, only I-frames will be decoded\n");
        avctx->iTotalError++;
        IMEDIA_SET_ERR_SEQ(avctx->iErrorCode, IMEDIA_ERR_SEQ_OTHER);
        //return -1;
        v->res_rtm_flag = 1;
    }

    //TODO: figure out what they mean (always 0x402F)
    if (!v->res_fasttx)
    { skip_bits(gb, 16); }

    av_log(avctx, AV_LOG_DEBUG,
           "Profile %i:\nfrmrtq_postproc=%i, bitrtq_postproc=%i\n"
           "LoopFilter=%i, MultiRes=%i, FastUVMC=%i, Extended MV=%i\n"
           "Rangered=%i, VSTransform=%i, Overlap=%i, SyncMarker=%i\n"
           "DQuant=%i, Quantizer mode=%i, Max B frames=%i\n",
           v->profile, v->frmrtq_postproc, v->bitrtq_postproc,
           v->s.loop_filter, v->multires, v->fastuvmc, v->extended_mv,
           v->rangered, v->vstransform, v->overlap, v->s.resync_marker,
           v->dquant, v->quantizer_mode, avctx->max_b_frames
          );

#ifdef LXW_DEBUG
    av_log(avctx, AV_LOG_WARNING, "Profile:%d Width:%d Height:%d LoopFilter:%d MultiRes:%d FastUVMC:%d Extended MV:%d Rangered:%d VSTransform:%d Overlap:%d DQuant:%d Quantizer mode:%d Max_B_Frm:%d\n",
           v->profile, width, height, v->s.loop_filter, v->multires, v->fastuvmc, v->extended_mv, v->rangered, v->vstransform, v->overlap, v->dquant, v->quantizer_mode, avctx->max_b_frames);
#endif


    return 0;
}

static int decode_sequence_header_adv(VC1Context *v, GetBitContext *gb)
{
    v->res_rtm_flag = 1;

    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 初始化函数指针 */
    v->zz_8x4 = ff_vc1_adv_progressive_8x4_zz;
    v->zz_4x8 = ff_vc1_adv_progressive_4x8_zz;

    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 增加解析profile语法元素 */
    v->profile = get_bits(gb, 2);

    if (v->profile != 3)
    {
        av_log(v->s.avctx, AV_LOG_WARNING, "Reserved Profile[%i]\n", v->profile);
    }

    v->level = get_bits(gb, 3);

    if (v->level >= 5)
    {
        av_log(v->s.avctx, AV_LOG_WARNING, "Reserved Level[%i]\n", v->level);
    }

    v->chromaformat = get_bits(gb, 2);

    if (v->chromaformat != 1)
    {
        av_log(v->s.avctx, AV_LOG_WARNING, "Chroma format[%d] is not supported(Only support 4:2:0)\n", v->chromaformat);
        /* 2010/06/13 16:30:00 liuxw+00139685 */
        /* 若chromaformat不正确，则报警告，并对其进行修正 */
        v->chromaformat = 1;
        //        return -1;
    }

    // (fps-2)/4 (->30)
    v->frmrtq_postproc = get_bits(gb, 3); //common
    // (bitrate-32kbps)/64kbps
    v->bitrtq_postproc = get_bits(gb, 5); //common
    v->postprocflag = get_bits1(gb); //common

    v->s.avctx->coded_width = (get_bits(gb, 12) + 1) << 1;
    v->s.avctx->coded_height = (get_bits(gb, 12) + 1) << 1;
    v->s.avctx->width = v->s.avctx->coded_width;
    v->s.avctx->height = v->s.avctx->coded_height;
    v->broadcast = get_bits1(gb);
    v->interlace = get_bits1(gb);
    v->tfcntrflag = get_bits1(gb);
    v->finterpflag = get_bits1(gb);
    skip_bits1(gb); // reserved

    v->s.h_edge_pos = v->s.avctx->coded_width;
    v->s.v_edge_pos = v->s.avctx->coded_height;

    av_log(v->s.avctx, AV_LOG_DEBUG,
           "Advanced Profile level %i:\nfrmrtq_postproc=%i, bitrtq_postproc=%i\n"
           "LoopFilter=%i, ChromaFormat=%i, Pulldown=%i, Interlace: %i\n"
           "TFCTRflag=%i, FINTERPflag=%i\n",
           v->level, v->frmrtq_postproc, v->bitrtq_postproc,
           v->s.loop_filter, v->chromaformat, v->broadcast, v->interlace,
           v->tfcntrflag, v->finterpflag
          );

    v->psf = get_bits1(gb);

    if (v->psf)
    {
        //PsF, 6.1.13
        av_log(v->s.avctx, AV_LOG_ERROR, "Progressive Segmented Frame mode: not supported (yet)\n");
        return -1;
    }

    v->s.max_b_frames = v->s.avctx->max_b_frames = 7;

    if (get_bits1(gb))
    {
        //Display Info - decoding is not affected by it
        int w, h, ar = 0;
        av_log(v->s.avctx, AV_LOG_DEBUG, "Display extended info:\n");
        v->s.avctx->coded_width  = w = get_bits(gb, 14) + 1;
        v->s.avctx->coded_height = h = get_bits(gb, 14) + 1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "Display dimensions: %ix%i\n", w, h);

        if (get_bits1(gb))
        { ar = get_bits(gb, 4); }

        if (ar && ar < 14)
        {
            v->s.avctx->sample_aspect_ratio = ff_vc1_pixel_aspect[ar];
        }
        else if (ar == 15)
        {
            w = get_bits(gb, 8);
            h = get_bits(gb, 8);
            /* 2010/05/27 10:00:00 liuxw+00139685 */
            /* 消除编译error（vc下） */
            //          v->s.avctx->sample_aspect_ratio = (AVRational){w, h};
            v->s.avctx->sample_aspect_ratio.num = w;
            v->s.avctx->sample_aspect_ratio.den = h;
        }

        av_log(v->s.avctx, AV_LOG_DEBUG, "Aspect: %i:%i\n", v->s.avctx->sample_aspect_ratio.num, v->s.avctx->sample_aspect_ratio.den);

        if (get_bits1(gb))
        {
            //framerate stuff
            if (get_bits1(gb))
            {
                v->s.avctx->time_base.num = 32;
                v->s.avctx->time_base.den = get_bits(gb, 16) + 1;
            }
            else
            {
                int nr, dr;
                nr = get_bits(gb, 8);
                dr = get_bits(gb, 4);

                if (nr && nr < 8 && dr && dr < 3)
                {
                    v->s.avctx->time_base.num = ff_vc1_fps_dr[dr - 1];
                    v->s.avctx->time_base.den = ff_vc1_fps_nr[nr - 1] * 1000;
                }
            }
        }

        if (get_bits1(gb))
        {
            v->color_prim = get_bits(gb, 8);
            v->transfer_char = get_bits(gb, 8);
            v->matrix_coef = get_bits(gb, 8);
        }
    }

    v->hrd_param_flag = get_bits1(gb);

    if (v->hrd_param_flag)
    {
        int i;
        v->hrd_num_leaky_buckets = get_bits(gb, 5);
        skip_bits(gb, 4); //bitrate exponent
        skip_bits(gb, 4); //buffer size exponent

        for (i = 0; i < v->hrd_num_leaky_buckets; i++)
        {
            skip_bits(gb, 16); //hrd_rate[n]
            skip_bits(gb, 16); //hrd_buffer[n]
        }
    }

    return 0;
}

static int decode_entry_point(AVCodecContext *avctx, GetBitContext *gb)
{
    VC1Context *v = avctx->priv_data;
    int i;

    av_log(avctx, AV_LOG_DEBUG, "Entry point: %08X\n", show_bits_long(gb, 32));
    v->broken_link = get_bits1(gb);
    v->closed_entry = get_bits1(gb);
    v->panscanflag = get_bits1(gb);
    v->refdist_flag = get_bits1(gb);
    v->s.loop_filter = get_bits1(gb);
    v->fastuvmc = get_bits1(gb);
    v->extended_mv = get_bits1(gb);
    v->dquant = get_bits(gb, 2);
    v->vstransform = get_bits1(gb);
    v->overlap = get_bits1(gb);
    v->quantizer_mode = get_bits(gb, 2);

    if (v->hrd_param_flag)
    {
        for (i = 0; i < v->hrd_num_leaky_buckets; i++)
        {
            skip_bits(gb, 8); //hrd_full[n]
        }
    }

    if (get_bits1(gb))
    {
        avctx->coded_width = (get_bits(gb, 12) + 1) << 1;
        avctx->coded_height = (get_bits(gb, 12) + 1) << 1;
    }

    if (v->extended_mv)
    { v->extended_dmv = get_bits1(gb); }

    if ((v->range_mapy_flag = get_bits1(gb)))
    {
        av_log(avctx, AV_LOG_ERROR, "Luma scaling is not supported, expect wrong picture\n");
        v->range_mapy = get_bits(gb, 3);
    }

    if ((v->range_mapuv_flag = get_bits1(gb)))
    {
        av_log(avctx, AV_LOG_ERROR, "Chroma scaling is not supported, expect wrong picture\n");
        v->range_mapuv = get_bits(gb, 3);
    }

    av_log(avctx, AV_LOG_DEBUG, "Entry point info:\n"
           "BrokenLink=%i, ClosedEntry=%i, PanscanFlag=%i\n"
           "RefDist=%i, Postproc=%i, FastUVMC=%i, ExtMV=%i\n"
           "DQuant=%i, VSTransform=%i, Overlap=%i, Qmode=%i\n",
           v->broken_link, v->closed_entry, v->panscanflag, v->refdist_flag, v->s.loop_filter,
           v->fastuvmc, v->extended_mv, v->dquant, v->vstransform, v->overlap, v->quantizer_mode);

    return 0;
}

static int vc1_parse_frame_header(VC1Context *v, GetBitContext *gb)
{
    int pqindex, lowquant, status;

#ifdef LXW_DEBUG
    int tmp = 0;

    if (frame_num == TARGET_FRAME)
    { av_log(v->s.avctx, AV_LOG_WARNING, "Come in Deubg frame %d\n", frame_num); }

#endif

    if (v->finterpflag)
    { v->interpfrm = get_bits1(gb); }  /* 2010/06/23 9:30:00 liuxw+00139685 not used by decode process */

    skip_bits(gb, 2); //framecnt unused
    v->rangeredfrm = 0;

    if (v->rangered)
    { v->rangeredfrm = get_bits1(gb); }

    v->s.pict_type = get_bits1(gb);

    if (v->s.avctx->max_b_frames)
    {
        if (!v->s.pict_type)
        {
            if (get_bits1(gb))
            { v->s.pict_type = FF_I_TYPE; }
            else
            { v->s.pict_type = FF_B_TYPE; }
        }
        else { v->s.pict_type = FF_P_TYPE; }
    }
    else { v->s.pict_type = v->s.pict_type ? FF_P_TYPE : FF_I_TYPE; }

    /* 2010/06/19 10:30:00 liuxw+00139685 */
    /* 更新解码状态信息 */
    switch (v->s.pict_type)
    {
        case FF_I_TYPE:
            v->s.avctx->uiDecIFrames++;
            break;

        case FF_P_TYPE:
            v->s.avctx->uiDecPFrames++;
            break;

        case FF_B_TYPE:
            v->s.avctx->uiDecBFrames++;
            break;

        default:
            break;
    }

    v->bi_type = 0;

    if (v->s.pict_type == FF_B_TYPE)
    {
        v->bfraction_lut_index = get_vlc2(gb, ff_vc1_bfraction_vlc.table, VC1_BFRACTION_VLC_BITS, 1);
        v->bfraction = ff_vc1_bfraction_lut[v->bfraction_lut_index];

        if (v->bfraction == 0)
        {
            v->s.pict_type = FF_BI_TYPE;
        }
    }

    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
    {
        /* 2010/06/19 9:30:00 liuxw+00139685 */
        /* buffer fullness在解码过程中虽然不使用，但依然要对其合法性进行判断 */
        //		skip_bits(gb, 7); // skip buffer fullness
        int tmp;
        tmp = get_bits(gb, 7);

        if (100 < tmp)
        {
            av_log(v->s.avctx, AV_LOG_WARNING, "Buffer fullness[%d] is out of range[0-100]!\n", tmp);
            v->s.avctx->iTotalError++;
            IMEDIA_SET_ERR_SLICE(v->s.avctx->iErrorCode, IMEDIA_ERR_SLICE_MARKER);
        }
    }

    /* calculate RND */
    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
    { v->rnd = 1; }

    if (v->s.pict_type == FF_P_TYPE)
    { v->rnd ^= 1; }

    /* Quantizer stuff */
    pqindex = get_bits(gb, 5);

    if (!pqindex)
    {
        /* 2010/06/19 9:30:00 liuxw+00139685 */
        /* 对qpindex的合法性进行判断，并进行修正 */
        av_log(v->s.avctx, AV_LOG_WARNING, "pqindex[0] is out of range[1-31]!\n", pqindex);
        v->s.avctx->iTotalError++;
        IMEDIA_SET_ERR_SLICE(v->s.avctx->iErrorCode, IMEDIA_ERR_SLICE_QP);
        pqindex = 16;
        //		return -1;
    }

    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
    { v->pq = ff_vc1_pquant_table[0][pqindex]; }
    else
    { v->pq = ff_vc1_pquant_table[1][pqindex]; }

    v->pquantizer = 1;

    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
    { v->pquantizer = pqindex < 9; }

    if (v->quantizer_mode == QUANT_NON_UNIFORM)
    { v->pquantizer = 0; }

    v->pqindex = pqindex;

    if (pqindex < 9)
    { v->halfpq = get_bits1(gb); }
    else
    { v->halfpq = 0; }

    if (v->quantizer_mode == QUANT_FRAME_EXPLICIT)
    { v->pquantizer = get_bits1(gb); }

    v->dquantfrm = 0;

    if (v->extended_mv == 1)
    { v->mvrange = get_unary(gb, 0, 3); }

    v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
    v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
    v->range_x = 1 << (v->k_x - 1);
    v->range_y = 1 << (v->k_y - 1);

    /* 2010/06/23 9:30:00 liuxw+00139685 */
    /* 除了B帧之外BI帧也是不支持multires的 */
    //  if (v->multires && v->s.pict_type != FF_B_TYPE)
    if (v->multires && (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_P_TYPE))
    {
        v->respic = get_bits(gb, 2);

        /* 2010/06/23 18:30:00 liuxw+00139685 */
        /* 目前不支持动态调整分辨率的功能 */
        if (v->respic != 0)
        {
            av_log(v->s.avctx, AV_LOG_WARNING, "Not support multi-resolution!\n");
            v->s.avctx->iTotalError++;
            IMEDIA_SET_ERR_PIC(v->s.avctx->iTotalError, IMEDIA_ERR_PIC_OTHER);
            v->respic = 0;
        }
    }

    if (v->res_x8 && (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE))
    {
        v->x8_type = get_bits1(gb);
    }
    else
    { v->x8_type = 0; }

    //av_log(v->s.avctx, AV_LOG_INFO, "%c Frame: QP=[%i]%i (+%i/2) %i\n",
    //        (v->s.pict_type == FF_P_TYPE) ? 'P' : ((v->s.pict_type == FF_I_TYPE) ? 'I' : 'B'), pqindex, v->pq, v->halfpq, v->rangeredfrm);

    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_P_TYPE)
    { v->use_ic = 0; }

    switch (v->s.pict_type)
    {
        case FF_P_TYPE:
            if (v->pq < 5)
            { v->tt_index = 0; }
            else if (v->pq < 13)
            { v->tt_index = 1; }
            else
            { v->tt_index = 2; }

            lowquant = (v->pq > 12) ? 0 : 1;
            v->mv_mode = ff_vc1_mv_pmode_table[lowquant][get_unary(gb, 1, 4)];

            if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
            {
                int scale, shift, i;
                v->mv_mode2 = ff_vc1_mv_pmode_table2[lowquant][get_unary(gb, 1, 3)];
                v->lumscale = get_bits(gb, 6);
                v->lumshift = get_bits(gb, 6);
                v->use_ic = 1;

                /* fill lookup tables for intensity compensation */
                if (!v->lumscale)
                {
                    scale = -64;
                    shift = (255 - v->lumshift * 2) << 6;

                    if (v->lumshift > 31)
                    { shift += (128 << 6); }
                }
                else
                {
                    scale = v->lumscale + 32;

                    if (v->lumshift > 31)
                    { shift = (v->lumshift - 64) << 6; }
                    else
                    { shift = v->lumshift << 6; }
                }

                for (i = 0; i < 256; i++)
                {
                    v->luty[i] = av_clip_uint8((scale * i + shift + 32) >> 6);
                    v->lutuv[i] = av_clip_uint8((scale * (i - 128) + 128 * 64 + 32) >> 6);
                }
            }

            if (v->mv_mode == MV_PMODE_1MV_HPEL || v->mv_mode == MV_PMODE_1MV_HPEL_BILIN)
            { v->s.quarter_sample = 0; }
            else if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
            {
                if (v->mv_mode2 == MV_PMODE_1MV_HPEL || v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN)
                { v->s.quarter_sample = 0; }
                else
                { v->s.quarter_sample = 1; }
            }
            else
            { v->s.quarter_sample = 1; }

            v->s.mspel = !(v->mv_mode == MV_PMODE_1MV_HPEL_BILIN || (v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN));

            if ((v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_MIXED_MV) || v->mv_mode == MV_PMODE_MIXED_MV)
            {
                status = bitplane_decoding(v->mv_type_mb_plane, &v->mv_type_is_raw, v);  //decode MVTYPEMB

                if (status < 0)
                { return -1; }

                av_log(v->s.avctx, AV_LOG_DEBUG, "MB MV Type plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);
            }
            else
            {
                v->mv_type_is_raw = 0;
                memset(v->mv_type_mb_plane, 0, v->s.mb_stride * v->s.mb_height);
            }

            status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);

            if (status < 0)
            { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);

            /* Hopefully this is correct for P frames */
            v->s.mv_table_index = get_bits(gb, 2); //but using ff_vc1_ tables
#ifdef LXW_DEBUG
            tmp = get_bits(gb, 2);
            v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[tmp];
#else
            v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];
#endif

            if (v->dquant)
            {
                av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
                vop_dquant_decoding(v);
            }

            v->ttfrm = 0; //FIXME Is that so ?

            if (v->vstransform)
            {
                v->ttmbf = get_bits1(gb);

                if (v->ttmbf)
                {
                    v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
                }
            }
            else
            {
                v->ttmbf = 1;
                v->ttfrm = TT_8X8;
            }

            break;

        case FF_B_TYPE:
            if (v->pq < 5)
            { v->tt_index = 0; }
            else if (v->pq < 13)
            { v->tt_index = 1; }
            else
            { v->tt_index = 2; }

            lowquant = (v->pq > 12) ? 0 : 1;
            v->mv_mode = get_bits1(gb) ? MV_PMODE_1MV : MV_PMODE_1MV_HPEL_BILIN;
            v->s.quarter_sample = (v->mv_mode == MV_PMODE_1MV);
            v->s.mspel = v->s.quarter_sample;

            status = bitplane_decoding(v->direct_mb_plane, &v->dmb_is_raw, v);

            if (status < 0)
            { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Direct Type plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);
            status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);

            if (status < 0)
            { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);

            v->s.mv_table_index = get_bits(gb, 2);
            v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

            if (v->dquant)
            {
                av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
                vop_dquant_decoding(v);
            }

            v->ttfrm = 0;

            if (v->vstransform)
            {
                v->ttmbf = get_bits1(gb);

                if (v->ttmbf)
                {
                    v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
                }
            }
            else
            {
                v->ttmbf = 1;
                v->ttfrm = TT_8X8;
            }

            break;

            /* 2010/06/23 10:30:00 liuxw+00139685 */
            /* 添加default */
        default:
            break;
    }

    if (!v->x8_type)
    {
        /* AC Syntax */
        v->c_ac_table_index = decode012(gb);

        if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
        {
            v->y_ac_table_index = decode012(gb);
        }

        /* DC Syntax */
        v->s.dc_table_index = get_bits1(gb);
    }

    if (v->s.pict_type == FF_BI_TYPE)
    {
        v->s.pict_type = FF_B_TYPE;
        v->bi_type = 1;
    }

#ifdef LXW_DEBUG

    if (frame_num == TARGET_FRAME)
    {
        av_log(v->s.avctx, AV_LOG_WARNING, "Rangeredfrm:%d pic_type:%d bi_type:%d BFraction:%d PQindex:%d HalfQP:%d PQuantizer:%d QP:%d MVRange:(%d,%d) Respic:%d MVMode:%d  \
										 MVMode2:%d LumScale:%d LumShift:%d use_ic:%d MVTab:%d CBPTab:%d TTMBF:%d TTFRM:%d TRANSCFRM:%d TRANSACFRM2:%d TRANSDCTAB:%d quarter_sample:%d\n",
               v->rangeredfrm, v->s.pict_type, v->bi_type, v->bfraction, pqindex, v->halfpq, v->pquantizer, v->pq, v->range_x, v->range_y, v->respic,
               v->mv_mode, v->mv_mode2, v->lumscale, v->lumshift, v->use_ic, v->s.mv_table_index, tmp, v->ttmbf, v->ttfrm,
               v->c_ac_table_index, v->y_ac_table_index, v->s.dc_table_index, v->s.quarter_sample);
    }

#endif

    return 0;
}

static int vc1_parse_frame_header_adv(VC1Context *v, GetBitContext *gb)
{
    int pqindex, lowquant;
    int status;

    v->p_frame_skipped = 0;

    if (v->interlace)
    {
        v->fcm = decode012(gb);

        if (v->fcm)
        {
            av_log(v->s.avctx, AV_LOG_WARNING, "Not support interlaced frame or interlaced field!\n");
            return -1; // interlaced frames/fields are not implemented
        }
    }

    switch (get_unary(gb, 0, 4))
    {
        case 0:
            v->s.pict_type = FF_P_TYPE;
            break;

        case 1:
            v->s.pict_type = FF_B_TYPE;
            break;

        case 2:
            v->s.pict_type = FF_I_TYPE;
            break;

        case 3:
            v->s.pict_type = FF_BI_TYPE;
            break;

        case 4:
            v->s.pict_type = FF_P_TYPE; // skipped pic
            v->p_frame_skipped = 1;
            return 0;
    }

    if (v->tfcntrflag)
    { skip_bits(gb, 8); }

    if (v->broadcast)
    {
        if (!v->interlace || v->psf)
        {
            v->rptfrm = get_bits(gb, 2);
        }
        else
        {
            v->tff = get_bits1(gb);
            v->rptfrm = get_bits1(gb);
        }
    }

    if (v->panscanflag)
    {
        //...
    }

    v->rnd = get_bits1(gb);

    if (v->interlace)
    { v->uvsamp = get_bits1(gb); }

    if (v->finterpflag) { v->interpfrm = get_bits1(gb); }

    if (v->s.pict_type == FF_B_TYPE)
    {
        v->bfraction_lut_index = get_vlc2(gb, ff_vc1_bfraction_vlc.table, VC1_BFRACTION_VLC_BITS, 1);
        v->bfraction = ff_vc1_bfraction_lut[v->bfraction_lut_index];

        if (v->bfraction == 0)
        {
            v->s.pict_type = FF_BI_TYPE; /* XXX: should not happen here */
        }
    }

    pqindex = get_bits(gb, 5);

    if (!pqindex) { return -1; }

    v->pqindex = pqindex;

    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
    { v->pq = ff_vc1_pquant_table[0][pqindex]; }
    else
    { v->pq = ff_vc1_pquant_table[1][pqindex]; }

    v->pquantizer = 1;

    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
    { v->pquantizer = pqindex < 9; }

    if (v->quantizer_mode == QUANT_NON_UNIFORM)
    { v->pquantizer = 0; }

    v->pqindex = pqindex;

    if (pqindex < 9) { v->halfpq = get_bits1(gb); }
    else { v->halfpq = 0; }

    if (v->quantizer_mode == QUANT_FRAME_EXPLICIT)
    { v->pquantizer = get_bits1(gb); }

    if (v->postprocflag)
    { v->postproc = get_bits(gb, 2); }

    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_P_TYPE) { v->use_ic = 0; }

    switch (v->s.pict_type)
    {
        case FF_I_TYPE:
        case FF_BI_TYPE:
            status = bitplane_decoding(v->acpred_plane, &v->acpred_is_raw, v);

            if (status < 0) { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "ACPRED plane encoding: "
                   "Imode: %i, Invert: %i\n", status >> 1, status & 1);
            v->condover = CONDOVER_NONE;

            if (v->overlap && v->pq <= 8)
            {
                v->condover = decode012(gb);

                if (v->condover == CONDOVER_SELECT)
                {
                    status = bitplane_decoding(v->over_flags_plane, &v->overflg_is_raw, v);

                    if (status < 0) { return -1; }

                    av_log(v->s.avctx, AV_LOG_DEBUG, "CONDOVER plane encoding: "
                           "Imode: %i, Invert: %i\n", status >> 1, status & 1);
                }
            }

            break;

        case FF_P_TYPE:
            if (v->extended_mv) { v->mvrange = get_unary(gb, 0, 3); }
            else { v->mvrange = 0; }

            v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
            v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
            v->range_x = 1 << (v->k_x - 1);
            v->range_y = 1 << (v->k_y - 1);

            if (v->pq < 5) { v->tt_index = 0; }
            else if (v->pq < 13) { v->tt_index = 1; }
            else { v->tt_index = 2; }

            lowquant = (v->pq > 12) ? 0 : 1;
            v->mv_mode = ff_vc1_mv_pmode_table[lowquant][get_unary(gb, 1, 4)];

            if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
            {
                int scale, shift, i;
                v->mv_mode2 = ff_vc1_mv_pmode_table2[lowquant][get_unary(gb, 1, 3)];
                v->lumscale = get_bits(gb, 6);
                v->lumshift = get_bits(gb, 6);

                /* fill lookup tables for intensity compensation */
                if (!v->lumscale)
                {
                    scale = -64;
                    shift = (255 - v->lumshift * 2) << 6;

                    if (v->lumshift > 31)
                    { shift += 128 << 6; }
                }
                else
                {
                    scale = v->lumscale + 32;

                    if (v->lumshift > 31)
                    { shift = (v->lumshift - 64) << 6; }
                    else
                    { shift = v->lumshift << 6; }
                }

                for (i = 0; i < 256; i++)
                {
                    v->luty[i] = av_clip_uint8((scale * i + shift + 32) >> 6);
                    v->lutuv[i] = av_clip_uint8((scale * (i - 128) + 128 * 64 + 32) >> 6);
                }

                v->use_ic = 1;
            }

            if (v->mv_mode == MV_PMODE_1MV_HPEL || v->mv_mode == MV_PMODE_1MV_HPEL_BILIN)
            { v->s.quarter_sample = 0; }
            else if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
            {
                if (v->mv_mode2 == MV_PMODE_1MV_HPEL || v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN)
                { v->s.quarter_sample = 0; }
                else
                { v->s.quarter_sample = 1; }
            }
            else
            { v->s.quarter_sample = 1; }

            v->s.mspel = !(v->mv_mode == MV_PMODE_1MV_HPEL_BILIN || (v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN));

            if ((v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_MIXED_MV) || v->mv_mode == MV_PMODE_MIXED_MV)
            {
                status = bitplane_decoding(v->mv_type_mb_plane, &v->mv_type_is_raw, v);

                if (status < 0)
                { return -1; }

                av_log(v->s.avctx, AV_LOG_DEBUG, "MB MV Type plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);
            }
            else
            {
                v->mv_type_is_raw = 0;
                memset(v->mv_type_mb_plane, 0, v->s.mb_stride * v->s.mb_height);
            }

            status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);

            if (status < 0)
            { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: ""Imode: %i, Invert: %i\n", status >> 1, status & 1);

            /* Hopefully this is correct for P frames */
            v->s.mv_table_index = get_bits(gb, 2); //but using ff_vc1_ tables
            v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

            if (v->dquant)
            {
                av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
                vop_dquant_decoding(v);
            }

            v->ttfrm = 0; //FIXME Is that so ?

            if (v->vstransform)
            {
                v->ttmbf = get_bits1(gb);

                if (v->ttmbf)
                {
                    v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
                }
            }
            else
            {
                v->ttmbf = 1;
                v->ttfrm = TT_8X8;
            }

            break;

        case FF_B_TYPE:
            if (v->extended_mv) { v->mvrange = get_unary(gb, 0, 3); }
            else { v->mvrange = 0; }

            v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
            v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
            v->range_x = 1 << (v->k_x - 1);
            v->range_y = 1 << (v->k_y - 1);

            if (v->pq < 5) { v->tt_index = 0; }
            else if (v->pq < 13) { v->tt_index = 1; }
            else { v->tt_index = 2; }

            lowquant = (v->pq > 12) ? 0 : 1;
            v->mv_mode = get_bits1(gb) ? MV_PMODE_1MV : MV_PMODE_1MV_HPEL_BILIN;
            v->s.quarter_sample = (v->mv_mode == MV_PMODE_1MV);
            v->s.mspel = v->s.quarter_sample;

            status = bitplane_decoding(v->direct_mb_plane, &v->dmb_is_raw, v);

            if (status < 0) { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Direct Type plane encoding: "
                   "Imode: %i, Invert: %i\n", status >> 1, status & 1);
            status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);

            if (status < 0) { return -1; }

            av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: "
                   "Imode: %i, Invert: %i\n", status >> 1, status & 1);

            v->s.mv_table_index = get_bits(gb, 2);
            v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

            if (v->dquant)
            {
                av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
                vop_dquant_decoding(v);
            }

            v->ttfrm = 0;

            if (v->vstransform)
            {
                v->ttmbf = get_bits1(gb);

                if (v->ttmbf)
                {
                    v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
                }
            }
            else
            {
                v->ttmbf = 1;
                v->ttfrm = TT_8X8;
            }

            break;
    }

    /* AC Syntax */
    v->c_ac_table_index = decode012(gb);

    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
    {
        v->y_ac_table_index = decode012(gb);
    }

    /* DC Syntax */
    v->s.dc_table_index = get_bits1(gb);

    if ((v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE) && v->dquant)
    {
        av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
        vop_dquant_decoding(v);
    }

    v->bi_type = 0;

    if (v->s.pict_type == FF_BI_TYPE)
    {
        v->s.pict_type = FF_B_TYPE;
        v->bi_type = 1;
    }

    return 0;
}

/***********************************************************************/
/**
 * @defgroup vc1block VC-1 Block-level functions
 * @see 7.1.4, p91 and 8.1.1.7, p(1)04
 * @{
 */

/**
 * @def GET_MQUANT
 * @brief Get macroblock-level quantizer scale
 */
/* 2010/08/31 19:00:00 liuxw+00139685 [AZ1D02293] */
/* 由于当pic_qp和altpq相等时，v->pq不知道是由谁得到，后面在计算qscale时是根据v->pq是否等于mqp来判断是否使用halfqp */
#if 0
#define GET_MQUANT()                                           \
    if (v->dquantfrm)                                            \
    {                                                            \
        int edges = 0;                                             \
        if (v->dqprofile == DQPROFILE_ALL_MBS)                     \
        {                                                          \
            if (v->dqbilevel)                                        \
            {                                                        \
                mquant = (get_bits1(gb)) ? v->altpq : v->pq;           \
            }                                                        \
            else                                                     \
            {                                                        \
                mqdiff = get_bits(gb, 3);                              \
                if (mqdiff != 7) mquant = v->pq + mqdiff;              \
                else mquant = get_bits(gb, 5);                         \
            }                                                        \
        }                                                          \
        if(v->dqprofile == DQPROFILE_SINGLE_EDGE)                  \
            edges = 1 << v->dqsbedge;                              \
        else if(v->dqprofile == DQPROFILE_DOUBLE_EDGES)            \
            edges = (3 << v->dqsbedge) % 15;                       \
        else if(v->dqprofile == DQPROFILE_FOUR_EDGES)              \
            edges = 15;                                            \
        if((edges&1) && !s->mb_x)                                  \
            mquant = v->altpq;                                     \
        if((edges&2) && s->first_slice_line)                       \
            mquant = v->altpq;                                     \
        if((edges&4) && s->mb_x == (s->mb_width - 1))              \
            mquant = v->altpq;                                     \
        if((edges&8) && s->mb_y == (s->mb_height - 1))             \
            mquant = v->altpq;                                     \
    }
#else
#define GET_MQUANT()                                           \
    if (v->dquantfrm)                                            \
    {                                                            \
        int ii = 0;                                                \
        int edges = 0;                                             \
        if (v->dqprofile == DQPROFILE_ALL_MBS)                     \
        {                                                          \
            if (v->dqbilevel)                                        \
            {                                                        \
                ii = get_bits1(gb);								       \
                mquant = ii ? v->altpq : v->pq;                        \
                v->useAltQP = ii;                                     \
            }                                                        \
            else                                                     \
            {                                                        \
                mqdiff = get_bits(gb, 3);                              \
                if (mqdiff != 7) mquant = v->pq + mqdiff;              \
                else mquant = get_bits(gb, 5);                         \
                v->useAltQP = 1;                                       \
            }                                                        \
        }                                                          \
        if(v->dqprofile == DQPROFILE_SINGLE_EDGE)                  \
            edges = 1 << v->dqsbedge;                              \
        else if(v->dqprofile == DQPROFILE_DOUBLE_EDGES)            \
            edges = (3 << v->dqsbedge) % 15;                       \
        else if(v->dqprofile == DQPROFILE_FOUR_EDGES)              \
            edges = 15;                                            \
        if((edges&1) && !s->mb_x)                                  \
        {														   \
            mquant = v->altpq;                                     \
            v->useAltQP = 1;									   \
        }                                                          \
        if((edges&2) && s->first_slice_line)                       \
        {														   \
            v->useAltQP = 1;                                       \
            mquant = v->altpq;                                     \
        }														   \
        if((edges&4) && s->mb_x == (s->mb_width - 1))              \
        {                                                          \
            v->useAltQP = 1;                                       \
            mquant = v->altpq;                                     \
        }                                                          \
        if((edges&8) && s->mb_y == (s->mb_height - 1))             \
        {														   \
            v->useAltQP = 1;                                        \
            mquant = v->altpq;                                     \
        }														   \
    }
#endif
/**
 * @def GET_MVDATA(_dmv_x, _dmv_y)
 * @brief Get MV differentials
 * @see MVDATA decoding from 8.3.5.2, p(1)20
 * @param _dmv_x Horizontal differential for decoded MV
 * @param _dmv_y Vertical differential for decoded MV
 */
#define GET_MVDATA(_dmv_x, _dmv_y)                                  \
    index = 1 + get_vlc2(gb, ff_vc1_mv_diff_vlc[s->mv_table_index].table,\
                         VC1_MV_DIFF_VLC_BITS, 2);                    \
    if (index > 36)                                                   \
    {                                                                 \
        mb_has_coeffs = 1;                                              \
        index -= 37;                                                    \
    }                                                                 \
    else mb_has_coeffs = 0;                                           \
    s->mb_intra = 0;                                                  \
    if (!index) { _dmv_x = _dmv_y = 0; }                              \
    else if (index == 35)                                             \
    {                                                                 \
        _dmv_x = get_bits(gb, v->k_x - 1 + s->quarter_sample);          \
        _dmv_y = get_bits(gb, v->k_y - 1 + s->quarter_sample);          \
    }                                                                 \
    else if (index == 36)                                             \
    {                                                                 \
        _dmv_x = 0;                                                     \
        _dmv_y = 0;                                                     \
        s->mb_intra = 1;                                                \
    }                                                                 \
    else                                                              \
    {                                                                 \
        index1 = index%6;                                               \
        if (!s->quarter_sample && index1 == 5) val = 1;                 \
        else                                   val = 0;                 \
        if(size_table[index1] - val > 0)                                \
            val = get_bits(gb, size_table[index1] - val);               \
        else                                   val = 0;                 \
        sign = 0 - (val&1);                                             \
        _dmv_x = (sign ^ ((val>>1) + offset_table[index1])) - sign;     \
        \
        index1 = index/6;                                               \
        if (!s->quarter_sample && index1 == 5) val = 1;                 \
        else                                   val = 0;                 \
        if(size_table[index1] - val > 0)                                \
            val = get_bits(gb, size_table[index1] - val);               \
        else                                   val = 0;                 \
        sign = 0 - (val&1);                                             \
        _dmv_y = (sign ^ ((val>>1) + offset_table[index1])) - sign;     \
    }

/** Predict and set motion vector
 */
static inline void vc1_pred_mv(MpegEncContext *s, int n, int dmv_x, int dmv_y, int mv1, int r_x, int r_y, uint8_t *is_intra)
{
    int xy, wrap, off = 0;
    int16_t *A, *B, *C;
    int px, py;
    int sum;

    /* scale MV difference to be quad-pel */
    dmv_x <<= 1 - s->quarter_sample;
    dmv_y <<= 1 - s->quarter_sample;

    wrap = s->b8_stride;
    xy = s->block_index[n];

    if (s->mb_intra)
    {
        s->mv[0][n][0] = s->current_picture.motion_val[0][xy][0] = 0;
        s->mv[0][n][1] = s->current_picture.motion_val[0][xy][1] = 0;
        s->current_picture.motion_val[1][xy][0] = 0;
        s->current_picture.motion_val[1][xy][1] = 0;

        if (mv1)
        {
            /* duplicate motion data for 1-MV block */
            s->current_picture.motion_val[0][xy + 1][0] = 0;
            s->current_picture.motion_val[0][xy + 1][1] = 0;
            s->current_picture.motion_val[0][xy + wrap][0] = 0;
            s->current_picture.motion_val[0][xy + wrap][1] = 0;
            s->current_picture.motion_val[0][xy + wrap + 1][0] = 0;
            s->current_picture.motion_val[0][xy + wrap + 1][1] = 0;
            s->current_picture.motion_val[1][xy + 1][0] = 0;
            s->current_picture.motion_val[1][xy + 1][1] = 0;
            s->current_picture.motion_val[1][xy + wrap][0] = 0;
            s->current_picture.motion_val[1][xy + wrap][1] = 0;
            s->current_picture.motion_val[1][xy + wrap + 1][0] = 0;
            s->current_picture.motion_val[1][xy + wrap + 1][1] = 0;
        }

        return;
    }

    C = s->current_picture.motion_val[0][xy - 1];
    A = s->current_picture.motion_val[0][xy - wrap];

    if (mv1)
    { off = (s->mb_x == (s->mb_width - 1)) ? -1 : 2; }
    else
    {
        //in 4-MV mode different blocks have different B predictor position
        switch (n)
        {
            case 0:
                off = (s->mb_x > 0) ? -1 : 1;
                break;

            case 1:
                off = (s->mb_x == (s->mb_width - 1)) ? -1 : 1;
                break;

            case 2:
                off = 1;
                break;

            case 3:
                off = -1;
        }
    }

    B = s->current_picture.motion_val[0][xy - wrap + off];

    if (!s->first_slice_line || (n == 2 || n == 3))
    {
        // predictor A is not out of bounds
        if (s->mb_width == 1)
        {
            px = A[0];
            py = A[1];
        }
        else
        {
            px = mid_pred(A[0], B[0], C[0]);
            py = mid_pred(A[1], B[1], C[1]);
        }
    }
    else if (s->mb_x || (n == 1 || n == 3))
    {
        // predictor C is not out of bounds
        px = C[0];
        py = C[1];
    }
    else
    {
        px = py = 0;
    }

    /* Pullback MV as specified in 8.3.5.3.4 */
    {
        int qx, qy, X, Y;
        qx = (s->mb_x << 6) + ((n == 1 || n == 3) ? 32 : 0);
        qy = (s->mb_y << 6) + ((n == 2 || n == 3) ? 32 : 0);
        X = (s->mb_width << 6) - 4;
        Y = (s->mb_height << 6) - 4;

        if (mv1)
        {
            if (qx + px < -60) { px = -60 - qx; }

            if (qy + py < -60) { py = -60 - qy; }
        }
        else
        {
            if (qx + px < -28) { px = -28 - qx; }

            if (qy + py < -28) { py = -28 - qy; }
        }

        if (qx + px > X) { px = X - qx; }

        if (qy + py > Y) { py = Y - qy; }
    }

    /* Calculate hybrid prediction as specified in 8.3.5.3.5 */
    if ((!s->first_slice_line || (n == 2 || n == 3)) && (s->mb_x || (n == 1 || n == 3)))
    {
        if (is_intra[xy - wrap])
        { sum = FFABS(px) + FFABS(py); }
        else
        { sum = FFABS(px - A[0]) + FFABS(py - A[1]); }

        if (sum > 32)
        {
            if (get_bits1(&s->gb)) //HYBRIDPRED liuxw+00139685
            {
                px = A[0];
                py = A[1];
            }
            else
            {
                px = C[0];
                py = C[1];
            }
        }
        else
        {
            if (is_intra[xy - 1])
            { sum = FFABS(px) + FFABS(py); }
            else
            { sum = FFABS(px - C[0]) + FFABS(py - C[1]); }

            if (sum > 32)
            {
                if (get_bits1(&s->gb)) //HYBRIDPRED liuxw+00139685
                {
                    px = A[0];
                    py = A[1];
                }
                else
                {
                    px = C[0];
                    py = C[1];
                }
            }
        }
    }

    /* store MV using signed modulus of MV range defined in 4.11 */
    s->mv[0][n][0] = s->current_picture.motion_val[0][xy][0] = ((px + dmv_x + r_x) & ((r_x << 1) - 1)) - r_x;
    s->mv[0][n][1] = s->current_picture.motion_val[0][xy][1] = ((py + dmv_y + r_y) & ((r_y << 1) - 1)) - r_y;

    if (mv1)
    {
        /* duplicate motion data for 1-MV block */
        s->current_picture.motion_val[0][xy + 1][0] = s->current_picture.motion_val[0][xy][0];
        s->current_picture.motion_val[0][xy + 1][1] = s->current_picture.motion_val[0][xy][1];
        s->current_picture.motion_val[0][xy + wrap][0] = s->current_picture.motion_val[0][xy][0];
        s->current_picture.motion_val[0][xy + wrap][1] = s->current_picture.motion_val[0][xy][1];
        s->current_picture.motion_val[0][xy + wrap + 1][0] = s->current_picture.motion_val[0][xy][0];
        s->current_picture.motion_val[0][xy + wrap + 1][1] = s->current_picture.motion_val[0][xy][1];
    }
}

/** Motion compensation for direct or interpolated blocks in B-frames
 */
static void vc1_interp_mc(VC1Context *v)
{
    MpegEncContext *s = &v->s;
    DSPContext *dsp = &v->s.dsp;
    uint8_t *srcY, *srcU, *srcV;
    int dxy, uvdxy, mx, my, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;

    if (!v->s.next_picture.data[0]) { return; }

    mx = s->mv[1][0][0];
    my = s->mv[1][0][1];
    uvmx = (mx + ((mx & 3) == 3)) >> 1;
    uvmy = (my + ((my & 3) == 3)) >> 1;

    if (v->fastuvmc)
    {
        uvmx = uvmx + ((uvmx < 0) ? -(uvmx & 1) : (uvmx & 1));
        uvmy = uvmy + ((uvmy < 0) ? -(uvmy & 1) : (uvmy & 1));
    }

    srcY = s->next_picture.data[0];
    srcU = s->next_picture.data[1];
    srcV = s->next_picture.data[2];

    src_x = s->mb_x * 16 + (mx >> 2);
    src_y = s->mb_y * 16 + (my >> 2);
    uvsrc_x = s->mb_x * 8 + (uvmx >> 2);
    uvsrc_y = s->mb_y * 8 + (uvmy >> 2);

    if (v->profile != PROFILE_ADVANCED)
    {
        src_x   = av_clip(  src_x, -16, s->mb_width  * 16);
        src_y   = av_clip(  src_y, -16, s->mb_height * 16);
        uvsrc_x = av_clip(uvsrc_x,  -8, s->mb_width  *  8);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->mb_height *  8);
    }
    else
    {
        src_x   = av_clip(  src_x, -17, s->avctx->coded_width);
        src_y   = av_clip(  src_y, -18, s->avctx->coded_height + 1);
        uvsrc_x = av_clip(uvsrc_x,  -8, s->avctx->coded_width  >> 1);
        uvsrc_y = av_clip(uvsrc_y,  -8, s->avctx->coded_height >> 1);
    }

    srcY += src_y * s->linesize + src_x;
    srcU += uvsrc_y * s->uvlinesize + uvsrc_x;
    srcV += uvsrc_y * s->uvlinesize + uvsrc_x;

    /* for grayscale we should not try to read from unknown area */
    if (s->flags & CODEC_FLAG_GRAY)
    {
        srcU = s->edge_emu_buffer + 18 * s->linesize;
        srcV = s->edge_emu_buffer + 18 * s->linesize;
    }

    /* 2010/05/27 10:00:00 liuxw+00139685 */
    /* 消除编译warning(在>的右边强制转化为unsigned) */
    if (v->rangeredfrm
        || (unsigned)src_x > (unsigned)(s->h_edge_pos - (mx & 3) - 16)
        || (unsigned)src_y > (unsigned)(s->v_edge_pos - (my & 3) - 16))
    {
        uint8_t *uvbuf = s->edge_emu_buffer + 19 * s->linesize;

        srcY -= s->mspel * (1 + s->linesize);
        ff_emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 17 + s->mspel * 2, 17 + s->mspel * 2,
                            src_x - s->mspel, src_y - s->mspel, s->h_edge_pos, s->v_edge_pos);
        srcY = s->edge_emu_buffer;
        ff_emulated_edge_mc(uvbuf     , srcU, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        ff_emulated_edge_mc(uvbuf + 16, srcV, s->uvlinesize, 8 + 1, 8 + 1,
                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
        srcU = uvbuf;
        srcV = uvbuf + 16;
        /* 2010/08/26 10:00:00 liuxw+00139685 */
        /* rangeredfrm功能使用错误 */
        /* if we deal with range reduction we need to scale source blocks */
        /*        if(v->rangeredfrm)
        		{
                    int i, j;
                    uint8_t *src, *src2;

                    src = srcY;
                    for(j = 0; j < 17 + s->mspel*2; j++)
        			{
                        for(i = 0; i < 17 + s->mspel*2; i++) src[i] = ((src[i] - 128) >> 1) + 128;
                        src += s->linesize;
                    }
                    src = srcU; src2 = srcV;
                    for(j = 0; j < 9; j++)
        			{
                        for(i = 0; i < 9; i++)
        				{
                            src[i] = ((src[i] - 128) >> 1) + 128;
                            src2[i] = ((src2[i] - 128) >> 1) + 128;
                        }
                        src += s->uvlinesize;
                        src2 += s->uvlinesize;
                    }
                } */
        srcY += s->mspel * (1 + s->linesize);
    }

    mx >>= 1;
    my >>= 1;
    dxy = ((my & 1) << 1) | (mx & 1);

    dsp->avg_pixels_tab[0][dxy](s->dest[0], srcY, s->linesize, 16);

    if (s->flags & CODEC_FLAG_GRAY) { return; }

    /* Chroma MC always uses qpel blilinear */
    uvdxy = ((uvmy & 3) << 2) | (uvmx & 3);
    uvmx = (uvmx & 3) << 1;
    uvmy = (uvmy & 3) << 1;
    dsp->avg_h264_chroma_pixels_tab[0](s->dest[1], srcU, s->uvlinesize, 8, uvmx, uvmy);
    dsp->avg_h264_chroma_pixels_tab[0](s->dest[2], srcV, s->uvlinesize, 8, uvmx, uvmy);
}

static av_always_inline int scale_mv(int value, int bfrac, int inv, int qs)
{
    int n = bfrac;

#if B_FRACTION_DEN==256

    if (inv)
    { n -= 256; }

    if (!qs)
    { return 2 * ((value * n + 255) >> 9); }

    return (value * n + 128) >> 8;
#else

    if (inv)
    { n -= B_FRACTION_DEN; }

    if (!qs)
    { return 2 * ((value * n + B_FRACTION_DEN - 1) / (2 * B_FRACTION_DEN)); }

    return (value * n + B_FRACTION_DEN / 2) / B_FRACTION_DEN;
#endif
}

/** Reconstruct motion vector for B-frame and do motion compensation
 */
static inline void vc1_b_mc(VC1Context *v, int dmv_x[2], int dmv_y[2], int direct, int mode)
{
    if (v->use_ic)
    {
        v->mv_mode2 = v->mv_mode;
        v->mv_mode = MV_PMODE_INTENSITY_COMP;
    }

    if (direct)
    {
        vc1_mc_1mv(v, 0);
        vc1_interp_mc(v);

        if (v->use_ic)
        { v->mv_mode = v->mv_mode2; }

        return;
    }

    if (mode == BMV_TYPE_INTERPOLATED)
    {
        vc1_mc_1mv(v, 0);
        vc1_interp_mc(v);

        if (v->use_ic)
        { v->mv_mode = v->mv_mode2; }

        return;
    }

    if (v->use_ic && (mode == BMV_TYPE_BACKWARD))
    { v->mv_mode = v->mv_mode2; }

    vc1_mc_1mv(v, (mode == BMV_TYPE_BACKWARD));

    if (v->use_ic)
    { v->mv_mode = v->mv_mode2; }
}

static inline void vc1_pred_b_mv(VC1Context *v, int dmv_x[2], int dmv_y[2], int direct, int mvtype)
{
    MpegEncContext *s = &v->s;
    int xy, wrap, off = 0;
    int16_t *A, *B, *C;
    int px, py;
    int sum;
    int r_x, r_y;
    const uint8_t *is_intra = v->mb_type[0];

    r_x = v->range_x;
    r_y = v->range_y;
    /* scale MV difference to be quad-pel */
    dmv_x[0] <<= 1 - s->quarter_sample;
    dmv_y[0] <<= 1 - s->quarter_sample;
    dmv_x[1] <<= 1 - s->quarter_sample;
    dmv_y[1] <<= 1 - s->quarter_sample;

    wrap = s->b8_stride;
    xy = s->block_index[0];

    if (s->mb_intra)
    {
        s->current_picture.motion_val[0][xy][0] =
            s->current_picture.motion_val[0][xy][1] =
                s->current_picture.motion_val[1][xy][0] =
                    s->current_picture.motion_val[1][xy][1] = 0;
        return;
    }

    /* 2010/08/31 19:00:00 liuxw+00139685 [AZ1D02293] */
    /* 当计算B宏块的direct的时候会用到后前参考帧相同位置宏块的mv，但在使用之前要先经过一个pull back的操作 协议8.4.5.4 */
    //   s->mv[0][0][0] = scale_mv(s->next_picture.motion_val[1][xy][0], v->bfraction, 0, s->quarter_sample);
    //   s->mv[0][0][1] = scale_mv(s->next_picture.motion_val[1][xy][1], v->bfraction, 0, s->quarter_sample);
    //   s->mv[1][0][0] = scale_mv(s->next_picture.motion_val[1][xy][0], v->bfraction, 1, s->quarter_sample);
    //   s->mv[1][0][1] = scale_mv(s->next_picture.motion_val[1][xy][1], v->bfraction, 1, s->quarter_sample);
    {
        int iX0B, iY0B;
        int mx , my;

        mx = s->next_picture.motion_val[1][xy][0];
        my = s->next_picture.motion_val[1][xy][1];

        iX0B = (s->mb_x << 3) + (mx >> 2);
        iY0B = (s->mb_y << 3) + (my >> 2);

        if (iX0B < -8)
        {
            mx -= (iX0B + 8) * 4;
        }
        else if (iX0B > s->mb_width * 8)
        {
            mx -= (iX0B - s->mb_width * 8) * 4;
        }

        if (iY0B < -8)
        {
            my -= (iY0B + 8) * 4;
        }
        else if (iY0B > s->mb_height * 8)
        {
            my -= (iY0B - s->mb_height * 8) * 4;
        }

        s->mv[0][0][0] = scale_mv(mx, v->bfraction, 0, s->quarter_sample);
        s->mv[0][0][1] = scale_mv(my, v->bfraction, 0, s->quarter_sample);
        s->mv[1][0][0] = scale_mv(mx, v->bfraction, 1, s->quarter_sample);
        s->mv[1][0][1] = scale_mv(my, v->bfraction, 1, s->quarter_sample);
    }

    /* Pullback predicted motion vectors as specified in 8.4.5.4 */
    s->mv[0][0][0] = av_clip(s->mv[0][0][0], -60 - (s->mb_x << 6), (s->mb_width  << 6) - 4 - (s->mb_x << 6));
    s->mv[0][0][1] = av_clip(s->mv[0][0][1], -60 - (s->mb_y << 6), (s->mb_height << 6) - 4 - (s->mb_y << 6));
    s->mv[1][0][0] = av_clip(s->mv[1][0][0], -60 - (s->mb_x << 6), (s->mb_width  << 6) - 4 - (s->mb_x << 6));
    s->mv[1][0][1] = av_clip(s->mv[1][0][1], -60 - (s->mb_y << 6), (s->mb_height << 6) - 4 - (s->mb_y << 6));

    if (direct)
    {
        s->current_picture.motion_val[0][xy][0] = s->mv[0][0][0];
        s->current_picture.motion_val[0][xy][1] = s->mv[0][0][1];
        s->current_picture.motion_val[1][xy][0] = s->mv[1][0][0];
        s->current_picture.motion_val[1][xy][1] = s->mv[1][0][1];
        return;
    }

    if ((mvtype == BMV_TYPE_FORWARD) || (mvtype == BMV_TYPE_INTERPOLATED))
    {
        C = s->current_picture.motion_val[0][xy - 2];
        A = s->current_picture.motion_val[0][xy - wrap * 2];
        off = (s->mb_x == (s->mb_width - 1)) ? -2 : 2;
        B = s->current_picture.motion_val[0][xy - wrap * 2 + off];

        if (!s->mb_x)
        { C[0] = C[1] = 0; }

        if (!s->first_slice_line)
        {
            // predictor A is not out of bounds
            if (s->mb_width == 1)
            {
                px = A[0];
                py = A[1];
            }
            else
            {
                px = mid_pred(A[0], B[0], C[0]);
                py = mid_pred(A[1], B[1], C[1]);
            }
        }
        else if (s->mb_x)
        {
            // predictor C is not out of bounds
            px = C[0];
            py = C[1];
        }
        else
        {
            px = py = 0;
        }

        /* Pullback MV as specified in 8.3.5.3.4 */
        {
            int qx, qy, X, Y;

            /* 2010/07/15 16:30:00 liuxw+00139685 [AZ1D02221] */
            /* 由于profile的宏定义进行过修改，导致main profile比advanced profile大，所以把条件反转过来 */
            //if(v->profile < PROFILE_ADVANCED)
            if (v->profile > PROFILE_ADVANCED)
            {
                qx = (s->mb_x << 5);
                qy = (s->mb_y << 5);
                X = (s->mb_width << 5) - 4;
                Y = (s->mb_height << 5) - 4;

                if (qx + px < -28) { px = -28 - qx; }

                if (qy + py < -28) { py = -28 - qy; }

                if (qx + px > X) { px = X - qx; }

                if (qy + py > Y) { py = Y - qy; }
            }
            else
            {
                qx = (s->mb_x << 6);
                qy = (s->mb_y << 6);
                X = (s->mb_width << 6) - 4;
                Y = (s->mb_height << 6) - 4;

                if (qx + px < -60) { px = -60 - qx; }

                if (qy + py < -60) { py = -60 - qy; }

                if (qx + px > X) { px = X - qx; }

                if (qy + py > Y) { py = Y - qy; }
            }
        }

        /* Calculate hybrid prediction as specified in 8.3.5.3.5 */
        if (0 && !s->first_slice_line && s->mb_x)
        {
            if (is_intra[xy - wrap])
            { sum = FFABS(px) + FFABS(py); }
            else
            { sum = FFABS(px - A[0]) + FFABS(py - A[1]); }

            if (sum > 32)
            {
                if (get_bits1(&s->gb))
                {
                    px = A[0];
                    py = A[1];
                }
                else
                {
                    px = C[0];
                    py = C[1];
                }
            }
            else
            {
                if (is_intra[xy - 2])
                { sum = FFABS(px) + FFABS(py); }
                else
                { sum = FFABS(px - C[0]) + FFABS(py - C[1]); }

                if (sum > 32)
                {
                    if (get_bits1(&s->gb))
                    {
                        px = A[0];
                        py = A[1];
                    }
                    else
                    {
                        px = C[0];
                        py = C[1];
                    }
                }
            }
        }

        /* store MV using signed modulus of MV range defined in 4.11 */
        s->mv[0][0][0] = ((px + dmv_x[0] + r_x) & ((r_x << 1) - 1)) - r_x;
        s->mv[0][0][1] = ((py + dmv_y[0] + r_y) & ((r_y << 1) - 1)) - r_y;
    }

    if ((mvtype == BMV_TYPE_BACKWARD) || (mvtype == BMV_TYPE_INTERPOLATED))
    {
        C = s->current_picture.motion_val[1][xy - 2];
        A = s->current_picture.motion_val[1][xy - wrap * 2];
        off = (s->mb_x == (s->mb_width - 1)) ? -2 : 2;
        B = s->current_picture.motion_val[1][xy - wrap * 2 + off];

        if (!s->mb_x) { C[0] = C[1] = 0; }

        if (!s->first_slice_line)
        {
            // predictor A is not out of bounds
            if (s->mb_width == 1)
            {
                px = A[0];
                py = A[1];
            }
            else
            {
                px = mid_pred(A[0], B[0], C[0]);
                py = mid_pred(A[1], B[1], C[1]);
            }
        }
        else if (s->mb_x)
        {
            // predictor C is not out of bounds
            px = C[0];
            py = C[1];
        }
        else
        {
            px = py = 0;
        }

        /* Pullback MV as specified in 8.3.5.3.4 */
        {
            int qx, qy, X, Y;

            /* 2010/07/15 16:30:00 liuxw+00139685 [AZ1D02221] */
            /* 由于profile的宏定义进行过修改，导致main profile比advanced profile大，所以把条件反转过来 */
            //if(v->profile < PROFILE_ADVANCED)
            if (v->profile > PROFILE_ADVANCED)
            {
                qx = (s->mb_x << 5);
                qy = (s->mb_y << 5);
                X = (s->mb_width << 5) - 4;
                Y = (s->mb_height << 5) - 4;

                if (qx + px < -28) { px = -28 - qx; }

                if (qy + py < -28) { py = -28 - qy; }

                if (qx + px > X) { px = X - qx; }

                if (qy + py > Y) { py = Y - qy; }
            }
            else
            {
                qx = (s->mb_x << 6);
                qy = (s->mb_y << 6);
                X = (s->mb_width << 6) - 4;
                Y = (s->mb_height << 6) - 4;

                if (qx + px < -60) { px = -60 - qx; }

                if (qy + py < -60) { py = -60 - qy; }

                if (qx + px > X) { px = X - qx; }

                if (qy + py > Y) { py = Y - qy; }
            }
        }

        /* Calculate hybrid prediction as specified in 8.3.5.3.5 */
        if (0 && !s->first_slice_line && s->mb_x)
        {
            if (is_intra[xy - wrap])
            { sum = FFABS(px) + FFABS(py); }
            else
            { sum = FFABS(px - A[0]) + FFABS(py - A[1]); }

            if (sum > 32)
            {
                if (get_bits1(&s->gb))
                {
                    px = A[0];
                    py = A[1];
                }
                else
                {
                    px = C[0];
                    py = C[1];
                }
            }
            else
            {
                if (is_intra[xy - 2])
                { sum = FFABS(px) + FFABS(py); }
                else
                { sum = FFABS(px - C[0]) + FFABS(py - C[1]); }

                if (sum > 32)
                {
                    if (get_bits1(&s->gb))
                    {
                        px = A[0];
                        py = A[1];
                    }
                    else
                    {
                        px = C[0];
                        py = C[1];
                    }
                }
            }
        }

        /* store MV using signed modulus of MV range defined in 4.11 */

        s->mv[1][0][0] = ((px + dmv_x[1] + r_x) & ((r_x << 1) - 1)) - r_x;
        s->mv[1][0][1] = ((py + dmv_y[1] + r_y) & ((r_y << 1) - 1)) - r_y;
    }

    s->current_picture.motion_val[0][xy][0] = s->mv[0][0][0];
    s->current_picture.motion_val[0][xy][1] = s->mv[0][0][1];
    s->current_picture.motion_val[1][xy][0] = s->mv[1][0][0];
    s->current_picture.motion_val[1][xy][1] = s->mv[1][0][1];
}

/** Get predicted DC value for I-frames only
 * prediction dir: left=0, top=1
 * @param s MpegEncContext
 * @param overlap flag indicating that overlap filtering is used
 * @param pq integer part of picture quantizer
 * @param[in] n block index in the current MB
 * @param dc_val_ptr Pointer to DC predictor
 * @param dir_ptr Prediction direction for use in AC prediction
 */
static inline int vc1_i_pred_dc(MpegEncContext *s, int overlap, int pq, int n,
                                int16_t **dc_val_ptr, int *dir_ptr)
{
    int a, b, c, wrap, pred, scale;
    int16_t *dc_val;
    static const uint16_t dcpred[32] =
    {
        -1, 1024,  512,  341,  256,  205,  171,  146,  128,
        114,  102,   93,   85,   79,   73,   68,   64,
        60,   57,   54,   51,   49,   47,   45,   43,
        41,   39,   38,   37,   35,   34,   33
    };

    /* find prediction - wmv3_dc_scale always used here in fact */
    if (n < 4)
    { scale = s->y_dc_scale; }
    else
    { scale = s->c_dc_scale; }

    wrap = s->block_wrap[n];
    dc_val = s->dc_val[0] + s->block_index[n];

    /* B A
     * C X
     */
    c = dc_val[ - 1];
    b = dc_val[ - 1 - wrap];
    a = dc_val[ - wrap];

    if (pq < 9 || !overlap)
    {
        /* Set outer values */
        if (s->first_slice_line && (n != 2 && n != 3)) //帧的第一行而且不是宏块的第2、3个块 liuxw+00139685
        { b = a = dcpred[scale]; }

        if (s->mb_x == 0 && (n != 1 && n != 3))   //帧的第一列而且不是宏块的第1、3个块 liuxw+00139685
        { b = c = dcpred[scale]; }
    }
    else
    {
        //if(pq >= 9 && overlap ) liuxw+00139685
        /* Set outer values */
        if (s->first_slice_line && (n != 2 && n != 3)) //帧的第一行而且不是宏块的第2、3个块 liuxw+00139685
        { b = a = 0; }

        if (s->mb_x == 0 && (n != 1 && n != 3))    //帧的第一列而且不是宏块的第1、3个块 liuxw+00139685
        { b = c = 0; }
    }

    if (abs(a - b) <= abs(b - c))
    {
        pred = c;
        *dir_ptr = 1;//left
    }
    else
    {
        pred = a;
        *dir_ptr = 0;//top
    }

    /* update predictor */
    *dc_val_ptr = &dc_val[0];
    return pred;
}


/** Get predicted DC value
 * prediction dir: left=0, top=1
 * @param s MpegEncContext
 * @param overlap flag indicating that overlap filtering is used
 * @param pq integer part of picture quantizer
 * @param[in] n block index in the current MB
 * @param a_avail flag indicating top block availability
 * @param c_avail flag indicating left block availability
 * @param dc_val_ptr Pointer to DC predictor
 * @param dir_ptr Prediction direction for use in AC prediction
 */
static inline int vc1_pred_dc(MpegEncContext *s, int overlap, int pq, int n,
                              int a_avail, int c_avail,
                              int16_t **dc_val_ptr, int *dir_ptr)
{
    int a, b, c, wrap, pred, scale;
    int16_t *dc_val;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int q1, q2 = 0;

    /* find prediction - wmv3_dc_scale always used here in fact */
    if (n < 4)     { scale = s->y_dc_scale; }
    else           { scale = s->c_dc_scale; }

    wrap = s->block_wrap[n];
    dc_val = s->dc_val[0] + s->block_index[n];

    /* B A
     * C X
     */
    c = dc_val[ - 1];
    b = dc_val[ - 1 - wrap];
    a = dc_val[ - wrap];
    /* scale predictors if needed */
    q1 = s->current_picture.qscale_table[mb_pos];

    if (c_avail && (n != 1 && n != 3))
    {
        q2 = s->current_picture.qscale_table[mb_pos - 1];

        if (q2 && q2 != q1)
        { c = (c * s->y_dc_scale_table[q2] * ff_vc1_dqscale[s->y_dc_scale_table[q1] - 1] + 0x20000) >> 18; }
    }

    if (a_avail && (n != 2 && n != 3))
    {
        q2 = s->current_picture.qscale_table[mb_pos - s->mb_stride];

        if (q2 && q2 != q1)
        { a = (a * s->y_dc_scale_table[q2] * ff_vc1_dqscale[s->y_dc_scale_table[q1] - 1] + 0x20000) >> 18; }
    }

    if (a_avail && c_avail && (n != 3))
    {
        int off = mb_pos;

        if (n != 1) { off--; }

        if (n != 2) { off -= s->mb_stride; }

        q2 = s->current_picture.qscale_table[off];

        if (q2 && q2 != q1)
        { b = (b * s->y_dc_scale_table[q2] * ff_vc1_dqscale[s->y_dc_scale_table[q1] - 1] + 0x20000) >> 18; }
    }

    if (a_avail && c_avail)
    {
        if (abs(a - b) <= abs(b - c))
        {
            pred = c;
            *dir_ptr = 1;//left
        }
        else
        {
            pred = a;
            *dir_ptr = 0;//top
        }
    }
    else if (a_avail)
    {
        pred = a;
        *dir_ptr = 0;//top
    }
    else if (c_avail)
    {
        pred = c;
        *dir_ptr = 1;//left
    }
    else
    {
        pred = 0;
        *dir_ptr = 1;//left
    }

    /* update predictor */
    *dc_val_ptr = &dc_val[0];
    return pred;
}

/** @} */ // Block group

/**
 * @defgroup vc1_std_mb VC1 Macroblock-level functions in Simple/Main Profiles
 * @see 7.1.4, p91 and 8.1.1.7, p(1)04
 * @{
 */

static inline int vc1_coded_block_pred(MpegEncContext *s, int n, uint8_t **coded_block_ptr)
{
    int xy, wrap, pred, a, b, c;

    xy = s->block_index[n];
    wrap = s->b8_stride;

    /* B C
     * A X
     */
    a = s->coded_block[xy - 1       ];
    b = s->coded_block[xy - 1 - wrap];
    c = s->coded_block[xy     - wrap];

    if (b == c)
    {
        pred = a;
    }
    else
    {
        pred = c;
    }

    /* store value */
    *coded_block_ptr = &s->coded_block[xy];

    return pred;
}

/**
 * Decode one AC coefficient
 * @param v The VC1 context
 * @param last Last coefficient
 * @param skip How much zero coefficients to skip
 * @param value Decoded AC coefficient value
 * @param codingset set of VLC to decode data
 * @see 8.1.3.4
 */
static void vc1_decode_ac_coeff(VC1Context *v, int *last, int *skip, int *value, int codingset)
{
    GetBitContext *gb = &v->s.gb;
    int index, escape, run = 0, level = 0, lst = 0;

    index = get_vlc2(gb, ff_vc1_ac_coeff_table[codingset].table, AC_VLC_BITS, 3);

    if (index != vc1_ac_sizes[codingset] - 1)
    {
        run = vc1_index_decode_table[codingset][index][0];
        level = vc1_index_decode_table[codingset][index][1];
        lst = index >= vc1_last_decode_table[codingset];

        if (get_bits1(gb))
        { level = -level; }
    }
    else
    {
        escape = decode210(gb);

        if (escape != 2)
        {
            index = get_vlc2(gb, ff_vc1_ac_coeff_table[codingset].table, AC_VLC_BITS, 3);
            run = vc1_index_decode_table[codingset][index][0];
            level = vc1_index_decode_table[codingset][index][1];
            lst = index >= vc1_last_decode_table[codingset];

            if (escape == 0)
            {
                if (lst)
                { level += vc1_last_delta_level_table[codingset][run]; }
                else
                { level += vc1_delta_level_table[codingset][run]; }
            }
            else
            {
                if (lst)
                { run += vc1_last_delta_run_table[codingset][level] + 1; }
                else
                { run += vc1_delta_run_table[codingset][level] + 1; }
            }

            if (get_bits1(gb))
            { level = -level; }
        }
        else
        {
            int sign;
            lst = get_bits1(gb);

            if (v->s.esc3_level_length == 0)
            {
                if (v->pq < 8 || v->dquantfrm)
                {
                    // table 59
                    v->s.esc3_level_length = get_bits(gb, 3);

                    if (!v->s.esc3_level_length)
                    { v->s.esc3_level_length = get_bits(gb, 2) + 8; }
                }
                else
                {
                    //table 60
                    v->s.esc3_level_length = get_unary(gb, 1, 6) + 2;
                }

                v->s.esc3_run_length = 3 + get_bits(gb, 2);
            }

            run = get_bits(gb, v->s.esc3_run_length);
            sign = get_bits1(gb);
            level = get_bits(gb, v->s.esc3_level_length);

            if (sign)
            { level = -level; }
        }
    }

    *last = lst;
    *skip = run;
    *value = level;
}

/** Decode intra block in intra frames - should be faster than decode_intra_block
 * @param v VC1Context
 * @param block block to decode
 * @param[in] n subblock index
 * @param coded are AC coeffs present or not
 * @param codingset set of VLC to decode data
 */
static int vc1_decode_i_block(VC1Context *v, DCTELEM block[64], int n, int coded, int codingset)
{
    GetBitContext *gb = &v->s.gb;
    MpegEncContext *s = &v->s;
    int dc_pred_dir = 0; /* Direction of the DC prediction used */
    int run_diff, i;
    int16_t *dc_val;
    int16_t *ac_val, *ac_val2;
    int dcdiff;

    /* Get DC differential */
    if (n < 4)
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_luma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }
    else
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_chroma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }

    if (dcdiff < 0)
    {
        /* 2010/06/23 16:30:00 liuxw+00139685 */
        /* 修改日志信息和级别 */
        //		av_log(s->avctx, AV_LOG_ERROR, "Illegal DC VLC\n");
        av_log(s->avctx, AV_LOG_WARNING, "Illegal DC VLC[%d]\n", dcdiff);
        s->avctx->iTotalError++;
        IMEDIA_SET_ERR_RESIDUAL(s->avctx->iErrorCode, IMEDIA_ERR_RESIDUAL_DC);
        return -1;
    }

    if (dcdiff)
    {
        if (dcdiff == 119 /* ESC index value */)
        {
            /* TODO: Optimize */
            if (v->pq == 1)
            { dcdiff = get_bits(gb, 10); }
            else if (v->pq == 2)
            { dcdiff = get_bits(gb, 9); }
            else
            { dcdiff = get_bits(gb, 8); }
        }
        else
        {
            if (v->pq == 1)
            { dcdiff = (dcdiff << 2) + get_bits(gb, 2) - 3; }
            else if (v->pq == 2)
            { dcdiff = (dcdiff << 1) + get_bits1(gb)   - 1; }
        }

        if (get_bits1(gb))  //DC sign liuxw+00139685
        { dcdiff = -dcdiff; }
    }

    /* Prediction */
    dcdiff += vc1_i_pred_dc(&v->s, v->overlap, v->pq, n, &dc_val, &dc_pred_dir);
    *dc_val = dcdiff;  //store current blokc DC for next MB predict liuxw+00139685

    /* Store the quantized DC coeff, used for prediction */
    if (n < 4)
    {
        block[0] = dcdiff * s->y_dc_scale;
    }
    else
    {
        block[0] = dcdiff * s->c_dc_scale;
    }

    /* Skip ? */
    run_diff = 0;
    i = 0;

    if (!coded) //no ac coeff liuxw+00139685
    {
        goto not_coded;
    }

    //AC Decoding
    i = 1;

    {
        int last = 0, skip, value;
        const int8_t *zz_table;
        int scale;
        int k;

        scale = v->pq * 2 + v->halfpq;  //get double_quant for inverse quantizer liuxw+00139685

        if (v->s.ac_pred)
        {
            if (!dc_pred_dir)
            { zz_table = wmv1_scantable[2]; } //vertical zigzag scan table liuxw+00139685
            else
            { zz_table = wmv1_scantable[3]; } //horizontal zigzag scan table liuxw+00139685
        }
        else
        { zz_table = wmv1_scantable[1]; } //normal zigzag scan table liuxw+00139685

        ac_val = s->ac_val[0][0] + s->block_index[n] * 16;
        ac_val2 = ac_val;

        if (dc_pred_dir) //left
        { ac_val -= 16; }
        else //top
        { ac_val -= 16 * s->block_wrap[n]; }

        while (!last)
        {
            vc1_decode_ac_coeff(v, &last, &skip, &value, codingset);
            i += skip;

            if (i > 63)
            { break; }

            block[zz_table[i++]] = value;  //zigzag scan liuxw+00139685
        }

        /* apply AC prediction if needed */
        if (s->ac_pred)
        {
            if (dc_pred_dir)
            {
                //left
                for (k = 1; k < 8; k++)
                { block[k << 3] += ac_val[k]; }
            }
            else
            {
                //top
                for (k = 1; k < 8; k++)
                { block[k] += ac_val[k + 8]; }
            }
        }

        /* save AC coeffs for further prediction */
        for (k = 1; k < 8; k++)
        {
            ac_val2[k] = block[k << 3];
            ac_val2[k + 8] = block[k];
        }

        /* scale AC coeffs */
        for (k = 1; k < 64; k++)
            if (block[k])
            {
                block[k] *= scale;  //uniform quantizer liuxw+00139685

                if (!v->pquantizer) //nonuniform quantizer liuxw+00139685
                { block[k] += (block[k] < 0) ? -v->pq : v->pq; }
            }

        if (s->ac_pred)
        { i = 63; }
    }

not_coded:

    if (!coded)
    {
        int k, scale;
        ac_val = s->ac_val[0][0] + s->block_index[n] * 16;
        ac_val2 = ac_val;

        scale = v->pq * 2 + v->halfpq;
        memset(ac_val2, 0, 16 * 2);

        if (dc_pred_dir)
        {
            //left
            ac_val -= 16;

            if (s->ac_pred)
            { memcpy(ac_val2, ac_val, 8 * 2); }
        }
        else
        {
            //top
            ac_val -= 16 * s->block_wrap[n];

            if (s->ac_pred)
            { memcpy(ac_val2 + 8, ac_val + 8, 8 * 2); }
        }

        /* apply AC prediction if needed */
        if (s->ac_pred)
        {
            if (dc_pred_dir)
            {
                //left
                for (k = 1; k < 8; k++)
                {
                    block[k << 3] = ac_val[k] * scale;

                    if (!v->pquantizer && block[k << 3])
                    { block[k << 3] += (block[k << 3] < 0) ? -v->pq : v->pq; }
                }
            }
            else
            {
                //top
                for (k = 1; k < 8; k++)
                {
                    block[k] = ac_val[k + 8] * scale;

                    if (!v->pquantizer && block[k])
                    { block[k] += (block[k] < 0) ? -v->pq : v->pq; }
                }
            }

            i = 63;
        }
    }

    s->block_last_index[n] = i;

    return 0;
}

/** Decode intra block in intra frames - should be faster than decode_intra_block
 * @param v VC1Context
 * @param block block to decode
 * @param[in] n subblock number
 * @param coded are AC coeffs present or not
 * @param codingset set of VLC to decode data
 * @param mquant quantizer value for this macroblock
 */
static int vc1_decode_i_block_adv(VC1Context *v, DCTELEM block[64], int n, int coded, int codingset, int mquant)
{
    GetBitContext *gb = &v->s.gb;
    MpegEncContext *s = &v->s;
    int dc_pred_dir = 0; /* Direction of the DC prediction used */
    int run_diff, i;
    int16_t *dc_val;
    int16_t *ac_val, *ac_val2;
    int dcdiff;
    int a_avail = v->a_avail, c_avail = v->c_avail;
    int use_pred = s->ac_pred;
    int scale;
    int q1, q2 = 0;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;

    /* Get DC differential */
    if (n < 4)
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_luma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }
    else
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_chroma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }

    if (dcdiff < 0)
    {
        av_log(s->avctx, AV_LOG_ERROR, "Illegal DC VLC\n");
        return -1;
    }

    if (dcdiff)
    {
        if (dcdiff == 119 /* ESC index value */)
        {
            /* TODO: Optimize */
            if (mquant == 1) { dcdiff = get_bits(gb, 10); }
            else if (mquant == 2) { dcdiff = get_bits(gb, 9); }
            else { dcdiff = get_bits(gb, 8); }
        }
        else
        {
            if (mquant == 1)
            { dcdiff = (dcdiff << 2) + get_bits(gb, 2) - 3; }
            else if (mquant == 2)
            { dcdiff = (dcdiff << 1) + get_bits1(gb)   - 1; }
        }

        if (get_bits1(gb))
        { dcdiff = -dcdiff; }
    }

    /* Prediction */
    dcdiff += vc1_pred_dc(&v->s, v->overlap, mquant, n, v->a_avail, v->c_avail, &dc_val, &dc_pred_dir);
    *dc_val = dcdiff;

    /* Store the quantized DC coeff, used for prediction */
    if (n < 4)
    {
        block[0] = dcdiff * s->y_dc_scale;
    }
    else
    {
        block[0] = dcdiff * s->c_dc_scale;
    }

    /* Skip ? */
    run_diff = 0;
    i = 0;

    //AC Decoding
    i = 1;

    /* check if AC is needed at all */
    if (!a_avail && !c_avail) { use_pred = 0; }

    ac_val = s->ac_val[0][0] + s->block_index[n] * 16;
    ac_val2 = ac_val;

    scale = mquant * 2 + ((mquant == v->pq) ? v->halfpq : 0);

    if (dc_pred_dir) //left
    { ac_val -= 16; }
    else //top
    { ac_val -= 16 * s->block_wrap[n]; }

    q1 = s->current_picture.qscale_table[mb_pos];

    if (dc_pred_dir && c_avail && mb_pos) { q2 = s->current_picture.qscale_table[mb_pos - 1]; }

    if (!dc_pred_dir && a_avail && mb_pos >= s->mb_stride) { q2 = s->current_picture.qscale_table[mb_pos - s->mb_stride]; }

    if (dc_pred_dir && n == 1) { q2 = q1; }

    if (!dc_pred_dir && n == 2) { q2 = q1; }

    if (n == 3) { q2 = q1; }

    if (coded)
    {
        int last = 0, skip, value;
        const int8_t *zz_table;
        int k;

        if (v->s.ac_pred)
        {
            if (!dc_pred_dir)
            { zz_table = wmv1_scantable[2]; }
            else
            { zz_table = wmv1_scantable[3]; }
        }
        else
        { zz_table = wmv1_scantable[1]; }

        while (!last)
        {
            vc1_decode_ac_coeff(v, &last, &skip, &value, codingset);
            i += skip;

            if (i > 63)
            { break; }

            block[zz_table[i++]] = value;
        }

        /* apply AC prediction if needed */
        if (use_pred)
        {
            /* scale predictors if needed*/
            if (q2 && q1 != q2)
            {
                q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                if (dc_pred_dir)  //left
                {
                    for (k = 1; k < 8; k++)
                    { block[k << 3] += (ac_val[k] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
                else     //top
                {
                    for (k = 1; k < 8; k++)
                    { block[k] += (ac_val[k + 8] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
            else
            {
                if (dc_pred_dir)  //left
                {
                    for (k = 1; k < 8; k++)
                    { block[k << 3] += ac_val[k]; }
                }
                else     //top
                {
                    for (k = 1; k < 8; k++)
                    { block[k] += ac_val[k + 8]; }
                }
            }
        }

        /* save AC coeffs for further prediction */
        for (k = 1; k < 8; k++)
        {
            ac_val2[k] = block[k << 3];
            ac_val2[k + 8] = block[k];
        }

        /* scale AC coeffs */
        for (k = 1; k < 64; k++)
            if (block[k])
            {
                block[k] *= scale;

                if (!v->pquantizer)
                { block[k] += (block[k] < 0) ? -mquant : mquant; }
            }

        if (use_pred) { i = 63; }
    }
    else     // no AC coeffs
    {
        int k;

        memset(ac_val2, 0, 16 * 2);

        if (dc_pred_dir) //left
        {
            if (use_pred)
            {
                memcpy(ac_val2, ac_val, 8 * 2);

                if (q2 && q1 != q2)
                {
                    q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                    q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                    for (k = 1; k < 8; k++)
                    { ac_val2[k] = (ac_val2[k] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
        }
        else    //top
        {
            if (use_pred)
            {
                memcpy(ac_val2 + 8, ac_val + 8, 8 * 2);

                if (q2 && q1 != q2)
                {
                    q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                    q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                    for (k = 1; k < 8; k++)
                    { ac_val2[k + 8] = (ac_val2[k + 8] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
        }

        /* apply AC prediction if needed */
        if (use_pred)
        {
            if (dc_pred_dir)  //left
            {
                for (k = 1; k < 8; k++)
                {
                    block[k << 3] = ac_val2[k] * scale;

                    if (!v->pquantizer && block[k << 3])
                    { block[k << 3] += (block[k << 3] < 0) ? -mquant : mquant; }
                }
            }
            else     //top
            {
                for (k = 1; k < 8; k++)
                {
                    block[k] = ac_val2[k + 8] * scale;

                    if (!v->pquantizer && block[k])
                    { block[k] += (block[k] < 0) ? -mquant : mquant; }
                }
            }

            i = 63;
        }
    }

    s->block_last_index[n] = i;

    return 0;
}

/** Decode intra block in inter frames - more generic version than vc1_decode_i_block
 * @param v VC1Context
 * @param block block to decode
 * @param[in] n subblock index
 * @param coded are AC coeffs present or not
 * @param mquant block quantizer
 * @param codingset set of VLC to decode data
 */
static int vc1_decode_intra_block(VC1Context *v, DCTELEM block[64], int n, int coded, int mquant, int codingset)
{
    GetBitContext *gb = &v->s.gb;
    MpegEncContext *s = &v->s;
    int dc_pred_dir = 0; /* Direction of the DC prediction used */
    int run_diff, i;
    int16_t *dc_val;
    int16_t *ac_val, *ac_val2;
    int dcdiff;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int a_avail = v->a_avail, c_avail = v->c_avail;
    int use_pred = s->ac_pred;
    int scale;
    int q1, q2 = 0;

    /* XXX: Guard against dumb values of mquant */
    mquant = (mquant < 1) ? 0 : ( (mquant > 31) ? 31 : mquant );

    /* Set DC scale - y and c use the same */
    s->y_dc_scale = s->y_dc_scale_table[mquant];
    s->c_dc_scale = s->c_dc_scale_table[mquant];

    /* Get DC differential */
    if (n < 4)
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_luma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }
    else
    {
        dcdiff = get_vlc2(&s->gb, ff_msmp4_dc_chroma_vlc[s->dc_table_index].table, DC_VLC_BITS, 3);
    }

    if (dcdiff < 0)
    {
        av_log(s->avctx, AV_LOG_ERROR, "Illegal DC VLC\n");
        return -1;
    }

    if (dcdiff)
    {
        if (dcdiff == 119 /* ESC index value */)
        {
            /* TODO: Optimize */
            if (mquant == 1) { dcdiff = get_bits(gb, 10); }
            else if (mquant == 2) { dcdiff = get_bits(gb, 9); }
            else { dcdiff = get_bits(gb, 8); }
        }
        else
        {
            if (mquant == 1)
            { dcdiff = (dcdiff << 2) + get_bits(gb, 2) - 3; }
            else if (mquant == 2)
            { dcdiff = (dcdiff << 1) + get_bits1(gb)   - 1; }
        }

        if (get_bits1(gb))
        { dcdiff = -dcdiff; }
    }

    /* Prediction */
    dcdiff += vc1_pred_dc(&v->s, v->overlap, mquant, n, a_avail, c_avail, &dc_val, &dc_pred_dir);
    *dc_val = dcdiff;

    /* Store the quantized DC coeff, used for prediction */

    if (n < 4)
    {
        block[0] = dcdiff * s->y_dc_scale;
    }
    else
    {
        block[0] = dcdiff * s->c_dc_scale;
    }

    /* Skip ? */
    run_diff = 0;
    i = 0;

    //AC Decoding
    i = 1;

    /* check if AC is needed at all and adjust direction if needed */
    if (!a_avail)
    { dc_pred_dir = 1; }

    if (!c_avail)
    { dc_pred_dir = 0; }

    if (!a_avail && !c_avail)
    { use_pred = 0; }

    ac_val = s->ac_val[0][0] + s->block_index[n] * 16;
    ac_val2 = ac_val;

    scale = mquant * 2 + v->halfpq;

    if (dc_pred_dir) //left
    { ac_val -= 16; }
    else //top
    { ac_val -= 16 * s->block_wrap[n]; }

    q1 = s->current_picture.qscale_table[mb_pos];

    if (dc_pred_dir && c_avail && mb_pos) { q2 = s->current_picture.qscale_table[mb_pos - 1]; }

    if (!dc_pred_dir && a_avail && mb_pos >= s->mb_stride) { q2 = s->current_picture.qscale_table[mb_pos - s->mb_stride]; }

    if (dc_pred_dir && n == 1) { q2 = q1; }

    if (!dc_pred_dir && n == 2) { q2 = q1; }

    if (n == 3) { q2 = q1; }

    if (coded)
    {
        int last = 0, skip, value;
        const int8_t *zz_table;
        int k;

        zz_table = wmv1_scantable[0];

        while (!last)
        {
            vc1_decode_ac_coeff(v, &last, &skip, &value, codingset);
            i += skip;

            if (i > 63)
            { break; }

            block[zz_table[i++]] = value;
        }

        /* apply AC prediction if needed */
        if (use_pred)
        {
            /* scale predictors if needed*/
            if (q2 && q1 != q2)
            {
                q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                if (dc_pred_dir)
                {
                    //left
                    for (k = 1; k < 8; k++)
                    { block[k << 3] += (ac_val[k] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
                else
                {
                    //top
                    for (k = 1; k < 8; k++)
                    { block[k] += (ac_val[k + 8] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
            else
            {
                if (dc_pred_dir)
                {
                    //left
                    for (k = 1; k < 8; k++)
                    { block[k << 3] += ac_val[k]; }
                }
                else
                {
                    //top
                    for (k = 1; k < 8; k++)
                    { block[k] += ac_val[k + 8]; }
                }
            }
        }

        /* save AC coeffs for further prediction */
        for (k = 1; k < 8; k++)
        {
            ac_val2[k] = block[k << 3];
            ac_val2[k + 8] = block[k];
        }

        /* scale AC coeffs */
        for (k = 1; k < 64; k++)
            if (block[k])
            {
                block[k] *= scale;

                if (!v->pquantizer)
                { block[k] += (block[k] < 0) ? -mquant : mquant; }
            }

        if (use_pred) { i = 63; }
    }
    else
    {
        // no AC coeffs
        int k;

        memset(ac_val2, 0, 16 * 2);

        if (dc_pred_dir)
        {
            //left
            if (use_pred)
            {
                memcpy(ac_val2, ac_val, 8 * 2);

                if (q2 && q1 != q2)
                {
                    q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                    q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                    for (k = 1; k < 8; k++)
                    { ac_val2[k] = (ac_val2[k] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
        }
        else
        {
            //top
            if (use_pred)
            {
                memcpy(ac_val2 + 8, ac_val + 8, 8 * 2);

                if (q2 && q1 != q2)
                {
                    q1 = q1 * 2 + ((q1 == v->pq) ? v->halfpq : 0) - 1;
                    q2 = q2 * 2 + ((q2 == v->pq) ? v->halfpq : 0) - 1;

                    for (k = 1; k < 8; k++)
                    { ac_val2[k + 8] = (ac_val2[k + 8] * q2 * ff_vc1_dqscale[q1 - 1] + 0x20000) >> 18; }
                }
            }
        }

        /* apply AC prediction if needed */
        if (use_pred)
        {
            if (dc_pred_dir)
            {
                //left
                for (k = 1; k < 8; k++)
                {
                    block[k << 3] = ac_val2[k] * scale;

                    if (!v->pquantizer && block[k << 3])
                    { block[k << 3] += (block[k << 3] < 0) ? -mquant : mquant; }
                }
            }
            else
            {
                //top
                for (k = 1; k < 8; k++)
                {
                    block[k] = ac_val2[k + 8] * scale;

                    if (!v->pquantizer && block[k])
                    { block[k] += (block[k] < 0) ? -mquant : mquant; }
                }
            }

            i = 63;
        }
    }

    s->block_last_index[n] = i;

    return 0;
}

/** Decode P block
 */
static int vc1_decode_p_block(VC1Context *v, DCTELEM block[64], int n, int mquant, int ttmb, int first_block,
                              uint8_t *dst, int linesize, int skip_block, int apply_filter, int cbp_top, int cbp_left)
{
    MpegEncContext *s = &v->s;
    GetBitContext *gb = &s->gb;
    int i, j;
    int subblkpat = 0;
    int scale, off, idx, last, skip, value;
    int ttblk = ttmb & 7;
    int pat = 0;

    if (ttmb == -1)
    {
        ttblk = ff_vc1_ttblk_to_tt[v->tt_index][get_vlc2(gb, ff_vc1_ttblk_vlc[v->tt_index].table, VC1_TTBLK_VLC_BITS, 1)];
    }

    if (ttblk == TT_4X4)
    {
        subblkpat = ~(get_vlc2(gb, ff_vc1_subblkpat_vlc[v->tt_index].table, VC1_SUBBLKPAT_VLC_BITS, 1) + 1);
    }

    if ((ttblk != TT_8X8 && ttblk != TT_4X4) && (v->ttmbf || (ttmb != -1 && (ttmb & 8) && !first_block)))
    {
        subblkpat = decode012(gb);

        if (subblkpat)
        { subblkpat ^= 3; } //swap decoded pattern bits

        if (ttblk == TT_8X4_TOP || ttblk == TT_8X4_BOTTOM)
        { ttblk = TT_8X4; }

        if (ttblk == TT_4X8_RIGHT || ttblk == TT_4X8_LEFT)
        { ttblk = TT_4X8; }
    }

    /* 2010/08/31 19:00:00 liuxw+00139685 [AZ1D02293] */
    /* 首先是根据v->pq == mquant来判断是否使用halqp，若其成立，则再看useAltQP标志看是否确实是用pic_qp */
    //  scale = 2 * mquant + ((v->pq == mquant) ? v->halfpq : 0);
    scale = 2 * mquant + ((v->pq == mquant && !v->useAltQP) ? v->halfpq : 0);

    // convert transforms like 8X4_TOP to generic TT and SUBBLKPAT
    if (ttblk == TT_8X4_TOP || ttblk == TT_8X4_BOTTOM)
    {
        subblkpat = 2 - (ttblk == TT_8X4_TOP);
        ttblk = TT_8X4;
    }

    if (ttblk == TT_4X8_RIGHT || ttblk == TT_4X8_LEFT)
    {
        subblkpat = 2 - (ttblk == TT_4X8_LEFT);
        ttblk = TT_4X8;
    }

    switch (ttblk)
    {
        case TT_8X8:
            pat = 0xF;
            i = 0;
            last = 0;

            while (!last)
            {
                vc1_decode_ac_coeff(v, &last, &skip, &value, v->codingset2);
                i += skip;

                if (i > 63)
                { break; }

                idx = wmv1_scantable[0][i++];
                block[idx] = value * scale;

                if (!v->pquantizer)
                { block[idx] += (block[idx] < 0) ? -mquant : mquant; }
            }

            if (!skip_block)
            {
                s->dsp.vc1_inv_trans_8x8(block);
                s->dsp.add_pixels_clamped(block, dst, linesize);

                if (apply_filter && cbp_top  & 0xC)
                { vc1_loop_filter(dst, 1, linesize, 8, mquant); }

                if (apply_filter && cbp_left & 0xA)
                { vc1_loop_filter(dst, linesize, 1, 8, mquant); }
            }

            break;

        case TT_4X4:
            pat = ~subblkpat & 0xF;

            for (j = 0; j < 4; j++)
            {
                last = subblkpat & (1 << (3 - j));
                i = 0;
                off = (j & 1) * 4 + (j & 2) * 16;

                while (!last)
                {
                    vc1_decode_ac_coeff(v, &last, &skip, &value, v->codingset2);
                    i += skip;

                    if (i > 15)
                    { break; }

                    idx = ff_vc1_simple_progressive_4x4_zz[i++];
                    block[idx + off] = value * scale;

                    if (!v->pquantizer)
                    { block[idx + off] += (block[idx + off] < 0) ? -mquant : mquant; }
                }

                if (!(subblkpat & (1 << (3 - j))) && !skip_block)
                {
                    s->dsp.vc1_inv_trans_4x4(dst + (j & 1) * 4 + (j & 2) * 2 * linesize, linesize, block + off);

                    if (apply_filter && (j & 2 ? pat & (1 << (j - 2)) : (cbp_top & (1 << (j + 2)))))
                    { vc1_loop_filter(dst + (j & 1) * 4 + (j & 2) * 2 * linesize, 1, linesize, 4, mquant); }

                    if (apply_filter && (j & 1 ? pat & (1 << (j - 1)) : (cbp_left & (1 << (j + 1)))))
                    { vc1_loop_filter(dst + (j & 1) * 4 + (j & 2) * 2 * linesize, linesize, 1, 4, mquant); }
                }
            }

            break;

        case TT_8X4:
            pat = ~((subblkpat & 2) * 6 + (subblkpat & 1) * 3) & 0xF;

            for (j = 0; j < 2; j++)
            {
                last = subblkpat & (1 << (1 - j));
                i = 0;
                off = j * 32;

                while (!last)
                {
                    vc1_decode_ac_coeff(v, &last, &skip, &value, v->codingset2);
                    i += skip;

                    if (i > 31)
                    { break; }

                    idx = v->zz_8x4[i++] + off;
                    block[idx] = value * scale;

                    if (!v->pquantizer)
                    { block[idx] += (block[idx] < 0) ? -mquant : mquant; }
                }

                if (!(subblkpat & (1 << (1 - j))) && !skip_block)
                {
                    s->dsp.vc1_inv_trans_8x4(dst + j * 4 * linesize, linesize, block + off);

                    if ((apply_filter && j) ? pat & 0x3 : (cbp_top & 0xC))
                    { vc1_loop_filter(dst + j * 4 * linesize, 1, linesize, 8, mquant); }

                    if (apply_filter && (cbp_left & (2 << j)))
                    { vc1_loop_filter(dst + j * 4 * linesize, linesize, 1, 4, mquant); }
                }
            }

            break;

        case TT_4X8:
            pat = ~(subblkpat * 5) & 0xF;

            for (j = 0; j < 2; j++)
            {
                last = subblkpat & (1 << (1 - j));
                i = 0;
                off = j * 4;

                while (!last)
                {
                    vc1_decode_ac_coeff(v, &last, &skip, &value, v->codingset2);
                    i += skip;

                    if (i > 31)
                    { break; }

                    idx = v->zz_4x8[i++] + off;
                    block[idx] = value * scale;

                    if (!v->pquantizer)
                    { block[idx] += (block[idx] < 0) ? -mquant : mquant; }
                }

                if (!(subblkpat & (1 << (1 - j))) && !skip_block)
                {
                    s->dsp.vc1_inv_trans_4x8(dst + j * 4, linesize, block + off);

                    if (apply_filter && cbp_top & (2 << j))
                    { vc1_loop_filter(dst + j * 4, 1, linesize, 4, mquant); }

                    if (apply_filter && j ? pat & 0x5 : (cbp_left & 0xA))
                    { vc1_loop_filter(dst + j * 4, linesize, 1, 8, mquant); }
                }
            }

            break;
    }

    return pat;
}

/** @} */ // Macroblock group

static const int size_table  [6] = { 0, 2, 3, 4,  5,  8 };
static const int offset_table[6] = { 0, 1, 3, 7, 15, 31 };

/** Decode one P-frame MB (in Simple/Main profile)
 */
static int vc1_decode_p_mb(VC1Context *v)
{
    MpegEncContext *s = &v->s;
    GetBitContext *gb = &s->gb;
    //  int i, j;
    int i;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int cbp; /* cbp decoding stuff */
    int mqdiff, mquant; /* MB quantization */
    int ttmb = v->ttfrm; /* MB Transform type */

    int mb_has_coeffs = 1; /* last_flag */
#ifdef LXW_DEBUG
    int dmv_x[4] = {0};
    int dmv_y[4] = {0};
    int is_intra[6], is_coded[6];
#else
    int dmv_x, dmv_y; /* Differential MV components */
#endif
    int index, index1; /* LUT indexes */
    int val, sign; /* temp values */
    int first_block = 1;
    int dst_idx, off;
    int skipped, fourmv;
    int block_cbp = 0, pat;
    int apply_loop_filter;

    mquant = v->pq; /* Loosy initialization */

    if (v->mv_type_is_raw)
    { fourmv = get_bits1(gb); }
    else
    { fourmv = v->mv_type_mb_plane[mb_pos]; }

    if (v->skip_is_raw)
    { skipped = get_bits1(gb); }
    else
    { skipped = v->s.mbskip_table[mb_pos]; }

    s->dsp.clear_blocks(s->block[0]);

    apply_loop_filter = s->loop_filter && !(s->avctx->skip_loop_filter >= AVDISCARD_NONKEY);

    if (!fourmv) /* 1MV mode */
    {
        if (!skipped)
        {
#ifdef LXW_DEBUG
            GET_MVDATA(dmv_x[0], dmv_y[0]);
            dmv_x[1] = dmv_x[2] = dmv_x[3] = dmv_x[0];
            dmv_y[1] = dmv_y[2] = dmv_y[3] = dmv_y[0];
#else
            GET_MVDATA(dmv_x, dmv_y);
#endif

            if (s->mb_intra)
            {
                s->current_picture.motion_val[1][s->block_index[0]][0] = 0;
                s->current_picture.motion_val[1][s->block_index[0]][1] = 0;
            }

            s->current_picture.mb_type[mb_pos] = s->mb_intra ? MB_TYPE_INTRA : MB_TYPE_16x16;
#ifdef LXW_DEBUG
            vc1_pred_mv(s, 0, dmv_x[0], dmv_y[0], 1, v->range_x, v->range_y, v->mb_type[0]);
#else
            vc1_pred_mv(s, 0, dmv_x, dmv_y, 1, v->range_x, v->range_y, v->mb_type[0]);
#endif

            /* FIXME Set DC val for inter block ? */
            if (s->mb_intra && !mb_has_coeffs)
            {
                GET_MQUANT();
                s->ac_pred = get_bits1(gb);
                cbp = 0;
            }
            else if (mb_has_coeffs)
            {
                if (s->mb_intra)
                { s->ac_pred = get_bits1(gb); }

                cbp = get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);
                GET_MQUANT();
            }
            else
            {
                mquant = v->pq;
                cbp = 0;
            }

            s->current_picture.qscale_table[mb_pos] = mquant;

            if (!v->ttmbf && !s->mb_intra && mb_has_coeffs)
            { ttmb = get_vlc2(gb, ff_vc1_ttmb_vlc[v->tt_index].table, VC1_TTMB_VLC_BITS, 2); }

            if (!s->mb_intra)
            { vc1_mc_1mv(v, 0); }

            dst_idx = 0;

            for (i = 0; i < 6; i++)
            {
                s->dc_val[0][s->block_index[i]] = 0;
                dst_idx += i >> 2;
                val = ((cbp >> (5 - i)) & 1);
                off = (i & 4) ? 0 : ((i & 1) * 8 + (i & 2) * 4 * s->linesize);
                v->mb_type[0][s->block_index[i]] = s->mb_intra;

                if (s->mb_intra)
                {
                    /* check if prediction blocks A and C are available */
                    v->a_avail = v->c_avail = 0;

                    if (i == 2 || i == 3 || !s->first_slice_line)
                    {
                        v->a_avail = v->mb_type[0][s->block_index[i] - s->block_wrap[i]];
                    }

                    if (i == 1 || i == 3 || s->mb_x)
                    {
                        v->c_avail = v->mb_type[0][s->block_index[i] - 1];
                    }

                    vc1_decode_intra_block(v, s->block[i], i, val, mquant, (i & 4) ? v->codingset2 : v->codingset);

                    if ((i > 3) && (s->flags & CODEC_FLAG_GRAY))
                    { continue; }

                    s->dsp.vc1_inv_trans_8x8(s->block[i]);
                    /* 2010/08/26 10:00:00 liuxw+00139685 */
                    /* rangeredfrm功能实现不对 */
                    /*                   if(v->rangeredfrm)
                    					for(j = 0; j < 64; j++)
                    						s->block[i][j] <<= 1;*/
                    s->dsp.put_signed_pixels_clamped(s->block[i], s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2));

                    if (v->pq >= 9 && v->overlap)
                    {
                        if (v->c_avail)
                        { s->dsp.vc1_h_overlap(s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2)); }

                        if (v->a_avail)
                        { s->dsp.vc1_v_overlap(s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2)); }
                    }

                    /* 2010/07/14 16:30:00 liuxw+00139685 [AZ1D2220] */
                    /* 修改判断条件 */
                    //          if(apply_loop_filter && s->mb_x && s->mb_x != (s->mb_width - 1) && s->mb_y && s->mb_y != (s->mb_height - 1))
                    if (apply_loop_filter && !((s->mb_y == 0 && (i == 0 || i == 1 || i == 4 || i == 5)) || (s->mb_x == 0 && (i == 0 || i == 2 || i == 4 || i == 5)) || (s->mb_y == (s->mb_height - 1) && (i == 2 || i == 3 || i == 4 || i == 5)) || (s->mb_x == (s->mb_width - 1) && (i == 1 || i == 3 || i == 4 || i == 5))))
                    {
                        int left_cbp, top_cbp;

                        if (i & 4)
                        {
                            left_cbp = v->cbp[s->mb_x - 1]            >> (i * 4);
                            top_cbp  = v->cbp[s->mb_x - s->mb_stride] >> (i * 4);
                        }
                        else
                        {
                            left_cbp = (i & 1) ? (cbp >> ((i - 1) * 4)) : (v->cbp[s->mb_x - 1]           >> ((i + 1) * 4));
                            top_cbp  = (i & 2) ? (cbp >> ((i - 2) * 4)) : (v->cbp[s->mb_x - s->mb_stride] >> ((i + 2) * 4));
                        }

                        if (left_cbp & 0xC)
                        { vc1_loop_filter(s->dest[dst_idx] + off, 1, i & 4 ? s->uvlinesize : s->linesize, 8, mquant); }

                        if (top_cbp  & 0xA)
                        { vc1_loop_filter(s->dest[dst_idx] + off, i & 4 ? s->uvlinesize : s->linesize, 1, 8, mquant); }
                    }

                    block_cbp |= 0xF << (i << 2);
                }
                else if (val)
                {
                    int left_cbp = 0, top_cbp = 0, filter = 0;

                    /* 2010/07/21 15:30:00 liuxw+00139685 [AZ1D2220] */
                    /* 修改判断条件 */
                    //                   if(apply_loop_filter && s->mb_x && s->mb_x != (s->mb_width - 1) && s->mb_y && s->mb_y != (s->mb_height - 1))
                    if (apply_loop_filter && !((s->mb_y == 0 && (i == 0 || i == 1 || i == 4 || i == 5)) || (s->mb_x == 0 && (i == 0 || i == 2 || i == 4 || i == 5)) || (s->mb_y == (s->mb_height - 1) && (i == 2 || i == 3 || i == 4 || i == 5)) || (s->mb_x == (s->mb_width - 1) && (i == 1 || i == 3 || i == 4 || i == 5))))
                    {
                        filter = 1;

                        if (i & 4)
                        {
                            left_cbp = v->cbp[s->mb_x - 1]            >> (i * 4);
                            top_cbp  = v->cbp[s->mb_x - s->mb_stride] >> (i * 4);
                        }
                        else
                        {
                            left_cbp = (i & 1) ? (cbp >> ((i - 1) * 4)) : (v->cbp[s->mb_x - 1]           >> ((i + 1) * 4));
                            top_cbp  = (i & 2) ? (cbp >> ((i - 2) * 4)) : (v->cbp[s->mb_x - s->mb_stride] >> ((i + 2) * 4));
                        }

                        /* 2010/07/14 16:30:00 liuxw+00139685 [AZ1D2220] */
                        /* 注释掉不必要的loop filter */
                        //if(left_cbp & 0xC)
                        //    vc1_loop_filter(s->dest[dst_idx] + off, 1, i & 4 ? s->uvlinesize : s->linesize, 8, mquant);
                        //if(top_cbp  & 0xA)
                        //    vc1_loop_filter(s->dest[dst_idx] + off, i & 4 ? s->uvlinesize : s->linesize, 1, 8, mquant);
                    }

                    pat = vc1_decode_p_block(v, s->block[i], i, mquant, ttmb, first_block, s->dest[dst_idx] + off, (i & 4) ? s->uvlinesize : s->linesize, (i & 4) && (s->flags & CODEC_FLAG_GRAY), filter, left_cbp, top_cbp);
                    block_cbp |= pat << (i << 2);

                    if (!v->ttmbf && ttmb < 8)
                    { ttmb = -1; }

                    first_block = 0;
                }
            }
        }
        else //Skipped
        {
            s->mb_intra = 0;

            for (i = 0; i < 6; i++)
            {
                v->mb_type[0][s->block_index[i]] = 0;
                s->dc_val[0][s->block_index[i]] = 0;
            }

            s->current_picture.mb_type[mb_pos] = MB_TYPE_SKIP;
            s->current_picture.qscale_table[mb_pos] = 0;
            vc1_pred_mv(s, 0, 0, 0, 1, v->range_x, v->range_y, v->mb_type[0]);
            vc1_mc_1mv(v, 0);
#ifdef LXW_DEBUG
            goto P_END;
#else
            return 0;
#endif
        }
    } //1MV mode
    else //4MV mode
    {
        if (!skipped /* unskipped MB */)
        {
            int intra_count = 0, coded_inter = 0;
#ifndef LXW_DEBUG
            int is_intra[6], is_coded[6];
#endif
            /* Get CBPCY */
            cbp = get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);

            for (i = 0; i < 6; i++)
            {
                val = ((cbp >> (5 - i)) & 1);
                s->dc_val[0][s->block_index[i]] = 0;
                s->mb_intra = 0;

                if (i < 4)
                {
#ifndef LXW_DEBUG
                    dmv_x = dmv_y = 0;
#endif
                    s->mb_intra = 0;
                    mb_has_coeffs = 0;

                    if (val)
                    {
#ifdef LXW_DEBUG
                        GET_MVDATA(dmv_x[i], dmv_y[i]);
                    }

                    vc1_pred_mv(s, i, dmv_x[i], dmv_y[i], 0, v->range_x, v->range_y, v->mb_type[0]);
#else
                        GET_MVDATA(dmv_x, dmv_y);

                    }

                    vc1_pred_mv(s, i, dmv_x, dmv_y, 0, v->range_x, v->range_y, v->mb_type[0]);
#endif

                    if (!s->mb_intra)
                    { vc1_mc_4mv_luma(v, i); }

                    intra_count += s->mb_intra;
                    is_intra[i] = s->mb_intra;
                    is_coded[i] = mb_has_coeffs;
                }

                if (i & 4)
                {
                    is_intra[i] = (intra_count >= 3);
                    is_coded[i] = val;
                }

                if (i == 4)
                { vc1_mc_4mv_chroma(v); }

                v->mb_type[0][s->block_index[i]] = is_intra[i];

                if (!coded_inter)
                { coded_inter = !is_intra[i] & is_coded[i]; }
            }

            // if there are no coded blocks then don't do anything more
            if (!intra_count && !coded_inter)
#ifdef LXW_DEBUG
                goto P_END;

#else
                return 0;
#endif
            dst_idx = 0;
            GET_MQUANT();
            s->current_picture.qscale_table[mb_pos] = mquant;
            /* test if block is intra and has pred */
            {
                int intrapred = 0;

                for (i = 0; i < 6; i++)
                    if (is_intra[i])
                    {
                        if (((!s->first_slice_line || (i == 2 || i == 3)) && v->mb_type[0][s->block_index[i] - s->block_wrap[i]])
                            || ((s->mb_x || (i == 1 || i == 3)) && v->mb_type[0][s->block_index[i] - 1]))
                        {
                            intrapred = 1;
                            break;
                        }
                    }

                if (intrapred)
                { s->ac_pred = get_bits1(gb); }
                else
                { s->ac_pred = 0; }
            }

            if (!v->ttmbf && coded_inter)
            { ttmb = get_vlc2(gb, ff_vc1_ttmb_vlc[v->tt_index].table, VC1_TTMB_VLC_BITS, 2); }

            for (i = 0; i < 6; i++)
            {
                dst_idx += i >> 2;
                off = (i & 4) ? 0 : ((i & 1) * 8 + (i & 2) * 4 * s->linesize);
                s->mb_intra = is_intra[i];

                if (is_intra[i])
                {
                    /* check if prediction blocks A and C are available */
                    v->a_avail = v->c_avail = 0;

                    if (i == 2 || i == 3 || !s->first_slice_line)
                    { v->a_avail = v->mb_type[0][s->block_index[i] - s->block_wrap[i]]; }

                    if (i == 1 || i == 3 || s->mb_x)
                    { v->c_avail = v->mb_type[0][s->block_index[i] - 1]; }

                    vc1_decode_intra_block(v, s->block[i], i, is_coded[i], mquant, (i & 4) ? v->codingset2 : v->codingset);

                    if ((i > 3) && (s->flags & CODEC_FLAG_GRAY))
                    { continue; }

                    s->dsp.vc1_inv_trans_8x8(s->block[i]);
                    /* 2010/08/26 10:00:00 liuxw+00139685 */
                    /* rangeredfrm功能使用错误 */
                    //if(v->rangeredfrm)
                    //	for(j = 0; j < 64; j++)
                    //		s->block[i][j] <<= 1;
                    s->dsp.put_signed_pixels_clamped(s->block[i], s->dest[dst_idx] + off, (i & 4) ? s->uvlinesize : s->linesize);

                    if (v->pq >= 9 && v->overlap)
                    {
                        if (v->c_avail)
                        { s->dsp.vc1_h_overlap(s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2)); }

                        if (v->a_avail)
                        { s->dsp.vc1_v_overlap(s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2)); }
                    }

                    /* 2010/07/21 15:30:00 liuxw+00139685 [AZ1D2220] */
                    /* 修改判断条件 */
                    //                  if(v->s.loop_filter && s->mb_x && s->mb_x != (s->mb_width - 1) && s->mb_y && s->mb_y != (s->mb_height - 1))
                    if (apply_loop_filter && !((s->mb_y == 0 && (i == 0 || i == 1 || i == 4 || i == 5)) || (s->mb_x == 0 && (i == 0 || i == 2 || i == 4 || i == 5)) || (s->mb_y == (s->mb_height - 1) && (i == 2 || i == 3 || i == 4 || i == 5)) || (s->mb_x == (s->mb_width - 1) && (i == 1 || i == 3 || i == 4 || i == 5))))
                    {
                        int left_cbp, top_cbp;

                        if (i & 4)
                        {
                            left_cbp = v->cbp[s->mb_x - 1]            >> (i * 4);
                            top_cbp  = v->cbp[s->mb_x - s->mb_stride] >> (i * 4);
                        }
                        else
                        {
                            left_cbp = (i & 1) ? (cbp >> ((i - 1) * 4)) : (v->cbp[s->mb_x - 1]           >> ((i + 1) * 4));
                            top_cbp  = (i & 2) ? (cbp >> ((i - 2) * 4)) : (v->cbp[s->mb_x - s->mb_stride] >> ((i + 2) * 4));
                        }

                        if (left_cbp & 0xC)
                        { vc1_loop_filter(s->dest[dst_idx] + off, 1, i & 4 ? s->uvlinesize : s->linesize, 8, mquant); }

                        if (top_cbp  & 0xA)
                        { vc1_loop_filter(s->dest[dst_idx] + off, i & 4 ? s->uvlinesize : s->linesize, 1, 8, mquant); }
                    }

                    block_cbp |= 0xF << (i << 2);
                }
                else if (is_coded[i])
                {
                    int left_cbp = 0, top_cbp = 0, filter = 0;

                    /* 2010/07/21 15:30:00 liuxw+00139685 [AZ1D2220] */
                    /* 修改判断条件 */
                    //                  if(apply_loop_filter && s->mb_x && s->mb_x != (s->mb_width - 1) && s->mb_y && s->mb_y != (s->mb_height - 1))
                    if (apply_loop_filter && !((s->mb_y == 0 && (i == 0 || i == 1 || i == 4 || i == 5)) || (s->mb_x == 0 && (i == 0 || i == 2 || i == 4 || i == 5)) || (s->mb_y == (s->mb_height - 1) && (i == 2 || i == 3 || i == 4 || i == 5)) || (s->mb_x == (s->mb_width - 1) && (i == 1 || i == 3 || i == 4 || i == 5))))
                    {
                        filter = 1;

                        if (i & 4)
                        {
                            left_cbp = v->cbp[s->mb_x - 1]            >> (i * 4);
                            top_cbp  = v->cbp[s->mb_x - s->mb_stride] >> (i * 4);
                        }
                        else
                        {
                            left_cbp = (i & 1) ? (cbp >> ((i - 1) * 4)) : (v->cbp[s->mb_x - 1]           >> ((i + 1) * 4));
                            top_cbp  = (i & 2) ? (cbp >> ((i - 2) * 4)) : (v->cbp[s->mb_x - s->mb_stride] >> ((i + 2) * 4));
                        }

                        /* 2010/07/21 15:30:00 liuxw+00139685 [AZ1D2220] */
                        /* 去掉不必要的代码 */
                        //if(left_cbp & 0xC)
                        //    vc1_loop_filter(s->dest[dst_idx] + off, 1, i & 4 ? s->uvlinesize : s->linesize, 8, mquant);
                        //if(top_cbp  & 0xA)
                        //    vc1_loop_filter(s->dest[dst_idx] + off, i & 4 ? s->uvlinesize : s->linesize, 1, 8, mquant);
                    }

                    pat = vc1_decode_p_block(v, s->block[i], i, mquant, ttmb, first_block, s->dest[dst_idx] + off, (i & 4) ? s->uvlinesize : s->linesize, (i & 4) && (s->flags & CODEC_FLAG_GRAY), filter, left_cbp, top_cbp);
                    block_cbp |= pat << (i << 2);

                    if (!v->ttmbf && ttmb < 8)
                    { ttmb = -1; }

                    first_block = 0;
                }
            }

#ifdef LXW_DEBUG
            goto P_END;
#else
            return 0;
#endif
        }
        else //Skipped MB
        {
            s->mb_intra = 0;
            s->current_picture.qscale_table[mb_pos] = 0;

            for (i = 0; i < 6; i++)
            {
                v->mb_type[0][s->block_index[i]] = 0;
                s->dc_val[0][s->block_index[i]] = 0;
            }

            for (i = 0; i < 4; i++)
            {
                vc1_pred_mv(s, i, 0, 0, 0, v->range_x, v->range_y, v->mb_type[0]);
                vc1_mc_4mv_luma(v, i);
            }

            vc1_mc_4mv_chroma(v);
            s->current_picture.qscale_table[mb_pos] = 0;
#ifdef LXW_DEBUG
            goto P_END;
#else
            return 0;
#endif
        }
    }
    v->cbp[s->mb_x] = block_cbp;

#ifdef LXW_DEBUG
P_END:

    if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
    {
        av_log(s->avctx, AV_LOG_WARNING, "frame_num:%d mb_x:%d mb_y:%d skip:%d fourmv:%d mb_intra:%d block0_intra:%d block1_intra:%d block2_intra:%d block3_intra:%d block0_coded:%d block1__coded:%d block2__coded:%d block3__coded:%d apply_loop_filter:%d\n",
               frame_num, s->mb_x, s->mb_y, skipped, fourmv, s->mb_intra, is_intra[0], is_intra[1], is_intra[2], is_intra[3], is_coded[0], is_coded[1], is_coded[2], is_coded[3], apply_loop_filter);
        av_log(s->avctx, AV_LOG_WARNING, "dmvx0:%d dmvy0:%d dmvx1:%d dmvy1:%d dmvx2:%d dmvy2:%d dmvx3:%d dmvy3:%d mvx0:%d mvy0:%d mvx1:%d mvy1:%d mvx2:%d mvy2:%d mvx3:%d mvy3:%d ac_pred:%d cbp:%d mquant:%d ttmb:%d\n",
               dmv_x[0], dmv_y[0], dmv_x[1], dmv_y[1], dmv_x[2], dmv_y[2], dmv_x[3], dmv_y[3], s->mv[0][0][0], s->mv[0][0][1], s->mv[0][1][0], s->mv[0][1][1], s->mv[0][2][0], s->mv[0][2][1], s->mv[0][3][0], s->mv[0][3][1], s->ac_pred, cbp, mquant, ttmb);
    }

    return 0;
#endif

    /* Should never happen */
    return -1;
}

/** Decode one B-frame MB (in Main profile)
 */
static void vc1_decode_b_mb(VC1Context *v)
{
    MpegEncContext *s = &v->s;
    GetBitContext *gb = &s->gb;
    //  int i, j;
    int i;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int cbp = 0; /* cbp decoding stuff */
    int mqdiff, mquant; /* MB quantization */
    int ttmb = v->ttfrm; /* MB Transform type */
    int mb_has_coeffs = 0; /* last_flag */
    int index, index1; /* LUT indexes */
    int val, sign; /* temp values */
    int first_block = 1;
    int dst_idx, off;
    int skipped, direct;
    int dmv_x[2], dmv_y[2];
    int bmvtype = BMV_TYPE_BACKWARD;

    mquant = v->pq; /* Loosy initialization */
    s->mb_intra = 0;

    if (v->dmb_is_raw)
    { direct = get_bits1(gb); }
    else
    { direct = v->direct_mb_plane[mb_pos]; }

    if (v->skip_is_raw)
    { skipped = get_bits1(gb); }
    else
    { skipped = v->s.mbskip_table[mb_pos]; }

    s->dsp.clear_blocks(s->block[0]);
    dmv_x[0] = dmv_x[1] = dmv_y[0] = dmv_y[1] = 0;

    for (i = 0; i < 6; i++)
    {
        v->mb_type[0][s->block_index[i]] = 0;
        s->dc_val[0][s->block_index[i]] = 0;
    }

    s->current_picture.qscale_table[mb_pos] = 0;

    if (!direct)
    {
        if (!skipped)
        {
            GET_MVDATA(dmv_x[0], dmv_y[0]);
            dmv_x[1] = dmv_x[0];
            dmv_y[1] = dmv_y[0];
        }

        if (skipped || !s->mb_intra)
        {
            bmvtype = decode012(gb);

            switch (bmvtype)
            {
                case 0:
                    bmvtype = (v->bfraction >= (B_FRACTION_DEN / 2)) ? BMV_TYPE_BACKWARD : BMV_TYPE_FORWARD;
                    break;

                case 1:
                    bmvtype = (v->bfraction >= (B_FRACTION_DEN / 2)) ? BMV_TYPE_FORWARD : BMV_TYPE_BACKWARD;
                    break;

                case 2:
                    bmvtype = BMV_TYPE_INTERPOLATED;
                    dmv_x[0] = dmv_y[0] = 0;
            }
        }
    }

    for (i = 0; i < 6; i++)
    { v->mb_type[0][s->block_index[i]] = s->mb_intra; }

#ifdef LXW_DEBUG

    if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
    {
        av_log(s->avctx, AV_LOG_WARNING, "frame_num:%d mb_x:%d mb_y:%d skip:%d direct:%d bmvtype:%d mb_intra:%d \n",
               frame_num, s->mb_x, s->mb_y, skipped, direct, bmvtype, s->mb_intra);
    }

#endif

    if (skipped)
    {
        if (direct)
        { bmvtype = BMV_TYPE_INTERPOLATED; }

        vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);
        vc1_b_mc(v, dmv_x, dmv_y, direct, bmvtype);
        //      return;
        goto END;
    }

    if (direct)
    {
        cbp = get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);
        GET_MQUANT();
        s->mb_intra = 0;
        mb_has_coeffs = 0;
        s->current_picture.qscale_table[mb_pos] = mquant;

        if (!v->ttmbf)
        { ttmb = get_vlc2(gb, ff_vc1_ttmb_vlc[v->tt_index].table, VC1_TTMB_VLC_BITS, 2); }

        dmv_x[0] = dmv_y[0] = dmv_x[1] = dmv_y[1] = 0;
        vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);
        vc1_b_mc(v, dmv_x, dmv_y, direct, bmvtype);
    }
    else
    {
        if (!mb_has_coeffs && !s->mb_intra)
        {
            /* no coded blocks - effectively skipped */
            vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);
            vc1_b_mc(v, dmv_x, dmv_y, direct, bmvtype);
            //          return;
            goto END;
        }

        if (s->mb_intra && !mb_has_coeffs)
        {
            GET_MQUANT();
            s->current_picture.qscale_table[mb_pos] = mquant;
            s->ac_pred = get_bits1(gb);
            cbp = 0;
            vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);
        }
        else
        {
            if (bmvtype == BMV_TYPE_INTERPOLATED)
            {
                GET_MVDATA(dmv_x[0], dmv_y[0]);

                if (!mb_has_coeffs)
                {
                    /* interpolated skipped block */
                    vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);
                    vc1_b_mc(v, dmv_x, dmv_y, direct, bmvtype);
                    //                  return;
                    goto END;
                }
            }

            vc1_pred_b_mv(v, dmv_x, dmv_y, direct, bmvtype);

            if (!s->mb_intra)
            {
                vc1_b_mc(v, dmv_x, dmv_y, direct, bmvtype);
            }

            if (s->mb_intra)
            { s->ac_pred = get_bits1(gb); }

            cbp = get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);
            GET_MQUANT();
            s->current_picture.qscale_table[mb_pos] = mquant;

            if (!v->ttmbf && !s->mb_intra && mb_has_coeffs)
            { ttmb = get_vlc2(gb, ff_vc1_ttmb_vlc[v->tt_index].table, VC1_TTMB_VLC_BITS, 2); }
        }
    }

    dst_idx = 0;

    for (i = 0; i < 6; i++)
    {
        s->dc_val[0][s->block_index[i]] = 0;
        dst_idx += i >> 2;
        val = ((cbp >> (5 - i)) & 1);
        off = (i & 4) ? 0 : ((i & 1) * 8 + (i & 2) * 4 * s->linesize);
        v->mb_type[0][s->block_index[i]] = s->mb_intra;

        if (s->mb_intra)
        {
            /* check if prediction blocks A and C are available */
            v->a_avail = v->c_avail = 0;

            if (i == 2 || i == 3 || !s->first_slice_line)
            { v->a_avail = v->mb_type[0][s->block_index[i] - s->block_wrap[i]]; }

            if (i == 1 || i == 3 || s->mb_x)
            { v->c_avail = v->mb_type[0][s->block_index[i] - 1]; }

            vc1_decode_intra_block(v, s->block[i], i, val, mquant, (i & 4) ? v->codingset2 : v->codingset);

            if ((i > 3) && (s->flags & CODEC_FLAG_GRAY))
            { continue; }

            s->dsp.vc1_inv_trans_8x8(s->block[i]);
            /* 2010/08/26 10:00:00 liuxw+00139685 */
            /* rangeredfrm功能使用错误 */
            /*            if(v->rangeredfrm)
            				for(j = 0; j < 64; j++)
            				s->block[i][j] <<= 1; */
            s->dsp.put_signed_pixels_clamped(s->block[i], s->dest[dst_idx] + off, s->linesize >> ((i & 4) >> 2));
        }
        else if (val)
        {
            vc1_decode_p_block(v, s->block[i], i, mquant, ttmb, first_block, s->dest[dst_idx] + off, (i & 4) ? s->uvlinesize : s->linesize, (i & 4) && (s->flags & CODEC_FLAG_GRAY), 0, 0, 0);

            if (!v->ttmbf && ttmb < 8)
            { ttmb = -1; }

            first_block = 0;
        }
    }

END:

#ifdef LXW_DEBUG

    if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
    {
        av_log(s->avctx, AV_LOG_WARNING, "dmvx0:%d dmvy0:%d dmvx1:%d dmvy1:%d mvx0:%d mvy0:%d mvx1:%d mvy1:%d ac_pred:%d cbp:%d mquant:%d ttmb:%d\n",
               dmv_x[0], dmv_y[0], dmv_x[1], dmv_y[1], s->mv[0][0][0], s->mv[0][0][1], s->mv[1][0][0], s->mv[1][0][1], s->ac_pred, cbp, mquant, ttmb);
    }

#endif

    return;
}

/** Decode blocks of I-frame
 */
static void vc1_decode_i_blocks(VC1Context *v)
{
    int k, j;
    MpegEncContext *s = &v->s;
    int cbp, val;
    uint8_t *coded_val;
    int mb_pos;
    /* 2010/06/23 16:30:00 liuxw+00139685 */
    /* 定义一个返回码 */
    int iRet = 0;

    /* select codingmode used for VLC tables selection */
    switch (v->y_ac_table_index)
    {
        case 0:
            v->codingset = (v->pqindex <= 8) ? CS_HIGH_RATE_INTRA : CS_LOW_MOT_INTRA;
            break;

        case 1:
            v->codingset = CS_HIGH_MOT_INTRA;
            break;

        case 2:
            v->codingset = CS_MID_RATE_INTRA;
            break;
    }

    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset2 = (v->pqindex <= 8) ? CS_HIGH_RATE_INTER : CS_LOW_MOT_INTER;
            break;

        case 1:
            v->codingset2 = CS_HIGH_MOT_INTER;
            break;

        case 2:
            v->codingset2 = CS_MID_RATE_INTER;
            break;
    }

    /* Set DC scale - y and c use the same */
    s->y_dc_scale = s->y_dc_scale_table[v->pq];
    s->c_dc_scale = s->c_dc_scale_table[v->pq];

    //do frame decode
    s->mb_x = s->mb_y = 0;
    s->mb_intra = 1;
    s->first_slice_line = 1;

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++)
    {
        for (s->mb_x = 0; s->mb_x < s->mb_width; s->mb_x++)
        {
            ff_init_block_index(s);
            ff_update_block_index(s);
            s->dsp.clear_blocks(s->block[0]);
            mb_pos = s->mb_x + s->mb_y * s->mb_width;
            s->current_picture.mb_type[mb_pos] = MB_TYPE_INTRA;
            s->current_picture.qscale_table[mb_pos] = v->pq;
            s->current_picture.motion_val[1][s->block_index[0]][0] = 0;
            s->current_picture.motion_val[1][s->block_index[0]][1] = 0;

            // do actual MB decoding and displaying
            cbp = get_vlc2(&v->s.gb, ff_msmp4_mb_i_vlc.table, MB_INTRA_VLC_BITS, 2);
            v->s.ac_pred = get_bits1(&v->s.gb);  //用于表示是否使用了ac预测 + l00139685

            for (k = 0; k < 6; k++)
            {
                val = ((cbp >> (5 - k)) & 1);

                if (k < 4)
                {
                    int pred = vc1_coded_block_pred(&v->s, k, &coded_val);
                    val = val ^ pred;
                    *coded_val = val;
                }

                cbp |= val << (5 - k);

                /* 2010/06/23 16:30:00 liuxw+00139685 */
                /* 增加返回码检测 */
                //              vc1_decode_i_block(v, s->block[k], k, val, (k<4)? v->codingset : v->codingset2);
                iRet = vc1_decode_i_block(v, s->block[k], k, val, (k < 4) ? v->codingset : v->codingset2);

                if (iRet < 0)
                {
                    return ;
                }

                s->dsp.vc1_inv_trans_8x8(s->block[k]);

                if (v->pq >= 9 && v->overlap)
                {
                    for (j = 0; j < 64; j++)
                    { s->block[k][j] += 128; }
                }
            }

            vc1_put_block(v, s->block);
#ifdef LXW_DEBUG

            if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
            {
                av_log(s->avctx, AV_LOG_WARNING, "Before Overlap Frame Num:%d mb_x:%d mb_y:%d \n", frame_num, s->mb_x, s->mb_y);

                for (k = 0; k < 16; k++)
                {
                    for (j = 0; j < 16; j++)
                    { av_log(s->avctx, AV_LOG_WARNING, "%03d ", s->dest[0][k * s->linesize + j]); }

                    av_log(s->avctx, AV_LOG_WARNING, "\n");
                }
            }

#endif

            if (v->pq >= 9 && v->overlap)
            {
                if (s->mb_x)
                {
                    s->dsp.vc1_h_overlap(s->dest[0], s->linesize);
                    s->dsp.vc1_h_overlap(s->dest[0] + 8 * s->linesize, s->linesize);

                    if (!(s->flags & CODEC_FLAG_GRAY))
                    {
                        s->dsp.vc1_h_overlap(s->dest[1], s->uvlinesize);
                        s->dsp.vc1_h_overlap(s->dest[2], s->uvlinesize);
                    }
                }

                s->dsp.vc1_h_overlap(s->dest[0] + 8, s->linesize);
                s->dsp.vc1_h_overlap(s->dest[0] + 8 * s->linesize + 8, s->linesize);

                if (!s->first_slice_line)
                {
                    s->dsp.vc1_v_overlap(s->dest[0], s->linesize);
                    s->dsp.vc1_v_overlap(s->dest[0] + 8, s->linesize);

                    if (!(s->flags & CODEC_FLAG_GRAY))
                    {
                        s->dsp.vc1_v_overlap(s->dest[1], s->uvlinesize);
                        s->dsp.vc1_v_overlap(s->dest[2], s->uvlinesize);
                    }
                }

                s->dsp.vc1_v_overlap(s->dest[0] + 8 * s->linesize, s->linesize);
                s->dsp.vc1_v_overlap(s->dest[0] + 8 * s->linesize + 8, s->linesize);

#ifdef LXW_DEBUG

                if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
                {
                    av_log(s->avctx, AV_LOG_WARNING, "After Overlap Frame Num:%d mb_x:%d mb_y:%d \n", frame_num, s->mb_x, s->mb_y);

                    for (k = 0; k < 16; k++)
                    {
                        for (j = 0; j < 16; j++)
                        { av_log(s->avctx, AV_LOG_WARNING, "%03d ", s->dest[0][k * s->linesize + j]); }

                        av_log(s->avctx, AV_LOG_WARNING, "\n");
                    }

                }

#endif

            }

            if (v->s.loop_filter)
            { vc1_loop_filter_iblk(s, s->current_picture.qscale_table[mb_pos]); }

#ifdef LXW_DEBUG

            if (frame_num == TARGET_FRAME && s->mb_x == MB_X && s->mb_y == MB_Y)
            {
                av_log(s->avctx, AV_LOG_WARNING, "After loop filter Frame Num:%d mb_x:%d mb_y:%d \n", frame_num, s->mb_x, s->mb_y);

                for (k = 0; k < 16; k++)
                {
                    for (j = 0; j < 16; j++)
                    { av_log(s->avctx, AV_LOG_WARNING, "%03d ", s->dest[0][k * s->linesize + j]); }

                    av_log(s->avctx, AV_LOG_WARNING, "\n");
                }
            }

#endif

            if (get_bits_count(&s->gb) > v->bits)
            {
                ff_er_add_slice(s, 0, 0, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END));
                /* 2010/06/23 11:30:00 liuxw+00139685 */
                /* 修改日志级别 */
                //              av_log(s->avctx, AV_LOG_ERROR, "Bits overconsumption: %i > %i\n", get_bits_count(&s->gb), v->bits);
                av_log(s->avctx, AV_LOG_WARNING, "Bits overconsumption: %i > %i\n", get_bits_count(&s->gb), v->bits);
                return;
            }
        }

        ff_draw_horiz_band(s, s->mb_y * 16, 16);
        s->first_slice_line = 0;
    }

    ff_er_add_slice(s, 0, 0, s->mb_width - 1, s->mb_height - 1, (AC_END | DC_END | MV_END));
}

/** Decode blocks of I-frame for advanced profile
 */
static void vc1_decode_i_blocks_adv(VC1Context *v)
{
    int k, j;
    MpegEncContext *s = &v->s;
    int cbp, val;
    uint8_t *coded_val;
    int mb_pos;
    int mquant = v->pq;
    int mqdiff;
    int overlap;
    GetBitContext *gb = &s->gb;

    /* select codingmode used for VLC tables selection */
    switch (v->y_ac_table_index)
    {
        case 0:
            v->codingset = (v->pqindex <= 8) ? CS_HIGH_RATE_INTRA : CS_LOW_MOT_INTRA;
            break;

        case 1:
            v->codingset = CS_HIGH_MOT_INTRA;
            break;

        case 2:
            v->codingset = CS_MID_RATE_INTRA;
            break;
    }

    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset2 = (v->pqindex <= 8) ? CS_HIGH_RATE_INTER : CS_LOW_MOT_INTER;
            break;

        case 1:
            v->codingset2 = CS_HIGH_MOT_INTER;
            break;

        case 2:
            v->codingset2 = CS_MID_RATE_INTER;
            break;
    }

    //do frame decode
    s->mb_x = s->mb_y = 0;
    s->mb_intra = 1;
    s->first_slice_line = 1;

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++)
    {
        for (s->mb_x = 0; s->mb_x < s->mb_width; s->mb_x++)
        {
            ff_init_block_index(s);
            ff_update_block_index(s);
            s->dsp.clear_blocks(s->block[0]);
            mb_pos = s->mb_x + s->mb_y * s->mb_stride;
            s->current_picture.mb_type[mb_pos] = MB_TYPE_INTRA;
            s->current_picture.motion_val[1][s->block_index[0]][0] = 0;
            s->current_picture.motion_val[1][s->block_index[0]][1] = 0;

            // do actual MB decoding and displaying
            cbp = get_vlc2(&v->s.gb, ff_msmp4_mb_i_vlc.table, MB_INTRA_VLC_BITS, 2);

            if (v->acpred_is_raw)
            { v->s.ac_pred = get_bits1(&v->s.gb); }
            else
            { v->s.ac_pred = v->acpred_plane[mb_pos]; }

            if (v->condover == CONDOVER_SELECT)
            {
                if (v->overflg_is_raw)
                { overlap = get_bits1(&v->s.gb); }
                else
                { overlap = v->over_flags_plane[mb_pos]; }
            }
            else
            { overlap = (v->condover == CONDOVER_ALL); }

            GET_MQUANT();

            s->current_picture.qscale_table[mb_pos] = mquant;
            /* Set DC scale - y and c use the same */
            s->y_dc_scale = s->y_dc_scale_table[mquant];
            s->c_dc_scale = s->c_dc_scale_table[mquant];

            for (k = 0; k < 6; k++)
            {
                val = ((cbp >> (5 - k)) & 1);

                if (k < 4)
                {
                    int pred = vc1_coded_block_pred(&v->s, k, &coded_val);
                    val = val ^ pred;
                    *coded_val = val;
                }

                cbp |= val << (5 - k);

                v->a_avail = !s->first_slice_line || (k == 2 || k == 3);
                v->c_avail = !!s->mb_x || (k == 1 || k == 3);

                vc1_decode_i_block_adv(v, s->block[k], k, val, (k < 4) ? v->codingset : v->codingset2, mquant);

                s->dsp.vc1_inv_trans_8x8(s->block[k]);

                for (j = 0; j < 64; j++)
                { s->block[k][j] += 128; }
            }

            vc1_put_block(v, s->block);

            if (overlap)
            {
                if (s->mb_x)
                {
                    s->dsp.vc1_h_overlap(s->dest[0], s->linesize);
                    s->dsp.vc1_h_overlap(s->dest[0] + 8 * s->linesize, s->linesize);

                    if (!(s->flags & CODEC_FLAG_GRAY))
                    {
                        s->dsp.vc1_h_overlap(s->dest[1], s->uvlinesize);
                        s->dsp.vc1_h_overlap(s->dest[2], s->uvlinesize);
                    }
                }

                s->dsp.vc1_h_overlap(s->dest[0] + 8, s->linesize);
                s->dsp.vc1_h_overlap(s->dest[0] + 8 * s->linesize + 8, s->linesize);

                if (!s->first_slice_line)
                {
                    s->dsp.vc1_v_overlap(s->dest[0], s->linesize);
                    s->dsp.vc1_v_overlap(s->dest[0] + 8, s->linesize);

                    if (!(s->flags & CODEC_FLAG_GRAY))
                    {
                        s->dsp.vc1_v_overlap(s->dest[1], s->uvlinesize);
                        s->dsp.vc1_v_overlap(s->dest[2], s->uvlinesize);
                    }
                }

                s->dsp.vc1_v_overlap(s->dest[0] + 8 * s->linesize, s->linesize);
                s->dsp.vc1_v_overlap(s->dest[0] + 8 * s->linesize + 8, s->linesize);
            }

            if (v->s.loop_filter)
            { vc1_loop_filter_iblk(s, s->current_picture.qscale_table[mb_pos]); }

            if (get_bits_count(&s->gb) > v->bits)
            {
                ff_er_add_slice(s, 0, 0, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END));
                av_log(s->avctx, AV_LOG_ERROR, "Bits overconsumption: %i > %i\n", get_bits_count(&s->gb), v->bits);
                return;
            }
        }

        ff_draw_horiz_band(s, s->mb_y * 16, 16);
        s->first_slice_line = 0;
    }

    ff_er_add_slice(s, 0, 0, s->mb_width - 1, s->mb_height - 1, (AC_END | DC_END | MV_END));
}

static void vc1_decode_p_blocks(VC1Context *v)
{
    MpegEncContext *s = &v->s;

    /* select codingmode used for VLC tables selection */
    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset = (v->pqindex <= 8) ? CS_HIGH_RATE_INTRA : CS_LOW_MOT_INTRA;
            break;

        case 1:
            v->codingset = CS_HIGH_MOT_INTRA;
            break;

        case 2:
            v->codingset = CS_MID_RATE_INTRA;
            break;
    }

    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset2 = (v->pqindex <= 8) ? CS_HIGH_RATE_INTER : CS_LOW_MOT_INTER;
            break;

        case 1:
            v->codingset2 = CS_HIGH_MOT_INTER;
            break;

        case 2:
            v->codingset2 = CS_MID_RATE_INTER;
            break;
    }

#ifdef LXW_DEBUG

    if (frame_num == TARGET_FRAME)
    {
        av_log(s->avctx, AV_LOG_WARNING, "frame_num:%d codingset:%d codingset2:%d \n", frame_num, v->codingset, v->codingset2);
    }

#endif

    s->first_slice_line = 1;
    memset(v->cbp_base, 0, sizeof(v->cbp_base[0]) * 2 * s->mb_stride);

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++)
    {
        for (s->mb_x = 0; s->mb_x < s->mb_width; s->mb_x++)
        {
            ff_init_block_index(s);
            ff_update_block_index(s);
            s->dsp.clear_blocks(s->block[0]);

            vc1_decode_p_mb(v);

            if (get_bits_count(&s->gb) > v->bits || get_bits_count(&s->gb) < 0)
            {
                ff_er_add_slice(s, 0, 0, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END));
                /* 2010/06/23 17:30:00 liuxw+00139685 */
                /* 修改日志级别 */
                //              av_log(s->avctx, AV_LOG_ERROR, "Bits overconsumption: %i > %i at %ix%i\n", get_bits_count(&s->gb), v->bits,s->mb_x,s->mb_y);
                av_log(s->avctx, AV_LOG_WARNING, "Bits overconsumption: %i > %i at %ix%i\n", get_bits_count(&s->gb), v->bits, s->mb_x, s->mb_y);
                return;
            }
        }

        memmove(v->cbp_base, v->cbp, sizeof(v->cbp_base[0])*s->mb_stride);
        ff_draw_horiz_band(s, s->mb_y * 16, 16);
        s->first_slice_line = 0;
    }

    ff_er_add_slice(s, 0, 0, s->mb_width - 1, s->mb_height - 1, (AC_END | DC_END | MV_END));
}

static void vc1_decode_b_blocks(VC1Context *v)
{
    MpegEncContext *s = &v->s;

    /* select codingmode used for VLC tables selection */
    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset = (v->pqindex <= 8) ? CS_HIGH_RATE_INTRA : CS_LOW_MOT_INTRA;
            break;

        case 1:
            v->codingset = CS_HIGH_MOT_INTRA;
            break;

        case 2:
            v->codingset = CS_MID_RATE_INTRA;
            break;
    }

    switch (v->c_ac_table_index)
    {
        case 0:
            v->codingset2 = (v->pqindex <= 8) ? CS_HIGH_RATE_INTER : CS_LOW_MOT_INTER;
            break;

        case 1:
            v->codingset2 = CS_HIGH_MOT_INTER;
            break;

        case 2:
            v->codingset2 = CS_MID_RATE_INTER;
            break;
    }

#ifdef LXW_DEBUG

    if (frame_num == TARGET_FRAME)
    {
        av_log(s->avctx, AV_LOG_WARNING, "frame_num:%d codingset:%d codingset2:%d \n", frame_num, v->codingset, v->codingset2);
    }

#endif

    s->first_slice_line = 1;

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++)
    {
        for (s->mb_x = 0; s->mb_x < s->mb_width; s->mb_x++)
        {
            ff_init_block_index(s);
            ff_update_block_index(s);
            s->dsp.clear_blocks(s->block[0]);

            vc1_decode_b_mb(v);

            if (get_bits_count(&s->gb) > v->bits || get_bits_count(&s->gb) < 0)
            {
                ff_er_add_slice(s, 0, 0, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END));
                av_log(s->avctx, AV_LOG_ERROR, "Bits overconsumption: %i > %i at %ix%i\n", get_bits_count(&s->gb), v->bits, s->mb_x, s->mb_y);
                return;
            }

            if (v->s.loop_filter)
            { vc1_loop_filter_iblk(s, s->current_picture.qscale_table[s->mb_x + s->mb_y * s->mb_stride]); }
        }

        ff_draw_horiz_band(s, s->mb_y * 16, 16);
        s->first_slice_line = 0;
    }

    ff_er_add_slice(s, 0, 0, s->mb_width - 1, s->mb_height - 1, (AC_END | DC_END | MV_END));
}

static void vc1_decode_skip_blocks(VC1Context *v)
{
    MpegEncContext *s = &v->s;

    ff_er_add_slice(s, 0, 0, s->mb_width - 1, s->mb_height - 1, (AC_END | DC_END | MV_END));
    s->first_slice_line = 1;

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++)
    {
        s->mb_x = 0;
        ff_init_block_index(s);
        ff_update_block_index(s);
        memcpy(s->dest[0], s->last_picture.data[0] + s->mb_y * 16 * s->linesize, s->linesize * 16);
        memcpy(s->dest[1], s->last_picture.data[1] + s->mb_y * 8 * s->uvlinesize, s->uvlinesize * 8);
        memcpy(s->dest[2], s->last_picture.data[2] + s->mb_y * 8 * s->uvlinesize, s->uvlinesize * 8);
        ff_draw_horiz_band(s, s->mb_y * 16, 16);
        s->first_slice_line = 0;
    }

    s->pict_type = FF_P_TYPE;
}

static void vc1_decode_blocks(VC1Context *v)
{

    v->s.esc3_level_length = 0;

    if (v->x8_type)
    {
        ff_intrax8_decode_picture(&v->x8, 2 * v->pq + v->halfpq, v->pq * (!v->pquantizer) );
    }
    else
    {
        switch (v->s.pict_type)
        {
            case FF_I_TYPE:
                if (v->profile == PROFILE_ADVANCED)
                { vc1_decode_i_blocks_adv(v); }
                else
                { vc1_decode_i_blocks(v); }

                break;

            case FF_P_TYPE:
                if (v->p_frame_skipped)
                { vc1_decode_skip_blocks(v); }
                else
                { vc1_decode_p_blocks(v); }

                break;

            case FF_B_TYPE:
                if (v->bi_type)
                {
                    if (v->profile == PROFILE_ADVANCED)
                    { vc1_decode_i_blocks_adv(v); }
                    else
                    { vc1_decode_i_blocks(v); }
                }
                else
                { vc1_decode_b_blocks(v); }

                break;
        }
    }
}

/** Find VC-1 marker in buffer
 * @return position where next marker starts or end of buffer if no marker found
 */
static av_always_inline const uint8_t *find_next_marker(const uint8_t *src, const uint8_t *end)
{
    uint32_t mrk = 0xFFFFFFFF;

    if (end - src < 4) { return end; }

    while (src < end)
    {
        mrk = (mrk << 8) | *src++;

        if (IS_MARKER(mrk))
        { return src - 4; }
    }

    return end;
}

/* 2010/06/17 17:30:00 liuxw+00139685 */
/* 将vc1_unescape_buffer函数的静态属性去掉：在probe header中会调用此函数 */
/*static av_always_inline*/ int vc1_unescape_buffer(const uint8_t *src, int size, uint8_t *dst)
{
    int dsize = 0, i;

    if (size < 4)
    {
        for (dsize = 0; dsize < size; dsize++) { *dst++ = *src++; }

        return size;
    }

    for (i = 0; i < size; i++, src++)
    {
        if (src[0] == 3 && i >= 2 && !src[-1] && !src[-2] && i < size - 1 && src[1] < 4)
        {
            dst[dsize++] = src[1];
            src++;
            i++;
        }
        else
        { dst[dsize++] = *src; }
    }

    return dsize;
}

/* 2010/06/12 14:30:00 liuxw+00139685 */
/* 新增加一个函数，主要完成在解完序列头之后的一些初始化操作 */
static int alloc_table(AVCodecContext *avctx)
{
    VC1Context *v = avctx->priv_data;
    MpegEncContext *s = &v->s;

    avctx->has_b_frames = !!(avctx->max_b_frames);
    s->low_delay = !avctx->has_b_frames;

    s->mb_width = (avctx->coded_width + 15) >> 4;
    s->mb_height = (avctx->coded_height + 15) >> 4;

    /* Allocate mb bitplanes */
    v->mv_type_mb_plane = av_malloc_hw(avctx, s->mb_stride * s->mb_height);

    if (NULL == v->mv_type_mb_plane)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for mv_type_mb_plane is failed!\n");
        goto fail;
    }

    v->direct_mb_plane = av_malloc_hw(avctx, s->mb_stride * s->mb_height);

    if (NULL == v->direct_mb_plane)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for direct_mb_plane is failed!\n");
        goto fail;
    }

    v->acpred_plane = av_malloc_hw(avctx, s->mb_stride * s->mb_height);

    if (NULL == v->acpred_plane)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for acpred_plane is failed!\n");
        goto fail;
    }

    v->over_flags_plane = av_malloc_hw(avctx, s->mb_stride * s->mb_height);

    if (NULL == v->over_flags_plane)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for over_flags_plane is failed!\n");
        goto fail;
    }

    v->cbp_base = av_malloc_hw(avctx, sizeof(v->cbp_base[0]) * 2 * s->mb_stride);

    if (NULL == v->cbp_base)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for cbp_base is failed!\n");
        goto fail;
    }

    v->cbp = v->cbp_base + s->mb_stride;

    /* allocate block type info in that way so it could be used with s->block_index[] */
    v->mb_type_base = av_malloc_hw(avctx, s->b8_stride * (s->mb_height * 2 + 1) + s->mb_stride * (s->mb_height + 1) * 2);

    if (NULL == v->mb_type_base)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for mb_type_base is failed!\n");
        goto fail;
    }

    v->mb_type[0] = v->mb_type_base + s->b8_stride + 1;
    v->mb_type[1] = v->mb_type_base + s->b8_stride * (s->mb_height * 2 + 1) + s->mb_stride + 1;
    v->mb_type[2] = v->mb_type[1] + s->mb_stride * (s->mb_height + 1);

    ff_intrax8_common_init(&v->x8, s);

    return 0;

fail:
    avctx->iMallocFailed = 1;
    return -1;
}


/** Initialize a VC1/WMV3 decoder
 * @todo TODO: Handle VC-1 IDUs (Transport level?)
 * @todo TODO: Decypher remaining bits in extra_data
 */
static av_cold int vc1_decode_init(AVCodecContext *avctx)
{
    VC1Context *v = avctx->priv_data;
    MpegEncContext *s = &v->s;

    /* 2010/06/12 14:30:00 liuxw+00139685 */
    /* 由于调用流程的调整，这些语句在新的流程中在此函数里是不需要的 */
    //  GetBitContext gb;
    //  if (!avctx->extradata_size || !avctx->extradata) return -1;

    if (!(avctx->flags & CODEC_FLAG_GRAY))
    { avctx->pix_fmt = PIX_FMT_YUV420P; }
    else
    { avctx->pix_fmt = PIX_FMT_GRAY8; }

    v->s.avctx = avctx;
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
    v->s.flags |= CODEC_FLAG_EMU_EDGE;

    if (avctx->idct_algo == FF_IDCT_AUTO)
    {
        avctx->idct_algo = FF_IDCT_WMV2;
    }

    if (ff_h263_decode_init(avctx) < 0)
    { return -1; }

    if (vc1_init_common(v) < 0) { return -1; }

    /* 2010/06/12 14:30:00 liuxw+00139685 */
    /* 将以下函数全部注释，部分代码改放到新增加的函数alloc_table中去 */
#if 0
    avctx->coded_width = avctx->width;
    avctx->coded_height = avctx->height;

    if (avctx->codec_id == CODEC_ID_WMV3)
    {
        int count = 0;
        int i;

        // looks like WMV3 has a sequence header stored in the extradata
        // advanced sequence header may be before the first frame
        // the last byte of the extradata is a version number, 1 for the
        // samples we can decode
        /* 2010/05/27 10:00:00 liuxw+00139685 */
        /* 以前extradata,现在被修改为数组 */
        //      init_get_bits(&gb, avctx->extradata, avctx->extradata_size*8);
        for (i = 0; i < avctx->extradata_num; i++)
        {
            init_get_bits(&gb, avctx->extradata[count], avctx->extradata_size[count] * 8);

            if (decode_sequence_header(avctx, &gb) < 0)
            { return -1; }

            count = avctx->extradata_size[i] * 8 - get_bits_count(&gb);

            if (count > 0)
            {
                av_log(avctx, AV_LOG_INFO, "Extra data[%d]: %i bits left, value: %X\n",
                       i, count, get_bits(&gb, count));
            }
            else if (count < 0)
            {
                av_log(avctx, AV_LOG_INFO, "Read %i bits in overflow\n", -count);
            }
        }
    }
    else     // VC1/WVC1
    {
        /* 2010/05/27 10:00:00 liuxw+00139685 */
        /* 以前extradata,现在被修改为数组 */
        int i;
        int seq_initialized = 0, ep_initialized = 0;

        for (i = 0; i < avctx->extradata_num; i++)
        {
            const uint8_t *start = avctx->extradata[i];
            uint8_t *end = avctx->extradata[i] + avctx->extradata_size[i];
            const uint8_t *next;
            int size, buf2_size;
            uint8_t *buf2 = NULL;

            if (avctx->extradata_size[i] < 16)
            {
                av_log(avctx, AV_LOG_ERROR, "Extradata size too small: %i\n", avctx->extradata_size);
                return -1;
            }

            buf2 = av_mallocz(avctx->extradata_size[i] + FF_INPUT_BUFFER_PADDING_SIZE);
            in WVC1 extradata first byte is its size
            next = start;

            for (; next < end; start = next)
            {
                next = find_next_marker(start + 4, end);
                size = next - start - 4;

                if (size <= 0) { continue; }

                buf2_size = vc1_unescape_buffer(start + 4, size, buf2);
                init_get_bits(&gb, buf2, buf2_size * 8);

                switch (AV_RB32(start))
                {
                    case VC1_CODE_SEQHDR:
                        if (decode_sequence_header(avctx, &gb) < 0)
                        {
                            av_free(buf2);
                            return -1;
                        }

                        seq_initialized = 1;
                        break;

                    case VC1_CODE_ENTRYPOINT:
                        if (decode_entry_point(avctx, &gb) < 0)
                        {
                            av_free(buf2);
                            return -1;
                        }

                        ep_initialized = 1;
                        break;
                }
            }

            av_free(buf2);
        }

        if (!seq_initialized || !ep_initialized)
        {
            av_log(avctx, AV_LOG_ERROR, "Incomplete extradata\n");
            return -1;
        }
    }

    avctx->has_b_frames = !!(avctx->max_b_frames);
    s->low_delay = !avctx->has_b_frames;

    s->mb_width = (avctx->coded_width + 15) >> 4;
    s->mb_height = (avctx->coded_height + 15) >> 4;

    /* Allocate mb bitplanes */
    v->mv_type_mb_plane = av_malloc(s->mb_stride * s->mb_height);
    v->direct_mb_plane = av_malloc(s->mb_stride * s->mb_height);
    v->acpred_plane = av_malloc(s->mb_stride * s->mb_height);
    v->over_flags_plane = av_malloc(s->mb_stride * s->mb_height);

    v->cbp_base = av_malloc(sizeof(v->cbp_base[0]) * 2 * s->mb_stride);
    v->cbp = v->cbp_base + s->mb_stride;

    /* allocate block type info in that way so it could be used with s->block_index[] */
    v->mb_type_base = av_malloc(s->b8_stride * (s->mb_height * 2 + 1) + s->mb_stride * (s->mb_height + 1) * 2);
    v->mb_type[0] = v->mb_type_base + s->b8_stride + 1;
    v->mb_type[1] = v->mb_type_base + s->b8_stride * (s->mb_height * 2 + 1) + s->mb_stride + 1;
    v->mb_type[2] = v->mb_type[1] + s->mb_stride * (s->mb_height + 1);

    /* Init coded blocks info */
    if (v->profile == PROFILE_ADVANCED)
    {
        //        if (alloc_bitplane(&v->over_flags_plane, s->mb_width, s->mb_height) < 0)
        //            return -1;
        //        if (alloc_bitplane(&v->ac_pred_plane, s->mb_width, s->mb_height) < 0)
        //            return -1;
    }

    ff_intrax8_common_init(&v->x8, s);
#endif
#define RBDU_BUFSIZE (1920*1088*2)
    v->rbdu_buf = av_malloc_hw(avctx, RBDU_BUFSIZE);

    if (NULL == v->rbdu_buf)
    {
        av_log(avctx, AV_LOG_ERROR, "Malloc memory for rbdu buffer failed!\n");
        return -1;
    }

    /* 2010/06/17 15:30:00 liuxw+00139685 */
    /* 在初始化函数中增加parse_context状态初始化(原来是在mpv_common_init中实现的) */
    s->parse_context.state = -1;

    return 0;
}

int ff_vc1_decode_rbdu_trailing(VC1Context *v, const uint8_t *src)
{
    int s = *src;
    int r;

    tprintf(v->s.avctx, "rbsp trailing %X\n", s);

    for (r = 1; r < 9; r++)
    {
        if (s & 1) { return r; }

        s >>= 1;
    }

    return 0;
}

/* 2010/06/12 15:30:00 liuxw+00139685 */
/* 新增函数：实现解码BDU(bitstream data unit) */
static int vc1_decode_bdu(VC1Context *v, const uint8_t *buf, int buf_size)
{
    MpegEncContext *s = &v->s;
    AVCodecContext *avctx = s->avctx;

    const uint8_t *start = buf;
    const uint8_t *end = start + buf_size;
    const uint8_t *next;
    int size, buf2_size;
    int bit_length;
    int context_count = 0;

    s->current_picture_ptr = NULL;  /* l00139685+20100617 init every frame */

    /* 2010/06/13 15:30:00 liuxw+00139685 */
    /* 首先得到第一个起始 */
    next = find_next_marker(start, end);

    if (next == end)
    {
        av_log(avctx, AV_LOG_WARNING, "Can not find the start code!\n");
        return buf_size;
    }

    start = next;

    for (; next < end; start = next)
    {
        next = find_next_marker(start + 4, end);
        size = next - start - 4;

        if (size <= 0 || size > RBDU_BUFSIZE)
        {
            av_log(avctx, AV_LOG_WARNING, "Bitstream has something wrong!\n");
            continue;
        }

        buf2_size = vc1_unescape_buffer(start + 4, size, v->rbdu_buf);

        /* 2010/06/13 11:30:00 liuxw+00139685 */
        /* 增加去trainling bit 和sufix leading zero bytes(只有vc1才会运行下列代码) */
        while (avctx->codec_id == CODEC_ID_VC1 && v->rbdu_buf[buf2_size - 1] == 0 && buf2_size > 0)
        { buf2_size--; }

        bit_length = (avctx->codec_id == CODEC_ID_WMV3) ? 8 * buf2_size : (8 * buf2_size - ff_vc1_decode_rbdu_trailing(v, v->rbdu_buf + buf2_size - 1));

        if (bit_length <= 0)
        { continue; }

        init_get_bits(&s->gb, v->rbdu_buf, bit_length);

        switch (AV_RB32(start))
        {
            case VC1_CODE_SEQHDR:
                if (avctx->codec_id == CODEC_ID_VC1)
                {
                    if (decode_sequence_header_adv(v, &s->gb) < 0)
                    {
                        av_log(avctx, AV_LOG_WARNING, "decode vc1 sequence header failed!\n");
                        continue;
                    }

                    avctx->iChromaFormat = v->chromaformat;
                    v->seq_initialized = 1;
                }
                else
                {
                    if (decode_sequence_header(avctx, &s->gb) < 0)
                    {
                        av_log(avctx, AV_LOG_WARNING, "decode wmv3 sequence header failed!\n");
                        continue;
                    }

                    avctx->iChromaFormat = 1;  //sp/mp 没有语法元素来表示chroma format，所以置为1(420)即可
                    v->seq_initialized = v->ep_initialized = 1;
                }

                /* 检测图像宽高的合法性 */
                if (avctx->usActualWidth && avctx->usActualHeight)
                {
                    if (avctx->usActualWidth != avctx->coded_width || avctx->usActualHeight != avctx->coded_height)
                    {
                        av_log(avctx, AV_LOG_WARNING, "Not support width/height change dynamicly!\n");
                        avctx->coded_width  = avctx->usActualWidth;
                        avctx->coded_height = avctx->usActualHeight;
                    }
                }
                else
                {
                    avctx->usActualWidth  = avctx->coded_width;
                    avctx->usActualHeight = avctx->coded_height;
                }

                /* 更新码流信息和解码器状态信息 */
                s->avctx->iActualProfile = v->profile;
                s->avctx->iActualLevel   = (v->profile == PROFILE_ADVANCED) ? v->level : (-1);
                s->avctx->iActualRefNum  = 1;
                s->avctx->iChromaFormat  = 1;

                break;

            case VC1_CODE_ENTRYPOINT:
                if (avctx->codec_id == CODEC_ID_VC1)
                {
                    if (decode_entry_point(avctx, &s->gb) < 0)
                    {
                        av_log(avctx, AV_LOG_WARNING, "decode vc1 entyr point failed!\n");
                        continue;
                    }

                    v->ep_initialized = 1;
                }
                else
                {
                    av_log(avctx, AV_LOG_WARNING, "WMV3 not support entry point!\n");
                    continue;
                }

                avctx->usActualWidth  = avctx->coded_width;
                avctx->usActualHeight = avctx->coded_height;

                break;

            case VC1_CODE_FRAME:
            {
                if (!v->seq_initialized || !v->ep_initialized)
                {
                    av_log(avctx, AV_LOG_WARNING, "Sequence header or entry point is not init!\n");
                    avctx->uiDiscardFrames++;
                    return buf_size;
                }

                s->width =  avctx->coded_width;
                s->height = avctx->coded_height;

                if (!s->context_initialized)
                {
                    if (MPV_common_init(s) < 0)
                    {
                        return -1;
                    }

                    if (alloc_table(avctx) < 0)
                    {
                        return -1;
                    }

                    s->context_initialized = 1;
                }

                if (avctx->codec_id == CODEC_ID_VC1)
                {
                    if (vc1_parse_frame_header_adv(v, &s->gb) == -1)
                    {
                        av_log(avctx, AV_LOG_WARNING, "Decode frame header failed!\n");
                        //							return -1;
                        avctx->uiDiscardFrames++;
                        continue;
                    }
                }
                else
                {
                    if (vc1_parse_frame_header(v, &s->gb) == -1)
                    {
                        av_log(avctx, AV_LOG_WARNING, "Decode frame header failed!\n");
                        //							return -1;
                        avctx->uiDiscardFrames++;
                        continue;
                    }
                }

                context_count++;
            }
            break;

            case VC1_CODE_FIELD:
            {
                av_log(avctx, AV_LOG_WARNING, "Not supporte field interlaced currently!\n");
            }
            break;

            case VC1_CODE_SLICE:
            {
                av_log(avctx, AV_LOG_WARNING, "vc1 slice not supported now!\n");
            }
            break;

            case VC1_CODE_ENDOFSEQ:
            {
                av_log(avctx, AV_LOG_INFO, "Bitstream is over!\n");
            }
            break;

            default:
                av_log(avctx, AV_LOG_WARNING, "Not support start code[0x%x]\n", AV_RB32(start));
                break;
        }

        if (1 == context_count)
        {
            /* 2010/06/19 10:30:00 liuxw+00139685 */
            /* res_rtm_flag在解析时就已经保证不会等于0 */
            //			if(s->pict_type != FF_I_TYPE && !v->res_rtm_flag)
            //			{
            //				return -1;
            //			}

            s->current_picture.pict_type = s->pict_type;
            s->current_picture.key_frame = (s->pict_type == FF_I_TYPE);

            /* skip B-frames if we don't have reference frames */
            if (NULL == s->last_picture_ptr && (FF_B_TYPE == s->pict_type || s->dropable))
            {
                /* 2010/06/19 11:00:00 liuxw+00139685 */
                /* 增加日志打印 */
                av_log(avctx, AV_LOG_WARNING, "Lost foreward reference picture for B picture!\n");
                avctx->uiDiscardFrames++;
                return -1;//buf_size;
            }

            if (s->next_p_frame_damaged)
            {
                if (FF_B_TYPE == s->pict_type)
                {
                    /* 2010/06/19 11:00:00 liuxw+00139685 */
                    /* 增加日志打印 */
                    av_log(avctx, AV_LOG_WARNING, "Backward reference picture for B picture is dmamged!\n");
                    avctx->uiDiscardFrames++;
                    //					return buf_size;
                    return -1;
                }
                else
                { s->next_p_frame_damaged = 0; }
            }

            if (MPV_frame_start(s, avctx) < 0)
            {
                return -1;
            }

            /* 2010/08/26 14:00:00 liuxw+00139685 */
            /* 增加rangedfrm功能 */
            s->current_picture_ptr->pict_type = v->s.pict_type;
            s->current_picture_ptr->rangedfrm = v->rangeredfrm;

            if (v->rangered && (FF_P_TYPE == v->s.pict_type || (FF_B_TYPE == v->s.pict_type && !v->bi_type)) && (v->rangeredfrm ^ s->last_picture_ptr->rangedfrm))
            {
                int i , j;
                unsigned char *p = NULL;

                if (FF_B_TYPE == v->s.pict_type && v->rangeredfrm != s->next_picture_ptr->rangedfrm)
                {
                    av_log(v->s.avctx, AV_LOG_WARNING, "Current is B frame,and rangedfrm is different between Cur and Next!\n");
                    s->current_picture_ptr->rangedfrm = s->next_picture_ptr->rangedfrm;
                }

                if (v->rangeredfrm)
                {
                    p = s->last_picture_ptr->data[0];

                    for (i = 0; i < v->s.avctx->height; i++)
                    {
                        for (j = 0; j < v->s.avctx->width; j++)
                        {
                            p[j] = ((p[j] - 128) >> 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[0];
                    }

                    p = s->last_picture_ptr->data[1];

                    for (i = 0; i < v->s.avctx->height / 2; i++)
                    {
                        for (j = 0; j < v->s.avctx->width / 2; j++)
                        {
                            p[j] = ((p[j] - 128) >> 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[1];
                    }

                    p = s->last_picture_ptr->data[2];

                    for (i = 0; i < v->s.avctx->height / 2; i++)
                    {
                        for (j = 0; j < v->s.avctx->width / 2; j++)
                        {
                            p[j] = ((p[j] - 128) >> 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[2];
                    }

                    if (!s->low_delay)
                    {
                        s->last_picture_ptr->recovery = 1;
                    }

                    s->last_picture_ptr->rangedfrm = 1;
                }
                else
                {
                    p = s->last_picture_ptr->data[0];

                    for (i = 0; i < v->s.avctx->height; i++)
                    {
                        for (j = 0; j < v->s.avctx->width; j++)
                        {
                            p[j] = ((p[j] - 128) << 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[0];
                    }

                    p = s->last_picture_ptr->data[1];

                    for (i = 0; i < v->s.avctx->height / 2; i++)
                    {
                        for (j = 0; j < v->s.avctx->width / 2; j++)
                        {
                            p[j] = ((p[j] - 128) << 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[1];
                    }

                    p = s->last_picture_ptr->data[2];

                    for (i = 0; i < v->s.avctx->height / 2; i++)
                    {
                        for (j = 0; j < v->s.avctx->width / 2; j++)
                        {
                            p[j] = ((p[j] - 128) << 1) + 128;
                        }

                        p += s->current_picture_ptr->linesize[2];
                    }

                    if (!s->low_delay)
                    {
                        s->last_picture_ptr->recovery = 2;
                    }

                    s->last_picture_ptr->rangedfrm = 0;
                }
            }

            s->me.qpel_put = s->dsp.put_qpel_pixels_tab;
            s->me.qpel_avg = s->dsp.avg_qpel_pixels_tab;

            ff_er_frame_start(s);

            //			v->bits = buf_size * 8;
            v->bits = s->gb.size_in_bits;

            vc1_decode_blocks(v);

            context_count = 0;
        }
    }

    return buf_size;
}

/** Decode a VC1/WMV3 frame
 * @todo TODO: Handle VC-1 IDUs (Transport level?)
 */
static int vc1_decode_frame(AVCodecContext *avctx,
                            void *data, int *data_size,
                            const uint8_t *buf, int buf_size)
{
    VC1Context *v = avctx->priv_data;
    MpegEncContext *s = &v->s;
    AVFrame *pict = data;
    //  uint8_t *buf2 = NULL;
    //  const uint8_t *buf_start = buf;
    int iRet = 0;

    /* no supplementary picture */
    if (buf_size == 0)
    {
        /* special case for last picture */
        if (s->low_delay == 0 && s->next_picture_ptr)
        {
            *pict = *(AVFrame *)s->next_picture_ptr;
            s->next_picture_ptr = NULL;
            *data_size = sizeof(AVFrame);

            /* 2010/06/12 14:30:00 liuxw+00139685 */
            /* 置位最后一帧标志 */
            pict->ucLastFrame = 1;
        }

        return 0;
    }

    /* We need to set current_picture_ptr before reading the header,
     * otherwise we cannot store anything in there. */
    //if(s->current_picture_ptr==NULL || s->current_picture_ptr->data[0]){
    //    int i= ff_find_unused_picture(s, 0);
    //    s->current_picture_ptr= &s->picture[i];
    //}

    if (s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
    {
        /* 2010/07/15 16:30:00 liuxw+00139685 [AZ1D02221] */
        /* 由于profile的宏定义进行过修改，导致main profile比advanced profile大，所以把条件反转过来 */
        //if(v->profile < PROFILE_ADVANCED)
        if (v->profile > PROFILE_ADVANCED)
        { avctx->pix_fmt = PIX_FMT_VDPAU_WMV3; }
        else
        { avctx->pix_fmt = PIX_FMT_VDPAU_VC1; }
    }

    /* 2010/06/13 16:30:00 liuxw+00139685 */
    /* 将下面的具体解码过程封装到一个新函数中去 */
#if 0

    //for advanced profile we may need to parse and unescape data
    if (avctx->codec_id == CODEC_ID_VC1)
    {
        int buf_size2 = 0;
        buf2 = av_mallocz(buf_size + FF_INPUT_BUFFER_PADDING_SIZE);

        if (IS_MARKER(AV_RB32(buf))) /* frame starts with marker and needs to be parsed */
        {
            const uint8_t *start, *end, *next;
            int size;

            next = buf;

            for (start = buf, end = buf + buf_size; next < end; start = next)
            {
                next = find_next_marker(start + 4, end);
                size = next - start - 4;

                if (size <= 0) { continue; }

                switch (AV_RB32(start))
                {
                    case VC1_CODE_FRAME:
                        if (s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
                        { buf_start = start; }

                        buf_size2 = vc1_unescape_buffer(start + 4, size, buf2);
                        break;

                    case VC1_CODE_ENTRYPOINT: /* it should be before frame data */
                        buf_size2 = vc1_unescape_buffer(start + 4, size, buf2);
                        init_get_bits(&s->gb, buf2, buf_size2 * 8);
                        decode_entry_point(avctx, &s->gb);
                        break;

                    case VC1_CODE_SLICE:
                        av_log(avctx, AV_LOG_ERROR, "Sliced decoding is not implemented (yet)\n");
                        av_free(buf2);
                        return -1;
                }
            }
        }
        else if (v->interlace && ((buf[0] & 0xC0) == 0xC0))  /* WVC1 interlaced stores both fields divided by marker */
        {
            const uint8_t *divider;

            divider = find_next_marker(buf, buf + buf_size);

            if ((divider == (buf + buf_size)) || AV_RB32(divider) != VC1_CODE_FIELD)
            {
                av_log(avctx, AV_LOG_ERROR, "Error in WVC1 interlaced frame\n");
                av_free(buf2);
                return -1;
            }

            buf_size2 = vc1_unescape_buffer(buf, divider - buf, buf2);
            // TODO
            av_free(buf2);
            return -1;
        }
        else
        {
            buf_size2 = vc1_unescape_buffer(buf, buf_size, buf2);
        }

        init_get_bits(&s->gb, buf2, buf_size2 * 8);
    }
    else
    { init_get_bits(&s->gb, buf, buf_size * 8); }

    // do parse frame header
    if (v->profile < PROFILE_ADVANCED)
    {
        if (vc1_parse_frame_header(v, &s->gb) == -1)
        {
            av_free(buf2);
            return -1;
        }
    }
    else
    {
        if (vc1_parse_frame_header_adv(v, &s->gb) == -1)
        {
            av_free(buf2);
            return -1;
        }
    }

    if (s->pict_type != FF_I_TYPE && !v->res_rtm_flag)
    {
        av_free(buf2);
        return -1;
    }

    // for hurry_up==5
    s->current_picture.pict_type = s->pict_type;
    s->current_picture.key_frame = s->pict_type == FF_I_TYPE;

    /* skip B-frames if we don't have reference frames */
    if (s->last_picture_ptr == NULL && (s->pict_type == FF_B_TYPE || s->dropable))
    {
        av_free(buf2);
        return -1;//buf_size;
    }

    /* skip b frames if we are in a hurry */
    if (avctx->hurry_up && s->pict_type == FF_B_TYPE) { return -1; } //buf_size;

    if (   (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type == FF_B_TYPE)
           || (avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type != FF_I_TYPE)
           ||  avctx->skip_frame >= AVDISCARD_ALL)
    {
        av_free(buf2);
        return buf_size;
    }

    /* skip everything if we are in a hurry>=5 */
    if (avctx->hurry_up >= 5)
    {
        av_free(buf2);
        return -1;//buf_size;
    }

    if (s->next_p_frame_damaged)
    {
        if (s->pict_type == FF_B_TYPE)
        { return buf_size; }
        else
        { s->next_p_frame_damaged = 0; }
    }

    if (MPV_frame_start(s, avctx) < 0)
    {
        av_free(buf2);
        return -1;
    }

    s->me.qpel_put = s->dsp.put_qpel_pixels_tab;
    s->me.qpel_avg = s->dsp.avg_qpel_pixels_tab;

    if ((CONFIG_VC1_VDPAU_DECODER || CONFIG_WMV3_VDPAU_DECODER)
        && s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)
    {
        /* 2010/05/31 17:00:00 liuxw+00139685 */
        /* 注释掉此函数：不支持硬件加速功能 */
        //      ff_vdpau_vc1_decode_picture(s, buf_start, (buf + buf_size) - buf_start);
        ;
    }
    else
    {
        ff_er_frame_start(s);

        v->bits = buf_size * 8;
        vc1_decode_blocks(v);
        //av_log(s->avctx, AV_LOG_INFO, "Consumed %i/%i bits\n", get_bits_count(&s->gb), buf_size*8);
        //  if(get_bits_count(&s->gb) > buf_size * 8)
        //      return -1;
        ff_er_frame_end(s);
    }

#endif

    if (!v->first_time && avctx->extradata_num)
    {
        int i;

        for (i = 0; i < avctx->extradata_num; i++)
        {
            iRet = vc1_decode_bdu(v, avctx->extradata[i], avctx->extradata_size[i]);

            if (iRet < 0)
            {
                return -1;
            }
        }

        v->first_time = 1;
    }

    iRet = vc1_decode_bdu(v, buf, buf_size);

    if (iRet < 0 || NULL == s->current_picture_ptr)
    {
        return buf_size;
    }

    ff_er_frame_end(s);

    MPV_frame_end(s);

    /* 2010/06/19 11:30:00 liuxw+00139685 */
    /* 更新码流信息和解码器状态信息 */
    s->current_picture_ptr->pict_type = s->pict_type;
    s->current_picture_ptr->interlaced_frame = (v->fcm == 3);
    s->current_picture_ptr->iErrorCode |= avctx->iErrorCode;

    assert(s->current_picture.pict_type == s->current_picture_ptr->pict_type);
    assert(s->current_picture.pict_type == s->pict_type);

    if (s->pict_type == FF_B_TYPE || s->low_delay)
    {
        *pict = *(AVFrame *)s->current_picture_ptr;
    }
    else if (s->last_picture_ptr != NULL)
    {
        *pict = *(AVFrame *)s->last_picture_ptr;
    }

    if (s->last_picture_ptr || s->low_delay)
    {
        *data_size = sizeof(AVFrame);
        ff_print_debug_info(s, pict);
    }

    /* 2010/08/30 10:00:00 liuxw+00139685 */
    /* 若当前有延迟，而且被延迟的帧又做了rangedfrm，所以在输出的时候要进行恢复 */
    if ((*data_size) && pict->recovery == 1)
    {
        unsigned char *p = NULL;
        int i, j;
        p = s->last_picture_ptr->data[0];

        for (i = 0; i < v->s.avctx->height; i++)
        {
            for (j = 0; j < v->s.avctx->width; j++)
            {
                p[j] = ((p[j] - 128) << 1) + 128;
            }

            p += s->current_picture_ptr->linesize[0];
        }

        p = s->last_picture_ptr->data[1];

        for (i = 0; i < v->s.avctx->height / 2; i++)
        {
            for (j = 0; j < v->s.avctx->width / 2; j++)
            {
                p[j] = ((p[j] - 128) << 1) + 128;
            }

            p += s->current_picture_ptr->linesize[1];
        }

        p = s->last_picture_ptr->data[2];

        for (i = 0; i < v->s.avctx->height / 2; i++)
        {
            for (j = 0; j < v->s.avctx->width / 2; j++)
            {
                p[j] = ((p[j] - 128) << 1) + 128;
            }

            p += s->current_picture_ptr->linesize[2];
        }

        s->last_picture_ptr->recovery  = 0;
        s->last_picture_ptr->rangedfrm = 0;
    }
    else if ((*data_size) && pict->recovery == 2)
    {
        unsigned char *p = NULL;
        int i, j;
        p = s->last_picture_ptr->data[0];

        for (i = 0; i < v->s.avctx->height; i++)
        {
            for (j = 0; j < v->s.avctx->width; j++)
            {
                p[j] = ((p[j] - 128) >> 1) + 128;
            }

            p += s->current_picture_ptr->linesize[0];
        }

        p = s->last_picture_ptr->data[1];

        for (i = 0; i < v->s.avctx->height / 2; i++)
        {
            for (j = 0; j < v->s.avctx->width / 2; j++)
            {
                p[j] = ((p[j] - 128) >> 1) + 128;
            }

            p += s->current_picture_ptr->linesize[1];
        }

        p = s->last_picture_ptr->data[2];

        for (i = 0; i < v->s.avctx->height / 2; i++)
        {
            for (j = 0; j < v->s.avctx->width / 2; j++)
            {
                p[j] = ((p[j] - 128) >> 1) + 128;
            }

            p += s->current_picture_ptr->linesize[2];
        }

        s->last_picture_ptr->recovery  = 0;
        s->last_picture_ptr->rangedfrm = 1;
    }

    /* Return the Picture timestamp as the frame number */
    /* we subtract 1 because it is added on utils.c     */
    avctx->frame_number = s->picture_number - 1;

#ifdef LXW_DEBUG
    {
        char pic_type[3][10] = {"I frame", "P frame", "B frame"};
        av_log(avctx, AV_LOG_WARNING, "frame num:%d frame type: %s\n", frame_num, pic_type[s->pict_type - 1]);

        if (*data_size)
        {
            av_log(avctx, AV_LOG_WARNING, "output num: %d frame_type: %s frame_num: %d\n", output_num++, pic_type[pict->pict_type - 1], pict->coded_picture_number);
        }

        frame_num++;
    }
#endif


    //  av_free(buf2);
    //  return buf_size;
    return iRet;
}

/** Close a VC1/WMV3 decoder
 * @warning Initial try at using MpegEncContext stuff
 */
static av_cold int vc1_decode_end(AVCodecContext *avctx)
{
    VC1Context *v = avctx->priv_data;

    av_freep(&v->hrd_rate);
    av_freep(&v->hrd_buffer);
    MPV_common_end(&v->s);
    av_freep(&v->mv_type_mb_plane);
    av_freep(&v->direct_mb_plane);
    av_freep(&v->acpred_plane);
    av_freep(&v->over_flags_plane);
    av_freep(&v->mb_type_base);
    av_freep(&v->cbp_base);
    ff_intrax8_common_end(&v->x8);

    /* 2010/06/18 16:30:00 liuxw+00139685 */
    /* 释放分配的vlc table所占的空间 */
    /* 2010/08/31 11:30:00 liuxw+00139685 [AZ1D02297] */
    /* 将动态分配的VLC空间全部转换为静态数组，所以无需内存释放 */
    //	msmpeg4_vlc_table_free();
    //	vc1_vlc_talbe_free();

    /* 2010/06/13 14:30:00 liuxw+00139685 */
    /* 释放新增的空间 */
    av_freep(&v->rbdu_buf);

    return 0;
}

#if 0
/* 2010/06/18 16:30:00 liuxw+00139685 */
/* 新增函数：在复位时调用(不使用vc1_decode_end,同因为它会释放所有的vlc表) */
static av_cold int vc1_decode_end_hw(AVCodecContext *avctx)
{
    VC1Context *v = avctx->priv_data;

    av_freep(&v->hrd_rate);
    av_freep(&v->hrd_buffer);
    MPV_common_end(&v->s);
    av_freep(&v->mv_type_mb_plane);
    av_freep(&v->direct_mb_plane);
    av_freep(&v->acpred_plane);
    av_freep(&v->over_flags_plane);
    av_freep(&v->mb_type_base);
    av_freep(&v->cbp_base);
    ff_intrax8_common_end(&v->x8);

    /* 2010/06/13 14:30:00 liuxw+00139685 */
    /* 释放新增的空间 */
    av_freep(&v->rbdu_buf);

    return 0;
}
#endif

/* 新增函数：复位WMV3/VC1解码器 */
static av_cold int vc1_reset(VC1Context *v)
{
    int iRet = 0;

    MpegEncContext *const	s		= &v->s;
    AVCodecContext 		*avctx	= s->avctx;

    /* 2010/08/18 15:30:00 liuxw+00139685 [AZ1D02265] */
    /* 复位时并不释放动态分配的table，所以必须保留当前table是否已分配的标志 */
    //int s_done = s->done;
    //int v_done = v->done;

    /* 清空VC1Context结构体变量 */
    memset(v, 0, sizeof(VC1Context));

    /* 2010/08/18 15:30:00 liuxw+00139685 [AZ1D02265] */
    /* 恢复其保留的标志 */
    //v->done = v_done;
    //s->done = s_done;

    /* 初始化VC1Context结构体变量 */
    iRet = vc1_decode_init(avctx);

    if (0 != iRet)
    {
        av_log(avctx, AV_LOG_ERROR, "ctx_dec[%p] vc1_ctx[%p] decode_init() failed! return code: %d !\n", avctx, v, iRet);
    }

    return iRet;
}
/* 2010/06/13 17:30:00 liuxw+00139685 */
/* 新增函数:WMV3/VC1 解码器复位函数 */
static av_cold int decode_reset(AVCodecContext *avctx)
{

    int iRet = 0;

    VC1Context *v = avctx->priv_data;
    MpegEncContext *s = &v->s;

    /*  除了不释放在init时分配的vlc表之外，其它的内存全部释放 */
    //	iRet = vc1_decode_end_hw(avctx);
    iRet = vc1_decode_end(avctx);

    /* 复位及初始化AVCodecContext结构体变量 */
    iRet = avcodec_reset(avctx);

    if (0 != iRet)
    {
        av_log(avctx, AV_LOG_WARNING, "ctx_dec[%p] avcodec_reset() failed! return code: %d !\n", avctx, iRet);
        return iRet;
    }

    /* 复位及初始化H264Context结构体变量 */
    iRet = vc1_reset(v);

    if (0 != iRet)
    {
        av_log(avctx, AV_LOG_WARNING, "ctx_dec[%p] vc1_ctx[%p] vc1 reset failed! return code: %d !\n", avctx, v, iRet);
        return iRet;
    }

    return iRet;
}


//#if __STDC_VERSION__ >= 199901L
//AVCodec vc1_decoder = {
//    "vc1",
//    CODEC_TYPE_VIDEO,
//    CODEC_ID_VC1,
//    sizeof(VC1Context),
//    vc1_decode_init,
//    NULL,
//    vc1_decode_end,
//    vc1_decode_frame,
//    CODEC_CAP_DELAY,
//    NULL,
//    .long_name = NULL_IF_CONFIG_SMALL("SMPTE VC-1"),
//    .pix_fmts = ff_pixfmt_list_420
//};
//#else
AVCodec vc1_decoder =
{
    "vc1",
    CODEC_TYPE_VIDEO,
    CODEC_ID_VC1,
    sizeof(VC1Context),
    vc1_decode_init,
    NULL,
    vc1_decode_end,
    vc1_decode_frame,
    CODEC_CAP_DELAY,
    NULL,
    ff_mpeg_flush,  //flush_dpb
    NULL,
    ff_pixfmt_list_420,
    NULL_IF_CONFIG_SMALL("SMPTE VC-1"),
    NULL,
    NULL,
    NULL,
    VC1_Frame_Parse,  // VC1_Frame_Parse
    decode_reset   //decode_reset
};
//#endif

//#if __STDC_VERSION__ >= 199901L
//AVCodec wmv3_decoder = {
//    "wmv3",
//    CODEC_TYPE_VIDEO,
//    CODEC_ID_WMV3,
//    sizeof(VC1Context),
//    vc1_decode_init,
//    NULL,
//    vc1_decode_end,
//    vc1_decode_frame,
//    CODEC_CAP_DELAY,
//    NULL,
//    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Video 9"),
//    .pix_fmts = ff_pixfmt_list_420
//};
//#else
AVCodec wmv3_decoder =
{
    "wmv3",
    CODEC_TYPE_VIDEO,
    CODEC_ID_WMV3,
    sizeof(VC1Context),
    vc1_decode_init,
    NULL,
    vc1_decode_end,
    vc1_decode_frame,
    CODEC_CAP_DELAY,
    NULL,
    ff_mpeg_flush,   //flush_dpb
    NULL,
    ff_pixfmt_list_420,
    NULL_IF_CONFIG_SMALL("Windows Media Video 9"),
    NULL,
    NULL,
    NULL,
    VC1_Frame_Parse,  // WMV3_Frame_Parse
    decode_reset   //decode_reset
};
//#endif

#if CONFIG_WMV3_VDPAU_DECODER
AVCodec wmv3_vdpau_decoder =
{
    "wmv3_vdpau",
    CODEC_TYPE_VIDEO,
    CODEC_ID_WMV3,
    sizeof(VC1Context),
    vc1_decode_init,
    NULL,
    vc1_decode_end,
    vc1_decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY | CODEC_CAP_HWACCEL_VDPAU,
    NULL,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Video 9 VDPAU"),
    .pix_fmts = (enum PixelFormat[]) {PIX_FMT_VDPAU_WMV3, PIX_FMT_NONE}
};
#endif

#if CONFIG_VC1_VDPAU_DECODER
AVCodec vc1_vdpau_decoder =
{
    "vc1_vdpau",
    CODEC_TYPE_VIDEO,
    CODEC_ID_VC1,
    sizeof(VC1Context),
    vc1_decode_init,
    NULL,
    vc1_decode_end,
    vc1_decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY | CODEC_CAP_HWACCEL_VDPAU,
    NULL,
    .long_name = NULL_IF_CONFIG_SMALL("SMPTE VC-1 VDPAU"),
    .pix_fmts = (enum PixelFormat[]) {PIX_FMT_VDPAU_VC1, PIX_FMT_NONE}
};
#endif
