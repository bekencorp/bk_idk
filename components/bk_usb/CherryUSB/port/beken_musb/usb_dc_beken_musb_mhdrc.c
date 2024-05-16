/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usb_beken_musb_reg.h"
#include "sys_driver.h"
#include <driver/int.h>
#include "bk_misc.h"

#define HWREG(x) \
    (*((volatile uint32_t *)(x)))
#define HWREGH(x) \
    (*((volatile uint16_t *)(x)))
#define HWREGB(x) \
    (*((volatile uint8_t *)(x)))

#ifndef USBD_IRQHandler
#define USBD_IRQHandler USB_Handler //use actual usb irq name instead
#endif

#ifndef USB_BASE
#define USB_BASE (SOC_USB_REG_BASE)
#endif

#define REG_USB_BASE_ADDR              USB_BASE

#define REG_USB_USR_700                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x700)))
#define REG_USB_USR_704                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x704)))
#define REG_USB_USR_708                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x708)))
#define REG_USB_USR_70C                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x70C)))
#define REG_USB_USR_710                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x710)))

/* 00h-0Fh Common USB registers */
#define MUSB_FADDR_OFFSET    0x00
#define MUSB_POWER_OFFSET    0x01
#define MUSB_INTRTX_OFFSET   0x02
#define MUSB_INTRRX_OFFSET   0x04
#define MUSB_INTRTXE_OFFSET  0x06
#define MUSB_INTRRXE_OFFSET  0x08
#define MUSB_INTRUSB_OFFSET  0x0A
#define MUSB_INTRUSBE_OFFSET 0x0B
#define MUSB_FRAME_OFFSET    0x0C
#define MUSB_INDEX_OFFSET    0x0E
#define MUSB_TESTMODE_OFFSET 0x0F

/* 10h-1Fh Host/Peripheral mode */
#define MUSB_IND_TXMAXP_OFFSET   0x10
#define MUSB_IND_TXCSRL_OFFSET   0x12
#define MUSB_IND_TXCSRH_OFFSET   0x13
#define MUSB_IND_RXMAXP_OFFSET   0x14
#define MUSB_IND_RXCSRL_OFFSET   0x16
#define MUSB_IND_RXCSRH_OFFSET   0x17
#define MUSB_IND_RXCOUNT_OFFSET  0x18

/* 20h-5Fh EP0-15 FIFOs */
#define MUSB_FIFO_OFFSET 0x20
#define USB_FIFO_BASE(ep_idx) (USB_BASE + MUSB_FIFO_OFFSET + 0x4 * ep_idx)

/* 60h-7Fh Additional Control & Configuration Registers */
#define MUSB_DEVCTL_OFFSET     0x60
#define MUSB_MISC_OFFSET       0x61
#define MUSB_TXFIFOSZ_OFFSET   0x62
#define MUSB_RXFIFOSZ_OFFSET   0x63
#define MUSB_TXFIFOADD_OFFSET  0x64
#define MUSB_RXFIFOADD_OFFSET  0x66

#define MUSB_LPM_ATTR_OFFSET   0x360
#define MUSB_LPM_CNTRL_OFFSET  0x362
#define MUSB_LPM_INTREN_OFFSET 0x363
#define MUSB_LPM_INTR_OFFSET   0x364
#define MUSB_LPM_FADDR_OFFSET  0x365

#ifndef USB_NUM_BIDIR_ENDPOINTS
#define USB_NUM_BIDIR_ENDPOINTS 16
#endif

typedef enum {
    USB_EP0_STATE_SETUP = 0x0,      /**< SETUP DATA */
    USB_EP0_STATE_IN_DATA = 0x1,    /**< IN DATA */
    USB_EP0_STATE_OUT_DATA = 0x3,   /**< OUT DATA */
    USB_EP0_STATE_IN_STATUS = 0x4,  /**< IN status */
    USB_EP0_STATE_OUT_STATUS = 0x5, /**< OUT status */
    USB_EP0_STATE_IN_ZLP = 0x6,     /**< OUT status */
    USB_EP0_STATE_STALL = 0x7,      /**< STALL status */
} ep0_state_t;

/* Endpoint state */
struct musb_ep_state {
    uint16_t ep_mps;    /* Endpoint max packet size */
    uint8_t ep_type;    /* Endpoint type */
    uint8_t ep_stalled; /* Endpoint stall flag */
    uint8_t ep_enable;  /* Endpoint enable */
    uint8_t *xfer_buf;
    uint32_t xfer_len;
    uint32_t actual_xfer_len;
};

