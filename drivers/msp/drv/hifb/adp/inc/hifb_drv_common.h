 /******************************************************************************
 *
 * Copyright (C) 2015 Hisilicon Technologies Co., Ltd.	All rights reserved. 
 *
 * This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
 * and may not be copied, reproduced, modified, disclosed to others, published or used, in
 * whole or in part, without the express prior written permission of Hisilicon.
 *
 ******************************************************************************
 File Name			 : hifb_drv_common.h
 Version			 : Initial Draft
 Author 			 : 
 Created			 : 2014/09/09
 Description		 : 所有信息的变化都在这一个文件中体现，其它的保持不动
 Function List		 : 
 History			 :
 Date						Author					 Modification
 2014/09/09 				y00181162				 Created file		 
 ******************************************************************************/
 
#ifndef __HIFB_DRV_COMMON_H__
#define __HIFB_DRV_COMMON_H__
 

/*********************************add include here******************************/
#include "hifb.h"
#include "hifb_comm.h"


/*****************************************************************************/


#ifdef __cplusplus
#if __cplusplus
   extern "C"
{
#endif
#endif /* __cplusplus */



/***************************** Macro Definition ******************************/

#define HIFB_WORK_QUEUE      	 	"HIFB_WorkQueque"
#define OPTM_GFX_WBC2_BUFFER 	 	"HIFB_GfxWbc2"
#define DISPLAY_BUFFER_HD    		"HIFB_DispBuf_HD"
#define DISPLAY_BUFFER_SD    		"HIFB_DispBuf_SD"
#define HIFB_ZME_COEF_BUFFER 		"HIFB_ZmeCoef"
#define OPTM_GFX_CMP_BUFFER  		"HIFB_GfxCmp"
#define OPTM_GFX_CMP_QUEUE   		"HIFB_CompressQueque"

#define CFG_HIFB_VIRTUAL_COORDINATE_SUPPORT

#ifndef HI_ADVCA_FUNCTION_RELEASE
#define CFG_HIFB_PROC_SUPPORT
#endif

//#define GFX_TEST_LOG	
#ifdef GFX_TEST_LOG
#define PRINT_IN  printk("func %s in!\n", __FUNCTION__)
#define PRINT_OUT printk("func %s out!\n",__FUNCTION__)
#else
#define PRINT_IN 
#define PRINT_OUT
#endif 

/*the point of callback function*/
/*CNcomment:回调函数指针*/
typedef HI_S32 (* IntCallBack)(HI_VOID *pParaml, HI_VOID *pParamr);


#define IS_HD_LAYER(enLayerId) (enLayerId < HIFB_LAYER_HD_1)
#define IS_SD_LAYER(enLayerId) ((enLayerId >= HIFB_LAYER_SD_0) && (enLayerId < HIFB_LAYER_SD_1))
#define IS_AD_LAYER(enLayerId) ((enLayerId >= HIFB_LAYER_AD_0) && (enLayerId <= HIFB_LAYER_AD_3))
#define IS_MINOR_HD_LAYER(enLayerId) ((enLayerId <= HIFB_LAYER_HD_3) && (enLayerId >= HIFB_LAYER_HD_1))
#define IS_MINOR_SD_LAYER(enLayerId) ((enLayerId <= HIFB_LAYER_SD_3) && (enLayerId >= HIFB_LAYER_SD_1))


#define HIFB_CHECK_PONITER(pStr) do{\
if (pStr == HI_NULL){\
      HIFB_ERROR("unable to process null pointer!\n");\
      return HI_FAILURE;}\
}while(0)


/*************************** Structure Definition ****************************/

typedef struct
{
    HI_BOOL bKeyEnable;         /** colorkey enable flag **//*CNcomment:colorkey 是否使能*/
    HI_BOOL bMaskEnable;        /** key mask enable flag **//*CNcomment:key mask 是否使能*/
    HI_U32 u32Key;              /** key value **/
    HI_U8 u8RedMask;           /** red mask   **/
    HI_U8 u8GreenMask;         /** green mask **/
    HI_U8 u8BlueMask;          /** blue mask  **/
    HI_U8 u8Reserved;           
    HI_U32 u32KeyMode;	 /*0:In region; 1:Out region*/

    /*Max colorkey value of red component*/
    /*CNcomment:colorkey红色分量最大值*/
    HI_U8 u8RedMax;

    /*Max colorkey value of Green component*/
    /*CNcomment:colorkey绿色分量最大值*/
    HI_U8 u8GreenMax; 

    /*Max colorkey value of blue component*/
    /*CNcomment:colorkey蓝色分量最大值*/
    HI_U8 u8BlueMax;           
    HI_U8 u8Reserved1;

    /*Min colorkey value of red component*/
    /*CNcomment:colorkey红色分量最小值*/
    HI_U8 u8RedMin;            

    /*Min colorkey value of Green component*/
    /*CNcomment:colorkey绿色分量最小值*/
    HI_U8 u8GreenMin;         

    /*Min colorkey value of blue component*/
    /*CNcomment:colorkey蓝色分量最小值*/
    HI_U8 u8BlueMin;            
    HI_U8 u8Reserved2;
}HIFB_COLORKEYEX_S;

