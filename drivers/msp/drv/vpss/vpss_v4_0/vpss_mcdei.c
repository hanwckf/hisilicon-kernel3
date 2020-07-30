/*-----------------------------------------------------------------------*/
/*!!Warning: Huawei key information asset. No spread without permission. */
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCEm2UPcyllv4D4NOje6cFLSYglw6LvPA978sGAr3yTchgOI0M46H
HZIZCDLcNqR1rbxHHGWmLNp+CRsGfVaxSWRuFrWwLOeVNv5lhxw6wJdrwY1AqiLq0jt5Zz5x
SQO5jJALxgPw6bIHWU1oHoPeiPDMdGjniSvJTRuMLSumOHn5uyMGcKp5DFmoQ3k8ByIPuAFF
uaN5GIBtdIbLR5DRwMZqQxRliINgw5d+riZXMEHv2ZV/T+QvrHig6PKrjQAgtQ==#*/
/*--!!Warning: Deleting or modifying the preceding information is prohibited.--*/

#include "vpss_mcdei.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

HI_U32 VPSS_INST_McDeiGetMedsWidth(HI_DRV_VIDEO_FRAME_S *pstSrcImage)
{
    HI_U32 u32StreamW;    

    u32StreamW = pstSrcImage->u32Width;

    return ((u32StreamW > 960) ? (u32StreamW / 2) : u32StreamW);

}

HI_BOOL VPSS_INST_McDeiGetMedsEn(HI_DRV_VIDEO_FRAME_S *pstSrcImage)
{
    HI_U32 u32StreamW;    

    u32StreamW = pstSrcImage->u32Width;

    return ((u32StreamW > 960) ? HI_TRUE : HI_FALSE);

}

HI_BOOL VPSS_INST_McDeiSupport(HI_DRV_VIDEO_FRAME_S *pstSrcImage)
{
        
    if(pstSrcImage->bProgressive == HI_TRUE)
    {
        return HI_FALSE;
    }
	
    return HI_TRUE;

}

HI_VOID VPSS_INST_SetMcDeiFrameInfo(HI_DRV_VIDEO_FRAME_S *pstFrame,
    VPSS_HAL_FRAME_S *pstHalFrm, HI_DRV_BUF_ADDR_E enBufLR)
{
    HI_DRV_VIDEO_PRIVATE_S *pstFrmPriv;
    HI_VDEC_PRIV_FRAMEINFO_S *pstVdecPriv;
    
    if(pstFrame == HI_NULL || pstHalFrm == HI_NULL)
    {
        VPSS_FATAL("pstFrame(%p), pstHalFrm(%p) NULL\n", pstFrame, pstHalFrm);
        return;
    }

    pstHalFrm->eFrmType = pstFrame->eFrmType;
    pstHalFrm->u32Width = pstFrame->u32Width;
    pstHalFrm->u32Height = pstFrame->u32Height;
    
    pstHalFrm->enFormat = pstFrame->ePixFormat;
    pstHalFrm->enFieldMode = pstFrame->enFieldMode;
    pstHalFrm->bProgressive = pstFrame->bProgressive;
    memcpy(&pstHalFrm->stAddr, &pstFrame->stBufAddr[enBufLR], sizeof(HI_DRV_VID_FRAME_ADDR_S));
    pstHalFrm->bCompressd = pstFrame->bCompressd;
    pstHalFrm->enBitWidth = pstFrame->enBitWidth;

    pstFrmPriv = (HI_DRV_VIDEO_PRIVATE_S *)&(pstFrame->u32Priv[0]);
    pstVdecPriv = (HI_VDEC_PRIV_FRAMEINFO_S *)&(pstFrmPriv->u32Reserve[0]);
    
    pstHalFrm->bTopFirst = pstFrame->bTopFieldFirst;
	pstHalFrm->bSecure = pstFrame->bSecure;
	if ( HI_DRV_PIXEL_BITWIDTH_10BIT == pstFrame->enBitWidth)  //@f00241306 for dither
	{
#if defined(CHIP_TYPE_hi3798cv200_a) || defined(CHIP_TYPE_hi3798cv200) 
		memcpy(&pstHalFrm->stAddr_LB, &pstFrame->stBufAddr_LB[enBufLR], sizeof(HI_DRV_VID_FRAME_ADDR_S));
#endif
	}   
    
}


