/******************************************************************************
*
* Copyright (C) 2014 Hisilicon Technologies Co., Ltd.  All rights reserved.
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon),
*  and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
*****************************************************************************

  File Name     : pq_mng_zme.c
  Version       : Initial Draft
  Author        : pengjunwei 00203646
  Created       : 2014/04/01
  Author        : pengjunwei 00203646
******************************************************************************/

#include "hi_drv_mmz.h"
#include "pq_mng_zme.h"
#include "pq_hal_comm.h"


/* VDP ZME COMMON SET: fixed to false; interspersed format */
#define SRC_COSITED  0

/* Filter and Mid Flag */
static HI_BOOL g_bVdpZmeFirEn  = HI_TRUE;
static HI_BOOL g_bVdpZmeMedEn  = HI_FALSE;
static HI_BOOL g_bVpssZmeFirEn = HI_TRUE;
static HI_BOOL g_bVpssZmeMedEn = HI_FALSE;

static HI_U32  sg_u32Zme2LThl     = 4; /*Zme二级缩放门限值 */
static HI_BOOL g_bZmeInitFlag     = HI_FALSE;
static HI_BOOL g_bVdpZmeInitFlag  = HI_FALSE;
static HI_BOOL g_bVpssZmeInitFlag = HI_FALSE;
static PQ_TUN_MODE_E sg_enZMETunMode = PQ_TUN_NORMAL;


ALG_VZME_MEM_S g_stVZMEVdpInstance;
ALG_VZME_MEM_S g_stVZMEVpssInstance;


typedef  HI_VOID (*FN_Zme_CoefCalculate)(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara);
typedef  HI_VOID (*FN_Zme_VdpRegCfg)(HI_U32 u32LayerId, ALG_VZME_RTL_PARA_S* pstZmeRtlPara, HI_BOOL bFirEnable);
typedef  HI_VOID (*FN_Zme_VpssRegCfg)(HI_U32 u32LayerId, S_CAS_REGS_TYPE* pstReg, ALG_VZME_RTL_PARA_S* pstZmeRtlPara, HI_BOOL  bFirEnable);

typedef struct
{
    FN_Zme_CoefCalculate   pfnZme_CoefCalculate;
    FN_Zme_VdpRegCfg       pfnZme_VdpRegCfg;
} ZME_VDP_FUNC_S;

typedef struct
{
    FN_Zme_CoefCalculate   pfnZme_CoefCalculate;
    FN_Zme_VpssRegCfg      pfnZme_VpssRegCfg;
} ZME_VPSS_FUNC_S;


HI_S32 PQ_MNG_SetZmeEnMode(PQ_ZME_MODE_E enMode, HI_BOOL bOnOff)
{
    switch (enMode)
    {
        case PQ_ZME_MODE_VDP_FIR:
            g_bVdpZmeFirEn = bOnOff;
            break;
        case PQ_ZME_MODE_VDP_MED:
            g_bVdpZmeMedEn = bOnOff;
            break;
        case PQ_ZME_MODE_VPSS_FIR:
            g_bVpssZmeFirEn = bOnOff;
            break;
        case PQ_ZME_MODE_VPSS_MED:
            g_bVpssZmeMedEn = bOnOff;
            break;
        default:
            break;
    }

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_GetZmeEnMode(PQ_ZME_MODE_E enMode, HI_BOOL* pbOnOff)
{
    PQ_CHECK_NULL_PTR(pbOnOff);

    switch (enMode)
    {
        case PQ_ZME_MODE_VDP_FIR:
            *pbOnOff = g_bVdpZmeFirEn;
            break;
        case PQ_ZME_MODE_VDP_MED:
            *pbOnOff = g_bVdpZmeMedEn;
            break;
        case PQ_ZME_MODE_VPSS_FIR:
            *pbOnOff = g_bVpssZmeFirEn;
            break;
        case PQ_ZME_MODE_VPSS_MED:
            *pbOnOff = g_bVpssZmeMedEn;
            break;
        default:
            break;
    }

    return HI_SUCCESS;
}


/* common zme coefficient memory intial; get a static address pointer;
   several zme modules can use one memory block to save coefficient.
   */
static HI_S32 PQ_MNG_VdpZmeComnInit(ALG_VZME_MEM_S* pstVZmeVdpCoefMem)
{
    /* load zme coefficient into the memory */
    PQ_HAL_VdpLoadCoef(pstVZmeVdpCoefMem);

    /* Filter Mode and Median Filter */
    PQ_MNG_SetZmeEnMode(PQ_ZME_MODE_VDP_FIR, HI_TRUE);
    PQ_MNG_SetZmeEnMode(PQ_ZME_MODE_VDP_MED, HI_FALSE);

    return HI_SUCCESS;
}

static HI_VOID PQ_MNG_VdpZmeComnDeInit(ALG_VZME_MEM_S* pstVZmeVdpCoefMem)
{
    return;
}

/* common zme coefficient memory intial; get a static address pointer;
   several zme modules can use one memory block to save coefficient
   */
static HI_S32 PQ_MNG_VpssZmeComnInit(ALG_VZME_MEM_S* pstVZmeVpssCoefMem)
{
    /* load zme coefficient into the memory */
    PQ_HAL_VpssLoadCoef((HI_U32*)(pstVZmeVpssCoefMem->stMBuf.pu8StartVirAddr),
                        pstVZmeVpssCoefMem->stMBuf.u32StartPhyAddr, &(pstVZmeVpssCoefMem->stZmeCoefAddr));

    /* Filter Mode and Median Filter */
    PQ_MNG_SetZmeEnMode(PQ_ZME_MODE_VPSS_FIR, HI_TRUE);
    PQ_MNG_SetZmeEnMode(PQ_ZME_MODE_VPSS_MED, HI_FALSE);

    return HI_SUCCESS;
}

static HI_VOID PQ_MNG_VpssZmeComnDeInit(ALG_VZME_MEM_S* pstVZmeVpssCoefMem)
{
    return;
}

static HI_S32 GetVerticalScalerOffset(HI_U32 u32VRatio, ALG_VZME_DRV_PARA_S* pstZmeDrvPara)
{
    HI_S32 s32VOffset = 0;

    if (pstZmeDrvPara->bZmeFrmFmtIn) /* processive input */
    {
        if (pstZmeDrvPara->bZmeFrmFmtOut) /* processive output */
        {
            s32VOffset = 0;
        }
        else /* interlaced output */
        {
            if (pstZmeDrvPara->bZmeBFOut) /* bottom field output */
            {
                s32VOffset = (HI_S32)u32VRatio >> 1;
            }
            else /*top field output */
            {
                s32VOffset = 0;
            }
        }
    }
    else /* interlaced input */
    {
        if (pstZmeDrvPara->bZmeFrmFmtOut) /* processive output */
        {
            if (pstZmeDrvPara->bZmeBFIn) /* bottom field input */
            {
                s32VOffset = -(ALG_V_VZME_PRECISION >> 1); /* -2048 */
            }
            else /* top field input */
            {
                s32VOffset = 0;
            }
        }
        else /* interlaced output */
        {
            if (pstZmeDrvPara->bZmeBFIn) /* bottom field input */
            {
                if (pstZmeDrvPara->bZmeBFOut) /* bottom field output */
                {
                    s32VOffset = ((HI_S32)u32VRatio - ALG_V_VZME_PRECISION) >> 1;
                }
                else /* top field output */
                {
                    s32VOffset = -(ALG_V_VZME_PRECISION >> 1); /* -2048 */
                }
            }
            else /* top field input */
            {
                if (pstZmeDrvPara->bZmeBFOut) /* bottom field output */
                {
                    s32VOffset = (HI_S32)u32VRatio >> 1;
                }
                else /* top field output */
                {
                    s32VOffset = 0;
                }
            }
        }
    }

    return s32VOffset;
}

/* NOTICE:
   Vdp video zme module is the same as graphic layer G0/G1;
   software don't need separate the progressive frome interlace of the output, hardware process it.
   */
HI_VOID PQ_MNG_VdpZmeComnSet(ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    HI_U32 u32ZmeCWIn, u32ZmeCWOut, u32ZmeCHIn, u32ZmeCHOut;
    HI_S32 s32VOffset, s32HOffset;
    HI_U32 u32ZmeRatioVLReal, u32ZmeRatioVCReal;

    /* config zme enable */
    pstZmeRtlPara->bZmeEnHL = HI_TRUE;
    pstZmeRtlPara->bZmeEnHC = HI_TRUE;
    pstZmeRtlPara->bZmeEnVL = HI_TRUE;
    pstZmeRtlPara->bZmeEnVC = HI_TRUE;

    /* config zme median filter enable */
    pstZmeRtlPara->bZmeMedHL = g_bVdpZmeMedEn; /* HI_FALSE */
    pstZmeRtlPara->bZmeMedHC = g_bVdpZmeMedEn;
    pstZmeRtlPara->bZmeMedVL = g_bVdpZmeMedEn;
    pstZmeRtlPara->bZmeMedVC = g_bVdpZmeMedEn;

    /* calculate luma real zme resolution of input and output */
    pstZmeRtlPara->u32ZmeWIn  = pstZmeDrvPara->u32ZmeFrmWIn;
    pstZmeRtlPara->u32ZmeWOut = pstZmeDrvPara->u32ZmeFrmWOut;
    pstZmeRtlPara->u32ZmeHIn  = pstZmeDrvPara->u32ZmeFrmHIn;
    pstZmeRtlPara->u32ZmeHOut = pstZmeDrvPara->u32ZmeFrmHOut;
    //pstZmeRtlPara->u32ZmeHIn  = ( pstZmeDrvPara->bZmeFrmFmtIn  == 1 ) ? pstZmeDrvPara->u32ZmeFrmHIn: pstZmeDrvPara->u32ZmeFrmHIn/2;
    //pstZmeRtlPara->u32ZmeHOut = ( pstZmeDrvPara->bZmeFrmFmtOut == 1 ) ? pstZmeDrvPara->u32ZmeFrmHOut: pstZmeDrvPara->u32ZmeFrmHOut/2;

    /* when wide > 4096, overflow will occurs */
    if (pstZmeRtlPara->u32ZmeWIn >= 4096)
    {
        pstZmeRtlPara->u32ZmeRatioHL = (pstZmeRtlPara->u32ZmeWIn / 2 * ALG_V_HZME_PRECISION) / pstZmeRtlPara->u32ZmeWOut * 2;
    }
    else
    {
        pstZmeRtlPara->u32ZmeRatioHL = pstZmeRtlPara->u32ZmeWIn * ALG_V_HZME_PRECISION / pstZmeRtlPara->u32ZmeWOut;
    }
    pstZmeRtlPara->u32ZmeRatioVL = pstZmeRtlPara->u32ZmeHIn * ALG_V_VZME_PRECISION / pstZmeRtlPara->u32ZmeHOut;

    /* calculate chroma zme ratio */
    u32ZmeCWIn  = ( pstZmeDrvPara->u8ZmeYCFmtIn  == 2 ) ? pstZmeRtlPara->u32ZmeWIn : pstZmeRtlPara->u32ZmeWIn / 2;
    u32ZmeCWOut = ( pstZmeDrvPara->u8ZmeYCFmtOut == 2 ) ? pstZmeRtlPara->u32ZmeWOut : pstZmeRtlPara->u32ZmeWOut / 2;
    u32ZmeCHIn  = ( pstZmeDrvPara->u8ZmeYCFmtIn  == 1 ) ? pstZmeRtlPara->u32ZmeHIn / 2 : pstZmeRtlPara->u32ZmeHIn;
    u32ZmeCHOut = ( pstZmeDrvPara->u8ZmeYCFmtOut == 1 ) ? pstZmeRtlPara->u32ZmeHOut / 2 : pstZmeRtlPara->u32ZmeHOut;

    /* when wide > 4096, overflow will occurs */
    if (u32ZmeCWIn >= 4096)
    {
        pstZmeRtlPara->u32ZmeRatioHC = (u32ZmeCWIn / 2 * ALG_V_HZME_PRECISION) / u32ZmeCWOut * 2;
    }
    else
    {
        pstZmeRtlPara->u32ZmeRatioHC = u32ZmeCWIn * ALG_V_HZME_PRECISION / u32ZmeCWOut;
    }
    pstZmeRtlPara->u32ZmeRatioVC = u32ZmeCHIn * ALG_V_VZME_PRECISION / u32ZmeCHOut;

    /* Input Progressive and Output Interlace */
    if (1 == pstZmeDrvPara->bZmeFrmFmtIn && 0 == pstZmeDrvPara->bZmeFrmFmtOut)
    {
        u32ZmeRatioVLReal = pstZmeRtlPara->u32ZmeRatioVL * 2;
        u32ZmeRatioVCReal = pstZmeRtlPara->u32ZmeRatioVC * 2;
    }
    else
    {
        u32ZmeRatioVLReal = pstZmeRtlPara->u32ZmeRatioVL;
        u32ZmeRatioVCReal = pstZmeRtlPara->u32ZmeRatioVC;
    }

    /* calculate luma zme offset */
    pstZmeRtlPara->s32ZmeOffsetHL = 0;

    /* Set Top filelds calc ZmeoffsetVL */
#if defined(CHIP_TYPE_hi3798mv100) || defined(CHIP_TYPE_hi3796mv100) || defined(CHIP_TYPE_hi3798cv200_a)||defined(CHIP_TYPE_hi3798cv200_b)
    /* mv410 need distinguish read top or bottom from vdp add 20150626 */
    pstZmeDrvPara->bZmeBFOut = 0; /* output top field */
#endif
    pstZmeRtlPara->s32ZmeOffsetVL = GetVerticalScalerOffset(u32ZmeRatioVLReal, pstZmeDrvPara);

    /* calculate chroma zme offset */
    s32VOffset = 0, s32HOffset = 0;
    if (1 == pstZmeDrvPara->u8ZmeYCFmtIn) /* 4:2:0 */
    {
        /* horizontal offset for cosited or interspersed format *//* fixed to false; interspersed format */
#if SRC_COSITED
        s32HOffset = -(ALG_V_HZME_PRECISION >> 1); /* -2048 */
#else
        s32HOffset = 0;
#endif
        /* vertical offset for processive or interlaced input format */
        if (pstZmeDrvPara->bZmeFrmFmtIn) /* processive input for zme */
        {
            s32VOffset = -(ALG_V_VZME_PRECISION >> 2) >> 1; /* -1024 >> 1 */
        }
        else /* interlaced input for zme */
        {
            if (pstZmeDrvPara->bZmeBFIn) /* bottom field input for zme */
            {
                s32VOffset = -((3 * ALG_V_VZME_PRECISION) >> 2) >> 1; /* -3072 >> 1 */
            }
            else /* top field input for zme */
            {
                s32VOffset = -(ALG_V_VZME_PRECISION >> 2) >> 1; /* -1024 >> 1 */
            }
        }
    }

    pstZmeRtlPara->s32ZmeOffsetHC = s32HOffset;
    pstZmeRtlPara->s32ZmeOffsetVC = s32VOffset + GetVerticalScalerOffset(u32ZmeRatioVCReal, pstZmeDrvPara);

    /* Set Bottom filelds calc s32ZmeOffsetVLBtm */ /* Calc Offset At one Function */
#if defined(CHIP_TYPE_hi3798mv100) || defined(CHIP_TYPE_hi3796mv100) || defined(CHIP_TYPE_hi3798cv200_a)||defined(CHIP_TYPE_hi3798cv200_b)
    /* mv410 need distinguish read top or bottom from vdp add 20150626 */
    pstZmeDrvPara->bZmeBFOut = 1; /* output bottom field */
#endif
    pstZmeRtlPara->s32ZmeOffsetVLBtm = GetVerticalScalerOffset(u32ZmeRatioVLReal, pstZmeDrvPara);
    pstZmeRtlPara->s32ZmeOffsetVCBtm = s32VOffset + GetVerticalScalerOffset(u32ZmeRatioVCReal, pstZmeDrvPara);

    /* config zme mode */
    pstZmeRtlPara->bZmeMdHL = (ALG_V_HZME_PRECISION == pstZmeRtlPara->u32ZmeRatioHL) ? HI_FALSE : HI_TRUE;
    pstZmeRtlPara->bZmeMdHC = (ALG_V_HZME_PRECISION == pstZmeRtlPara->u32ZmeRatioHC) ? HI_FALSE : HI_TRUE;
    pstZmeRtlPara->bZmeMdHL = (pstZmeRtlPara->bZmeMdHL) && g_bVdpZmeFirEn;
    pstZmeRtlPara->bZmeMdHC = (pstZmeRtlPara->bZmeMdHC) && g_bVdpZmeFirEn;

    /* when 1:1 vertical scaler, directly copy */
    if (ALG_V_VZME_PRECISION == pstZmeRtlPara->u32ZmeRatioVL && 0 == pstZmeRtlPara->s32ZmeOffsetVL)
    {
        pstZmeRtlPara->bZmeMdVL = HI_FALSE;
    }
    else
    {
        pstZmeRtlPara->bZmeMdVL = HI_TRUE && g_bVdpZmeFirEn;
    }
    /* when 1:1 vertical scaler, directly copy */
    if (ALG_V_VZME_PRECISION == pstZmeRtlPara->u32ZmeRatioVC && 0 == pstZmeRtlPara->s32ZmeOffsetVC)
    {
        pstZmeRtlPara->bZmeMdVC = HI_FALSE;
    }
    else
    {
        pstZmeRtlPara->bZmeMdVC = HI_TRUE && g_bVdpZmeFirEn;
    }

    if (HI_FALSE == pstZmeDrvPara->bDispProgressive && 0 == pstZmeDrvPara->u32Fidelity) /* interlace output and no rwzb */
    {
        if ( (pstZmeDrvPara->stOriRect.s32Width == pstZmeDrvPara->u32ZmeFrmWOut
              && pstZmeDrvPara->stOriRect.s32Height == pstZmeDrvPara->u32ZmeFrmHOut
              && pstZmeDrvPara->u32InRate != pstZmeDrvPara->u32OutRate)
             || (pstZmeDrvPara->stOriRect.s32Height > pstZmeDrvPara->u32ZmeFrmHOut) )
        {
            pstZmeRtlPara->bZmeMdHL = HI_TRUE && g_bVdpZmeFirEn;
            pstZmeRtlPara->bZmeMdHC = HI_TRUE && g_bVdpZmeFirEn;
            pstZmeRtlPara->bZmeMdVL = HI_TRUE && g_bVdpZmeFirEn;
            pstZmeRtlPara->bZmeMdVC = HI_TRUE && g_bVdpZmeFirEn;
        }
        else
        {}
    }
    else
    {}

    /* config vertical chroma zme tap and zme order */
    pstZmeRtlPara->bZmeTapVC = 0;
    pstZmeRtlPara->bZmeOrder = (pstZmeRtlPara->u32ZmeRatioHL >= ALG_V_HZME_PRECISION) ? 0 : 1;

    pstZmeRtlPara->u8ZmeYCFmtIn  = pstZmeDrvPara->u8ZmeYCFmtIn;
    pstZmeRtlPara->u8ZmeYCFmtOut = pstZmeDrvPara->u8ZmeYCFmtOut;

    return;
}

HI_S32 PQ_MNG_VpssZmeComnSet(ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    /* for TQE bSrcCosited is fixed to false */
#if 0
    HI_BOOL bSrcCosited = HI_FALSE;
#endif
    HI_U32 u32ZmeCWIn, u32ZmeCWOut, u32ZmeCHIn, u32ZmeCHOut;
    HI_S32 s32VOffset, s32HOffset;

    if (pstZmeDrvPara->u32ZmeFrmWOut == 0)
    {
        HI_ERR_PQ("u32ZmeFrmWOut == 0");
        return HI_FAILURE;
    }
    if (pstZmeDrvPara->u32ZmeFrmHOut == 0)
    {
         HI_ERR_PQ("u32ZmeFrmHOut == 0");
        return HI_FAILURE;
    }

    /* config zme enable */
    pstZmeRtlPara->bZmeEnHL = HI_TRUE;
    pstZmeRtlPara->bZmeEnHC = HI_TRUE;
    pstZmeRtlPara->bZmeEnVL = HI_TRUE;
    pstZmeRtlPara->bZmeEnVC = HI_TRUE;

    /* config zme median filter enable */
    pstZmeRtlPara->bZmeMedHL = g_bVpssZmeMedEn; /* HI_FALSE */
    pstZmeRtlPara->bZmeMedHC = g_bVpssZmeMedEn;
    pstZmeRtlPara->bZmeMedVL = g_bVpssZmeMedEn;
    pstZmeRtlPara->bZmeMedVC = g_bVpssZmeMedEn;

    /* calculate luma real zme resolution of input and output */
    pstZmeRtlPara->u32ZmeWIn  = pstZmeDrvPara->u32ZmeFrmWIn;
    pstZmeRtlPara->u32ZmeWOut = pstZmeDrvPara->u32ZmeFrmWOut;
    pstZmeRtlPara->u32ZmeHIn  = ( pstZmeDrvPara->bZmeFrmFmtIn  == 1 ) ? pstZmeDrvPara->u32ZmeFrmHIn : pstZmeDrvPara->u32ZmeFrmHIn / 2;
    pstZmeRtlPara->u32ZmeHOut = ( pstZmeDrvPara->bZmeFrmFmtOut == 1 ) ? pstZmeDrvPara->u32ZmeFrmHOut : pstZmeDrvPara->u32ZmeFrmHOut / 2;

    /* when wide > 4096, overflow will occurs */
    if (pstZmeRtlPara->u32ZmeWIn >= 4096)
    {
        pstZmeRtlPara->u32ZmeRatioHL = (pstZmeRtlPara->u32ZmeWIn / 2 * ALG_V_HZME_PRECISION) / pstZmeRtlPara->u32ZmeWOut * 2;
    }
    else
    {
        pstZmeRtlPara->u32ZmeRatioHL = pstZmeRtlPara->u32ZmeWIn * ALG_V_HZME_PRECISION / pstZmeRtlPara->u32ZmeWOut;
    }
    pstZmeRtlPara->u32ZmeRatioVL = pstZmeRtlPara->u32ZmeHIn * ALG_V_VZME_PRECISION / pstZmeRtlPara->u32ZmeHOut;

    /* calculate chroma zme ratio */
    u32ZmeCWIn  = ( pstZmeDrvPara->u8ZmeYCFmtIn  == PQ_ALG_ZME_PIX_FORMAT_SP444) ? pstZmeRtlPara->u32ZmeWIn : pstZmeRtlPara->u32ZmeWIn / 2;
    u32ZmeCWOut = ( pstZmeDrvPara->u8ZmeYCFmtOut == PQ_ALG_ZME_PIX_FORMAT_SP444) ? pstZmeRtlPara->u32ZmeWOut : pstZmeRtlPara->u32ZmeWOut / 2;
    u32ZmeCHIn  = ( pstZmeDrvPara->u8ZmeYCFmtIn  == PQ_ALG_ZME_PIX_FORMAT_SP422) ? pstZmeRtlPara->u32ZmeHIn / 2 : pstZmeRtlPara->u32ZmeHIn;
    u32ZmeCHOut = ( pstZmeDrvPara->u8ZmeYCFmtOut == PQ_ALG_ZME_PIX_FORMAT_SP422) ? pstZmeRtlPara->u32ZmeHOut / 2 : pstZmeRtlPara->u32ZmeHOut;

    /* when wide > 4096, overflow will occurs */
    if (pstZmeRtlPara->u32ZmeWIn >= 4096)
    {
        pstZmeRtlPara->u32ZmeRatioHC = (u32ZmeCWIn / 2 * ALG_V_HZME_PRECISION) / u32ZmeCWOut * 2;
    }
    else
    {
        pstZmeRtlPara->u32ZmeRatioHC = u32ZmeCWIn * ALG_V_HZME_PRECISION / u32ZmeCWOut;
    }

    pstZmeRtlPara->u32ZmeRatioVC = u32ZmeCHIn * ALG_V_VZME_PRECISION / u32ZmeCHOut;

    /* calculate luma zme offset */
    pstZmeRtlPara->s32ZmeOffsetHL = 0;
    pstZmeRtlPara->s32ZmeOffsetVL = GetVerticalScalerOffset(pstZmeRtlPara->u32ZmeRatioVL, pstZmeDrvPara);

    /* calculate chroma zme offset */
    s32VOffset = 0, s32HOffset = 0;
    /* it may influence rwzb,chrome linhang chuanrao */
#if 0
    if (pstZmeDrvPara->eZmeYCFmtIn == HI_DRV_PIX_FMT_NV21) /* 4:2:0 */
    {
        /* horizontal offset for cosited or interspersed format *//* for TQE bSrcCosited is fixed to false */
#if 0
        if (bSrcCosited) /* fixed to false*/ /* interspersed format */
        {
            s32HOffset = -(ALG_V_HZME_PRECISION >> 1); //-2048;
        }
        else
        {
            s32HOffset = 0;
        }
#else
        s32HOffset = 0;
#endif
        /* vertical offset for processive or interlaced input format */
        if (pstZmeDrvPara->bZmeFrmFmtIn) /* processive input for zme */
        {
            s32VOffset = -(ALG_V_VZME_PRECISION >> 2) >> 1; //-1024 >> 1;
        }
        else /* interlaced input for zme */
        {
            if (pstZmeDrvPara->bZmeBFIn) /* bottom field input for zme */
            {
                s32VOffset = -((3 * ALG_V_VZME_PRECISION) >> 2) >> 1; //-3072 >> 1;
            }
            else /* top field input for zme */
            {
                s32VOffset = -(ALG_V_VZME_PRECISION >> 2) >> 1; //-1024 >> 1;
            }
        }
    }
#endif

    pstZmeRtlPara->s32ZmeOffsetHC = s32HOffset;
    pstZmeRtlPara->s32ZmeOffsetVC = s32VOffset + GetVerticalScalerOffset(pstZmeRtlPara->u32ZmeRatioVC, pstZmeDrvPara);
    /* config zme mode */
    pstZmeRtlPara->bZmeMdHL = (ALG_V_HZME_PRECISION == pstZmeRtlPara->u32ZmeRatioHL) ? HI_FALSE : HI_TRUE;
    pstZmeRtlPara->bZmeMdHC = (ALG_V_HZME_PRECISION == pstZmeRtlPara->u32ZmeRatioHC) ? HI_FALSE : HI_TRUE;
    pstZmeRtlPara->bZmeMdHL = (pstZmeRtlPara->bZmeMdHL) && g_bVpssZmeFirEn; /* HI_TRUE */
    pstZmeRtlPara->bZmeMdHC = (pstZmeRtlPara->bZmeMdHC) && g_bVpssZmeFirEn;
    /* when 1:1 vertical scaler, directly copy */
    if (ALG_V_VZME_PRECISION == pstZmeRtlPara->u32ZmeRatioVL && 0 == pstZmeRtlPara->s32ZmeOffsetVL)
    {
        pstZmeRtlPara->bZmeMdVL = HI_FALSE;
    }
    else
    {
        pstZmeRtlPara->bZmeMdVL = HI_TRUE && g_bVpssZmeFirEn;
    }

    /* when 1:1 vertical scaler, directly copy */
    if (ALG_V_VZME_PRECISION == pstZmeRtlPara->u32ZmeRatioVC && 0 == pstZmeRtlPara->s32ZmeOffsetVC)
    {
        pstZmeRtlPara->bZmeMdVC = HI_FALSE;
    }
    else
    {
        pstZmeRtlPara->bZmeMdVC = HI_TRUE && g_bVpssZmeFirEn;
    }

    /* config vertical chroma zme tap and zme order */
    pstZmeRtlPara->bZmeTapVC = 0;
    pstZmeRtlPara->bZmeOrder = (pstZmeRtlPara->u32ZmeRatioHL >= ALG_V_HZME_PRECISION) ? 0 : 1;

    pstZmeRtlPara->u8ZmeYCFmtIn  = pstZmeDrvPara->u8ZmeYCFmtIn;
    pstZmeRtlPara->u8ZmeYCFmtOut = pstZmeDrvPara->u8ZmeYCFmtOut;

    return HI_SUCCESS;
}

/* Tmp No Using */
static HI_U32 GetHLCfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8C4_0;   }

    return u32CoefAddr;
}

/* Tmp No Using */
static HI_U32 GetVLCfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, VZME_PICKCOEF_PARA_S stPickCoef)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == stPickCoef.u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        stPickCoef.u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / stPickCoef.u32Ratio;

    if ( (stPickCoef.u32TapL) == 6)
    {
        if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_1;   }
        else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_E1;  }
        else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_075; }
        else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_05;  }
        else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_033; }
        else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_025; }
        else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6C4_0;   }
    }
    else
    {
        if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_1;   }
        else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_E1;  }
        else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_075; }
        else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_05;  }
        else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_033; }
        else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_025; }
        else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4C4_0;   }
    }

    return u32CoefAddr;
}

/* commonly used */
static HI_U32 GetHLfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHL8_0;   }

    return u32CoefAddr;
}

/* commonly used */
static HI_U32 GetHCfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC4_0;   }

    return u32CoefAddr;
}

/* commonly used */
static HI_U32 GetVCfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC4_0;   }

    return u32CoefAddr;
}

/* 98M And MV410 */
static HI_U32 GetVLfirCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL4_0;   }

    return u32CoefAddr;
}

/* Add from 3798c and hiFoneB2 */
static HI_U32 GetVLFilterCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVL6_0;   }

    return u32CoefAddr;
}

/* 98CV200_A */
static HI_U32 GetHCFilterCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrHC8_0;   }

    return u32CoefAddr;
}

/* Add from 3798c and hiFoneB2 *//* 98CV200_A */
static HI_U32 GetVCFilterCoef(ALG_VZME_COEF_ADDR_S* pstCoefAddr, HI_U32 u32Ratio)
{
    HI_U32 u32CoefAddr;
    HI_U32 s32TmpRatio;

    if (0 == u32Ratio)
    {
        HI_ERR_PQ("ratio equal 0, error\n");
        u32Ratio = 1;
    }

    s32TmpRatio = 4096 * 4096 / u32Ratio;

    if      (s32TmpRatio > 4096 ) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_1;   }
    else if (s32TmpRatio == 4096) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_E1;  }
    else if (s32TmpRatio >= 3072) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_075; }
    else if (s32TmpRatio >= 2048) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_05;  }
    else if (s32TmpRatio >= 1365) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_033; }
    else if (s32TmpRatio >= 1024) { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_025; }
    else                          { u32CoefAddr = pstCoefAddr->u32ZmeCoefAddrVC6_0;   }

    return u32CoefAddr;
}

/* Mixed HLC84 VLC64 *//* Tmp No Using */
HI_VOID PQ_MNG_VdpZmeSQSetAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    VZME_PICKCOEF_PARA_S stPickCoef;

    memset(&stPickCoef, 0, sizeof(VZME_PICKCOEF_PARA_S));
    PQ_MNG_VdpZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );

    stPickCoef.u32Ratio = pstZmeRtlPara->u32ZmeRatioVL;
    stPickCoef.u32TapL  = 4;
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLCfirCoef( &(pstMem->stZmeCoefAddr), stPickCoef );

    return;
}

/* Separated HL8 HC4 VL4 VC4 *//* 98M And MV410 */
HI_VOID PQ_MNG_VdpZmeSQSetSptAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    PQ_MNG_VdpZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    /* Hor */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 );

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL );
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC );

    return;
}

/* add from 3716CV200 VPSS, 98MV100 is the same: in 98MV100 HScalar also use the Horizontal Coef HL8 HC4 */
/* Separated HL8 HC4 VL6 VC4 *//* 98M */
HI_VOID PQ_MNG_VpssZmeHQSetSptAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    /* fit refer to input H and V */
    PQ_MNG_VpssZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    /* Hor */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 );

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL );
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC );
    return;
}

/* add from 3798cv100 VDP *//* Separated HL8 HC8 VL6 VC6 *//* 98CV200_A */
HI_VOID PQ_MNG_VdpZmeHQSetSptAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    PQ_MNG_VdpZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    /* Hor */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 );

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL );
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC );

    return;
}

/* add from HiFoneB2 VPSS *//* Separated HL8 HC8 VL6 VC6 *//* 98CV200_A */
HI_VOID PQ_MNG_VpssZmeSetSptAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    /* fit refer to input H and V */
    PQ_MNG_VpssZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    /* Hor */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 );

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL );
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC );

    return;
}

/* Separated HL8 HC4 VL6 VC4 *//* HL8 HC4 VL6 VC4 in 3716mv410 vdp *//* 98CV200_A And MV410 */
HI_VOID PQ_MNG_VdpZmeHQSetAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    PQ_MNG_VdpZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* config zme coefficient address */
    /* Hor 20 bits precision in logic but 12 bits precision in calculation */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 );
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 );

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLFilterCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL );
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC );

    return;
}

/* add from Hi3716mv410 VPSS *//* Separated HL8 HC4 VL4 VC4 *//* MV410 */
HI_VOID PQ_MNG_VpssZmeSQSetSptAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    PQ_MNG_VpssZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);/* fit refer to input H and V */

    /* config zme coefficient address */
    /* Hor */
    pstZmeRtlPara->u32ZmeCoefAddrHL = GetHLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHL >> 8 ); /* HL8 */
    pstZmeRtlPara->u32ZmeCoefAddrHC = GetHCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioHC >> 8 ); /* HC4 */

    /* Vert */
    pstZmeRtlPara->u32ZmeCoefAddrVL = GetVLfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVL ); /* VL4 */
    pstZmeRtlPara->u32ZmeCoefAddrVC = GetVCfirCoef( &(pstMem->stZmeCoefAddr), pstZmeRtlPara->u32ZmeRatioVC ); /* VC4 */

    return;
}

/* add from HiFoneB2 SR *//* Separated HL8 HC8 VL6 VC6 */ /* 98CV200_A And 98M */
HI_VOID PQ_MNG_VdpSRZmeSetAddr(ALG_VZME_MEM_S* pstMem, ALG_VZME_DRV_PARA_S* pstZmeDrvPara, ALG_VZME_RTL_PARA_S* pstZmeRtlPara)
{
    PQ_MNG_VdpZmeComnSet(pstZmeDrvPara, pstZmeRtlPara);

    /* Set SR zme coefficient */
    /* Hor and Vert */

    return;
}

HI_S32 PQ_MNG_InitVdpZme(HI_VOID)
{
    HI_S32 s32Ret;

    if (g_bVdpZmeInitFlag)
    {
        return HI_SUCCESS;
    }

    s32Ret = PQ_HAL_MMZ_AllocAndMap("PQ_VdpZmeCoef", HI_NULL, VDP_MMZ_SIZE, 16, &g_stVZMEVdpInstance.stMBuf);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_PQ("Get PQ_Zme failed\n");
        return HI_FAILURE;
    }

    /* load Vdp zme coef; Init Vdp Zme Coef */
    s32Ret = PQ_MNG_VdpZmeComnInit(&g_stVZMEVdpInstance);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_PQ("PQ_MNG_VdpZmeComnInit failed!\n");
        PQ_HAL_MMZ_UnmapAndRelease(&g_stVZMEVdpInstance.stMBuf);
        g_stVZMEVdpInstance.stMBuf.pu8StartVirAddr = HI_NULL;

        return HI_FAILURE;
    }

    g_bVdpZmeInitFlag = HI_TRUE;

    return HI_SUCCESS;
}

static HI_S32 PQ_MNG_InitVpssZme(HI_VOID)
{
    PQ_MMZ_BUF_S* pstMBuf;
    HI_S32 s32Ret;

    if (g_bVpssZmeInitFlag)
    {
        return HI_SUCCESS;
    }

    /* apply memory for zme coefficient, and get the address */
    pstMBuf = &(g_stVZMEVpssInstance.stMBuf);

    s32Ret = PQ_HAL_MMZ_AllocAndMap("PQ_VpssZmeCoef", HI_NULL, VPSS_MMZ_SIZE, 0, pstMBuf);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_PQ("PQ_MNG_MMZ_AllocAndMap failed!\n");
        return HI_FAILURE;
    }
    /* load Vpss zme coef; Init Vdp Zme Coef */
    s32Ret = PQ_MNG_VpssZmeComnInit(&g_stVZMEVpssInstance);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_PQ("PQ_MNG_VpssZmeComnInit failed!\n");
        PQ_HAL_MMZ_UnmapAndRelease(pstMBuf);
        //g_stVZMEVpssInstance.stMBuf.u32StartVirAddr = 0;

        return HI_FAILURE;
    }
    g_bVpssZmeInitFlag = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_DeInitVdpZme(HI_VOID)
{
    if (g_bVdpZmeInitFlag)
    {
        PQ_MNG_VdpZmeComnDeInit(&g_stVZMEVdpInstance);
        /* release Vdp zme coefficient memory */
        if (HI_NULL != g_stVZMEVdpInstance.stMBuf.pu8StartVirAddr)
        {
            PQ_HAL_MMZ_UnmapAndRelease(&g_stVZMEVdpInstance.stMBuf);
            g_stVZMEVdpInstance.stMBuf.pu8StartVirAddr = HI_NULL;
        }
        g_bVdpZmeInitFlag = HI_FALSE;
    }

    return HI_SUCCESS;
}

static HI_S32 PQ_MNG_DeInitVpssZme(HI_VOID)
{
    if (g_bVpssZmeInitFlag)
    {
        PQ_MNG_VpssZmeComnDeInit(&g_stVZMEVpssInstance);
        /* release Vpss zme coefficient memory */
        if (HI_NULL != g_stVZMEVpssInstance.stMBuf.pu8StartVirAddr)
        {
            PQ_HAL_MMZ_UnmapAndRelease(&g_stVZMEVpssInstance.stMBuf);
            g_stVZMEVpssInstance.stMBuf.pu8StartVirAddr = HI_NULL;
        }
        g_bVpssZmeInitFlag = HI_FALSE;
    }

    return HI_SUCCESS;
}


HI_S32 PQ_MNG_InitZme(PQ_PARAM_S* pstPqParam, HI_BOOL bDefault)
{
    if (g_bZmeInitFlag)
    {
        return HI_SUCCESS;
    }

    if (HI_NULL == pstPqParam)
    {
        return HI_FAILURE;
    }

    (HI_VOID)PQ_MNG_InitVpssZme();
    (HI_VOID)PQ_MNG_InitVdpZme();

    g_bZmeInitFlag = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_DeInitZme(HI_VOID)
{
    if (g_bZmeInitFlag)
    {
        (HI_VOID)PQ_MNG_DeInitVpssZme();
        (HI_VOID)PQ_MNG_DeInitVdpZme();

        g_bZmeInitFlag = HI_FALSE;
    }

    return HI_SUCCESS;
}

ZME_VDP_FUNC_S stZmeVdpFunc[HI_PQ_DISP_LAYER_ZME_BUTT]  =
{
#if defined(CHIP_TYPE_hi3798mv100) || defined(CHIP_TYPE_hi3796mv100)
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V0  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V1  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V2  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V3  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V4  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_WbcZmeRegCfg},   /* WBC */
    {PQ_MNG_VdpSRZmeSetAddr,    PQ_HAL_SRZmeRegCfg },   /* SR  */
#elif defined(CHIP_TYPE_hi3798cv200_a)||defined(CHIP_TYPE_hi3798cv200_b)||defined(CHIP_TYPE_hi3798cv200)
    {PQ_MNG_VdpZmeHQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V0  */
    {PQ_MNG_VdpZmeHQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V1  */
    {PQ_MNG_VdpZmeHQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V2  */
    {PQ_MNG_VdpZmeHQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V3  */
    {PQ_MNG_VdpZmeHQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V4  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_WbcZmeRegCfg},   /* WBC */
    {HI_NULL, HI_NULL},
#elif defined(CHIP_TYPE_hi3716mv410)||defined(CHIP_TYPE_hi3716mv420)
    {PQ_MNG_VdpZmeHQSetAddr,    PQ_HAL_VdpZmeRegCfg},   /* V0  */
    {PQ_MNG_VdpZmeHQSetAddr,    PQ_HAL_VdpZmeRegCfg},   /* V1  */
    {PQ_MNG_VdpZmeHQSetAddr,    PQ_HAL_VdpZmeRegCfg},   /* V2  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V3  */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* V4  *//* It does not exist V4 in mv410 */
    {PQ_MNG_VdpZmeSQSetSptAddr, PQ_HAL_VdpZmeRegCfg},   /* WBC *//* WBC is same to V3 */
    {HI_NULL, HI_NULL},                                 /* SR  */
#else
    {HI_NULL, HI_NULL},                                 /* V0  */
    {HI_NULL, HI_NULL},                                 /* V1  */
    {HI_NULL, HI_NULL},                                 /* V2  */
    {HI_NULL, HI_NULL},                                 /* V3  */
    {HI_NULL, HI_NULL},                                 /* V4  */
    {HI_NULL, HI_NULL},                                 /* WBC */
    {HI_NULL, HI_NULL},                                 /* SR  */
#endif
};

ZME_VPSS_FUNC_S stZmeVpssFunc[HI_PQ_VPSS_LAYER_ZME_BUTT]  =
{
#if defined(CHIP_TYPE_hi3798mv100) || defined(CHIP_TYPE_hi3796mv100)
    {PQ_MNG_VpssZmeHQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT0 */
    {PQ_MNG_VpssZmeHQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT1 */
    {PQ_MNG_VpssZmeHQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT2 */
    {PQ_MNG_VpssZmeHQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* HSCL  */
#elif defined(CHIP_TYPE_hi3798cv200_a)||defined(CHIP_TYPE_hi3798cv200_b)
    {PQ_MNG_VpssZmeSetSptAddr,   PQ_HAL_VpssZmeRegCfg}, /* PORT0 */
    {PQ_MNG_VpssZmeSetSptAddr,   PQ_HAL_VpssZmeRegCfg}, /* PORT1 */
    {PQ_MNG_VpssZmeSetSptAddr,   PQ_HAL_VpssZmeRegCfg}, /* PORT2 */
    {HI_NULL, HI_NULL},                                 /* HSCL  */
#elif defined(CHIP_TYPE_hi3716mv410)||defined(CHIP_TYPE_hi3716mv420)||defined(CHIP_TYPE_hi3798cv200)
    {PQ_MNG_VpssZmeSQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT0 */
    {PQ_MNG_VpssZmeSQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT0 */
    {PQ_MNG_VpssZmeSQSetSptAddr, PQ_HAL_VpssZmeRegCfg}, /* PORT0 */
    {HI_NULL, HI_NULL},                                 /* HSCL  */
#else
    {HI_NULL, HI_NULL},                                 /* PORT0 */
    {HI_NULL, HI_NULL},                                 /* PORT1 */
    {HI_NULL, HI_NULL},                                 /* PORT2 */
    {HI_NULL, HI_NULL},                                 /* HSCL  */
#endif
};

HI_S32 PQ_MNG_SetVdpZme(HI_PQ_ZME_LAYER_E enLayerId, HI_PQ_ZME_PARA_IN_S* pstZmePara, HI_BOOL  bFirEnable)
{
    ALG_VZME_RTL_PARA_S stZmeRtlParam;

    memset((void*)&stZmeRtlParam, 0, sizeof(ALG_VZME_RTL_PARA_S));
    stZmeVdpFunc[enLayerId].pfnZme_CoefCalculate(&g_stVZMEVdpInstance, (ALG_VZME_DRV_PARA_S*)pstZmePara, &stZmeRtlParam);
    stZmeVdpFunc[enLayerId].pfnZme_VdpRegCfg(enLayerId, &stZmeRtlParam, bFirEnable);

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_SetVpssZme(HI_PQ_VPSS_ZME_LAYER_E enLayerId, S_CAS_REGS_TYPE* pstReg, HI_PQ_ZME_PARA_IN_S* pstZmePara, HI_BOOL  bFirEnable)
{
    ALG_VZME_RTL_PARA_S stZmeRtlParam;

    memset((void*)&stZmeRtlParam, 0, sizeof(ALG_VZME_RTL_PARA_S));
    stZmeVpssFunc[enLayerId].pfnZme_CoefCalculate(&g_stVZMEVpssInstance, (ALG_VZME_DRV_PARA_S*)pstZmePara, &stZmeRtlParam);
    stZmeVpssFunc[enLayerId].pfnZme_VpssRegCfg(enLayerId, pstReg, &stZmeRtlParam, bFirEnable);

    return HI_SUCCESS;
}

/* VPSS ZME二级缩放校验
   u32InWitdh: 输入宽; u32InHeigh: 输入高; stZmeWin: 各层输出宽高;
   u32OutWitdh: 输出宽; u32OutHeigh: 输出高;
   HI_SUCCESS : 需要二级缩放; HI_FAILURE : 不需要二级缩放
   */
HI_S32 PQ_MNG_ZME_2L_Check(HI_U32 u32InWitdh, HI_U32 u32InHeigh, HI_PQ_ZME_WIN_S stZmeWin, HI_U32* pu32OutWitdh, HI_U32* pu32OutHeigh)
{
    HI_U32 u32Port0WitdhNum, u32Port0HeighNum, u32Port1WitdhNum, u32Port1HeighNum;

    if ((0 == stZmeWin.stPort0Win.u32Width * stZmeWin.stPort0Win.u32Height)
        || (0 == stZmeWin.stPort1Win.u32Width * stZmeWin.stPort1Win.u32Height))
    {
        return HI_FAILURE;
    }

    u32Port0WitdhNum = u32InWitdh / stZmeWin.stPort0Win.u32Width;
    u32Port0HeighNum = u32InHeigh / stZmeWin.stPort0Win.u32Height;
    u32Port1WitdhNum = u32InWitdh / stZmeWin.stPort1Win.u32Width;
    u32Port1HeighNum = u32InHeigh / stZmeWin.stPort1Win.u32Height;

    if ((u32Port0WitdhNum >= sg_u32Zme2LThl) && (u32Port1WitdhNum >= sg_u32Zme2LThl)
        && (u32Port0HeighNum >= sg_u32Zme2LThl) && (u32Port1HeighNum >= sg_u32Zme2LThl)) /*HD和SD需要2级缩放*/
    {
        *pu32OutWitdh = u32InWitdh / 2;
        *pu32OutHeigh = u32InHeigh / 2;

        pqprint(PQ_PRN_ZME, "Port0 set 2L ZME, Port1 set 2L ZME\n");
    }
    else if (((u32Port0WitdhNum < sg_u32Zme2LThl) || (u32Port0HeighNum < sg_u32Zme2LThl))
             && (u32Port1WitdhNum >= sg_u32Zme2LThl) && (u32Port0HeighNum >= sg_u32Zme2LThl)) /*SD需要2级缩放,HD不需要2级缩放*/
    {
        *pu32OutWitdh = stZmeWin.stPort0Win.u32Width;
        *pu32OutHeigh = stZmeWin.stPort0Win.u32Height;

        pqprint(PQ_PRN_ZME, "Port0 not set 2L ZME, Port1 set 2L ZME\n");
    }
    else if (((u32Port0WitdhNum < sg_u32Zme2LThl) || (u32Port0HeighNum < sg_u32Zme2LThl))
             && ((u32Port1WitdhNum < sg_u32Zme2LThl) || (u32Port1HeighNum < sg_u32Zme2LThl))) /*HD和SD都不需要2级缩放*/
    {
        *pu32OutWitdh = 0;
        *pu32OutHeigh = 0;

        pqprint(PQ_PRN_ZME, "Port0 not set 2L ZME, Port1 not set 2L ZME\n");
    }
    else /*其他情况,HD和SD都不需要2级缩放*/
    {
        *pu32OutWitdh = 0;
        *pu32OutHeigh = 0;

        pqprint(PQ_PRN_ZME, "not set 2L ZME,Port0 Witdh:%d, Port0 Heigh:%d, Port1 Witdh:%d, Port1 Heigh:%d\n", \
                u32Port0WitdhNum, u32Port0HeighNum, u32Port1WitdhNum, u32Port1HeighNum);
    }

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_GetZmeCoef(HI_PQ_ZME_COEF_RATIO_E enCoefRatio, HI_PQ_ZME_COEF_TYPE_E enCoefType, HI_PQ_ZME_TAP_E enZmeTap, HI_S16* ps16Coef)
{
    HI_S32 s32Ret;

    s32Ret = PQ_HAL_GetZmeCoef((PQ_HAL_ZME_COEF_RATIO_E)enCoefRatio, (PQ_HAL_ZME_COEF_TYPE_E)enCoefType, (PQ_HAL_ZME_TAP_E)enZmeTap, ps16Coef);

    return s32Ret;
}

HI_S32 PQ_MNG_GetVdpZmeStrategy (HI_PQ_ZME_LAYER_E enLayerId, HI_PQ_ZME_STRATEGY_IN_U* pstZmeIn, HI_PQ_ZME_STRATEGY_OUT_U* pstZmeOut)
{
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = PQ_HAL_GetVdpZmeStrategy ((VDP_ZME_LAYER_E)enLayerId, pstZmeIn, pstZmeOut);

    return s32Ret;
}

HI_S32 PQ_MNG_SetZmeTunMode(PQ_TUN_MODE_E enZMETunMode)
{
    sg_enZMETunMode = enZMETunMode;

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_GetZmeTunMode(PQ_TUN_MODE_E* enZMETunMode)
{
    *enZMETunMode = sg_enZMETunMode;

    return HI_SUCCESS;
}

HI_S32 PQ_MNG_SetZmeDefault(HI_BOOL bOnOff)
{
    if (PQ_TUN_NORMAL == sg_enZMETunMode)
    {
        PQ_HAL_SetZmeDefault(bOnOff);
    }
    else
    {
        PQ_HAL_SetZmeDefault(HI_FALSE);/* When Debug rwzb no using */
    }

    return HI_SUCCESS;
}


static stPQAlgFuncs stZMEFuncs =
{
    .Init               = PQ_MNG_InitZme,
    .DeInit             = PQ_MNG_DeInitZme,
    .SetVdpZme          = PQ_MNG_SetVdpZme,
    .SetVpssZme         = PQ_MNG_SetVpssZme,
    .CheckZme2L         = PQ_MNG_ZME_2L_Check,
    .SetZmeEnMode       = PQ_MNG_SetZmeEnMode,
    .GetZmeEnMode       = PQ_MNG_GetZmeEnMode,
    .GetZmeCoef         = PQ_MNG_GetZmeCoef,
    .GetVdpZmeStrategy  = PQ_MNG_GetVdpZmeStrategy,
    .SetZmeDefault      = PQ_MNG_SetZmeDefault,
    .GetTunMode         = PQ_MNG_GetZmeTunMode,
    .SetTunMode         = PQ_MNG_SetZmeTunMode,
};

HI_S32 PQ_MNG_RegisterZme(PQ_REG_TYPE_E  enType)
{
    HI_S32 s32Ret;

    s32Ret = PQ_COMM_AlgRegister(HI_PQ_MODULE_ZME, enType, PQ_BIN_ADAPT_SINGLE, "zme", HI_NULL, &stZMEFuncs);

    return s32Ret;
}

HI_S32 PQ_MNG_UnRegisterZme()
{
    HI_S32 s32Ret;

    s32Ret = PQ_COMM_AlgUnRegister(HI_PQ_MODULE_ZME);

    return s32Ret;
}


