/*********************************************************************************
*
*  Copyright (C) 2014 Hisilicon Technologies Co., Ltd.  All rights reserved.
*
*  This program is confidential and proprietary to Hisilicon Technologies Co., Ltd.
*  (Hisilicon), and may not be copied, reproduced, modified, disclosed to
*  others, published or used, in whole or in part, without the express prior
*  written permission of Hisilicon.
*
***********************************************************************************/

#include <linux/module.h>
//#include <linux/config.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/version.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include "hi_drv_spi.h"
#include "drv_spi_ioctl.h"
#include "hi_common.h"
#include "hi_error_mpi.h"
#include "hi_drv_mem.h"
#include "hi_module.h"
#include "hi_drv_module.h"
#include "hi_drv_mem.h"
#include "hi_drv_gpio.h"


#define SPI_HW_WRITE_REG(addr, value)      ((*(volatile unsigned int *)(addr)) = (value))
#define SPI_HW_READ_REG(addr, ret)         (ret =(*(volatile unsigned int *)(addr)))

#define SYS_PERI_CRG_ADDR   (0xF8A22000)
#define SPI_CRG_OFFSET      (0x70)

#if defined(CHIP_TYPE_hi3798cv200) || defined(CHIP_TYPE_hi3798hv100)
#define SPI0_BASE   0xF8B1A000
#define SPI1_BASE   0xF8B1B000
#else
#define SPI0_BASE   0xF8B1A000
#define SPI1_BASE   0xF8007000
#endif

#define SPI0_IRQ     (32+45)
#define SPI1_IRQ     (32+46)


/* SPI register definition .*/
#define SPI0_CR0              (SPI0_BASE + 0x00)
#define SPI0_CR1              (SPI0_BASE + 0x04)
#define SPI0_DR               (SPI0_BASE + 0x08)
#define SPI0_SR               (SPI0_BASE + 0x0C)
#define SPI0_CPSR             (SPI0_BASE + 0x10)
#define SPI0_IMSC             (SPI0_BASE + 0x14)
#define SPI0_RIS              (SPI0_BASE + 0x18)
#define SPI0_MIS              (SPI0_BASE + 0x1C)
#define SPI0_ICR              (SPI0_BASE + 0x20)
#define SPI0_TXFIFOCR         (SPI0_BASE + 0x28)
#define SPI0_RXFIFOCR         (SPI0_BASE + 0x2C)

#define SPI1_CR0              (SPI1_BASE + 0x00)
#define SPI1_CR1              (SPI1_BASE + 0x04)
#define SPI1_DR               (SPI1_BASE + 0x08)
#define SPI1_SR               (SPI1_BASE + 0x0C)
#define SPI1_CPSR             (SPI1_BASE + 0x10)
#define SPI1_IMSC             (SPI1_BASE + 0x14)
#define SPI1_RIS              (SPI1_BASE + 0x18)
#define SPI1_MIS              (SPI1_BASE + 0x1C)
#define SPI1_ICR              (SPI1_BASE + 0x20)
#define SPI1_TXFIFOCR         (SPI1_BASE + 0x28)
#define SPI1_RXFIFOCR         (SPI1_BASE + 0x2C)


#define SPI_TIME_OUT_COUNT    5000    //us
#define SPI_FIFO_SIZE 512

#if defined(CHIP_TYPE_hi3798mv100) || defined(CHIP_TYPE_hi3796mv100)
#define SPI0_CSGPIO_NO (6*8+0)
#elif  defined(CHIP_TYPE_hi3716mv410) || defined(CHIP_TYPE_hi3716mv420)  \
    || defined(CHIP_TYPE_hi3798cv200) || defined(CHIP_TYPE_hi3798hv100)
#define SPI0_CSGPIO_NO (1*8+4)
#define SPI1_CSGPIO_NO (13*8+5)
#else
#define SPI0_CSGPIO_NO (2*8+3)
#define SPI1_CSGPIO_NO (2*8+4)
#endif

//#define USE_SPI_INTERRUPT


wait_queue_head_t        SpiWaitQueue;
static HI_U8  irq_occur=0;
static HI_U8  spi_gpiocs=0;

HI_VOID SPI_WRITE_REG(HI_U32 RegAddr, HI_S32 value)
{
    void * __iomem ioaddr;
    ioaddr = ioremap_nocache(RegAddr, 0x4);
    if(ioaddr == HI_NULL)
    {
         HI_ERR_SPI("spi w ioremap failed.\n");
    }
    SPI_HW_WRITE_REG(ioaddr,value);
    iounmap(ioaddr);
}

HI_VOID SPI_READ_REG(HI_U32 RegAddr, HI_S32 *value)
{
    void * __iomem ioaddr;
    HI_S32 tmpval;

    ioaddr = ioremap_nocache(RegAddr, 0x4);
    if(ioaddr == HI_NULL)
    {
         HI_ERR_SPI("spi r ioremap failed.\n");
    }
    SPI_HW_READ_REG(ioaddr,tmpval);
    *value = tmpval;
    iounmap(ioaddr);
}

HI_S32 HI_DRV_SPI_Init(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_DRV_MODULE_Register(HI_ID_SPI, "HI_SPI", HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_SPI("register failed 0x%x.\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

HI_S32 HI_DRV_SPI_DeInit(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_DRV_MODULE_UnRegister(HI_ID_SPI);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_SPI("SPI Module unregister failed 0x%x.\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;

}

HI_VOID HI_DRV_SPI_SetCs_Gpio(HI_U8 devId)
{
    if(devId==0)
    {
        if(spi_gpiocs)
        {
            HI_DRV_GPIO_SetDirBit(SPI0_CSGPIO_NO, 0);
        }

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)   \
    || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para2 error!\n");
        return;
#else
        if(spi_gpiocs)
        {
            HI_DRV_GPIO_SetDirBit(SPI1_CSGPIO_NO, 0);//GPIOCS设置为OUT
        }
#endif
    }
    else
    {
        HI_ERR_SPI("para2 error!\n");
    }
}



HI_U8 HI_DRV_SPI_Get_CsCfg(HI_VOID)
{
        if(spi_gpiocs)
            return 1;
        else
            return 0;
}


HI_VOID HI_DRV_SPI_SetCs_Level(HI_U8 devId,HI_U32 Level)
{
    if(HI_DRV_SPI_Get_CsCfg()==0)
    return;
    if(devId==0)
    {
        HI_DRV_GPIO_WriteBit(SPI0_CSGPIO_NO,Level);//CS拉高
    }
    else if(devId==1)
    {
#if     defined(CHIP_TYPE_hi3718mv100)  \
     || defined(CHIP_TYPE_hi3719mv100)  \
     || defined(CHIP_TYPE_hi3798mv100)  \
     || defined(CHIP_TYPE_hi3716mv410)   \
     || defined(CHIP_TYPE_hi3716mv420)  \
     || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para2 error!\n");
        return;
#else
        HI_DRV_GPIO_WriteBit(SPI1_CSGPIO_NO,Level);//CS拉高
#endif
    }
    else
    {
        HI_ERR_SPI("para2 error!\n");
    }
}

HI_VOID HI_DRV_SPI_Set_CsCfg(HI_U8 gpioCs)
{
    if(gpioCs)
        spi_gpiocs=1;
    else
        spi_gpiocs=0;
}


/*
 * SPI reset set or clear.
 *
 */

HI_S32 SPI_EnableClock(HI_U8 devId,HI_BOOL bEnable)
{
	HI_U32 u32Timeout = 0;
	U_PERI_CRG28 uTmpValue;

	if(0 == devId)
	{
	    uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
	    uTmpValue.bits.ssp0_cken = bEnable;
		g_pstRegCrg->PERI_CRG28.u32 = uTmpValue.u32;
		while(u32Timeout < 100)
		{
			uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
			if(uTmpValue.bits.ssp0_cken != bEnable )
			{
				u32Timeout++;
				udelay(1);
			}
			else
			{
				break;
			}
		}
		if(u32Timeout >= 100)
		{
			HI_ERR_SPI("ssp0 set clock timeout!bEnale:%d!\n",bEnable);
			return HI_FAILURE;
		}
	}
	if(1 == devId)
	{
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
    HI_ERR_SPI("para2 error!\n");
        return HI_FAILURE;
#else
		uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
	    uTmpValue.bits.ssp1_cken = bEnable;
		g_pstRegCrg->PERI_CRG28.u32 = uTmpValue.u32;
		while(u32Timeout < 100)
		{
			uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
			if(uTmpValue.bits.ssp1_cken != bEnable )
			{
				u32Timeout++;
				udelay(1);
			}
			else
			{
				break;
			}
		}
		if(u32Timeout >= 100)
		{
			HI_ERR_SPI("ssp1 set clock timeout!bEnale:%d!\n",bEnable);
			return HI_FAILURE;
		}
#endif
	}

	mb();
	return HI_SUCCESS;
}

HI_S32 SPI_Reset(HI_U8 devId,HI_BOOL bReset)
{
	HI_U32 u32Timeout = 0;
	U_PERI_CRG28 uTmpValue;

	if(0 == devId)
	{
	    uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
	    uTmpValue.bits.sci0_srst_req = bReset;
		g_pstRegCrg->PERI_CRG28.u32 = uTmpValue.u32;
		while(u32Timeout < 100)
		{
			uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
			if(uTmpValue.bits.sci0_srst_req != bReset )
			{
				u32Timeout++;
				udelay(1);
			}
			else
			{
				break;
			}
		}
		if(u32Timeout >= 100)
		{
			HI_ERR_SCI("SPI0 reset clock timeout!bReset:%d!\n",bReset);
			return HI_FAILURE;
		}
	}
	if(1 == devId)
	{
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
    HI_ERR_SPI("para2 error!\n");
        return HI_FAILURE;
#else
		uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
	    uTmpValue.bits.sci1_srst_req = bReset;
		g_pstRegCrg->PERI_CRG28.u32 = uTmpValue.u32;
		while(u32Timeout < 100)
		{
			uTmpValue.u32  = g_pstRegCrg->PERI_CRG28.u32;
			if(uTmpValue.bits.sci1_srst_req != bReset )
			{
				u32Timeout++;
				udelay(1);
			}
			else
			{
				break;
			}
		}
		if(u32Timeout >= 100)
		{
			HI_ERR_SCI("SPI1 reset clock timeout!bReset:%d!\n",bReset);
			return HI_FAILURE;
		}
#endif
	}
	mb();
	return HI_SUCCESS;
}

/*
 * SPI reset set or clear.
 *
 */

HI_VOID HI_DRV_SPI_Reset(HI_U8 set, HI_U8 devId)
{
	if(devId > 1)
	{
		HI_ERR_SPI("para2 error!\n");
        return;
	}
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
    	if (devId==1)
       	 	HI_ERR_SPI("para2 error!\n");
        return;
#endif
	
    if (set )
    {
       if(SPI_Reset(devId,HI_TRUE))
	 		return ;
    }
    else
    {
        if(SPI_Reset(devId,HI_FALSE))
	   		return ;
    }

	return;

}

/*
 * enable or disable SPI clock
 *
 */

HI_VOID HI_DRV_SPI_CLKEnable(HI_S32 enable,HI_U8 devId)
{

	if(devId > 1)
	{
		HI_ERR_SPI("para2 error!\n");
        return;
	}
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
    	if (devId==1)
       	 	HI_ERR_SPI("para2 error!\n");
        return;
#endif
	
    if (enable )
    {
       if(SPI_EnableClock(devId,HI_TRUE))
	 		return ;
    }
    else
    {
        if(SPI_EnableClock(devId,HI_FALSE))
	   		return ;
    }

	return;
}

/*
 * enable SPI routine.
 *
 */
HI_VOID HI_DRV_SPI_Enable(HI_U8 devId)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR1,&ret);
        ret |= 0x01<<1;
        SPI_WRITE_REG(SPI0_CR1,ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)

        HI_ERR_SPI("para error!\n");
        return;
#else

        SPI_READ_REG(SPI1_CR1,&ret);
        ret |= 0x01<<1;
        SPI_WRITE_REG(SPI1_CR1,ret);
#endif
    }
    else
    {
         HI_ERR_SPI("para error!\n");
    }
}


/*
 * disable SPI routine.
 *
 */

HI_VOID HI_DRV_SPI_DisEnable(HI_U8 devId)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR1,&ret);
        ret &= ~(0x01<<1);
        SPI_WRITE_REG(SPI0_CR1,ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)

        HI_ERR_SPI("para error!\n");
        return;
#else
        SPI_READ_REG(SPI1_CR1,&ret);
        ret &= ~(0x01<<1);
        SPI_WRITE_REG(SPI1_CR1,ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para error!\n");
    }
}


/* Loop Model  for test
set = 0 :normal
set = 1:loopMod
*/
HI_VOID HI_DRV_SPI_SetLoop(HI_U8 devId,HI_U8 set)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR1,&ret);
        if(set == 0)
        {
            ret &= 0xfe;
            SPI_WRITE_REG(SPI0_CR1,ret);
        }
        else
        {
            ret |= 0x01;
            SPI_WRITE_REG(SPI0_CR1,ret);
        }
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para error!\n");
        return;
#else
        SPI_READ_REG(SPI1_CR1,&ret);
        if(set == 0)
        {
            ret &= 0xfe;
            SPI_WRITE_REG(SPI1_CR1,ret);
        }
        else
        {
            ret |= 0x01;
            SPI_WRITE_REG(SPI1_CR1,ret);
        }
#endif
    }
    else
    {
         HI_ERR_SPI("para error!\n");
    }
}



HI_VOID HI_DRV_SPI_SetFIFO(HI_U8 devId,HI_U8 intsize)
{
    if(devId==0)
    {
        SPI_WRITE_REG(SPI0_RXFIFOCR,intsize<<3);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)   \
    || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para error!\n");
        return;
#else
        SPI_WRITE_REG(SPI1_RXFIFOCR,intsize<<3);
#endif
    }
    else
    {
         HI_ERR_SPI("para error!\n");
    }
}


/*
 * set SPI frame form routine.
 *
 * @param framemode: frame form
 * 00: Motorola SPI frame form.
 * when set the mode,need set SPICLKOUT phase and SPICLKOUT voltage level.
 * 01: TI synchronous serial frame form
 * 10: National Microwire frame form
 * 11: reserved
 * @param sphvalue: SPICLKOUT phase (0/1)
 * @param sp0: SSPCLKOUT voltage level (0/1)
 * @param datavalue: data bit
 * 0000: reserved    0001: reserved    0010: reserved    0011: 4bit data
 * 0100: 5bit data   0101: 6bit data   0110:7bit data    0111: 8bit data
 * 1000: 9bit data   1001: 10bit data  1010:11bit data   1011: 12bit data
 * 1100: 13bit data  1101: 14bit data  1110:15bit data   1111: 16bit data
 *
 * @return value: 0--success; -1--error.
 *
 */

HI_S32 HI_DRV_SPI_SetFrom(HI_U8 devId,HI_U8 framemode,HI_U8 spo,HI_U8 sph,HI_U8 datawidth)
{
    HI_S32 ret = 0;
    HI_INFO_SPI("devId=%d framemode=%d,spo=%d,sph=%d,datawith=%d\n",devId,framemode,spo,sph,datawidth);
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR0,&ret);
        if(framemode > 3)
        {
            //HI_INFO_SPI("set frame parameter err.\n");
            return HI_FAILURE;
        }
        ret = (ret & 0xFFCF) | (framemode << 4);
        if((ret & 0x30) == 0)
        {
            if(spo > 1)
            {
                //HI_INFO_SPI("set spo parameter err.\n");
                return HI_FAILURE;
            }
            if(sph > 1)
            {
                //HI_INFO_SPI("set sph parameter err.\n");
                return HI_FAILURE;
            }
            ret = (ret & 0xFF3F) | (sph << 7) | (spo << 6);
        }
        if((datawidth > 16) || (datawidth < 4))
        {
            //HI_INFO_SPI("set datawidth parameter err.\n");
            return HI_FAILURE;
        }
        ret = (ret & 0xFFF0) | (datawidth -1);
        SPI_WRITE_REG(SPI0_CR0,ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)   \
    || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para1 error!\n");
        return HI_FAILURE;
#else

        SPI_READ_REG(SPI1_CR0,&ret);
        if(framemode > 3)
        {
            //HI_INFO_SPI("set frame parameter err.\n");
            return HI_FAILURE;
        }
        ret = (ret & 0xFFCF) | (framemode << 4);
        if((ret & 0x30) == 0)
        {
            if(spo > 1)
            {
                //HI_INFO_SPI("set spo parameter err.\n");
                return HI_FAILURE;
            }
            if(sph > 1)
            {
                //HI_INFO_SPI("set sph parameter err.\n");
                return HI_FAILURE;
            }
            ret = (ret & 0xFF3F) | (sph << 7) | (spo << 6);
        }
        if((datawidth > 16) || (datawidth < 4))
        {
            //HI_INFO_SPI("set datawidth parameter err.\n");
            return HI_FAILURE;
        }
        ret = (ret & 0xFFF0) | (datawidth -1);
        SPI_WRITE_REG(SPI1_CR0,ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }

    return HI_SUCCESS;
}


