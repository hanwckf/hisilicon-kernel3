/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWinjiDDUykL9e8pckESWBbMVmSWkBuyJO01cTiy3TdzKxGk0oBQa
mSMf7J4FkTpfvy2G1NnyQwzUAGIEtSnc4g7Wg/c3XrtyX23Ys9f4aeZ/vlFxbPVNacoGPgQM
kA3hNwce+Yr9XD5LbMxvwjwN2KFfwkKDhIH2emy9d0HSpkAvsbwjW7Dbsff00w8p4LU9rogF
o/ugeNAbMVzgjABRm/le2M/a0yYv5d1sg3AoUnKOrO6CeX1DxuOsE9/eXJ5RCA==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
#ifndef __VPSS_HAL_3798CV200_H__
#define __VPSS_HAL_3798CV200_H__
#include"hi_type.h"

#include "hi_drv_mmz.h"

#include"vpss_common.h"

#include "hi_drv_vpss.h"
#include "vpss_reg_3798cv200.h"

#include "vpss_sttinf.h"
#include "vpss_wbc.h"
#include "vpss_rwzb.h"
#include "vpss_his.h"
#include "drv_pq_ext.h"
#include "vpss_wbc_mcdei.h"
#include "vpss_stt_mcdei.h"

#define DEF_LOGIC_TIMEOUT  0xffffffff//0xe4e1c0


#define  VPSS_ZME_HPREC        (1<<20)
#define  VPSS_ZME_HPREC_F        (1<<19)
#define  VPSS_ZME_HPREC_B        (1<<1)

#define  VPSS_ZME_VPREC        (1<<12)

#define HAL_VERSION_3798M 0x300

#define VPSS0_BASE_ADDR  0xf8cb0000
#define VPSS1_BASE_ADDR  0xffffffff
#define VPSS_REG_SIZE    0xf550

#define VPSS_REG_SIZE_CALC(start, end)\
    (offsetof(VPSS_REG_S, end) + sizeof(HI_U32) -\
     offsetof(VPSS_REG_S, start))
     
#define VPSS_HAL_CHECK_IP_VAILD(enIP) \
do{\
    if((enIP != VPSS_IP_0)&&(enIP != VPSS_IP_1))\
    {\
        VPSS_ERROR("VPSS IP%d, is Not Vaild\n", enIP);\
        return HI_FAILURE;\
    }\
}while(0)

#define VPSS_HAL_CHECK_NODE_ID_VAILD(enTaskNodeId) \
do{\
    if(enTaskNodeId >= VPSS_HAL_TASK_NODE_BUTT)\
    {\
        VPSS_ERROR("VPSS NODE ID%d, is Not Vaild\n", enTaskNodeId);\
        return HI_FAILURE;\
    }\
}while(0)

#define VPSS_HAL_CHECK_INIT(bInit) \
do{\
    if (HI_FALSE == bInit){\
        VPSS_ERROR("VPSS HAL Is Not Init\n");\
        return HI_FAILURE;\
    }\
}while(0)


#define VPSS_HAL_CHECK_NULL_PTR(ptr) \
do{\
    if (HI_NULL == ptr){\
        VPSS_ERROR("pointer is NULL!\n");\
        return HI_FAILURE;\
    }\
}while(0)

#define DEF_VPSS_HAL_PORT_NUM 1

typedef enum hiVPSS_IP_E
{
    VPSS_IP_0 = 0,
    VPSS_IP_1,
    VPSS_IP_BUTT
}VPSS_IP_E;

typedef enum hiVPSS_HAL_TASK_NODE_E
{
    VPSS_HAL_TASK_NODE_2D_FIELD = 0,
	VPSS_HAL_TASK_NODE_2D_FIELD_VIRTUAL,
    VPSS_HAL_TASK_NODE_2D,
	VPSS_HAL_TASK_NODE_SPLIT_L,
	VPSS_HAL_TASK_NODE_SPLIT_R,
    VPSS_HAL_TASK_NODE_2D_VIRTUAL,
    VPSS_HAL_TASK_NODE_3D_R,
    VPSS_HAL_TASK_NODE_3DDET,
    VPSS_HAL_TASK_NODE_P0_ZME_L2,
    VPSS_HAL_TASK_NODE_P1_ZME_L2,
    VPSS_HAL_TASK_NODE_P2_ZME_L2,
    VPSS_HAL_TASK_NODE_P0_RO_Y,
    VPSS_HAL_TASK_NODE_P0_RO_C,
    VPSS_HAL_TASK_NODE_P1_RO_Y,
    VPSS_HAL_TASK_NODE_P1_RO_C,
    VPSS_HAL_TASK_NODE_P2_RO_Y,
    VPSS_HAL_TASK_NODE_P2_RO_C,
	VPSS_HAL_TASK_NODE_2D_DETILE_STEP1,
	VPSS_HAL_TASK_NODE_2D_DETILE_STEP2,
    VPSS_HAL_TASK_NODE_BUTT
}VPSS_HAL_TASK_NODE_E;


