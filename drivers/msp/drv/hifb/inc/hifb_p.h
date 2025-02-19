/******************************************************************************
*
* Copyright (C) 2015 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
* and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
******************************************************************************
File Name           : hifb_p.h
Version             : Initial Draft
Author              : 
Created             : 2015/05/19
Description         :
Function List       : 
History             :
Date                       Author                   Modification
2015/05/19                 y00181162                Created file        
******************************************************************************/

#ifndef __HIFB_P_H__
#define __HIFB_P_H__


/*********************************add include here******************************/
#include "hi_type.h"
#include <linux/fb.h>
#include "hifb_drv.h"
#include "hifb_drv_common.h"
#include "hifb_scrolltext.h"
#include "hifb_config.h"

/*****************************************************************************/


#ifdef __cplusplus
#if __cplusplus
   extern "C"
{
#endif
#endif /* __cplusplus */



/***************************** Macro Definition ******************************/


/**
 ** support 32 layers most, the limit is from linux fb
 **/
#define MAX_FB  32

/**
 ** define the value of default set of each layer 
 **/
#define HIFB_HD_DEF_WIDTH    1280                     /* unit: pixel            */
#define HIFB_HD_DEF_HEIGHT   720                      /* unit: pixel            */
#define HIFB_HD_DEF_STRIDE   (HIFB_HD_DEF_WIDTH*4)    /* unit: byte             */
#define HIFB_HD_DEF_VRAM     7200                     /* unit: KB 1280*720*4*2  */


#define HIFB_SD_DEF_WIDTH    1280
#define HIFB_SD_DEF_HEIGHT   720
#define HIFB_SD_DEF_STRIDE   (HIFB_SD_DEF_WIDTH*4)
#define HIFB_SD_DEF_VRAM     7200                     /* unit: KB 720*576*4*2   */


#define HIFB_AD_DEF_WIDTH    1280
#define HIFB_AD_DEF_HEIGHT   80
#define HIFB_AD_DEF_STRIDE   (HIFB_AD_DEF_WIDTH*4)
#define HIFB_AD_DEF_VRAM     800                      /* unit: KB 1280*80*4*2   */

#define HIFB_CURSOR_DEF_WIDTH    128
#define HIFB_CURSOR_DEF_HEIGHT   128
#define HIFB_CURSOR_DEF_STRIDE   (HIFB_CURSOR_DEF_WIDTH*4)
#define HIFB_CURSOR_DEF_VRAM     128                  /* unit: KB 128*128*4*2   */


#define HIFB_DEF_DEPTH        16
#define HIFB_DEF_XSTART       0
#define HIFB_DEF_YSTART       0
#define HIFB_DEF_ALPHA        0xff
#define HIFB_DEF_PIXEL_FMT    HIFB_FMT_ARGB1555

#define HIFB_IS_CLUTFMT(eFmt)     (HIFB_FMT_1BPP <= (eFmt) && (eFmt) <= HIFB_FMT_ACLUT88)
#define HIFB_ALPHA_OPAQUE         0xff
#define HIFB_ALPHA_TRANSPARENT    0x00

/**
 ** support level 5 deflicker most 
 **/
#define HIFB_DEFLICKER_LEVEL_MAX  5

#define HIFB_MAX_LAYER_ID         (HIFB_LAYER_ID_BUTT-1)
#define HIFB_MAX_LAYER_NUM        HIFB_LAYER_ID_BUTT

#define HIFB_MAX_FLIPBUF_NUM      2

#define HIFB_MAX_PIXFMT_NUM       28

/*************************** Structure Definition ****************************/

typedef enum
{
	HIFB_ANTIFLICKER_NONE,	/* no antiflicker.If scan mode is progressive, hifb will set antiflicker mode to none */
	HIFB_ANTIFLICKER_TDE,	/* tde antiflicker mode, it's effect for 1buf or 2buf only */
	HIFB_ANTIFLICKER_VO,	/* vo antiflicker mode, need hardware supprot */
	HIFB_ANTIFLICKER_BUTT
} HIFB_LAYER_ANTIFLICKER_MODE_E;


/**
 ** only use in logo transition
 **/
typedef enum
{
    HIFB_STATE_LOGO_IN         = 0x1,     /* boot start with logo           */
    HIFB_STATE_PUT_VSCREENINFO = 0x2,     /* ioctl:FBIOPUT_VSCREENINFO      */
    HIFB_STATE_PAN_DISPLAY     = 0x4,     /* call function hifb_pan_display */
    HIFB_STATE_REFRESH         = 0x8,     /* refresh app                    */
    HIFB_STATE_BUTT
}HIFB_STATE_E;

/**
 ** frame info
 **/
typedef struct
{
    HI_U32  u32RefreshFrame;
    HI_U32  u32StartTimeMs;
    HI_U32  u32Fps;
    HI_BOOL bFrameHit;
}HIFB_FRAME_INFO_S;

/**
 ** 3D MEM INFO STRUCT
 **/
typedef struct
{
    HI_U32 u32StereoMemStart;
    HI_U32 u32StereoMemLen;
    struct mutex stStereoMemLock;    
}HIFB_3DMEM_INFO_S;


/**
 ** 3D PAR INFO STRUCT
 **/
typedef struct
{
	HI_S32                 s32StereoDepth;	
	HI_U32                 u32rightEyeAddr;     /**<  right eye address */
	HI_U32                 u32DisplayAddr[HIFB_MAX_FLIPBUF_NUM];
    HIFB_STEREO_MODE_E     enInStereoMode;
    HIFB_STEREO_MODE_E     enOutStereoMode; 	
	HIFB_RECT              st3DUpdateRect;
	HIFB_SURFACE_S         st3DSurface;
    HIFB_3DMEM_INFO_S      st3DMemInfo;
}HIFB_3D_PAR_S;


/**
 ** N3D PAR INFO STRUCT
 **/
typedef struct
{
	HIFB_RECT       stCmpRect;
	HIFB_RECT       stUpdateRect;
	HI_U32          u32DisplayAddr[HIFB_MAX_FLIPBUF_NUM]; /** 加载KO分配的两个块内存 **/
	HIFB_SURFACE_S  stCanvasSur;         /**
	                                      ** canvas surface allocated for user，单buffer就给硬件 
	                                      ** 双buffer， canvas buffer blit display buffer
	                                      **/
	HIFB_BUFFER_S   stUserBuffer;        /** 备份用户信息，可以重新刷新用户的最后一帧数据，要是没有
	                                      ** 备份，重新刷新可能刷新异常
	                                      ** backup usr's refreshing buffer data, 
										  ** using when refresh again or refresh all
										  **/
}HIFB_DISP_INFO_S;


/** 运行相关的 **/
typedef struct 
{    
	HI_BOOL   bModifying;
	HI_U32    u32ParamModifyMask; 
    HI_BOOL   bNeedFlip;       /* when tde blit job completed, we need to flip buffer, only using in pandisplay and 2buf*/
    HI_BOOL   bFliped;	       /* a flag to record buf has been swithed no not in vo isr, effect only in 2 buf mode*/
    HI_U32    u32IndexForInt;  /* index of screen buf*/
	HI_U32    u32BufNum;       /* count of flip buffer*/	
	HI_U32    u32ScreenAddr;   /* screen buf addr */
	HI_S32    s32RefreshHandle;/* job handle of tde blit*/
}HIFB_RTIME_INFO_S;


/** 显示相关的 **/
typedef struct
{
	HI_BOOL                         bOpen;               /* open status*/
	HI_BOOL                         bShow;               /* show status */
    HIFB_COLOR_FMT_E                enColFmt;            /* color format */
	HIFB_LAYER_BUF_E                enBufMode;           /* refresh mode*/
	HI_U32                          u32DisplayWidth;     /* width  of layer's display buffer*/
    HI_U32                          u32DisplayHeight;    /* height of layer's display buffer*/
    HI_U32                          u32ScreenWidth;      /* 无用 width  of layer's  show    area*/
    HI_U32                          u32ScreenHeight;     /* 无用 height of layer's  show    area*/
	HIFB_POINT_S                    stPos;               /* beginning position of layer*/ 
    HIFB_ALPHA_S                    stAlpha;             /* alpha attribution */
    HIFB_COLORKEYEX_S               stCkey;              /* colorkey attribution */  	
}HIFB_EXTEND_INFO_S;

typedef struct
{
	HI_U32             u32LayerID;       /* layer id */
	atomic_t           ref_count;        /* framebuffer reference count */
	spinlock_t         lock;             /* using in 2buf refresh */
	HI_BOOL            bPreMul;
	HI_BOOL            bNeedAntiflicker;
	HI_U32             u32HDflevel;      /* horizontal deflicker level */
	HI_U32             u32VDflevel;      /* vertical deflicker level */
	HI_UCHAR           ucHDfcoef[HIFB_DEFLICKER_LEVEL_MAX - 1];  /* horizontal deflicker coefficients */
	HI_UCHAR           ucVDfcoef[HIFB_DEFLICKER_LEVEL_MAX - 1];  /* vertical deflicker coefficients */
	HIFB_LAYER_ANTIFLICKER_LEVEL_E  enAntiflickerLevel;          /* antiflicker level */
	HIFB_LAYER_ANTIFLICKER_MODE_E   enAntiflickerMode;           /* antiflicker mode */ 
}HIFB_BASE_INFO_S;


#ifdef CFG_HIFB_PROC_SUPPORT 
typedef struct
{
    HI_BOOL bWbcProc;                  /* whether wbc proc or not*/
	HI_BOOL bCreatedProc;              /* have created proc */
	HI_U32          u32MasterLayerNum; /* the master layer num of slavery layer */
	HIFB_LAYER_ID_E enWbcLayerID;      /* id of the layer attached to master layer */
}HIFB_PROC_S;
#endif

typedef struct
{
    HI_BOOL bStartRelease;
    HI_U32  u32PhyBuf;
    HI_U32  u32SmemStartPhy;
    HI_CHAR *pVirBuf;
    struct work_struct  bDisBufWork;
}HIFB_DISP_TMPBUF_S;

typedef struct
{   	
	HI_BOOL             bSetStereoMode;
	HI_BOOL             bPanFlag;
    HI_BOOL             bVblank;
    HI_BOOL             bHwcRefresh;
#ifdef CONFIG_HIFB_GP0_GALPHA_SUM_SUPPORT
    HI_BOOL             bLayerClose;
#endif
    HI_U32              u32SmemStartPhy;
    HIFB_BASE_INFO_S    stBaseInfo;
    HIFB_EXTEND_INFO_S  stExtendInfo;
    
    HIFB_3D_PAR_S       st3DInfo;
    HIFB_DISP_INFO_S    stDispInfo;

	HIFB_RTIME_INFO_S   stRunInfo;      /**run time info for N3D and 3D*/
    HIFB_FRAME_INFO_S   stFrameInfo;
#ifdef CFG_HIFB_PROC_SUPPORT
	HIFB_PROC_S         stProcInfo;
#endif
    HIFB_DISP_TMPBUF_S  stDispTmpBuf;   /** 重新分配内存释放旧的内存使用 **/
}HIFB_PAR_S;


typedef struct 
{
    struct fb_info *pstInfo;
    HI_U32    u32LayerSize;     /*u32LayerSize = fb.smem_len*/
} HIFB_LAYER_S;



typedef enum 
{
    HIFB_LAYER_TYPE_HD,
    HIFB_LAYER_TYPE_SD,
    HIFB_LAYER_TYPE_AD,
    HIFB_LAYER_TYPE_CURSOR,
    HIFB_LAYER_TYPE_BUTT,    
}HIFB_LAYER_TYPE_E;


typedef struct
{
    struct fb_bitfield stRed;     /* bitfield in fb mem if true color, */
    struct fb_bitfield stGreen;   /* else only length is significant */
    struct fb_bitfield stBlue;
    struct fb_bitfield stTransp;  /* transparency	*/
} HIFB_ARGB_BITINFO_S;


/********************** Global Variable declaration **************************/

extern HIFB_DRV_OPS_S    s_stDrvOps;
extern HIFB_DRV_TDEOPS_S g_stTdeExportFuncs;
/**
 **to save layer id and layer size 
 **/
extern HIFB_LAYER_S s_stLayer[HIFB_MAX_LAYER_NUM];

extern HIFB_ARGB_BITINFO_S s_stArgbBitField[HIFB_MAX_PIXFMT_NUM];
	
#ifdef CFG_HIFB_SCROLLTEXT_SUPPORT
extern HIFB_SCROLLTEXT_INFO_S s_stTextLayer[HIFB_LAYER_ID_BUTT];
#endif


/******************************* API declaration *****************************/

#ifdef __cplusplus

#if __cplusplus

}
#endif
#endif /* __cplusplus */

#endif /* __HIFB_P_H__ */
