/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : hmac_sms4.h
  版 本 号   : 初稿
  作    者   : 
  生成日期   : 2015年5月20日
  最近修改   :
  功能描述   : hmac_sms4.c的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2015年5月20日
    作    者   : 
    修改内容   : 创建文件

******************************************************************************/


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
//#include "mac_resource.h"
//#include "hmac_vap.h"
//#include "hmac_user.h"
//#include "mac_11i.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WAPI_SMS4_H
#ifdef _PRE_WLAN_FEATURE_WAPI
/*****************************************************************************/
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define Rotl(_x, _y) (((_x) << (_y)) | ((_x) >> (32 - (_y)))) /* 循环左移 */
#define getbyte(_x,_y) (((unsigned char *)&(_x))[_y]) /* 以y为下标，获取x对应的字节值 */

#define ByteSub(_S, _A) ((_S)[((oal_uint32)(_A)) >> 24 & 0xFF] << 24 ^ \
                     (_S)[((oal_uint32)(_A)) >> 16 & 0xFF] << 16 ^ \
                     (_S)[((oal_uint32)(_A)) >>  8 & 0xFF] <<  8 ^ \
                     (_S)[((oal_uint32)(_A)) & 0xFF])

#define L1(_B) ((_B) ^ Rotl(_B, 2) ^ Rotl(_B, 10) ^ Rotl(_B, 18) ^ Rotl(_B, 24))

#define L2(_B) ((_B) ^ Rotl(_B, 13) ^ Rotl(_B, 23))

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
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/




/*****************************************************************************
  10 函数声明
*****************************************************************************/


extern    oal_void hmac_sms4_crypt(oal_uint8 *puc_Input, oal_uint8 *puc_Output, oal_uint32 *pul_rk);
extern    oal_void hmac_sms4_keyext(oal_uint8 *puc_key, oal_uint32 *pul_rk);

#endif



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif



