/******************************************************************************

  Copyright (C), 2014-2024, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : drv_hdmi_debug.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/06/24
  Description   :
  History       :
  Date          : 2015/06/24
  Author        : t00273561
  Modification  :
*******************************************************************************/

#include "drv_hdmi_debug.h"
#include "drv_hdmi_common.h"
#include "drv_hdmi_intf.h"
#include "drv_hdmi_event.h"
#include "drv_hdmi_edid.h"
#include "drv_hdmi_common.h"
#include "hdmi_hal.h"
#include "hdmi_platform.h"
#include "hdmi_osal.h"



#define HDMI_DBG_PRINT              HI_DRV_PROC_EchoHelper  
#define HDMI_DBG_ERR                HDMI_DBG_PRINT("[HDMI DBG ERROR]: %s [%d],",__func__,__LINE__);HDMI_DBG_PRINT

#define DEBUG_MAX_CMD_CHAR_NUM      2048
#define DEBUG_MAX_ARGV_NUM          10

#define DEBUG_MAX_DELAY_MS          100000

#define DEBUG_CHAR_SPACE            0x20
#define DEBUG_CHAR_TAB              0x09
#define DEBUG_CHAR_END              0x0a

#define DEBUG_MAX_INFOFRAME_SIZE    31
#define DEBUG_MAX_HDCPKEY_SIZE      2048
#define DEBUG_MAX_EDIDBUF_SIZE      2048
#define DEBUG_DUMP_DEFAULT_LEN      16

#define STR_SUCC                    "success"
#define STR_FAIL                    "fail"

#ifndef TOLOWER
#define TOLOWER(x)                  ((x) | 0x20) 
#endif


#define DEBUG_NULL_CHK(p) do{\
	if(HI_NULL == p)\
	{HDMI_WARN("%s null pointer!\n",#p);\
	HDMI_DBG_ERR("%s null pointer!\n",#p);\
	return HI_FAILURE;}\
}while(0)

#define DEBUG_FAILURE_RET_CHK(s32Ret) do{\
	if(HI_SUCCESS != s32Ret)\
	{HDMI_WARN("return failure!\n");\
	HDMI_DBG_ERR("return failure!\n");\
	return HI_FAILURE;}\
}while(0)



typedef struct file *PFILE;

typedef enum
{
    HDMI_DEBUG_BASE_OSD = 8 ,
    HDMI_DEBUG_BASE_DEC = 10,
    HDMI_DEBUG_BASE_HEX = 16,
    
}HDMI_DEBUG_BASE_E;


typedef struct{

    HDMI_DEVICE_ID_E    enHdmiId;
    HI_CHAR             *pcArgv[DEBUG_MAX_ARGV_NUM];
    HI_U32              u32Argc;
    HI_U32              u32RemainLen;

}HDMI_DEBUG_CMD_ARG_S;

typedef HI_S32 (*PFN_CMD_FUNC)(HDMI_DEBUG_CMD_ARG_S *pstCmdArg);

typedef struct{
    HI_CHAR         *pcName;
    HI_CHAR         *pcShortName;
    PFN_CMD_FUNC    pfnCmdFunc;
    HI_CHAR         *pCommentHelp;
    
}HDMI_DEBUG_CMD_INFO_S;

typedef struct {
    HI_BOOL bDataValid;
    HI_S32  s32Len;
    HI_U8   au8Data[HDMI_EDID_TOTAL_SIZE];
}HDMI_DEBUG_EDID_S;

static HDMI_DEBUG_EDID_S s_stDebugEdid = {0};

const static char *s_aEventStrTable[] = {
    "HPD",                      //HDMI_EVENT_HOTPLUG = 0x10,      
    "HOTUNPLUG",                //HDMI_EVENT_HOTUNPLUG,            
    "EDID_FAIL",                //HDMI_EVENT_EDID_FAIL,            
    "HDCP_FAIL",                //HDMI_EVENT_HDCP_FAIL,            
    "HDCP_SUCCESS",             //HDMI_EVENT_HDCP_SUCCESS,       
    "RSEN_CONNECT",             //HDMI_EVENT_RSEN_CONNECT,        
    "RSEN_DISCONNECT",          //HDMI_EVENT_RSEN_DISCONNECT,      
    "HDCP_USERSETTING",         //HDMI_EVENT_HDCP_USERSETTING,    
    "SCRAMBLE_FAIL",            //HDMI_EVENT_SCRAMBLE_FAIL,            
    "SCRAMBLE_SUCCESS",         //HDMI_EVENT_SCRAMBLE_SUCCESS,     
                                //HDMI_EVENT_BUTT
};

typedef struct
{
    HI_U32          u32DdcRegCfg;
    HI_U32          u32ApproximateValue;
    HI_CHAR         *pstReadValue;
}HDMI_DDC_FREQ_S;

HDMI_DDC_FREQ_S s_astDdcFreq[] = {
    
    {0x45,   20,  " 19.89"},    
    {0x29,   30,  " 29.97"}, 
    {0x21,   40,  " 40.06"}, 
    {0x1a,   50,  " 49.90"}, 
    {0x15,   60,  " 60.53"}, 
    {0x12,   70,  " 69.42"}, 
    {0x0f,   80,  " 81.38"}, 
    {0x0d,   90,  " 91.87"}, 
    {0x0c,   100, " 98.11"}, 
    {0x0a,   110, "113.78"}, 
    {0x07,   150, "149.85"}, 
    {0x04,   220, "219.43"}

};

static struct file *Debugfopen(const char *filename, int flags, int mode)
{
        struct file *filp = filp_open(filename, flags, mode);
        return (IS_ERR(filp)) ? NULL : filp;
}

static void Debugfclose(struct file *filp)
{
        if (filp)
            filp_close(filp, NULL);
}

static int Debugfread(char *buf, unsigned int len, struct file *filp)
{
        int readlen;
        mm_segment_t oldfs;

        if (filp == NULL)
                return -ENOENT;
        if (filp->f_op->read == NULL)
                return -ENOSYS;
        if (((filp->f_flags & O_ACCMODE) & (O_RDONLY | O_RDWR)) == 0)
                return -EACCES;
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        readlen = filp->f_op->read(filp, buf, len, &filp->f_pos);
        set_fs(oldfs);

        return readlen;
}

static int Debugfwrite(char *buf, int len, struct file *filp)
{
        int writelen;
        mm_segment_t oldfs;

        if (filp == NULL)
                return -ENOENT;
        if (filp->f_op->write == NULL)
                return -ENOSYS;
        if (((filp->f_flags & O_ACCMODE) & (O_WRONLY | O_RDWR)) == 0)
                return -EACCES;
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        writelen = filp->f_op->write(filp, buf, len, &filp->f_pos);
        set_fs(oldfs);

        return writelen;
}

static HI_S32 DebugSnprintf(HI_CHAR *pszStr, HI_SIZE_T ulLen, const HI_CHAR *pszFormat, ...)
{
    HI_S32 s32Len;
    va_list stArgs = {0};

    va_start(stArgs, pszFormat);
    s32Len = vsnprintf(pszStr, ulLen, pszFormat, stArgs);
    va_end(stArgs);

    return s32Len;
}

static HI_BOOL IS_HEX_SPACE(HI_CHAR a)
{
	return ( a== DEBUG_CHAR_SPACE || a==DEBUG_CHAR_TAB || a==',' || a==';' || a=='\0' || a=='\n' || a=='\r' )  ?  HI_TRUE : HI_FALSE;
}

static HI_BOOL IS_HEX_CHAR(HI_CHAR a)		
{
	return  ( (a>='0' && a<='9') || (a>='a' && a<='f') || (a>='A' && a<='F') )  ?  HI_TRUE : HI_FALSE;
}

static HI_CHAR HEX_VALUE_GET(HI_CHAR a)
{
	if ((a>='0' && a<='9'))
	{
		return a - '0';
	}
	else if (a>='a' && a<='f')
	{
		return a - 'a' + 0xa;
	}
	else if(a>='A' && a<='F')
	{
		return a - 'A' + 0xa;
	}
	else 
	{
		return 0xff;
	}
}

static HI_BOOL IS_HEX_START(HI_CHAR *p,HI_CHAR **pcEnd)
{
	if (*p=='0' && (p[1]=='x' || p[1]=='X') )
	{
		if (pcEnd)
		{
			*pcEnd = p+2;
		}
			
		return HI_TRUE;
	}
	else if ( IS_HEX_CHAR(p[0])  
			&&( IS_HEX_SPACE(p[1])|| (IS_HEX_CHAR(p[1]) && IS_HEX_SPACE(p[2]))) )
	{
		if (pcEnd)
		{
			*pcEnd = p;
		}
		return HI_TRUE;
	}
	else
	{
		if (pcEnd)
		{
			*pcEnd = p;
		}
		return HI_FALSE;
	}
}


static HI_S32 DebugStr2Hex(HI_CHAR *pcDest,HI_S32 s32Dlen,HI_CHAR *pcSrc,HI_U32 u32Slen)
{
	HI_U8 u8Result = 0;
	HI_CHAR *p = pcSrc;
	HI_S32  s32OldLen = s32Dlen;

	DEBUG_NULL_CHK(pcSrc);
	DEBUG_NULL_CHK(pcDest);

	while(p < (pcSrc+u32Slen) && s32Dlen)
	{
		if (IS_HEX_START(p,&p))
		{
			if(IS_HEX_CHAR(p[0]))
			{
				u8Result = HEX_VALUE_GET(p[0]);
				p++;
				if (IS_HEX_CHAR(p[0]))
				{
					u8Result <<= 4;
					u8Result |= HEX_VALUE_GET(p[0]);
					p++;
				}
				
				if (pcDest)
				{
					*pcDest++ = u8Result;
					s32Dlen--;
				}
				else
				{
					return HI_FAILURE;
				}
			}
			else
			{
				return HI_FAILURE;
			}
		}
		else
		{
			p++;
		}
		
	}

	return s32OldLen - s32Dlen;
}


static HDMI_DEBUG_BASE_E DebugBaseGet(const HI_CHAR *pcStr)  
{  
    if (pcStr[0] == '0') 
    {  
        if (TOLOWER(pcStr[1]) == 'x' )  
        {
            return HDMI_DEBUG_BASE_HEX;  
        }
        else
        {
            return HDMI_DEBUG_BASE_OSD;  
        }
    }
    else 
    {  
        return HDMI_DEBUG_BASE_DEC;  
    }  
}  
  
static HI_U32 DebugStrToU32(const HI_CHAR *pcStr, HI_CHAR **ppcEnd, HDMI_DEBUG_BASE_E enBase)  
{  
    HI_U32 u32Result = 0;  
  
    if (!enBase)  
    {
        enBase = DebugBaseGet(pcStr);  
    }
    
    if (enBase == HDMI_DEBUG_BASE_HEX && pcStr[0] == '0' && TOLOWER(pcStr[1]) == 'x')  
    {
        pcStr += 2;  
    }
    
    while (*pcStr) 
    {  
        HI_U32 u32Value;  
  
        u32Value = ((*pcStr >= '0') && (*pcStr <= '9')) ? *pcStr - '0' : TOLOWER(*pcStr) - 'a' + 10;  
        if (u32Value >= enBase)
        {
            break;  
        }
        u32Result = u32Result * enBase + u32Value;  
        pcStr++;  
    }  
  
    if (ppcEnd)  
    {
        *ppcEnd = (HI_CHAR *)pcStr;  
    }
    
    return u32Result;  
}  

/************************ HDMI DEBUG CMD HANDLE ********************************************/

static HI_S32 DebugPrintHelp(HDMI_DEBUG_CMD_ARG_S *pstCmdArg);

static HI_S32 DebugThread(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32          s32Ret = HI_FAILURE;
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL)
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       s32Ret = DRV_HDMI_ThreadStateSet(pstHdmiDev,HDMI_THREAD_STATE_STOP);
       HDMI_DBG_PRINT("\n thread set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       s32Ret = DRV_HDMI_ThreadStateSet(pstHdmiDev,HDMI_THREAD_STATE_RUN);
       HDMI_DBG_PRINT("\n thread set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo thread argv1 > /proc/msp/hdmi0\n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--thread off.\n"
                   "           1\t--thread on.\n");
    HDMI_DBG_PRINT("[example]: echo thread 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugAuthenticate(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;

    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       pstHdmiDev->stAttr.stAppAttr.bAuthMode = false;
       HDMI_DBG_PRINT("\n aut disable %s! \n\n", STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       pstHdmiDev->stAttr.stAppAttr.bAuthMode = true;
       HDMI_DBG_PRINT("\n aut enable %s! \n\n", STR_SUCC );
    }
    else
    {
        HDMI_DBG_PRINT("\n aut status %d! \n\n", pstHdmiDev->stAttr.stAppAttr.bAuthMode);
    }

    return HI_SUCCESS;

usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo aut argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--aut off.\n"
                   "           1\t--aut on.\n"
                   "           2\t--aut status get.\n");
    HDMI_DBG_PRINT("[example]: echo aut 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
    
}

static HI_S32 DebugEmi(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HI_S32              s32Ret = 0;
    HDMI_ATTR_S         stAttr; 
    HDMI_EMI_CONFIG_S   stEmiConfig;
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev, &stAttr);
    if (s32Ret!=HI_SUCCESS)
    {
        HDMI_DBG_ERR("get attr error!\n");
        return HI_FAILURE;
    }
    memset(&stEmiConfig, 0, sizeof(HDMI_EMI_CONFIG_S));
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        DRV_HDMI_Stop(pstHdmiDev);
        pstHdmiDev->bEmiEnable = HI_FALSE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT(">>>>> disable emi.\n");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        DRV_HDMI_Stop(pstHdmiDev);
        pstHdmiDev->bEmiEnable = HI_TRUE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT(">>>>> enable emi.\n");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stEmiConfig.bDebugEnable = HI_TRUE; 
        pstHdmiDev->pstHdmiHal->HAL_HDMI_EmiSet(pstHdmiDev->pstHdmiHal, &stEmiConfig);

        HDMI_DBG_PRINT(">>>>> emi debug enable.\n");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        stEmiConfig.bDebugEnable = HI_FALSE; 
        pstHdmiDev->pstHdmiHal->HAL_HDMI_EmiSet(pstHdmiDev->pstHdmiHal, &stEmiConfig);

        HDMI_DBG_PRINT(">>>>> emi debug disable.\n");
    }

    return HI_SUCCESS;

usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo emi argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--emi off.\n"
                   "           1\t--emi on.\n"
                   "           2\t--emi debug enable, it will not config emi regs.\n"
                   "           3\t--emi debug disable.\n");
    HDMI_DBG_PRINT("[example]: echo emi 1 > /proc/msp/hdmi0 \n");
    return HI_FAILURE;
}


static HI_S32 DebugCmd(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;

    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1], "0"))
    {
        DRV_HDMI_Stop(pstHdmiDev);
        HDMI_DBG_PRINT(">>>>> hdmi stop.\n");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1], "1"))
    {
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT(">>>>> hdmi start.\n");
    }
    else 
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    return HI_SUCCESS;

usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo cmd argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--hdmi stop.\n"
                   "           1\t--hdmi start.\n");
    HDMI_DBG_PRINT("[example]: echo cmd 1 > /proc/msp/hdmi0 \n");
    return HI_FAILURE;
}


static HI_S32 DebugOe(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    HDMI_BLACK_FRAME_INFO_S stBlackInfo;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    stBlackInfo.inBitDepth = pstHdmiDev->stAttr.stVOAttr.enInBitDepth;
    stBlackInfo.inColorSpace = pstHdmiDev->stAttr.stVOAttr.enInColorSpace;
    stBlackInfo.inQuantization = (stBlackInfo.inColorSpace == HDMI_COLORSPACE_RGB) ? \
    pstHdmiDev->stAttr.stVOAttr.enRGBQuantization : pstHdmiDev->stAttr.stVOAttr.enYCCQuantization;

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       DRV_HDMI_AudioOutputEnableSet(pstHdmiDev, HI_FALSE);
       stBlackInfo.bBlacEnable = HI_TRUE;
       pstHdmiDev->pstHdmiHal->HAL_HDMI_BlackDataSet(pstHdmiDev->pstHdmiHal, &stBlackInfo); 
       DRV_HDMI_AVMuteSet(pstHdmiDev, HI_TRUE);
	   msleep(200);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_FALSE);
       HDMI_DBG_PRINT("\n oe set OFF %s! \n\n", STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       DRV_HDMI_AVMuteSet(pstHdmiDev, HI_FALSE);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_TRUE);
       stBlackInfo.bBlacEnable = HI_FALSE;
       pstHdmiDev->pstHdmiHal->HAL_HDMI_BlackDataSet(pstHdmiDev->pstHdmiHal, &stBlackInfo);
       DRV_HDMI_AudioOutputEnableSet(pstHdmiDev, HI_TRUE);
       HDMI_DBG_PRINT("\n oe set ON %s! \n\n", STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_FALSE);
       HDMI_DBG_PRINT("\n oe set OFF %s! \n\n", STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"4"))
    {
        pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_TRUE);
        HDMI_DBG_PRINT("\n oe set ON %s! \n\n", STR_SUCC );
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo oe argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--oe off.\n"
                   "           1\t--oe on.\n"
                   "           2\t--oe off whith no black frame.\n"
                   "           3\t--oe on whith no black frame.\n");
    HDMI_DBG_PRINT("[example]: echo oe 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugEvent(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_EVENT_E            enEvent = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	HI_CHAR                 *pcStr = pstCmdArg->pcArgv[1];
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    enEvent = (HDMI_EVENT_E)DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    
    if ( (enEvent >= HDMI_EVENT_HOTPLUG) 
        && (enEvent < HDMI_EVENT_BUTT) )
    {
        s32Ret = DRV_HDMI_EventPoolWrite(&pstHdmiDev->stEventInfo,enEvent);
        if ((enEvent-HDMI_EVENT_HOTPLUG) < HDMI_ARRAY_SIZE(s_aEventStrTable))
        {
            HDMI_DBG_PRINT("hardware event %s simulate %s.\n",
                        s_aEventStrTable[enEvent-HDMI_EVENT_HOTPLUG],
                        s32Ret!=HI_SUCCESS ? STR_FAIL: STR_SUCC);
        }
    }
    else
    {
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo event argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: event type(DECMICAL) as follow\n");
    for (enEvent = HDMI_EVENT_HOTPLUG; 
        enEvent < HDMI_EVENT_BUTT && ((enEvent-HDMI_EVENT_HOTPLUG) < HDMI_ARRAY_SIZE(s_aEventStrTable));
        enEvent++)
    {
        HDMI_DBG_PRINT("%-10s %d\t-- (0x%02x)%s\n","",enEvent,enEvent,
                        s_aEventStrTable[enEvent-HDMI_EVENT_HOTPLUG]);
    }
    HDMI_DBG_PRINT("[example]: echo event 16 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugEdid(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HI_S32              i = 0,s32Len = 0;
    PFILE               pFile = HI_NULL;
    HI_U8               *pu8Buf = HI_NULL;
    HDMI_DEBUG_EDID_S   *pstDbgEdid = &s_stDebugEdid;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 3)
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    DEBUG_NULL_CHK(pstCmdArg->pcArgv[2]);
    DEBUG_NULL_CHK(pstCmdArg->pcArgv[1]);

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"s")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        memset(pstDbgEdid,0,sizeof(HDMI_DEBUG_EDID_S));  
        s32Ret = DRV_HDMI_EdidUpdate(&pstHdmiDev->stEdidInfo,HDMI_EDID_UPDATE_SINK);
        if (s32Ret != HI_SUCCESS)
        {
            HDMI_DBG_ERR("update edid from sink fail!Make sure cable connect!\n");
        }
#if 0
        else
        {
            DRV_HDMI_EventPoolWrite(&pstHdmiDev->stEventInfo,HDMI_EVENT_HOTUNPLUG);
            msleep(100);
            DRV_HDMI_EventPoolWrite(&pstHdmiDev->stEventInfo,HDMI_EVENT_HOTPLUG);
        }
#endif
    } 
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"l")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        HDMI_DBG_ERR("pstDbgEdid->s32Len=%d !\n",pstDbgEdid->s32Len);
        s32Ret = DebugStr2Hex(pstDbgEdid->au8Data + pstDbgEdid->s32Len,
                                HDMI_EDID_TOTAL_SIZE-pstDbgEdid->s32Len,
                                pstCmdArg->pcArgv[2],
                                pstCmdArg->u32RemainLen);
        if (s32Ret < 0)
        {
            HDMI_DBG_ERR("error hex raw data format!\n");
            goto usage_error;
        }
        else
        {
            pstDbgEdid->bDataValid = HI_FALSE;
            pstDbgEdid->s32Len     += s32Ret;
            HDMI_DBG_ERR("pstDbgEdid->s32Len=%d !\n",pstDbgEdid->s32Len);
        }
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"e")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        HDMI_DBG_ERR("pstDbgEdid->s32Len=%d !\n",pstDbgEdid->s32Len);
        s32Ret = DebugStr2Hex(pstDbgEdid->au8Data + pstDbgEdid->s32Len,
                                HDMI_EDID_TOTAL_SIZE-pstDbgEdid->s32Len,
                                pstCmdArg->pcArgv[2],
                                pstCmdArg->u32RemainLen);
        if (s32Ret < 0)
        {
            HDMI_DBG_ERR("error hex raw data format!\n");
            goto usage_error;
        }
        else
        {
            pstDbgEdid->bDataValid = HI_TRUE;
            pstDbgEdid->s32Len     += s32Ret;
            s32Ret = DRV_HDMI_EdidUpdate(&pstHdmiDev->stEdidInfo,HDMI_EDID_UPDATE_DEBUG);
        }
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"f")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        DEBUG_NULL_CHK(pstCmdArg->pcArgv[2]);
        pFile = Debugfopen(pstCmdArg->pcArgv[2],O_RDWR ,0);
        if (pFile == HI_NULL)
        {
            HDMI_DBG_ERR("open file %s fail!\n",pstCmdArg->pcArgv[2]);
        }

        pu8Buf = (HI_U8 *)HDMI_KMALLOC(DEBUG_MAX_EDIDBUF_SIZE);
        if (pu8Buf)
        {
            memset(pstDbgEdid,0,sizeof(HDMI_DEBUG_EDID_S));

            s32Len = Debugfread(pu8Buf,DEBUG_MAX_EDIDBUF_SIZE,pFile);
            if (s32Len <= 0)
            {
                HDMI_DBG_PRINT("read file size = %d!\n",s32Len);
            }
            else
            {
                s32Len = DebugStr2Hex(pstDbgEdid->au8Data,HDMI_EDID_TOTAL_SIZE,pu8Buf,(HI_U32)s32Len);
                if (s32Len < 0)
                {
                    HDMI_DBG_ERR("string convert to hex error!\n");        
                }
                else
                {
                    pstDbgEdid->bDataValid = HI_TRUE;
                    pstDbgEdid->s32Len     += s32Len;
                    s32Ret = DRV_HDMI_EdidUpdate(&pstHdmiDev->stEdidInfo,HDMI_EDID_UPDATE_DEBUG);
                }
            }
            HDMI_KFREE(pu8Buf);
        }
        else
        {
            HDMI_DBG_ERR("kmalloc error!\n");
        }
        Debugfclose(pFile);

        if (s32Len < 0)
        {
            HDMI_DBG_ERR("error hex raw data format!\n");
            goto usage_error;
        }
       
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    HDMI_DBG_PRINT(" edid load %s! \n",s32Ret<0 ? STR_FAIL : STR_SUCC);
    HDMI_DBG_PRINT("[Edid Data]:");
    pu8Buf= (HI_U8 *)HDMI_KMALLOC(HDMI_EDID_TOTAL_SIZE);
    if (pu8Buf)
    {
        s32Ret = DRV_HDMI_EdidRawGet(&pstHdmiDev->stEdidInfo,pu8Buf,HDMI_EDID_TOTAL_SIZE);
        for (i=0;i<s32Ret;i++)
        {
            if (!(i%16)){HDMI_DBG_PRINT("\n");}
            HDMI_DBG_PRINT(" %02x",pu8Buf[i]);
        }
        
        HDMI_DBG_PRINT("\n\n");
        HDMI_KFREE(pu8Buf);
    }
    else
    {
        HDMI_DBG_ERR("kzmalloc fail!\n");
    }
        
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo edid argv1 argv2 > /proc/msp/hdmi0\n");
    HDMI_DBG_PRINT("[argv1  ]: edid operation mode as below\n");
    HDMI_DBG_PRINT("%10s 0(s)\t--finish using debug edid and read from sink.\n","");
    HDMI_DBG_PRINT("%10s 1(l)\t--read debug edid from argv2(a hex-string-line).\n","");
    HDMI_DBG_PRINT("%10s 2(el)\t--read debug edid from argv2(a hex-string-line),then finish reading.\n","");
    HDMI_DBG_PRINT("%10s 3(f)\t--read debug edid from argv2(a hex-string-file-path).\n","");
    HDMI_DBG_PRINT("[argv2  ]: hex-data string/filepath.You have to use at least 1 charater of 'SPACE|TAB|,|0x' between 2 hex-bytes.\n");
    HDMI_DBG_PRINT("[example]: echo edid s > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[example]: echo edid l 0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[example]: echo edid f /mnt/edid_skyworth_300m.txt > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugInfoframe(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
	HI_U8               au8IfBuf[DEBUG_MAX_INFOFRAME_SIZE+1];
    HI_U32              i = 0,u32Sum = 0;
    HI_U8               u8ChkSum = 0;
    HDMI_INFOFRAME_ID_E enInfoFrameId;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 3)
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    DEBUG_NULL_CHK(pstCmdArg->pcArgv[2]);
    DEBUG_NULL_CHK(pstCmdArg->pcArgv[1]);
    memset(au8IfBuf,0,sizeof(au8IfBuf));

    s32Ret = DebugStr2Hex(au8IfBuf,DEBUG_MAX_INFOFRAME_SIZE,
                pstCmdArg->pcArgv[2],pstCmdArg->u32RemainLen);
    if (s32Ret < 0)
    {
        HDMI_DBG_ERR("error hex raw data format!\n");
        goto usage_error;
    }
    
    for (i=0,u32Sum=0;i <= DEBUG_MAX_INFOFRAME_SIZE; i++)
    {
        if(i != 3)
        {
            u32Sum += au8IfBuf[i];
        }
    }
//    u8ChkSum = (HI_U8)(0x100 - u32Sum);
    u8ChkSum = (HI_U8)((0x100 - (u32Sum % 0x100)) % 0x100);
    if (au8IfBuf[3] != u8ChkSum)
    {
        HDMI_DBG_PRINT("error checksum 0x%02x -> 0x%02x\n",au8IfBuf[3],u8ChkSum);
        au8IfBuf[3] = u8ChkSum;
    }
      
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"avi")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"v"))
    {
        enInfoFrameId = HDMI_INFOFRAME_TYPE_AVI;
        HDMI_DBG_PRINT(" avi");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"audio")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"a"))
    {
        enInfoFrameId = HDMI_INFOFRAME_TYPE_AUDIO;
        HDMI_DBG_PRINT(" audio");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"vs")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"s"))
    {
        enInfoFrameId = HDMI_INFOFRAME_TYPE_VENDOR;
        HDMI_DBG_PRINT(" vs");
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"4")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"gdb")
        || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"g"))
    {
        enInfoFrameId = HDMI_INFOFRAME_TYPE_GBD;
        HDMI_DBG_PRINT(" gdb");
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    pstHdmiDev->pstHdmiHal->HAL_HDMI_InfoframeEnableSet(pstHdmiDev->pstHdmiHal, enInfoFrameId, HI_FALSE);
    pstHdmiDev->pstHdmiHal->HAL_HDMI_InfoframeSet(pstHdmiDev->pstHdmiHal, enInfoFrameId, au8IfBuf);
    pstHdmiDev->pstHdmiHal->HAL_HDMI_InfoframeEnableSet(pstHdmiDev->pstHdmiHal, enInfoFrameId, HI_TRUE);

    HDMI_DBG_PRINT(" infoframe send %s! \n",STR_SUCC);
    HDMI_DBG_PRINT("[InfoFrame Data]:");
    for (i=0;i<sizeof(au8IfBuf);i++)
    {
        HDMI_DBG_PRINT(" %02x",au8IfBuf[i]);
    }
    HDMI_DBG_PRINT("\n\n");
        
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo infoframe argv1 argv2 > /proc/msp/hdmi0\n");
    HDMI_DBG_PRINT("[argv1  ]: infoframe type list as below\n");
    HDMI_DBG_PRINT("%10s 0(v/avi)\t\t--avi .\n","");
    HDMI_DBG_PRINT("%10s 1(a/audio)\t\t--audio .\n","");
    HDMI_DBG_PRINT("%10s 2(s/vs)\t\t--vendor specif .\n","");
    HDMI_DBG_PRINT("%10s 3(g/gdb)\t\t--gdb packet.\n","");
    HDMI_DBG_PRINT("[argv2  ]: infoframe hex-byte string.You have to use at least 1 charater of 'SPACE|TAB|,|0x' between 2 hex-bytes.\n");
    HDMI_DBG_PRINT("[example]: echo infoframe 0 82 02 0d 78 50 a8 00 13 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[example]: echo infoframe avi 0x82,0x2,0xd,0x78,0x50,0xa8,0x00,0x13 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugAvmute(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       DRV_HDMI_AVMuteSet(pstHdmiDev, HI_FALSE);
       HDMI_DBG_PRINT("\n avmute set OFF %s! \n\n", STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       DRV_HDMI_AVMuteSet(pstHdmiDev, HI_TRUE);
       HDMI_DBG_PRINT("\n avmute set ON %s! \n\n", STR_SUCC );
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo avmute argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--avmute off.\n"
                   "           1\t--avmute on.\n");
    HDMI_DBG_PRINT("[example]: echo avmute 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugAmute(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    
    if(IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        s32Ret = DRV_HDMI_AudioOutputEnableSet(pstHdmiDev,HI_FALSE);
        HDMI_DBG_PRINT("\n audio mute set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else if(IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        s32Ret = DRV_HDMI_AudioOutputEnableSet(pstHdmiDev,HI_TRUE);
        HDMI_DBG_PRINT("\n audio mute set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo amute argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--OFF.\n"
                   "           1\t--ON.\n");
    HDMI_DBG_PRINT("[example]: echo amute 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugVmute(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    
    if(IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        s32Ret = DRV_HDMI_VideoOutputEnableSet(pstHdmiDev,HI_TRUE);
        HDMI_DBG_PRINT("\n Video mute set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else if(IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        s32Ret = DRV_HDMI_VideoOutputEnableSet(pstHdmiDev,HI_FALSE);
        HDMI_DBG_PRINT("\n Video mute set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo vmute argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--OFF.\n"
                   "           1\t--ON.\n");
    HDMI_DBG_PRINT("[example]: echo vmute 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugDelay(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HI_CHAR             *pcStr = pstCmdArg->pcArgv[1];
    HI_U32              u32DelayTime = 0;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1] == HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    
    u32DelayTime = DebugStrToU32(pcStr, &pcStr, HDMI_DEBUG_BASE_DEC);
    if(DEBUG_MAX_DELAY_MS < u32DelayTime)
    {
        HDMI_DBG_ERR("input para error.\n");
        goto usage_error;
    }
    
    HDMI_DBG_PRINT("\n HDMI debug context delay change old(%d)->new(%d). \n\n", pstHdmiDev->stDebugContext.u32StopDelay, u32DelayTime);
    pstHdmiDev->stDebugContext.u32StopDelay = u32DelayTime;

    return HI_SUCCESS;
    
usage_error:
        HDMI_DBG_PRINT("[Usage  ]: echo debugdelay argv1 > /proc/msp/hdmi0 \n");
        HDMI_DBG_PRINT("[argv1  ]: a delay time value in range [0 , %u] ms \n", DEBUG_MAX_DELAY_MS);
        HDMI_DBG_PRINT("[example]: echo debugdelay 100 > /proc/msp/hdmi0 \n");
       
    return HI_FAILURE;
}

static HI_S32 DebugMutedelay(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HI_U32                  u32OldDelay = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	HDMI_DELAY_S            stDelay;
	HI_CHAR                 *pcStr = pstCmdArg->pcArgv[1];
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_DelayGet(pstHdmiDev,&stDelay);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldDelay = stDelay.u32MuteDelay;
    
    stDelay.u32MuteDelay = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    if (stDelay.u32MuteDelay > DEBUG_MAX_DELAY_MS)
    {
        HDMI_DBG_ERR("error delay time!\n");
        goto usage_error;
    }
    
    s32Ret = DRV_HDMI_DelaySet(pstHdmiDev,&stDelay);
    HDMI_DBG_PRINT("\n avmute delay set %u -> %u ms %s! \n\n",
                    u32OldDelay,
                    stDelay.u32MuteDelay,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo mutedelay argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: a delay time value in range [0 , %u] ms \n",DEBUG_MAX_DELAY_MS);
    HDMI_DBG_PRINT("[example]: echo mutedelay 100 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugFmtdelay(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HI_U32                  u32OldDelay = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	HDMI_DELAY_S            stDelay;
	HI_CHAR                 *pcStr = pstCmdArg->pcArgv[1];
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_DelayGet(pstHdmiDev,&stDelay);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldDelay = stDelay.u32FmtDelay;
    
    stDelay.u32FmtDelay = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    if (stDelay.u32FmtDelay > DEBUG_MAX_DELAY_MS)
    {
        HDMI_DBG_ERR("error delay time!\n");
        goto usage_error;
    }
    
    s32Ret = DRV_HDMI_DelaySet(pstHdmiDev,&stDelay);
    HDMI_DBG_PRINT("\n format delay set %u -> %u ms %s! \n\n",
                    u32OldDelay,
                    stDelay.u32FmtDelay,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo fmtdelay argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: a delay time value in range [0 , %u] ms \n",DEBUG_MAX_DELAY_MS);
    HDMI_DBG_PRINT("[example]: echo fmtdelay 100 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugFmtForce(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HI_BOOL                 bOldMode = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	HDMI_DELAY_S            stDelay;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_DelayGet(pstHdmiDev,&stDelay);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    bOldMode = stDelay.bForceFmtDelay;
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       stDelay.bForceFmtDelay = HI_FALSE;  
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       stDelay.bForceFmtDelay = HI_TRUE; 
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    s32Ret = DRV_HDMI_DelaySet(pstHdmiDev,&stDelay);
    HDMI_DBG_PRINT("\n Format change set %s->%s mode %s! \n\n",
                    bOldMode ? "USER" : "KERNEL_DEFAULT",
                    stDelay.bForceFmtDelay ? "USER" : "KERNEL_DEFAULT",
                    s32Ret!=HI_SUCCESS ? STR_FAIL: STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo fmtforce argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--format change mode in kernel default mode.\n"
                   "           1\t--format change mode in user mode.\n");
    HDMI_DBG_PRINT("[example]: echo fmtforce 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugMuteforce(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HI_BOOL                 bOldMode = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	HDMI_DELAY_S            stDelay;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_DelayGet(pstHdmiDev,&stDelay);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    bOldMode = stDelay.bForceMuteDelay;
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       stDelay.bForceMuteDelay = HI_FALSE;  
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       stDelay.bForceMuteDelay = HI_TRUE; 
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    s32Ret = DRV_HDMI_DelaySet(pstHdmiDev,&stDelay);
    HDMI_DBG_PRINT("\n Format change set %s->%s mode %s! \n\n",
                    bOldMode ? "USER" : "KERNEL_DEFAULT",
                    stDelay.bForceMuteDelay ? "USER" : "KERNEL_DEFAULT",
                    s32Ret!=HI_SUCCESS ? STR_FAIL: STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo muteforce argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--avmute change mode in kernel default mode.\n"
                   "           1\t--avmute change mode in user mode.\n");
    HDMI_DBG_PRINT("[example]: echo muteforce 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugCbar(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    HI_BOOL         bEnable = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       bEnable = HI_FALSE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_COLOR_BAR,&bEnable);
       HDMI_DBG_PRINT("\n ColorBar is OFF %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       bEnable = HI_TRUE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_COLOR_BAR,&bEnable);
       HDMI_DBG_PRINT("\n ColorBar is ON %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo cbar argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--colorbar off.\n"
                   "           1\t--colorbar on.\n");
    HDMI_DBG_PRINT("[example]: echo cbar 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugSwrst(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 1  )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_SW_RESET,HI_NULL);
    HDMI_DBG_PRINT("\n software reset set %s! \n\n",STR_SUCC);

    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo swrst > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv   ]: none\n");
    HDMI_DBG_PRINT("[example]: echo swrst > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugScdc(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HDMI_SCDC_STATUS_S  stScdcStatus;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       stScdcStatus.bSourceScrambleOn = HI_FALSE;
       stScdcStatus.bSinkScrambleOn = HI_FALSE;
       stScdcStatus.u8TmdsBitClkRatio = 10;
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_FALSE);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_ScdcStatusSet(pstHdmiDev->pstHdmiHal, &stScdcStatus);
       msleep(90); 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_TRUE);
       memset(&stScdcStatus,0,sizeof(stScdcStatus));
       
       msleep(3000);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_ScdcStatusGet(pstHdmiDev->pstHdmiHal, &stScdcStatus);
       HDMI_DBG_PRINT("\n scdc set OFF %s! \n\n", 
                    (stScdcStatus.bSourceScrambleOn == HI_FALSE) &&
                    (stScdcStatus.bSinkScrambleOn == HI_FALSE) &&
                    (stScdcStatus.u8TmdsBitClkRatio == 10) ? STR_SUCC : STR_FAIL);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       stScdcStatus.bSourceScrambleOn = HI_TRUE;
       stScdcStatus.bSinkScrambleOn = HI_TRUE;
       stScdcStatus.u8TmdsBitClkRatio = 40;
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_FALSE);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_ScdcStatusSet(pstHdmiDev->pstHdmiHal, &stScdcStatus);
       msleep(90);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_TRUE);
       
       memset(&stScdcStatus,0,sizeof(stScdcStatus)); 
       msleep(3000);
       pstHdmiDev->pstHdmiHal->HAL_HDMI_ScdcStatusGet(pstHdmiDev->pstHdmiHal, &stScdcStatus);

       HDMI_DBG_PRINT("\n scdc set ON %s! \n\n", 
                    (stScdcStatus.bSourceScrambleOn == HI_TRUE) &&
                    (stScdcStatus.bSinkScrambleOn == HI_TRUE) &&
                    (stScdcStatus.u8TmdsBitClkRatio == 40) ? STR_SUCC : STR_FAIL);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))    
    {
        pstHdmiDev->pstHdmiHal->HAL_HDMI_ScdcStatusGet(pstHdmiDev->pstHdmiHal, &stScdcStatus);
        HDMI_DBG_PRINT("\n scdc get bSourceScrambleOn[%d] \n"
                       "            bSinkScrambleOn[%d] \n"
                       "          u8TmdsBitClkRatio[%d] \n\n", 
                                    stScdcStatus.bSourceScrambleOn,
                                    stScdcStatus.bSinkScrambleOn,
                                    stScdcStatus.u8TmdsBitClkRatio);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo scdc argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--scdc off.\n"
                   "           1\t--scdc on.\n"
                   "           2\t--scdc get.\n");
    HDMI_DBG_PRINT("[example]: echo scdc 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugHdmimode(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        pstHdmiDev->pstHdmiHal->HAL_HDMI_TmdsModeSet(pstHdmiDev->pstHdmiHal,HDMI_TMDS_MODE_DVI);
        HDMI_DBG_PRINT("\n hdmi tmds mode set DVI %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        pstHdmiDev->pstHdmiHal->HAL_HDMI_TmdsModeSet(pstHdmiDev->pstHdmiHal,HDMI_TMDS_MODE_HDMI_1_4);
        HDMI_DBG_PRINT("\n hdmi tmds mode set HDMI1.4 %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        pstHdmiDev->pstHdmiHal->HAL_HDMI_TmdsModeSet(pstHdmiDev->pstHdmiHal,HDMI_TMDS_MODE_HDMI_2_0);
        HDMI_DBG_PRINT("\n hdmi tmds mode set HDMI2.0 %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo hdmimode argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--DVI.\n"
                   "           1\t--HDMI1.4.\n"
                   "           2\t--HDMI2.0.\n");
    HDMI_DBG_PRINT("[example]: echo hdmimode 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugRgb2yuv(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    HI_BOOL                 bEnable = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    HDMI_DBG_PRINT(" in-colorspace is %d(0-RGB;2-YUV444) !\n",pstHdmiDev->stAttr.stVOAttr.enInColorSpace);

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        bEnable = HI_FALSE;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_RGB2YUV,&bEnable);
        HDMI_DBG_PRINT("\n rgb2yuv set OFF %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        bEnable = HI_TRUE;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_RGB2YUV,&bEnable);       
        HDMI_DBG_PRINT("\n rgb2yuv set ON %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo rgb2yuv argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--OFF.\n"
                   "           1\t--ON.\n");
    HDMI_DBG_PRINT("[example]: echo rgb2yuv 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugYuv2rgb(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    HI_BOOL                 bEnable = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    HDMI_DBG_PRINT("in-colorspace is %d(0-RGB;2-YUV444) !\n",pstHdmiDev->stAttr.stVOAttr.enInColorSpace);

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        bEnable = HI_FALSE;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_YUV2RGB,&bEnable);
        HDMI_DBG_PRINT("\n Yuv2rgb set OFF %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        bEnable = HI_TRUE;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_YUV2RGB,&bEnable);       
        HDMI_DBG_PRINT("\n Yuv2rgb set ON %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo yuv2rgb argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--OFF.\n"
                   "           1\t--ON.\n");
    HDMI_DBG_PRINT("[example]: echo yuv2rgb 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugOutclrspace(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;   
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HDMI_ATTR_S         stAttr;    
    HI_U32              u32OldMode = 0;

    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAppAttr.enOutColorSpace;
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stAppAttr.enOutColorSpace = HDMI_COLORSPACE_RGB;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stAppAttr.enOutColorSpace = HDMI_COLORSPACE_YCbCr422;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stAttr.stAppAttr.enOutColorSpace = HDMI_COLORSPACE_YCbCr444;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        stAttr.stAppAttr.enOutColorSpace = HDMI_COLORSPACE_YCbCr420;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n output colorspace mode %d->%d %s! \n\n",
                    u32OldMode,
                    stAttr.stAppAttr.enOutColorSpace,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo outclrspace argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--RGB.\n"
                   "           1\t--YUV422.\n"
                   "           2\t--YUV444.\n"
                   "           3\t--YUV420.\n");
    HDMI_DBG_PRINT("[example]: echo outclrspace 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugInclrspace(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;   
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HDMI_ATTR_S         stAttr;    
    HI_U32              u32OldMode = 0;

    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stVOAttr.enInColorSpace;
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stVOAttr.enInColorSpace = HDMI_COLORSPACE_RGB;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stVOAttr.enInColorSpace = HDMI_COLORSPACE_YCbCr422;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stAttr.stVOAttr.enInColorSpace = HDMI_COLORSPACE_YCbCr444;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        stAttr.stVOAttr.enInColorSpace = HDMI_COLORSPACE_YCbCr420;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n input colorspace mode %d->%d %s! \n\n",
                    u32OldMode,
                    stAttr.stVOAttr.enInColorSpace,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo inclrspace argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--RGB.\n"
                   "           1\t--YUV422.\n"
                   "           2\t--YUV444.\n"
                   "           3\t--YUV420.\n");
    HDMI_DBG_PRINT("[example]: echo inclrspace 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugDeepclr(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;   
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HDMI_ATTR_S         stAttr;    
    HI_U32              u32OldMode = 0;

    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAppAttr.enDeepColorMode;

    if (stAttr.stAppAttr.enOutColorSpace == HDMI_COLORSPACE_YCbCr422)
    {
        HDMI_DBG_ERR("current output colorspace YUV422 not support deepcolor set!Force set\n");
        //return HI_FAILURE;
    }
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stAppAttr.enDeepColorMode = HDMI_DEEP_COLOR_24BIT;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stAppAttr.enDeepColorMode = HDMI_DEEP_COLOR_30BIT;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stAttr.stAppAttr.enDeepColorMode = HDMI_DEEP_COLOR_36BIT;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n deppcolor colorspace mode %d->%d %s! \n\n",
                    u32OldMode,
                    stAttr.stAppAttr.enDeepColorMode,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo deepclr argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--08 bit.\n"
                   "           1\t--10 bit.\n"
                   "           2\t--12 bit.\n");
    HDMI_DBG_PRINT("[example]: echo deepclr 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 Debug3dmode(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stVOAttr.enStereoMode;
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stVOAttr.enStereoMode = HDMI_3D_BUTT;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stVOAttr.enStereoMode = HDMI_3D_SIDE_BY_SIDE_FULL;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stAttr.stVOAttr.enStereoMode = HDMI_3D_TOP_AND_BOTTOM;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        stAttr.stVOAttr.enStereoMode = HDMI_3D_FRAME_PACKING;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"4"))
    {
        stAttr.stVOAttr.enStereoMode = HDMI_3D_SIDE_BY_SIDE_HALF;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n 3d mode set %d->%d  %s! \n\n",
                        u32OldMode ,
                        stAttr.stVOAttr.enStereoMode,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo 3dmode argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--2D mode(%d) mode.\n",HDMI_3D_BUTT);
    HDMI_DBG_PRINT("           1\t--SIDE_BY_SIDE_FULL(%d) mode.\n",HDMI_3D_SIDE_BY_SIDE_FULL);
    HDMI_DBG_PRINT("           2\t--TOP_AND_BOTTOM(%d) mode.\n",HDMI_3D_TOP_AND_BOTTOM);
    HDMI_DBG_PRINT("           3\t--FRAME_PACKING(%d) mode.\n",HDMI_3D_FRAME_PACKING);
    HDMI_DBG_PRINT("           4\t--SIDE_BY_SIDE_HALF(%d) mode.\n",HDMI_3D_SIDE_BY_SIDE_HALF);
    HDMI_DBG_PRINT("[example]: echo 3dmode 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugPixrepet(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;
    HI_U32                  u32TmpRepeat = 0;
    HI_CHAR                 *pcStr = HI_NULL;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stVOAttr.u32PixelRepeat;

    pcStr = pstCmdArg->pcArgv[1];
    u32TmpRepeat = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    
    if (u32TmpRepeat >=1 && u32TmpRepeat <=10 && *pcStr=='\0')
    {
        stAttr.stVOAttr.u32PixelRepeat = u32TmpRepeat;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n pixel repetition set %d->%d times %s! \n\n",
                        u32OldMode ,
                        stAttr.stVOAttr.u32PixelRepeat,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo pixrepet argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: pixel repetition times in range [1,10].\n");
    HDMI_DBG_PRINT("[example]: echo pixrepet 2 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugSoundintf (HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;

	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAOAttr.enSoundIntf;

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stAOAttr.enSoundIntf = HDMI_AUDIO_INTERFACE__I2S;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stAOAttr.enSoundIntf = HDMI_AUDIO_INTERFACE__SPDIF;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        stAttr.stAOAttr.enSoundIntf = HDMI_AUDIO_INTERFACE__HBRA;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n sound interface type set %d->%d %s! \n\n",
                        u32OldMode ,
                        stAttr.stAOAttr.enSoundIntf,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo soundintf argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--I2S.\n"
                   "           1\t--SPDIF.\n"
                   "           2\t--HBRA.\n");
    HDMI_DBG_PRINT("[example]: echo soundintf 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugSamplefs (HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;
    HI_U32                  u32TmpSample = 0;
    HI_CHAR                 *pcStr = HI_NULL;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
	
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAOAttr.enSampleFs;

    pcStr = pstCmdArg->pcArgv[1];
    u32TmpSample = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    if (*pcStr != '\0')
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    switch(u32TmpSample)
    {
        case 0 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_UNKNOWN;
                break;
        case 1 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_8K;
                break;
        case 2 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_11K;
                break;
        case 3 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_12K;
                break;
        case 4 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_16K;
                break;
        case 5 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_22K;
                break;
        case 6 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_24K;
                break;
        case 7 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_32K;
                break;
        case 8 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_44K;
                break;
        case 9 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_48K;
                break;
        case 10 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_88K;
                break;
        case 11 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_96K;
                break;
        case 12 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_176K;
                break;
        case 13 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_192K;
                break;
        case 14 :
                stAttr.stAOAttr.enSampleFs = HDMI_SAMPLE_RATE_768K;
                break;
        default:
            HDMI_DBG_ERR("error argvs!\n");
            goto usage_error;
            break;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n audio sampling frequency  set %d->%d HZ %s! \n\n",
                        u32OldMode ,
                        stAttr.stAOAttr.enSampleFs,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo samplefs argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: audio sampling frequency list as below (HZ)\n");
    HDMI_DBG_PRINT("           0\t--%u.\n",HDMI_SAMPLE_RATE_UNKNOWN);
    HDMI_DBG_PRINT("           1\t--%u.\n",HDMI_SAMPLE_RATE_8K);
    HDMI_DBG_PRINT("           2\t--%u.\n",HDMI_SAMPLE_RATE_11K);
    HDMI_DBG_PRINT("           3\t--%u.\n",HDMI_SAMPLE_RATE_12K);
    HDMI_DBG_PRINT("           4\t--%u.\n",HDMI_SAMPLE_RATE_16K);
    HDMI_DBG_PRINT("           5\t--%u.\n",HDMI_SAMPLE_RATE_22K);
    HDMI_DBG_PRINT("           6\t--%u.\n",HDMI_SAMPLE_RATE_24K);
    HDMI_DBG_PRINT("           7\t--%u.\n",HDMI_SAMPLE_RATE_32K);
    HDMI_DBG_PRINT("           8\t--%u.\n",HDMI_SAMPLE_RATE_44K);
    HDMI_DBG_PRINT("           9\t--%u.\n",HDMI_SAMPLE_RATE_48K);
    HDMI_DBG_PRINT("           10\t--%u.\n",HDMI_SAMPLE_RATE_88K);
    HDMI_DBG_PRINT("           11\t--%u.\n",HDMI_SAMPLE_RATE_96K);
    HDMI_DBG_PRINT("           12\t--%u.\n",HDMI_SAMPLE_RATE_176K);
    HDMI_DBG_PRINT("           13\t--%u.\n",HDMI_SAMPLE_RATE_192K);
    HDMI_DBG_PRINT("           14\t--%u.\n",HDMI_SAMPLE_RATE_768K);
    HDMI_DBG_PRINT("[example]: echo samplefs 1 > /proc/msp/hdmi0 \n");
    return HI_FAILURE;
}


static HI_S32 DebugChannel(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;

	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAOAttr.enChanels;

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        stAttr.stAOAttr.enChanels = HDMI_AUDIO_FORMAT_2CH;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        stAttr.stAOAttr.enChanels = HDMI_AUDIO_FORMAT_8CH;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n audio channel set %d->%d %s! \n\n",
                        u32OldMode ,
                        stAttr.stAOAttr.enChanels,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo channel argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--2 channels.\n"
                   "           1\t--8 channels (multi-channel).\n");
    HDMI_DBG_PRINT("[example]: echo channel 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugSampledepth(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
    HDMI_ATTR_S             stAttr;
    HI_U32                  u32OldMode = 0;
    HI_U32                  u32TmpDepth = 0;
    HI_CHAR                 *pcStr = HI_NULL;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev,&stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    u32OldMode = (HI_U32)stAttr.stAOAttr.enSampleDepth;

    pcStr = pstCmdArg->pcArgv[1];
    u32TmpDepth = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
    if (*pcStr != '\0')
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    switch(u32TmpDepth)
    {
        case 0 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_UNKNOWN;
                break;
        case 1 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_8;
                break;
        case 2 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_16;
                break;
        case 3 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_18;
                break;
        case 4 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_20;
                break;
        case 5 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_24;
                break;
        case 6 :
                stAttr.stAOAttr.enSampleDepth = HDMI_AUDIO_BIT_DEPTH_32;
                break;
        default:
                HDMI_DBG_ERR("error argvs!\n");
                goto usage_error;
                //break;
    }

    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev,&stAttr);
    HDMI_DBG_PRINT("\n audio sampling depth set %d->%d %s! \n\n",
                        u32OldMode ,
                        stAttr.stAOAttr.enSampleDepth,
                        s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo sampledepth argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: audio sampling depth list as below (BIT)\n");
    HDMI_DBG_PRINT("           0\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_UNKNOWN);
    HDMI_DBG_PRINT("           1\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_8);
    HDMI_DBG_PRINT("           2\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_16);
    HDMI_DBG_PRINT("           3\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_18);
    HDMI_DBG_PRINT("           4\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_20);
    HDMI_DBG_PRINT("           5\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_24);
    HDMI_DBG_PRINT("           6\t--%u.\n",HDMI_AUDIO_BIT_DEPTH_32);
    HDMI_DBG_PRINT("[example]: echo sampledepth 1 > /proc/msp/hdmi0 \n");
    return HI_FAILURE;
}


static HI_S32 DebugDither(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    HI_BOOL         bEnable = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       bEnable = HI_FALSE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DITHER,&bEnable);
       HDMI_DBG_PRINT("\n dither is OFF %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       bEnable = HI_TRUE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DITHER,&bEnable);
       HDMI_DBG_PRINT("\n dither is ON %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: dither argv1 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--dither off.\n"
                   "           1\t--dither on.\n");
    HDMI_DBG_PRINT("[example]: dither 1 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugDdc(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    
    HI_U32          u32DdcFreq      = 0;
    HI_U32          i               = 0;
	HDMI_DEVICE_S   *pstHdmiDev     = HI_NULL;
    HI_CHAR         *pcStr          = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    pcStr = pstCmdArg->pcArgv[1];

    
    u32DdcFreq = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);

    if (u32DdcFreq >= HDMI_ARRAY_SIZE(s_astDdcFreq))
    {
        goto usage_error;
    }
    
    pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DDC_FREQ,&s_astDdcFreq[u32DdcFreq].u32DdcRegCfg);
    HDMI_DBG_PRINT("\n ddc frequency set %s KHz %s! \n\n",s_astDdcFreq[u32DdcFreq].pstReadValue,STR_SUCC);
    
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: ddc argv1 \n");
    HDMI_DBG_PRINT("[argv1  ]: ddc frequency [0,%u]\n",HDMI_ARRAY_SIZE(s_astDdcFreq)-1);
    for (i = 0;i < HDMI_ARRAY_SIZE(s_astDdcFreq);i++)
    {
        HDMI_DBG_PRINT("           %2u -- %3u KHz (real %s KHz,Reg=0x%02x)\n",
            i,
            s_astDdcFreq[i].u32ApproximateValue,
            s_astDdcFreq[i].pstReadValue,
            s_astDdcFreq[i].u32DdcRegCfg);        
    }
    HDMI_DBG_PRINT("[example]: ddc 1 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugBypass(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    HI_BOOL         bEnable = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       bEnable = HI_FALSE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_BYPASS,&bEnable);
       HDMI_DBG_PRINT("\n bypass is OFF %s! \n\n",STR_SUCC);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       bEnable = HI_TRUE; 
       pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_BYPASS,&bEnable);
       HDMI_DBG_PRINT("\n bypass is ON %s! \n\n",STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: bypass argv1 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--bypass off.\n"
                   "           1\t--bypass on.\n");
    HDMI_DBG_PRINT("[example]: bypass 1 \n");
        
    return HI_FAILURE;
}


static HI_S32 DebugPhyParaPrint(HDMI_HAL_PHY_PARA_S *pstPhyCfg)
{
    DEBUG_NULL_CHK(pstPhyCfg);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "DataSwing",          pstPhyCfg->u8DataSwing);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "ClkSwing",           pstPhyCfg->u8ClkSwing);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "SrcTermination",     pstPhyCfg->u8SrcTermination);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "DrvVnbValue",        pstPhyCfg->u8DrvVnbValue);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "ClkSrcFineAdjust",   pstPhyCfg->u8ClkSrcFineAdjust);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "TxRegZone",          pstPhyCfg->u8TxRegZone);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "RiseTime",           pstPhyCfg->u8RiseTime);
    HDMI_DBG_PRINT("\t  %-20s= 0x%02x\n", "FallTime",           pstPhyCfg->u8FallTime);
    return HI_SUCCESS;
}

static HI_S32 DebugPhypara(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              i = 0;
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HI_CHAR             *pcStr = HI_NULL;
    HI_CHAR             *pu8Phy = HI_NULL;
    HDMI_HAL_PHY_PARA_S stPhyCfg;
    HDMI_HAL_PHY_PARA_S stPhyDefault;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_PHY_DEFAULT_GET,&stPhyDefault);
    
    if (pstCmdArg->u32Argc != 9)
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
        
    }
    for (i=0,pcStr = pstCmdArg->pcArgv[1],pu8Phy = (HI_U8 *)&stPhyCfg;
          i < sizeof(stPhyCfg) && pcStr;
          ++i,pcStr=pstCmdArg->pcArgv[1+i] )
    {
        pu8Phy[i] = (HI_U8)DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_HEX);
    }
    pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_FALSE);
    pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_PHY_PARA_SET,&stPhyCfg);
    msleep(90);
    pstHdmiDev->pstHdmiHal->HAL_HDMI_PhyOutputEnableSet(pstHdmiDev->pstHdmiHal, HI_TRUE);
    HDMI_DBG_PRINT("\n config phy parameter manually %s! \n\n",STR_SUCC);
    DebugPhyParaPrint(&stPhyCfg); 
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo phypara argv1 argv2 argv3 argv4 argv5 argv6 argv7 argv8 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1~8]: phypara parameter & default value list below\n");
    DebugPhyParaPrint(&stPhyDefault);
    HDMI_DBG_PRINT("[example]: echo phypara a > /proc/msp/hdmi0\n");
    HDMI_DBG_PRINT("[example]: echo phypara 0x16 0x15 0x0 0x2 0x1 0x2 0x01 0x01 > /proc/msp/hdmi0\n");
        
    return HI_FAILURE;
}

static HI_S32 DebugDump(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HI_CHAR             *pcStr = HI_NULL;
    HI_S32              s32Ret = 0;
    HI_U32              i = 0,u32DumpLen = DEBUG_DUMP_DEFAULT_LEN;
    HI_S32              j = 0;
    HDMI_HAL_REG_S      stReg = {0};
    HI_U32              u32RegAddr = 0;
    PFILE               pFile = HI_NULL;
    HI_U8               au8Data[50];
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 2)
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (pstCmdArg->u32Argc == 4 && pstCmdArg->pcArgv[3])
    {
        pFile = Debugfopen(pstCmdArg->pcArgv[3], O_RDWR | O_CREAT| O_TRUNC, 0777);
        if (pFile == HI_NULL)
        {
            HDMI_DBG_ERR("errro open file %s\n",pstCmdArg->pcArgv[3]);
            return HI_FAILURE;
        }
    }

    pcStr = pstCmdArg->pcArgv[2];
    if (pcStr)
    {
       u32DumpLen = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_HEX);
    }

    DEBUG_NULL_CHK(pstCmdArg->pcArgv[1]);    
    pcStr = pstCmdArg->pcArgv[1];
    s32Ret = strlen(pcStr);
    
    if (s32Ret == 4 || s32Ret == 6)
    {
        stReg.u32RegAddr = DebugStrToU32(pcStr, &pcStr, HDMI_DEBUG_BASE_HEX);
        if (stReg.u32RegAddr < 0x3000 || stReg.u32RegAddr > 0x3fff)
        {
            HDMI_DBG_ERR("error addr 0x%x !\n",stReg.u32RegAddr);
            goto usage_error;
        }
        
        stReg.u32RegAddr = ((stReg.u32RegAddr-0x3000) * 4) | HDMI_CTRL_BASE_ADDR;
        stReg.u32RegVaule = 0;
        u32RegAddr = stReg.u32RegAddr;
        
        if (pFile==HI_NULL)
        {
            HDMI_DBG_PRINT("\n     %-10s %-10s \n","ADDR","VALUE");
        }
               
#if defined(CONFIG_HDMI_STB_SDK)
        stReg.u32RegAddr = 0xf8a22278;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a2210c;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a22110;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a2217c;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n\n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        for (j=7; (j>=0) && (j<=7) ;j--)
        {
            stReg.u32RegAddr = 0xf8ccc000 | (j*4);
            pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
            HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
            {
                memset(au8Data,'\0',sizeof(au8Data));
                DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
            if(j == 0)
            {
                DebugSnprintf(au8Data,sizeof(au8Data),"------ HDMI register ------\n");
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
        }
#endif        

        stReg.u32RegAddr = u32RegAddr;
        stReg.u32RegVaule = 0;

        for (i = 0; i < u32DumpLen && stReg.u32RegAddr <= (HDMI_CTRL_BASE_ADDR | 0x3ffc); i++,stReg.u32RegAddr+=4)
        {
            stReg.u32RegAddr |= HDMI_CTRL_BASE_ADDR;
            pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
            
            if (pstCmdArg->u32Argc == 3 || pstCmdArg->u32Argc == 2)
            {
                if (!(i%16)){HDMI_DBG_PRINT("\n");}
                HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            }
            else if (pFile)
            {
                memset(au8Data,'\0',sizeof(au8Data));
                DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n%s",stReg.u32RegAddr,stReg.u32RegVaule,(i%16)? "" :"\n");
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
            else
            {
                HDMI_DBG_ERR("error argvs!\n");
                goto usage_error;
            }
        }
    }
    else if (s32Ret == 8 || 10)
    {
        stReg.u32RegAddr = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_HEX);
        stReg.u32RegVaule = 0;

        if (stReg.u32RegAddr < HDMI_CTRL_BASE_ADDR || stReg.u32RegAddr > (HDMI_CTRL_BASE_ADDR|0xffff))
        {
            HDMI_DBG_ERR("error addr 0x%x !\n",stReg.u32RegAddr);
            goto usage_error;
        }
        u32RegAddr = stReg.u32RegAddr;
        
        if (pFile==HI_NULL)
        {
            HDMI_DBG_PRINT("\n     %-10s %-10s \n","ADDR","VALUE");
        }
        
#if defined(CONFIG_HDMI_STB_SDK)
        stReg.u32RegAddr = 0xf8a22278;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a2210c;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a22110;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        stReg.u32RegAddr = 0xf8a2217c;
        pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
        HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n\n",stReg.u32RegAddr,stReg.u32RegVaule);
        if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
        {
            memset(au8Data,'\0',sizeof(au8Data));
            DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            Debugfwrite(au8Data,strlen(au8Data),pFile);
        }

        for (j=7; (j>=0) && (j<=7) ;j--)
        {
            stReg.u32RegAddr = 0xf8ccc000 | (j*4);
            pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);
            HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            if(pstCmdArg->u32Argc != 3 && pstCmdArg->u32Argc != 2 && pFile)
            {
                memset(au8Data,'\0',sizeof(au8Data));
                DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
            if(j == 0)
            {
                DebugSnprintf(au8Data,sizeof(au8Data),"------ HDMI register ------\n");
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
        }
#endif        

        stReg.u32RegAddr = u32RegAddr;
        stReg.u32RegVaule = 0;

        for (i = 0; i < u32DumpLen && stReg.u32RegAddr <= (HDMI_CTRL_BASE_ADDR | 0x3ffc); i++,stReg.u32RegAddr+=4)
        {
            pstHdmiDev->pstHdmiHal->HAL_HDMI_Debug(pstHdmiDev->pstHdmiHal,HDMI_DEBUG_CMD_DUMP,&stReg);  
            if (pstCmdArg->u32Argc == 3 || pstCmdArg->u32Argc == 2)
            {
                if (!(i%16)){HDMI_DBG_PRINT("\n");}
                HDMI_DBG_PRINT("himm 0x%08x 0x%08x \n",stReg.u32RegAddr,stReg.u32RegVaule);
            }
            else if (pFile)
            {
                memset(au8Data,'\0',sizeof(au8Data));
                DebugSnprintf(au8Data,sizeof(au8Data),"himm 0x%08x 0x%08x \n%s",stReg.u32RegAddr,stReg.u32RegVaule,(i%16)? "" :"\n");
                Debugfwrite(au8Data,strlen(au8Data),pFile);
            }
            else
            {
                HDMI_DBG_ERR("error argvs!\n");
                goto usage_error;
            }
        }
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    HDMI_DBG_PRINT("\n dump register u32DumpLen=%u(0x%x) %s! \n\n",u32DumpLen,u32DumpLen,STR_SUCC);

    if (pFile)
    {
        Debugfclose(pFile);
    }
    
    return HI_SUCCESS;
    
usage_error:
    if (pFile)
    {
        Debugfclose(pFile);
    }
    HDMI_DBG_PRINT("[Usage  ]: echo dump argv1 argv2 [argv3] > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: dump address in hex.range in [0x3000,0x3fff] or [0x%x,0x%x].\n", HDMI_CTRL_BASE_ADDR,HDMI_CTRL_BASE_ADDR|0x3ffc);
    HDMI_DBG_PRINT("[argv2  ]: dump length in hex.range in [0x1,0x1000] \n");
    HDMI_DBG_PRINT("[argv3  ]: dump file path,optional.when there is argv3 then dump to argv3,or dump to terminal.\n");
    HDMI_DBG_PRINT("[example]: echo dump 0x3710 0x1000 > /proc/msp/hdmi0\n");
    HDMI_DBG_PRINT("[example]: echo dump 0x%x 0x1000 > /proc/msp/hdmi0\n",HDMI_CTRL_BASE_ADDR);
    HDMI_DBG_PRINT("[example]: echo dump 0x%x 0x1000 /mnt/stream/20150708.txt > /proc/msp/hdmi0\n",HDMI_CTRL_BASE_ADDR);
        
    return HI_FAILURE;
}

static HI_S32 DebugAvpath(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    
    HI_U8 *pu8VirtualAddr = HI_NULL;
    
    HDMI_DEVICE_S *pstHdmiDev = HI_NULL;
    DEBUG_NULL_CHK(pstCmdArg);

    if( pstCmdArg->u32Argc > 2 )
    {
        goto usage_error;
    }
    
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId); 
    if(pstHdmiDev == HI_NULL)
    {
        HDMI_ERR("pstHdmiDev is null! \n");
        goto usage_error;
    }
    if(pstHdmiDev->pu32VirtualBaseAddr == HI_NULL)
    {
        HDMI_ERR("pstHdmiDev->pu32VirtualBaseAddr is null! \n");
        return HI_FAILURE;
    }
    
#if defined(CONFIG_HDMI_STB_SDK)
    /* HDMI hw reset */
    pu8VirtualAddr = (HI_U8 *)HDMI_IO_MAP(HDMI_CRG_BASE_ADDR + 0x210c, 4);

    *(volatile HI_U32 *)( pu8VirtualAddr )= 0x433f;
    msleep(10);
    *(volatile HI_U32 *)( pu8VirtualAddr )= 0x403f;

    HDMI_IO_UNMAP(pu8VirtualAddr);
#else
    HI_U32 u32RegValue = 0;
    pu8VirtualAddr = (HI_U8 *)HDMI_IO_MAP(HDMI_CRG_BASE_ADDR, 4);
    u32RegValue = *(volatile HI_U32 *)( pu8VirtualAddr );
    
    *(volatile HI_U32 *)( pu8VirtualAddr ) = u32RegValue | HDMITX_CTRL_SRST_REQ | HDMITX_CTRL_BUS_SRST_REQ;
    msleep(10);
    *(volatile HI_U32 *)( pu8VirtualAddr ) = u32RegValue &(~( HDMITX_CTRL_SRST_REQ | HDMITX_CTRL_BUS_SRST_REQ));
    HDMI_IO_UNMAP(pu8VirtualAddr);
#endif
    msleep(10);
    
    pu8VirtualAddr = (HI_U8 *)pstHdmiDev->pu32VirtualBaseAddr;
    
    
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC8 ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1fc0 ) = 0x0;
    msleep(10);
    

    /* VO 1080p60 timming ,cbar enable */
    pu8VirtualAddr = (HI_U8 *)HDMI_IO_MAP(HDMI_COLOR_BAR_BASE, 0x30);

    *(volatile HI_U32 *)( pu8VirtualAddr + 0x004 ) = 0x00c28437;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x008 ) = 0x00bf077f;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x00c ) = 0x00000057;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x010 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x014 ) = 0x0004002b;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x018 ) = 0x01c282a3;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x01c ) = 0x83dd8232;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x000 ) = 0xe0000011;


    HDMI_IO_UNMAP(pu8VirtualAddr);

    /* HDMI ctrl */
	pu8VirtualAddr = (HI_U8 *)pstHdmiDev->pu32VirtualBaseAddr;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x0034 ) = 0x00000006;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x002C ) = 0x00000001;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x004C ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x01E4 ) = 0x6;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x0194 ) = 0x00000085;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x03DC ) = 0x2;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x0890 ) = 0x4;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2C70 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x3480 ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x3484 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x3680 ) = 0x00000040;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x3684 ) = 0x00000003;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC0 ) = 0x00000090;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC4 ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC8 ) = 0xa7;

    *(volatile HI_U32 *)( pu8VirtualAddr + 0x01D4 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x288C ) = 0x00000010;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2870 ) = 0xe4;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2808 ) = 0x00000001;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x28BC ) = 0x2;

    *(volatile HI_U32 *)( pu8VirtualAddr + 0x280C ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2810 ) = 0x00000018;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2814 ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2818 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x281C ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2820 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2874 ) = 0x60;

    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2890 ) = 0xb;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2878 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x287C ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2880 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2884 ) = 0x00000002;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2888 ) = 0x0000000B;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2850 ) = 0x00000010;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x284C ) = 0x1;
  
    /* AVI IF*/
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1afc ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b00 ) = 0x82;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b04 ) = 0x2;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b08 ) = 0x0d;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b0c ) = 0x67;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b10 ) = 0x50;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b14 ) = 0xa8;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b18 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b1c ) = 0x10;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b20 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b24 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b28 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b2c ) = 0x00; 
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b30 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b34 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b38 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b3c ) = 0x00; 
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b40 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b44 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b48 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b4c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b50 ) = 0x00;

    /* AVI IF enable */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1afc ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b7c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b7c ) = 0xe0;


    /* AUDIO IF */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1afc ) = 0x2 ; 
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b00 ) = 0x84;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b04 ) = 0x1;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b08 ) = 0x0a;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b0c ) = 0x71;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b10 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b14 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b18 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b1c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b20 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b24 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b28 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b2c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b30 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b34 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b38 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b3c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b40 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b44 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b48 ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b4c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b50 ) = 0x00;
    
    /* AUDIO IF enable */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1afc ) = 0x02;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b7c ) = 0x00;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1b7c ) = 0xe0;

    /* AUDIO MUTE */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x28bc ) = 0x82;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x28bc ) = 0x02;

    /* AUDIO ID = I2S */
	pu8VirtualAddr = (HI_U8 *)HDMI_IO_MAP(PERI_HDMITX_CTRL_ADDR, 4);
    *(volatile HI_U32 *)( pu8VirtualAddr ) = 0x00050000; //bvt 0x12120150 / stb 0xf8a208b0
    *(volatile HI_U32 *)( pu8VirtualAddr ) = 0x00050002;
	HDMI_IO_UNMAP(pu8VirtualAddr);
	
    /* AUDIO PATH 48000HZ,2ch,16bit */
	pu8VirtualAddr = (HI_U8 *)pstHdmiDev->pu32VirtualBaseAddr;	
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2850 ) = 0x00000010;   
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2878 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x287c ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2880 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2884 ) = 0x00000002;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2888 ) = 0x0000000b;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x28a0 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x28a4 ) = 0x00000000;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x298c ) = 0x00000082;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x2984 ) = 0x00000007;

    /* AUDIO enable */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x284c ) = 0x0;
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x284c ) = 0x1;

    /* OE */
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC8 ) = 0x0;
    msleep(100);          
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1fc0 ) = 0x0;
    msleep(100);          
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1EC8 ) = 0xa7;
    msleep(100);         
    *(volatile HI_U32 *)( pu8VirtualAddr + 0x1fc0 ) = 0x10;

    HDMI_DBG_PRINT("\n video 1080p60 & cbar & audio path set %s! \n\n", STR_SUCC );

    return HI_SUCCESS;
      
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo avpath > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[example]: echo avpath > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;

}

#ifdef HDMI_CEC_SUPPORT
static HI_S32 DebugCec(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32          s32Ret = HI_FAILURE;
	HDMI_DEVICE_S   *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
       s32Ret = DRV_HDMI_CecEnableSet(pstHdmiDev,HI_FALSE);
       HDMI_DBG_PRINT("\n cec set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
       s32Ret = DRV_HDMI_CecEnableSet(pstHdmiDev,HI_TRUE);
       HDMI_DBG_PRINT("\n cec set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo cec argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 1\t--cec on.\n"
                   "           0\t--cec off.\n");
    HDMI_DBG_PRINT("[example]: echo cec 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}
#endif

#ifdef HDMI_HDR_SUPPORT
static HI_S32 DebugUserHdrMode(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;   
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
    HDMI_ATTR_S         stAttr;    
    HI_U32              u32OldMode = 0;
    HDMI_HDR_ATTR_S     *pstHdrAttr = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1] == HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    pstHdrAttr = &(pstHdmiDev->stAttr.stHdrAttr);
    
    u32OldMode = pstHdrAttr->enUserHdrMode;
        
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        pstHdrAttr->enUserHdrMode = HDMI_HDR_USERMODE_SDR;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        pstHdrAttr->enUserHdrMode = HDMI_HDR_USERMODE_HDR10;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2"))
    {
        pstHdrAttr->enUserHdrMode = HDMI_HDR_USERMODE_DOLBY;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3"))
    {
        pstHdrAttr->enUserHdrMode = HDMI_HDR_USERMODE_DOLBY;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }
    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev, &stAttr);
    DEBUG_FAILURE_RET_CHK(s32Ret);
    
	DRV_HDMI_Stop(pstHdmiDev);
    s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
	DRV_HDMI_Start(pstHdmiDev);
    HDMI_DBG_PRINT("\n user hdr mode change %d->%d %s! \n\n",
                    u32OldMode,
                    stAttr.stHdrAttr.enUserHdrMode,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);
    
    return HI_SUCCESS;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo usrhdrmode argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--SDR.\n"
                   "           1\t--HDR10.\n"
                   "           2\t--Dolby.\n"
                   "           3\t--HDR10 AUTH.\n");
    HDMI_DBG_PRINT("[example]: echo usrhdrmode 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

#endif


#ifdef HDMI_HDCP_SUPPORT
static HI_S32 DebugHdcp14(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
	HDMI_ATTR_S         stAttr;
	//static HI_BOOL      bHdcp1preBack = HI_FALSE;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    //bHdcp1preBack = pstHdmiDev->stHdcpInfo.bHdcp1Prep;
    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev, &stAttr);
    if (s32Ret!=HI_SUCCESS)
    {
        HDMI_DBG_ERR("get attr error!\n");
        return HI_FAILURE;
    }
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        DRV_HDMI_Stop(pstHdmiDev);
        stAttr.stAppAttr.bHDCPEnable = HI_FALSE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT("\n hdcp1.4 set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
        //pstHdmiDev->stHdcpInfo.bHdcp1Prep = HI_FALSE;
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {

        DRV_HDMI_Stop(pstHdmiDev);
        stAttr.stAppAttr.bHDCPEnable = HI_TRUE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT("\n hdcp1.4 set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
        //pstHdmiDev->stHdcpInfo.bHdcp1Prep = HI_TRUE;
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo hdcp14 argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 1\t--hdcp1.4 on.\n"
                   "           0\t--hdcp1.4 off.\n");
    HDMI_DBG_PRINT("[example]: echo hdcp14 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugHdcp22(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
	HDMI_ATTR_S         stAttr;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    s32Ret = DRV_HDMI_AttrGet(pstHdmiDev, &stAttr);
    if (s32Ret!=HI_SUCCESS)
    {
        HDMI_DBG_ERR("get attr error!\n");
        return HI_FAILURE;
    }
    
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0"))
    {
        
        DRV_HDMI_Stop(pstHdmiDev);
        stAttr.stAppAttr.bHDCPEnable = HI_FALSE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT("\n hdcp2.2 set OFF %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1"))
    {
        DRV_HDMI_Stop(pstHdmiDev);
        stAttr.stAppAttr.bHDCPEnable = HI_TRUE;
        s32Ret = DRV_HDMI_AttrSet(pstHdmiDev, &stAttr);
        DRV_HDMI_Start(pstHdmiDev);
        HDMI_DBG_PRINT("\n hdcp2.2 set ON %s! \n\n",s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC );
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo hdcp22 argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0\t--hdcp2.2 off.\n"
                   "           1\t--hdcp2.2 on.\n");
    HDMI_DBG_PRINT("[example]: echo hdcp22 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugLoadkey(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32              s32Ret = HI_FAILURE;   
	HDMI_DEVICE_S       *pstHdmiDev = HI_NULL;
	HI_U8               *pu8Buf = HI_NULL;
	PFILE               pFile = HI_NULL;
    HDMI_LOAD_KEY_S     stLoadKey ;
    
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc != 2 || pstCmdArg->pcArgv[1]==HI_NULL )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
    
    memset(&stLoadKey,0,sizeof(stLoadKey));
    pu8Buf = (HI_U8 *)HDMI_KMALLOC(DEBUG_MAX_HDCPKEY_SIZE);
    DEBUG_NULL_CHK(pu8Buf);
    memset(pu8Buf,0,DEBUG_MAX_HDCPKEY_SIZE);
    
    pFile = Debugfopen(pstCmdArg->pcArgv[1],O_RDWR ,0);
    if (pFile == HI_NULL)
    {
        HDMI_DBG_ERR("errro open file %s\n",pstCmdArg->pcArgv[1]);
        HDMI_KFREE(pu8Buf);
        return HI_FAILURE;
    }
    s32Ret = Debugfread((HI_CHAR *)pu8Buf,DEBUG_MAX_HDCPKEY_SIZE-1,pFile);
    Debugfclose(pFile);
    if (s32Ret > 0)
    {
        stLoadKey.u32KeyLength = s32Ret;
        stLoadKey.pu8InputEncryptedKeyAddr = pu8Buf;
    }
    else
    {
        HDMI_DBG_ERR("read file len=%d \n",s32Ret);
        HDMI_KFREE(pu8Buf);
        return HI_FAILURE;
    }
    s32Ret = DRV_HDMI_KeyLoad(pstHdmiDev,&stLoadKey);
    HDMI_DBG_PRINT("\n loadkey size %u bytes %s! \n\n",
                    stLoadKey.u32KeyLength,
                    s32Ret==HI_SUCCESS ? STR_SUCC : STR_FAIL);
                    
    HDMI_KFREE(pu8Buf);
    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo loadkey argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: a keyfile path string.\n");
    HDMI_DBG_PRINT("[example]: echo loadkey /mnt/stream/key/EncryptedKey_332bytes.bin > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}
#endif


#ifdef HDMI_DEBUG
static HI_S32 DebugLogcat(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 2 || pstCmdArg->u32Argc > 3)
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (pstCmdArg->u32Argc==2 && IS_STR_EQUAL(pstCmdArg->pcArgv[1],"s")) 
    {
        s32Ret = DRV_HDMI_LogCat(LOG_PRINT_TO_SCREEN, HI_NULL);
        HDMI_DBG_PRINT("\n logcat to screen %s! \n\n", 
                                s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else if (pstCmdArg->pcArgv[2] 
            && (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2")  || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"f")))
    {
        s32Ret = DRV_HDMI_LogCat(LOG_PRINT_TO_FILE, pstCmdArg->pcArgv[2]);
        HDMI_DBG_PRINT("\n logcat to %s %s! \n\n",
                                pstCmdArg->pcArgv[2] , 
                                s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    
    }
    else if (pstCmdArg->pcArgv[2]
             && (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"a")))
    {
        s32Ret = DRV_HDMI_LogCat(LOG_PRINT_TO_ALL, pstCmdArg->pcArgv[2]);
        HDMI_DBG_PRINT("\n logcat to screen & %s %s! \n\n",
                                pstCmdArg->pcArgv[2] , 
                                s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo logcat argv1 [argv2] > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: print destination.\n");
    HDMI_DBG_PRINT("           0(s)\t--print to screen.\n");
    HDMI_DBG_PRINT("           1(f)\t--print to file.\n");
    HDMI_DBG_PRINT("           2(a)\t--print to both screen & file.\n");
    HDMI_DBG_PRINT("[argv2  ]: log output file path.optional,argv1 is 'f|a' needed.\n");
    HDMI_DBG_PRINT("[example]: echo logcat > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[example]: echo logcat f /mnt/stream/hdmilog/log20150710.txt > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugLogmask(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 2  )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }
 
    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"n"))
    {
        s32Ret = DRV_HDMI_LogSetMask(LOG_TYPE_FUNC_ALL);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"a"))
    {
        s32Ret = DRV_HDMI_LogSetMask(LOG_TYPE_FUNC_API);

    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2")  || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"h"))
    {
        s32Ret = DRV_HDMI_LogSetMask(LOG_TYPE_FUNC_HDCP);

    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"e"))
    {
        s32Ret = DRV_HDMI_LogSetMask(LOG_TYPE_FUNC_EDID);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"4") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"c"))
    {
        s32Ret = DRV_HDMI_LogSetMask(LOG_TYPE_FUNC_CEC);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    HDMI_DBG_PRINT("\n logmask set %s %s! \n\n",pstCmdArg->pcArgv[1],s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo logmask argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0(n)\t--none.\n");
    HDMI_DBG_PRINT("           1(a)\t--api.\n");
    HDMI_DBG_PRINT("           2(h)\t--hdcp.\n");
    HDMI_DBG_PRINT("           3(e)\t--edid.\n");
    HDMI_DBG_PRINT("           4(c)\t--cec .\n");
    HDMI_DBG_PRINT("[example]: echo logmode 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

static HI_S32 DebugLoglevel(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HDMI_DEVICE_S           *pstHdmiDev = HI_NULL;
    
    DEBUG_NULL_CHK(pstCmdArg);
    pstHdmiDev = GetHdmiDevice(pstCmdArg->enHdmiId);
    DEBUG_NULL_CHK(pstHdmiDev);
    DEBUG_NULL_CHK(pstHdmiDev->pstHdmiHal);

    if (pstCmdArg->u32Argc < 2 )
    {
        HDMI_DBG_ERR("error argcs!\n");
        goto usage_error;
    }

    if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"0") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"i"))
    {
        s32Ret = DRV_HDMI_LogSetLevel(LOG_LEVEL_INFO);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"1") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"w"))
    {
        s32Ret = DRV_HDMI_LogSetLevel(LOG_LEVEL_WARN);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"2")  || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"e"))
    {
        s32Ret = DRV_HDMI_LogSetLevel(LOG_LEVEL_ERROR);
    }
    else if (IS_STR_EQUAL(pstCmdArg->pcArgv[1],"3") || IS_STR_EQUAL(pstCmdArg->pcArgv[1],"f"))
    {
        s32Ret = DRV_HDMI_LogSetLevel(LOG_LEVEL_FATAL);
    }
    else
    {
        HDMI_DBG_ERR("error argvs!\n");
        goto usage_error;
    }

    HDMI_DBG_PRINT("\n loglevel set %s %s! \n\n",pstCmdArg->pcArgv[1],s32Ret!=HI_SUCCESS ? STR_FAIL : STR_SUCC);

    return s32Ret;
    
usage_error:
    HDMI_DBG_PRINT("[Usage  ]: echo loglevel argv1 > /proc/msp/hdmi0 \n");
    HDMI_DBG_PRINT("[argv1  ]: 0(i)\t--info.\n");
    HDMI_DBG_PRINT("           1(w)\t--warn.\n");
    HDMI_DBG_PRINT("           2(e)\t--error.\n");
    HDMI_DBG_PRINT("           3(f)\t--fatal.\n");
    HDMI_DBG_PRINT("[example]: echo loglevel 1 > /proc/msp/hdmi0 \n");
        
    return HI_FAILURE;
}

#endif /* #ifdef HDMI_DEBUG */


const static HDMI_DEBUG_CMD_INFO_S s_stCmdTable[] = {
    /***name**** short name *** func ************** help info **********************/
    {"help"     ,   "h",    DebugPrintHelp      ,  "print these help info."                             },
    {"thread"   ,   "t",    DebugThread         ,  "thread status set on/off."                          },
    {"oe"       ,   "o",    DebugOe             ,  "phyoutput enable(oe) set on/off."                   },
    {"event"    ,   "e" ,   DebugEvent          ,  "hardware event [16,25] simulate."                   },
    {"infoframe",   "i",    DebugInfoframe      ,  "infoframe type set avi/audio/vs/gdb."               },
    {"edid"     ,   "ed",   DebugEdid           ,  "edid raw data load."                                },
    {"avmute"   ,   "a",    DebugAvmute         ,  "audio & video mute set on/off."                     },
    {"amute"    ,   "am",   DebugAmute          ,  "audio mute set on/off."                             },
    {"vmute"    ,   "vm",   DebugVmute          ,  "video mute set on/off."                             },
    {"mutedelay",   "md",   DebugMutedelay      ,  "avmute delay time(ms) [0,100000] set."              },
    {"fmtdelay" ,   "fd",   DebugFmtdelay       ,  "format setting delay time(ms) [0,100000] set."      },
    {"fmtforce" ,   "ff",   DebugFmtForce       ,  "format change mode set kernel/user."                },
    {"muteforce",   "mf",   DebugMuteforce      ,  "avute change mode set kernel/user."                 },
    {"cbar"     ,   "c",    DebugCbar           ,  "colorbar set on/off."                               },
    {"swrst"    ,   "s",    DebugSwrst          ,  "software reset."                                    },
    {"scdc"     ,   "sc",   DebugScdc           ,  "scdc set on/off."                                   },
    {"hdmimode" ,   "m" ,   DebugHdmimode       ,  "hdmi tmds mode set dvi/1.4/2.0."                    },
    {"rgb2yuv"  ,   "r",    DebugRgb2yuv        ,  "rgb to yuv444 set on/off."                          },
    {"yuv2rgb"  ,   "y",    DebugYuv2rgb        ,  "yuv444 to rgb set on/off."                          }, 
    {"outclrspace" ,"oc",   DebugOutclrspace    ,  "output colorspace mode set rgb/yuv422/yuv444/yuv420."},
    {"inclrspace" , "ic",   DebugInclrspace     ,  "input colorspace mode set rgb/yuv422/yuv444/yuv420."},
    {"deepclr"  ,   "d",    DebugDeepclr        ,  "output color depth  set 08/10/12bit."               },
    {"3dmode"   ,   "3d",   Debug3dmode         ,  "3d mode set fpk/sbs/tab."                           },
    {"pixrepet" ,   "p",    DebugPixrepet       ,  "pixel repetition [1,10] times set."                 },
    {"soundintf",   "si",   DebugSoundintf      ,  "audio sound interface tpye set i2s/spdif/hbra."     },
    {"samplefs" ,   "sf",   DebugSamplefs       ,  "audio sampling frequency set."                      },
    {"channel" ,    "ch",   DebugChannel        ,  "audio channels set 2/8."                            },
    {"sampledepth", "sd",   DebugSampledepth    ,  "audio sampling depth set."                          },
    {"dither"   ,   "di",   DebugDither         ,  "dither set on/off."                                 },
    {"ddc"      ,   "dd",   DebugDdc            ,  "ddc frequency set in range [0,11]."                 },
    {"bypass"   ,   "by",   DebugBypass         ,  "bypass set on/off."                                 },
    {"phypara"  ,   "ph",   DebugPhypara        ,  "phy parameter config."                              },
    {"dump"     ,   "dp",   DebugDump           ,  "dump a number of regisers."                         },
	{"aut"      ,   "au",   DebugAuthenticate   ,  "authentication mode set on/off."                    },
	{"avpath"   ,   "pt",   DebugAvpath         ,  "reset hdmi hw,audio path & video 1080p60 colorbar." },
    {"emi"      ,   "em",   DebugEmi            ,  "emi set on/off."                                    },
    {"cmd"      ,   "cmd",  DebugCmd            ,  "cmd set ."                                          },
    {"debugdelay",  "dl",   DebugDelay          ,  "debug context delay set."                           },
#ifdef HDMI_CEC_SUPPORT
    {"cec"      ,   "ce",   DebugCec            ,  "cec set on/off."                                    },
#endif
#ifdef HDMI_HDCP_SUPPORT    
    {"hdcp14"   ,   "h1",   DebugHdcp14         ,  "hdcp1.4 set on/off."                                },
    {"hdcp22"   ,   "h2",   DebugHdcp22         ,  "hdcp2.2 set on/off."                                },
    {"loadkey"  ,   "l",    DebugLoadkey        ,  "load a hdcp1.4 key file."                           },
#endif
#ifdef HDMI_DEBUG
    {"logcat"   ,   "lc",   DebugLogcat         ,  "log cat set screen/file/all."                       },
    {"logmask"  ,   "lk",   DebugLogmask        ,  "log mask set none/api/edid/hdcp/cec."               },
    {"loglevel" ,   "ll",   DebugLoglevel       ,  "log level set info/warn/error/fatal."               },
#endif 
#ifdef HDMI_HDR_SUPPORT
    {"hdrmode"  ,   "hm",   DebugUserHdrMode    ,  "user hdr mode set\n"                                },
#endif
};
static HI_S32 DebugPrintHelp(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_U32 i = 0;

    DEBUG_NULL_CHK(pstCmdArg);

    for (i=0 ; i < HDMI_ARRAY_SIZE(s_stCmdTable) ; i++)
    {
        if (s_stCmdTable[i].pCommentHelp && s_stCmdTable[i].pcName)
        {
            HDMI_DBG_PRINT("    %02d. %-20s",i,s_stCmdTable[i].pcName);
            HDMI_DBG_PRINT("--[%-2s]%s\n",s_stCmdTable[i].pcShortName,s_stCmdTable[i].pCommentHelp);
        }
        else
        {
            HDMI_DBG_ERR("s_stCmdTable[%d].pCommentHelp or pcName is null!\n");
        }
    }

    HDMI_DBG_PRINT("\n");
    HDMI_DBG_PRINT("    You can use 'echo cmd argv... > /proc/msp/hdmi0' execute a debug command.\n"
                   "    You can use 'echo cmd > /proc/msp/hdmi0' for more details about every command.\n"
                   "    Here,'cmd' refers to NO./complete-name/short-name above.\n");
    return HI_SUCCESS;
}

static HI_S32 DebugCmdHandle(HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
    HI_U32          i = 0,u32CmdNum = 0;
    HI_CHAR         *pcStr = pstCmdArg->pcArgv[0];

    DEBUG_NULL_CHK(pstCmdArg);

    if (pstCmdArg->u32Argc > 0)
    {
        u32CmdNum = DebugStrToU32(pcStr,&pcStr,HDMI_DEBUG_BASE_DEC);
        
        for (i=0 ; i < HDMI_ARRAY_SIZE(s_stCmdTable) ; i++)
        {
            if ( s_stCmdTable[i].pcName )
            {     
                if( ((u32CmdNum == i) && (*pcStr == '\0') )
                   || IS_STR_EQUAL(s_stCmdTable[i].pcName,pstCmdArg->pcArgv[0])
                   || IS_STR_EQUAL(s_stCmdTable[i].pcShortName,pstCmdArg->pcArgv[0]) )
                {
                    
                    if (s_stCmdTable[i].pfnCmdFunc)
                    {
                        if(s_stCmdTable[i].pfnCmdFunc(pstCmdArg))
                        {
                            //goto error_help;
                        }
                    }
                    else
                    {
                        HDMI_DBG_ERR("s_stCmdTable[%d].pfnCmdFunc null\n",i);
                        goto error_help;
                    }
                    break;
                }
                
            }
            else
            {
                HDMI_DBG_ERR("s_stCmdTable[%d].pcName null\n",i);
                goto error_help;
            }

        }
    }

    if ( i == HDMI_ARRAY_SIZE(s_stCmdTable) )
    {
        goto error_help;
    }
    
    return HI_SUCCESS;
    
error_help:
    DebugPrintHelp(pstCmdArg);
    
    return HI_FAILURE;
}

static HI_S32 DebugCmdLineParse(HI_CHAR *pcCmdStr,HI_U32 u32Len,
                                HDMI_DEBUG_CMD_ARG_S *pstCmdArg)
{
	HI_U32      i = 0;		
	HI_U32      u32TmpArgc = 0;
	HI_CHAR     *pTmpStr = NULL;


	if (pcCmdStr == HI_NULL 
	    || pstCmdArg == HI_NULL 
	    || u32Len == 0  
	    || u32Len >= DEBUG_MAX_CMD_CHAR_NUM)
	{
	    HDMI_DBG_ERR("invalid param!\n");
		return HI_FAILURE;
	}
	else
	{
		pTmpStr = pcCmdStr;
	}

	pstCmdArg->u32Argc = 0;
	for (i = 0; i < DEBUG_MAX_ARGV_NUM; i++)
	{
		pstCmdArg->pcArgv[i] = HI_NULL;
	}
		
	if ( (*pTmpStr != DEBUG_CHAR_SPACE) && (*pTmpStr != DEBUG_CHAR_TAB) )
	{
		pstCmdArg->pcArgv[u32TmpArgc] = pTmpStr;
		u32TmpArgc++;
		pTmpStr++; 
	}
		
	for (i=0; i < u32Len; pTmpStr++,i++)
	{
		if ( (*pTmpStr == DEBUG_CHAR_SPACE ) || (*pTmpStr == DEBUG_CHAR_TAB)  )
		{
			*pTmpStr = '\0';

			if( (*(pTmpStr+1) != DEBUG_CHAR_SPACE) && (*(pTmpStr+1) != DEBUG_CHAR_TAB))
			{
				pstCmdArg->pcArgv[u32TmpArgc] = pTmpStr + 1;
				u32TmpArgc++;

				if (u32TmpArgc == 3 )
                {   
                    pstCmdArg->u32RemainLen = u32Len - i; 
                    //break;
                }
		
				if (u32TmpArgc >= DEBUG_MAX_ARGV_NUM)
				{
				    HDMI_DBG_PRINT(" too much argcs.max argc = %d\n",DEBUG_MAX_ARGV_NUM);
                    u32TmpArgc = DEBUG_MAX_ARGV_NUM;
                    break;
				}
			}
	
		}
	}
	
	pstCmdArg->u32Argc = u32TmpArgc;

	return HI_SUCCESS;
}


/********************** HDMI DEBUG public interface ************************************/

HI_S32 DRV_HDMI_DebugEdidRawRead(HDMI_EDID_UPDATE_MODE_E enMode,
											HI_U8 *pau8Data,
											HI_U32 u32Len)
{
    HDMI_DEBUG_EDID_S *pstDbgEdid = &s_stDebugEdid;
    
	if ( (HDMI_EDID_UPDATE_DEBUG != enMode) 
	    || (HI_FALSE == pstDbgEdid->bDataValid)
		|| (HI_NULL == pau8Data)
		|| (u32Len > HDMI_EDID_TOTAL_SIZE)    )
	{
	    HDMI_ERR("Debug edid read fail,bDataValid=%d !",pstDbgEdid->bDataValid);
		return HI_FAILURE;	
	}
	
	HDMI_MEMCPY(pau8Data,pstDbgEdid->au8Data,pstDbgEdid->s32Len);
	
	return pstDbgEdid->s32Len;
}


HI_S32 DRV_HDMI_DebugSourceWrite(struct file *fp,
        									const char __user *pcUserBuf,
        									size_t ulCount, 
        									loff_t *pData)
{
    HI_S32                  s32Ret = HI_FAILURE;
	HI_U32                  u32Len = 0;
    HI_CHAR                 *pcTmpBuf = HI_NULL;
	HDMI_DEBUG_CMD_ARG_S    stCmdArg;
    struct seq_file         *p = fp->private_data;
    DRV_PROC_ITEM_S         *pProcItem = p->private;

    memset(&stCmdArg, 0 , sizeof(HDMI_DEBUG_CMD_ARG_S));

    pProcItem           = p->private;
    stCmdArg.enHdmiId   = (HDMI_DEVICE_ID_E)pProcItem->data;

    pcTmpBuf = (HI_CHAR *)HDMI_KMALLOC(DEBUG_MAX_CMD_CHAR_NUM);
    if (!pcTmpBuf)
    {
        HDMI_DBG_ERR("kmalloc fail!\n");   
        return EFAULT;
    }
    memset(pcTmpBuf, '\0' , DEBUG_MAX_CMD_CHAR_NUM);

	if(ulCount >= DEBUG_MAX_CMD_CHAR_NUM)
    {
        HDMI_DBG_ERR("\n Echo chars too long. ulCount=%d,max cnt=%d\n",ulCount,DEBUG_MAX_CMD_CHAR_NUM);
		goto error;
    }
    
    if(copy_from_user(pcTmpBuf,pcUserBuf,ulCount))
    {
        HDMI_DBG_ERR("\n copy from user failed. \n");
        goto error;
    }

    if (ulCount > 0)
    {
        /* end char '\n' set '\0' */
        pcTmpBuf[ulCount-1] = '\0';
        u32Len = ulCount - 1;
    }
    else
    {
        HDMI_DBG_ERR("ulCount=%d\n",ulCount);
        goto error;
    }
    
    s32Ret = DebugCmdLineParse(pcTmpBuf, u32Len, &stCmdArg);
    if (s32Ret != HI_SUCCESS)
    {     
        HDMI_DBG_ERR("\n Cmd Parse fail! \n");    
        goto error;
    }
#if 0
    HDMI_DBG_PRINT("\n argc=%d ",stCmdArg.u32Argc);
    for (i = 0; i < stCmdArg.u32Argc; i++)
    {
        HDMI_DBG_PRINT(" ,argv[%d]=%s ",i,stCmdArg.pcArgv[i]);
    }
    HDMI_DBG_PRINT("\n");
#endif
    s32Ret = DebugCmdHandle(&stCmdArg);
    if (s32Ret != HI_SUCCESS)
    {
        HDMI_DBG_ERR("\n Cmd handle error \n");
        goto error;
    }
    HDMI_KFREE(pcTmpBuf);
	return ulCount;
	
error:
    HDMI_KFREE(pcTmpBuf);
	return EFAULT;
	
}