HI_S32 VPSS_INST_McDeiInit(VPSS_MCDEI_INFO_S *pstMcdeiInfo, HI_DRV_VIDEO_FRAME_S *pstSrcImage)
{     
    HI_U32 u32StreamW;
    HI_U32 u32StreamH; 
    HI_U32 u32InitResult;    
    VPSS_RGME_ATTR_S stWbcRgmeAttr; 
    VPSS_BLEND_ATTR_S stWbcBlendAttr;
    VPSS_MC_ATTR_S stSttAttr;
    VPSS_RGME_S          *pstRgmeWbcInfo;
    VPSS_BLEND_S         *pstBlendWbcInfo;
    VPSS_ST_RGME_S       *pstRgmeInfo;   
    VPSS_ST_BLKMV_S      *pstBlkmvInfo; 
    VPSS_ST_PRJH_S       *pstPrjhInfo;  
    VPSS_ST_PRJV_S       *pstPrjvInfo;
    
    if ((HI_NULL == pstMcdeiInfo) || (HI_NULL == pstSrcImage))
    {
        VPSS_ERROR("para pstMcdeiInfo or pstSrcImage is HI_NULL\n");
        return HI_FAILURE;
    }
    
    pstMcdeiInfo->bMcEnable = VPSS_INST_McDeiSupport(pstSrcImage);
    if (HI_TRUE != pstMcdeiInfo->bMcEnable)
    {
        return HI_SUCCESS;
    }
    
    pstRgmeWbcInfo = &(pstMcdeiInfo->stRgmeWbcInfo);
    pstBlendWbcInfo = &(pstMcdeiInfo->stBlendWbcInfo);
    pstRgmeInfo = &(pstMcdeiInfo->stRgmeInfo);
    pstBlkmvInfo = &(pstMcdeiInfo->stBlkmvInfo);
    pstPrjhInfo = &(pstMcdeiInfo->stPrjhInfo);
    pstPrjvInfo = &(pstMcdeiInfo->stPrjvInfo);
    
    u32StreamW = pstSrcImage->u32Width;
    u32StreamH = pstSrcImage->u32Height;
    
    stWbcBlendAttr.u32Width = u32StreamW;
    stWbcBlendAttr.u32Height = u32StreamH;
    stWbcBlendAttr.ePixFormat = pstSrcImage->ePixFormat;
    stWbcBlendAttr.enBitWidth = pstSrcImage->enBitWidth;
    stWbcBlendAttr.enBufMode = VPSS_BLEND_BUFMODE_NORMAL;
    stWbcBlendAttr.enMode = VPSS_BLEND_MODE_4FIELD;

    stWbcRgmeAttr.u32Width = VPSS_INST_McDeiGetMedsWidth(pstSrcImage);
    stWbcRgmeAttr.u32Height = u32StreamH;
    stWbcRgmeAttr.enBitWidth = pstSrcImage->enBitWidth;
    stWbcRgmeAttr.ePixFormat = pstSrcImage->ePixFormat;
    stWbcRgmeAttr.enMode = VPSS_RGME_MODE_4FIELD;
    stWbcRgmeAttr.enRefMode = VPSS_RGME_PREIMG_MODE_INIT;

    stSttAttr.u32Width = VPSS_INST_McDeiGetMedsWidth(pstSrcImage);
    stSttAttr.u32Height = u32StreamH;

    u32InitResult = VPSS_BLEND_Init(pstBlendWbcInfo,&stWbcBlendAttr);
    u32InitResult |= VPSS_RGME_Init(pstRgmeWbcInfo,&stWbcRgmeAttr);
    u32InitResult |= VPSS_ST_RGME_Init(pstRgmeInfo,&stSttAttr);
    u32InitResult |= VPSS_ST_BLKMV_Init(pstBlkmvInfo,&stSttAttr);
    u32InitResult |= VPSS_ST_PRJH_Init(pstPrjhInfo,&stSttAttr);
    u32InitResult |= VPSS_ST_PRJV_Init(pstPrjvInfo,&stSttAttr);    

    return HI_SUCCESS;
}