/* Driver state */
struct musb_udc {
    volatile uint8_t dev_addr;
    volatile uint32_t fifo_size_offset;
    __attribute__((aligned(32))) struct usb_setup_packet setup;
    struct musb_ep_state in_ep[USB_NUM_BIDIR_ENDPOINTS];  /*!< IN endpoint parameters*/
    struct musb_ep_state out_ep[USB_NUM_BIDIR_ENDPOINTS]; /*!< OUT endpoint parameters */
} g_musb_udc;

void USBD_IRQHandler(void);

static volatile uint8_t usb_ep0_state = USB_EP0_STATE_SETUP;
volatile bool zlp_flag = 0;

/* get current active ep */
static uint8_t musb_get_active_ep(void)
{
    return HWREGB(USB_BASE + MUSB_INDEX_OFFSET);
}

/* set the active ep */
static void musb_set_active_ep(uint8_t ep_index)
{
    HWREGB(USB_BASE + MUSB_INDEX_OFFSET) = ep_index;
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

static uint32_t musb_get_fifo_size(uint16_t mps, uint16_t *used)
{
    uint32_t size;

    for (uint8_t i = USB_TXFIFOSZ_SIZE_8; i <= USB_TXFIFOSZ_SIZE_2048; i++) {
        size = (8 << i);
        if (mps <= size) {
            *used = size;
            return i;
        }
    }

    *used = 0;
    return USB_TXFIFOSZ_SIZE_8;
}

__WEAK void usb_dc_low_level_init(void)
{
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_120M);

    sys_hal_usb_analog_phy_en(true);

    sys_drv_usb_clock_ctrl(true, NULL);
    sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);

    bk_int_isr_register(INT_SRC_USB, USBD_IRQHandler, NULL);
    bk_int_set_priority(INT_SRC_USB, 2);

    REG_USB_USR_710 |= (0x1<<15);
    REG_USB_USR_710 |= (0x1<<14);
    REG_USB_USR_710 |= (0x1<<16);
    REG_USB_USR_710 |= (0x1<<17);
    REG_USB_USR_710 |= (0x1<<18);
    REG_USB_USR_710 |= (0x1<<19);
    REG_USB_USR_710 &=~(0x1<<20);
    REG_USB_USR_710 |= (0x1<<21);
    REG_USB_USR_710 |= (0x0<< 0);
    REG_USB_USR_710 |= (0x1<< 5);
    REG_USB_USR_710 |= (0x1<< 6);
    REG_USB_USR_710 |= (0x1<< 9);
    REG_USB_USR_710 |= (0x1<<10);
    REG_USB_USR_710 |= (0x1<< 7);

    REG_USB_USR_708 = 0x1;
}

__WEAK void usb_dc_low_level_deinit(void)
{
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);
    bk_int_isr_unregister(INT_SRC_USB);
    sys_hal_usb_analog_phy_en(false);
    sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
    sys_drv_usb_clock_ctrl(false, NULL);
}

int usb_dc_init(void)
{
    usb_dc_low_level_init();

#ifdef CONFIG_USB_HS
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
#else
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_HSENAB;
#endif

    musb_set_active_ep(0);
    HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

    /* Enable USB interrupts */
    HWREGB(USB_BASE + MUSB_INTRUSBE_OFFSET) = (USB_IE_RESET | USB_IE_SUSPND | USB_IE_RESUME);
    HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) = USB_TXIE_EP0;
    HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) = 0;

    /* Enable and support extended LPM transactions */
    HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) = (USB_LPMCNTRL_EN_M | USB_LPMCNTRL_TXLPM | USB_LPMCNTRL_NAK);
    HWREGB(USB_BASE + MUSB_LPM_INTREN_OFFSET) = (USB_LPMIM_ACK | USB_LPMIM_RES);

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_SOFTCONN;

    return 0;
}

int usb_dc_deinit(void)
{
    usb_dc_low_level_deinit();
    return 0;
}

int usbd_set_address(const uint8_t addr)
{
    if (addr == 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;
    }

    g_musb_udc.dev_addr = addr;
    return 0;
}

int usbd_ep_open(const struct usbd_endpoint_cfg *ep_cfg)
{
    uint16_t used = 0;
    uint16_t fifo_size = 0;
    uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);
    uint8_t old_ep_idx;
    uint32_t ui32Flags = 0;
    uint16_t ui32Register = 0;

    if (ep_idx == 0) {
        g_musb_udc.out_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.out_ep[0].ep_type = 0x00;
        g_musb_udc.out_ep[0].ep_enable = true;
        g_musb_udc.in_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.in_ep[0].ep_type = 0x00;
        g_musb_udc.in_ep[0].ep_enable = true;
        return 0;
    }

    if (ep_idx > (USB_NUM_BIDIR_ENDPOINTS - 1)) {
        USB_LOG_ERR("Ep addr %d overflow\r\n", ep_cfg->ep_addr);
        return -1;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep_cfg->ep_addr)) {
        g_musb_udc.out_ep[ep_idx].ep_mps = ep_cfg->ep_mps;
        g_musb_udc.out_ep[ep_idx].ep_type = ep_cfg->ep_type;
        g_musb_udc.out_ep[ep_idx].ep_enable = true;

        HWREGH(USB_BASE + MUSB_IND_RXMAXP_OFFSET) = ep_cfg->ep_mps;

        //
        // Allow auto clearing of RxPktRdy when packet of size max packet
        // has been unloaded from the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_CLEAR) {
            ui32Register = USB_RXCSRH1_AUTOCL;
        }
        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_RXCSRH1_DMAEN | USB_RXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_RXCSRH1_DMAEN;
        }
        //
        // If requested, disable NYET responses for high-speed bulk and
        // interrupt endpoints.
        //
        if (ui32Flags & USB_EP_DIS_NYET) {
            ui32Register |= USB_RXCSRH1_DISNYET;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (ep_cfg->ep_type == 0x01) {
            ui32Register |= USB_RXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY)
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = (USB_RXCSRL1_CLRDT | USB_RXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = USB_RXCSRL1_CLRDT;

        fifo_size = musb_get_fifo_size(ep_cfg->ep_mps, &used);

        HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = fifo_size & 0x0f;
        HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = (g_musb_udc.fifo_size_offset >> 3);

        g_musb_udc.fifo_size_offset += used;
    } else {
        g_musb_udc.in_ep[ep_idx].ep_mps = ep_cfg->ep_mps;
        g_musb_udc.in_ep[ep_idx].ep_type = ep_cfg->ep_type;
        g_musb_udc.in_ep[ep_idx].ep_enable = true;

        HWREGH(USB_BASE + MUSB_IND_TXMAXP_OFFSET) = ep_cfg->ep_mps;

        //
        // Allow auto setting of TxPktRdy when max packet size has been loaded
        // into the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_SET) {
            ui32Register |= USB_TXCSRH1_AUTOSET;
        }

        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_TXCSRH1_DMAEN;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (ep_cfg->ep_type == 0x01) {
            ui32Register |= USB_TXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY)
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_TXCSRL1_CLRDT | USB_TXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_CLRDT;

        fifo_size = musb_get_fifo_size(ep_cfg->ep_mps, &used);

        HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = fifo_size & 0x0f;
        HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = (g_musb_udc.fifo_size_offset >> 3);

        g_musb_udc.fifo_size_offset += used;
    }

    musb_set_active_ep(old_ep_idx);

    return 0;
}

int usbd_ep_close(const uint8_t ep)
{
    return 0;
}

int usbd_ep_set_stall(const uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_STALL;
        }
    } else {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_STALL;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_clear_stall(const uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an OUT endpoint.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_STALL | USB_RXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_CLRDT;
        }
    } else {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an IN endpoint.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~(USB_TXCSRL1_STALL | USB_TXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_CLRDT;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_is_stalled(const uint8_t ep, uint8_t *stalled)
{
    return 0;
}

int usbd_ep_start_write(const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.in_ep[ep_idx].ep_enable) {
        return -2;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY) {
        musb_set_active_ep(old_ep_idx);
        return -3;
    }

    g_musb_udc.in_ep[ep_idx].xfer_buf = (uint8_t *)data;
    g_musb_udc.in_ep[ep_idx].xfer_len = data_len;
    g_musb_udc.in_ep[ep_idx].actual_xfer_len = 0;

    if (data_len == 0) {
        if (ep_idx == 0x00) {
            if (g_musb_udc.setup.wLength == 0) {
                usb_ep0_state = USB_EP0_STATE_IN_STATUS;
            } else {
                usb_ep0_state = USB_EP0_STATE_IN_ZLP;
            }
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
        }
        musb_set_active_ep(old_ep_idx);
        return 0;
    }
    data_len = MIN(data_len, g_musb_udc.in_ep[ep_idx].ep_mps);

    musb_write_packet(ep_idx, (uint8_t *)data, data_len);
    HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) |= (1 << ep_idx);

    if (ep_idx == 0x00) {
        usb_ep0_state = USB_EP0_STATE_IN_DATA;
        if (data_len < g_musb_udc.in_ep[ep_idx].ep_mps) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;
        }
    } else {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_start_read(const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.out_ep[ep_idx].ep_enable) {
        return -2;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    g_musb_udc.out_ep[ep_idx].xfer_buf = data;
    g_musb_udc.out_ep[ep_idx].xfer_len = data_len;
    g_musb_udc.out_ep[ep_idx].actual_xfer_len = 0;

    if (data_len == 0) {
        if (ep_idx == 0) {
            usb_ep0_state = USB_EP0_STATE_SETUP;
        }
        musb_set_active_ep(old_ep_idx);
        return 0;
    }
    if (ep_idx == 0) {
        usb_ep0_state = USB_EP0_STATE_OUT_DATA;
    } else {
        HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) |= (1 << ep_idx);
    }
    musb_set_active_ep(old_ep_idx);
    return 0;
}

void usbd_remote_wakeup_from_L2_state(void)
{
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESUME;
    delay_ms(10);
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_RESUME;
}

void usbd_remote_wakeup_from_L1_state(void)
{
    HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) |= USB_LPMCNTRL_RES;
}

static void handle_ep0(void)
{
    uint8_t ep0_status = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);
    uint16_t read_count;

    /* SentStall */
    if (ep0_status & USB_CSRL0_STALLED) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        usb_ep0_state = USB_EP0_STATE_SETUP;
        return;
    }

    /* SetupEnd */
    if (ep0_status & USB_CSRL0_SETEND) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_SETENDC;
    }

    /* Function Address */
    if (g_musb_udc.dev_addr > 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = g_musb_udc.dev_addr;
        g_musb_udc.dev_addr = 0;
    }

    switch (usb_ep0_state) {
        case USB_EP0_STATE_SETUP:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);
                if (read_count != 8) {
                    return;
                }

                musb_read_packet(0, (uint8_t *)&g_musb_udc.setup, 8);
                if (g_musb_udc.setup.wLength) {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                }

                usbd_event_ep0_setup_complete_handler((uint8_t *)&g_musb_udc.setup);
            }
            break;

        case USB_EP0_STATE_IN_DATA:
            if (g_musb_udc.in_ep[0].xfer_len > g_musb_udc.in_ep[0].ep_mps) {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].ep_mps;
                g_musb_udc.in_ep[0].xfer_len -= g_musb_udc.in_ep[0].ep_mps;
            } else {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].xfer_len;
                g_musb_udc.in_ep[0].xfer_len = 0;
            }

            usbd_event_ep_in_complete_handler(0x80, g_musb_udc.in_ep[0].actual_xfer_len);

            break;
        case USB_EP0_STATE_OUT_DATA:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                musb_read_packet(0, g_musb_udc.out_ep[0].xfer_buf, read_count);
                g_musb_udc.out_ep[0].xfer_buf += read_count;
                g_musb_udc.out_ep[0].actual_xfer_len += read_count;

                if (read_count < g_musb_udc.out_ep[0].ep_mps) {
                    usbd_event_ep_out_complete_handler(0x00, g_musb_udc.out_ep[0].actual_xfer_len);
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                    usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                }
            }
            break;
        case USB_EP0_STATE_IN_STATUS:
        case USB_EP0_STATE_IN_ZLP:
            usb_ep0_state = USB_EP0_STATE_SETUP;
            usbd_event_ep_in_complete_handler(0x80, 0);
            break;
    }
}