HI_S32 HI_DRV_SPI_GetFrom(HI_U8 devId,HI_U8 *framemode,HI_U8 *spo,HI_U8 *sph,HI_U8 *datawidth)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR0,&ret);

        *framemode = (ret & 0x30)>>4;
        *spo = (ret & 0x40)>>6;
        *sph = (ret & 0x80)>>7;
        *datawidth =( ret & 0x0F)+1;
        HI_INFO_SPI("framemode=%d spo=%d sph=%d datawidth=%d\n",*framemode,*spo,*sph,*datawidth);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_FAILURE;
#else
        SPI_READ_REG(SPI1_CR0,&ret);

        *framemode = (ret & 0x30)>>4;
        *spo = (ret & 0x40)>>6;
        *sph = (ret & 0x80)>>7;
        *datawidth =( ret & 0x0F)+1;
        HI_INFO_SPI("framemode=%d spo=%d sph=%d datawidth=%d\n",*framemode,*spo,*sph,*datawidth);
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
    return HI_SUCCESS;
}


/*
 * set SPI serial clock rate routine.
 *
 * @param scr: scr value.(0-255,usually it is 0)
 * @param cpsdvsr: Clock prescale divisor.(2-254 even)
 *
 * @return value: 0--success; -1--error.
 *
 */

HI_S32 HI_DRV_SPI_SetClk(HI_U8 devId,HI_U8 scr,HI_U8 cpsdvsr)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR0,&ret);
        ret = (ret & 0xFF) | (scr << 8);
        SPI_WRITE_REG(SPI0_CR0,ret);
        if((cpsdvsr & 0x1))
        {
            //HI_INFO_SPI("set cpsdvsr parameter err.\n");
            return HI_FAILURE;
        }
        SPI_WRITE_REG(SPI0_CPSR,cpsdvsr);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_FAILURE;
#else

        SPI_READ_REG(SPI1_CR0,&ret);
        ret = (ret & 0xFF) | (scr << 8);
        SPI_WRITE_REG(SPI1_CR0,ret);
        if((cpsdvsr & 0x1))
        {
            //HI_INFO_SPI("set cpsdvsr parameter err.\n");
            return HI_FAILURE;
        }
        SPI_WRITE_REG(SPI1_CPSR,cpsdvsr);
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
    return HI_SUCCESS;
}

HI_VOID HI_DRV_SPI_SetBlend(HI_U8 devId,HI_U8 bBigEnd)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR1,&ret);
        if(bBigEnd)
        {
            ret = ret | (0x1 << 4);
        }
        else
        {
            ret = ret & (~(0x1 << 4));
        }
        SPI_WRITE_REG(SPI0_CR1,ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return ;

#else
        SPI_READ_REG(SPI1_CR1,&ret);
        if(bBigEnd)
        {
            //ret = ret & (~(0x1 << 4)); /* big/little end, 0: little, 1: big */
            ret = ret | (0x1 << 4);
        }
        else
        {
            //ret = ret | (0x1 << 4); /* big/little end,  0: little, 1: big*/
            ret = ret & (~(0x1 << 4));
        }
        SPI_WRITE_REG(SPI1_CR1,ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
}

HI_U8 HI_DRV_SPI_GetBlend(HI_U8 devId)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_CR1,&ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return 0;

#else
        SPI_READ_REG(SPI1_CR1,&ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para error!\n");
    }
    return (ret&0x10)>>4;
}



/*
 * set SPI interrupt routine.
 *
 * @param regvalue: SSP_IMSC register value.(0-255,usually it is 0)
 *
 */
HI_VOID HI_DRV_SPI_SetItr(HI_U8 devId,HI_U8 regvalue)
{
    if(devId==0)
    {
        SPI_WRITE_REG(SPI0_IMSC,(regvalue&0x0f));
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return;

#else
        SPI_WRITE_REG(SPI1_IMSC,(regvalue&0x0f));
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
}


/*
 * clear SPI interrupt routine.
 *
 */

HI_VOID HI_DRV_SPI_ClearItr(HI_U8 devId)
{
    if(devId==0)
    {
        SPI_WRITE_REG(SPI0_ICR,0x3);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)   \
    || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para error!\n");
        return;

#else
        SPI_WRITE_REG(SPI1_ICR,0x3);
#endif
    }
    else
    {
        HI_ERR_SPI("para error!\n");
    }

}


/*
 * check SPI busy state routine.
 *
 * @return value: 0--free; 1--busy.
 *
 */

HI_U32 HI_DRV_SPI_BusyState(HI_U8 devId)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_SR,&ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para error!\n");
        return 2;
#else

        SPI_READ_REG(SPI1_SR,&ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para error!\n");
    }
    if((ret & 0x10) != 0x10)
        return 0;
    else
        return 1;

}

HI_U32 HI_DRV_SPI_FifoEmpty(HI_U8 devId,HI_S32 bSend)
{
    HI_S32 ret = 0;
    if(devId==0)
    {
        SPI_READ_REG(SPI0_SR,&ret);
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return 2;
#else
        SPI_READ_REG(SPI1_SR,&ret);
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
    if (bSend)
    {
        if((ret & 0x1) == 0x1) /* send fifo */
            return 1;
        else
            return 0;
    }
    else
    {
        if((ret & 0x4) == 0x4) /* receive fifo */
            return 0;
        else
            return 1;
    }
}

/*
 *  write SPI_DR register rountine.
 *
 *  @param  sdata: data of SSP_DR register
 *
 */

HI_S32 HI_DRV_SPI_WriteQuery(HI_U8 devId,HI_U8 *Send, HI_U32 SendCnt)
{
    HI_S32 regval;
    HI_S32 i=0;
    HI_S32 count;
    HI_S32 rd;

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);
    if(devId==0)
    {
        for(i=0;i<SendCnt;i++)
        {
            count = 0;
            SPI_WRITE_REG(SPI0_DR,Send[i]);
            do
            {
                SPI_READ_REG(SPI0_SR,&regval);
                count++;
            }while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));
            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI0_DR,&rd);
                SPI_READ_REG(SPI0_SR,&regval);
            }
            while((regval & 0x04) != 0x00);
        }

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<SendCnt;i++)
        {
            count = 0;
            SPI_WRITE_REG(SPI1_DR,Send[i]);
            do
            {
                SPI_READ_REG(SPI1_SR,&regval);
                count++;
            }while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));
            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI1_DR,&rd);
                SPI_READ_REG(SPI1_SR,&regval);
            }
            while((regval & 0x04) != 0x00);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;

}

HI_S32 HI_DRV_SPI_WriteIsr(HI_U8 devId,HI_U8 *Send, HI_U32 SendCnt)
{
    HI_S32 regval;
    HI_S32 i=0;
    HI_S32 rd;

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);
    if(devId==0)
    {
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,Send[i]);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI0_DR,&rd);
                SPI_READ_REG(SPI0_SR,&regval);
            }
            while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x06);
            HI_DRV_SPI_SetItr(1,0x06);
        }

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)   \
    || defined(CHIP_TYPE_hi3796mv100)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,Send[i]);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x06);
                HI_DRV_SPI_SetItr(1,0x06);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI1_DR,&rd);
                SPI_READ_REG(SPI1_SR,&regval);
            }
            while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("para1 error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;
}



HI_S32 HI_DRV_SPI_ReadQuery(HI_U8 devId, HI_U8 *Read, HI_U32 ReadCnt)
{
    HI_S32 regval;
    HI_S32 count;
    HI_S32 i=0,j=0;
    HI_S32 tmp_regval;

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);

    if(devId==0)
    {
        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,0);
            count = 0;
            do
            {
                SPI_READ_REG(SPI0_SR,&regval);
                count++;
            }
            while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));
            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
                {
                	SPI_READ_REG(SPI0_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
                j++;
            }
            while((regval & 0x04) != 0x00);
        }

        HI_INFO_SPI("----spi readcnt=%d----\n",j);

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,0);
            count = 0;
            do
            {
                SPI_READ_REG(SPI1_SR,&regval);
                count++;
            }
            while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));
            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
				{
                	SPI_READ_REG(SPI1_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI1_DR,&tmp_regval);
                    SPI_READ_REG(SPI1_SR,&regval);
                    j++;
            }while((regval & 0x04) != 0x00);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("hi_ssp_readdataex para error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;
}



HI_S32 HI_DRV_SPI_ReadIsr(HI_U8 devId, HI_U8 *Read, HI_U32 ReadCnt)
{
    HI_S32 regval;
    HI_S32 i=0,j=0;
    HI_S32 tmp_regval;

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);

    if(devId==0)
    {
        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,0);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
				{
                	SPI_READ_REG(SPI0_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
                j++;
            }
            while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,0);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
                {
                	SPI_READ_REG(SPI1_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI1_DR,&tmp_regval);
                SPI_READ_REG(SPI1_SR,&regval);
                j++;
            } while ((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("hi_ssp_readdataex para error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;
}


/*
 *  read SPI_DR register rountine.
 *
 *  @return value: data from SSP_DR register readed
 *
 */

HI_S32 HI_DRV_SPI_ReadEx(HI_U8 devId, HI_U8 *Send, HI_U32 SendCnt, HI_U8 *Read, HI_U32 ReadCnt)
{
    HI_S32 regval;
    HI_S32 count;
    HI_S32 tmp_regval;
    HI_S32 i=0,j=0;

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);
    if(devId==0)
    {
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,Send[i]);
            count = 0;
            do
            {
                SPI_READ_REG(SPI0_SR,&regval);
                count++;
            }
            while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));

            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
            }while((regval & 0x04) != 0x00);
        }

        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,0);
            count = 0;
            do
            {
                SPI_READ_REG(SPI0_SR,&regval);
                count++;
            } while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));

            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
				{
                	SPI_READ_REG(SPI0_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
                j++;
            } while((regval & 0x04) != 0x00);
        }

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,Send[i]);
            count = 0;
            do
            {
                SPI_READ_REG(SPI1_SR,&regval);
                count++;
            } while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));

            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI1_DR,&tmp_regval);
                SPI_READ_REG(SPI1_SR,&regval);
            }while((regval & 0x04) != 0x00);
        }

        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,0);
            count = 0;
            do
            {
                SPI_READ_REG(SPI1_SR,&regval);
                count++;
            } while(((regval&0x15)!=0x05)&&(count<SPI_TIME_OUT_COUNT));

            if(count >= SPI_TIME_OUT_COUNT)
            {
                HI_ERR_SPI("SPI_TIME_OUT\n");
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                if(j<ReadCnt)
                {
                	SPI_READ_REG(SPI1_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI1_DR,&tmp_regval);
                SPI_READ_REG(SPI1_SR,&regval);
                j++;
            } while((regval & 0x04) != 0x00);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("hi_ssp_readdataex para error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;
}



HI_S32 HI_DRV_SPI_ReadEx_Isr(HI_U8 devId, HI_U8 *Send, HI_U32 SendCnt, HI_U8 *Read, HI_U32 ReadCnt)
{
    HI_S32 regval;
    HI_S32 i=0,j=0;
    HI_S32 tmp_regval;
    HI_U8 rd[SPI_FIFO_SIZE];

    HI_DRV_SPI_Enable(devId);
    HI_DRV_SPI_SetCs_Level(devId,0);
    memset(rd,0,SPI_FIFO_SIZE);
    if(devId==0)
    {
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,Send[i]);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
            }while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }

        for(i=0;i<ReadCnt;i++)
        {
            SPI_WRITE_REG(SPI0_DR,0);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }

            do
            {
                if (j < ReadCnt)
				{
                	SPI_READ_REG(SPI0_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI0_DR,&tmp_regval);
                SPI_READ_REG(SPI0_SR,&regval);
                j++;
            } while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }

    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para1 error!\n");
        return HI_ERR_SPI_INVALID_PARA;