typedef struct hiVPSS_HAL_CTX_S
{
    HI_BOOL  bInit;
    HI_BOOL  bClockEn;
    HI_U32   u32LogicVersion;
    
    HI_U32   u32BaseRegPhy;
    HI_U8*   pu8BaseRegVir;
    
    HI_U32   au32AppPhy[VPSS_HAL_TASK_NODE_BUTT];
    HI_U8*   apu8AppVir[VPSS_HAL_TASK_NODE_BUTT];
    MMZ_BUFFER_S stRegBuf;
    
#ifdef HI_VPSS_SMMU_SUPPORT
    SMMU_BUFFER_S     stDeTileMMUBuf;	   //解tile使用的临时buffer
#else
    MMZ_BUFFER_S     stDeTileMMZBuf;	   //解tile使用的临时buffer
#endif

} VPSS_HAL_CTX_S;


typedef struct hiVPSS_HAL_ZME_PARAM_S
{
    HI_BOOL bYUV;
    HI_U32 u32YHRatio;
    HI_U32 u32CHRatio;
    HI_U32 u32YVRatio;
    HI_U32 u32CVRatio;
    ZME_FORMAT_E enInFmt;
    ZME_FORMAT_E enOutFmt;    
}VPSS_HAL_ZME_PARAM_S;

typedef struct hiVPSS_HAL_FRAME_S{
    HI_DRV_FRAME_TYPE_E      eFrmType;
    HI_U32 u32Width;
    HI_U32 u32Height;
    HI_DRV_PIX_FORMAT_E enFormat;
    HI_DRV_FIELD_MODE_E enFieldMode;
    HI_BOOL bProgressive;
    HI_DRV_VID_FRAME_ADDR_S stAddr;
	HI_DRV_VID_FRAME_ADDR_S stAddr_LB; //@f00241306 for dither
	HI_DRV_VID_FRAME_ADDR_S stAddr_META; 
	HI_U32                   u32MetaSize;
    HI_BOOL                  bCompressd;
	HI_BOOL                  bUVInver;
    HI_DRV_PIXEL_BITWIDTH_E  enBitWidth;
    HI_U32 u32TunnelAddr;
    HI_BOOL  bTopFirst;
	HI_BOOL  bSecure;
}VPSS_HAL_FRAME_S;


typedef struct hiVPSS_HAL_PORT_INFO_S
{
    HI_BOOL    bEnable;
    HI_RECT_S  stInCropRect; /* PORT CROP信息 */
    HI_RECT_S  stVideoRect; /* 真实显示区域 */
    HI_RECT_S   stOutCropRect;
    HI_DRV_VPSS_ROTATION_E enRotation; /* 旋转信息 */
    HI_BOOL bNeedFlip;
    HI_BOOL bNeedMirror;
    HI_BOOL bCmpLoss;
	HI_BOOL bConfig;

    VPSS_HAL_FRAME_S stOutInfo; /* PORT输出信息 */
} VPSS_HAL_PORT_INFO_S;


typedef struct hiVPSS_DIE_INFO_S
{
	VPSS_DIESTCFG_S stDieStCfg;
    HI_BOOL bBottom_first;

	//:TODO: 插值选择信息
	
}VPSS_DIE_INFO_S;

typedef struct hiVPSS_NR_INFO_S
{
    HI_BOOL bNrEn;
	VPSS_NRMADCFG_S stNrMadCfg;
}VPSS_NR_INFO_S;

typedef struct hiVPSS_CCCL_INFO_S
{
    HI_BOOL bCCCLEn;
	VPSS_HAL_FRAME_S stInRefInfo[2];
	VPSS_CCCLCNTCFG_S stCCCLCntCfg;
}VPSS_CCCL_INFO_S;


typedef enum hiVPSS_HAL_NODE_TYPE_E
{
    VPSS_HAL_NODE_2D_FRAME = 0,
    VPSS_HAL_NODE_2D_5Field,
    VPSS_HAL_NODE_2D_3Field,
    VPSS_HAL_NODE_2D_Field,
    VPSS_HAL_NODE_3D_FRAME_R, //用于配置读取偏移，在解码源为SBS/TAB，暂时不考虑拆分之后还有隔行的情况
    VPSS_HAL_NODE_PZME, //对应隔行，单场的源的类型
    VPSS_HAL_NODE_UHD, // 4K*2K场景，后面看是否有UHD非标的特殊场景，再增加类型
    VPSS_HAL_NODE_UHD_HIGH_SPEED,// 4K*2K场景，DTV  4k@60场景,一拍两像素配置
    VPSS_HAL_NODE_UHD_SPLIT_L,
    VPSS_HAL_NODE_UHD_SPLIT_R,
    VPSS_HAL_NODE_UHD_HALF,
    VPSS_HAL_NODE_3DDET,// 3D检测通路，只需要Y分量
    VPSS_HAL_NODE_ZME_2L,// 2级缩放节点
    VPSS_HAL_NODE_ROTATION_Y,
    VPSS_HAL_NODE_ROTATION_C,
    VPSS_HAL_NODE_HDR_EL,
	VPSS_HAL_NODE_2D_DETILE_STEP1, //de-tile node
	VPSS_HAL_NODE_2D_DETILE_STEP2, //first3field's 2D node
    VPSS_HAL_NODE_BUTT
} VPSS_HAL_NODE_TYPE_E;

typedef struct hiVPSS_MCDEI_RFR_S
{
    HI_BOOL bMcdeiEn;   //MCDEI开关
    HI_BOOL bMedsEn;    //ME下采样开关
    VPSS_HAL_FRAME_S stRgmeWbc;  //rgme回写帧
    VPSS_HAL_FRAME_S stRgmeRef[3];  //rgme参考帧
    VPSS_HAL_FRAME_S stBlendWbc;  //blend回写帧
    VPSS_HAL_FRAME_S stBlendRef;  //blend参考帧	
} VPSS_MCDEI_RFR_S;

typedef struct hiVPSS_MCDEI_ST_S
{
    VPSS_ST_RGME_CFG_S   stRgmeCfg;   //rgme运动信息
    VPSS_ST_BLKMV_CFG_S  stBlkmvCfg;  //blkmv运动信息
    VPSS_ST_PRJH_CFG_S   stPrjhCfg;  //prjh运动信息
    VPSS_ST_PRJV_CFG_S   stPrjvCfg;  //prjv运动信息
} VPSS_MCDEI_ST_S;
typedef struct hiVPSS_HAL_INFO_S
{
    VPSS_REG_S *pstPqCfg;
    VPSS_HAL_NODE_TYPE_E enNodeType; 
	VPSS_HAL_FRAME_S stInInfo;             //输入源信息
    VPSS_RWZB_INFO_S stRwzbInfo;
    
	/*VPSS V2_0*/
    VPSS_HAL_FRAME_S stInRefInfo[4];       //参考帧信息
    VPSS_HAL_FRAME_S stInWbcInfo;          //回写信息
    VPSS_HAL_FRAME_S stDeTileFrame;	   //解tile的临时buffer
    HI_U32			 u32DetileFieldIdx;	   //解tile 的前3场序号
    HI_U32 u32stt_w_phy_addr;
    HI_U8* pu8stt_w_vir_addr;
	
	VPSS_NR_INFO_S stNrInfo;
	VPSS_CCCL_INFO_S stCCCLInfo;
	
	/*VPSS V1_0*/
    HI_DRV_VID_FRAME_ADDR_S stFieldAddr[6];
    VPSS_HIS_ADDR_S stHisAddr;
	
    VPSS_DIE_INFO_S stDieInfo; 

    HI_U32 u32ScdValue;
    VPSS_MCDEI_RFR_S stMcdeiRfrInfo;    //mcdei回写信息
    VPSS_MCDEI_ST_S    stMcdeiStInfo;     //mcdei运动信息 
    VPSS_HAL_PORT_INFO_S astPortInfo[DEF_HI_DRV_VPSS_PORT_MAX_NUMBER];

} VPSS_HAL_INFO_S;


HI_S32 VPSS_HAL_Init(VPSS_IP_E enIP);
HI_S32 VPSS_HAL_DelInit(VPSS_IP_E enIP);

HI_S32 VPSS_HAL_SetClockEn(VPSS_IP_E enIP, HI_BOOL bClockEn);
HI_S32 VPSS_HAL_GetClockEn(VPSS_IP_E enIP, HI_BOOL *pbClockEn);

HI_S32 VPSS_HAL_GetIntState(VPSS_IP_E enIP, HI_U32* pu32IntState);
HI_S32 VPSS_HAL_ClearIntState(VPSS_IP_E enIP, HI_U32 u32IntState);

HI_S32 VPSS_HAL_SetNodeInfo(VPSS_IP_E enIP,
     VPSS_HAL_INFO_S *pstHalInfo,  VPSS_HAL_TASK_NODE_E enTaskNodeId);

HI_S32 VPSS_HAL_StartLogic(VPSS_IP_E enIP, 
    HI_BOOL abNodeVaild[VPSS_HAL_TASK_NODE_BUTT]);

HI_S32 VPSS_HAL_GetSCDInfo(HI_U32* pu32AppVir,HI_S32 s32SCDInfo[32]);

HI_VOID VPSS_HAL_GetDetPixel(HI_U32* pu32AppVir,HI_U32 BlkNum, HI_U8* pstData);

HI_S32 VPSS_HAL_GetBaseRegAddr(VPSS_IP_E enIP,
                                 HI_U32 *pu32PhyAddr,
                                 HI_U8 **ppu8VirAddr);
HI_S32 VPSS_HAL_SetAlgParaAddr(HI_U32* pu32AppVir,HI_U32 u32AppPhy);
HI_VOID VPSS_HAL_DumpReg(VPSS_IP_E enIP, VPSS_HAL_TASK_NODE_E enTaskNodeId);
#endif

