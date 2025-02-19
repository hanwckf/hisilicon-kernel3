/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWinjiDDUykL9e8pckESWBbMVmSWkBuyJO01cTiy3TdzKxGk0oBQa
mSMf7J4FkTpfvy2G1NnyQwzUAGIEtSnc4g6WfcmyrindyKEmSRsJJCf0/FaCG8z2Rdw1dJuN
tl2RyN2MPlcNNXmmOB5tNJDLRXFaVrUICrSY9JkqMCZ+BtQaYqGs4qIs9OCErQNfYdSzKGQl
9Mbxcfm/tPR2qa38CVN7avMQj9pFOIZCCFve9VuXeDBsCS+vh2t+2juESROb9A==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
#ifndef __VPSS_REG_STRUCT_H__
#define __VPSS_REG_STRUCT_H__
#include "hi_drv_video.h"
#include "vpss_common.h"
typedef enum hiVPSS_REG_PORT_E
{
    VPSS_REG_STR = 0,
    VPSS_REG_HD,
    VPSS_REG_SD,
    VPSS_REG_BUTT
}VPSS_REG_PORT_E;

typedef enum hiVPSS_REG_PREZME_E{
    PREZME_DISABLE = 0,
    PREZME_2X,
    PREZME_4X,
    PREZME_8X,
	PREZME_VFIELD,
    PREZME_BUTT
}VPSS_REG_PREZME_E;
typedef enum hiREG_FIELDPOS_E
{
    LAST_FIELD = 0,
    CUR_FIELD,
    NEXT1_FIELD,
    NEXT2_FIELD,
    NEXT3_FIELD,
    BUTT_FIELD
}REG_FIELDPOS_E;


typedef enum hiREG_FRAMEPOS_E
{
    LAST_FRAME = 0,
    CUR_FRAME,
    NEXT_FRAME,
    BUTT_FRAME
}REG_FRAMEPOS_E;

typedef enum hiREG_CHANELPOS_E
{
    CUR_CHANEL = 0,
    NEXT1_CHANEL,
    NEXT2_CHANEL,
    REF_CHANEL,
    BUTT_CHANEL
}REG_CHANELPOS_E;


typedef enum hiREG_TUNLPOS_E
{
    ROW_2_WIRTE_TUNL = 0,
    ROW_4_WIRTE_TUNL,
    ROW_8_WIRTE_TUNL,
    ROW_16_WIRTE_TUNL,
    BUTT_WIRTE_TUNL
}REG_TUNLPOS_E;
//#endif


typedef enum hiREG_ZME_MODE_E
{
    REG_ZME_MODE_HOR = 0,
    REG_ZME_MODE_VER,

    REG_ZME_MODE_HORL,  
    REG_ZME_MODE_HORC,  
    REG_ZME_MODE_VERL,
    REG_ZME_MODE_VERC,

    REG_ZME_MODE_ALL,
    REG_ZME_MODE_NONL,
    REG_ZME_MODE_BUTT
      
}REG_ZME_MODE_E;


typedef enum hiREG_PIX_BITW_E
{
    REG_PIX_8BIT = 0,
    REG_PIX_10BIT,
    REG_PIX_BUTT

}REG_PIX_BITW_E;

typedef enum hiREG_DITHER_MODE_E
{
    REG_DITHER_MODE_DITHER = 0,
    REG_DITHER_MODE_ROUND,
    REG_DITHER_MODE_BUTT

}REG_DITHER_MODE_E;

typedef enum hiREG_AXI_CHN_E
{
    REG_AXI_AR = 0,
    REG_AXI_AW,
    REG_AXI_BUTT

}REG_AXI_CHN_E;

typedef enum hiREG_AXI_RID_CHN_E
{
    REG_NX2C_RID = 0,
    REG_NX2Y_RID,
    REG_NX1C_RID,
    REG_NX1Y_RID,
    REG_REFC_RID,
    REG_REFY_RID,
    REG_CURC_RID,
    REG_CURY_RID,
    REG_CAS_RID,
    REG_PR0C_ID,
    REG_PR2C_ID,
    REG_PR2Y_ID,
    REG_PR1C_ID,
    REG_PR1Y_ID,
    REG_NX3C_ID,
    REG_NX3Y_ID,
    REG_CCRC_ID,
    REG_CCRY_ID,
    REG_SRMD_ID,
    REG_TRMD_ID,
    REG_RST_RID,
    REG_DS_ID,
    REG_PRJ_ID,
    REG_RGMV_ID,
    REG_BLKMV_ID,
    REG_REEC_ID,
    REG_REEY_ID,
    REG_AIX_RID_BUTT

}REG_AXI_RID_CHN_E;
typedef enum hiREG_SMMU_CHN_E
{
    REG_SMMU_RCH_RUNL,
    REG_SMMU_RCH_CCNT,
    REG_SMMU_RCH_YCNT,
    REG_SMMU_RCH_SRMD,
    REG_SMMU_RCH_TRMD,
    REG_SMMU_RCH_BLKMV_CUR,
    REG_SMMU_RCH_BLKMV_REF,
    REG_SMMU_RCH_RGMV_CUR,
    REG_SMMU_RCH_RGMV_NX1,
    REG_SMMU_RCH_PRJH_CUR,
    REG_SMMU_RCH_PRJV_CUR,
    REG_SMMU_RCH_DS_REF,
    REG_SMMU_RCH_DS_NX1,
    REG_SMMU_RCH_REEC,
    REG_SMMU_RCH_REEY,
    REG_SMMU_RCH_RST,
    REG_SMMU_RCH_PR0C,
    REG_SMMU_RCH_PR2C,
    REG_SMMU_RCH_PR2Y,
    REG_SMMU_RCH_PR1C,
    REG_SMMU_RCH_PR1Y,
    REG_SMMU_RCH_NX3C,
    REG_SMMU_RCH_NX3Y,
    REG_SMMU_RCH_NX2C,
    REG_SMMU_RCH_NX2Y,
    REG_SMMU_RCH_NX1C,
    REG_SMMU_RCH_NX1Y,
    REG_SMMU_RCH_REFC,
    REG_SMMU_RCH_REFY,
    REG_SMMU_RCH_CURC,
    REG_SMMU_RCH_CURY,
    REG_SMMU_RCH_BLKMV_NX1,
    
    REG_SMMU_WCH_STT,
    REG_SMMU_WCH_CCNT,
    REG_SMMU_WCH_YCNT,
    REG_SMMU_WCH_TWMD,
    REG_SMMU_WCH_WST,
    REG_SMMU_WCH_BLKMVNX2,
    REG_SMMU_WCH_RGMVNX2,
    REG_SMMU_WCH_PRJHNX2,
    REG_SMMU_WCH_PRJVNX2,
    REG_SMMU_WCH_DSNX2,
    REG_SMMU_WCH_CUEC,
    REG_SMMU_WCH_CUEY,
    REG_SMMU_WCH_RFRC,
    REG_SMMU_WCH_RFRY,
    REG_SMMU_WCH_VHD0C,
    REG_SMMU_WCH_VHD0Y,

    REG_SMMU_CHN_BUTT

}REG_SMMU_CHN_E;

typedef enum hiREG_DIE_MODE_E
{
    REG_DIE_MODE_CHROME = 0,
    REG_DIE_MODE_LUMA,
    REG_DIE_MODE_ALL,
    REG_DIE_MODE_BUTT
      
}REG_DIE_MODE_E;

typedef enum hiREG_BURST_LEN_E
{
    REG_BURST_LEN_16 = 0,
    REG_BURST_LEN_8,
    REG_BURST_LEN_4,
    REG_BURST_LEN_BUTT
      
}REG_BURST_LEN_E;

//--------------------------------------------
//ADD in HiFone B02 by z214841
typedef enum hiVPSS_REG_ADDR_E{
	REG_VPSS_VC1_STR_ADDR,
	REG_VPSS_ZME_ADDR,
	REG_VPSS_HSP_ADDR,
	REG_VPSS_DB_ADDR,
	REG_VPSS_DR_ADDR,
	REG_VPSS_TNR_ADDR,
	REG_VPSS_TNR_CLUT_ADDR,
	REG_VPSS_DBM_ADDR,
	REG_VPSS_DEI_ADDR,
	REG_VPSS_CHK_SUM_W_ADDR,
	REG_VPSS_VHD0_LCMP_ADDR,
	REG_VPSS_PNEXT_ADDR,
}VPSS_REG_ADDR_E;
#define VPSS_ZME_ADDR_GET(appviraddr)	((appviraddr) + 0x2000)



typedef enum hiVPSS_REG_VHD0_LCMP_E{
	VPSS_VHD0_LCMP_Y,
	VPSS_VHD0_LCMP_C
}VPSS_REG_VHD0_LCMP_E;

typedef enum hiVPSS_REG_ZME_FMT_E
{
	ZME_FMT_422,
	ZME_FMT_420
}VPSS_REG_ZME_FMT_E;

/*
@f00241306 add CMP structure definition
*/
typedef struct 
{
    HI_U32 u32Width;  
    HI_U32 u32Height;
    HI_U32 u32Cfg0_Max_rg_comp_bits;  
    HI_U32 u32Cfg0_Ri_bits; 
    HI_U32 u32Cfg0_Mb_bits;
    HI_U32 u32Cfg1_Rc_smth_ngain;  
    HI_U32 u32Cfg1_Sad_bits_ngain; 
    HI_U32 u32Cfg1_Max_qp;
    HI_U32 u32Cfg2_Pix_diff_thr;  
    HI_U32 u32Cfg2_Smth_pix_num_thr; 
    HI_U32 u32Cfg2_Smth_thr;
    HI_U32 u32Cfg3_Adj_sad_bit_thr; 
    HI_U32 u32Cfg3_Adj_sad_thr; 
    HI_U32 u32Cfg4_Qp_chg2_bits_thr; 
    HI_U32 u32Cfg4_Qp_chg1_bits_thr;
    HI_U32 u32Cfg5_Smth_lftbits_thr1;
    HI_U32 u32Cfg5_Smth_lftbits_thr0;
    HI_U32 u32Cfg5_Smth_qp1;
    HI_U32 u32Cfg5_Smth_qp0;
    
}REG_CMP_DATA_S;
typedef enum hiREG_DBDET_MODE_E
{
    REG_DBDET_MODE_SIZE = 0,
    REG_DBDET_MODE_VY,
    REG_DBDET_MODE_HY,
    REG_DBDET_MODE_HC
}REG_DBDET_MODE_E;

typedef enum hiREG_DB_MODE_E
{
    REG_DB_MODE_VC = 0,
    REG_DB_MODE_VY,
    REG_DB_MODE_HY,
    REG_DB_MODE_HC
}REG_DB_MODE_E;


typedef enum hiREG_CHN_ARBITRATE_E
{
	REG_R_CURY = 0,
	REG_R_CURC,
	REG_R_REFY,
	REG_R_REFC,
	REG_R_NXT1Y,
	REG_R_NXT1C,
	REG_R_NXT2Y,
	REG_R_NXT2C,
	REG_R_NXT3Y,
	REG_R_NXT3C,
	REG_R_PR0C,
	REG_R_PR1Y,
	REG_R_PR1C,
	REG_R_PR2Y,
	REG_R_PR2C,
	REG_R_RFRY,
	REG_R_RFRC,
	REG_W_VHD0Y,
	REG_W_VHD0C,
	REG_W_VSDY,
	REG_W_VSDC,
	REG_R_ST,
	REG_W_ST,
	REG_R_YCNT,
	REG_R_CCNT,
	REG_W_YCNT,
	REG_W_CCNT,
	REG_R_TNR_MAD,
	REG_W_TNR_MAD,
	REG_R_SNR_MAD,
	REG_W_STT,
	REG_W_CHK_SUM,

    REG_CHN_ARBITRATE_BUTT
}REG_CHN_ARBITRATE_E;
#endif
