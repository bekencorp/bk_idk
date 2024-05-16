/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usbh_hub.h"
#include "usb_beken_musb_reg.h"
#include "sys_driver.h"
#include <driver/int_types.h>
#include <driver/hal/hal_int_types.h>
#include <driver/gpio.h>

#define HWREG(x) \
    (*((volatile uint32_t *)(x)))
#define HWREGH(x) \
    (*((volatile uint16_t *)(x)))
#define HWREGB(x) \
    (*((volatile uint8_t *)(x)))

#define HWREGB_WRITE(x, value) \
    (*((volatile uint8_t *)(x))) = value

#define HWREGB_SET_ACTIVE_EP(ep, regx, value) \
{ \
    uint32_t int_level = rtos_disable_int(); \
    uint32_t old_ep = musb_get_active_ep(); \
    musb_set_active_ep(ep); \
    (*((volatile uint8_t *)(regx))) = value; \
    musb_set_active_ep(old_ep); \
	rtos_enable_int(int_level); \
}

#define HWREGB_CLEAR_ACTIVE_EP(ep, regx, value) \
{ \
    uint32_t int_level = rtos_disable_int(); \
    uint32_t old_ep = musb_get_active_ep(); \
    musb_set_active_ep(ep); \
    (*((volatile uint8_t *)(regx))) &= ~value; \
    musb_set_active_ep(old_ep); \
	rtos_enable_int(int_level); \
}

#define HWREGB_OR_ACTIVE_EP(ep, regx, value) \
{ \
    uint32_t int_level = rtos_disable_int(); \
    uint32_t old_ep = musb_get_active_ep(); \
    musb_set_active_ep(ep); \
    (*((volatile uint8_t *)(regx))) |= value; \
    musb_set_active_ep(old_ep); \
	rtos_enable_int(int_level); \
}

//#if CONFIG_USBHOST_PIPE_NUM != 4
//#error musb host ip only supports 4 pipe num
//#endif

#ifndef USBH_IRQHandler
void USBH_IRQHandler(void);//#define USBH_IRQHandler USB_INT_Handler
#endif

#ifndef USB_BASE
#define USB_BASE (SOC_USB_REG_BASE)
#endif

#ifndef USB_PHY_BASE
#define USB_PHY_BASE (SOC_USB_REG_BASE + 0x400)
#endif
#define NANENG_PHY_CFG 1

#if CONFIG_MUSB_MHDRC
#define MUSB_FADDR_OFFSET 0x00
#define MUSB_POWER_OFFSET 0x01
#define MUSB_TXIS_OFFSET  0x02
#define MUSB_RXIS_OFFSET  0x04
#define MUSB_TXIE_OFFSET  0x06
#define MUSB_TXIEH_OFFSET 0x07
#define MUSB_RXIE_OFFSET  0x08
#define MUSB_RXIEH_OFFSET 0x09
#define MUSB_IS_OFFSET    0x0A
#define MUSB_IE_OFFSET    0x0B

#define MUSB_EPIDX_OFFSET 0x0E

#define MUSB_IND_TXMAP_OFFSET      0x10
#define MUSB_IND_TXCSRL_OFFSET     0x12
#define MUSB_IND_TXCSRH_OFFSET     0x13
#define MUSB_IND_RXMAP_OFFSET      0x14
#define MUSB_IND_RXCSRL_OFFSET     0x16
#define MUSB_IND_RXCSRH_OFFSET     0x17
#define MUSB_IND_RXCOUNT_OFFSET    0x18
#define MUSB_IND_TXTYPE_OFFSET     0x1A
#define MUSB_IND_TXINTERVAL_OFFSET 0x1B
#define MUSB_IND_RXTYPE_OFFSET     0x1C
#define MUSB_IND_RXINTERVAL_OFFSET 0x1D

#define MUSB_FIFO_OFFSET 0x20

#define MUSB_DEVCTL_OFFSET 0x60

#define MUSB_TXFIFOSZ_OFFSET  0x62
#define MUSB_RXFIFOSZ_OFFSET  0x63
#define MUSB_TXFIFOADD_OFFSET 0x64
#define MUSB_RXFIFOADD_OFFSET 0x66
#define MUSB_FDRC_FIFO_TOTALSIZE 0x1000  /* 4096 bytes*/

#define MUSB_TXFUNCADDR0_OFFSET 0x80
#define MUSB_TXHUBADDR0_OFFSET  0x82
#define MUSB_TXHUBPORT0_OFFSET  0x83
#define MUSB_TXFUNCADDRx_OFFSET 0x88
#define MUSB_TXHUBADDRx_OFFSET  0x8A
#define MUSB_TXHUBPORTx_OFFSET  0x8B
#define MUSB_RXFUNCADDRx_OFFSET 0x8C
#define MUSB_RXHUBADDRx_OFFSET  0x8E
#define MUSB_RXHUBPORTx_OFFSET  0x8F

#define USB_TXADDR_BASE(ep_idx)    (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx)
#define USB_TXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 2)
#define USB_TXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 3)
#define USB_RXADDR_BASE(ep_idx)    (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 4)
#define USB_RXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 6)
#define USB_RXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 7)

#define MUSB_DMA_INTR_OFFSET  0x200
#define MUSB_DMA_CNTL_BASE(chn_idx) (USB_BASE + MUSB_DMA_INTR_OFFSET + 0x4 + 0x10 * chn_idx)
#define MUSB_DMA_ADDR_BASE(chn_idx) (USB_BASE + MUSB_DMA_INTR_OFFSET + 0x8 + 0x10 * chn_idx)
#define MUSB_DMA_COUNT_BASE(chn_idx) (USB_BASE + MUSB_DMA_INTR_OFFSET + 0xc + 0x10 * chn_idx)


#define NANENG_PHY_FC_REG01 (0x01 * 4)
#define NANENG_PHY_FC_REG02 (0x02 * 4)
#define NANENG_PHY_FC_REG03 (0x03 * 4)
#define NANENG_PHY_FC_REG04 (0x04 * 4)
#define NANENG_PHY_FC_REG05 (0x05 * 4)
#define NANENG_PHY_FC_REG06 (0x06 * 4)
#define NANENG_PHY_FC_REG07 (0x07 * 4)
#define NANENG_PHY_FC_REG08 (0x08 * 4)
#define NANENG_PHY_FC_REG09 (0x09 * 4)
#define NANENG_PHY_FC_REG0A (0x0A * 4)
#define NANENG_PHY_FC_REG0B (0x0B * 4)
#define NANENG_PHY_FC_REG0C (0x0C * 4)
#define NANENG_PHY_FC_REG0D (0x0D * 4)
#define NANENG_PHY_FC_REG0E (0x0E * 4)
#define NANENG_PHY_FC_REG0F (0x0F * 4)
#define NANENG_PHY_FC_REG0F_BYTE 0x0F

#define NANENG_PHY_FC_REG10 (0x10 * 4)
#define NANENG_PHY_FC_REG11 (0x11 * 4)
#define NANENG_PHY_FC_REG12 (0x12 * 4)
#define NANENG_PHY_FC_REG13 (0x13 * 4)
#define NANENG_PHY_FC_REG14 (0x14 * 4)
#define NANENG_PHY_FC_REG15 (0x15 * 4)
#define NANENG_PHY_FC_REG16 (0x16 * 4)
#define NANENG_PHY_FC_REG17 (0x17 * 4)
#define NANENG_PHY_FC_REG18 (0x18 * 4)
#define NANENG_PHY_FC_REG19 (0x19 * 4)
#define NANENG_PHY_FC_REG1A (0x1A * 4)
#define NANENG_PHY_FC_REG1B (0x1B * 4)
#define NANENG_PHY_FC_REG1C (0x1C * 4)
#define NANENG_PHY_FC_REG1D (0x1D * 4)
#define NANENG_PHY_FC_REG1E (0x1E * 4)
#define NANENG_PHY_FC_REG1F (0x1F * 4)

#endif

#if CONFIG_MUSB_FDRC
#define MUSB_FADDR_OFFSET 0x00
#define MUSB_POWER_OFFSET 0x01
#define MUSB_TXIS_OFFSET  0x02
#define MUSB_RXIS_OFFSET  0x04
#define MUSB_TXIE_OFFSET 0x07
#define MUSB_TXIEH_OFFSET 0x08
#define MUSB_RXIE_OFFSET 0x09
#define MUSB_RXIEH_OFFSET 0x0A
#define MUSB_IS_OFFSET    0x06
#define MUSB_IE_OFFSET    0x0B

#define MUSB_EPIDX_OFFSET 0x0E

#define MUSB_IND_TXMAP_OFFSET   0x10
#define MUSB_IND_TXCSRL_OFFSET  0x11
#define MUSB_IND_TXCSRH_OFFSET  0x12
#define MUSB_IND_RXMAP_OFFSET   0x13
#define MUSB_IND_RXCSRL_OFFSET  0x14
#define MUSB_IND_RXCSRH_OFFSET  0x15
#define MUSB_IND_RXCOUNT_OFFSET 0x16
#define MUSB_IND_RXCOUNT2_OFFSET   0x17
#define MUSB_IND_TXTYPE_OFFSET     0x18
#define MUSB_IND_TXINTERVAL_OFFSET 0x19
#define	MUSB_IND_NAKLIMIT0_OFFSET  0x19
#define MUSB_IND_RXTYPE_OFFSET     0x1A
#define MUSB_IND_RXINTERVAL_OFFSET 0x1B

#define MUSB_FIFO_OFFSET 0x20

#define MUSB_DEVCTL_OFFSET 0x0F

#define MUSB_TXRXFIFOSZ_OFFSET  0x1F

#define MUSB_TX_DYNA_CONG_OFFSET 0x1C
#define MUSB_RX_DYNA_CONG_OFFSET 0x1E
/* Dynamic FIFO sizing: */
#define MUSB_IND_FDRC_TXFIFO1_OFFSET 0x1C  /* lower 8-bits of 8-byte-aligned address */
#define MUSB_IND_FDRC_TXFIFO2_OFFSET 0x1D  /* 3-bit size, double-buffer flag, upper 4 address bits */
#define MUSB_IND_FDRC_RXFIFO1_OFFSET 0x1E  /* lower 8-bits of 8-byte-aligned address */
#define MUSB_IND_FDRC_RXFIFO2_OFFSET 0x1F  /* 3-bit size, double-buffer flag, upper 4 address bits */
#define MUSB_FDRC_FIFO_TOTALSIZE 0x800  /* 2048 bytes*/

#endif

#define USB_FIFO_BASE(ep_idx) (USB_BASE + MUSB_FIFO_OFFSET + 0x4 * ep_idx)

#ifndef CONIFG_USB_MUSB_PIPE_NUM
#define CONIFG_USB_MUSB_PIPE_NUM 8
#endif

typedef enum {
    USB_EP0_STATE_SETUP = 0x0, /**< SETUP DATA */
    USB_EP0_STATE_IN_DATA,     /**< IN DATA */
    USB_EP0_STATE_IN_STATUS,   /**< IN status*/
    USB_EP0_STATE_OUT_DATA,    /**< OUT DATA */
    USB_EP0_STATE_OUT_STATUS,  /**< OUT status */
} ep0_state_t;

struct musb_pipe {
    uint8_t dev_addr;
    uint8_t ep_addr;
    uint8_t ep_type;
    uint8_t ep_interval;
    uint8_t speed;
    uint16_t ep_mps;
    uint8_t ep_local_index;
    bool inuse;
    uint32_t xfrd;
    volatile bool waiter;
    usb_osal_sem_t waitsem;
    struct usbh_hubport *hport;
    struct usbh_urb *urb;
    uint32_t iso_frame_idx;
};

struct musb_hcd {
    volatile bool port_csc;
    volatile bool port_pec;
    volatile bool port_pe;
    volatile uint8_t ep_local_index_record;
    volatile uint32_t fifo_size_offset;
    struct musb_pipe pipe_pool[CONFIG_USBHOST_PIPE_NUM][2]; /* Support Bidirectional ep */
} g_musb_hcd;

static volatile uint8_t usb_ep0_state = USB_EP0_STATE_SETUP;

/* set NANENG_PHY_FC_REG0F bit_4 cfg_hsrx_test*/
static void naneng_usb_phy_0f_byte_set(bool param)
{
	if(param) {
		HWREGB(USB_PHY_BASE + NANENG_PHY_FC_REG0F_BYTE) |= (0x1 << 4);
	} else {
		HWREGB(USB_PHY_BASE + NANENG_PHY_FC_REG0F_BYTE) &= ~(0x1 << 4);
	}
}

/* get current active ep */
static uint8_t musb_get_active_ep(void)
{
    return HWREGB(USB_BASE + MUSB_EPIDX_OFFSET);
}

/* set the active ep */
static void musb_set_active_ep(uint8_t ep_index)
{
    HWREGB(USB_BASE + MUSB_EPIDX_OFFSET) = ep_index;
}

static void musb_fifo_flush(uint8_t ep)
{
    uint8_t ep_idx = ep & 0x7f;
    if (ep_idx == 0) {
        if ((HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & (USB_CSRL0_RXRDY | USB_CSRL0_TXRDY)) != 0)
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_CSRH0_FLUSH;
    } else {
        if (ep & 0x80) {
            if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY)
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_FLUSH;
        } else {
            if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY)
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_FLUSH;
        }
    }
}

static void musb_write_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;

    if ((uint32_t)buffer & 0x03) {
        buf8 = buffer;
        for (i = 0; i < len; i++) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    } else {
        count32 = len >> 2;
        count8 = len & 0x03;

        buf32 = (uint32_t *)buffer;

        while (count32--) {
            HWREG(USB_FIFO_BASE(ep_idx)) = *buf32++;
        }

        buf8 = (uint8_t *)buf32;

        while (count8--) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    }
}

static void musb_read_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;
    uint8_t chn_idx = (0xFF & ep_idx) - 1;

    if(len > 1024) {
        HWREGB(USB_BASE + MUSB_DMA_INTR_OFFSET) = (0xFF & ep_idx);
        HWREGB(MUSB_DMA_ADDR_BASE(chn_idx)) = *buffer;
        HWREGB(MUSB_DMA_COUNT_BASE(chn_idx)) = len * 10;
        HWREGB(MUSB_DMA_CNTL_BASE(chn_idx)) = (USB_DMACTL_ENABLE | USB_DMACTL_IE | ((ep_idx << 4) & USB_DMACTL_EP_M));
    } else {
        if ((uint32_t)buffer & 0x03) {
            buf8 = buffer;
            for (i = 0; i < len; i++) {
                *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
            }
        } else {
            count32 = len >> 2;
            count8 = len & 0x03;
    
            buf32 = (uint32_t *)buffer;
    
            while (count32--) {
                *buf32++ = HWREG(USB_FIFO_BASE(ep_idx));
            }
    
            buf8 = (uint8_t *)buf32;
    
            while (count8--) {
                *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
            }
        }
    }
}

void musb_control_pipe_init(struct musb_pipe *pipe, struct usb_setup_packet *setup, uint8_t *buffer, uint32_t buflen)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    //uint8_t old_ep_index;

    //old_ep_index = musb_get_active_ep();
    //musb_set_active_ep(0);

#if CONFIG_MUSB_MHDRC
    HWREGB(USB_TXADDR_BASE(0)) = pipe->dev_addr;
    //HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = pipe->speed;
    HWREGB_SET_ACTIVE_EP(0, (USB_BASE + MUSB_IND_TXTYPE_OFFSET), pipe->speed);
    HWREGB(USB_TXHUBADDR_BASE(0)) = 0;
    HWREGB(USB_TXHUBPORT_BASE(0)) = 0;
#endif

    musb_write_packet(0, (uint8_t *)setup, 8);
    //HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY | USB_CSRL0_SETUP;
    HWREGB_SET_ACTIVE_EP(0, (USB_BASE + MUSB_IND_TXCSRL_OFFSET), USB_CSRL0_TXRDY | USB_CSRL0_SETUP);
    //musb_set_active_ep(old_ep_index);

	GLOBAL_INT_RESTORE();

    USB_LOG_DBG("[-]%s\r\n", __func__);

}