void USBD_IRQHandler(void)
{
    uint32_t is;
    uint32_t txis;
    uint32_t rxis;
    uint32_t lpmris;
    uint8_t old_ep_idx;
    uint8_t ep_idx;
    uint16_t write_count, read_count;

    is = HWREGB(USB_BASE + MUSB_INTRUSB_OFFSET);
    txis = HWREGH(USB_BASE + MUSB_INTRTX_OFFSET);
    rxis = HWREGH(USB_BASE + MUSB_INTRRX_OFFSET);
    lpmris = HWREGB(USB_BASE + MUSB_LPM_INTR_OFFSET);
    HWREGB(USB_BASE + MUSB_INTRUSB_OFFSET) = is;
    HWREGB(USB_BASE + MUSB_LPM_INTR_OFFSET) = lpmris;
    old_ep_idx = musb_get_active_ep();

    /* Receive a reset signal from the USB bus */
    if (is & USB_IS_RESET) {
        memset(&g_musb_udc, 0, sizeof(struct musb_udc));
        g_musb_udc.fifo_size_offset = USB_CTRL_EP_MPS;
        usbd_event_reset_handler();
        HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) = USB_TXIE_EP0;
        HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) = 0;

        for (uint8_t i = 1; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
            musb_set_active_ep(i);
            HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = 0;
            HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = 0;
            HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = 0;
            HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = 0;
        }
        usb_ep0_state = USB_EP0_STATE_SETUP;
    }

    if (is & USB_IS_SOF) {
    }

    if (is & USB_IS_RESUME) {
        USB_LOG_INFO("usbd resume int triggered\r\n");
        usbd_event_resume_handler();
    }

    if (is & USB_IS_SUSPEND) {
        USB_LOG_INFO("usbd suspend int triggered\r\n");
        usbd_event_suspend_handler();
    }

    if (lpmris & USB_LPMRIS_ACK) {
        USB_LOG_INFO("usbd enter L1 state\r\n");
        HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) |= USB_LPMCNTRL_NAK;
    }

    if (lpmris & USB_LPMRIS_RES) {
        USB_LOG_INFO("usbd LPM resume int\r\n");
        HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) &= ~USB_LPMCNTRL_NAK;
    }

    txis &= HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET);
    /* Handle EP0 interrupt */
    if (txis & USB_TXIE_EP0) {
        HWREGH(USB_BASE + MUSB_INTRTX_OFFSET) = USB_TXIE_EP0;
        musb_set_active_ep(0);
        handle_ep0();
        txis &= ~USB_TXIE_EP0;
    }

    ep_idx = 1;
    while (txis) {
        if (txis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_INTRTX_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_UNDRN) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_UNDRN;
            }

            if (g_musb_udc.in_ep[ep_idx].xfer_len > g_musb_udc.in_ep[ep_idx].ep_mps) {
                g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].ep_mps;
                g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].ep_mps;
                g_musb_udc.in_ep[ep_idx].xfer_len -= g_musb_udc.in_ep[ep_idx].ep_mps;
            } else {
                g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].xfer_len;
                g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].xfer_len;
                g_musb_udc.in_ep[ep_idx].xfer_len = 0;
            }

            if (g_musb_udc.in_ep[ep_idx].xfer_len == 0) {
                HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) &= ~(1 << ep_idx);
                usbd_event_ep_in_complete_handler(ep_idx | 0x80, g_musb_udc.in_ep[ep_idx].actual_xfer_len);
            } else {
                write_count = MIN(g_musb_udc.in_ep[ep_idx].xfer_len, g_musb_udc.in_ep[ep_idx].ep_mps);

                musb_write_packet(ep_idx, g_musb_udc.in_ep[ep_idx].xfer_buf, write_count);
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
            }

            txis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    rxis &= HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET);
    ep_idx = 1;
    while (rxis) {
        if (rxis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_INTRRX_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                musb_read_packet(ep_idx, g_musb_udc.out_ep[ep_idx].xfer_buf, read_count);
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_RXRDY);

                g_musb_udc.out_ep[ep_idx].xfer_buf += read_count;
                g_musb_udc.out_ep[ep_idx].actual_xfer_len += read_count;
                g_musb_udc.out_ep[ep_idx].xfer_len -= read_count;

                if ((read_count < g_musb_udc.out_ep[ep_idx].ep_mps) || (g_musb_udc.out_ep[ep_idx].xfer_len == 0)) {
                    HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) &= ~(1 << ep_idx);
                    usbd_event_ep_out_complete_handler(ep_idx, g_musb_udc.out_ep[ep_idx].actual_xfer_len);
                } else {
                }
            }

            rxis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    musb_set_active_ep(old_ep_idx);
}
