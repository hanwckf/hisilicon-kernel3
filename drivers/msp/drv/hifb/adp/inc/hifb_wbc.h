/******************************************************************************
*
* Copyright (C) 2015 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
* and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
******************************************************************************
File Name           : hifb_wbc.h
Version             : Initial Draft
Author              : 
Created             : 2015/06/15
Description         : ��д����
Function List       : 
History             :
Date                       Author                   Modification
2015/06/15                y00181162                Created file        
******************************************************************************/

#ifndef __HIFB_WBC_H__
#define __HIFB_WBC_H__


/*********************************add include here******************************/
#include "optm_hal.h"
#include "optm_hifb.h"
#include "optm_define.h"
#include "optm_alg_csc.h"
#include "optm_alg_gzme.h"
#include "optm_alg_gsharp.h"
#include "hi_drv_disp.h"
#include "hi_gfx_comm_k.h"

/*****************************************************************************/


#ifdef __cplusplus
#if __cplusplus
   extern "C"
{
#endif
#endif /* __cplusplus */



/***************************** Macro Definition ******************************/
/**
 **ʹ�õĻ�дbuffer����LOGOֻ��Ҫһ��Ϳ����ˣ���Ϊֻ��дһ��
 **/
#ifndef HI_BUILD_IN_BOOT
	#define OPTM_WBCBUFFER_NUM               2
#else
	#define OPTM_WBCBUFFER_NUM               1
#endif
	
/*************************** Structure Definition ****************************/
typedef enum tagOPTM_WBC_MODE_E{
    OPTM_WBC_MODE_MONO      = 0x0,
    OPTM_WBC_MODE_LFET_EYE  = 0x2,
    OPTM_WBC_MODE_RIGHT_EYE = 0x3,
    OPTM_WBC_MODE_BUTT,
}OPTM_WBC_MODE_E;

typedef struct tagOPTM_GFX_WBC_S{
    HI_BOOL                bOpened;
	HI_BOOL				   bWorking;
    OPTM_VDP_LAYER_WBC_E   enWbcHalId;
    /* setting */
    HI_S32                 s32BufferWidth;
    HI_S32                 s32BufferHeight;
    HI_U32                 u32BufferStride;
    HI_U32                 u32BufIndex;
    HI_S32                 s32WbcCnt;
    HI_U32                 u32WBCBuffer[OPTM_WBCBUFFER_NUM];
    HI_U32                 u32WriteBufAddr;
    HI_U32                 u32ReadBufAddr;
    GFX_MMZ_BUFFER_S       stFrameBuffer;
    HI_U32                 u32DataPoint;  /* 0, feeder; others, reserve */
    HIFB_COLOR_FMT_E       enDataFmt;
    HIFB_RECT              stInRect;
    HI_BOOL                bInProgressive;
    HIFB_RECT              stOutRect;
    HI_BOOL                bOutProgressive;
    HI_U32                 u32BtmOffset;
    HI_BOOL                bHdDispProgressive;
    OPTM_VDP_DITHER_E      enDitherMode;
    OPTM_VDP_WBC_OFMT_E    stWBCFmt;
    OPTM_VDP_DATA_RMODE_E  enReadMode;
    OPTM_VDP_DATA_RMODE_E  enOutMode;
    OPTM_WBC_MODE_E        enWbcMode;
    OPTM_VDP_INTMSK_E      enWbcInt;
}OPTM_GFX_WBC_S;

/********************** Global Variable declaration **************************/



/******************************* API declaration *****************************/

/***************************************************************************************
* func          : HIFB_WBC2_SetRegUp
* description   : CNcomment:���»�д�Ĵ��� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_SetRegUp(OPTM_VDP_LAYER_WBC_E enWbcID);


/***************************************************************************************
* func          : HIFB_WBC2_CloseSlvLayer
* description   : CNcomment: ͬԴ��д������¹رձ���ͼ�� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_CloseSlvLayer(HIFB_LAYER_ID_E enLayerId);

/***************************************************************************************
* func          : HIFB_WBC2_OpenSlvLayer
* description   : CNcomment: ͬԴ��д������´򿪻�д����ͼ�㣬��������ر�����
                             ��ز��� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_OpenSlvLayer(HIFB_LAYER_ID_E enLayerId);


#ifndef HI_BUILD_IN_BOOT
/***************************************************************************************
* func          : HIFB_WBC2_WorkQueue
* description   : CNcomment: ͬԴ��д�������� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_WorkQueue(struct work_struct *data);
#endif


/***************************************************************************************
* func          : HIFB_WBC2_SetTcFlag
* description   : CNcomment: ����TC��д��� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_SetTcFlag(HI_BOOL bFlag);

/***************************************************************************************
* func          : HIFB_WBC2_SetCropReso
* description   : CNcomment: ����WBC_GP0�ü��ֱ��ʣ�Ҳ���������ʼ�ͽ������� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_SetCropReso(OPTM_VDP_DISP_RECT_S stInputRect);

/***************************************************************************************
* func          : HIFB_WBC2_SetPreZmeEn
* description   : CNcomment: ����ZMEʹ�� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_SetPreZmeEn(OPTM_ALG_GZME_DRV_PARA_S *pstZmeDrvPara);


/***************************************************************************************
* func          : HIFB_WBC2_Init
* description   : wbc initial
                  CNcomment: ��д��ʼ�� CNend\n
* param[in]     : 
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_Init(HI_VOID);

/***************************************************************************************
* func          : HIFB_WBC2_Dinit
* description   : wbc initial
                  CNcomment: ��дȥ��ʼ�� CNend\n
* param[in]     : 
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_Dinit(HI_VOID);

/***************************************************************************************
* func          : HIFB_WBC2_SetEnable
* description   : CNcomment: ���û�дʹ��״̬ CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_SetEnable(HI_BOOL bEnable);

/***************************************************************************************
* func          : HIFB_WBC2_CfgSlvLayer
* description   : CNcomment: ���ñ���ͼ�� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_CfgSlvLayer(HIFB_LAYER_ID_E enLayerId,HI_RECT_S *pstRect);

#ifndef HI_BUILD_IN_BOOT
/***************************************************************************************
* func          : HIFB_WBC2_FrameEndProcess
* description   : CNcomment: ֡��ɻ�д���� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_FrameEndProcess(OPTM_GFX_GP_E enGpId);

/***************************************************************************************
* func          : HIFB_WBC2_SlvLayerProcess
* description   : CNcomment: ��д����㴦�� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_SlvLayerProcess(OPTM_GFX_GP_E enGpId);

/***************************************************************************************
* func          : HIFB_WBC2_GetSlvLayerInfo
* description   : CNcomment: ��ȡ�����дͼ����Ϣ CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 HIFB_WBC2_GetSlvLayerInfo(HIFB_SLVLAYER_DATA_S *pstLayerInfo);

#else
/***************************************************************************************
* func          : HIFB_WBC2_Isr
* description   : CNcomment: ��д���� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_Isr(HI_VOID* u32Param0, HI_VOID* u32Param1);
#endif

/***************************************************************************************
* func          : HIFB_WBC2_Recovery
* description   : CNcomment: ��������WBC�ͻ�дͼ������ CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_Recovery(OPTM_GFX_GP_E enGPId);

/***************************************************************************************
* func          : HIFB_WBC2_Reset
* description   : CNcomment: ��д״̬��λ CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID HIFB_WBC2_Reset(HI_BOOL bWbcRegUp);


#ifdef __cplusplus

#if __cplusplus

}
#endif
#endif /* __cplusplus */

#endif /* __HIFB_WBC_H__ */
