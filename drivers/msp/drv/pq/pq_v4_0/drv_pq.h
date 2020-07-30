/******************************************************************************
*
* Copyright (C) 2014 Hisilicon Technologies Co., Ltd.  All rights reserved.
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon),
*  and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
*****************************************************************************

  File Name     : drv_pq.h
  Version       : Initial Draft
  Author        : p00203646
  Created       : 2014/04/01
  Description   :

******************************************************************************/

#ifndef __DRV_PQ_H__
#define __DRV_PQ_H__

#include "hi_osal.h"
#include "hi_debug.h"
#include "hi_type.h"

#include "hi_drv_pq.h"
#include "hi_drv_proc.h"
#include "hi_drv_mem.h"
#include "hi_drv_mmz.h"
#include "hi_drv_module.h"

#include "drv_pq_table.h"
#include "pq_hal_comm.h"

#ifdef PQ_ALG_ZME
#include "pq_mng_zme.h"
#endif
#ifdef PQ_ALG_CSC
#include "pq_mng_csc.h"
#endif
#ifdef PQ_ALG_DEI
#include "pq_mng_dei.h"
#endif
#ifdef PQ_ALG_DB
#include "pq_mng_db.h"
#endif
#ifdef PQ_ALG_DM
#include "pq_mng_dm.h"
#endif
#ifdef PQ_ALG_TNR
#include "pq_mng_tnr.h"
#endif
#ifdef PQ_ALG_SNR
#include "pq_mng_snr.h"
#endif
#ifdef PQ_ALG_DERING
#include "pq_mng_dering.h"
#endif
#ifdef PQ_ALG_DESHOOT
#include "pq_mng_deshoot.h"
#endif
#ifdef PQ_ALG_ARTDS
#include "pq_mng_artds.h"
#endif
#ifdef PQ_ALG_GFXCSC
#include "pq_mng_gfxcsc.h"
#endif
#ifdef PQ_ALG_GFXZME
#include "pq_mng_gfxzme.h"
#endif
#ifdef PQ_ALG_ACM
#include "pq_mng_acm.h"
#endif
#ifdef PQ_ALG_FMD
#include "pq_mng_ifmd.h"
#endif
#ifdef PQ_ALG_SHARPEN
#include "pq_mng_sharpen.h"
#endif
#ifdef PQ_ALG_ACM
#include "pq_mng_acm.h"
#endif
#ifdef PQ_ALG_DCI
#include "pq_mng_dci.h"
#endif
#ifdef PQ_ALG_HDR
#include "pq_mng_hdr.h"
#endif



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define PQ_NAME            "HI_PQ"


/*�û�PQ ���ݽṹ*/
typedef struct  hiDRV_PQ_PARAM_S
{
    PICTURE_SETTING_S stSDPictureSetting;  /*graph SD setting*/
    PICTURE_SETTING_S stHDPictureSetting;  /*graph HD setting*/
    PICTURE_SETTING_S stSDVideoSetting;    /*video SD setting*/
    PICTURE_SETTING_S stHDVideoSetting;    /*video HD setting*/
    HI_U32  u32Strength[HI_PQ_MODULE_BUTT];
    HI_BOOL bDemoOnOff[HI_PQ_MODULE_BUTT];
    HI_BOOL bModuleOnOff[HI_PQ_MODULE_BUTT];
    HI_PQ_COLOR_ENHANCE_S stColorEnhance;
    HI_PQ_DEMO_MODE_E enDemoMode;
} HI_DRV_PQ_PARAM_S;


