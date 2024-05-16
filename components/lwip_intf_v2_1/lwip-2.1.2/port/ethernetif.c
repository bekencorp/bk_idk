// Copyright 2020-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "common/bk_include.h"

#if CONFIG_ETH
#include <string.h>
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "lwip/ethip6.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "lan8742.h"
#include "eth_mac.h"
#include "cache.h"
#include <driver/hal/hal_gpio_types.h>
#include "sys_ll.h"
#include "driver/int_types.h"
#include "driver/int.h"
#include "gpio_driver.h"
#include "lwip/netifapi.h"
#include "net.h"
#include "sys_hal.h"

/* Private define ------------------------------------------------------------*/
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE ( 1024 )
/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ETH Setting  */
#define ETH_DMA_TRANSMIT_TIMEOUT               ( 20U )
#define ETH_TX_BUFFER_MAX                      ((ETH_TX_DESC_CNT) * 2U)
#define ETH_RX_BUFFER_SIZE                     ( 1536 )
#define ETH_TX_DESCS_PER_TX                    8

/* ASYNC mode */
#define ETH_TX_CPLT_IN_SEP_TASK                1

/* Private variables ---------------------------------------------------------*/
/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx buffers are allocated statically and passed directly to the LwIP stack
          they will return back to ETH DMA after been processed by the stack.
        - Tx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number must be between ETH_RX_DESC_CNT and 2*ETH_RX_DESC_CNT
  2.b. Rx Buffers must have the same size: ETH_RX_BUFFER_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
  2.c  The RX Ruffers addresses and sizes must be properly defined to be aligned
       to L1-CACHE line size (32 bytes).
*/

/* Data Type Definitions */
typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

/* Memory Pool Declaration */
#define ETH_RX_BUFFER_CNT             24U
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

/* Variable Definitions */
static uint8_t RxAllocStatus;

__IO uint32_t TxPkt = 0;
__IO uint32_t RxPkt = 0;

static ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
static ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];   /* Ethernet Tx DMA Descriptors */
extern u8_t memp_memory_RX_POOL_base[];
extern int32_t xTaskGetTickCount( void );
static beken_semaphore_t RxPktSemaphore = NULL;   /* Semaphore to signal incoming packets */
static beken_semaphore_t TxPktSemaphore = NULL;   /* Semaphore to signal transmit packet complete */
#if ETH_TX_CPLT_IN_SEP_TASK
static beken_semaphore_t SyncSemaphore = NULL;
#endif

/* Global Ethernet handle */
static ETH_HandleTypeDef heth;
static ETH_TxPacketConfig TxConfig;

/* Private function prototypes -----------------------------------------------*/
static int32_t ETH_PHY_IO_Init(void);
static int32_t ETH_PHY_IO_DeInit (void);
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
static int32_t ETH_PHY_IO_GetTick(void);
static void ethernetif_tx_cplt(void *argument);

static lan8742_Object_t LAN8742;
static lan8742_IOCtx_t  LAN8742_IOCtx = {ETH_PHY_IO_Init,
                                  ETH_PHY_IO_DeInit,
                                  ETH_PHY_IO_WriteReg,
                                  ETH_PHY_IO_ReadReg,
                                  ETH_PHY_IO_GetTick};

/* Private functions ---------------------------------------------------------*/
static void pbuf_free_custom(struct pbuf *p);
static void ethernetif_input(void* argument);

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  rtos_set_semaphore(&RxPktSemaphore);
}

/**
  * @brief  Ethernet Tx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  rtos_set_semaphore(&TxPktSemaphore);
}

/**
  * @brief  Ethernet DMA transfer error callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *handlerEth)
{
  if((HAL_ETH_GetDMAError(handlerEth) & ETH_DMACSR_RBU) == ETH_DMACSR_RBU)
  {
    rtos_set_semaphore(&RxPktSemaphore);
  }
}

void ETH_IRQHandler(void)
{
  //LWIP_LOGI("%s\n", __func__);
  HAL_ETH_IRQHandler(&heth);
}

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
 * @brief In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static bk_err_t low_level_init(struct netif *netif)
{
  HAL_StatusTypeDef hal_eth_init_status = HAL_OK;
  uint8_t MACAddr[6] ;
  bk_err_t ret = BK_OK;

  LWIP_LOGI("ETH %s start XXX\n", __func__);
  /* Start ETH HAL Init */
  heth.Instance = ETH;
  bk_get_mac(MACAddr, MAC_TYPE_ETH);
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1536;

  hal_eth_init_status = HAL_ETH_Init(&heth);

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);

#if LWIP_ARP || LWIP_ETHERNET

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] =  heth.Init.MACAddr[0];
  netif->hwaddr[1] =  heth.Init.MACAddr[1];
  netif->hwaddr[2] =  heth.Init.MACAddr[2];
  netif->hwaddr[3] =  heth.Init.MACAddr[3];
  netif->hwaddr[4] =  heth.Init.MACAddr[4];
  netif->hwaddr[5] =  heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* Accept broadcast address and ARP traffic */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  #if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
  #else
    netif->flags |= NETIF_FLAG_BROADCAST;
  #endif /* LWIP_ARP */

  /* create a binary semaphore used for informing ethernetif of frame reception */
  ret = rtos_init_semaphore_ex(&RxPktSemaphore, 1, 0);
  if (ret != BK_OK) {
    LWIP_LOGE("%s init rx sema failed: %d\n", __func__, ret);
    return ret;
  }

  /* create a binary semaphore used for informing ethernetif of frame transmission */
  ret = rtos_init_semaphore_ex(&TxPktSemaphore, 1, 0);
  if (ret != BK_OK) {
    LWIP_LOGE("%s init tx sema failed: %d\n", __func__, ret);
    return ret;
  }

  /* create the task that handles the ETH_MAC */
  ret = rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY, /* TBD: ETH priority */
      "EthIf", ethernetif_input, INTERFACE_THREAD_STACK_SIZE, netif);
  if (ret != BK_OK) {
    LWIP_LOGE("%s create EthIf thread failed: %d\n", __func__, ret);
    return ret;
  }

#if ETH_TX_CPLT_IN_SEP_TASK
  ret = rtos_init_semaphore_ex(&SyncSemaphore, 1, 1);
  if (ret != BK_OK) {
    LWIP_LOGE("%s init sync sema failed: %d\n", __func__, ret);
    return ret;
  }

  /* create the task that handles the ETH_MAC */
  ret = rtos_create_thread(NULL, 4 - 1, /* TBD: higher than iperf */
      "EthIfTxCplt", ethernetif_tx_cplt, INTERFACE_THREAD_STACK_SIZE, netif);
  if (ret != BK_OK) {
    LWIP_LOGE("%s create EthIfTxCplt thread failed: %d\n", __func__, ret);
    return ret;
  }
#endif

  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  LAN8742_Init(&LAN8742);

  if (LAN8742_GetAutoNego(&LAN8742))
  {
    LWIP_LOGI("AutoNego enabled\n");
  }
  else
  {
    LWIP_LOGI("AutoNego disabled\n");
  }

  if (hal_eth_init_status != HAL_OK)
  {
    LWIP_LOGI("%s: Init ETH failed\n", __func__);

    ret = BK_FAIL;
  }

  LWIP_LOGI("ETH %s end, ret %d\n", __func__, ret);

  return ret;
#endif /* LWIP_ARP || LWIP_ETHERNET */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESCS_PER_TX];  // ETH_TX_DESC_CNT

  memset(Txbuffer, 0, ETH_TX_DESCS_PER_TX * sizeof(ETH_BufferTypeDef));

  for (q = p; q != NULL; q = q->next)
  {
    if (i >= ETH_TX_DESCS_PER_TX)
      return ERR_IF;

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if (i > 0)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if (q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  TxConfig.Length = p->tot_len;
  TxConfig.TxBuffer = Txbuffer;
  TxConfig.pData = p;

  pbuf_ref(p);

#if ETH_TX_CPLT_IN_SEP_TASK
    rtos_get_semaphore(&SyncSemaphore, BEKEN_WAIT_FOREVER);
#endif
    if (HAL_ETH_Transmit_IT(&heth, &TxConfig))  // FIXME: may failed, revise me
    {
      LWIP_LOGE("HAL_ETH_Transmit_IT failed\n");
      pbuf_free(p);
      errval = ERR_MEM;
    }
#if ETH_TX_CPLT_IN_SEP_TASK
    rtos_set_semaphore(&SyncSemaphore);
#endif
#if !ETH_TX_CPLT_IN_SEP_TASK
  while(rtos_get_semaphore(&TxPktSemaphore, BEKEN_WAIT_FOREVER) != BK_OK)
  {
  }

  HAL_ETH_ReleaseTxPacket(&heth);
#endif

  return errval;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;

  if(RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void ethernetif_input(void* argument)
{
  struct pbuf *p = NULL;
  struct netif *netif = (struct netif *) argument;

  LWIP_LOGI("ETH receive task created\n");
  for( ;; )
  {
    if (rtos_get_semaphore(&RxPktSemaphore, BEKEN_WAIT_FOREVER) == BK_OK)
    {
      do
      {
        p = low_level_input(netif);
        if (p != NULL)
        {
          if (netif->input(p, netif) != ERR_OK )
          {
            pbuf_free(p);
          }
        }
      } while(p!=NULL);
    }
  }
}

#if ETH_TX_CPLT_IN_SEP_TASK
static void ethernetif_tx_cplt(void *argument)
{

  LWIP_LOGI("ETH tx complete task created\n");
  for( ;; )
  {
    if (rtos_get_semaphore(&TxPktSemaphore, BEKEN_WAIT_FOREVER) == BK_OK)
    {
      rtos_get_semaphore(&SyncSemaphore, BEKEN_WAIT_FOREVER);
      HAL_ETH_ReleaseTxPacket(&heth);
      rtos_set_semaphore(&SyncSemaphore);
    }
  }
}
#endif

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  err_t errval;
  errval = ERR_OK;

  return errval;

}
#endif /* LWIP_ARP */

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  // MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write its own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
static void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
   * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
    rtos_set_semaphore(&RxPktSemaphore);
  }
}

void HAL_ETH_MspInit(ETH_HandleTypeDef* ethHandle)
{
  //LWIP_LOGI("HW DeviceID: 0x%x\n", REG_READ((ETH_BASE + 0x800*4)));
  //LWIP_LOGI("HW VersionID: 0x%x\n", REG_READ((ETH_BASE + 0x801*4)));

  // GPIO PinMUX: group0(27, 29-39), or group2(46-55)
  // FIXME: BK7236, use dts instead of hard coding.
#ifdef CONFIG_ETH_PIN_GROUP0
  gpio_dev_unmap(GPIO_27);  // PHY INT
  gpio_dev_unmap(GPIO_29);  // MDC
  gpio_dev_unmap(GPIO_32);  // MDIO
  gpio_dev_unmap(GPIO_33);  // RXD[0]
  gpio_dev_unmap(GPIO_34);  // RXD[1]
  gpio_dev_unmap(GPIO_35);  // RXDV
  gpio_dev_unmap(GPIO_36);  // TXD[0]
  gpio_dev_unmap(GPIO_37);  // TXD[1]
  gpio_dev_unmap(GPIO_38);  // TXEN
  gpio_dev_unmap(GPIO_39);  // REF_CLK

  gpio_dev_map(GPIO_27, GPIO_DEV_ENET_PHY_INT);
  gpio_dev_map(GPIO_29, GPIO_DEV_ENET_MDC);
  gpio_dev_map(GPIO_32, GPIO_DEV_ENET_MDIO);
  gpio_dev_map(GPIO_33, GPIO_DEV_ENET_RXD0);
  gpio_dev_map(GPIO_34, GPIO_DEV_ENET_RXD1);
  gpio_dev_map(GPIO_35, GPIO_DEV_ENET_RXDV);
  gpio_dev_map(GPIO_36, GPIO_DEV_ENET_TXD0);
  gpio_dev_map(GPIO_37, GPIO_DEV_ENET_TXD1);
  gpio_dev_map(GPIO_38, GPIO_DEV_ENET_TXEN);
  gpio_dev_map(GPIO_39, GPIO_DEV_ENET_REF_CLK);
#elif defined(CONFIG_ETH_PIN_GROUP1)
  // group2(46-55)
  gpio_dev_unmap(GPIO_46);  // PHY INT
  gpio_dev_unmap(GPIO_47);  // MDC
  gpio_dev_unmap(GPIO_48);  // MDIO
  gpio_dev_unmap(GPIO_49);  // RXD[0]
  gpio_dev_unmap(GPIO_50);  // RXD[1]
  gpio_dev_unmap(GPIO_51);  // RXDV
  gpio_dev_unmap(GPIO_52);  // TXD[0]
  gpio_dev_unmap(GPIO_53);  // TXD[1]
  gpio_dev_unmap(GPIO_54);  // TXEN
  gpio_dev_unmap(GPIO_55);  // REF_CLK

  gpio_dev_map(GPIO_46, GPIO_DEV_ENET_PHY_INT);
  gpio_dev_map(GPIO_47, GPIO_DEV_ENET_MDC);
  gpio_dev_map(GPIO_48, GPIO_DEV_ENET_MDIO);
  gpio_dev_map(GPIO_49, GPIO_DEV_ENET_RXD0);
  gpio_dev_map(GPIO_50, GPIO_DEV_ENET_RXD1);
  gpio_dev_map(GPIO_51, GPIO_DEV_ENET_RXDV);
  gpio_dev_map(GPIO_52, GPIO_DEV_ENET_TXD0);
  gpio_dev_map(GPIO_53, GPIO_DEV_ENET_TXD1);
  gpio_dev_map(GPIO_54, GPIO_DEV_ENET_TXEN);
  gpio_dev_map(GPIO_55, GPIO_DEV_ENET_REF_CLK);
#endif

  LWIP_LOGI("ETH SoftRest\n");
  // ETH soft reset
  SET_BIT(ETH_RESET_CTRL, 0x1);

  // ETH bypass clockgate
  WRITE_REG(ETH_RESET_CTRL, 0x3);

  // ETH CLK enable
  sys_ll_set_reserver_reg0xd_enet_cken(1);

  // Register ETH interrupt ISR
  bk_int_isr_register(INT_SRC_ETH, ETH_IRQHandler, NULL);

  // Enable ETH IRQ
  sys_hal_enable_eth_int();
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* ethHandle)
{
  if(ethHandle->Instance==ETH)
  {
#ifdef ETH_NOT_IMPLEMENTED
    /* Disable Peripheral clock */
    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11|GPIO_PIN_13);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(ETH_IRQn);
#endif
  }
}

/*******************************************************************************
                       PHI IO Functions
*******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t ETH_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
static int32_t ETH_PHY_IO_GetTick(void)
{
  return xTaskGetTickCount();
}

/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param  argument: netif
  * @retval None
  */
void ethernet_link_thread(void *argument)
{
  ETH_MACConfigTypeDef MACConf = {0};
  int32_t PHYLinkState = 0;
  uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;

  struct netif *netif = (struct netif *) argument;

  /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
  // LOCK_TCPIP_CORE();

  /* ETH link init */

  for(;;)
  {
    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

    if(netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
      LWIP_LOGI("ETH link down\n");
      HAL_ETH_Stop_IT(&heth);

      // stop dynamic address or static ip address
      eth_ip_down();  // XXX: netifapi_XXX
    }
    else if(!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
      switch (PHYLinkState)
      {
      case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      default:
        break;
      }

      if(linkchanged)
      {
        LWIP_LOGI("ETH link up, speed %d, duplex %d\n", speed, duplex);

        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&heth, &MACConf);
        MACConf.DuplexMode = duplex;
        MACConf.Speed = speed;
        HAL_ETH_SetMACConfig(&heth, &MACConf);
        HAL_ETH_Start_IT(&heth);
        netifapi_netif_set_link_up(netif);

        eth_ip_start();   // netifapi_XXX
      }
    }

    /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
    // UNLOCK_TCPIP_CORE();
    rtos_delay_milliseconds(200);
    // LOCK_TCPIP_CORE();
  }
}


void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;
    /* Initialize the struct pbuf.
    * This must be performed whenever a buffer's allocated because it may be
    * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
    RxAllocStatus = RX_ALLOC_ERROR;
    *buff = NULL;
  }
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

  /* Invalidate data cache because Rx DMA's writing to physical memory makes it stale. */
#if 0
  SCB_InvalidateDCache_by_Addr((uint32_t *)buff, Length);
#else
#if CONFIG_CACHE_ENABLE
  flush_dcache(buff, Length);
#endif
#endif
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
  pbuf_free((struct pbuf *)buff);
}

void ETH_DumpAllRegisters(void)
{
  HAL_ETH_DumpAllRegisters(&heth);
}

#endif /* CONFIG_ETH */

