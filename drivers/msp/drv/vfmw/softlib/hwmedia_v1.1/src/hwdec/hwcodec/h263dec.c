/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroTg/gb7m//Rv+EIzshynaQyAWDP4BRlrwgdv8v5
rZsbnZJVYx6e+pyYQgDto/J3U8UF8ZXFMbZUbLv9s0iDkWVxOAom6C8GgI5bCWsxVyCrgpNP
x8ShwIS4kr79I/p/7/FJK1afCqZ5e8d+plTei+I1ZuZLa+2JY4ptg+cJHsKEXQ==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/*
 * H.263 decoder
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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
 * @file libavcodec/h263dec.c
 * H.263 decoder.
 */


#include "avcodec.h"
#include "dsputil.h"
#include "internal.h"
#include "mpegvideo.h"
#include "h263_parser.h"
#include "mpeg4video_parser.h"
#include "msmpeg4.h"

//#define DEBUG
//#define PRINT_FRAME_TIME
int alloc_picture(MpegEncContext *s, Picture *pic, int shared);

av_cold int ff_h263_decode_init(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
    /* 2010/06/18 18:30:00 liuxw+00139685 */
    /* 增加返回码 */
    int iRet = 0;

    s->avctx = avctx;
    s->out_format = FMT_H263;

    /* 2010/09/14 15:30:00 liuxw+00139685 */
    /* 将avctx中保存的静态参数中的宽、高赋给s */
    //s->width  = avctx->coded_width;
    //s->height = avctx->coded_height;
    s->width  = avctx->usSourceWidth;
    s->height = avctx->usSourceHeight;
    s->workaround_bugs = avctx->workaround_bugs;

    // set defaults
    MPV_decode_defaults(s);
    s->quant_precision = 5;
    s->decode_mb = ff_h263_decode_mb;
    s->low_delay = 1;
    avctx->pix_fmt = avctx->get_format(avctx, avctx->codec->pix_fmts);
    s->unrestricted_mv = 1;

    /* select sub codec */
    switch (avctx->codec->id)
    {
        case CODEC_ID_H263:
            s->unrestricted_mv = 0;
            break;

        case CODEC_ID_MPEG4:
            s->decode_mb = ff_mpeg4_decode_mb;
            s->time_increment_bits = 4; /* default value for broken headers */
            s->h263_pred = 1;
            s->low_delay = 0; //default, might be overridden in the vol header during header parsing
            break;
            /*guoshan + 00101841 20100415*/
            /*以下解码格式不支持，改为报错处理*/
            //   case CODEC_ID_MSMPEG4V1:
            //         s->h263_msmpeg4 = 1;
            //         s->h263_pred = 1;
            //         s->msmpeg4_version=1;
            //         break;
            //    case CODEC_ID_MSMPEG4V2:
            //         s->h263_msmpeg4 = 1;
            //         s->h263_pred = 1;
            //         s->msmpeg4_version=2;
            //         break;
            //    case CODEC_ID_MSMPEG4V3:
            //         s->h263_msmpeg4 = 1;
            //         s->h263_pred = 1;
            //         s->msmpeg4_version=3;
            //         break;
            //    case CODEC_ID_WMV1:
            //         s->h263_msmpeg4 = 1;
            //         s->h263_pred = 1;
            //         s->msmpeg4_version=4;
            //         break;
            //    case CODEC_ID_WMV2:
            //         s->h263_msmpeg4 = 1;
            //         s->h263_pred = 1;
            //         s->msmpeg4_version=5;
            break;

        case CODEC_ID_VC1:
        case CODEC_ID_WMV3:
            s->h263_msmpeg4 = 1;
            s->h263_pred = 1;
            s->msmpeg4_version = 6;
            break;

            //    case CODEC_ID_H263I:
            // break;
        case CODEC_ID_FLV1:
            s->h263_flv = 1;
            break;

        default:
        {
            //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
            //av_log(avctx, AV_LOG_ERROR, "unsupported codec_id[%d] !\n",avctx->codec->id);
            av_log(avctx, AV_LOG_WARNING, "unsupported codec_id[%d] !\n", avctx->codec->id);
            return -1;
        }

    }

    s->codec_id = avctx->codec->id;
    avctx->hwaccel = ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);  //guoshan+00101841 20100510: (give a guess)找已有的codec,加速解码准备过程

    /* 2010/06/12 10:30:00 liuxw+00139685 */
    /* 将判断条件增加一个：如果是wmv3或vc1时，也是不用调用MPV_common_init */
    /* for h263, we allocate the images after having read the header */
    //	if (avctx->codec->id != CODEC_ID_H263 && avctx->codec->id != CODEC_ID_MPEG4) //guoshan+00101841 20100510: 没找到h.263||mpeg-4 codec就重新初始化
    /* 2010/09/14 15:30:00 liuxw+00139685 */
    /* 若是h263，则先调用MPV_common_init */
    //if (avctx->codec->id != CODEC_ID_H263 && avctx->codec->id != CODEC_ID_MPEG4 && avctx->codec->id != CODEC_ID_VC1 && avctx->codec->id != CODEC_ID_WMV3 && avctx->codec->id != CODEC_ID_FLV1)
    if (avctx->codec->id != CODEC_ID_MPEG4 && avctx->codec->id != CODEC_ID_VC1 && avctx->codec->id != CODEC_ID_WMV3)
        if (MPV_common_init(s) < 0)  /* liuxiaowei+00139685 20100612: 除了h63/mpeg4/vc1/wmv3(MPV_common_init在decode_frame中调用)的codec外，都在这里先调用MPV_common_init */
        { return -1; }