/**
 \brief ��ʾPQ״̬��Ϣ
 \attention \n
��

 \param[in] *s;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_ProcRead(struct seq_file* s, HI_VOID* data);

HI_S32 DRV_PQ_ProcWrite(struct file* file, const char __user* buf, size_t count, loff_t* ppos);

/**
 \brief ��ȡ��������
 \attention \n
��

 \param[out] pu32Brightness ����ֵ,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetSDBrightness(HI_U32* pu32Brightness);

/**
 \brief ���ñ�������
 \attention \n
��

 \param[in] u32Brightness, ����ֵ,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetSDBrightness(HI_U32 u32Brightness);

/**
 \brief ��ȡ����Աȶ�
 \attention \n
��

 \param[out] pu32Contrast �Աȶ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetSDContrast(HI_U32* pu32Contrast);

/**
 \brief ���ñ���Աȶ�
 \attention \n
��

 \param[in] u32Contrast, �Աȶ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetSDContrast(HI_U32 u32Contrast);

/**
 \brief ��ȡ����ɫ��
 \attention \n
��

 \param[out] pu32Hue  ɫ��, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetSDHue(HI_U32* pu32Hue);

/**
 \brief ���ñ���ɫ��
 \attention \n
��

 \param[in] u32Hue   ɫ��, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetSDHue(HI_U32 u32Hue);

/**
 \brief ��ȡ���履�Ͷ�
 \attention \n
��

 \param[out] pu32Saturation  ���Ͷ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetSDSaturation(HI_U32* pu32Saturation);

/**
 \brief ���ñ��履�Ͷ�
 \attention \n
��

 \param[in] u32Saturation ���Ͷ�,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetSDSaturation(HI_U32 u32Saturation);

/**
 \brief ��ȡ��������
 \attention \n
��

 \param[out] pu32Brightness ����ֵ,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetHDBrightness(HI_U32* pu32Brightness);

/**
 \brief ���ø�������
 \attention \n
��

 \param[in] u32Brightness, ����ֵ,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetHDBrightness(HI_U32 u32Brightness);

/**
 \brief ��ȡ����Աȶ�
 \attention \n
��

 \param[out] pu32Contrast �Աȶ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetHDContrast(HI_U32* pu32Contrast);

/**
 \brief ���ø���Աȶ�
 \attention \n
��

 \param[in] u32Contrast, �Աȶ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_SetHDContrast(HI_U32 u32Contrast);

/**
 \brief ��ȡ����ɫ��
 \attention \n
��

 \param[out] pu32Hue  ɫ��, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetHDHue(HI_U32* pu32Hue);

/**
 \brief ���ø���ɫ��
 \attention \n
��

 \param[in] u32Hue   ɫ��, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetHDHue(HI_U32 u32Hue);

/**
 \brief ��ȡ���履�Ͷ�
 \attention \n
��

 \param[out] pu32Saturation  ���Ͷ�, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetHDSaturation(HI_U32* pu32Saturation);

/**
 \brief ���ø��履�Ͷ�
 \attention \n
��

 \param[in] u32Saturation ���Ͷ�,��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetHDSaturation(HI_U32 u32Saturation);

/**
 \brief ��ȡ������
 \attention \n
��

 \param[out] pu32Sharpness  ������, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetSharpness(HI_U32* pu32Sharpness);

/**
 \brief ����������
 \attention \n
��

 \param[in] u32Sharpness, ������, ��Ч��Χ: 0~255;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetSharpness(HI_U32 u32Sharpness);


/**
 \brief ��ȡ����ǿ��
 \attention \n
��

 \param[out] pu32NRLevel: ����ȼ�, ��Ч��Χ: 0~255


 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetTnrLevel(HI_U32* pu32NRLevel);

HI_S32 DRV_PQ_GetSnrLevel(HI_U32* pu32NRLevel);


/**
 \brief ���ý���ǿ��
 \attention \n
��

 \param[in] u32tNRLevel: ����ȼ�, ��Ч��Χ: 0~255

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetTnrLevel(HI_U32 u32TnrLevel);

HI_S32 DRV_PQ_SetSnrLevel(HI_U32 u32TnrLevel);


/**
 \brief ��ȡ��ɫ��ǿ
 \attention \n
��

 \param[out] pu32ColorGainLevel

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetColorEhance(HI_U32* pu32ColorGainLevel);

/**
 \brief ������ɫ��ǿ
 \attention \n
��

 \param[in] u32ColorGainLevel

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetColorEhance(HI_U32 u32ColorGainLevel);

/**
 \brief ��ȡ��ɫ��ǿ
 \attention \n
  ��

 \param[out] pu32FleshToneLevel

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetFleshToneLevel(HI_U32* pu32FleshToneLevel);

/**
 \brief ���÷�ɫ��ǿ
 \attention \n
  ��

 \param[in] enFleshToneLevel

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetFleshToneLevel(HI_PQ_FLESHTONE_E enFleshToneLevel);

/**
 \brief ����DCIǿ������ȼ�
 \attention \n
��

 \param[in] none;

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetDCILevel(HI_U32 u32DCILevel);

HI_S32 DRV_PQ_GetDCILevel(HI_U32* pu32DCILevel);



/**
 \brief ��������ģʽ����
 \attention \n
��

 \param[in] enModule

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetDemoMode(HI_PQ_MODULE_E enModule, HI_BOOL bOnOff);

/**
 \brief ��ȡPQģ�鿪��״̬
 \attention \n
  ��

 \param[in] enFlags
 \param[out] *pu32OnOff

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetPQModule(HI_PQ_MODULE_E enModule, HI_U32* pu32OnOff);

/**
 \brief ����PQģ�鿪��
 \attention \n
  ��

 \param[in] enModule
 \param[in] u32OnOff

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_SetPQModule(HI_PQ_MODULE_E enModule, HI_U32 u32OnOff);

/**
 \brief ��ȡPQBin �������ַ
 \attention \n
��

 \param[in] pu32Addr:

 \retval ::HI_SUCCESS

 */

HI_S32 DRV_PQ_GetBinPhyAddr(HI_U32* pu32Addr);


/**
 \brief ����ɫ��������
 \attention \n
  ��

 \param[in] enSixColorType;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_SetSixBaseColorLevel(HI_PQ_SIX_BASE_S* pstSixBaseColorOffset);


HI_S32 DRV_PQ_GetSixBaseColorLevel(HI_PQ_SIX_BASE_S* pstSixBaseColorOffset);

/**
 \brief ��ɫ��ǿģʽ����
 \attention \n
  ��

 \param[in] enColorSpecMode 0-RECOMMEND;1-BLUE;2-GREEN;3-BG;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_SetColorEnhanceMode(HI_PQ_COLOR_SPEC_MODE_E enColorSpecMode);

HI_S32 DRV_PQ_GetColorEnhanceMode(HI_PQ_COLOR_SPEC_MODE_E* penColorSpecMode);


/**
 \brief set demo display mode
 \attention \n
  ��

 \param[in] enDemoMode ;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_SetDemoDispMode(PQ_REG_TYPE_E enFlag, HI_PQ_DEMO_MODE_E enDemoMode);

/**
 \brief get demo display mode
 \attention \n
  ��

 \param[in] penDemoMode ;

 \retval ::HI_SUCCESS

 */
HI_S32 DRV_PQ_GetDemoDispMode(HI_PQ_DEMO_MODE_E* penDemoMode);

HI_S32 DRV_PQ_SetDefaultParam(HI_BOOL bDefault);

HI_S32 DRV_PQ_SetReg(HI_PQ_REGISTER_S* pstAttr);

HI_S32 DRV_PQ_GetReg(HI_PQ_REGISTER_S* pstAttr);

HI_S32 DRV_PQ_SetDemoCoordinate(PQ_REG_TYPE_E enType, HI_U32 u32X);

HI_S32 DRV_PQ_GetDemoCoordinate(HI_U32* pu32X);

HI_S32 DRV_PQ_GetDbLevel(HI_U32* pu32DbLevel);

HI_S32 DRV_PQ_SetDbLevel(HI_U32 u32DbLevel);

HI_S32 DRV_PQ_GetDmLevel(HI_U32* pu32DmLevel);

HI_S32 DRV_PQ_SetDmLevel(HI_U32 u32DmLevel);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __DRV_PQ_H__ */
