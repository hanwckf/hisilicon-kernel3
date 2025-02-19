/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : oal_thread.h
  版 本 号   : 初稿
  作    者   : 
  生成日期   : 2014年1月29日
  最近修改   :
  功能描述   : oal_thread.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2014年10月13日
    作    者   : 
    修改内容   : 创建文件

******************************************************************************/

#ifndef __OAL_WINDOWS_THREAD_H__
#define __OAL_WINDOWS_THREAD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_schedule.h"

/*****************************************************************************
  2 STRUCT定义
*****************************************************************************/
typedef struct task_struct          oal_kthread_stru;
typedef struct _kthread_param_{
    oal_uint32         ul_stacksize;
    oal_int32           l_prio;
    oal_int32           l_policy;
    oal_int32           l_cpuid;
    oal_int32           l_nice;
}oal_kthread_param_stru;

#define OAL_SCHED_FIFO      (1)
#define OAL_SCHED_RR        (2)

#define NOT_BIND_CPU        (-1)

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
  7 宏定义
*****************************************************************************/

#define oal_kthread_should_stop()     (0)
#define oal_schedule
#define oal_wake_up_process(task)
#define oal_set_current_state
#define oal_cond_resched
#define oal_sched_setscheduler(task, policy, param) (0)

#define OAL_CURRENT current
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
typedef int (*oal_thread_func)(void *);

/*****************************************************************************
  10 函数声明
*****************************************************************************/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif
