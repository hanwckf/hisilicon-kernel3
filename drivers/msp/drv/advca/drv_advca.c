/******************************************************************************

  Copyright (C), 2011-2014, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : drv_advca.c
  Version       : Initial Draft
  Author        : Hisilicon hisecurity team
  Created       :
  Last Modified :
  Description   :
  Function List :
  History       :

******************************************************************************/
#include "hi_unf_advca.h"
#include "drv_advca_ext.h"
#include "drv_advca.h"
#include "hi_module.h"
#include "hi_common.h"
#include "hi_drv_cipher.h"
#include "hi_drv_otp.h"
#include "hi_drv_sys.h"
#include "hi_drv_dev.h"
#include "hi_drv_proc.h"
#include "hi_drv_module.h"
#include "hi_error_mpi.h"
#if defined(SDK_SECURITY_ARCH_VERSION_V2) && defined(ADVCA_NAGRA)
#include <linux/notifier.h>
#endif
#include "drv_advca_ioctl.h"
#include "drv_advca.h"
#include "drv_otp_ext.h"
#include "hi_drv_struct.h"
#include "drv_advca_internal.h"
#ifdef SDK_SECURITY_ARCH_VERSION_V2
#include "hi_struct.h"
#endif

OTP_EXPORT_FUNC_S *g_pOTPExportFunctionList = HI_NULL;

static ADVCA_EXPORT_FUNC_S s_CaExportFunctionList =
{
    .pfnAdvcaCrypto  = HI_DRV_ADVCA_Crypto,
	.pfnAdvcaSuspend = HI_DRV_ADVCA_Suspend,
	.pfnAdvcaResume  = HI_DRV_ADVCA_Resume,
};

extern HI_VOID HI_DRV_SYS_GetChipVersion(HI_CHIP_TYPE_E *penChipType, HI_CHIP_VERSION_E *penChipVersion);

HI_S32 DRV_ADVCA_GetChipVersion(HI_CHIP_TYPE_E *penChipType, HI_CHIP_VERSION_E *penChipVersion)
{
    if ((NULL == penChipType) || (NULL == penChipVersion))
    {
        HI_FATAL_CA("CA get chip version failed!\n");
        return HI_FAILURE;
    }

    (HI_VOID)HI_DRV_SYS_GetChipVersion(penChipType, penChipVersion);

    return HI_SUCCESS;
}

#if defined(SDK_SECURITY_ARCH_VERSION_V2) && defined(ADVCA_NAGRA)
extern int register_reboot_notifier(struct notifier_block *nb);

static int DRV_ADVCA_SystemReboot(struct notifier_block *nb, unsigned long event, void *unused)
{
    HI_U32 u32Stat = 0;
    HI_U8 * pu8VirAddr = HI_NULL;
    
    pu8VirAddr= (HI_U8*)ioremap_nocache(OTP_CTRL_ADDR,0x10);
    if(HI_NULL == pu8VirAddr)
    {
        HI_ERR_OTP("ioremap_nocache map error\n");
        return HI_FAILURE;
    }
    /*Set OTP read address to CA key*/
	DRV_ADVCA_SysWriteReg(pu8VirAddr, 0x0);
    iounmap(pu8VirAddr);

    pu8VirAddr= (HI_U8*)ioremap_nocache(OTP_CHANGE_MODE_ADDR,0x10);
    if(HI_NULL == pu8VirAddr)
    {
        HI_ERR_OTP("ioremap_nocache map error\n");
        return HI_FAILURE;
    }
	/*Set OTP read mode to differencial mode*/
	DRV_ADVCA_SysWriteReg(pu8VirAddr, 0x2);

	while (1)
	{
	    u32Stat = DRV_ADVCA_SysReadReg(pu8VirAddr);
	    if (0 != (u32Stat & 0x08))
	    {
	        break;
	    }
	    ca_mdelay(1);
	}
    iounmap(pu8VirAddr);
	ca_mdelay(10);

	return HI_SUCCESS;
}

static struct notifier_block advca_reboot_notifier = {
	.notifier_call = DRV_ADVCA_SystemReboot,
};
#endif

HI_VOID DRV_ADVCA_V200_RegisterRebootNotifier(HI_VOID)
{
#if defined(SDK_SECURITY_ARCH_VERSION_V2) && defined(ADVCA_NAGRA)
    register_reboot_notifier(&advca_reboot_notifier);
#endif
    return;
}

HI_S32 CA_Ioctl(struct inode *inode, struct file *file, unsigned int cmd, void* arg)
{
    HI_S32 ret = HI_SUCCESS;
    ret = DRV_ADVCA_V300_Ioctl(cmd, arg);

    return ret;
}

#if 0
/* only for Hi3716X!!! */
HI_S32 DRV_ADVCA_GetOTPFunction(HI_VOID* pstOTPFunction)
{
	if( NULL == pstOTPFunction)
	{
		return HI_FAILURE;
	}
	ca_memset(pstOTPFunction, 0x0, sizeof(OTP_EXPORT_FUNC_S));

	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V200_Read = HAL_OTP_V200_Read;
 	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V200_ReadByte = HAL_OTP_V200_ReadByte;
 	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V200_Write = HAL_OTP_V200_Write;
 	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V200_WriteByte = HAL_OTP_V200_WriteByte;
 	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V200_WriteBit = HAL_OTP_V200_WriteBit;

	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_WriteByte = HAL_OTP_V100_WriteByte;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_WriteBit = HAL_OTP_V100_WriteBit;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_SetWriteProtect = HAL_OTP_V100_SetWriteProtect;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_GetWriteProtect = HAL_OTP_V100_GetWriteProtect;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_Read = HAL_OTP_V100_Read;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_GetSrBit = HAL_OTP_V100_GetSrBit;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_SetSrBit = HAL_OTP_V100_SetSrBit;
	((OTP_EXPORT_FUNC_S *)pstOTPFunction)->HAL_OTP_V100_FuncDisable = HAL_OTP_V100_FuncDisable;

	return	HI_SUCCESS;
}
#endif

int DRV_ADVCA_Open(struct inode *inode, struct file *filp)
{
    HI_U32 ret = HI_SUCCESS;

    ret = DRV_ADVCA_V300_Open();
    if( HI_SUCCESS != ret)
    {
        HI_FATAL_CA("DRV_ADVCA_V300_Open failed!\n");
        return HI_FAILURE;
    }

    if( NULL == g_pOTPExportFunctionList)
    {
        ret = HI_DRV_MODULE_GetFunction(HI_ID_OTP, (HI_VOID**)&g_pOTPExportFunctionList);
        if( NULL == g_pOTPExportFunctionList)
        {
            HI_FATAL_CA("Get otp functions failed!\n");
            return HI_FAILURE;
        }
    }

    return ret;
}

int DRV_ADVCA_Release(struct inode *inode, struct file *filp)
{
    HI_U32 ret = HI_SUCCESS;

    ret = DRV_ADVCA_V300_Release();

    return ret;
}

HI_S32 HI_DRV_ADVCA_Resume(HI_VOID)
{
    return HI_SUCCESS;
}

HI_S32 HI_DRV_ADVCA_Suspend(HI_VOID)
{
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype    :
 Description  : CA Module interface for Demux Chipset Pairing
                It is to set DVB level 2 Input
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/
HI_S32 DRV_ADVCA_DecryptCws(HI_U32 AddrID,HI_U32 EvenOrOdd, HI_U8 *pu8Data)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu8Data)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }


    ret = HAL_ADVCA_V300_GetCWLadderLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_DecryptCw(enKeyladderLevel, (HI_U32 *)pu8Data, AddrID, EvenOrOdd);


    return ret;
}

/*****************************************************************************
 Prototype    :
 Description  : CA Module interface for Demux Chipset Pairing
                It is to set DVB level 2 Input
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/
HI_S32 DRV_ADVCA_DecryptDCAS(HI_U32 enLevel, HI_U32 AddrID,HI_U32 EvenOrOdd, HI_U8 *pu8Data, HI_UNF_ADVCA_CA_TARGET_E enKlTarget)
{
    HI_S32 ret = HI_SUCCESS;

    if(NULL == pu8Data)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret = HAL_ADVCA_V300_DecryptDCAS(enLevel, (HI_U32 *)pu8Data, AddrID, EvenOrOdd, NULL, enKlTarget);
 
    return ret;
}

HI_S32 DRV_ADVCA_DecryptCsa3s(HI_U32 AddrID,HI_U32 EvenOrOdd, HI_U8 *pu8Data)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu8Data)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret = HAL_ADVCA_V300_GetCSA3LadderLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_DecryptCsa3(enKeyladderLevel, (HI_U32 *)pu8Data, AddrID, EvenOrOdd);

    return ret;
}

/*****************************************************************************
 Prototype    :
 Description  : Conax Module interface for Demux Chipset Pairing
                It is to set DVB level 2 Input
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/

HI_S32 DRV_ADVCA_DecryptSPs(HI_U32 AddrID,HI_U32 EvenOrOdd, HI_U8 *pu8Data,DRV_ADVCA_CA_TARGET_E enCwTarget)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu8Data)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret = HAL_ADVCA_V300_GetSPLadderLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_DecryptSP(enKeyladderLevel, (HI_U32*)pu8Data, AddrID, EvenOrOdd, enCwTarget);

    return ret;
}

/*****************************************************************************
 Prototype    :
 Description  : CA Module interface for Cipher
                It is to set Cipher level 2 Input
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/
HI_S32 DRV_ADVCA_DecryptCipher(HI_U32 AddrID, HI_U32 *pu32DataIn)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu32DataIn)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret = HAL_ADVCA_V300_GetR2RLadderLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_CryptR2R(enKeyladderLevel, pu32DataIn, AddrID, HI_TRUE);

    return ret;
}

HI_S32 DRV_ADVCA_EncryptCipher(HI_U32 AddrID, HI_U32 *pu32DataIn)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu32DataIn)
    {
        HI_ERR_CA("Error! Invald param, null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret  = HAL_ADVCA_V300_GetR2RLadderLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_CryptR2R(enKeyladderLevel, pu32DataIn, AddrID, HI_FALSE);

    return ret;
}

HI_S32 DRV_ADVCA_DecryptMisc(HI_U32 AddrID,HI_U32 EvenOrOdd, HI_U8 *pu8DataIn, HI_UNF_ADVCA_CA_TARGET_E enKlTarget)
{
    HI_S32 ret = HI_SUCCESS;

    HI_UNF_ADVCA_KEYLADDER_LEV_E enKeyladderLevel = HI_UNF_ADVCA_KEYLADDER_BUTT;

    if(NULL == pu8DataIn)
    {
        HI_ERR_CA("Error, Null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret  = HAL_ADVCA_V300_GetMiscKlLevel(&enKeyladderLevel);
    ret |= HAL_ADVCA_V300_DecryptMisc(enKeyladderLevel, (HI_U32 *)pu8DataIn, AddrID, EvenOrOdd, (DRV_ADVCA_CA_TARGET_E)enKlTarget);

    return ret;
}

HI_S32 DRV_ADVCA_CryptGDRM(HI_U32 AddrID,
                    HI_U32 *pu32DataIn,
                    HI_BOOL bIsDeCrypt,
                    HI_UNF_ADVCA_CA_TARGET_E enKlTarget)
{
    HI_S32 ret = HI_SUCCESS;

    if(NULL == pu32DataIn)
    {
        HI_ERR_CA("Error, Null pointer!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    ret = HAL_ADVCA_V300_CryptGDRM(HI_UNF_ADVCA_KEYLADDER_LEV3, pu32DataIn, AddrID, bIsDeCrypt, enKlTarget);

    return ret;
}

HI_S32 HI_DRV_ADVCA_Crypto(DRV_ADVCA_EXTFUNC_PARAM_S stParam)
{
    HI_S32 ret = HI_SUCCESS;

    if(HI_UNF_CIPHER_CA_TYPE_SP == stParam.enCAType)  //transfer from Demux, cipher engine
    {
        if(g_MiscInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptMisc(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);        
        }
        else
        {
            ret = DRV_ADVCA_DecryptSPs(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, (DRV_ADVCA_CA_TARGET_E)stParam.enTarget);
        }
    }
    else if(HI_UNF_CIPHER_CA_TYPE_R2R == stParam.enCAType)//transfer from cipher engine
    {
        if(g_MiscInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptMisc(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);        
        }
        else if(g_SPInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptSPs(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, (DRV_ADVCA_CA_TARGET_E)stParam.enTarget);
        }
        else if(g_DCASInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptDCAS(0x05, stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);
        }
        else
        {
            ret = DRV_ADVCA_DecryptCipher(stParam.AddrID, (HI_U32 *)stParam.pu8Data);
        } 
    }
    else if(HI_UNF_CIPHER_CA_TYPE_CSA2 == stParam.enCAType) //transfer from Demux
    {
        if(g_MiscInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptMisc(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);        
        }
        else if(g_SPInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptSPs(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, (DRV_ADVCA_CA_TARGET_E)stParam.enTarget);
        }
        else if(g_DCASInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptDCAS(0x04, stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);
        }
        else
        {
            ret = DRV_ADVCA_DecryptCws(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data);
        }        
    }
    else if(HI_UNF_CIPHER_CA_TYPE_CSA3 == stParam.enCAType) //transfer from Demux
    {
        if(g_MiscInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptMisc(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);        
        }
        else if(g_SPInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptSPs(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, (DRV_ADVCA_CA_TARGET_E)stParam.enTarget);
        }
        else if(g_DCASInfo.OpenFlag == HI_TRUE)
        {
            ret = DRV_ADVCA_DecryptDCAS(0x04, stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data, stParam.enTarget);
        }
        else
        {
            ret = DRV_ADVCA_DecryptCsa3s(stParam.AddrID, stParam.EvenOrOdd, stParam.pu8Data);
        }
    }
    else if(HI_UNF_CIPHER_CA_TYPE_GDRM == stParam.enCAType)  //transfer from cipher engine
    {
        
    }
    else if(HI_UNF_CIPHER_CA_TYPE_BLPK == stParam.enCAType) //transfer from cipher engine
    {
        ret = HAL_ADVCA_V300_DecryptSWPK((HI_U32 *)stParam.pu8Data, stParam.AddrID);
    }
    else if(HI_UNF_CIPHER_CA_TYPE_IRDETO_HCA == stParam.enCAType)
    {
        ret = DRV_ADVCA_DecryptIrdetoHCA(stParam.AddrID);
    }
    else
    {
        HI_ERR_CA("Invalid CA type!\n");
        return HI_ERR_CA_INVALID_PARA;
    }

    return ret;
}

HI_S32 DRV_ADVCA_GetVendorId(HI_U32 *pu32VendorId)
{
    HI_S32 ret = HI_SUCCESS;

    if (HI_NULL == pu32VendorId)
    {
        HI_ERR_CA("Invalid Parameters.\n");
        return HI_ERR_CA_INVALID_PARA;
    }
    
    ret = DRV_CA_OTP_V200_GetVendorId((CA_OTP_VENDOR_TYPE_E *)pu32VendorId);

    return ret;
}

/*****************************************************************************
 Prototype    :
 Description  : CA Module interface for Irdeto high level code authentication 
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/
HI_S32 DRV_ADVCA_DecryptIrdetoHCA(HI_U32 AddrID)
{
    HI_S32 ret = HI_SUCCESS;
 
    ret = HAL_ADVCA_V300_CryptHCA(AddrID);
  
    return ret;
}


/*****************************************************************************
 Prototype    :
 Description  : CA Module interface for Other Modules
 Input        : None
 Output       : None
 Return Value : None
*****************************************************************************/
HI_S32 DRV_ADVCA_Callback(HI_U32 u32Cmd, HI_VOID * pArgs)
{
    switch(u32Cmd)
    {
        default:
        {
            HI_ERR_CA("CA  Not Support\n");
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

HI_S32  DRV_ADVCA_ModeInit_0(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;

    if( NULL == g_pOTPExportFunctionList)
    {
        ret = HI_DRV_MODULE_GetFunction(HI_ID_OTP, (HI_VOID**)&g_pOTPExportFunctionList);
        if( NULL == g_pOTPExportFunctionList)
        {
            HI_FATAL_CA("Get otp functions failed(ignored)!\n");
        }
    }	

    ret = HI_DRV_MODULE_Register(HI_ID_CA, HI_MOD_CA, (HI_VOID*)&s_CaExportFunctionList);
    if (HI_SUCCESS != ret)
    {
        HI_FATAL_CA("HI_DRV_MODULE_Register failed\n");
        return ret;
    }

    ret = DRV_ADVCA_V300_ModeInit_0();

    return ret;
}

HI_VOID DRV_ADVCA_ModeExit_0(HI_VOID)
{
  
    DRV_ADVCA_V300_ModeExit_0();

    HI_DRV_MODULE_UnRegister(HI_ID_CA);

    HI_INFO_CA("Release g_pOTPExportFunctionList!\n");
    if( NULL != g_pOTPExportFunctionList)
    {
        g_pOTPExportFunctionList = NULL;
    }
	
    return;
}

EXPORT_SYMBOL(HI_DRV_ADVCA_Crypto);
EXPORT_SYMBOL(HI_DRV_ADVCA_Resume);
EXPORT_SYMBOL(HI_DRV_ADVCA_Suspend);