HI_S32 VPSS_INST_McDeiDeInit(VPSS_MCDEI_INFO_S *pstMcdeiInfo)
{  
    VPSS_RGME_S          *pstRgmeWbcInfo;
    VPSS_BLEND_S         *pstBlendWbcInfo;
    VPSS_ST_RGME_S       *pstRgmeInfo;   
    VPSS_ST_BLKMV_S      *pstBlkmvInfo; 
    VPSS_ST_PRJH_S       *pstPrjhInfo;  
    VPSS_ST_PRJV_S       *pstPrjvInfo;

    if (HI_NULL == pstMcdeiInfo)
    {
        VPSS_ERROR("para pstMcdeiInfo is HI_NULL\n");
        return HI_FAILURE;
    }
    
    pstRgmeWbcInfo = &(pstMcdeiInfo->stRgmeWbcInfo);
    pstBlendWbcInfo = &(pstMcdeiInfo->stBlendWbcInfo);
    pstRgmeInfo = &(pstMcdeiInfo->stRgmeInfo);
    pstBlkmvInfo = &(pstMcdeiInfo->stBlkmvInfo);
    pstPrjhInfo = &(pstMcdeiInfo->stPrjhInfo);
    pstPrjvInfo = &(pstMcdeiInfo->stPrjvInfo);

    /*����ط�����ֱ���˳�����ֹ�ڴ�й¶*/
    if (HI_TRUE != pstMcdeiInfo->bMcEnable)
    {
        //return HI_SUCCESS;
    }

    VPSS_BLEND_DeInit(pstBlendWbcInfo);
    VPSS_RGME_DeInit(pstRgmeWbcInfo);
    VPSS_ST_RGME_DeInit(pstRgmeInfo);
    VPSS_ST_BLKMV_DeInit(pstBlkmvInfo);
    VPSS_ST_PRJH_DeInit(pstPrjhInfo);
    VPSS_ST_PRJV_DeInit(pstPrjvInfo); 
    return HI_SUCCESS;
}
HI_S32 VPSS_INST_McDeiComplete(VPSS_MCDEI_INFO_S *pstMcdeiInfo)
{    
    VPSS_RGME_S          *pstRgmeWbcInfo;
    VPSS_BLEND_S         *pstBlendWbcInfo;
    VPSS_ST_RGME_S       *pstRgmeInfo;   
    VPSS_ST_BLKMV_S      *pstBlkmvInfo; 
    VPSS_ST_PRJH_S       *pstPrjhInfo;  
    VPSS_ST_PRJV_S       *pstPrjvInfo;

    if (HI_NULL == pstMcdeiInfo)
    {
        VPSS_ERROR("para pstMcdeiInfo is HI_NULL\n");
        return HI_FAILURE;
    }
    pstRgmeWbcInfo = &(pstMcdeiInfo->stRgmeWbcInfo);
    pstBlendWbcInfo = &(pstMcdeiInfo->stBlendWbcInfo);
    pstRgmeInfo = &(pstMcdeiInfo->stRgmeInfo);
    pstBlkmvInfo = &(pstMcdeiInfo->stBlkmvInfo);
    pstPrjhInfo = &(pstMcdeiInfo->stPrjhInfo);
    pstPrjvInfo = &(pstMcdeiInfo->stPrjvInfo);

    if (HI_TRUE != pstMcdeiInfo->bMcEnable)
    {
        return HI_SUCCESS;
    }

    VPSS_BLEND_CompleteImage(pstBlendWbcInfo);
    VPSS_RGME_CompleteImage(pstRgmeWbcInfo);
    VPSS_ST_RGME_Complete(pstRgmeInfo);
    VPSS_ST_BLKMV_Complete(pstBlkmvInfo);
    VPSS_ST_PRJH_Complete(pstPrjhInfo);
    VPSS_ST_PRJV_Complete(pstPrjvInfo);     
    return HI_SUCCESS;

}

HI_S32 VPSS_INST_McDeiReset(VPSS_MCDEI_INFO_S *pstMcdeiInfo)
{
 
    VPSS_RGME_S          *pstRgmeWbcInfo;
    VPSS_BLEND_S         *pstBlendWbcInfo;
    VPSS_ST_RGME_S       *pstRgmeInfo;   
    VPSS_ST_BLKMV_S      *pstBlkmvInfo; 
    VPSS_ST_PRJH_S       *pstPrjhInfo;  
    VPSS_ST_PRJV_S       *pstPrjvInfo;

    if (HI_NULL == pstMcdeiInfo)
    {
        VPSS_ERROR("para pstMcdeiInfo is HI_NULL\n");
        return HI_FAILURE;
    }
    pstRgmeWbcInfo = &(pstMcdeiInfo->stRgmeWbcInfo);
    pstBlendWbcInfo = &(pstMcdeiInfo->stBlendWbcInfo);
    pstRgmeInfo = &(pstMcdeiInfo->stRgmeInfo);
    pstBlkmvInfo = &(pstMcdeiInfo->stBlkmvInfo);
    pstPrjhInfo = &(pstMcdeiInfo->stPrjhInfo);
    pstPrjvInfo = &(pstMcdeiInfo->stPrjvInfo);

    if (HI_TRUE != pstMcdeiInfo->bMcEnable)
    {
        return HI_SUCCESS;
    }

    VPSS_BLEND_Reset(pstBlendWbcInfo);
    VPSS_RGME_Reset(pstRgmeWbcInfo);
    VPSS_ST_RGME_Reset(pstRgmeInfo);
    VPSS_ST_BLKMV_Reset(pstBlkmvInfo);
    VPSS_ST_PRJH_Reset(pstPrjhInfo);
    VPSS_ST_PRJV_Reset(pstPrjvInfo); 
    return HI_SUCCESS;

}

HI_S32 VPSS_INST_SetHalMcdeiInfo(VPSS_MCDEI_INFO_S * pstMcdeiInfo,VPSS_HAL_INFO_S *pstHalInfo,
    HI_DRV_BUF_ADDR_E enLR, HI_DRV_VIDEO_FRAME_S *pstSrcImage)
{   
    HI_S32 i;     
    HI_S32 s32Ret = HI_FAILURE;
    HI_DRV_VIDEO_FRAME_S *pstRef[4] = {HI_NULL,HI_NULL,HI_NULL,HI_NULL};
    HI_DRV_VIDEO_FRAME_S *pstWbc;
    VPSS_RGME_S          *pstRgmeWbcInfo;
    VPSS_BLEND_S         *pstBlendWbcInfo;
    VPSS_ST_RGME_S       *pstRgmeInfo;   
    VPSS_ST_BLKMV_S      *pstBlkmvInfo; 
    VPSS_ST_PRJH_S       *pstPrjhInfo;  
    VPSS_ST_PRJV_S       *pstPrjvInfo;

    if (HI_TRUE != pstMcdeiInfo->bMcEnable)
    {   
        pstHalInfo->stMcdeiRfrInfo.bMcdeiEn = HI_FALSE;
        return HI_SUCCESS;
    }  

    pstRgmeWbcInfo = &(pstMcdeiInfo->stRgmeWbcInfo);
    pstBlendWbcInfo = &(pstMcdeiInfo->stBlendWbcInfo);
    pstRgmeInfo = &(pstMcdeiInfo->stRgmeInfo);
    pstBlkmvInfo = &(pstMcdeiInfo->stBlkmvInfo);
    pstPrjhInfo = &(pstMcdeiInfo->stPrjhInfo);
    pstPrjvInfo = &(pstMcdeiInfo->stPrjvInfo);

    pstHalInfo->stMcdeiRfrInfo.bMedsEn = VPSS_INST_McDeiGetMedsEn(pstSrcImage);
    pstHalInfo->stMcdeiRfrInfo.bMcdeiEn = HI_TRUE;

     /* Blend WbcInfo */
    s32Ret = VPSS_BLEND_GetWbcInfo(pstBlendWbcInfo, &pstWbc);
    if (HI_FAILURE == s32Ret)
    {
        VPSS_FATAL("VPSS_BLEND_GetWbcInfo failed!\n");
        return HI_FAILURE;
    }
    
    VPSS_INST_SetMcDeiFrameInfo(pstWbc, &pstHalInfo->stMcdeiRfrInfo.stBlendWbc, enLR); 
    s32Ret = VPSS_BLEND_GetRefInfo(pstBlendWbcInfo, pstRef);
    if (HI_FAILURE == s32Ret)
    {
        VPSS_FATAL("VPSS_RGME_GetRefInfo failed!\n");
        return HI_FAILURE;
    }
    
    VPSS_INST_SetMcDeiFrameInfo(pstRef[0], &pstHalInfo->stMcdeiRfrInfo.stBlendRef, enLR);

    /* MCDEI STT INFO */
    VPSS_ST_RGME_GetCfg(pstRgmeInfo,&(pstHalInfo->stMcdeiStInfo.stRgmeCfg));
    VPSS_ST_BLKMV_GetCfg(pstBlkmvInfo,&(pstHalInfo->stMcdeiStInfo.stBlkmvCfg));
    VPSS_ST_PRJH_GetCfg(pstPrjhInfo,&(pstHalInfo->stMcdeiStInfo.stPrjhCfg));
    VPSS_ST_PRJV_GetCfg(pstPrjvInfo,&(pstHalInfo->stMcdeiStInfo.stPrjvCfg));      
	
	/* RGME WbcInfo */
    s32Ret = VPSS_RGME_GetWbcInfo(pstRgmeWbcInfo, &pstWbc);
    if (HI_FAILURE == s32Ret)
    {
        VPSS_FATAL("VPSS_RGME_GetWbcInfo failed!\n");
        return HI_FAILURE;
    }
    
    VPSS_INST_SetMcDeiFrameInfo(pstWbc, &pstHalInfo->stMcdeiRfrInfo.stRgmeWbc, enLR);
    
    s32Ret = VPSS_RGME_GetRefInfo(pstRgmeWbcInfo, pstRef);
    if (HI_FAILURE == s32Ret)
    {
        VPSS_FATAL("VPSS_RGME_GetRefInfo failed!\n");
        return HI_FAILURE;
    }

    if(HI_NULL == pstRef[0])
    {
        pstHalInfo->stMcdeiRfrInfo.bMcdeiEn = HI_FALSE;
        pstHalInfo->stMcdeiRfrInfo.bMedsEn = HI_FALSE; 
        return HI_SUCCESS;
    }
    
    for(i = 0; i < VPSS_RGME_WBC_MAX_NODE - 1; i++)
    {
        VPSS_INST_SetMcDeiFrameInfo(pstRef[i], &(pstHalInfo->stMcdeiRfrInfo.stRgmeRef[i]), enLR);
    }   
    return HI_SUCCESS;
}

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */

