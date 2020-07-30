/******************************************************************************
*
* Copyright (C) 2015 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
* and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
******************************************************************************
File Name           : hifb_logo.c
Version             : Initial Draft
Author              : 
Created             : 2014/09/09
Description         : 
Function List       : 
History             :
Date                       Author                   Modification
2015/05/19                 y00181162                Created file        
******************************************************************************/

#ifdef CFG_HIFB_LOGO_SUPPORT

/*********************************add include here******************************/

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/fb.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <asm/types.h>
#include <asm/stat.h>
#include <asm/fcntl.h>
#include <linux/sched.h>

#include "hifb_drv.h"
#include "hifb_comm.h"
#include "hifb_logo.h"
#include "hifb_wbc.h"
#include "hifb_p.h"
#include "hifb_config.h"

#include "drv_pdm_ext.h"

/***************************** Macro Definition ******************************/


/*************************** Structure Definition ****************************/


/********************** Global Variable declaration **************************/
static HIFB_LOGO_S gs_stLogo;
extern HIFB_LAYER_S s_stLayer[HIFB_MAX_LAYER_NUM];

#ifndef CONFIG_HIFB_CRC_LOGO_TO_APP
extern OPTM_GFX_WBC_S  g_stGfxWbc2;
#endif
/******************************* API declaration *****************************/

/***************************************************************************************
* func          : hifb_freelogomem
* description   : free logo mem CNcomment
                  CNcomment: ��logo�ڴ棬ж��������ʱ����� CNend\n
* param[in]     : enLogoChn
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_VOID hifb_freelogomem(HIFB_LOGO_CHANNEL_E enLogoChn)
{

    PDM_EXPORT_FUNC_S *ps_PdmExportFuncs = HI_NULL;

    if (HI_SUCCESS != HI_DRV_MODULE_GetFunction(HI_ID_PDM, (HI_VOID**)&ps_PdmExportFuncs)){
        return;
    }

    if(HI_NULL == ps_PdmExportFuncs){
        return;
    }

	/**
	 ** msleep 80ms to asure wbc closed or screen addr switched
	 ** max time or each fps is 1000 / 24 = 42ms
	 **/
	msleep(80);

	/**
	 **android���ٿ���������²���Ҫ�ͷţ������޷����¿���
	 **/
    if (HIFB_LOGO_CHN_HD == enLogoChn){
		#if !defined(CONFIG_PM_HIBERNATE) && !defined(CONFIG_HISI_SNAPSHOT_BOOT)
        	ps_PdmExportFuncs->pfnPDM_ReleaseReserveMem(DISPLAY_BUFFER_HD);
        	ps_PdmExportFuncs->pfnPDM_ReleaseReserveMem(OPTM_GFX_WBC2_BUFFER);
		#endif
    }else{
		#if !defined(CONFIG_PM_HIBERNATE) && !defined(CONFIG_HISI_SNAPSHOT_BOOT)
        	ps_PdmExportFuncs->pfnPDM_ReleaseReserveMem(DISPLAY_BUFFER_SD);
		#endif
    }

	gs_stLogo.u32LogoNum--;

	/**hd and sd use the same zme buffer, so disable hd and sd all,release this buffer
	 **����ͱ��干��һ��ϵ�������Եȸ߱��嶼û��logo��������ͷ�
	 **/
    if (0 == gs_stLogo.u32LogoNum){
		#if !defined(CONFIG_PM_HIBERNATE) && !defined(CONFIG_HISI_SNAPSHOT_BOOT)
        	ps_PdmExportFuncs->pfnPDM_ReleaseReserveMem(HIFB_ZME_COEF_BUFFER);
		#endif
    }

    return;
}


/***************************************************************************************
* func          : hifb_freelogomem_work
* description   : free logo mem CNcomment
                             CNcomment: ��logo�ڴ棬����logo���ɵ�ʱ����� CNend\n
* param[in]     : enLogoChn  CNcomment: �������� CNend\n
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_VOID hifb_freelogomem_work(struct work_struct *work)
{
	HIFB_LOGO_INFO_S *pstLogoInfo  = NULL;
	HIFB_LOGO_CHANNEL_E enLogoChn  = HIFB_LOGO_CHN_BUTT;

	pstLogoInfo = (HIFB_LOGO_INFO_S *)container_of(work, HIFB_LOGO_INFO_S, freeLogoMemWork); 

	if (IS_HD_LAYER(pstLogoInfo->eLogoID) || IS_MINOR_HD_LAYER(pstLogoInfo->eLogoID)){
		enLogoChn = HIFB_LOGO_CHN_HD;
	}else{
		enLogoChn = HIFB_LOGO_CHN_SD;
	}	

	hifb_freelogomem(enLogoChn);
	
    return;
}

/***************************************************************************************
* func          : hifb_logo_setstate
* description   : CNcomment: set state when insmod ko CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID hifb_logo_setstate(HI_VOID)
{
	memset(&gs_stLogo,0,sizeof(HIFB_LOGO_S));
	gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].eLogoID = HIFB_HD_LOGO_LAYER_ID;
	gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].eLogoID = HIFB_SD_LOGO_LAYER_ID;
}


#ifdef CONFIG_HIFB_CRC_LOGO_TO_APP
/***************************************************************************************
* func          : hifb_logo_hd
* description   : CNcomment: ����ͨ��logo���� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_VOID hifb_logo_crc(HI_S32 s32LayerId)
{
	HIFB_PAR_S *par      = NULL;
	struct fb_info *info = NULL;
	HI_S32 s32Row = 0;
	HI_S32 s32Col = 0;
	HI_CHAR* pSrcBuf = NULL;
	info    = s_stLayer[s32LayerId].pstInfo;
	par     = (HIFB_PAR_S *)(info->par);
    pSrcBuf = (HI_CHAR*)info->screen_base;
    if(NULL == pSrcBuf){
		return;
    }
    for(s32Row = 0; s32Row < par->stExtendInfo.u32DisplayHeight; s32Row++){
		for(s32Col = 0; s32Col < par->stExtendInfo.u32DisplayWidth; s32Col++){
			if(0 != pSrcBuf[s32Col + s32Row * info->fix.line_length]){
				gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].bClean = HI_TRUE;
				return;
			}
		}
    }
	return;
}

/***************************************************************************************
* func          : hifb_logo_hd
* description   : CNcomment: ����ͨ��logo���� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_S32 hifb_logo_hd(HI_VOID *pData)
{
	HIFB_LAYER_ID_E eLogoID = gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].eLogoID;
	while(1){
		 hifb_logo_crc(eLogoID);
		 if(HI_TRUE == gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].bClean){
		     hifb_clear_logo(eLogoID,HI_FALSE);
		     gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].bClean = HI_FALSE;
		     return HI_SUCCESS;
		 }
		 /** waite 2ms, release cpu **/
		 schedule_timeout(HZ / 500);
	}
	return HI_SUCCESS;
}

/***************************************************************************************
* func          : hifb_logo_sd
* description   : CNcomment: ����ͨ��logo���� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_S32 hifb_logo_sd(HI_VOID *pData)
{
	HIFB_LAYER_ID_E eLogoID = gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].eLogoID;
	while(1){
		 hifb_logo_crc(eLogoID);
		 if(HI_TRUE == gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].bClean){
		     hifb_clear_logo(eLogoID,HI_FALSE);
		     gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].bClean = HI_FALSE;
		     return HI_SUCCESS;
		 }
		 /** waite 2ms, release cpu **/
		 schedule_timeout(HZ / 500);
		 #if 0
		 set_current_state(TASK_UNINTERRUPTIBLE);
         if(kthread_should_stop()){
         	break;
         }
         #endif
	}
	return HI_SUCCESS;
}
#endif

/***************************************************************************************
* func          : hifb_logo_init
* description   : CNcomment: logo��ʼ��������ֻ������һ������Ͷ�Ӧ��ͼ��򿪱�� CNend\n
* param[in]     : HI_VOID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID hifb_logo_init(HI_U32 u32LayerId)
{
	HIFB_LOGO_CHANNEL_E enLogoChn = HIFB_LOGO_CHN_HD;
    HIFB_OSD_DATA_S pstLogoLayerData; 

	/**
	 ** check whether logo init
	 ** �ж�logo�Ƿ��Ѿ���ʼ��
	 **/
	if(HI_TRUE == gs_stLogo.bLogoInit){
		return;
	}
	gs_stLogo.bLogoInit = HI_TRUE;

	if(HIFB_HD_LOGO_LAYER_ID != u32LayerId){
		/** not need logo to app
		 **����Ҫƽ������,ֻ��ʹ��HD0����ܹ�ʵ��ƽ������
		 **/
		return;
	}
	gs_stLogo.u32LogoNum = 0;
	memset(&pstLogoLayerData,0,sizeof(HIFB_OSD_DATA_S));
	
#ifdef CONFIG_HIFB_GFX3_TO_GFX5
	/**
	 ** only at two display should check two display channel
	 ** ˫�Ե�����¿���logo������
	 **/
	for (enLogoChn = 0; enLogoChn < HIFB_LOGO_CHN_BUTT; enLogoChn++){
#endif
		/**
		 ** judge whether has logo
		 ** ����ֻ���ȡͼ���Ƿ�ʹ��
		 **/
		s_stDrvOps.HIFB_DRV_GetOSDData(gs_stLogo.stLogoInfo[enLogoChn].eLogoID, &pstLogoLayerData);
		
		if(pstLogoLayerData.eState == HIFB_LAYER_STATE_ENABLE){
			/**
			 ** only support two display can run this two times, Hi3716CV200 support it
			 ** ֻ�����п���logo������²��ܽ���,����˫�Ե������������
			 **/
	    	gs_stLogo.u32LogoNum++;
	        gs_stLogo.stLogoInfo[enLogoChn].bShow        = HI_TRUE;
	        gs_stLogo.stLogoInfo[enLogoChn].u32LogoAddr  = pstLogoLayerData.u32RegPhyAddr;
			/**
			 ** the logo has not transitioned,so the updata the G0 register��
			 ** ����logoû�й����꣬������ͼ��Ĵ�����ʱ���ܸ���,��������������
			 **/
			s_stDrvOps.HIFB_DRV_SetLayerMaskFlag(gs_stLogo.stLogoInfo[enLogoChn].eLogoID, HI_TRUE);

			/**ͬԴ��д�����**/
			s_stDrvOps.HIFB_DRV_GetOSDData(gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].eLogoID, &pstLogoLayerData);	
			if(pstLogoLayerData.eState == HIFB_LAYER_STATE_ENABLE){
				gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].u32LogoSdAddr = pstLogoLayerData.u32RegPhyAddr;
			}
	    }

	#ifdef CONFIG_HIFB_CRC_LOGO_TO_APP
		if(HIFB_LOGO_CHN_HD == enLogoChn){
			/** ��ʾͨ��1�߳� **/
			gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].pThreadTask = kthread_create(hifb_logo_hd, &gs_stLogo, "pThtreadHDLogo");
			if(IS_ERR(gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].pThreadTask)){
				gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].pThreadTask = NULL;
				return;
			}
			wake_up_process(gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_HD].pThreadTask);
		}else{
			/** ��ʾͨ�����߳�**/
			gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].pThreadTask = kthread_create(hifb_logo_sd, &gs_stLogo, "pThtreadHDLogo");
			if(IS_ERR(gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].pThreadTask)){
				gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].pThreadTask = NULL;
				return;
			}
			wake_up_process(gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].pThreadTask);
		}
	#endif
#ifdef CONFIG_HIFB_GFX3_TO_GFX5
	}
#endif
    return;
}


/***************************************************************************************
* func          : hifb_clear_logo
* description   : clear logo   CNcomment: ��logo CNend\n
* param[in]     : u32LayerID
* param[in]     : bModExit
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID hifb_clear_logo(HI_U32 u32LayerID, HI_BOOL bModExit)
{

	HIFB_LAYER_ID_E enLogoLayerId = HIFB_LAYER_HD_0;
	HIFB_LOGO_CHANNEL_E enLogoChn = HIFB_LOGO_CHN_HD;

    if(HIFB_HD_LOGO_LAYER_ID != u32LayerID){
		return;
	}
    
	if (IS_HD_LAYER(u32LayerID) || IS_MINOR_HD_LAYER(u32LayerID)){
		enLogoChn = HIFB_LOGO_CHN_HD;
	}else{
		enLogoChn = HIFB_LOGO_CHN_SD;
	}

#ifdef CONFIG_HIFB_GFX3_TO_GFX5
	/**
	 ** only at two display should check two display channel
	 ** ˫�Ե�����¿���logo������
	 **/
	for (enLogoChn = 0; enLogoChn < HIFB_LOGO_CHN_BUTT; enLogoChn++){
#endif

		if(!gs_stLogo.stLogoInfo[enLogoChn].bShow){
			return;
		}
		enLogoLayerId = gs_stLogo.stLogoInfo[enLogoChn].eLogoID;

		s_stDrvOps.HIFB_DRV_SetLayerMaskFlag(enLogoLayerId, HI_FALSE);
	    s_stDrvOps.HIFB_DRV_ClearLogo(enLogoLayerId);
	    s_stDrvOps.HIFB_DRV_UpdataLayerReg(enLogoLayerId);
	    /**
	     ** wait for logo closed ,so we can free logo buffer
	     **/
	    if (bModExit){
			hifb_freelogomem(enLogoChn);
		}else{
			INIT_WORK(&(gs_stLogo.stLogoInfo[enLogoChn].freeLogoMemWork), hifb_freelogomem_work);
	    	schedule_work(&(gs_stLogo.stLogoInfo[enLogoChn].freeLogoMemWork));    
		}
		
	    gs_stLogo.stLogoInfo[enLogoChn].bShow = HI_FALSE;

#ifdef CONFIG_HIFB_GFX3_TO_GFX5
	}
#endif
}



#ifndef CONFIG_HIFB_CRC_LOGO_TO_APP
/***************************************************************************************
* func          : hifb_convertbootfmt2fbfmt
* description   : CNcomment: ��boot�����ظ�ʽת��HiFB�����ظ�ʽ CNend\n
* param[in]     : enBootFmt
* retval        : NA
* others:       : NA
***************************************************************************************/
static HIFB_COLOR_FMT_E hifb_convertbootfmt2fbfmt(HI_U32 enBootFmt)
{
	switch(enBootFmt){
		case HIFB_BOOT_FMT_1555:
			return HIFB_FMT_ARGB1555;
		case HIFB_BOOT_FMT_8888:
			return HIFB_FMT_ARGB8888;
		default:
			return HIFB_FMT_BUTT;        
	}
}
/***************************************************************************************
* func          : hifb_convertlogochn2dispchn
* description   : CNcomment: ��logoͨ��ת��dispͨ�� CNend\n
* param[in]     : enLogoChn
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_UNF_DISP_E hifb_convertlogochn2dispchn(HIFB_LOGO_CHANNEL_E enLogoChn)
{
	if (enLogoChn >= HIFB_LOGO_CHN_BUTT){
		return HI_UNF_DISPLAY_BUTT;
	}
	switch(enLogoChn){
		case HIFB_LOGO_CHN_HD:
			return HI_UNF_DISPLAY1;
		case HIFB_LOGO_CHN_SD:
			return HI_UNF_DISPLAY0;
		default:
			return HI_UNF_DISPLAY_BUTT;        
	}
}

/***************************************************************************************
* func          : hifb_set_logosd
* description   : set logo osd 
                  CNcomment: ����logo������Ϣ,Ҫ������ʹ��buffer�л�����
                             buffer�����ݵ�ʱ���е�ַ��ʹ���û�̬����Ϣ��
                             ����Ͳ���Ҫ����logo��Ϣ�� CNend\n
* param[in]     : u32LayerID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_S32 hifb_set_logosd(HI_U32 u32LayerID)
{

	HI_S32 s32Ret        = HI_SUCCESS;     	
    HIFB_PAR_S *par      = NULL;
	struct fb_info *info = NULL;
	HIFB_LOGO_CHANNEL_E enLogoChn  = HIFB_LOGO_CHN_BUTT;
	HIFB_RECT         stInRect     = {0};
	HIFB_COLOR_FMT_E  enHifbFmt    = HIFB_FMT_BUTT;
	PDM_EXPORT_FUNC_S *pstPdmFuncs = NULL;	
    HI_DISP_PARAM_S   stDispParam;
    HI_U32 u32PixDepth = 0;
    HI_U32 u32Stride   = 0;

    if(HIFB_HD_LOGO_LAYER_ID != u32LayerID){
		return HI_SUCCESS;
	}
    
	if (IS_HD_LAYER(u32LayerID) || IS_MINOR_HD_LAYER(u32LayerID)){
		enLogoChn = HIFB_LOGO_CHN_HD;
	}else if(IS_SD_LAYER(u32LayerID) || IS_MINOR_SD_LAYER(u32LayerID)){
		enLogoChn = HIFB_LOGO_CHN_SD;
	}else{
		return HI_SUCCESS;
	}
	if (!gs_stLogo.stLogoInfo[enLogoChn].bShow){
	    /**not have logo
	     ** û�п���logo�ˣ���android��Ҫ����base������������ݣ��������ﲻ��ֱ�ӷ���
	     **/
		//return HI_SUCCESS;
	}
    
	memset(&stDispParam,0,sizeof(HI_DISP_PARAM_S));
	
    s32Ret = HI_DRV_MODULE_GetFunction(HI_ID_PDM, (HI_VOID**)&pstPdmFuncs);
    if(HI_FAILURE == s32Ret || NULL == pstPdmFuncs || NULL == pstPdmFuncs->pfnPDM_GetDispParam){
        HIFB_WARNING("get pdm module function failed\r\n");
		return HI_FAILURE;
    }
    
	s32Ret = pstPdmFuncs->pfnPDM_GetDispParam(hifb_convertlogochn2dispchn(enLogoChn), &stDispParam);
	if (HI_FAILURE == s32Ret){
        HIFB_WARNING("PDM_GetDispParam failed\r\n");
		return HI_FAILURE;
    }

	if(0 == stDispParam.u32VirtScreenWidth || 0 == stDispParam.u32VirtScreenHeight){
		HIFB_ERROR("logo virtual screen error\n");
		return HI_FAILURE;
	}
	
	enHifbFmt = hifb_convertbootfmt2fbfmt((HI_U32)stDispParam.enPixelFormat);
	if (enHifbFmt >= HIFB_FMT_PUYVY){
		HIFB_ERROR("unsupported fmt received from boot!\n");
		return HI_FAILURE;
	}

	info   = s_stLayer[u32LayerID].pstInfo;
	par    = (HIFB_PAR_S *)(info->par);
	/**
	 **get pixel depth
	 **/
	info->var.bits_per_pixel = hifb_getbppbyfmt(enHifbFmt);
	if(0 == info->var.bits_per_pixel){
		HIFB_WARNING("unsupported fmt received from boot!\n");
		return HI_FAILURE;
	}

	info->var.red    = s_stArgbBitField[enHifbFmt].stRed;
    info->var.green  = s_stArgbBitField[enHifbFmt].stGreen;
    info->var.blue   = s_stArgbBitField[enHifbFmt].stBlue;
    info->var.transp = s_stArgbBitField[enHifbFmt].stTransp;

	/**
	 **the layer input size is equal to virscreen size
	 **ͼ��������Сʼ�պ�������Ļ��С����һ�µ�
	 **/
	info->var.xres         = stDispParam.u32VirtScreenWidth;
	info->var.yres         = stDispParam.u32VirtScreenHeight;
	info->var.xres_virtual = info->var.xres;
	info->var.yres_virtual = info->var.yres;

    u32PixDepth = info->var.bits_per_pixel >> 3;
	HI_HIFB_GetStride(info->var.xres_virtual,u32PixDepth,&u32Stride,CONFIG_HIFB_STRIDE_16ALIGN);
	info->fix.line_length  = u32Stride;
    u32Stride = (info->var.xres_virtual * u32PixDepth + CONFIG_HIFB_STRIDE_16ALIGN - 1) & (~(CONFIG_HIFB_STRIDE_16ALIGN - 1));
    
    par->stExtendInfo.enColFmt         = enHifbFmt;
	par->stExtendInfo.stPos.s32XPos    = 0;
	par->stExtendInfo.stPos.s32YPos    = 0;
	par->stExtendInfo.u32DisplayWidth  = info->var.xres;
	par->stExtendInfo.u32DisplayHeight = info->var.yres;

	stInRect.x = par->stExtendInfo.stPos.s32XPos;
	stInRect.y = par->stExtendInfo.stPos.s32YPos;
	stInRect.w = par->stExtendInfo.u32DisplayWidth;
	stInRect.h = par->stExtendInfo.u32DisplayHeight;

	/**
	 ** updata the input message after layer init��include the input rect,stride,fmt
	 ** ���豸��ͼ���ʼ�������������������³�ʼ����һЩ������Ϣ
	 ** ���logo��ʱ���������Щ��������������ط���Ҫ������Ϣ��������쳣
	 **/
    s_stDrvOps.HIFB_DRV_SetLayerInRect (u32LayerID, &stInRect);
	s_stDrvOps.HIFB_DRV_SetLayerStride (u32LayerID, u32Stride);
	s_stDrvOps.HIFB_DRV_SetLayerDataFmt(u32LayerID, par->stExtendInfo.enColFmt);

	return HI_SUCCESS;
}


/***************************************************************************************
* func          : hifb_logo_sd
* description   : copy logo para and memory to sd layer
                  CNcomment: ͬԴ��д������£������Ҳ��Ҫlogo����
                             ����Ĵ������µ�ʱ���дʹ���µĵ�ַû�����ݣ��м����һ��

                             1������ڻ�д�д�����д�ж��ϱ���д������е�ַ����Ȼ����
                                ʹ��logo�ĵ�ַ
                             2����д����жϲ��е�ַ����д�����֮��SD�����ݲ���Ҫ����
                                ������� CNend\n
* param[in]     : u32LayerID
* retval        : NA
* others:       : NA
***************************************************************************************/
static HI_VOID hifb_logo_app_sd(HI_VOID)
{
	HIFB_BUFFER_S   stLogoSdBuf;
	HIFB_BUFFER_S   stScreenSdBuf;
	HIFB_COLOR_FMT_E eFmt = HIFB_FMT_ARGB8888;

    memset(&stScreenSdBuf,  0, sizeof(HIFB_BUFFER_S));
    memset(&stLogoSdBuf,    0, sizeof(HIFB_BUFFER_S));

	stLogoSdBuf.stCanvas.enFmt        = eFmt;
    stLogoSdBuf.stCanvas.u32Height    = g_stGfxWbc2.s32BufferHeight;
    stLogoSdBuf.stCanvas.u32Width     = g_stGfxWbc2.s32BufferWidth;
    stLogoSdBuf.stCanvas.u32Pitch     = g_stGfxWbc2.u32BufferStride;
    stLogoSdBuf.stCanvas.u32PhyAddr   = gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].u32LogoSdAddr;

    stLogoSdBuf.UpdateRect.x          = 0;
    stLogoSdBuf.UpdateRect.y          = 0;
    stLogoSdBuf.UpdateRect.w          = stLogoSdBuf.stCanvas.u32Width;
    stLogoSdBuf.UpdateRect.h          = stLogoSdBuf.stCanvas.u32Height;

	stScreenSdBuf.stCanvas.enFmt      = eFmt;
    stScreenSdBuf.stCanvas.u32Height  = g_stGfxWbc2.s32BufferHeight;
    stScreenSdBuf.stCanvas.u32Width   = g_stGfxWbc2.s32BufferWidth;
    stScreenSdBuf.stCanvas.u32Pitch   = g_stGfxWbc2.u32BufferStride;
    stScreenSdBuf.stCanvas.u32PhyAddr = g_stGfxWbc2.u32WBCBuffer[g_stGfxWbc2.u32BufIndex];
    stScreenSdBuf.UpdateRect.x        = 0;
    stScreenSdBuf.UpdateRect.y        = 0;
    stScreenSdBuf.UpdateRect.w        = stScreenSdBuf.stCanvas.u32Width;
    stScreenSdBuf.UpdateRect.h        = stScreenSdBuf.stCanvas.u32Height;

#ifdef CONFIG_GFX_MMU_SUPPORT
	g_stTdeExportFuncs.HIFB_DRV_QuickCopyEx(&stLogoSdBuf, &stScreenSdBuf);
#else
    g_stTdeExportFuncs.HIFB_DRV_QuickCopy(&stLogoSdBuf, &stScreenSdBuf);
#endif
    g_stTdeExportFuncs.HIFB_DRV_WaitAllTdeDone(HI_TRUE);

	return;
    
}

/***************************************************************************************
* func          : hifb_logo_app
* description   : copy logo para and memory to app layer
                  CNcomment: ��logo�����ݿ�������ͼ���С�
                             Ҫ��ʹ�÷����������ݻ��ߵ�ַ�仯�ٸ��£��Ͳ���Ҫ����
                             �����ˡ�CNend\n
* param[in]     : u32LayerID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID hifb_logo_app(HI_U32 u32LayerID)
{
	struct fb_info *info  = s_stLayer[u32LayerID].pstInfo;
	HIFB_PAR_S *par       = (HIFB_PAR_S *)(info->par);
	HIFB_LOGO_CHANNEL_E enLogoChn  = HIFB_LOGO_CHN_BUTT;
	
	HIFB_BUFFER_S stLogoBuf;
	HIFB_BUFFER_S stScreenBuf;
    HI_U32 u32PixDepth = 0;
    HI_U32 u32Stride   = 0;


    if(HIFB_HD_LOGO_LAYER_ID != u32LayerID){
		return;
	}
    
	if (IS_HD_LAYER(u32LayerID) || IS_MINOR_HD_LAYER(u32LayerID)){
		enLogoChn = HIFB_LOGO_CHN_HD;
	}else{
		enLogoChn = HIFB_LOGO_CHN_SD;
	}

	if (!gs_stLogo.stLogoInfo[enLogoChn].bShow){
		/**not have logo
	     ** û�п���logo��
	     **/
		return;
	}

    memset(&stScreenBuf,  0, sizeof(HIFB_BUFFER_S));
    memset(&stLogoBuf,    0, sizeof(HIFB_BUFFER_S));

    u32PixDepth = info->var.bits_per_pixel >> 3;
    u32Stride = (info->var.xres_virtual * u32PixDepth + CONFIG_HIFB_STRIDE_16ALIGN - 1) & (~(CONFIG_HIFB_STRIDE_16ALIGN - 1));
    
	stLogoBuf.stCanvas.enFmt        = par->stExtendInfo.enColFmt;
    stLogoBuf.stCanvas.u32Height    = par->stExtendInfo.u32DisplayHeight;
    stLogoBuf.stCanvas.u32Width     = par->stExtendInfo.u32DisplayWidth;
    stLogoBuf.stCanvas.u32Pitch     = u32Stride;
    stLogoBuf.stCanvas.u32PhyAddr   = gs_stLogo.stLogoInfo[enLogoChn].u32LogoAddr;

    stLogoBuf.UpdateRect.x          = 0;
    stLogoBuf.UpdateRect.y          = 0;
    stLogoBuf.UpdateRect.w          = stLogoBuf.stCanvas.u32Width;
    stLogoBuf.UpdateRect.h          = stLogoBuf.stCanvas.u32Height;

    
	stScreenBuf.stCanvas.enFmt      = par->stExtendInfo.enColFmt;
    stScreenBuf.stCanvas.u32Height  = par->stExtendInfo.u32DisplayHeight;
    stScreenBuf.stCanvas.u32Width   = par->stExtendInfo.u32DisplayWidth;
    stScreenBuf.stCanvas.u32Pitch   = info->fix.line_length;
    stScreenBuf.stCanvas.u32PhyAddr = info->fix.smem_start;
    stScreenBuf.UpdateRect.x        = 0;
    stScreenBuf.UpdateRect.y        = 0;
    stScreenBuf.UpdateRect.w        = stScreenBuf.stCanvas.u32Width;
    stScreenBuf.UpdateRect.h        = stScreenBuf.stCanvas.u32Height;
    
	if(0 != gs_stLogo.stLogoInfo[enLogoChn].u32LogoAddr){
        #ifdef CONFIG_GFX_MMU_SUPPORT
            /** boot�·�MMZ��������ַ��smmu��ַ���� **/
		    g_stTdeExportFuncs.HIFB_DRV_QuickCopyEx(&stLogoBuf, &stScreenBuf);
        #else
            g_stTdeExportFuncs.HIFB_DRV_QuickCopy(&stLogoBuf, &stScreenBuf);
        #endif
	}
	if(    0 != gs_stLogo.stLogoInfo[HIFB_LOGO_CHN_SD].u32LogoSdAddr \
		&& 0 != g_stGfxWbc2.u32WBCBuffer[g_stGfxWbc2.u32BufIndex]){
		/** ��ͬԴ��д,��Ҫ���±�����Դ��ַ������ **/
		hifb_logo_app_sd();
	}
    
	return;
    
}
#else
/***************************************************************************************
* func          : hifb_logo_app
* description   : now can logo to app
                  CNcomment: ���Խ���logo��APP�����ˣ����ñ�ǣ������̺߳�������CNend\n
* param[in]     : u32LayerID
* retval        : NA
* others:       : NA
***************************************************************************************/
HI_VOID hifb_logo_app(HI_U32 u32LayerID)
{

	HIFB_LOGO_CHANNEL_E enLogoChn = HIFB_LOGO_CHN_HD;

    if(HIFB_HD_LOGO_LAYER_ID != u32LayerID){
		return;
	}
    
	if (IS_HD_LAYER(u32LayerID) || IS_MINOR_HD_LAYER(u32LayerID)){
		enLogoChn = HIFB_LOGO_CHN_HD;
	}else{
		enLogoChn = HIFB_LOGO_CHN_SD;
	}
	#if 0
	if(NULL != gs_stLogo.stLogoInfo[enLogoChn].pThreadTask){
		kthread_stop(gs_stLogo.stLogoInfo[enLogoChn].pThreadTask);
	}
	#endif
	gs_stLogo.stLogoInfo[enLogoChn].bClean = HI_TRUE;
	
	return;
}
#endif/** CONFIG_HIFB_CRC_LOGO_TO_APP **/
#endif/** CFG_HIFB_LOGO_SUPPORT **/