typedef enum
{
    /** VO vertical timing interrupt **/
    /** CNcomment:垂直时序中断       **/
    HIFB_CALLBACK_TYPE_VO          = 0x1,
	/** 3D Mode changed interrupt **/
    /** CNcomment:3D模式改变中断  **/
    HIFB_CALLBACK_TYPE_3DMode_CHG  = 0x2,
	/** VO Register update completed interrupt **/
    /** CNcomment:寄存器更新完成中断           **/
    HIFB_CALLBACK_TYPE_REGUP       = 0x4,
	/** Frame start interrupt  **/
    /** CNcomment:帧开始中断   **/
    HIFB_CALLBACK_TYPE_FRAME_START = 0x8,
	/** Frame end interrupt    **/
    /** CNcomment:帧结束中断   **/
    HIFB_CALLBACK_TYPE_FRAME_END   = 0x10,
    HIFB_CALLBACK_TYPE_BUTT,
}HIFB_CALLBACK_TPYE_E;


/*scan mode*/
typedef enum
{
    HIFB_SCANMODE_P,
    HIFB_SCANMODE_I,
    HIFB_SCANMODE_BUTT,
}HIFB_SCAN_MODE_E;

/*layer state*/
typedef enum 
{
    HIFB_LAYER_STATE_ENABLE = 0x0,  /*Layer enable*/ /*CNcomment:层使能*/
    HIFB_LAYER_STATE_DISABLE,          /*Layer disable*/ /*CNcomment:层未使能*/
    HIFB_LAYER_STATE_INVALID,          /*Layer invalid*/ /*CNcomment:层无效,不存在*/
    HIFB_LAYER_STATE_BUTT 
} HIFB_LAYER_STATE_E;


/* GFX mode  */
typedef enum tagHIFB_GFX_MODE_EN
{
    HIFB_GFX_MODE_NORMAL = 0,
    HIFB_GFX_MODE_HD_WBC,
    HIFB_GFX_MODE_BUTT
}HIFB_GFX_MODE_EN;



/*osd info*/
typedef struct
{
    HIFB_LAYER_STATE_E eState;
	/*Layer work mode, same source mode or different source mode*/
    /*CNcomment:图层工作模式，同源或非同源*/
    HIFB_GFX_MODE_EN  eGfxWorkMode;
	HI_BOOL bPreMul;
    HI_U32  u32RegPhyAddr; 
    HI_U32  u32Stride;
	/*Screen width in current format*/
    /*CNcomment:当前制式下屏幕宽*/
    HI_U32 u32ScreenWidth;  
    /*Screen height in current format*/
    /*CNcomment:当前制式下屏幕高度*/
    HI_U32 u32ScreenHeight; 
    /**outRect size*/
	HIFB_RECT stOutRect;
	HIFB_RECT stInRect;
    HIFB_COLOR_FMT_E eFmt;
    HIFB_ALPHA_S stAlpha;
    HIFB_COLORKEYEX_S stColorKey; 
	HIFB_SCAN_MODE_E eScanMode;
	HIFB_LAYER_ID_E  enSlaveryLayerID;
}HIFB_OSD_DATA_S;

/*osd info*/
typedef struct
{
    HI_BOOL bShow;
	HI_BOOL bOpen;
	/*Layer work mode, same source mode or different source mode*/
    /*CNcomment:图层工作模式，同源或非同源*/
    HIFB_GFX_MODE_EN  eGfxWorkMode;
    HI_U32 u32Stride;
	HI_U32 u32WbcBufNum;
    HI_U32 u32WbcBufSize;
	HI_U32 u32ReadBufAddr;

	/*source buffer of write back*/
	HIFB_RECT stSrcBufRect;
	/*current buffer of write back*/
	HIFB_RECT stCurWBCBufRect;
	/*max buffer of write back*/
	HIFB_RECT stMaxWbcBufRect;
	/*display region of write back*/
    HIFB_RECT stScreenRect;
	
    HIFB_COLOR_FMT_E eFmt;
	HIFB_SCAN_MODE_E eScanMode;
	HIFB_LAYER_ID_E  enLayerID;
}HIFB_SLVLAYER_DATA_S;


/********************** Global Variable declaration **************************/



/******************************* API declaration *****************************/

#ifdef __cplusplus

#if __cplusplus

}
#endif
#endif /* __cplusplus */

#endif /* __HIFB_DRV_COMMON_H__ */
