/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : dmac_tid.h
  版 本 号   : 初稿
  作    者   : 
  生成日期   : 2012年11月15日
  最近修改   :
  功能描述   : dmac_tid.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2012年11月15日
    作    者   : 
    修改内容   : 创建文件

******************************************************************************/

#ifndef __DMAC_TID_H__
#define __DMAC_TID_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_TID_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/*****************************************************************************
  结构名  : dmac_tid_stru
  结构说明: tid缓存队列结构体定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  dmac_tid_tx_queue_init(dmac_tid_stru *past_tid_queue, mac_user_stru *pst_user);
#if 0
extern oal_void  dmac_tid_flush_retry_frame(mac_device_stru *pst_device, dmac_tid_stru *pst_tid);
#endif
extern oal_uint32  dmac_tid_tx_queue_enqueue_head(dmac_tid_stru *pst_tid_queue, oal_dlist_head_stru *pst_tx_dscr_list_hdr, oal_uint8 uc_mpdu_num);
extern oal_uint32  dmac_tid_clear(mac_user_stru *pst_mac_user, mac_device_stru *pst_mac_device);
extern oal_uint32  dmac_tid_get_normal_rate_stats(mac_user_stru *pst_mac_user, oal_uint8 uc_tid_id, dmac_tx_normal_rate_stats_stru **ppst_rate_stats_info);






#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_tid.h */
