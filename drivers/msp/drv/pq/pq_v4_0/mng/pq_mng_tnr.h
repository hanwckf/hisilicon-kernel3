/******************************************************************************
*
* Copyright (C) 2015 Hisilicon Technologies Co., Ltd.  All rights reserved.
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon),
*  and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
*****************************************************************************

  File Name     : pq_mng_tnr.c
  Version       : Initial Draft
  Author        : p00203646
  Created       : 2015/1/9
  Description   :

******************************************************************************/

#ifndef __PQ_MNG_TNR_H__
#define __PQ_MNG_TNR_H__

#include "drv_pq_comm.h"
#include "pq_hal_tnr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */




/**
  \brief ����TNR��PixMean-Ratio For YMotion Detection����
  \attention \n
 ��

  \param[in] pstTNR_YMotionPixmeanRatio

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrYMotionPixMeanRatio(TNR_MAPPING_S* pstTNR_YMotionPixmeanRatio);


/**
  \brief ��ȡTNR��PixMean-Ratio For YMotion Detection����
  \attention \n
 ��

  \param[in] pstTNR_YMotionPixmeanRatio

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrYMotionPixMeanRatio(TNR_MAPPING_S* pstTNR_YMotionPixmeanRatio);


/**
  \brief ����TNR��PixMean-Ratio For CMotion Detection����
  \attention \n
 ��

  \param[in] pstTNR_CMotionPixmeanRatio

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrCMotionPixMeanRatio(TNR_MAPPING_S* pstTNR_CMotionPixmeanRatio);


/**
  \brief ��ȡTNR��PixMean-Ratio For CMotion Detection����
  \attention \n
 ��

  \param[in] pstTNR_CMotionPixmeanRatio

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrCMotionPixMeanRatio(TNR_MAPPING_S* pstTNR_CMotionPixmeanRatio);


/**
  \brief ����TNR��YMotionMapping����
  \attention \n
 ��

  \param[in] pstTNR_YMotionMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrYMotionMapping(TNR_MAPPING_S* pstTNR_YMotionMapping);


/**
  \brief ��ȡTNR��YMotionMapping����
  \attention \n
 ��

  \param[in] pstTNR_YMotionMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrYMotionMapping(TNR_MAPPING_S* pstTNR_YMotionMapping);


/**
  \brief ����TNR��CMotonMapping ����
  \attention \n
 ��

  \param[in] pstTNR_CMotionMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrCMotionMapping(TNR_MAPPING_S* pstTNR_CMotionMapping);


/**
  \brief ��ȡTNR��CMotonMapping����
  \attention \n
 ��

  \param[in] pstTNR_CMotionMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrCMotionMapping(TNR_MAPPING_S* pstTNR_CMotionMapping);


/**
  \brief ����TNR��Final Y Motion Mapping����
  \attention \n
 ��

  \param[in] pstTNR_FnlMotionYMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrFnlMotionYMapping(TNR_MAPPING_S* pstTNR_FnlMotionYMapping);


/**
  \brief ��ȡTNR��Final Y Motion Mapping����
  \attention \n
 ��

  \param[in] pstTNR_FnlMotionYMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrFnlMotionYMapping(TNR_MAPPING_S* pstTNR_FnlMotionYMapping);


/**
  \brief ����TNR��Final C Motion Mapping����
  \attention \n
 ��

  \param[in] pstTNR_FnlMotionCMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_SetTnrFnlMotionCMapping(TNR_MAPPING_S* pstTNR_FnlMotionCMapping);



/**
  \brief ��ȡTNR��Final C Motion Mapping����
  \attention \n
 ��

  \param[in] pstTNR_FnlMotionCMapping

  \retval ::HI_SUCCESS

  */

HI_S32 PQ_MNG_GetTnrFnlMotionCMapping(TNR_MAPPING_S* pstTNR_FnlMotionCMapping);

/**
  \brief ���� TNR��Y Motion Mapping����
  \attention \n
 ��

  \param[in] pstTNR_YfmotionMapping

  \retval ::HI_SUCCESS

  */
HI_S32 PQ_MNG_SetTnrFmotionMapping(TNR_FMOTION_MAPPING_S* pstTNR_YfmotionMapping);

/**
  \brief ��ȡTNR��Y Motion Mapping  ����
  \attention \n
 ��

  \param[in] pstTNR_YfmotionMapping

  \retval ::HI_SUCCESS

  */
HI_S32 PQ_MNG_GetTnrFmotionMapping(TNR_FMOTION_MAPPING_S* pstTNR_YfmotionMapping);

HI_S32 PQ_MNG_RegisterTNR(PQ_REG_TYPE_E  enType);

HI_S32 PQ_MNG_UnRegisterTNR(HI_VOID);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /*End of #ifndef __MNG_PQ_NR_H__ */