#else
        for(i=0;i<SendCnt;i++)
        {
            SPI_WRITE_REG(SPI1_DR,Send[i]);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if(regval >0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }
            do
            {
                SPI_READ_REG(SPI1_DR,&tmp_regval);
                SPI_READ_REG(SPI1_SR,&regval);
            }while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);

        }

        for (i = 0; i < ReadCnt; i++)
        {
            SPI_WRITE_REG(SPI1_DR,0);
            regval = wait_event_interruptible_timeout(SpiWaitQueue,(irq_occur==1),msecs_to_jiffies(SPI_TIME_OUT_COUNT));
            if (regval > 0)
            {
                irq_occur=0;
                HI_INFO_SPI("wake up success\n");
            }
            else
            {
                irq_occur=0;
                HI_ERR_SPI("wake up TIMEOUT\n");
                HI_DRV_SPI_SetItr(0,0x6);
                HI_DRV_SPI_SetItr(1,0x6);
                return HI_ERR_SPI_READ_TIMEOUT;
            }

            do
            {
                if (j < ReadCnt)
                {
                	SPI_READ_REG(SPI1_DR,&tmp_regval);
                    Read[j] = tmp_regval & 0xff;
                }
                else
                    SPI_READ_REG(SPI1_DR, &tmp_regval);
                SPI_READ_REG(SPI1_SR, &regval);
                j++;
            } while((regval & 0x04) != 0x00);
            HI_DRV_SPI_SetItr(0,0x6);
            HI_DRV_SPI_SetItr(1,0x6);
        }
#endif
    }
    else
    {
        HI_ERR_SPI("hi_ssp_readdataex para error!\n");
    }
    HI_DRV_SPI_SetCs_Level(devId,1);
    HI_DRV_SPI_DisEnable(devId);
    return HI_SUCCESS;
}


irqreturn_t Spi_Isr(HI_S32 irq, HI_VOID *dev_id)
{
   irq_occur=1;
   HI_INFO_SPI("Spi_Isr para occur!\n");
   wake_up_interruptible(&SpiWaitQueue);
   HI_DRV_SPI_SetItr(0,0x0);
   HI_DRV_SPI_SetItr(1,0x0);
   return IRQ_HANDLED;
}


HI_S32 HI_DRV_SPI_Open(HI_U8 devId)
{
    static HI_CHAR SpiIrqName[12];
    HI_S32 ret=0;
    HI_DRV_SPI_Reset(0,devId);
    HI_DRV_SPI_CLKEnable(1,devId);
    HI_DRV_SPI_DisEnable(devId);
    ret=HI_DRV_SPI_SetClk(devId,4,10);///TODO:USE 2Mhz CLOCK,you may need to modify it
    //ret=HI_DRV_SPI_SetClk(devId,4,2);//5MHz
    HI_DRV_SPI_SetLoop(devId,0);
	memset(SpiIrqName, 0, sizeof(SpiIrqName));
#ifdef USE_SPI_INTERRUPT
    HI_DRV_SPI_SetItr(devId,0x06);
    HI_DRV_SPI_SetFIFO(devId,0x07);
    init_waitqueue_head(&SpiWaitQueue);
    
    if(devId==0)
    {
        snprintf(SpiIrqName, sizeof(SpiIrqName), "hi_spi0_irq");
        ret |= request_irq(SPI0_IRQ, Spi_Isr, IRQF_DISABLED, SpiIrqName, NULL);
        if (ret != HI_SUCCESS)
        {
            HI_FATAL_SPI("register spi Isr failedret=%d.\n",ret);
        }
    }
    else if(devId==1)
    {
#if    defined(CHIP_TYPE_hi3718mv100)   \
    || defined(CHIP_TYPE_hi3719mv100)   \
    || defined(CHIP_TYPE_hi3798mv100)   \
    || defined(CHIP_TYPE_hi3796mv100)   \
    || defined(CHIP_TYPE_hi3716mv410)   \
    || defined(CHIP_TYPE_hi3716mv420)
        HI_ERR_SPI("para error!\n");
        return HI_FAILURE;
#else
        snprintf(SpiIrqName, sizeof(SpiIrqName), "hi_spi1_irq");
        ret |= request_irq(SPI1_IRQ, Spi_Isr, IRQF_DISABLED, SpiIrqName, NULL);
        if (ret != HI_SUCCESS)
        {
            HI_FATAL_SPI("register spi Isr failed ret=%d.\n",ret);
        }
#endif
    }
#else
    HI_DRV_SPI_SetItr(devId,0);
    HI_DRV_SPI_ClearItr(devId);
#endif
    return ret;
}

HI_S32 HI_DRV_SPI_Close(HI_U8 devId)
{
    HI_DRV_SPI_CLKEnable(0,devId);
#ifdef USE_SPI_INTERRUPT
    if(devId==0)
    free_irq(SPI0_IRQ, HI_NULL);
    else
    free_irq(SPI1_IRQ, HI_NULL);
#endif
    return HI_SUCCESS;
}



HI_S32 HI_DRV_SPI_Ioctl(struct file *file, HI_U32 cmd, HI_SIZE_T arg)
{
    HI_S32 Ret = 0;
    HI_U8  *pData = NULL;
    HI_U8  *rData = NULL;
    SPI_DATA_S SSPData;
    SPI_DATAEX_S EXData;
    SPI_FFORM_S SSPFForm;
    SPI_BLEND_S SSPBLEND;

    HI_VOID __user *argp = (HI_VOID __user*)arg;

    switch (cmd)
    {
    case CMD_SPI_OPEN:
        {

            Ret=HI_DRV_SPI_Open(arg);
            //HI_INFO_SPI("ssp open arg=%d\n",arg);
            if(Ret)
                HI_ERR_SPI("ssp open fail!\n");
            break;
        }
    case CMD_SPI_CLOSE:
        {
            Ret=HI_DRV_SPI_Close(arg);
            //HI_INFO_SPI("ssp close arg=%d\n",arg);
            if(Ret)
                HI_ERR_SPI("ssp close fail!\n");
            break;
        }
    case CMD_SPI_WRITE:
        {
            if (copy_from_user(&SSPData, argp, sizeof(SPI_DATA_S)))
            {
                HI_ERR_SPI("ssp write copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }

            pData = HI_KMALLOC(HI_ID_SPI, SSPData.sDataCnt, GFP_KERNEL);
            if (!pData)
            {
                HI_ERR_SPI("ssp write kmalloc fail!\n");
                Ret = HI_ERR_SPI_MALLOC_ERR;
                break;
            }

            memset(pData,0,SSPData.sDataCnt);

            if (copy_from_user(pData, SSPData.sData, SSPData.sDataCnt))
            {
                HI_ERR_SPI("ssp write copy data from user fail!\n");
                HI_KFREE(HI_ID_SPI, pData);
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }

#ifdef USE_SPI_INTERRUPT
            if(HI_DRV_SPI_WriteIsr(SSPData.devId,pData,SSPData.sDataCnt))
            {
                HI_ERR_SPI("ssp write timeout!\n");
                Ret = HI_ERR_SPI_READ_TIMEOUT;
            }
#else
            if(HI_DRV_SPI_WriteQuery(SSPData.devId,pData,SSPData.sDataCnt))
            {
                HI_ERR_SPI("ssp write timeout!\n");
                Ret = HI_ERR_SPI_READ_TIMEOUT;
            }
#endif
            HI_KFREE(HI_ID_SPI,pData);
            break;
        }

    case CMD_SPI_READ:
        {
            if (copy_from_user(&SSPData, argp, sizeof(SPI_DATA_S)))
            {
                HI_INFO_SPI("ssp read copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }

            pData = HI_KMALLOC(HI_ID_SPI,SSPData.sDataCnt, GFP_KERNEL);
            if (!pData)
            {
                HI_ERR_SPI("ssp read ssp kmalloc fail!\n");
                Ret = HI_ERR_SPI_MALLOC_ERR;
                break;
            }
            memset(pData,0,SSPData.sDataCnt);
#ifdef USE_SPI_INTERRUPT
            if(HI_DRV_SPI_ReadIsr(SSPData.devId,pData,SSPData.sDataCnt))
            {
                Ret = HI_ERR_SPI_READ_TIMEOUT;
                HI_KFREE(HI_ID_SPI, pData);
                break;
            }
#else
            if(HI_DRV_SPI_ReadQuery(SSPData.devId,pData,SSPData.sDataCnt))
            {
                Ret = HI_ERR_SPI_READ_TIMEOUT;
                HI_KFREE(HI_ID_SPI, pData);
                break;
            }
#endif
            if (HI_SUCCESS == Ret)
            {
                if (copy_to_user(SSPData.sData, pData, SSPData.sDataCnt))
                {
                    HI_ERR_SPI("ssp read copy data to user fail!\n");
                    Ret = HI_ERR_SPI_COPY_DATA_ERR;
                }
            }

            HI_KFREE(HI_ID_SPI, pData);

            break;
        }

    case CMD_SPI_READEX:
        {
            HI_INFO_SPI("CMD_SSP_READEX\n");
            if (copy_from_user(&EXData, argp, sizeof(SPI_DATAEX_S)))
            {
                HI_ERR_SPI("ssp readex copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }
            pData = HI_KMALLOC(HI_ID_SPI,EXData.sDataCnt, GFP_KERNEL);
            rData = HI_KMALLOC(HI_ID_SPI,EXData.rDataCnt, GFP_KERNEL);
            if ((!pData)||(!rData))
            {
                HI_ERR_SPI("ssp readex kmalloc fail!\n");
                Ret = HI_ERR_SPI_MALLOC_ERR;
                break;
            }
            memset(pData,0,EXData.sDataCnt);
            memset(rData,0,EXData.rDataCnt);
            if (copy_from_user(pData,EXData.sData, EXData.sDataCnt))
            {
                HI_ERR_SPI("ssp readex copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                HI_KFREE(HI_ID_SPI, pData);
                HI_KFREE(HI_ID_SPI, rData);
                break;
            }
#ifdef USE_SPI_INTERRUPT
            if(HI_DRV_SPI_ReadEx_Isr(EXData.devId,pData,EXData.sDataCnt,rData,EXData.rDataCnt))
            {
                Ret = HI_ERR_SPI_READ_TIMEOUT;
                HI_KFREE(HI_ID_SPI, pData);
                HI_KFREE(HI_ID_SPI, rData);
                break;
            }
#else
            if(HI_DRV_SPI_ReadEx(EXData.devId,pData,EXData.sDataCnt,rData,EXData.rDataCnt))
            {
                Ret = HI_ERR_SPI_READ_TIMEOUT;
                HI_KFREE(HI_ID_SPI, pData);
                HI_KFREE(HI_ID_SPI, rData);
                break;
            }
#endif
            if (copy_to_user(EXData.rData, rData, EXData.rDataCnt))
            {
                 HI_ERR_SPI("ssp readex copy data to user fail!\n");
                 Ret = HI_ERR_SPI_COPY_DATA_ERR;
            }
            HI_KFREE(HI_ID_SPI, pData);
            HI_KFREE(HI_ID_SPI, rData);
            break;
        }

    case  CMD_SPI_SET_ATTR:
        {
            if (copy_from_user(&SSPFForm, argp, sizeof(SPI_FFORM_S)))
            {
                HI_ERR_SPI("ssp setattr copy data from user fail!\n");
                Ret = HI_FAILURE;
                break;
            }
            if(SSPFForm.cscfg)
                HI_DRV_SPI_Set_CsCfg(1);
            else
                HI_DRV_SPI_Set_CsCfg(0);

            HI_DRV_SPI_SetCs_Gpio(SSPFForm.devId);
            HI_DRV_SPI_SetCs_Level(SSPFForm.devId,1);

            Ret = HI_DRV_SPI_SetFrom(SSPFForm.devId, SSPFForm.mode,SSPFForm.spo,SSPFForm.sph,SSPFForm.dss);
            break;
        }
    case  CMD_SPI_GET_ATTR:
        {
            if (copy_from_user(&SSPFForm, argp, sizeof(SPI_FFORM_S)))
            {
                HI_ERR_SPI("ssp getattr copy data from user fail!\n");
                Ret = HI_FAILURE;
                break;
            }
            SSPFForm.cscfg=HI_DRV_SPI_Get_CsCfg();
            Ret=HI_DRV_SPI_GetFrom(SSPFForm.devId,&(SSPFForm.mode),&(SSPFForm.spo),&(SSPFForm.sph),&(SSPFForm.dss));
            if (HI_SUCCESS == Ret)
            {
                if (copy_to_user(argp, &SSPFForm,sizeof(SPI_FFORM_S)))
                {
                    HI_ERR_SPI("ssp getattr copy data to user fail!\n");
                    Ret = HI_ERR_SPI_COPY_DATA_ERR;
                }
            }

            break;
        }
    case CMD_SPI_SET_BLEND:
        {
            if (copy_from_user(&SSPBLEND, argp, sizeof(SPI_BLEND_S)))
            {
                HI_ERR_SPI("ssp set blend copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }
            HI_DRV_SPI_SetBlend(SSPBLEND.devId,SSPBLEND.setbend);
            break;
        }
    case CMD_SPI_GET_BLEND:
        {
            if (copy_from_user(&SSPBLEND, argp, sizeof(SPI_BLEND_S)))
            {
                HI_ERR_SPI("ssp get blend copy data from user fail!\n");
                Ret = HI_ERR_SPI_COPY_DATA_ERR;
                break;
            }
            SSPBLEND.setbend=HI_DRV_SPI_GetBlend(SSPBLEND.devId);
            if (copy_to_user(argp, &SSPBLEND,sizeof(SPI_BLEND_S)))
            {
                    HI_ERR_SPI("ssp get blend copy data to user fail!\n");
                    Ret = HI_ERR_SPI_COPY_DATA_ERR;
            }
            break;
        }
    default:
        {
            return -ENOIOCTLCMD;
        }
    }

    return Ret;
}


EXPORT_SYMBOL(HI_DRV_SPI_Init);
EXPORT_SYMBOL(HI_DRV_SPI_DeInit);
EXPORT_SYMBOL(HI_DRV_SPI_Open);
EXPORT_SYMBOL(HI_DRV_SPI_Close);
EXPORT_SYMBOL(HI_DRV_SPI_SetCs_Gpio);
EXPORT_SYMBOL(HI_DRV_SPI_Get_CsCfg);
EXPORT_SYMBOL(HI_DRV_SPI_Set_CsCfg);
EXPORT_SYMBOL(HI_DRV_SPI_SetCs_Level);
EXPORT_SYMBOL(HI_DRV_SPI_Reset);
EXPORT_SYMBOL(HI_DRV_SPI_CLKEnable);
EXPORT_SYMBOL(HI_DRV_SPI_Enable);
EXPORT_SYMBOL(HI_DRV_SPI_DisEnable);
EXPORT_SYMBOL(HI_DRV_SPI_SetBlend);
EXPORT_SYMBOL(HI_DRV_SPI_ReadEx);
EXPORT_SYMBOL(HI_DRV_SPI_ReadEx_Isr);
EXPORT_SYMBOL(HI_DRV_SPI_WriteIsr);
EXPORT_SYMBOL(HI_DRV_SPI_WriteQuery);
EXPORT_SYMBOL(HI_DRV_SPI_ReadQuery);
EXPORT_SYMBOL(HI_DRV_SPI_ReadIsr);
EXPORT_SYMBOL(HI_DRV_SPI_SetFrom);
EXPORT_SYMBOL(HI_DRV_SPI_SetClk);
EXPORT_SYMBOL(HI_DRV_SPI_SetItr);
EXPORT_SYMBOL(HI_DRV_SPI_ClearItr);