#if CONFIG_MSMPEG4_DECODER

    if (/*CONFIG_MSMPEG4_DECODER && */s->h263_msmpeg4)
    {
        iRet = ff_msmpeg4_decode_init(s);

        if (0 > iRet)
        {
            return -1;
        }
    }
    else
#endif
        h263_decode_init_vlc(s);

    /* 2010/09/14 15:30:00 liuxw+00139685 */
    /* 增加初始化函数：为每个picture分配yuv空间及相关的空间 */
    {
        unsigned int i;
        avctx->width  = avctx->usSourceWidth;
        avctx->height = avctx->usSourceHeight;

        for (i = 0; i < avctx->uiBufNum; i++)
        {
            if (alloc_picture(s, &(s->picture[i]), 0) < 0)
            {
                MPV_common_end(s);
                return -1;
            }
        }

    }

    return iRet;
}

av_cold int ff_h263_decode_end(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;

    MPV_common_end(s);
    return 0;
}

/**
 * returns the number of bytes consumed for building the current frame
 */
static int get_consumed_bytes(MpegEncContext *s, int buf_size)
{
    int pos = (get_bits_count(&s->gb) + 7) >> 3;

    if (s->divx_packed || s->avctx->hwaccel)
    {
        //we would have to scan through the whole buf to handle the weird reordering ...
        return buf_size;
    }
    else if (s->flags & CODEC_FLAG_TRUNCATED)
    {
        pos -= s->parse_context.last_index;

        if (pos < 0) { pos = 0; } // padding is not really read so this might be -1

        return pos;
    }
    else
    {
        if (pos == 0) { pos = 1; } //avoid infinite loops (i doubt that is needed but ...)

        if (pos + 10 > buf_size) { pos = buf_size; } // oops ;)

        return pos;
    }
}

static int decode_slice(MpegEncContext *s)
{
    const int part_mask = s->partitioned_frame ? (AC_END | AC_ERROR) : 0x7F;
    const int mb_size = 16 >> s->avctx->lowres;
    s->last_resync_gb = s->gb;
    s->first_slice_line = 1;

    s->resync_mb_x = s->mb_x;
    s->resync_mb_y = s->mb_y;

    /*guoshan+00101841 20100428 设置qscale,同时检测和修正qscale的合法范围*/
    /*所以在header解析过程中没重复检测*/
    ff_set_qscale(s, s->qscale);

    if (s->avctx->hwaccel)
    {
        const uint8_t *start = s->gb.buffer + get_bits_count(&s->gb) / 8;
        const uint8_t *end  = ff_h263_find_resync_marker(start + 1, s->gb.buffer_end);
        skip_bits_long(&s->gb, 8 * (end - start));
        return s->avctx->hwaccel->decode_slice(s->avctx, start, end - start);
    }

    if (s->partitioned_frame)
    {
        const int qscale = s->qscale;

        if (s->codec_id == CODEC_ID_MPEG4)
        {
            /*guoshan+00101841 20100512*/
            /*下列函数只是提取各个partition中的相关解码信息赋给结构体s的成员，具体的解码过程全在后面的decode_mb()过程中*/
            if (ff_mpeg4_decode_partitions(s) < 0)
            { return -1; }
        }

        /* restore variables which were modified */
        s->first_slice_line = 1;
        s->mb_x = s->resync_mb_x;
        s->mb_y = s->resync_mb_y;
        ff_set_qscale(s, qscale);
    }

    for (; s->mb_y < s->mb_height; s->mb_y++)
    {
        /* per-row end of slice checks */
        if (s->msmpeg4_version)
        {
            if (s->resync_mb_y + s->slice_height == s->mb_y)
            {
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x - 1, s->mb_y, AC_END | DC_END | MV_END);

                return 0;
            }
        }

        if (s->msmpeg4_version == 1)
        {
            s->last_dc[0] =
                s->last_dc[1] =
                    s->last_dc[2] = 128;
        }

        ff_init_block_index(s);

        for (; s->mb_x < s->mb_width; s->mb_x++)
        {
            int ret;

            ff_update_block_index(s);

            if (s->resync_mb_x == s->mb_x && s->resync_mb_y + 1 == s->mb_y)
            {
                s->first_slice_line = 0;
            }

            /* DCT & quantize */

            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
            //            s->mb_skipped = 0;

            ret = s->decode_mb(s, s->block);

            if (s->pict_type != FF_B_TYPE)
            { ff_h263_update_motion_val(s); }

            if (ret < 0)
            {
                const int xy = s->mb_x + s->mb_y * s->mb_stride;

                if (ret == SLICE_END)
                {
                    MPV_decode_mb(s, s->block);

                    if (s->loop_filter)
                        /*guoshan+00101841 20101011 增加h263_loop_filter时间统计*/
                        //						UINT64 ullTime = IMedia_OS_Milliseconds();
                    { ff_h263_loop_filter(s); }

                    //						s->avctx->loop_filter_time += IMedia_OS_Milliseconds() - ullTime;

                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END)&part_mask);

                    s->padding_bug_score--;

                    if (++s->mb_x >= s->mb_width)
                    {
                        s->mb_x = 0;
                        ff_draw_horiz_band(s, s->mb_y * mb_size, mb_size);
                        s->mb_y++;
                    }

                    return 0;
                }
                else if (ret == SLICE_NOEND)
                {
                    //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
                    //av_log(s->avctx, AV_LOG_ERROR, "Slice mismatch at MB: %d\n", xy);
                    av_log(s->avctx, AV_LOG_WARNING, "Slice mismatch at MB: %d\n", xy);
                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x + 1, s->mb_y, (AC_END | DC_END | MV_END)&part_mask);
                    return -1;
                }

                //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
                //av_log(s->avctx, AV_LOG_ERROR, "Error at MB: %d\n", xy);
                av_log(s->avctx, AV_LOG_WARNING, "Error at MB: %d\n", xy);
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR | DC_ERROR | MV_ERROR)&part_mask);

                return -1;
            }

            MPV_decode_mb(s, s->block);

            if (s->loop_filter)
                /*guoshan+00101841 20101011 增加h263_loop_filter时间统计*/
                //				UINT64 ullTime = IMedia_OS_Milliseconds();
            { ff_h263_loop_filter(s); }

            //				s->avctx->loop_filter_time += IMedia_OS_Milliseconds() - ullTime;
        }

        ff_draw_horiz_band(s, s->mb_y * mb_size, mb_size);

        s->mb_x = 0;
    }

    assert(s->mb_x == 0 && s->mb_y == s->mb_height);

    /* try to detect the padding bug */
    if (      s->codec_id == CODEC_ID_MPEG4
              &&   (s->workaround_bugs & FF_BUG_AUTODETECT)
              &&    s->gb.size_in_bits - get_bits_count(&s->gb) >= 0
              &&    s->gb.size_in_bits - get_bits_count(&s->gb) < 48
              //       &&   !s->resync_marker
              &&   !s->data_partitioning)
    {

        const int bits_count = get_bits_count(&s->gb);
        const int bits_left = s->gb.size_in_bits - bits_count;

        if (bits_left == 0)
        {
            s->padding_bug_score += 16;
        }
        else if (bits_left != 1)
        {
            int v = show_bits(&s->gb, 8);
            v |= 0x7F >> (7 - (bits_count & 7));

            if (v == 0x7F && bits_left <= 8)
            { s->padding_bug_score--; }
            else if (v == 0x7F && ((get_bits_count(&s->gb) + 8) & 8) && bits_left <= 16)
            { s->padding_bug_score += 4; }
            else
            { s->padding_bug_score++; }
        }
    }

    if (s->workaround_bugs & FF_BUG_AUTODETECT)
    {
        if (s->padding_bug_score > -2 && !s->data_partitioning /*&& (s->divx_version || !s->resync_marker)*/)
        { s->workaround_bugs |=  FF_BUG_NO_PADDING; }
        else
        { s->workaround_bugs &= ~FF_BUG_NO_PADDING; }
    }

    // handle formats which don't have unique end markers
    if (s->msmpeg4_version || (s->workaround_bugs & FF_BUG_NO_PADDING))
    {
        //FIXME perhaps solve this more cleanly
        int left = s->gb.size_in_bits - get_bits_count(&s->gb);
        int max_extra = 7;

        /* no markers in M$ crap */
        if (s->msmpeg4_version && s->pict_type == FF_I_TYPE)
        { max_extra += 17; }

        /* buggy padding but the frame should still end approximately at the bitstream end */
        if ((s->workaround_bugs & FF_BUG_NO_PADDING) && s->error_recognition >= 3)
        { max_extra += 48; }
        else if ((s->workaround_bugs & FF_BUG_NO_PADDING))
        { max_extra += 256 * 256 * 256 * 64; }

        if (left > max_extra)
        {
            //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
            //av_log(s->avctx, AV_LOG_ERROR, "discarding %d junk bits at end, next would be %X\n", left, show_bits(&s->gb, 24));
            av_log(s->avctx, AV_LOG_WARNING, "discarding %d junk bits at end, next would be %X\n", left, show_bits(&s->gb, 24));
        }
        else if (left < 0)
        {
            //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
            //av_log(s->avctx, AV_LOG_ERROR, "overreading %d bits\n", -left);
            av_log(s->avctx, AV_LOG_WARNING, "overreading %d bits\n", -left);
        }
        else
        { ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x - 1, s->mb_y, AC_END | DC_END | MV_END); }

        return 0;
    }

    //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
    //     av_log(s->avctx, AV_LOG_ERROR, "slice end not reached but screenspace end (%d left %06X, score= %d)\n",
    //             s->gb.size_in_bits - get_bits_count(&s->gb),
    //             show_bits(&s->gb, 24), s->padding_bug_score);

    av_log(s->avctx, AV_LOG_WARNING, "slice end not reached but screenspace end (%d left %06X, score= %d)\n",
           s->gb.size_in_bits - get_bits_count(&s->gb),
           show_bits(&s->gb, 24), s->padding_bug_score);

    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END | DC_END | MV_END)&part_mask);

    return -1;
}

int ff_h263_decode_frame(AVCodecContext *avctx,
                         void *data, int *data_size,
                         const uint8_t *buf, int buf_size)
{
    MpegEncContext *s = avctx->priv_data;
    int ret;
    AVFrame *pict = data;

#ifdef PRINT_FRAME_TIME
    uint64_t time = rdtsc();
#endif
#ifdef DEBUG
    av_log(avctx, AV_LOG_DEBUG, "*****frame %d size=%d\n", avctx->frame_number, buf_size);

    if (buf_size > 0)
    { av_log(avctx, AV_LOG_DEBUG, "bytes=%x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]); }

#endif
    s->flags = avctx->flags;
    s->flags2 = avctx->flags2;

    /* no supplementary picture */
    if (buf_size == 0)
    {
        /* special case for last picture */
        if (s->low_delay == 0 && s->next_picture_ptr)
        {
            *pict = *(AVFrame *)s->next_picture_ptr;
            s->next_picture_ptr = NULL;

            *data_size = sizeof(AVFrame);

            pict->ucLastFrame = 1;
        }

        return AVCODEC_RET_DECODE_LAST_FRAME;
    }

    if (s->flags & CODEC_FLAG_TRUNCATED)
    {
        int next;

        if (CONFIG_MPEG4_DECODER && s->codec_id == CODEC_ID_MPEG4)
        {
            next = ff_mpeg4_find_frame_end(&s->parse_context, buf, buf_size);
        }
        else if (CONFIG_H263_DECODER && s->codec_id == CODEC_ID_H263)
        {
            next = ff_h263_find_frame_end(&s->parse_context, buf, buf_size);
        }
        else
        {
            //guoshan+00101841 20100517修改AV_LOG_ERROR为AV_LOG_WARNING
            //av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
            av_log(s->avctx, AV_LOG_WARNING, "this codec does not support truncated bitstreams\n");
            return AVCODEC_RET_DECODE_ABORT;
        }

        if ( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
        { return buf_size; }
    }

    if (s->bitstream_buffer_size && (s->divx_packed || buf_size < 20))
    {
        //divx 5.01+/xvid frame reorder
        init_get_bits(&s->gb, s->bitstream_buffer, s->bitstream_buffer_size * 8);
    }
    else
    { init_get_bits(&s->gb, buf, buf_size * 8); }

    s->bitstream_buffer_size = 0;

    /* 2010/09/14 16:30:00 liuxw+00139685 */
    /* 结构调整，不调用以下代码 */
#if 0

    if (!s->context_initialized)
    {
        /*guoshan + 00101841 20100611*/
        /*ff_h263_decode_frame函数返回负数会导致接口层停止调用解码函数，解析错误后只用返回，重新调用下一帧*/
        if (MPV_common_init(s) < 0) //we need the idct permutation for reading a custom matrix
            // return -1;
        { return GOTO_NEXT_FRAME; }
    }

#endif
    /* We need to set current_picture_ptr before reading the header,
     * otherwise we cannot store anything in there */
    /* 2010/09/14 15:30:00 liuxw+00139685 */
    /* 由于结构调整，以下代码的实现将调整到MPV_Frame_start */
    //   if(s->current_picture_ptr==NULL || s->current_picture_ptr->data[0])
    //{
    //       int i= ff_find_unused_picture(s, 0);
    //       s->current_picture_ptr= &s->picture[i];
    //   }

    /* let's go :-) */

    /*guoshan + 00101841 20100415*/
    /*不支持wmv2,msmpet4,注释掉*/
    //     if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5)
    //     {
    //         ret= ff_wmv2_decode_picture_header(s);
    //     }
    //     else if (CONFIG_MSMPEG4_DECODER && s->msmpeg4_version)
    //     {
    //         ret = msmpeg4_decode_picture_header(s);
    //     }

    /*else */if (s->h263_pred)  //vicky 20100414: mpeg4, msmpegv1, msmpegv2
    {
        /*guoshan + 00101841 20100415*/
        /*这个变量被修改成数组*/
        int ii = 0;
        //  if(s->avctx->extradata_size && s->picture_number==0)

        for (ii = 0; ii < s->avctx->extradata_num; ii++)
        {
            if (s->avctx->extradata_size[ii] && s->picture_number == 0)
            {
                GetBitContext gb;
                // init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);
                init_get_bits(&gb, s->avctx->extradata[ii], s->avctx->extradata_size[ii] * 8);
                ret = ff_mpeg4_decode_picture_header(s, &gb);
            }
        }

        ret = ff_mpeg4_decode_picture_header(s, &s->gb);
    }
    //     else if (s->codec_id == CODEC_ID_H263I)
    //     {
    //         ret = intel_h263_decode_picture_header(s);
    //     }
    else if (s->h263_flv)
    {
        ret = flv_h263_decode_picture_header(s);
    }
    else
    {
        ret = h263_decode_picture_header(s);
    }

    if (ret == FRAME_SKIPPED)
    { return get_consumed_bytes(s, buf_size); }

    /* skip if the header was thrashed */
    if (ret < 0)
    {
        av_log(s->avctx, AV_LOG_WARNING, "header damaged(ret code:%d)\n", ret);
        //       return -1;
        /*guoshan + 00101841 20100610*/
        /*ff_h263_decode_frame函数返回负数会导致接口层停止调用解码函数，解析错误后只用返回，重新调用下一帧*/
        return ret;
    }

    avctx->has_b_frames = !s->low_delay;

    if (s->xvid_build == 0 && s->divx_version == 0 && s->lavc_build == 0)
    {
        if (s->stream_codec_tag == AV_RL32("XVID") || s->codec_tag == AV_RL32("XVID") ||
            s->codec_tag == AV_RL32("XVIX") || s->codec_tag == AV_RL32("RMP4"))
        { s->xvid_build = -1; }

#if 0

        if (s->codec_tag == AV_RL32("DIVX") && s->vo_type == 0 && s->vol_control_parameters == 1
            && s->padding_bug_score > 0 && s->low_delay) // XVID with modified fourcc
        { s->xvid_build = -1; }

#endif
    }

    if (s->xvid_build == 0 && s->divx_version == 0 && s->lavc_build == 0)
    {
        if (s->codec_tag == AV_RL32("DIVX") && s->vo_type == 0 && s->vol_control_parameters == 0)
        { s->divx_version = 400; } //divx 4
    }

    if (s->xvid_build && s->divx_version)
    {
        s->divx_version =
            s->divx_build = 0;
    }

    if (s->workaround_bugs & FF_BUG_AUTODETECT)
    {
        if (s->codec_tag == AV_RL32("XVIX"))
        { s->workaround_bugs |= FF_BUG_XVID_ILACE; }

        if (s->codec_tag == AV_RL32("UMP4"))
        {
            s->workaround_bugs |= FF_BUG_UMP4;
        }

        if (s->divx_version >= 500 && s->divx_build < 1814)
        {
            s->workaround_bugs |= FF_BUG_QPEL_CHROMA;
        }

        if (s->divx_version > 502 && s->divx_build < 1814)
        {
            s->workaround_bugs |= FF_BUG_QPEL_CHROMA2;
        }

        if (s->xvid_build && s->xvid_build <= 3)
        { s->padding_bug_score = 256 * 256 * 256 * 64; }

        if (s->xvid_build && s->xvid_build <= 1)
        { s->workaround_bugs |= FF_BUG_QPEL_CHROMA; }

        if (s->xvid_build && s->xvid_build <= 12)
        { s->workaround_bugs |= FF_BUG_EDGE; }

        if (s->xvid_build && s->xvid_build <= 32)
        { s->workaround_bugs |= FF_BUG_DC_CLIP; }

#define SET_QPEL_FUNC(postfix1, postfix2) \
    s->dsp.put_ ## postfix1 = ff_put_ ## postfix2;\
    s->dsp.put_no_rnd_ ## postfix1 = ff_put_no_rnd_ ## postfix2;\
    s->dsp.avg_ ## postfix1 = ff_avg_ ## postfix2;

        if (s->lavc_build && s->lavc_build < 4653)
        { s->workaround_bugs |= FF_BUG_STD_QPEL; }

        if (s->lavc_build && s->lavc_build < 4655)
        { s->workaround_bugs |= FF_BUG_DIRECT_BLOCKSIZE; }

        if (s->lavc_build && s->lavc_build < 4670)
        {
            s->workaround_bugs |= FF_BUG_EDGE;
        }

        if (s->lavc_build && s->lavc_build <= 4712)
        { s->workaround_bugs |= FF_BUG_DC_CLIP; }

        if (s->divx_version)
        { s->workaround_bugs |= FF_BUG_DIRECT_BLOCKSIZE; }

        if (s->divx_version == 501 && s->divx_build == 20020416)
        { s->padding_bug_score = 256 * 256 * 256 * 64; }

        if (s->divx_version && s->divx_version < 500)
        {
            s->workaround_bugs |= FF_BUG_EDGE;
        }

        if (s->divx_version)
        { s->workaround_bugs |= FF_BUG_HPEL_CHROMA; }

#if 0

        if (s->divx_version == 500)
        { s->padding_bug_score = 256 * 256 * 256 * 64; }

        /* very ugly XVID padding bug detection FIXME/XXX solve this differently
         * Let us hope this at least works.
         */
        if (   s->resync_marker == 0 && s->data_partitioning == 0 && s->divx_version == 0
               && s->codec_id == CODEC_ID_MPEG4 && s->vo_type == 0)
        { s->workaround_bugs |= FF_BUG_NO_PADDING; }

        if (s->lavc_build && s->lavc_build < 4609) //FIXME not sure about the version num but a 4609 file seems ok
        { s->workaround_bugs |= FF_BUG_NO_PADDING; }

#endif
    }

    /*内核态编译栈空间过大 x00141957 20101230*/
#if 0

    if (s->workaround_bugs & FF_BUG_STD_QPEL)
    {
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 5], qpel16_mc11_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 7], qpel16_mc31_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][ 9], qpel16_mc12_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][11], qpel16_mc32_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][13], qpel16_mc13_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[0][15], qpel16_mc33_old_c)

        SET_QPEL_FUNC(qpel_pixels_tab[1][ 5], qpel8_mc11_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][ 7], qpel8_mc31_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][ 9], qpel8_mc12_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][11], qpel8_mc32_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][13], qpel8_mc13_old_c)
        SET_QPEL_FUNC(qpel_pixels_tab[1][15], qpel8_mc33_old_c)
    }

#endif

    if (avctx->debug & FF_DEBUG_BUGS)
        av_log(s->avctx, AV_LOG_DEBUG, "bugs: %X lavc_build:%d xvid_build:%d divx_version:%d divx_build:%d %s\n",
               s->workaround_bugs, s->lavc_build, s->xvid_build, s->divx_version, s->divx_build,
               s->divx_packed ? "p" : "");

#if 0 // dump bits per frame / qp / complexity
    {
        static FILE *f = NULL;

        if (!f) { f = fopen("rate_qp_cplx.txt", "w"); }

        fprintf(f, "%d %d %f\n", buf_size, s->qscale, buf_size * (double)s->qscale);
    }
#endif

#if HAVE_MMX

    if (s->codec_id == CODEC_ID_MPEG4 && s->xvid_build && avctx->idct_algo == FF_IDCT_AUTO && (mm_flags & FF_MM_MMX))
    {
        avctx->idct_algo = FF_IDCT_XVIDMMX;
        avctx->coded_width = 0; // force reinit
        //        dsputil_init(&s->dsp, avctx);
        s->picture_number = 0;
    }

#endif

    /* After H263 & mpeg4 header decode we have the height, width,*/
    /* and other parameters. So then we could init the picture   */
    /* FIXME: By the way H263 decoder is evolving it should have */
    /* an H263EncContext                                         */
    /* 2010/09/14 15:30:00 liuxw+00139685 */
    /* 结构调整，不需要以下代码 */
    //if (   s->width  != avctx->coded_width || s->height != avctx->coded_height)
    //{
    //    /* H.263 could change picture size any time */
    //    ParseContext pc= s->parse_context; //FIXME move these demuxng hack to avformat
    //    s->parse_context.buffer=0;
    //    MPV_common_end(s);
    //    s->parse_context= pc;
    //}
    if (!s->context_initialized)
    {
        avcodec_set_dimensions(avctx, s->width, s->height);

        /*guoshan + 00101841 20100416*/
        /*新增函数，设置avcodec的chroma_format*/
        avcodec_set_chroma_format(avctx, s->chroma_format);

        /* 2010/09/14 15:30:00 liuxw+00139685 */
        /* 根据当前序列实际的宽高更新句柄s中所有和宽高相关的变量 */
        MPV_common_update(s);
        s->context_initialized = 1;
        //      goto retry;
    }

    if ((s->codec_id == CODEC_ID_H263 || s->codec_id == CODEC_ID_H263P || s->codec_id == CODEC_ID_H263I))
    { s->gob_index = ff_h263_get_gob_height(s); }

    // for hurry_up==5
    s->current_picture.pict_type = s->pict_type;
    s->current_picture.key_frame = s->pict_type == FF_I_TYPE;

    /*guoshan + 00101841 20100609增加对解码帧类型的统计*/
    switch (s->pict_type)
    {
        case FF_I_TYPE:
        {
            /*guoshan+00101841 2010609 增加帧类型统计*/
            s->avctx->uiDecIFrames++;
            break;
        }

        case FF_P_TYPE:
        case FF_S_TYPE:
        {
            /*guoshan+00101841 2010609 增加帧类型统计*/
            s->avctx->uiDecPFrames++;
            break;
        }

        case FF_B_TYPE:
        {
            /*guoshan+00101841 2010609 增加帧类型统计*/
            s->avctx->uiDecBFrames++;
            break;
        }

        //case FF_S_TYPE:
        //	{
        //		/*guoshan+00101841 2010609 增加S帧类型统计,归入P帧类型*/
        //		s->avctx->uiDecPFrames++;
        //		break;
        //	}
        default:
        {
            /*guoshan+00101841 20100423添加log和错误码*/
            av_log(s->avctx, AV_LOG_WARNING, "pic_type[%d] invalid\n", s->pict_type);
            IMEDIA_SET_ERR_PIC( s->avctx->iErrorCode, IMEDIA_ERR_PIC_FRAME_TYPE);
            s->avctx->iTotalError++;
            /*guoshan + 00101841 20100610*/
            /*ff_h263_decode_frame函数返回负数会导致接口层停止调用解码函数，解析错误后只用返回，重新调用下一帧*/
            return AVCODEC_RET_DECODE_FAILURE;
            //		return -1;
        }
    }

    /* skip B-frames if we don't have reference frames */
    if (s->last_picture_ptr == NULL && (s->pict_type == FF_B_TYPE || s->dropable))
    { return get_consumed_bytes(s, buf_size); }

    /* skip b frames if we are in a hurry */
    if (avctx->hurry_up && s->pict_type == FF_B_TYPE)
    { return get_consumed_bytes(s, buf_size); }

    if (   (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type == FF_B_TYPE)
           || (avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type != FF_I_TYPE)
           ||  avctx->skip_frame >= AVDISCARD_ALL)
    { return get_consumed_bytes(s, buf_size); }

    /* skip everything if we are in a hurry>=5 */
    if (avctx->hurry_up >= 5)
    { return get_consumed_bytes(s, buf_size); }

    if (s->next_p_frame_damaged)  //20100510 guoshan+00101841: s->next_p_frame_damaged这个参数没有在哪个地方被初始化过
    {
        if (s->pict_type == FF_B_TYPE)
        { return get_consumed_bytes(s, buf_size); }
        else
        { s->next_p_frame_damaged = 0; }
    }

    if ((s->avctx->flags2 & CODEC_FLAG2_FAST) && s->pict_type == FF_B_TYPE)
    {
        s->me.qpel_put = s->dsp.put_2tap_qpel_pixels_tab;
        s->me.qpel_avg = s->dsp.avg_2tap_qpel_pixels_tab;
    }
    else if ((!s->no_rounding) || s->pict_type == FF_B_TYPE)
    {
        s->me.qpel_put = s->dsp.put_qpel_pixels_tab;
        s->me.qpel_avg = s->dsp.avg_qpel_pixels_tab;
    }
    else
    {
        s->me.qpel_put = s->dsp.put_no_rnd_qpel_pixels_tab;
        s->me.qpel_avg = s->dsp.avg_qpel_pixels_tab;
    }

    //    if(MPV_frame_start(s, avctx) < 0)
    /*guoshan + 00101841 20100610*/
    /*ff_h263_decode_frame函数返回负数会导致接口层停止调用解码函数，解析错误后只用返回，重新调用下一帧*/
    //return -1;
    ret = MPV_frame_start(s, avctx);

    if (ret < 0)
    { return ret; }

    /* 2010/09/14 11:30:00 liuxw+00139685 */
    /* 保存当前帧的pts */
    s->current_picture_ptr->iPts = s->avctx->iPts;

    if (avctx->hwaccel)
    {
        if (avctx->hwaccel->start_frame(avctx, buf, buf_size) < 0)
        { return AVCODEC_RET_DECODE_FAILURE; }
    }

#ifdef DEBUG
    av_log(avctx, AV_LOG_DEBUG, "qscale=%d\n", s->qscale);
#endif

    ff_er_frame_start(s);

    //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture->mb_type
    //which is not available before MPV_frame_start()
    //     if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5)
    //     {
    //         ret = ff_wmv2_decode_secondary_picture_header(s);
    //         if(ret<0) return ret;
    //         if(ret==1) goto intrax8_decoded;
    //     }

    /* decode each macroblock */
    s->mb_x = 0;
    s->mb_y = 0;

    decode_slice(s);

    while (s->mb_y < s->mb_height)
    {
        if (s->msmpeg4_version)
        {
            if (s->slice_height == 0 || s->mb_x != 0 || (s->mb_y % s->slice_height) != 0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
            { break; }
        }
        else
        {
            if (ff_h263_resync(s) < 0) //guoshan+00101841 20100504 没找到正确的同步位置(按GOB同步)
            { break; }
        }

        if (s->msmpeg4_version < 4 && s->h263_pred)
        { ff_mpeg4_clean_buffers(s); }

        decode_slice(s);
    }

    //     if (s->h263_msmpeg4 && s->msmpeg4_version<4 && s->pict_type==FF_I_TYPE)
    //         if(!CONFIG_MSMPEG4_DECODER || msmpeg4_decode_ext_header(s, buf_size) < 0){
    //             s->error_status_table[s->mb_num-1]= AC_ERROR|DC_ERROR|MV_ERROR;
    //         }

    /* divx 5.01+ bistream reorder stuff */
    if (s->codec_id == CODEC_ID_MPEG4 && s->bitstream_buffer_size == 0 && s->divx_packed)
    {
        int current_pos = get_bits_count(&s->gb) >> 3;
        int startcode_found = 0;

        if (buf_size - current_pos > 5)
        {
            int i;

            for (i = current_pos; i < buf_size - 3; i++)
            {
                if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1 && buf[i + 3] == 0xB6)
                {
                    startcode_found = 1;
                    break;
                }
            }
        }

        if (s->gb.buffer == s->bitstream_buffer && buf_size > 20)
        {
            //xvid style
            startcode_found = 1;
            current_pos = 0;
        }

        if (startcode_found)
        {
            s->bitstream_buffer = av_fast_realloc(
                                      s->bitstream_buffer,
                                      &s->allocated_bitstream_buffer_size,
                                      buf_size - current_pos + FF_INPUT_BUFFER_PADDING_SIZE);
            memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
            s->bitstream_buffer_size = buf_size - current_pos;
        }
    }

    //intrax8_decoded:
    ff_er_frame_end(s);

    if (avctx->hwaccel)
    {
        if (avctx->hwaccel->end_frame(avctx) < 0)
        { return AVCODEC_RET_DECODE_FAILURE; }
    }

    MPV_frame_end(s); //guoshan+00101841 2010510:解码完成后，把current_pic_ptr传递给s->avctx->coded_frame
    /* 2010/09/13 19:30:00 liuxw+00139685 */
    /* 将当前帧存放YUV的第4个空间存放当前pic的指针 */
    //	s->current_picture_ptr->data[3] = s->current_picture_ptr;
    s->current_picture_ptr->isbusy = 1;

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

    /* Return the Picture timestamp as the frame number */
    /* we subtract 1 because it is added on utils.c     */
    avctx->frame_number = s->picture_number - 1;

#ifdef PRINT_FRAME_TIME
    av_log(avctx, AV_LOG_DEBUG, "%"PRId64"\n", rdtsc() - time);
#endif

    return get_consumed_bytes(s, buf_size);
}

#if 0
AVCodec mpeg4_decoder =
{
    "mpeg4",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MPEG4,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush = ff_mpeg_flush,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-4 part 2"),
    .pix_fmts = ff_hwaccel_pixfmt_list_420,
};

AVCodec h263_decoder =
{
    "h263",
    CODEC_TYPE_VIDEO,
    CODEC_ID_H263,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush = ff_mpeg_flush,
    .long_name = NULL_IF_CONFIG_SMALL("H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2"),
    .pix_fmts = ff_hwaccel_pixfmt_list_420,
};

AVCodec msmpeg4v1_decoder =
{
    "msmpeg4v1",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MSMPEG4V1,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 1"),
    .pix_fmts = ff_pixfmt_list_420,
};

AVCodec msmpeg4v2_decoder =
{
    "msmpeg4v2",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MSMPEG4V2,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 2"),
    .pix_fmts = ff_pixfmt_list_420,
};

AVCodec msmpeg4v3_decoder =
{
    "msmpeg4",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MSMPEG4V3,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 3"),
    .pix_fmts = ff_pixfmt_list_420,
};

AVCodec wmv1_decoder =
{
    "wmv1",
    CODEC_TYPE_VIDEO,
    CODEC_ID_WMV1,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Video 7"),
    .pix_fmts = ff_pixfmt_list_420,
};

AVCodec h263i_decoder =
{
    "h263i",
    CODEC_TYPE_VIDEO,
    CODEC_ID_H263I,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Intel H.263"),
    .pix_fmts = ff_pixfmt_list_420,
};

AVCodec flv_decoder =
{
    "flv",
    CODEC_TYPE_VIDEO,
    CODEC_ID_FLV1,
    sizeof(MpegEncContext),
    ff_h263_decode_init,
    NULL,
    ff_h263_decode_end,
    ff_h263_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Flash Video (FLV)"),
    .pix_fmts = ff_pixfmt_list_420,
};
#endif