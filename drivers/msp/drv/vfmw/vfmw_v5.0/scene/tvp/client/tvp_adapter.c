/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroTK5QUHRI9UX5KeSe3MjVz/EbC4xx64tuANphe3
Vboa6fWh2Wrn9EKXu39zg7zZOknXhzgIdfEBdD1dfeog2BRat37c4D6ogCA+mwIycCqOA5nm
2dVRr6oQ12Lw8zq13tfXPdGrKTZPjrjdfrSKecANoVGIFag0ccnYrEmYCLTvZQ==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/




















/******************************************************************************

  版权所有 (C), 2001-2013, 华为技术有限公司

******************************************************************************
    文 件 名   : drv_firmware.c
    版 本 号   : 初稿
    作    者   :
    生成日期   :
    最近修改   :
    功能描述   : firmware的对外接口实现


    修改历史   :
    1.日    期 : 2013-10-22
    作    者   :
    修改内容   :

******************************************************************************/

#include "hi_osal.h"
#include "hi_drv_proc.h"
#include "hi_drv_module.h"
#include "public.h"
#include "vfmw.h"
#include "vfmw_debug.h"
#include "vfmw_osal.h"
#include "tvp_adapter.h"

#include <teek_client_api.h>

#if (1 == DEBUG_SAVE_SUPPORT)
#include "sec_mmz.h"
#endif

#define PATH_LEN              (64)
#define PROC_CMD_LEN          (16)
#define SECURE_NOTIFY_IRQ_NUM (158+32)

#define PROC_NAME             "vfmw_sec"
#define PROC_CMD_HELP         "help"
#define PROC_CMD_SUSPEND      "suspend"
#define PROC_CMD_RESUME       "resume"
#define PROC_CMD_SAVERAW      "saveraw"
#define PROC_CMD_SAVEYUV      "saveyuv"
#define PROC_CMD_START        "start"
#define PROC_CMD_STOP         "stop"
#define PROC_CMD_SETPRINT     (0x000)

#define INVALID_HANDLE        (-1)

#define PRINT_TEE_TIME        (0)

#ifndef HI_ADVCA_FUNCTION_RELEASE
#define SecPrint(type, fmt, arg...)             \
    do{                                             \
        if (PRN_ALWS == type                        \
            || 0 != (g_SecPrintEnable & (1 << type)))  \
        {                                           \
            printk(fmt, ##arg);       \
        }                                           \
    }while(0)
#else
// 高安不能检测到字符串和printk
#define SecPrint(type, fmt, arg...)             \
    do{                                             \
    }while(0)
#endif

#define  TVP_VDEC_ASSERT( cond, else_print )               \
do {                                                            \
    if( !(cond) )                                               \
    {                                                           \
        SecPrint(PRN_FATAL,"%s at L%d: %s\n", __func__, __LINE__, else_print ); \
        return VDEC_ERR;                                       \
    }                                                           \
}while(0)
typedef enum hiCHAN_STATE_E
{
    CHAN_INVALID = 0,
    CHAN_START,
    CHAN_STOP,
    CHAN_BUTT,
} CHAN_STATE_E;

typedef enum hiTHREAD_STATE_E
{
    THREAD_INVALID = 0,
    THREAD_SLEEP,
    THREAD_RUN,
    THREAD_EXIT,
    THREAD_BUTT,
} THREAD_STATE_E;

typedef enum
{
    VFMW_CMD_ID_INVALID = 0x0,
    VFMW_CMD_ID_VDEC_INIT,
    VFMW_CMD_ID_VDEC_EXIT,
    VFMW_CMD_ID_VDEC_RESUME,
    VFMW_CMD_ID_VDEC_SUSPEND,
    VFMW_CMD_ID_VDEC_CONTROL,
    VFMW_CMD_ID_VFMW_READPROC,
    VFMW_CMD_ID_VFMW_WRITEPROC,
    VFMW_CMD_ID_VFMW_THREADPROC,
    VFMW_CMD_ID_VCTRL_SETDBGOPTION,
    VFMW_CMD_ID_VFMW_GETCHANIMAGE,
    VFMW_CMD_ID_VFMW_RELEASECHANIMAGE,
    VFMW_CMD_ID_VDEC_INITWITHOPERATION,
} TEE_VFMW_CMD_ID;

typedef struct
{
    HI_BOOL            bIsSecMode;
    SINT32             OutputImgEn;
    SINT32             ChanID;
    CHAN_STATE_E       ChanState;
    OSAL_SEMA          ChanSema;

    struct file       *pRawFile;

} CHAN_CONTEXT_S;

/*指针全局变量*/
MEMORY_NEEDED_SECVFMW_S *g_pSecVfmwMem  = NULL;
CALLBACK_ARRAY_NS       *g_pCallbackBuf = NULL;
RAW_ARRAY_NS            *g_pStreamBuf   = NULL;

IMAGE_QUEUE_NS          *g_pImageQue    = NULL;
#if (1 == DEBUG_SAVE_SUPPORT)
UINT8                   *g_pSaveStreamBuf = NULL;
#endif
#ifndef  HI_ADVCA_FUNCTION_RELEASE
UINT8                   *g_pProcBuf     = NULL;
#endif

/*TEE 通信相关变量*/
static TEEC_Context      g_TeeContext;
static TEEC_Session      g_TeeSession;

/*静态全局变量*/
static MEM_DESC_S        g_SecVfmwMem;
static SINT32            g_SecPrintEnable       = 0x0;
static SINT32            g_SecureInstNum        = 0;
static SINT32            g_TrustDecoderInitCnt  = 0;
static HI_BOOL           g_bSecEnvSetUp    = HI_FALSE;
static THREAD_STATE_E    g_THREAD_STATE         = THREAD_INVALID;
static OSAL_SEMA         g_stSem_s;
static VFMW_CALLBACK_S   g_CallBack;
OSAL_EVENT               g_SecThreadEvent;
#if (1 == DEBUG_SAVE_SUPPORT)
static HI_BOOL           g_RawSaveEnable        = HI_FALSE;
static HI_CHAR           g_SavePath[PATH_LEN]   = {'/', 'm', 'n', 't', '\0'};
#endif

/*通道相关全局变量*/
static STREAM_INTF_S     g_StreamIntf[TOTAL_MAX_CHAN_NUM];
static CHAN_CONTEXT_S    g_ChanContext[TOTAL_MAX_CHAN_NUM];


#if (1 == DEBUG_SAVE_SUPPORT)
VOID TVP_VDEC_SaveStream(SINT32 ChanID, HI_BOOL bSaveEnable, UADDR PhyAddr, SINT32 Length)
{
    mm_segment_t       oldfs;
    HI_CHAR FilePath[PATH_LEN];
    struct file **ppFile = &g_ChanContext[ChanID].pRawFile;

    if (HI_TRUE == bSaveEnable && *ppFile == NULL)
    {
        snprintf(FilePath, sizeof(FilePath), "%s/trusted_vfmw_chan%d.raw", g_SavePath, ChanID);
        *ppFile = filp_open(FilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXO);

        if (IS_ERR(*ppFile))
        {
            SecPrint(PRN_ERROR, "%s open raw file failed, ret=%ld\n", __func__, PTR_ERR(*ppFile));
            *ppFile = NULL;
        }
        else
        {
            SecPrint(PRN_ALWS, "Start to save stream of inst_%d in %s\n", ChanID, FilePath);
        }
    }
    else if (HI_FALSE == bSaveEnable && *ppFile != NULL)
    {
        filp_close(*ppFile, NULL);
        *ppFile = NULL;
        SecPrint(PRN_ALWS, "Stop saving stream of inst_%d.\n", ChanID);
    }

    if (*ppFile != NULL)
    {
        HI_SEC_MMZ_TA2CA(PhyAddr, __pa(g_pSaveStreamBuf), Length);
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        (*ppFile)->f_op->write(*ppFile, g_pSaveStreamBuf, Length, &(*ppFile)->f_pos);
        set_fs(oldfs);
        SecPrint(PRN_ALWS, "Saving stream of inst_%d\n", ChanID);
    }

    return;
}
#endif


SINT32 TVP_VDEC_SetCallBack(VDEC_ADAPTER_TYPE_E eType, SINT32 (*event_report)(SINT32 ChanID, SINT32 type, VOID *p_args))
{
    TVP_VDEC_ASSERT(event_report != NULL, "event_report null!");

    switch (eType)
    {
        case ADAPTER_TYPE_VDEC:
            g_CallBack.event_report_vdec = event_report;
            break;

        case ADAPTER_TYPE_OMXVDEC:
            g_CallBack.event_report_omxvdec = event_report;
            break;

        default:
            SecPrint(PRN_FATAL, "%s Unkown Adapter Type: %d\n", __func__, eType);
            return VDEC_ERR;
    }

	return VDEC_OK;
}

SINT32 TVP_VDEC_ProcessCallBack(VOID)
{
    SINT32              ret = VDEC_OK;
    SINT32              ChanID;
    SINT32              Type;
    VDEC_ADAPTER_TYPE_E eAdapterType;
    UINT8               *pPara;

    while (g_pCallbackBuf->Tail != g_pCallbackBuf->Head)
    {
        if (g_pCallbackBuf->Message[g_pCallbackBuf->Tail].IsValid)
        {
        	ChanID = g_pCallbackBuf->Message[g_pCallbackBuf->Tail].ChanID;
			Type   = g_pCallbackBuf->Message[g_pCallbackBuf->Tail].Type;
			pPara  = (HI_U8 *)g_pCallbackBuf->Message[g_pCallbackBuf->Tail].para;
    		eAdapterType = g_pCallbackBuf->Message[g_pCallbackBuf->Tail].eAdapterType;

            switch (eAdapterType)
            {
                case ADAPTER_TYPE_VDEC:
                    if (NULL == g_CallBack.event_report_vdec)
                    {
                        SecPrint(PRN_FATAL, "s_VfmwCTrl.event_report_vdec = NULL\n");
                    }
                    else
                    {
                        ret = (g_CallBack.event_report_vdec)(ChanID, Type, pPara);
                    }
                    break;

                case ADAPTER_TYPE_OMXVDEC:
                    if (NULL == g_CallBack.event_report_omxvdec)
                    {
                        SecPrint(PRN_FATAL, "s_VfmwCTrl.event_report_omxvdec = NULL\n");
                    }
                    else
                    {
                        ret = (g_CallBack.event_report_omxvdec)(ChanID, Type, pPara);
					}
                    break;

                default:
                    SecPrint(PRN_FATAL, "%s Unkown Adapter Type: %d\n", __func__, eAdapterType);
                    break;
            }
        }

        g_pCallbackBuf->Tail = (g_pCallbackBuf->Tail + 1) % MAX_EVENT_NUM;
    }

	return ret;
}

#ifdef VFMW_VPSS_BYPASS_EN
SINT32 TVP_VDEC_FindOccupiedFrame(SINT32 ChanID, VDEC_SPECIAL_FRM_S *pSpecialFrmInfo)
{
    SINT32 i = 0, j = 0;
    SINT32 ret = VDEC_ERR;
    OUTPUT_IMAGE_RECORD_S *pOutputRec;
    
    if (NULL != g_pImageQue)
    {
        pOutputRec = g_pImageQue[ChanID].StOutputRec;
        for (i = 0; i < TRUSTED_MAX_IMAGE_NUM; i++)
        {
            if (1 == pOutputRec[i].IsUsed)
            {
                pSpecialFrmInfo->specialFrmRec[j++].PhyAddr = pOutputRec[i].TopLumaPhyAddr;

                
            }
        }
        
        pSpecialFrmInfo->specialFrameNum = j;
		ret = VDEC_OK;

    }
    
    return ret;
}
#endif
SINT32 TVP_VDEC_GetChanImage( SINT32 ChanID, IMAGE *pImage )
{
    SINT32 i;
    SINT32 ret = VDEC_ERR;
    IMAGE_LIST_S *pImgList;
    OUTPUT_IMAGE_RECORD_S *pOutputRec;

    //Down_Interruptible_with_Option(&g_stSem_s);
    if (!g_bSecEnvSetUp)
    {
        SecPrint(PRN_ERROR, "%s: TrustedDecoder not creat yet!\n", __func__);
        //Up_Interruptible_with_Option(&g_stSem_s);
        return ret;
    }
    
    if (1 == g_ChanContext[ChanID].OutputImgEn)
    {
        if ((NULL != g_pImageQue) && (ChanID >= TRUSTED_CHAN_BEGIN) && (ChanID < TOTAL_MAX_CHAN_NUM) && (NULL != pImage))
        {
            pImgList = &(g_pImageQue[ChanID].StOutputImg);
            if (pImgList->Head != pImgList->Tail)
            {
                memcpy(pImage, &(pImgList->StImage[pImgList->Head]), sizeof(IMAGE));
                pOutputRec = g_pImageQue[ChanID].StOutputRec;
                for (i=0; i<TRUSTED_MAX_IMAGE_NUM; i++)
                {
                    if (pOutputRec[i].IsUsed == 0)
                    {
                        pOutputRec[i].TopLumaPhyAddr = pImage->top_luma_phy_addr;
                        pOutputRec[i].IsUsed        = 1;
                        break;
                    }
                }
                pImgList->Head = (pImgList->Head + 1) % TRUSTED_MAX_IMAGE_NUM;
                ret = VDEC_OK;
            }
        }
    }
    
    //Up_Interruptible_with_Option(&g_stSem_s);
    return ret;
}

SINT32 TVP_VDEC_ReleaseChanImage(SINT32 ChanID, IMAGE *pImage )
{
    SINT32 i;
    SINT32 ret = VDEC_ERR;
    IMAGE_LIST_S *pImgList;
    OUTPUT_IMAGE_RECORD_S *pOutputRec;

    //Down_Interruptible_with_Option(&g_stSem_s);

    if (!g_bSecEnvSetUp)
    {
        SecPrint(PRN_ERROR, "%s: TrustedDecoder not creat yet!\n", __func__);
        //Up_Interruptible_with_Option(&g_stSem_s);
        return ret;
    }

    if (1 == g_ChanContext[ChanID].OutputImgEn)
    {
        if ((NULL!=g_pImageQue) && (ChanID>=TRUSTED_CHAN_BEGIN) && (ChanID<TOTAL_MAX_CHAN_NUM) && (NULL!=pImage))
        {
            pImgList = &g_pImageQue[ChanID].StReleaseImg;
            pImgList->StImage[pImgList->Tail].image_id = pImage->image_id;
            pImgList->StImage[pImgList->Tail].image_id_1 = pImage->image_id_1;
            pImgList->StImage[pImgList->Tail].top_luma_phy_addr = pImage->top_luma_phy_addr;
            pOutputRec = g_pImageQue[ChanID].StOutputRec;
            for (i=0; i<TRUSTED_MAX_IMAGE_NUM; i++)
            {
                if ((pOutputRec[i].IsUsed == 1) && (pImage->top_luma_phy_addr == pOutputRec[i].TopLumaPhyAddr))
                {
                    pOutputRec[i].TopLumaPhyAddr = 0;
                    pOutputRec[i].IsUsed      = 0;
                    break;
                }
            }
            pImgList->Tail = (pImgList->Tail + 1) % TRUSTED_MAX_IMAGE_NUM;
            ret = VDEC_OK;
        }
    }

    //Up_Interruptible_with_Option(&g_stSem_s);
    return ret;
}

/************************************************************************
   安全侧触发的中断处理程序
 ************************************************************************/
SINT32 TVP_VDEC_SecNotify_Isr(SINT32 irq, VOID *dev_id)
{
    OSAL_GiveEvent_s(&g_SecThreadEvent);
    return 1;
}

/************************************************************************
   非安全侧线程，管理码流和callback信息
 ************************************************************************/
SINT32 TVP_VDEC_MiddleWare(VOID *pArgs)
{
    SINT32 NextInstID;
    STREAM_DATA_S *pStrRawPacket;
    TEEC_Operation operation;
    TEEC_Result result;

#if (1 == PRINT_TEE_TIME)
    static UINT32 BeginTime = 0;
    static UINT32 StartTime = 0;
    static UINT32 EndTime = 0;
    static UINT32 TotalTime = 0;
    static UINT32 Count = 0;
#endif

    while (1)
    {
        //SecPrint(PRN_FATAL, "%s:%d\n", __func__, __LINE__);
        switch (g_THREAD_STATE)
        {
            case THREAD_RUN:
                break;

            case THREAD_SLEEP:
                goto sleep;

            case THREAD_EXIT:
                goto exit;

            default:
                ;
        }

        /*读码流*/
        for ( NextInstID = TRUSTED_CHAN_BEGIN; NextInstID < TOTAL_MAX_CHAN_NUM; NextInstID++ )
        {
            if (NULL != g_StreamIntf[NextInstID].read_stream
                && CHAN_START == g_ChanContext[NextInstID].ChanState)
            {
                Down_Interruptible_with_Option(&g_ChanContext[NextInstID].ChanSema);

                while (1)
                {
                    if (((g_pStreamBuf[NextInstID].Head_NS + 1) % MAX_RAW_NUM) != g_pStreamBuf[NextInstID].Tail_NS)
                    {
                        pStrRawPacket = &(g_pStreamBuf[NextInstID].RawPacket[g_pStreamBuf[NextInstID].Head_NS]);

                        if (0 == g_StreamIntf[NextInstID].read_stream(g_StreamIntf[NextInstID].stream_provider_inst_id, pStrRawPacket))
                        {
                            g_pStreamBuf[NextInstID].Head_NS = (g_pStreamBuf[NextInstID].Head_NS + 1) % MAX_RAW_NUM;

                        #if (1 == DEBUG_SAVE_SUPPORT)
                            TVP_VDEC_SaveStream(NextInstID, g_RawSaveEnable, pStrRawPacket->PhyAddr, pStrRawPacket->Length);
                        #endif
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                Up_Interruptible_with_Option(&g_ChanContext[NextInstID].ChanSema);
            }
        }

#if (1 == PRINT_TEE_TIME)
        StartTime = OSAL_GetTimeInUs();
        if (Count == 0)
        {
            BeginTime = StartTime;
        }
#endif

        /*调用安全侧的主线程函数*/
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
        operation.started = 1;

        result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VFMW_THREADPROC, &operation, NULL);

        if (TEEC_SUCCESS != result)
        {
            SecPrint(PRN_FATAL, "InvokeCommand VFMW_CMD_ID_VFMW_THREADPROC Failed!\n");
            return VDEC_ERR;
        }
#ifdef VDH_DISTRIBUTOR_ENABLE
        VDH_Secure_WakeUp_Distributor();
#endif

#if (1 == PRINT_TEE_TIME)
        EndTime = OSAL_GetTimeInUs();
        TotalTime += EndTime - StartTime;
        Count++;

        if (EndTime - BeginTime >= 1000000)
        {
            SecPrint(PRN_ALWS, "THREADPROC Total: %d, Count: %d, Avg: %d us\n", TotalTime, Count, TotalTime / Count);
            TotalTime = BeginTime = StartTime = EndTime = 0;
            Count = 0;
        }
#endif

        /*上报callback信息*/
		TVP_VDEC_ProcessCallBack();

        /*释放码流*/
        for ( NextInstID = TRUSTED_CHAN_BEGIN; NextInstID < TOTAL_MAX_CHAN_NUM; NextInstID++ )
        {
            if (NULL != g_StreamIntf[NextInstID].release_stream
                && (CHAN_START == g_ChanContext[NextInstID].ChanState))
            {
                Down_Interruptible_with_Option(&g_ChanContext[NextInstID].ChanSema);

                while (1)
                {
                    if (g_pStreamBuf[NextInstID].Tail_NS != g_pStreamBuf[NextInstID].Tail_S)
                    {
                        pStrRawPacket = &(g_pStreamBuf[NextInstID].RawPacket[g_pStreamBuf[NextInstID].Tail_NS]);

                        if (0 == g_StreamIntf[NextInstID].release_stream(g_StreamIntf[NextInstID].stream_provider_inst_id, pStrRawPacket))
                        {
                            //SecPrint(PRN_FATAL, "g_pStreamBuf[%d].Tail_S = %d Tail_NS = %d \n", NextInstID, g_pStreamBuf[NextInstID].Tail_S, g_pStreamBuf[NextInstID].Tail_NS);
                            g_pStreamBuf[NextInstID].Tail_NS = (g_pStreamBuf[NextInstID].Tail_NS + 1) % MAX_RAW_NUM;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                Up_Interruptible_with_Option(&g_ChanContext[NextInstID].ChanSema);
            }
        }

    sleep:
        OSAL_WaitEvent_s(&g_SecThreadEvent, 10);
    }

exit:
    g_THREAD_STATE = THREAD_INVALID;
    return VDEC_OK;
}

/************create thread*******************/
SINT32 TVP_VDEC_Thread_Init(VOID)
{
    OSAL_TASK pTask;
    OSAL_InitEvent_s(&g_SecThreadEvent, 0);

    OSAL_CreateTask((VOID *)&pTask, "SecVideoDec", TVP_VDEC_MiddleWare);

    if ( NULL == pTask)
    {
        SecPrint(PRN_FATAL, "Creat thread SecVideoDec Failed!\n");
        return VDEC_ERR;
    }

    g_THREAD_STATE = THREAD_SLEEP;

    return VDEC_OK;
}

/************reset chan release_stream*******************/
SINT32 TVP_VDEC_Release_Stream(SINT32 ChanID)
{
    STREAM_DATA_S *pStrRawPacket;

    /*清还未被安全侧读取的码流*/
    if (NULL != g_StreamIntf[ChanID].release_stream)
    {
        while (1)
        {
            if (g_pStreamBuf[ChanID].Tail_NS != g_pStreamBuf[ChanID].Head_NS)
            {
                pStrRawPacket = &(g_pStreamBuf[ChanID].RawPacket[g_pStreamBuf[ChanID].Tail_NS]);

                if (0 == g_StreamIntf[ChanID].release_stream(g_StreamIntf[ChanID].stream_provider_inst_id, pStrRawPacket))
                {
                    g_pStreamBuf[ChanID].Tail_NS = (g_pStreamBuf[ChanID].Tail_NS + 1) % MAX_RAW_NUM;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    memset(&g_pStreamBuf[ChanID], 0, sizeof(RAW_ARRAY_NS));

    return VDEC_OK;
}

/********清相应通道的callback信息******/
SINT32 TVP_VDEC_Release_Callback(SINT32 ChanID)
{
    SINT32 Tail;

    Tail = g_pCallbackBuf->Tail;

    while (Tail != g_pCallbackBuf->Head)
    {
        if (ChanID == g_pCallbackBuf->Message[Tail].ChanID)
        {
            g_pCallbackBuf->Message[Tail].IsValid = 0;
        }

        Tail = (Tail + 1) % MAX_EVENT_NUM;
    }

    return VDEC_OK;
}

SINT32 TVP_VDEC_Suspend(VOID)
{
    SINT32 ret = VDEC_OK;
    TEEC_Result result;
    TEEC_Operation operation;

    Down_Interruptible_with_Option(&g_stSem_s);

    if (HI_TRUE == g_bSecEnvSetUp)
    {
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

        operation.started = 1;
        operation.params[0].value.a = 0;
        operation.params[0].value.b = TEEC_VALUE_UNDEF;

        result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_SUSPEND, &operation, NULL);

        if (result != TEEC_SUCCESS)
        {
            SecPrint(PRN_FATAL, "InvokeCommand VFMW_CMD_ID_VDEC_SUSPEND Failed!\n");
            Up_Interruptible_with_Option(&g_stSem_s);
        }

        g_THREAD_STATE = THREAD_SLEEP;
        ret = operation.params[0].value.a;
    }

    Up_Interruptible_with_Option(&g_stSem_s);

    return ret;
}

SINT32 TVP_VDEC_Resume(VOID)
{
    SINT32 ret = VDEC_OK;
    TEEC_Result result;
    TEEC_Operation operation;

    Down_Interruptible_with_Option(&g_stSem_s);

    if (HI_TRUE == g_bSecEnvSetUp)
    {
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

        operation.started = 1;
        operation.params[0].value.a = 0;
        operation.params[0].value.b = TEEC_VALUE_UNDEF;

        result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_RESUME, &operation, NULL);

        if (result != TEEC_SUCCESS)
        {
            SecPrint(PRN_FATAL, "InvokeCommand VFMW_CMD_ID_VDEC_RESUME Failed!\n");
            Up_Interruptible_with_Option(&g_stSem_s);
        }

        g_THREAD_STATE = THREAD_RUN;
        ret = operation.params[0].value.a;
    }

    Up_Interruptible_with_Option(&g_stSem_s);

    return ret;
}

SINT32 TVP_VDEC_SetDbgOption(UINT32 opt, UINT8 *p_args)
{
    return VCTRL_SetDbgOption(opt, p_args);
}


/************************************************************************
    VDEC控制引擎
    ChanID:  需要操作的通道号(对于通道无关操作，此参数可为任意值)
    eCmdID:  命令编码，指定需要VDEC执行何种动作
    pArgs:   命令参数，其格式与eCmdID相关
 ************************************************************************/
SINT32 TVP_VDEC_Control( SINT32 ChanID, VDEC_CID_E eCmdID, VOID *pArgs )
{
    TEEC_Result result;
    TEEC_Operation operation;
    IMAGE_INTF_S *pImgIntf;
    VDEC_CHAN_RESET_OPTION_S *opt;

    SINT8  *vir_addr = NULL;
    SINT64 *p64 = NULL;
    SINT32 *p32 = NULL;
    UADDR  phy_addr = 0;
    SINT32 IsSecFlag = 0;
    SINT32 ret = VDEC_ERR;

#if (1 == PRINT_TEE_TIME)
    static UINT32 BeginTime = 0;
    static UINT32 StartTime = 0;
    static UINT32 EndTime = 0;
    static UINT32 TotalTime = 0;
    static UINT32 Count = 0;
#endif

    if(INVALID_HANDLE != ChanID)
    {
        IsSecFlag = g_ChanContext[ChanID].bIsSecMode;
    }
    else
    {
        if (VDEC_CID_CREATE_CHAN_WITH_OPTION == eCmdID || VDEC_CID_CREATE_CHAN == eCmdID)
        {
            p64 = (SINT64*)pArgs;
            IsSecFlag = ((VDEC_CHAN_OPTION_S*)UINT64_PTR(p64[1]))->s32IsSecMode;
        }
    }

    if (IsSecFlag)
    {
        //if (eCmdID != VDEC_CID_GET_CHAN_STATE)
        //{
            //SecPrint(PRN_ERROR, "Secure Command: ChanId = %d, CmdID = %d\n", ChanID, eCmdID);
        //}
#ifdef VFMW_VPSS_BYPASS_EN
        switch (eCmdID)
        {
            case VDEC_CID_SET_IMG_OUTPUT_INFO:
                if ((NULL != pArgs) && (ChanID >= TRUSTED_CHAN_BEGIN) && (ChanID < TOTAL_MAX_CHAN_NUM))
                {
                    g_ChanContext[ChanID].OutputImgEn = *(SINT32*)pArgs;
                    ret = VDEC_OK;
                }
                return ret;
                break;
                
            case VDEC_CID_REPORT_OCCUQIED_FRM:
                if ((NULL != pArgs) && (ChanID >= TRUSTED_CHAN_BEGIN) && (ChanID < TOTAL_MAX_CHAN_NUM))
                {
                    ret = TVP_VDEC_FindOccupiedFrame(ChanID, pArgs);
                }
                return ret;
                break;
                
            default:
                break;
        }
#endif
        Down_Interruptible_with_Option(&g_stSem_s);

        if (HI_FALSE == g_bSecEnvSetUp)
        {
            SecPrint(PRN_FATAL, "Trusted Decoder Invalid!!\n");
            Up_Interruptible_with_Option(&g_stSem_s);
            return VDEC_ERR;
        }

        phy_addr = (NULL == pArgs)? 0:(__pa(pArgs));

        switch (eCmdID)
        {
            case VDEC_CID_CREATE_CHAN_WITH_OPTION:
            case VDEC_CID_GET_CHAN_DETAIL_MEMSIZE_WITH_OPTION:
                p64 = (SINT64 *)pArgs;
                p32 = (SINT32 *)pArgs;
                vir_addr = UINT64_PTR(p64[1]);
                memcpy(&g_pSecVfmwMem->ChanOption, vir_addr, sizeof(VDEC_CHAN_OPTION_S));
                break;

            case VDEC_CID_GET_CHAN_STATE:
                if ((NULL != pArgs) && (NULL != g_pSecVfmwMem) && 
                    (ChanID >=TRUSTED_CHAN_BEGIN) && (ChanID < TOTAL_MAX_CHAN_NUM))
                {
                    memcpy(pArgs, &(g_pSecVfmwMem->ChanState[ChanID]), sizeof(VDEC_CHAN_STATE_S));
                    ret = VDEC_OK;
                }
                else
                {
                    SecPrint(PRN_FATAL, "VDEC_CID_GET_CHAN_STATE Invalid Param\n");
                }
                goto Exit_Entry;
                break;

            case VDEC_CID_SET_STREAM_INTF:  /* 设置通道的码流接口 */
                if (NULL != pArgs)
                {
                    memcpy(&g_StreamIntf[ChanID], (STREAM_INTF_S *)pArgs, sizeof(STREAM_INTF_S));
                    ret = VDEC_OK;
                }
                else
                {
                    SecPrint(PRN_FATAL, "VDEC_CID_SET_STREAM_INTF Invalid Param\n");
                }
                goto Exit_Entry;
                break;

            case VDEC_CID_GET_IMAGE_INTF:  /* 获取通道的图象接口 */
                if (NULL != pArgs)
                {
                    pImgIntf = (IMAGE_INTF_S *)pArgs;
                    pImgIntf->image_provider_inst_id = ChanID;
                    pImgIntf->read_image = TVP_VDEC_GetChanImage;
                    pImgIntf->release_image = TVP_VDEC_ReleaseChanImage;
                    ret = VDEC_OK;
                }
                else
                {
                    SecPrint(PRN_FATAL, "VDEC_CID_GET_IMAGE_INTF Invalid Param\n");
                }

                goto Exit_Entry;
                break;

            default:
                break;
        }

        operation.started = 1;
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);
        operation.params[0].value.a = ChanID;
        operation.params[0].value.b = eCmdID;
        operation.params[1].value.a = phy_addr;
        operation.params[1].value.b = TEEC_VALUE_UNDEF;

#if (1 == PRINT_TEE_TIME)
        StartTime = OSAL_GetTimeInUs();

        if (Count == 0)
        {
            BeginTime = StartTime;
        }

#endif

        result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_CONTROL, &operation, NULL);
        if (TEEC_SUCCESS != result)
        {
            SecPrint(PRN_FATAL, "TEEC_InvokeCommand VFMW_CMD_ID_VDEC_CONTROL Failed!\n");
            goto Exit_Entry;
        }

#if (1 == PRINT_TEE_TIME)
        EndTime = OSAL_GetTimeInUs();
        TotalTime += EndTime - StartTime;
        Count++;

        if (EndTime - BeginTime >= 1000000)
        {
            SecPrint(PRN_ALWS, "Control Total: %d, Count: %d, Avg: %d us\n", TotalTime, Count, TotalTime / Count);
            TotalTime = BeginTime = StartTime = EndTime = 0;
            Count = 0;
        }
#endif

        ret = operation.params[0].value.a;

        switch (eCmdID)
        {
            case VDEC_CID_CREATE_CHAN_WITH_OPTION:
                if (VDEC_OK == ret)
                {
                    g_ChanContext[p32[0]].ChanID = p32[0];
                    g_ChanContext[p32[0]].bIsSecMode = HI_TRUE;
                    Sema_Init_with_Option(&g_ChanContext[p32[0]].ChanSema);
                    g_SecureInstNum++;
                }

                if (NULL != vir_addr)
                {
                    memcpy(vir_addr, &g_pSecVfmwMem->ChanOption, sizeof(VDEC_CHAN_OPTION_S));
                }

                break;

            case VDEC_CID_GET_CHAN_DETAIL_MEMSIZE_WITH_OPTION:
                if (NULL != vir_addr)
                {
                    memcpy(vir_addr, &g_pSecVfmwMem->ChanOption, sizeof(VDEC_CHAN_OPTION_S));
                }

                break;

            case VDEC_CID_START_CHAN:
                g_ChanContext[ChanID].ChanState = CHAN_START;
                g_ChanContext[ChanID].OutputImgEn = 1;
                g_THREAD_STATE = THREAD_RUN;
                break;

            case VDEC_CID_STOP_CHAN:
                g_ChanContext[ChanID].ChanState = CHAN_STOP;
                break;

            case VDEC_CID_DESTROY_CHAN:
            case VDEC_CID_DESTROY_CHAN_WITH_OPTION:
                Down_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                TVP_VDEC_Release_Callback(ChanID);
                TVP_VDEC_Release_Stream(ChanID);
                memset(&g_pImageQue[ChanID], 0, sizeof(IMAGE_QUEUE_NS));

                g_SecureInstNum--;

                if (g_SecureInstNum <= 0)
                {
                    g_SecureInstNum = 0;
                    g_THREAD_STATE = THREAD_SLEEP;
                }

                if (g_ChanContext[ChanID].pRawFile != NULL)
                {
                    filp_close(g_ChanContext[ChanID].pRawFile, NULL);
                    g_ChanContext[ChanID].pRawFile = NULL;
                }

                g_ChanContext[ChanID].ChanState = CHAN_INVALID;
                Up_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                break;

            case VDEC_CID_RESET_CHAN:
                Down_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                TVP_VDEC_Release_Callback(ChanID);
                TVP_VDEC_Release_Stream(ChanID);
                memset(&g_pImageQue[ChanID], 0, sizeof(IMAGE_QUEUE_NS));
                Up_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                break;

            case VDEC_CID_RESET_CHAN_WITH_OPTION:
                opt = (VDEC_CHAN_RESET_OPTION_S *)pArgs;

                if (opt->s32KeepBS == 0)
                {
                    Down_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                    TVP_VDEC_Release_Stream(ChanID);//释放还未被安全侧读取的码流
                    TVP_VDEC_Release_Callback(ChanID);
                    memset(&g_pImageQue[ChanID], 0, sizeof(IMAGE_QUEUE_NS));
                    Up_Interruptible_with_Option(&g_ChanContext[ChanID].ChanSema);
                }
                break;

            default:
                break;
        }

    Exit_Entry:
        Up_Interruptible_with_Option(&g_stSem_s);

        return ret;
    }
    else
    {
        ret = VDEC_Control(ChanID, eCmdID, pArgs);

        if (ret == VDEC_OK)
        {
            switch (eCmdID)
            {
                case VDEC_CID_CREATE_CHAN:
                case VDEC_CID_CREATE_CHAN_WITH_OPTION:
                    p32 = (SINT32 *)pArgs;
                    g_ChanContext[p32[0]].ChanID = p32[0];
                    g_ChanContext[p32[0]].bIsSecMode = HI_FALSE;
                    break;

                case VDEC_CID_START_CHAN:
                    g_ChanContext[ChanID].ChanState = CHAN_START;
                    break;

                case VDEC_CID_STOP_CHAN:
                    g_ChanContext[ChanID].ChanState = CHAN_STOP;
                    break;

                case VDEC_CID_DESTROY_CHAN:
                case VDEC_CID_DESTROY_CHAN_WITH_OPTION:
                    g_ChanContext[ChanID].ChanState = CHAN_INVALID;
                    break;

                default:
                    break;
            }
        }

        return ret;
    }
}

#ifndef  HI_ADVCA_FUNCTION_RELEASE
static inline SINT32 TVP_VDEC_String2Value(HI_PCHAR str, UINT32 *data)
{
    UINT32 i, d, dat, weight;

    dat = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        i = 2;
        weight = 16;
    }
    else
    {
        i = 0;
        weight = 10;
    }

    for (; i < 10; i++)
    {
        if (str[i] < 0x20)
        {
            break;
        }
        else if (weight == 16 && str[i] >= 'a' && str[i] <= 'f')
        {
            d = str[i] - 'a' + 10;
        }
        else if (weight == 16 && str[i] >= 'A' && str[i] <= 'F')
        {
            d = str[i] - 'A' + 10;
        }
        else if (str[i] >= '0' && str[i] <= '9')
        {
            d = str[i] - '0';
        }
        else
        {
            return -1;
        }

        dat = dat * weight + d;
    }

    *data = dat;

    return 0;
}

VOID TVP_VDEC_HelpProc(VOID)
{
    HI_DRV_PROC_EchoHelper("\n"
                           "================= SEC_VFMW HELP =================\n"
                           "USAGE:echo [cmd] [para] >/proc/sec_vfmw\n"
                           "  help,     [not_care]    :read help infomation\n"
                           "  saveraw,  [start/stop]  :enable/disable raw save,debug only\n"
                           "  saveyuv,  [start/stop]  :enable/disable yuv save,debug only\n"
                           "\n");
    HI_DRV_PROC_EchoHelper(""
                           "Further command avalible in trusted decoder:\n"
                           "  0x0,      [print_word]  :set print enable word\n"
                           "  0x2,      [0~100]       :set err thr\n"
                           "  0x4,      [0/1]         :set output order(0/1=DISP/DEC)\n"
                           "  0x5,      [0/1/2]       :set dec mode(0/1/2=IPB/IP/I)\n"
                           "  0x7,      [0~100]       :set discard before dec thr\n");
    HI_DRV_PROC_EchoHelper(""
                           "  0xb,      [0/1]         :set frame/adaptive(0/1) storage\n"
                           "  0xd,      [8~15]        :set to abserve specify channel\n"
                           "  0x400,    [ms]          :set dec task schedule delay\n"
                           "  0x402,    [0/1]         :start/stop syntax proccess\n"
                           "  0x501,    [ms]          :set scd state period\n"
                           "  0x502,    [ms]          :set vdh state period\n"
                           "  0x503,    [ms]          :set rcv/rls frame period\n"
                           "=================================================\n"
                           "\n");

    return;
}

SINT32 TVP_VDEC_ParamProccess(const SINT8 __user *buffer, size_t count, UINT32 *p_option, UINT32 *p_value)
{
    SINT32  i, j;
    SINT32  ret = HI_FAILURE;
    HI_CHAR buf[PROC_CMD_LEN * 2];
    HI_CHAR str1[PROC_CMD_LEN];
    HI_CHAR str2[PROC_CMD_LEN];

    if (count < 1 || count >= sizeof(buf))
    {
        SecPrint(PRN_ALWS, "Parameter count(%d) Invalid!\n", count);
        return HI_FAILURE;
    }

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, buffer, count))
    {
        SecPrint(PRN_ALWS, "Copy from user Failed!\n");
        return HI_FAILURE;
    }

    buf[count] = 0;

    /* 1.读取参数1 */
    i = 0;
    j = 0;

    for (; i < count; i++)
    {
        if (j == 0 && buf[i] == ' ')
        {
            continue;
        }

        if (buf[i] > ' ')
        {
            str1[j++] = buf[i];
        }

        if (j > 0 && buf[i] == ' ')
        {
            break;
        }
    }

    str1[j] = 0;

    /* 2.读取参数2 */
    j = 0;

    for (; i < count; i++)
    {
        if (j == 0 && buf[i] == ' ')
        {
            continue;
        }

        if (buf[i] > ' ')
        {
            str2[j++] = buf[i];
        }

        if (j > 0 && buf[i] == ' ')
        {
            break;
        }
    }

    str2[j] = 0;

    /* 3.参数判断 */
    if (!HI_OSAL_Strncmp(str1, PROC_CMD_HELP, PROC_CMD_LEN))
    {
        TVP_VDEC_HelpProc();
    }

#if (1 == DEBUG_SAVE_SUPPORT)
    else if (!HI_OSAL_Strncmp(str1, PROC_CMD_SAVERAW, PROC_CMD_LEN))
    {
        if (!HI_OSAL_Strncmp(str2, PROC_CMD_START, PROC_CMD_LEN))
        {
            SecPrint(PRN_ALWS, "Enable Raw Stream Save.\n");
            g_RawSaveEnable = HI_TRUE;
        }
        else if (!HI_OSAL_Strncmp(str2, PROC_CMD_STOP, PROC_CMD_LEN))
        {
            SecPrint(PRN_ALWS, "Disable Raw Stream Save.\n");
            g_RawSaveEnable = HI_FALSE;
        }
    }

#endif
    else if (!HI_OSAL_Strncmp(str1, PROC_CMD_SUSPEND, PROC_CMD_LEN))
    {
        TVP_VDEC_Suspend();
    }
    else if (!HI_OSAL_Strncmp(str1, PROC_CMD_RESUME, PROC_CMD_LEN))
    {
        TVP_VDEC_Resume();
    }
    else
    {
        /*转化参数1*/
        if (TVP_VDEC_String2Value(str1, p_option) != 0)
        {
            SecPrint(PRN_ALWS, "Unknown param %s\n", str1);
        }
        /*转化参数2*/
        else if (TVP_VDEC_String2Value(str2, p_value) != 0)
        {
            SecPrint(PRN_ALWS, "Unknown param %s\n", str2);
        }
        /*两个参数都有效*/
        else
        {
            switch (*p_option)
            {
                case PROC_CMD_SETPRINT:
                    g_SecPrintEnable = *p_value;
                    SecPrint(PRN_ALWS, "Set SecPrintEnable = 0x%x\n", g_SecPrintEnable);
                    break;

                default: /*默认参数必需通过TEE传递*/
                    if (HI_FALSE == g_bSecEnvSetUp)
                    {
                        SecPrint(PRN_ALWS, "Trusted decoder NOT create yet!\n");
                    }

                    break;
            }

            /*安全解码器有效时才返回HI_SUCCESS*/
            if (HI_TRUE == g_bSecEnvSetUp)
            {
                ret = HI_SUCCESS;
            }
        }
    }

    return ret;
}

SINT32 TVP_VDEC_SendProcCommand(UINT32 option, UINT32 value)
{
    TEEC_Result result;
    TEEC_Operation operation;

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    operation.params[0].value.a = option;
    operation.params[0].value.b = value;

    result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VFMW_WRITEPROC, &operation, NULL);

    if (result != TEEC_SUCCESS)
    {
        SecPrint(PRN_ALWS, "InvokeCommand VFMW_CMD_ID_VFMW_WRITEPROC Failed!\n");
        return VDEC_ERR;
    }

    return operation.params[0].value.a;
}

static SINT32 TVP_VDEC_ReadProc(struct seq_file *p, VOID *v)
{
    UINT32 i;
    TEEC_Result result;
    TEEC_Operation operation;
	SINT8 *proc_buf = (SINT8 *)(g_pProcBuf);

    Down_Interruptible_with_Option(&g_stSem_s);

    //在没启动解码器之前应该允许查询部分信息
    PROC_PRINT(p, "\n");
    PROC_PRINT(p, "======================= Global Info ========================\n");
    PROC_PRINT(p, "SecEnvSetUp      :%-7d  | SecPrintEnable      :0x%-5x\n", g_bSecEnvSetUp,  g_SecPrintEnable);
#ifdef HI_SMMU_SUPPORT
    PROC_PRINT(p, "ThreadState      :%-7d  | VfmwMemPhyAddr      :0x%-5x\n", g_THREAD_STATE,  g_SecVfmwMem.MmuAddr);
#else
    PROC_PRINT(p, "ThreadState      :%-7d  | VfmwMemPhyAddr      :0x%-5x\n", g_THREAD_STATE,  g_SecVfmwMem.PhyAddr);
#endif
    PROC_PRINT(p, "SecureInstNum    :%-7d  | VfmwMemLength       :%d\n",     g_SecureInstNum, g_SecVfmwMem.Length);
#if (1 == DEBUG_SAVE_SUPPORT)
    PROC_PRINT(p, "RawSaveEnable    :%-7d  | SavePath            :%s\n",     g_RawSaveEnable, g_SavePath);
#endif
    PROC_PRINT(p, "\n");

    for (i = 0; i < TOTAL_MAX_CHAN_NUM; i++)
    {
        if (g_ChanContext[i].ChanState != CHAN_INVALID)
        {
    		PROC_PRINT(p, "Chan %d\n", i);
    		PROC_PRINT(p, "State            :%-7d  | SecMode             :%d\n", g_ChanContext[i].ChanState, g_ChanContext[i].bIsSecMode);

            if (HI_TRUE == g_ChanContext[i].bIsSecMode)
            {
    		PROC_PRINT(p, "StreamList       : %d/%d (%d,%d,%d,%d)\n",
                           (g_pStreamBuf[i].Head_S - g_pStreamBuf[i].Tail_S + MAX_RAW_NUM) % MAX_RAW_NUM,
                           (g_pStreamBuf[i].Head_NS - g_pStreamBuf[i].Tail_NS + MAX_RAW_NUM) % MAX_RAW_NUM,
                           g_pStreamBuf[i].Head_S, g_pStreamBuf[i].Tail_S, g_pStreamBuf[i].Head_NS, g_pStreamBuf[i].Tail_NS);
            }
        }
    }

    PROC_PRINT(p, "\n");
    if (g_bSecEnvSetUp && g_SecureInstNum > 0)
    {
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
        operation.started = 1;
        operation.params[0].value.a = 0;
        operation.params[0].value.b = MAX_PROC_SIZE;

        result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VFMW_READPROC, &operation, NULL);

        if (result != TEEC_SUCCESS)
        {
            SecPrint(PRN_ALWS, "InvokeCommand VFMW_CMD_ID_VFMW_READPROC Failed!\n");
            Up_Interruptible_with_Option(&g_stSem_s);
            return VDEC_ERR;
        }

        PROC_PRINT(p, proc_buf);
    }
    else
    {
        PROC_PRINT(p, " No secure channel exit.\n");
    }

    PROC_PRINT(p, "============================================================\n");
    PROC_PRINT(p, "\n");

    Up_Interruptible_with_Option(&g_stSem_s);

    return 0;
}

SINT32 TVP_VDEC_WriteProc(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    UINT32 option, value;
    SINT32 ret;
    SecPrint(PRN_ALWS, "enter write proc!\n");

    ret = TVP_VDEC_ParamProccess(buffer, count, &option, &value);

    Down_Interruptible_with_Option(&g_stSem_s);

    if (HI_SUCCESS != ret)
    {
        Up_Interruptible_with_Option(&g_stSem_s);
        return count;
    }

    if (HI_TRUE == g_bSecEnvSetUp)
    {
        ret = TVP_VDEC_SendProcCommand(option, value);

        if (HI_SUCCESS != ret)
        {
            SecPrint(PRN_ALWS, "Invalid CMD(%d %d), refer to help.\n", option, value);
            TVP_VDEC_HelpProc();
        }
    }

    Up_Interruptible_with_Option(&g_stSem_s);
    SecPrint(PRN_ALWS, "enter write proc SUCCESS!\n");
    return count;
}
#endif

VOID TVP_VDEC_ContextInit(VOID)
{
    /*全局指针初始化*/
    g_pSecVfmwMem         = NULL;
    g_pCallbackBuf        = NULL;
    g_pStreamBuf          = NULL;

    g_pImageQue           = NULL;
#if (1 == DEBUG_SAVE_SUPPORT)
    g_pSaveStreamBuf      = NULL;
#endif
#ifndef  HI_ADVCA_FUNCTION_RELEASE
    g_pProcBuf            = NULL;
#endif

    /*静态全局变量初始化*/
    g_SecureInstNum       = 0;
    g_TrustDecoderInitCnt = 0;
    g_bSecEnvSetUp        = HI_FALSE;
    g_THREAD_STATE        = THREAD_INVALID;

    /*全局结构体初始化*/
    memset(&g_SecVfmwMem, 0, sizeof(MEM_DESC_S));
    memset(g_StreamIntf,  0, sizeof(STREAM_INTF_S)*TOTAL_MAX_CHAN_NUM);
    memset(&g_CallBack, 0, sizeof(VFMW_CALLBACK_S));

    //因为这个结构体和非安通道相关，不应该放在这里初始化
    //memset(g_ChanContext, 0, sizeof(CHAN_CONTEXT_S)*TOTAL_MAX_CHAN_NUM);
}

SINT32 TVP_VDEC_TrustDecoderInit(VDEC_OPERATION_S *pArgs)
{
    TEEC_Result result;
    TEEC_Operation operation;
    TEEC_UUID svc_id = {0x0D0D0D0D, 0x0D0D, 0x0D0D,
                        {0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}
                       };
    VDEC_OPERATION_S stInitArgs;
    SINT32 ret;
    UADDR phy_addr = 0;

    SecPrint(PRN_ERROR, "%s Enter\n", __func__);

    Down_Interruptible_with_Option(&g_stSem_s);

    if ((NULL == pArgs)||(NULL == pArgs->VdecCallback))
    {
        SecPrint(PRN_FATAL, "%s: Param Invalid!\n", __func__);
        Up_Interruptible_with_Option(&g_stSem_s);
        return VDEC_ERR;
    }

    if (HI_TRUE == g_bSecEnvSetUp)
    {
        SecPrint(PRN_ERROR, "TrustedDecoder already init(%d), return OK.\n", g_TrustDecoderInitCnt);
        g_TrustDecoderInitCnt++;

        ret = TVP_VDEC_SetCallBack(pArgs->eAdapterType, pArgs->VdecCallback);
        if (VDEC_OK != ret)
        {
            SecPrint(PRN_ERROR, "Set CallBack Failed!\n");
        }

        Up_Interruptible_with_Option(&g_stSem_s);
        return ret;
    }

    /* 1. 建立通信环境 */
    result = TEEK_InitializeContext(NULL, &g_TeeContext);
    if (result != TEEC_SUCCESS)
    {
        SecPrint(PRN_FATAL, "TEEC_InitializeContext Failed!\n");
        Up_Interruptible_with_Option(&g_stSem_s);
        return VDEC_ERR;
    }

    /* 2. 建立会话*/
    result = TEEK_OpenSession(&g_TeeContext, &g_TeeSession, &svc_id, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);

    if (result != TEEC_SUCCESS)
    {
        SecPrint(PRN_FATAL, "TEEC_OpenSession Failed!\n");

        goto InitWithfree_0;
    }

    /* 3. 初始化全局信息*/
    TVP_VDEC_ContextInit();

    /* 4. 申请内存，全局信息赋值*/
    if (HI_SUCCESS != KernelMemMalloc("VFMW_Share_Buf", sizeof(MEMORY_NEEDED_SECVFMW_S), 4, 0, &g_SecVfmwMem))
    {
        SecPrint(PRN_FATAL, "Alloc Sec Vfmw Buffer Failed!\n");
        goto InitWithfree_1;
    }
#ifdef HI_SMMU_SUPPORT
    g_SecVfmwMem.PhyAddr = g_SecVfmwMem.MmuAddr;
#endif
    g_pSecVfmwMem = (MEMORY_NEEDED_SECVFMW_S *)(ULONG)(g_SecVfmwMem.VirAddr);
    memset(g_pSecVfmwMem, 0, sizeof(MEMORY_NEEDED_SECVFMW_S));
    g_pCallbackBuf   = (CALLBACK_ARRAY_NS *)(&g_pSecVfmwMem->CallBackBuf);
    g_pStreamBuf     = (RAW_ARRAY_NS *)(g_pSecVfmwMem->StreamBuf);
    g_pImageQue      = (IMAGE_QUEUE_NS *)(g_pSecVfmwMem->ImageQue);
#ifndef  HI_ADVCA_FUNCTION_RELEASE
    g_pProcBuf       = (UINT8 *)(g_pSecVfmwMem->ProcBuf);
#endif
#if (1 == DEBUG_SAVE_SUPPORT)
    g_pSaveStreamBuf = (UINT8 *)(g_pSecVfmwMem->SaveStreamBuf);
#endif

    memcpy(&stInitArgs, pArgs, sizeof(VDEC_OPERATION_S));
    memset(&stInitArgs.stExtHalMem, 0, sizeof(MEM_DESC_S));
    stInitArgs.VdecCallback = (VOID *)g_SecVfmwMem.PhyAddr;

#ifdef VDH_DISTRIBUTOR_ENABLE
    if (g_HalDisable != 1)
    {
        ret = VDH_Get_ShareData(&stInitArgs.ShareData);
        if (ret != VDH_OK)
        {
    	    SecPrint(PRN_FATAL, "%s VDH_Get_ContextData failed\n", __func__);
        }
    }
    /* 同步硬件抽象层控制字*/
    stInitArgs.ShareData.hal_disable = g_HalDisable;
#endif
    /* 4. 同步安全打印控制字*/
    stInitArgs.ShareData.sec_print_word = g_SecPrintEnable;

    phy_addr = __pa((&stInitArgs));

    /* 5. 调用安全侧初始化接口*/
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    operation.params[0].value.a = phy_addr;
    operation.params[0].value.b = TEEC_VALUE_UNDEF;

    result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_INITWITHOPERATION, &operation, NULL);

    if (result != TEEC_SUCCESS || operation.params[0].value.a != VDEC_OK)
    {
        SecPrint(PRN_FATAL, "InvokeCommand VFMW_CMD_ID_VDEC_INITWITHOPERATION failed, result_val=%d\n", operation.params[0].value.a);
        goto InitWithfree_2;
    }

	ret = TVP_VDEC_SetCallBack(pArgs->eAdapterType, pArgs->VdecCallback);
	if (VDEC_OK != ret)
	{
        SecPrint(PRN_ERROR, "Set CallBack Failed!\n");
        goto InitWithfree_2;
	}

    /* 6. 建立安全解码线程*/
    ret = TVP_VDEC_Thread_Init();

    if (ret != VDEC_OK)
    {
        SecPrint(PRN_FATAL, "Call VDEC_Thread_Init() Failed!\n");
        goto InitWithfree_3;
    }

    /* 7. 注册安全侧触发的中断*/
    ret = vfmw_Osal_Func_Ptr_S.pfun_Osal_request_irq(SECURE_NOTIFY_IRQ_NUM, TVP_VDEC_SecNotify_Isr, 0x00000020, "SecInvokeirq", NULL);

    if (ret != VDEC_OK)
    {
        SecPrint(PRN_FATAL, "Call VDEC_Thread_Init() Failed!\n");
        goto InitWithfree_4;
    }
    g_TrustDecoderInitCnt = 1;
    g_bSecEnvSetUp = HI_TRUE;

    SecPrint(PRN_ERROR, "TrustedDecoder init Success.\n");

    Up_Interruptible_with_Option(&g_stSem_s);

    return VDEC_OK;
InitWithfree_4:
    vfmw_Osal_Func_Ptr_S.pfun_Osal_free_irq(SECURE_NOTIFY_IRQ_NUM, NULL);

InitWithfree_3:
    //调用安全侧去初始化函数
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    operation.params[0].value.a = 0;
    operation.params[0].value.b = TEEC_VALUE_UNDEF;
    TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_EXIT, &operation, NULL);

InitWithfree_2:
    KernelMemFree(&g_SecVfmwMem);
    TVP_VDEC_ContextInit();

InitWithfree_1:
    TEEK_CloseSession(&g_TeeSession);

InitWithfree_0:
    //因为close后再finalize会导致挂死，中软暂未解决，先规避，只需close即可，现在可以打开试试了
    TEEK_FinalizeContext(&g_TeeContext);

    Up_Interruptible_with_Option(&g_stSem_s);
    return VDEC_ERR;

}

SINT32 TVP_VDEC_TrustDecoderExit(VOID)
{
    UINT32 i = 0;
    TEEC_Result result;
    TEEC_Operation operation;
    SINT32 ret;

    SecPrint(PRN_ERROR, "%s Enter\n", __func__);

    Down_Interruptible_with_Option(&g_stSem_s);

    if (HI_FALSE == g_bSecEnvSetUp)
    {
        SecPrint(PRN_ERROR, "TrustedDecoder not init, return OK.\n");
        Up_Interruptible_with_Option(&g_stSem_s);
        return VDEC_OK;
    }

    if (g_SecureInstNum > 0)
    {
        SecPrint(PRN_ERROR, "SecureInstNum(%d) > 0, return OK.\n", g_SecureInstNum);
        Up_Interruptible_with_Option(&g_stSem_s);
        return VDEC_OK;
    }

    g_THREAD_STATE = THREAD_EXIT;

    for (i = 0; i < 50; i++)
    {
        if (THREAD_INVALID == g_THREAD_STATE)
        {
            break;
        }
        else
        {
            OSAL_MSLEEP(10);
        }
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.started = 1;
    operation.params[0].value.a = 0;
    operation.params[0].value.b = TEEC_VALUE_UNDEF;

    result = TEEK_InvokeCommand(&g_TeeSession, VFMW_CMD_ID_VDEC_EXIT, &operation, NULL);

    if (result != TEEC_SUCCESS)
    {
        SecPrint(PRN_FATAL, "InvokeCommand VFMW_CMD_ID_VDEC_EXIT Failed!\n");
    }

    ret = operation.params[0].value.a;
    vfmw_Osal_Func_Ptr_S.pfun_Osal_free_irq(SECURE_NOTIFY_IRQ_NUM, NULL);

    /*关闭VFMW的TEEC通信*/
    TEEK_CloseSession(&g_TeeSession);
    // 因为close后再finalize会导致挂死，中软暂未解决，先规避，只需close即可，现在可以打开试试了
    TEEK_FinalizeContext(&g_TeeContext);

    KernelMemFree(&g_SecVfmwMem);
    g_TrustDecoderInitCnt = 0;
    g_bSecEnvSetUp = HI_FALSE;

    SecPrint(PRN_ERROR, "TrustedDecoder deinit Success.\n");

    Up_Interruptible_with_Option(&g_stSem_s);
    return ret;
}

SINT32 TVP_VDEC_Init(SINT32 (*VdecCallback)(SINT32, SINT32, VOID *))
{
    SecPrint(PRN_ERROR, "%s Enter\n", __func__);

    return VDEC_Init(VdecCallback);
}

SINT32 TVP_VDEC_InitWithOperation(VDEC_OPERATION_S *pArgs)
{
    SINT32 ret;

    SecPrint(PRN_ERROR, "%s Enter\n", __func__);

    ret = VDEC_InitWithOperation(pArgs);

    return (VDEC_OK == ret)? HI_SUCCESS: HI_FAILURE;
}

SINT32 TVP_VDEC_Exit(VOID)
{
    SINT32 ret = VDEC_OK;

    SecPrint(PRN_ERROR, "%s Enter\n", __func__);

    ret = VDEC_Exit();

    return ret;
}

SINT32 TVP_VDEC_init_proc (VOID)
{
    HI_CHAR aszBuf[16];
    DRV_PROC_ITEM_S *pstItem;

    /* Create proc */
    snprintf(aszBuf, sizeof(aszBuf), PROC_NAME);
    pstItem = HI_DRV_PROC_AddModule(aszBuf, NULL, NULL);
    if (!pstItem)
    {
        SecPrint(PRN_ALWS, "Create tvp proc entry %s failed!\n", PROC_NAME);
        return HI_FAILURE;
    }

    /* Set functions */
    pstItem->read  = TVP_VDEC_ReadProc;
    pstItem->write = TVP_VDEC_WriteProc;

    SecPrint(PRN_ALWS, "Create TVP VDEC proc entry SUCCESS!\n");

    return HI_SUCCESS;
}

VOID TVP_VDEC_exit_proc(VOID)
{
    HI_CHAR aszBuf[16];

    snprintf(aszBuf, sizeof(aszBuf), PROC_NAME);
    HI_DRV_PROC_RemoveModule(aszBuf);

    return;
}

/************************************************************************
  打开/退出VDEC组件: 在insmod/rmmod时调用如下函数，
  主要职责是创建/销毁proc文件系统
************************************************************************/
VOID TVP_VDEC_OpenModule(VOID)
{
    SINT32 ret = 0;
#ifndef  HI_ADVCA_FUNCTION_RELEASE

    ret = TVP_VDEC_init_proc();
    /*
        struct proc_dir_entry *p;

        p = create_proc_entry(PROC_NAME, 0644, NULL);
        if(NULL == p)
        {
            SecPrint(PRN_ALWS, "Create proc %s Failed!\n", PROC_NAME);
        	return;
        }

        p->read_proc = TVP_VDEC_ReadProc;
        p->write_proc = TVP_VDEC_WriteProc;
    */
#endif

    Sema_Init_with_Option(&g_stSem_s);

	memset(g_ChanContext, 0, sizeof(CHAN_CONTEXT_S)*TOTAL_MAX_CHAN_NUM);
    return;

}

VOID TVP_VDEC_ExitModule(VOID)
{
#ifndef  HI_ADVCA_FUNCTION_RELEASE
    TVP_VDEC_exit_proc();
    //remove_proc_entry(PROC_NAME, NULL);
#endif

    return;
}