void musb_bulk_pipe_init(struct musb_pipe *pipe, uint8_t *buffer, uint32_t buflen)
{
    //uint8_t ep_idx;
    //uint8_t old_ep_index;

    //ep_idx = pipe->ep_addr & 0x7f;
    //old_ep_index = musb_get_active_ep();
    //musb_set_active_ep(ep_idx);
#if CONFIG_MUSB_MHDRC
	/* select hw ep */
	//musb_set_active_ep(pipe->ep_local_index);
#endif

    if (pipe->ep_addr & 0x80) {
#if CONFIG_MUSB_MHDRC
#if 0
        HWREGB(USB_RXADDR_BASE(pipe->ep_local_index)) = pipe->dev_addr;
        HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | pipe->speed | USB_RXTYPE1_PROTO_BULK;
        HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = 0;
        HWREGB(USB_RXHUBADDR_BASE(pipe->ep_local_index)) = 0;
        HWREGB(USB_RXHUBPORT_BASE(pipe->ep_local_index)) = 0;
#endif
        //HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) &= ~USB_TXCSRH1_MODE;
        HWREGB_CLEAR_ACTIVE_EP(pipe->ep_local_index, (USB_BASE + MUSB_IND_TXCSRH_OFFSET), USB_TXCSRH1_MODE);
        //HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_REQPKT;
        HWREGB_OR_ACTIVE_EP(pipe->ep_local_index, (USB_BASE + MUSB_IND_RXCSRL_OFFSET), USB_RXCSRL1_REQPKT);
#endif
    } else {
#if CONFIG_MUSB_MHDRC
#if 0
        HWREGB(USB_TXADDR_BASE(pipe->ep_local_index)) = pipe->dev_addr;
        HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | pipe->speed | USB_TXTYPE1_PROTO_BULK;
        HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0;
        HWREGB(USB_TXHUBADDR_BASE(pipe->ep_local_index)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(pipe->ep_local_index)) = 0;
#endif
#endif
        if (buflen > pipe->ep_mps) {
            buflen = pipe->ep_mps;
        }

        musb_write_packet(pipe->ep_local_index, buffer, buflen);
        //HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) &= ~USB_TXCSRH1_MODE;
        //HWREGB_CLEAR_ACTIVE_EP(pipe->ep_local_index, (USB_BASE + MUSB_IND_TXCSRH_OFFSET), USB_TXCSRH1_MODE);
        //HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) |= USB_TXCSRH1_MODE;
        HWREGB_OR_ACTIVE_EP(pipe->ep_local_index, (USB_BASE + MUSB_IND_TXCSRH_OFFSET), USB_TXCSRH1_MODE);
        //HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
        HWREGB_SET_ACTIVE_EP(pipe->ep_local_index, (USB_BASE + MUSB_IND_TXCSRL_OFFSET), USB_TXCSRL1_TXRDY);
    }
    //musb_set_active_ep(old_ep_index);
}

void musb_intr_pipe_init(struct musb_pipe *pipe, uint8_t *buffer, uint32_t buflen)
{
    uint8_t ep_idx;
    uint8_t old_ep_index;
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    ep_idx = pipe->ep_addr & 0x7f;
    old_ep_index = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (pipe->ep_addr & 0x80) {
#if CONFIG_MUSB_MHDRC
        HWREGB(USB_RXADDR_BASE(ep_idx)) = pipe->dev_addr;
        HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | pipe->speed | USB_RXTYPE1_PROTO_INT;
        HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = pipe->ep_interval;
        HWREGB(USB_RXHUBADDR_BASE(ep_idx)) = 0;
        HWREGB(USB_RXHUBPORT_BASE(ep_idx)) = 0;
        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = USB_RXCSRL1_REQPKT;
#endif
    } else {
#if CONFIG_MUSB_MHDRC
        HWREGB(USB_TXADDR_BASE(ep_idx)) = pipe->dev_addr;
        HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | pipe->speed | USB_TXTYPE1_PROTO_INT;
        HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = pipe->ep_interval;
        HWREGB(USB_TXHUBADDR_BASE(ep_idx)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(ep_idx)) = 0;
#endif
        if (buflen > pipe->ep_mps) {
            buflen = pipe->ep_mps;
        }

        musb_write_packet(ep_idx, buffer, buflen);
        //HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) |= USB_TXCSRH1_MODE;
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
    }
    musb_set_active_ep(old_ep_index);

    GLOBAL_INT_RESTORE();
}

void musb_isoc_pipe_init(struct musb_pipe *pipe, uint8_t *buffer, uint32_t buflen)
{
    uint8_t old_ep_index;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

#if CONFIG_MUSB_FDRC
    uint8_t ep_idx; = pipe->ep_addr & 0x7f;
#endif
    old_ep_index = musb_get_active_ep();
    USB_LOG_DBG("[+]%s\r\n", __func__);

#if CONFIG_MUSB_MHDRC
    /* select hw ep */
    //musb_set_active_ep(ep_idx);
	musb_set_active_ep(pipe->ep_local_index);

#endif
#if CONFIG_MUSB_FDRC
    uint8_t bCsr1, bCsr2, bIntrRxE1, bIntrRxE2;
    musb_set_active_ep(pipe->ep_local_index);
#endif
    if (pipe->ep_addr & 0x80) {
#if CONFIG_MUSB_MHDRC /* check ep*/
#if NANENG_PHY_CFG
        naneng_usb_phy_0f_byte_set(1);
#endif
        HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = USB_RXCSRL1_REQPKT;// | USB_RXCSRL1_CLRDT;
#endif
#if CONFIG_MUSB_FDRC
        /* read RxCSRs for preparation */
        bCsr1 = HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET);
        bCsr2 = HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET);
            /* prepare for the non-DMA case */
        bCsr2 &= ~(USB_RXCSRH2_DMAEN | USB_RXCSRH2_DMAMOD);
            /* is host */
        bCsr1 |= USB_RXCSRL1_REQPKT;
        bCsr1 &= ~USB_RXCSRL1_RXRDY;
        /* write protocol/endpoint */
        HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | pipe->speed | USB_RXTYPE1_PROTO_ISOC;
        HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = pipe->ep_interval;
        /* disable endpoint interrupt */
        bIntrRxE1 = HWREGB(USB_BASE + MUSB_RXIE_OFFSET);
        bIntrRxE2 = HWREGB(USB_BASE + MUSB_RXIEH_OFFSET);
        if (pipe->ep_local_index < 8)
        {
            bIntrRxE1 |= (1 << pipe->ep_local_index);
            HWREGB(USB_BASE + MUSB_RXIE_OFFSET) = bIntrRxE1 & ~(1 << pipe->ep_local_index);
        }
        else
        {
            bIntrRxE2 |= (1 << (pipe->ep_local_index - 8));
            HWREGB(USB_BASE + MUSB_RXIEH_OFFSET) = bIntrRxE2 & ~(1 << (pipe->ep_local_index - 8));
        }
        /* re-enable interrupt */
        HWREGB(USB_BASE + MUSB_RXIE_OFFSET) = bIntrRxE1;
        HWREGB(USB_BASE + MUSB_RXIEH_OFFSET) = bIntrRxE2;
        /* write RxCSRs */
        HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = bCsr1;
        HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET) = bCsr2;
#endif
    } else {
#if CONFIG_MUSB_MHDRC
#if NANENG_PHY_CFG
		naneng_usb_phy_0f_byte_set(1);
#endif
        if(buffer) {
            musb_write_packet(pipe->ep_local_index, buffer, buflen);
		}
		HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_TXRDY;
#if 0
        HWREGB(USB_TXADDR_BASE(pipe->ep_local_index)) = pipe->dev_addr;
        /* host mode; program protocol/speed and destination */
        HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | pipe->speed | USB_TXTYPE1_PROTO_ISOC;
        //USB_LOG_INFO("[=]%s tx ep_interval:%d\r\n", __func__, pipe->ep_interval);
        HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = pipe->ep_interval;
        HWREGB(USB_TXHUBADDR_BASE(pipe->ep_local_index)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(pipe->ep_local_index)) = 0;
        musb_write_packet(pipe->ep_local_index, buffer, buflen);
        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;

        uint8_t bCsr1, bCsr2, bIntrTxE1, bIntrTxE2;
        /* read RxCSRs for preparation */
        bCsr1 = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);
        bCsr1 &= ~USB_TXCSRL1_UNDRN;
        bCsr2 = HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET);
        bIntrTxE1 = HWREGB(USB_BASE + MUSB_TXIE_OFFSET);
        bIntrTxE2 = HWREGB(USB_BASE + MUSB_TXIEH_OFFSET);
        if (pipe->ep_local_index < 8)
        {
            bIntrTxE1 |= (1 << pipe->ep_local_index);
            HWREGB(USB_BASE + MUSB_TXIE_OFFSET) = bIntrTxE1 & ~(1 << pipe->ep_local_index);
        }
        else
        {
            bIntrTxE2 |= (1 << (pipe->ep_local_index - 8));
            HWREGB(USB_BASE + MUSB_TXIEH_OFFSET) = bIntrTxE2 & ~(1 << (pipe->ep_local_index - 8));
        }
        //USB_LOG_INFO("[=]%s buflen:%d ep_mps:%d\r\n", __func__, buflen, pipe->ep_mps);
        if (buflen > pipe->ep_mps) {
            buflen = pipe->ep_mps;
        }



        HWREGB(USB_BASE + MUSB_TXIE_OFFSET) = bIntrTxE1;
        HWREGB(USB_BASE + MUSB_TXIEH_OFFSET) = bIntrTxE2;
        /* prepare CSR */
        bCsr1 |= USB_TXCSRL1_TXRDY;
        bCsr2 &= ~(USB_TXCSRH1_ISO | USB_TXCSRH1_DMAEN);
        bCsr2 |= USB_TXCSRH1_MODE;
        /* write CSRs */
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = bCsr1;
        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) = bCsr2;
#endif
#endif
#if CONFIG_MUSB_FDRC
        /* host mode; write protocol/endpoint */
        HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | pipe->speed | USB_TXTYPE1_PROTO_ISOC;
        HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = pipe->ep_interval;
        /* read RxCSRs for preparation */
        bCsr1 = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);
        bCsr1 &= ~USB_TXCSRL1_UNDRN;
        bCsr2 = HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET);

        /* load FIFO */
        if (buflen > pipe->ep_mps) {
            buflen = pipe->ep_mps;
        }
        musb_write_packet(ep_idx, buffer, buflen);

        /* prepare CSR */
        bCsr1 |= USB_TXCSRL1_TXRDY;
        bCsr2 &= ~(USB_TXCSRH1_ISO | USB_TXCSRH1_DMAEN);
        bCsr2 |= USB_TXCSRH1_MODE;
        /* write CSRs */
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = bCsr1;
        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) = bCsr2;
#endif



    }
    musb_set_active_ep(old_ep_index);
	GLOBAL_INT_RESTORE();
    USB_LOG_DBG("[-]%s\r\n", __func__);

}

static int usbh_reset_port(const uint8_t port)
{
    g_musb_hcd.port_pe = 0;
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESET;
    usb_osal_msleep(20);
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~(USB_POWER_RESET);
    usb_osal_msleep(20);
    g_musb_hcd.port_pe = 1;
    return 0;
}

static uint8_t usbh_get_port_speed(const uint8_t port)
{
    uint8_t speed = USB_SPEED_FULL;

#if CONFIG_MUSB_MHDRC
    if (HWREGB(USB_BASE + MUSB_POWER_OFFSET) & USB_POWER_HSMODE)
        speed = USB_SPEED_HIGH;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_FSDEV)
        speed = USB_SPEED_FULL;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_LSDEV)
        speed = USB_SPEED_LOW;
#endif

#if CONFIG_MUSB_FDRC
    if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_FSDEV)
        speed = USB_SPEED_FULL;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_LSDEV)
        speed = USB_SPEED_LOW;
#endif
    return speed;
}

__WEAK void usb_hc_low_level_init(void)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);
#if NANENG_PHY_CFG
    //NANENG_PHY_CFG_HSRX_TEST
	HWREGB(USB_PHY_BASE + NANENG_PHY_FC_REG0F) |= (0x1 << 4);
	//disconnect value 640mv
	HWREGB(USB_PHY_BASE + NANENG_PHY_FC_REG0B) = 0x7C;
#endif
    bk_int_isr_register(INT_SRC_USB, USBH_IRQHandler, NULL);
    sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
    USB_LOG_DBG("[-]%s\r\n", __func__);
}

int usb_hc_mhdrc_register_init(void)
{
#if CONFIG_MUSB_MHDRC
	uint32_t fifo_offset = 0;
	USB_LOG_DBG("[=]%s CONFIG_MUSB_MHDRC USB_BASE:0x%x\r\n", __func__, USB_BASE);
	musb_set_active_ep(0);
	HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0;
	HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_64;
	HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = 0;
	HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_64;
	HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = 0;
	fifo_offset += 128;
	g_musb_hcd.fifo_size_offset = fifo_offset;
	
	for (uint8_t i = 1; i < CONIFG_USB_MUSB_PIPE_NUM; i++) {
		musb_set_active_ep(i);
		HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_512;
		HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = fifo_offset;
		HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_512;
		HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = fifo_offset;
		fifo_offset += 512;
	}

    /* Enable USB interrupts */
    uint8_t regval = USB_IE_RESET | USB_IE_CONN | USB_IE_DISCON |
             USB_IE_RESUME | USB_IE_SUSPND |
             USB_IE_BABBLE | USB_IE_SESREQ | USB_IE_VBUSERR;

    HWREGB(USB_BASE + MUSB_IE_OFFSET) = regval;
    HWREGB(USB_BASE + MUSB_TXIE_OFFSET) = regval;//USB_TXIE_EP0;
    HWREGB(USB_BASE + MUSB_TXIEH_OFFSET) = regval;
    HWREGB(USB_BASE + MUSB_RXIE_OFFSET) = regval;//0;
    HWREGB(USB_BASE + MUSB_RXIEH_OFFSET) = regval;

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

    musb_set_active_ep(0);

    USB_LOG_DBG("[-]%s\r\n", __func__);
#endif
    return 0;


}

int usb_hc_init(void)
{
    uint8_t regval;

    USB_LOG_DBG("[+]%s\r\n", __func__);
    memset(&g_musb_hcd, 0, sizeof(struct musb_hcd));

    for (uint8_t i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        g_musb_hcd.pipe_pool[i][0].waitsem = usb_osal_sem_create(1);
        g_musb_hcd.pipe_pool[i][1].waitsem = usb_osal_sem_create(1);
    }

    usb_hc_low_level_init();

#if CONFIG_MUSB_MHDRC
    uint32_t fifo_offset = 0;
	USB_LOG_DBG("[=]%s CONFIG_MUSB_MHDRC USB_BASE:0x%x\r\n", __func__, USB_BASE);
    musb_set_active_ep(0);
    HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0;
    HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_64;
    HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = 0;
    HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_64;
    HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = 0;
    fifo_offset += 128;
    g_musb_hcd.fifo_size_offset = fifo_offset;

    for (uint8_t i = 1; i < CONIFG_USB_MUSB_PIPE_NUM; i++) {
        musb_set_active_ep(i);
        HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_512;
        HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = fifo_offset;
        HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = USB_TXFIFOSZ_SIZE_512;
        HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = fifo_offset;
        fifo_offset += 512;
    }
#endif

#if CONFIG_MUSB_FDRC
		uint32_t fifo_offset = 0;
	
		musb_set_active_ep(0);
		HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0;
        HWREGH(USB_BASE + MUSB_IND_FDRC_TXFIFO1_OFFSET) = (USB_TXFIFOSZ_SIZE_64 << 13) + (fifo_offset >> 3);
        HWREGH(USB_BASE + MUSB_IND_FDRC_RXFIFO1_OFFSET) = (USB_TXFIFOSZ_SIZE_64 << 13) + ((fifo_offset + 64) >> 3);
		HWREGB(USB_BASE + MUSB_IND_TXMAP_OFFSET) = USB_TXFIFOSZ_SIZE_64;
		HWREGB(USB_BASE + MUSB_IND_RXMAP_OFFSET) = USB_TXFIFOSZ_SIZE_64;
		fifo_offset += 128;
		g_musb_hcd.fifo_size_offset = fifo_offset;

		for (uint8_t i = 1; i < CONIFG_USB_MUSB_PIPE_NUM; i++) {
			musb_set_active_ep(i);
	        HWREGH(USB_BASE + MUSB_IND_FDRC_TXFIFO1_OFFSET) = (USB_TXFIFOSZ_SIZE_128 << 13) + (fifo_offset >> 3);
	        HWREGH(USB_BASE + MUSB_IND_FDRC_RXFIFO1_OFFSET) = (USB_TXFIFOSZ_SIZE_128 << 13) + ((fifo_offset + 128) >> 3);
			HWREGB(USB_BASE + MUSB_IND_TXMAP_OFFSET) = USB_TXFIFOSZ_SIZE_128;
			HWREGB(USB_BASE + MUSB_IND_RXMAP_OFFSET) = USB_TXFIFOSZ_SIZE_128;
			fifo_offset += 256;
		}
#endif

    /* Enable USB interrupts */
    regval = USB_IE_RESET | USB_IE_CONN | USB_IE_DISCON |
             USB_IE_RESUME | USB_IE_SUSPND |
             USB_IE_BABBLE | USB_IE_SESREQ | USB_IE_VBUSERR;

    HWREGB(USB_BASE + MUSB_IE_OFFSET) = regval;
    HWREGB(USB_BASE + MUSB_TXIE_OFFSET) = regval;//USB_TXIE_EP0;
    HWREGB(USB_BASE + MUSB_TXIEH_OFFSET) = regval;
    HWREGB(USB_BASE + MUSB_RXIE_OFFSET) = regval;//0;
    HWREGB(USB_BASE + MUSB_RXIEH_OFFSET) = regval;

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

#ifdef CONFIG_USB_MUSB_SUNXI
    musb_set_active_ep(0);
    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;
#endif
    musb_set_active_ep(0);

    USB_LOG_DBG("[-]%s\r\n", __func__);
    return 0;
}

__WEAK void usb_hc_low_level_deinit(void)
{
    sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
    bk_int_isr_unregister(INT_SRC_USB);
}

int usb_hc_deinit(void)
{
    for (uint8_t i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        usb_osal_sem_delete(g_musb_hcd.pipe_pool[i][0].waitsem);
        usb_osal_sem_delete(g_musb_hcd.pipe_pool[i][1].waitsem);
    }
    usb_hc_low_level_deinit();
    return 0;
}

int usbh_roothub_control(struct usb_setup_packet *setup, uint8_t *buf)
{
    //__IO uint32_t hprt0;
    uint8_t nports;
    uint8_t port;
    uint32_t status;

    nports = CONFIG_USBHOST_MAX_RHPORTS;
    port = setup->wIndex;
    if (setup->bmRequestType & USB_REQUEST_RECIPIENT_DEVICE) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -EPIPE;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -EPIPE;
                }
                break;
            case HUB_REQUEST_GET_DESCRIPTOR:
                break;
            case HUB_REQUEST_GET_STATUS:
                memset(buf, 0, 4);
                break;
            default:
                break;
        }
    } else if (setup->bmRequestType & USB_REQUEST_RECIPIENT_OTHER) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                if (!port || port > nports) {
                    return -EPIPE;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_ENABLE:
                        break;
                    case HUB_PORT_FEATURE_SUSPEND:
                    case HUB_PORT_FEATURE_C_SUSPEND:
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        break;
                    case HUB_PORT_FEATURE_C_CONNECTION:
                        g_musb_hcd.port_csc = 0;
                        break;
                    case HUB_PORT_FEATURE_C_ENABLE:
                        g_musb_hcd.port_pec = 0;
                        break;
                    case HUB_PORT_FEATURE_C_OVER_CURREN:
                        break;
                    case HUB_PORT_FEATURE_C_RESET:
                        break;
                    default:
                        return -EPIPE;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                if (!port || port > nports) {
                    return -EPIPE;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_SUSPEND:
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        break;
                    case HUB_PORT_FEATURE_RESET:
                        usbh_reset_port(port);
                        break;

                    default:
                        return -EPIPE;
                }
                break;
            case HUB_REQUEST_GET_STATUS:
                if (!port || port > nports) {
                    return -EPIPE;
                }

                status = 0;
                if (g_musb_hcd.port_csc) {
                    status |= (1 << HUB_PORT_FEATURE_C_CONNECTION);
                }
                if (g_musb_hcd.port_pec) {
                    status |= (1 << HUB_PORT_FEATURE_C_ENABLE);
                }

                if (g_musb_hcd.port_pe) {
                    status |= (1 << HUB_PORT_FEATURE_CONNECTION);
                    status |= (1 << HUB_PORT_FEATURE_ENABLE);
                    if (usbh_get_port_speed(port) == USB_SPEED_LOW) {
                        status |= (1 << HUB_PORT_FEATURE_LOWSPEED);
                    } else if (usbh_get_port_speed(port) == USB_SPEED_HIGH) {
                        status |= (1 << HUB_PORT_FEATURE_HIGHSPEED);
                    }
                }

                memcpy(buf, &status, 4);
                break;
            default:
                break;
        }
    }
    return 0;
}

int usbh_ep0_pipe_reconfigure(usbh_pipe_t pipe, uint8_t dev_addr, uint8_t ep_mps, uint8_t speed)
{
    struct musb_pipe *ppipe = (struct musb_pipe *)pipe;

    ppipe->dev_addr = dev_addr;
    ppipe->ep_mps = ep_mps;

    if (speed == USB_SPEED_HIGH) {
        ppipe->speed = USB_TYPE0_SPEED_HIGH;
    } else if (speed == USB_SPEED_FULL) {
        ppipe->speed = USB_TYPE0_SPEED_FULL;
    } else if (speed == USB_SPEED_LOW) {
        ppipe->speed = USB_TYPE0_SPEED_LOW;
    }
    /* Function Address */
    if (ppipe->dev_addr > 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = ppipe->dev_addr;
    }

    return 0;
}

int usbh_pipe_alloc(usbh_pipe_t *pipe, const struct usbh_endpoint_cfg *ep_cfg)
{
	USB_LOG_DBG("[+]%s\r\n", __func__);

    struct musb_pipe *ppipe;
	struct musb_pipe mpipe;
    uint8_t old_ep_index;
    uint8_t ep_idx;
    usb_osal_sem_t waitsem = NULL;

    ep_idx = ep_cfg->ep_addr & 0x7f;

    if (ep_idx > CONIFG_USB_MUSB_PIPE_NUM) {
        return -ENOMEM;
    }

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    old_ep_index = musb_get_active_ep();

    musb_set_active_ep(ep_idx);

#if 0
    if (ep_cfg->ep_addr & 0x80) {
        ppipe = &g_musb_hcd.pipe_pool[9][1];
    } else {
        ppipe = &g_musb_hcd.pipe_pool[9][0];
    }

    /* store variables */
    waitsem = ppipe->waitsem;
#endif
	ppipe = &mpipe;

    memset(ppipe, 0, sizeof(struct musb_pipe));

    ppipe->ep_addr = ep_cfg->ep_addr;
    ppipe->ep_type = ep_cfg->ep_type;
    ppipe->ep_mps = ep_cfg->ep_mps;
    ppipe->ep_interval = ep_cfg->ep_interval;
    ppipe->speed = ep_cfg->hport->speed;
    ppipe->dev_addr = ep_cfg->hport->dev_addr;
    ppipe->hport = ep_cfg->hport;

    if (ep_cfg->ep_type == USB_ENDPOINT_TYPE_CONTROL) {
        ppipe->ep_local_index = 0;
        if (ep_cfg->ep_addr & 0x80) {
            waitsem = g_musb_hcd.pipe_pool[0][1].waitsem;
            memcpy(&g_musb_hcd.pipe_pool[0][1], ppipe, sizeof(struct musb_pipe));
            ppipe = &g_musb_hcd.pipe_pool[0][1];
        } else {
            waitsem = g_musb_hcd.pipe_pool[0][0].waitsem;
            memcpy(&g_musb_hcd.pipe_pool[0][0], ppipe, sizeof(struct musb_pipe));
            ppipe = &g_musb_hcd.pipe_pool[0][0];
        }

        g_musb_hcd.ep_local_index_record = 0;
    } else {
        if (ppipe->speed == USB_SPEED_HIGH) {
            ppipe->speed = USB_TXTYPE1_SPEED_HIGH;
        } else if (ppipe->speed == USB_SPEED_FULL) {
            ppipe->speed = USB_TXTYPE1_SPEED_FULL;
        } else if (ppipe->speed == USB_SPEED_LOW) {
            ppipe->speed = USB_TXTYPE1_SPEED_LOW;
        }

		{
#if 1//CONFIG_MUSB_FDRC
		if(*pipe) if(((struct musb_pipe *)*pipe)->ep_local_index > 0) {
			GLOBAL_INT_RESTORE(); 
			musb_set_active_ep(old_ep_index);
			return 0;
		}

		uint16_t musb_dynamic_fifo_size[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

		uint32_t dwTotalSize = MUSB_FDRC_FIFO_TOTALSIZE;
		uint32_t dwAvailSize;
		uint32_t dwUsedSize = g_musb_hcd.fifo_size_offset;
		uint16_t wSize;
		uint16_t bVal;
		uint16_t musb_bInterval;
		uint32_t ep_interval;
		uint8_t ep_local_idx = g_musb_hcd.ep_local_index_record + 1;
		dwAvailSize = dwUsedSize + ep_cfg->ep_mps;

		if((dwAvailSize > dwTotalSize) || (ep_local_idx >= CONIFG_USB_MUSB_PIPE_NUM)){
			USB_LOG_ERR("%s Not enough space\r\n", __func__);
			USB_LOG_ERR("%s pipe not enough\r\n", __func__);
		} else {
			wSize = ep_cfg->ep_mps;
			bVal = USB_TXFIFOSZ_SIZE_8;
			while (wSize > 1)
			{
				bVal++;
				wSize >>= 1;
			}
			if (bVal >= 3){
				bVal -= 3;
			} else {
				bVal = 0;
			}
			wSize = musb_dynamic_fifo_size[bVal];
			if(ep_cfg->ep_mps > wSize) {
				bVal += 1;
				wSize = musb_dynamic_fifo_size[bVal];
			}
			g_musb_hcd.ep_local_index_record = ep_local_idx;
			musb_set_active_ep(ep_local_idx);
			ppipe->ep_local_index = ep_local_idx;
			if (ep_cfg->ep_addr & 0x80) {
#if CONFIG_MUSB_FDRC
				HWREGB(USB_BASE + MUSB_IND_FDRC_RXFIFO1_OFFSET) = (((bVal << 13) + (g_musb_hcd.fifo_size_offset >> 3)) & 0x00FF);
				HWREGB(USB_BASE + MUSB_IND_FDRC_RXFIFO2_OFFSET) = ((((bVal << 13) + (g_musb_hcd.fifo_size_offset >> 3)) & 0xFF00) >> 8);
#endif
#if CONFIG_MUSB_MHDRC
				HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = bVal;
				HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = g_musb_hcd.fifo_size_offset;
#endif
				HWREGB(USB_BASE + MUSB_IND_RXMAP_OFFSET) = wSize;
				HWREGB(USB_BASE + MUSB_RXIE_OFFSET) |= ((1 << ep_local_idx) & 0x00FF);
				HWREGB(USB_BASE + MUSB_RXIEH_OFFSET) |= (((1 << ep_local_idx) & 0xFF00) >> 8);
				waitsem = g_musb_hcd.pipe_pool[ep_local_idx][1].waitsem;
				memcpy(&g_musb_hcd.pipe_pool[ep_local_idx][1], ppipe, sizeof(struct musb_pipe));
				ppipe = &g_musb_hcd.pipe_pool[ep_local_idx][1];

				HWREGB(USB_RXADDR_BASE(ppipe->ep_local_index)) = ppipe->dev_addr;
				musb_bInterval = 0;
				ep_interval = ppipe->ep_interval;
				while (ep_interval >= 1)
				{
					musb_bInterval++;
					ep_interval >>= 1;
				}
				switch (ppipe->ep_type) {
					case USB_ENDPOINT_TYPE_BULK:
						HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_BULK;
						if(musb_bInterval <= 2) {
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = 0x0;//register set 2^(m-1), m = 2~16
						} else {
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 2~16
						}
						break;
					case USB_ENDPOINT_TYPE_INTERRUPT:
						HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_INT;
						if(musb_bInterval <= 1) {
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = 0x1;//register set 2^(m-1), m = 1~16
						} else{
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 1~16
						}
						break;
					case USB_ENDPOINT_TYPE_ISOCHRONOUS:
						HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_ISOC;
						if(musb_bInterval <= 1) {
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = 0x1;//register set 2^(m-1), m = 1~16
						} else{
							HWREGB(USB_BASE + MUSB_IND_RXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 1~16
						}
						break;
					default:
						break;
				}

				USB_LOG_INFO("%s RX ppipe:%x index:%d\r\n", __func__,ppipe, ppipe->ep_local_index);
			} else {
#if CONFIG_MUSB_FDRC
				HWREGB(USB_BASE + MUSB_IND_FDRC_TXFIFO1_OFFSET) = (((bVal << 13) + (g_musb_hcd.fifo_size_offset >> 3)) & 0x00FF);
				HWREGB(USB_BASE + MUSB_IND_FDRC_TXFIFO2_OFFSET) = ((((bVal << 13) + (g_musb_hcd.fifo_size_offset >> 3)) & 0xFF00) >> 8);
#endif
#if CONFIG_MUSB_MHDRC
				HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = bVal;
				HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = g_musb_hcd.fifo_size_offset;
#endif
				HWREGB(USB_BASE + MUSB_IND_TXMAP_OFFSET) = wSize;
				HWREGB(USB_BASE + MUSB_TXIE_OFFSET) |= ((1 << ep_local_idx) & 0x00FF);
				HWREGB(USB_BASE + MUSB_TXIEH_OFFSET) |= (((1 << ep_local_idx) & 0xFF00) >> 8);
				waitsem = g_musb_hcd.pipe_pool[ep_local_idx][0].waitsem;
				memcpy(&g_musb_hcd.pipe_pool[ep_local_idx][0], ppipe, sizeof(struct musb_pipe));
				ppipe = &g_musb_hcd.pipe_pool[ep_local_idx][0];

				HWREGB(USB_TXADDR_BASE(ppipe->ep_local_index)) = ppipe->dev_addr;
				musb_bInterval = 0;
				ep_interval = ppipe->ep_interval;
				while (ep_interval >= 1)
				{
					musb_bInterval++;
					ep_interval >>= 1;
				}
				switch (ppipe->ep_type) {
					case USB_ENDPOINT_TYPE_BULK:
						HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_BULK;
						if(musb_bInterval <= 2) {
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0x0;//register set 2^(m-1), m = 2~16
						} else {
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 2~16
						}
						break;
					case USB_ENDPOINT_TYPE_INTERRUPT:
						HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_INT;
						if(musb_bInterval <= 1) {
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0x1;//register set 2^(m-1), m = 1~16
						} else{
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 1~16
						}
						break;
					case USB_ENDPOINT_TYPE_ISOCHRONOUS:
						HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) = ep_idx | ppipe->speed | USB_RXTYPE1_PROTO_ISOC;
						if(musb_bInterval <= 1) {
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = 0x1;//register set 2^(m-1), m = 1~16
						} else{
							HWREGB(USB_BASE + MUSB_IND_TXINTERVAL_OFFSET) = musb_bInterval + 0x1;//register set 2^(m-1), m = 1~16
						}
						break;
					default:
						break;
				}

				HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) |= (USB_TXCSRH1_AUTOSET);
				USB_LOG_INFO("%s TX ppipe:%x index:%d\r\n", __func__,ppipe, ppipe->ep_local_index);
			}
			g_musb_hcd.fifo_size_offset += wSize;
		}
#endif
		}

    }
    /* restore variable */
    ppipe->inuse = true;
    ppipe->waitsem = waitsem;

    musb_set_active_ep(old_ep_index);
    *pipe = (usbh_pipe_t)ppipe;
	GLOBAL_INT_RESTORE();

	USB_LOG_DBG("[-]%s\r\n", __func__);
    return 0;
}

int usbh_pipe_free(usbh_pipe_t pipe)
{
    if(!pipe)
        return -EINVAL;

    struct musb_pipe *ppipe;
    struct usbh_urb *urb;

    ppipe = (struct musb_pipe *)pipe;

    if (!ppipe) {
        return -EINVAL;
    }

    urb = ppipe->urb;

    if (urb) {
        usbh_kill_urb(urb);
    }

    return 0;
}

int usbh_submit_urb(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;
    size_t flags;
    int ret = 0;
    USB_LOG_DBG("[+]%s\r\n", __func__);

    if (!urb) {
        USB_LOG_INFO("[=]%s urb is null\r\n", __func__);
        return -EINVAL;
    }

    pipe = urb->pipe;

    if (!pipe) {
		USB_LOG_INFO("[=]%s pipe is null\r\n", __func__);
        return -EINVAL;
    }

    flags = usb_osal_enter_critical_section();
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    if (!pipe->hport->connected) {
		GLOBAL_INT_RESTORE();
		return -ENODEV;
    }

    if (pipe->urb) {
        GLOBAL_INT_RESTORE();
		return -EBUSY;
    }

    pipe->waiter = false;
    pipe->xfrd = 0;
    pipe->urb = urb;
    urb->errorcode = -EBUSY;
    urb->actual_length = 0;

    if (urb->timeout > 0) {
        pipe->waiter = true;
    }
	GLOBAL_INT_RESTORE();
    usb_osal_leave_critical_section(flags);

    switch (pipe->ep_type) {
        case USB_ENDPOINT_TYPE_CONTROL:
            usb_ep0_state = USB_EP0_STATE_SETUP;
            musb_control_pipe_init(pipe, urb->setup, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        case USB_ENDPOINT_TYPE_BULK:
            musb_bulk_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
            musb_intr_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            pipe->iso_frame_idx = 0;
            musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        default:
            break;
    }
    if (urb->timeout > 0) {
        /* wait until timeout or sem give */
        ret = usb_osal_sem_take(pipe->waitsem, urb->timeout);
        if (ret < 0) {
			//bk_gpio_set_value(7, 2);
			//bk_gpio_set_value(7, 0);
            goto errout_timeout;
        }

        ret = urb->errorcode;
    }
    USB_LOG_DBG("[-]%s ret: %d\r\n", __func__, ret);
    return ret;
errout_timeout:
    pipe->waiter = false;
    usbh_kill_urb(urb);
    return ret;
}

int usbh_kill_urb(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;

    pipe = urb->pipe;
    pipe->urb = NULL;

    if (!urb || !pipe) {
        return -EINVAL;
    }

    if (pipe->waiter) {
        pipe->waiter = false;
        urb->errorcode = -ESHUTDOWN;
        usb_osal_sem_give(pipe->waitsem);
    }

    return 0;
}

static inline void musb_pipe_waitup(struct musb_pipe *pipe)
{
    struct usbh_urb *urb;

    urb = pipe->urb;
    pipe->urb = NULL;

    if (pipe->waiter) {
        pipe->waiter = false;
        if(pipe->waitsem)
            usb_osal_sem_give(pipe->waitsem);
    }

    if (urb->complete) {
        if (urb->errorcode < 0) {
            urb->complete(urb->arg, urb->errorcode);
        } else {
            urb->complete(urb->arg, urb->actual_length);
        }
    }
}

void handle_ep0(void)
{
    uint8_t ep0_status;
    struct musb_pipe *pipe;
    struct usbh_urb *urb;
    uint32_t size;
	uint8_t old_ep_idx;

    pipe = (struct musb_pipe *)&g_musb_hcd.pipe_pool[0][0];
    urb = pipe->urb;
    if (urb == NULL) {
        return;
    }
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	old_ep_idx = musb_get_active_ep();

    musb_set_active_ep(0);
    ep0_status = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);

    if (ep0_status & USB_CSRL0_STALLED) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        usb_ep0_state = USB_EP0_STATE_SETUP;
		GLOBAL_INT_RESTORE();
        urb->errorcode = -EPERM;
        musb_pipe_waitup(pipe);
        musb_set_active_ep(old_ep_idx);
        return;
    }
    if (ep0_status & USB_CSRL0_ERROR) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_ERROR;
        musb_fifo_flush(0);
        usb_ep0_state = USB_EP0_STATE_SETUP;
        GLOBAL_INT_RESTORE();
        urb->errorcode = -EIO;
        musb_pipe_waitup(pipe);
        musb_set_active_ep(old_ep_idx);
        return;
    }
    if (ep0_status & USB_CSRL0_STALL) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALL;
        usb_ep0_state = USB_EP0_STATE_SETUP;
        GLOBAL_INT_RESTORE();
        urb->errorcode = -EPERM;
        musb_pipe_waitup(pipe);
        musb_set_active_ep(old_ep_idx);
        return;
    }

    switch (usb_ep0_state) {
        case USB_EP0_STATE_SETUP:
            urb->actual_length += 8;
            if (urb->transfer_buffer_length) {
                if (urb->setup->bmRequestType & 0x80) {
                    usb_ep0_state = USB_EP0_STATE_IN_DATA;
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_REQPKT;
                } else {
                    usb_ep0_state = USB_EP0_STATE_OUT_DATA;
                    size = urb->transfer_buffer_length;
                    if (size > pipe->ep_mps) {
                        size = pipe->ep_mps;
                    }

                    musb_write_packet(0, urb->transfer_buffer, size);
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;

                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;
                }
            } else {
                usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_REQPKT | USB_CSRL0_STATUS);
            }
            break;
        case USB_EP0_STATE_IN_DATA:
            if (ep0_status & USB_CSRL0_RXRDY) {
                size = urb->transfer_buffer_length;
                if (size > pipe->ep_mps) {
                    size = pipe->ep_mps;
                }

                size = MIN(size, HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET));
                musb_read_packet(0, urb->transfer_buffer, size);
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_RXRDY;
                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;

                if ((size < pipe->ep_mps) || (urb->transfer_buffer_length == 0)) {
                    usb_ep0_state = USB_EP0_STATE_OUT_STATUS;
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_STATUS);
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_REQPKT;
                }
            }
            break;
        case USB_EP0_STATE_OUT_DATA:
            if (urb->transfer_buffer_length > 0) {
                size = urb->transfer_buffer_length;
                if (size > pipe->ep_mps) {
                    size = pipe->ep_mps;
                }

                musb_write_packet(0, urb->transfer_buffer, size);
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;

                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;
            } else {
                usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_REQPKT | USB_CSRL0_STATUS);
            }
            break;
        case USB_EP0_STATE_OUT_STATUS:
            urb->errorcode = 0;
            musb_pipe_waitup(pipe);
            break;
        case USB_EP0_STATE_IN_STATUS:
            if (ep0_status & (USB_CSRL0_RXRDY | USB_CSRL0_STATUS)) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~(USB_CSRL0_RXRDY | USB_CSRL0_STATUS);
                urb->errorcode = 0;
                musb_pipe_waitup(pipe);
            }
            break;
    }
    musb_set_active_ep(old_ep_idx);
    GLOBAL_INT_RESTORE();
}

uint8_t s_stall_count = 0;
uint32_t rx_uvc_ready_packet = 0;
uint32_t stall_error_n = 0;
void USBH_IRQHandler(void)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);
    uint32_t is;
    uint32_t txis;
    uint32_t rxis;
    uint32_t dmais;
    uint8_t ep_csrl_status;
    // uint8_t ep_csrh_status;
    struct musb_pipe *pipe;
    struct usbh_urb *urb;
    uint8_t ep_idx;
    uint8_t old_ep_idx;
    uint32_t size;
    uint8_t *pbuffer;
    uint32_t count;

    is = HWREGB(USB_BASE + MUSB_IS_OFFSET);
    txis = HWREGH(USB_BASE + MUSB_TXIS_OFFSET);
    rxis = HWREGH(USB_BASE + MUSB_RXIS_OFFSET);
    dmais = HWREGB(USB_BASE + MUSB_DMA_INTR_OFFSET);

    HWREGB(USB_BASE + MUSB_IS_OFFSET) = is;

    old_ep_idx = musb_get_active_ep();
    //USB_LOG_INFO("%s is: 0x%x txis: 0x%x rxis:0x%x dmais:0x%x\r\n", __func__, is, txis, rxis, dmais);

    if(dmais) {
		//USB_LOG_INFO("%s is: 0x%x txis: 0x%x rxis:0x%x dmais:0x%x\r\n", __func__, is, txis, rxis, dmais);
        return;
    }

    if (is & USB_IS_CONN) {
        g_musb_hcd.port_csc = 1;
        g_musb_hcd.port_pec = 1;
        g_musb_hcd.port_pe = 1;
        usbh_roothub_thread_wakeup(1);
    }

    if ((is & USB_IS_DISCON) || (is & USB_IS_BABBLE)) {
        g_musb_hcd.port_csc = 1;
        g_musb_hcd.port_pec = 1;
        g_musb_hcd.port_pe = 0;
        g_musb_hcd.ep_local_index_record = 0;
        g_musb_hcd.fifo_size_offset = 128;
        for (uint8_t index = 0; index < CONFIG_USBHOST_PIPE_NUM; index++) {
            for (uint8_t j = 0; j < 2; j++) {
                struct musb_pipe *pipe = &g_musb_hcd.pipe_pool[index][j];
                struct usbh_urb *urb = pipe->urb;
                if (pipe->waiter) {
                    pipe->waiter = false;
                    urb->errorcode = -ESHUTDOWN;
                    usb_osal_sem_give(pipe->waitsem);
                }
            }
        }
        usbh_roothub_thread_wakeup(1);
        musb_set_active_ep(old_ep_idx);
        return;
    } else if (is & USB_IS_CONN) {
        g_musb_hcd.port_csc = 1;
        g_musb_hcd.port_pec = 1;
        g_musb_hcd.port_pe = 1;
        usbh_roothub_thread_wakeup(1);
        musb_set_active_ep(old_ep_idx);
        return;
    }

    if (is & USB_IS_SOF) {
    }

    if (is & USB_IS_RESUME) {
    }

    if (is & USB_IS_SUSPEND) {
    }

    if (is & USB_IS_VBUSERR) {
    }

    if (is & USB_IS_SESREQ) {
    }

    //if (is & USB_IS_BABBLE) {
    //}

    txis &= (HWREGB(USB_BASE + MUSB_TXIE_OFFSET) | HWREGB(USB_BASE + MUSB_TXIEH_OFFSET));
    /* Handle EP0 interrupt */
    if (txis & USB_TXIE_EP0) {
        txis &= ~USB_TXIE_EP0;
        HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = USB_TXIE_EP0;
        handle_ep0();
    }

    for (ep_idx = 1; ep_idx < CONIFG_USB_MUSB_PIPE_NUM; ep_idx++) {
        if (txis & (1 << ep_idx)) {
            HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = (1 << ep_idx);
            pipe = &g_musb_hcd.pipe_pool[ep_idx][0];
            urb = pipe->urb;
			if(urb == NULL)
				continue;
            musb_set_active_ep(ep_idx);

            ep_csrl_status = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);

            if (ep_csrl_status & USB_TXCSRL1_ERROR) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_ERROR;
				USB_LOG_INFO("[=]%s ep_idx:%d USB_TXCSRL1_ERROR\r\n", __func__, ep_idx);
                urb->errorcode = -EIO;
                goto pipe_wait;
            } else if (ep_csrl_status & USB_TXCSRL1_NAKTO) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_NAKTO;
				USB_LOG_INFO("[=]%s ep_idx:%d USB_TXCSRL1_NAKTO\r\n", __func__, ep_idx);
                urb->errorcode = -EBUSY;
                goto pipe_wait;
            } else if (ep_csrl_status & USB_TXCSRL1_STALL) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_STALL;
                urb->errorcode = -EPERM;
                goto pipe_wait;
            } else {
                if(HWREGB(USB_BASE + MUSB_IND_TXTYPE_OFFSET) & USB_TXTYPE1_PROTO_ISOC){
                    uint8_t complete = 0;
                    size = urb->iso_packet[pipe->iso_frame_idx].transfer_buffer_length;
                    pbuffer = urb->iso_packet[pipe->iso_frame_idx].transfer_buffer;
                    pipe->iso_frame_idx++;
                    complete = (pipe->iso_frame_idx >= urb->num_of_iso_packets) ? 1 : 0;

                    if(complete) {
                        USB_LOG_DBG("[=]%s complete ep_idx:%d size:%d ep_mps:%d\r\n", __func__, ep_idx, size, pipe->ep_mps);
                        urb->errorcode = 0;
						if(urb->num_of_iso_packets == 1) {
								if(urb->transfer_buffer_length) {
									musb_pipe_waitup(pipe);
									pipe->iso_frame_idx = 0;
									pipe->urb = urb;
									musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
								}

						} else {
		                        musb_pipe_waitup(pipe);//goto pipe_wait;
						}
                    } else{
                        musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
                    }
                } else {
                    size = urb->transfer_buffer_length;
    
                    if (size > pipe->ep_mps) {
                        size = pipe->ep_mps;
                    }
    
                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;
    
                    if (urb->transfer_buffer_length == 0) {
                        urb->errorcode = 0;
                        goto pipe_wait;
                    } else {
                        musb_write_packet(ep_idx, urb->transfer_buffer, size);
                        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
                    }
                }
            }
        }
    }

    rxis &= (HWREGB(USB_BASE + MUSB_RXIE_OFFSET) | HWREGB(USB_BASE + MUSB_RXIEH_OFFSET));
    for (ep_idx = 1; ep_idx < CONIFG_USB_MUSB_PIPE_NUM; ep_idx++) {
        if (rxis & (1 << ep_idx)) {
#if NANENG_PHY_CFG
			naneng_usb_phy_0f_byte_set(0);
#endif
            HWREGH(USB_BASE + MUSB_RXIS_OFFSET) = (1 << ep_idx); // clear isr flag
            pipe = &g_musb_hcd.pipe_pool[ep_idx][1];
            urb = pipe->urb;
			if(urb == NULL)
				continue;
            musb_set_active_ep(ep_idx);

            ep_csrl_status = HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET);
            //ep_csrh_status = HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET); // todo:for iso transfer

            if (ep_csrl_status & USB_RXCSRL1_ERROR) {
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_ERROR;
				USB_LOG_INFO("[=]%s ep_idx:%d USB_RXCSRL1_ERROR\r\n", __func__, ep_idx);
				HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_RXRDY;
                urb->errorcode = -EIO;
                goto pipe_wait;
            } else if (ep_csrl_status & USB_RXCSRL1_NAKTO) {
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_NAKTO;
				USB_LOG_INFO("[=]%s ep_idx:%d USB_RXCSRL1_NAKTO\r\n", __func__, ep_idx);
                urb->errorcode = -EBUSY;
				musb_pipe_waitup(pipe);
                //goto pipe_wait;
            } else if (ep_csrl_status & USB_RXCSRL1_STALL) {
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_STALL;
                urb->errorcode = -EPERM;
				stall_error_n++;
				if(s_stall_count > 2) {
					USB_LOG_INFO("[=]%s USB_RXCSRL1_STALL\r\n", __func__);
					musb_pipe_waitup(pipe);
					//goto pipe_wait;
				}
				s_stall_count++;
				musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
            } else if (ep_csrl_status & USB_RXCSRL1_RXRDY) {
                if(HWREGB(USB_BASE + MUSB_IND_RXTYPE_OFFSET) & USB_RXTYPE1_PROTO_ISOC){
                    uint8_t complete = 0;
                    count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);
                    urb->iso_packet[pipe->iso_frame_idx].actual_length = count;
                    size = urb->iso_packet[pipe->iso_frame_idx].transfer_buffer_length;
                    pbuffer = urb->iso_packet[pipe->iso_frame_idx].transfer_buffer;
                    if(s_stall_count > 0)
                        urb->iso_packet[pipe->iso_frame_idx].errorcode = 3;
                    else
                        urb->iso_packet[pipe->iso_frame_idx].errorcode = 0;
                    if(count > 0) pipe->iso_frame_idx++;
                    complete = (pipe->iso_frame_idx >= urb->num_of_iso_packets) ? 1 : 0;

                    if (size > pipe->ep_mps) {
                        size = pipe->ep_mps;
                    }
                    size = MIN(size, count);
					if(pbuffer != NULL)
                    	musb_read_packet(ep_idx, pbuffer, size);
					else
						USB_LOG_INFO("[=]USB_RXTYPE1_PROTO_ISOC buffer is NULL\r\n");
					s_stall_count = 0;
					rx_uvc_ready_packet++;
					if(rx_uvc_ready_packet % 10000 == 0) USB_LOG_DBG("[=]%s ready_packet_n:%d stall_error_n:%d \r\n", __func__, rx_uvc_ready_packet, stall_error_n);
                    HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_RXRDY;

                    if(complete) {
						urb->errorcode = 0;
                        USB_LOG_DBG("[=]%s complete ep_idx:%d size:%d ep_mps:%d count:%d\r\n", __func__, ep_idx, size, pipe->ep_mps, count);
						if(urb->num_of_iso_packets == 1) {
								if(urb->transfer_buffer_length) {
									musb_pipe_waitup(pipe);
									pipe->iso_frame_idx = 0;
									pipe->urb = urb;
									musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
								}

								//if(rxis & (~(1 << ep_idx)))
									//USB_LOG_INFO("[=]%s complete rxis:%x ep_idx:%d\r\n", __func__, rxis, ep_idx);
								//break;
						} else {
								musb_pipe_waitup(pipe);
		                        //goto pipe_wait;
						}
                    } else{
                        musb_isoc_pipe_init(pipe, urb->transfer_buffer, urb->transfer_buffer_length);
                    }
                } else {
                    size = urb->transfer_buffer_length;
                    if (size > pipe->ep_mps) {
                        size = pipe->ep_mps;
                    }
                    size = MIN(size, HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET));
    
                    musb_read_packet(ep_idx, urb->transfer_buffer, size);
    
                    HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~USB_RXCSRL1_RXRDY;
    
                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;
					//USB_LOG_INFO("[=]size:%d ep_mps:%d transfer_buffer_length:%d\r\n", size, pipe->ep_mps, urb->transfer_buffer_length);

                    if ((size < pipe->ep_mps) || (urb->transfer_buffer_length == 0)) {
                        urb->errorcode = 0;
                        musb_pipe_waitup(pipe);
                        if(urb->transfer_buffer_length > 0) {
                            pipe->urb = urb;
                            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_REQPKT;
                        } //else { USB_LOG_INFO("[=============]buffer_length < 0\r\n");}
                        //goto pipe_wait;
                    } else {
                        HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_REQPKT;
                    }
                }
            }
        }
    }
    musb_set_active_ep(old_ep_idx);
    USB_LOG_DBG("[-]%s\r\n", __func__);
    return;
pipe_wait:
    musb_set_active_ep(old_ep_idx);
    musb_pipe_waitup(pipe);

}