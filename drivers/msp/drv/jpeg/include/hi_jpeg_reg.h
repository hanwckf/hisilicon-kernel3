/******************************************************************************
*
* Copyright (C) 2015 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
* This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon), 
* and may not be copied, reproduced, modified, disclosed to others, published or used, in
* whole or in part, without the express prior written permission of Hisilicon.
*
******************************************************************************
File Name	    : hi_jpeg_reg.h
Version		    : Initial Draft
Author		    : 
Created		    : 2015/03/01
Description	    : ����Ĵ���
Function List 	: 

			  		  
History       	:
Date				Author        		Modification
2015/03/01		    y00181162  		    Created file      	
******************************************************************************/

#ifndef __HI_JPEG_REG_H__
#define __HI_JPEG_REG_H__


/*********************************add include here******************************/
#include "hi_jpeg_config.h"
#include "hi_type.h"

/*****************************************************************************/


#ifdef __cplusplus
#if __cplusplus
   extern "C" 
{
#endif
#endif /* __cplusplus */


    /***************************** Macro Definition ******************************/
	#define JPGD_IRQ_NUM				          (97 + 32)
	#if defined(CHIP_TYPE_hi3796cv100) || defined(CHIP_TYPE_hi3798cv100)
		#define JPGD_REG_BASEADDR			      (0xFF300000)
	#else
		#define JPGD_REG_BASEADDR				  (0xf8c40000)
	#endif
	#ifndef CONFIG_JPEG_USE_SDK_CRG_ENABLE
		#define JPGD_CRG_REG_PHYADDR		      (0xf8a22000 + 0X7C)
	#endif

	#define JPGD_MMU_REG_BASEADDR				  (JPGD_REG_BASEADDR + 0xF000)

	#define JPGD_CLOCK_SELECT			          0x000 				/** ʱ��Ƶ��ѡ��,�����200MHz **/
	#define JPGD_CLOCK_ON				          0x1					/** ��ʱ�ӣ���������	  **/
	#define JPGD_CLOCK_OFF				          0xFFFFFFFE		    /** �ر�ʱ�ӣ���������� 	 **/
	#define JPGD_RESET_REG_VALUE 		          0x10					/** ��λ����������		  **/
	#define JPGD_UNRESET_REG_VALUE		          0xFFFFFFEF		    /** ����λ�����������		 **/
	
    /** �Ĵ������ȣ����߼��������ӵ�ʱ��ȷ�ϸó����Ƿ���� **/
	#ifdef CONFIG_JPEG_TEST_CHIP_PRESS
	/**FPGA���Ե�ʱ���߼�������һ����ѹ��ģ�飬����һЩ�Ĵ���
	 **����ӳ�䳤��Ҫ��һЩ **/
	#define JPGD_REG_LENGTH                          0xFF30
	#else
	/** the length of register */
	/** CNcomment:jpeg�Ĵ������ȣ�ע��Ҫ�������мĴ��� */
	#define JPGD_REG_LENGTH					         0x6BF  /** <64K  **/
	#endif
	
	#define JPGD_CRG_REG_LENGTH				         0x4

#if 0
	#define JPGD_MMU_REG_LENGTH				         0x340
#endif

	/** JPEG register that decoding start */
	/** CNcomment:jpeg ��ʼ����Ĵ��� */
	#define JPGD_REG_START						 	 0x0
	/** JPEG continue stream register */
	/** CNcomment:jpeg ����������Ĵ��� */
	#define JPGD_REG_RESUME 						 0x4
	/** the pic_vld_num register */
	/** CNcomment:jpeg ÿ֡�Ĳв�Ĵ��� */
	#define JPGD_REG_PICVLDNUM					 	 0x8
	/** VHB stride register */
	/** CNcomment:jpeg �м��Ĵ��� */
	#define JPGD_REG_STRIDE 						 0xC
	/** picture width register */
	/** CNcomment:jpeg ���Ⱥ͸߶ȼĴ��� */
	#define JPGD_REG_PICSIZE						 0x10
	/** picture type register */
	/** CNcomment:jpeg ͼƬ���ͼĴ��� */
	#define JPGD_REG_PICTYPE						 0x14
	/** picture decode time cost */
	/** CNcomment:jpeg ����ʱ��Ĵ��� */
	#define JPGD_REG_TIME                            0x18
	/** stream buffer start register */
	/** CNcomment:jpeg Ӳ��buffer��ʼ��ַ�Ĵ��� */
	#define JPGD_REG_STADDR 						 0x20
	/** stream buffer end register */
	/** CNcomment:jpeg Ӳ��buffer������ַ�Ĵ��� */
	#define JPGD_REG_ENDADDR						 0x24
	/** stream start address and end address must in hard start and end address */
	/** CNcomment:����buffer����ʼ��ַ�ͽ�����ַ������Ӳ��buffer��ʼ�ͽ����������� */
	/** stream saved start register */
	/** CNcomment:jpeg ����buffer��ʼ��ַ�Ĵ��� */
	#define JPGD_REG_STADD						 	 0x28
	/** stream save end register */
	/** CNcomment:jpeg ����buffer������ַ�Ĵ��� */
	#define JPGD_REG_ENDADD 						 0x2C
	/** luminance address register */
	/** CNcomment:���������ַ�Ĵ��� */
	#define JPGD_REG_YSTADDR						 0x30
	/** chrominance address register */
	/** CNcomment:ɫ�������ַ�Ĵ��� */
	#define JPGD_REG_UVSTADDR						 0x34
	/** scale register */
	/** CNcomment:���ű����Ĵ��� */
	#define JPGD_REG_SCALE						 	 0x40

#if defined(CONFIG_JPEG_HARDDEC2ARGB) || defined(CONFIG_JPEG_OUTPUT_YUV420SP)
    /** the dither register, used when output argb1555 */
	/** CNcomment:dither(argb1555���õ�)���˲������ͼ������ */
	#define JPGD_REG_OUTTYPE						 0x44
#endif

#ifdef CONFIG_JPEG_HARDDEC2ARGB
	/** the argb out stride */
	/** CNcomment:�������ARGBʱ����м�� */
	#define JPGD_REG_ARGBOUTSTRIDE		             0x1C
	/** the alpha register */
	/** CNcomment:alphaֵ�Ĵ��� */
	#define JPGD_REG_ALPHA						     0x48
	/** the crop start pos */
	/** CNcomment:�ü���ʼ���� */
	#define JPGD_REG_OUTSTARTPOS					 0xd8
	/** the crop end pos */
	/** CNcomment:�ü��������� */
	#define JPGD_REG_OUTENDPOS					     0xdc
	/** the hard need ddr buffer */
	/** CNcomment:rgb������߼���ҪDDR�ռ䣬��buffer */
	#define JPGD_REG_MINADDR						 0xc8
	/** the hard need ddr buffer */
	/** CNcomment:rgb������߼���ҪDDR�ռ䣬��buffer */
	#define JPGD_REG_MINADDR1						 0xcc
	
	#define JPGD_REG_HORCOEF00_01				     0x4C
	#define JPGD_REG_HORCOEF02_03				     0x50
	#define JPGD_REG_HORCOEF04_05				     0x54
	#define JPGD_REG_HORCOEF06_07				     0x58
	#define JPGD_REG_HORCOEF20_21				     0x6C
	#define JPGD_REG_HORCOEF22_23				     0x70
	#define JPGD_REG_HORCOEF24_25				     0x74
	#define JPGD_REG_HORCOEF26_27				     0x78
	
	#define JPGD_REG_VERCOEF00_01				     0x8C
	#define JPGD_REG_VERCOEF02_03				     0x90
	#define JPGD_REG_VERCOEF10_11				     0x94
	#define JPGD_REG_VERCOEF12_13				     0x98
	#define JPGD_REG_VERCOEF20_21				     0x9C
	#define JPGD_REG_VERCOEF22_23				     0xA0
	#define JPGD_REG_VERCOEF30_31				     0xA4
	#define JPGD_REG_VERCOEF32_33				     0xA8
	
	#define JPGD_REG_CSC00_01						 0xB4
	#define JPGD_REG_CSC02_10						 0xB8
	#define JPGD_REG_CSC11_12						 0xBC
	#define JPGD_REG_CSC20_21						 0xC0
	#define JPGD_REG_CSC22						     0xC4
#endif
	
		
	/** halt status register */
	/** CNcomment:jpeg �ж�״̬�Ĵ��� */
	#define JPGD_REG_INT							 0x100
	/** halt shield register */
	/** CNcomment:jpeg �ж�����Ĵ��� */
	#define JPGD_REG_INTMASK						 0x104
	/** debug register */
	/** CNcomment:jpeg ���ԼĴ���,��������Ӳ����ʱʱ�� */
	#define JPGD_REG_DEBUG						     0x108

#ifdef CONFIG_JPEG_OUTPUT_LUPIXSUM
	/** save lu pixle data register */
	/** CNcomment:ͳ��������Ϣ,32λȫ�� */
	#define JPGD_REG_LPIXSUM0						 0x114
	/** save lu pixle data register */
	/** CNcomment:ͳ��������Ϣ����4λ */
	#define JPGD_REG_LPIXSUM1						 0x118
#endif

#ifdef CONFIG_JPEG_4KDDR_DISABLE_SHOWMSG
	#define JPGD_REG_AXI4KCNT						 0x11C
	#define JPGD_REG_AXI16MCNT						 0x120
#endif
	
	#define JPGD_REG_SAMPLINGFACTOR			         0x124

    #define JPGD_REG_DRI   			                 0x128
    #define JPGD_REG_DRI_CNT_CFG			         0x12C
    #define JPGD_REG_DRI_CNT       			         0x130

    /** add mmu memory register */
	/** CNcomment:���Ӷ�mmu�ڴ�����ļĴ��� */
    #define JPGD_REG_MMU_BYPASS    			         0x134

	/** dqt set register */
	/** CNcomment:�������Ĵ��� */
	#define JPGD_REG_QUANT						     0x200
	/** Huffman  set register */
	/** CNcomment:huffman���Ĵ��� */
	#define JPGD_REG_HDCTABLE						 0x300
	/** Huffman AC mincode memory register */
	/** CNcomment:huffman��ac�Ĵ��� */
	#define JPGD_REG_HACMINTABLE					 0x340
	/** Huffman AC base memory register */
	/** CNcomment:huffman��ac�Ĵ��� */
	#define JPGD_REG_HACBASETABLE				     0x360
	/** Huffman AC symbol memory register */
	/** CNcomment:huffman��ac�Ĵ��� */
	#define JPGD_REG_HACSYMTABLE					 0x400
	
	
#ifdef CONFIG_JPEG_SUSPEND
	#define JPGD_REG_PD_CBCR_CFG					 0x38
	#define JPGD_REG_PD_Y_CFG						 0x3c
	#define JPGD_REG_MCUY_CFG						 0xd4
	#define JPGD_REG_MCUY							 0xe0
	#define JPGD_REG_BSRES						     0xe4
	#define JPGD_REG_BSRES_DATA0					 0xe8
	#define JPGD_REG_BSRES_DATA1					 0xec
	#define JPGD_REG_BSRES_BIT					     0xf0
	#define JPGD_REG_BSRES_DATA0_CFG				 0xf4
	#define JPGD_REG_BSRES_DATA1_CFG				 0xf8
	#define JPGD_REG_BSRES_BIT_CFG				     0xfc
	#define JPGD_REG_PD_Y							 0x10c
	#define JPGD_REG_PD_CBCR						 0x110
#endif


#ifdef CONFIG_JPEG_TEST_CHIP_PRESS
	/** ���Է�ѹ��صļĴ��� **/
	#define JPGD_REG_PRESS_BYPASS                  0xff00
	#define JPGD_REG_PRESS_FF04                    0xff04
	#define JPGD_REG_PRESS_FF08                    0xff08
	#define JPGD_REG_PRESS_FF0C                    0xff0c
	#define JPGD_REG_PRESS_FF10                    0xff10
	#define JPGD_REG_PRESS_FF20                    0xff20
#endif


#if 0
	/** ���mmu��ַ����ƫ�Ƶ�0xf000,��Щ�ǹ����Ĵ������ŵ������ļ��в��� **/
	#define JPGD_REG_SMMU_SCR                      0x0     /** SMMUȫ�ֿ��ƼĴ���           **/
	#define JPGD_REG_SMMU_LP_CTRL                  0x8     /** SMMU�͹��Ŀ��ƼĴ���         **/
	
	#define JPGD_REG_INTMASK_S                     0x10    /** SMMU��ȫ�ж����μĴ���       **/
	#define JPGD_REG_INTMASK_NS                    0x20    /** SMMU�ǰ�ȫ���μĴ���         **/
	
	#define JPGD_REG_SMMU_INTRAW_S                 0x14    /** SMMU��ȫ�ж�Դ�Ĵ���         **/
	#define JPGD_REG_SMMU_INTRAW_NS                0x24    /** SMMU�ǰ�ȫ�ж�Դ�Ĵ���       **/

	#define JPGD_REG_SMMU_INTSTAT_S                0x18    /** SMMU��ȫ�ж�״̬�Ĵ���       **/
	#define JPGD_REG_SMMU_INTSTAT_NS               0x28    /** SMMU�ǰ�ȫ�ж�״̬�Ĵ���     **/

	#define JPGD_REG_SMMU_INTCLR_S                 0x1C    /** SMMU��ȫ�ж�����Ĵ���       **/
	#define JPGD_REG_SMMU_INTCLR_NS                0x2C    /** SMMU�ǰ�ȫ�ж�����Ĵ���     **/

	#define JPGD_REG_SMMU_SCB_TTBR                 0x208    /** SMMU��ȫҳ������ַ�Ĵ���    **/
	#define JPGD_REG_SMMU_CB_TTBR                  0x20C    /** SMMU�ǰ�ȫҳ������ַ�Ĵ���  **/

	#define JPGD_REG_SMMU_ERR_RDADDR               0x304    /** SMMU�������ַĬ�ϼĴ���    **/
	#define JPGD_REG_SMMU_ERR_WRADDR               0x308    /** SMMUд�����ַĬ�ϼĴ���    **/

	/** ��ʱû���õ��� **/
	#define JPGD_REG_SMMU_FAULT_ADDR_PTW_S         0x310    /** SMMU��ȫԤȡ�����ַ�Ĵ���     **/
	#define JPGD_REG_SMMU_FAULT_ID_PTW_S           0x314    /** SMMU��ȫԤȡ����ID�Ĵ���       **/
	#define JPGD_REG_SMMU_FAULT_ADDR_PTW_NS        0x320    /** SMMU�ǰ�ȫԤȡ�����ַ�Ĵ���   **/
	#define JPGD_REG_SMMU_FAULT_ID_PTW_NS          0x324    /** SMMU�ǰ�ȫԤȡ����ID�Ĵ���     **/
	#define JPGD_REG_SMMU_FAULT_PTW_NUM            0x328    /** SMMUԤȡ����PTWQ���мĴ���     **/
	#define JPGD_REG_SMMU_FAULT_ADDR_WR_S          0x330    /** SMMU��ȫд���������ַ�Ĵ���   **/
	#define JPGD_REG_SMMU_FAULT_TLB_WR_S           0x334    /** SMMU_FAULT_TLB_WR_S            **/
	#define JPGD_REG_SMMU_FAULT_ID_WR_S            0x338    /** SMMU��ȫд����ID�Ĵ���         **/
	#define JPGD_REG_SMMU_FAULT_ADDR_WR_NS         0x340    /** SMMU�ǰ�ȫд���������ַ�Ĵ��� **/
	#define JPGD_REG_SMMU_FAULT_TLB_WR_NS          0x344    /** SMMU�ǰ�ȫд����ҳ���Ĵ���     **/
	#define JPGD_REG_SMMU_FAULT_ID_WR_NS           0x348    /** SMMU�ǰ�ȫд����ID�Ĵ���       **/
	#define JPGD_REG_SMMU_FAULT_ADDR_RD_S          0x350    /** SMMU��ȫ�����������ַ�Ĵ���   **/
	#define JPGD_REG_SMMU_FAULT_TLB_RD_S           0x354    /** SMMU��ȫ������ҳ���Ĵ���       **/
	#define JPGD_REG_SMMU_FAULT_ID_RD_S            0x358    /** SMMU��ȫ������ID�Ĵ���         **/
	#define JPGD_REG_SMMU_FAULT_ADDR_RD_NS         0x360    /** SMMU�ǰ�ȫ�����������ַ�Ĵ��� **/
	#define JPGD_REG_SMMU_FAULT_TLB_RD_NS          0x364    /** SMMU�ǰ�ȫ������ҳ���Ĵ���     **/
	#define JPGD_REG_SMMU_FAULT_ID_RD_NS           0x368    /** SMMU�ǰ�ȫ������ID�Ĵ���       **/
	#define JPGD_REG_SMMU_FAULT_TBU_INFO           0x36C    /** SMMU TBU������Ϣ�Ĵ���         **/
	#define JPGD_REG_SMMU_FAULT_TBU_DBG            0x370    /** SMMU TBU DBG��Ϣ�Ĵ���         **/
	#define JPGD_REG_SMMU_PREF_BUFFER_EMPTY        0x374    /** SMMUԤȡbuffer��״̬�Ĵ���     **/
	#define JPGD_REG_SMMU_PTWQ_IDLE                0x378    /** SMMUԤȡPTWQ����״̬�Ĵ���     **/
	#define JPGD_REG_SMMU_RESET_STATE              0x37C    /** SMMU��λ״̬�Ĵ���             **/
	#define JPGD_REG_SMMU_MASTER_DBG0              0x380    /** SMMU MASTER DBG0�Ĵ���         **/
	#define JPGD_REG_SMMU_MASTER_DBG1              0x384    /** SMMU MASTER DBG1�Ĵ���         **/
	#define JPGD_REG_SMMU_MASTER_DBG2              0x388    /** SMMU MASTER DBG2�Ĵ���         **/
	#define JPGD_REG_SMMU_MASTER_DBG3              0x38C    /** SMMU MASTER DBG3�Ĵ���         **/
#endif
	
    /*************************** Structure Definition ****************************/

    /********************** Global Variable declaration **************************/


    /******************************* API declaration *****************************/
	/*****************************************************************************
	* func			  : jpeg_reg_read
	* description	  : read register value
	* param[in] 	  : offset
	* retval		  : none
	* output		  : none
	* others:		  : nothing
	*****************************************************************************/
	HI_U32 jpeg_reg_read(HI_U32 offset);

	/*****************************************************************************
	* func			  : jpeg_reg_write
	* description	  : write register value
	* param[in] 	  : offset
	* param[in] 	  : value
	* retval		  : none
	* output		  : none
	* others:		  : nothing
	*****************************************************************************/
	HI_VOID  jpeg_reg_write(HI_U32 offset, HI_U32 value);

    /*****************************************************************************
    * func			  : jpeg_reg_write
    * description	  : write register value
    * param[in] 	  : offset
    * param[in] 	  : value
    * retval		  : none
    *****************************************************************************/
    HI_VOID jpeg_reg_writebuf(const HI_VOID *pInMem,HI_S32 s32PhyOff,HI_U32 u32Bytes);

#ifdef __cplusplus
    
#if __cplusplus
   
}
#endif
#endif /* __cplusplus */

#endif /* __HI_JPEG_REG_H__ */
